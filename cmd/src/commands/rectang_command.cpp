// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/rectang_command.hpp"

#include "tg/cmd/value_format.hpp"
#include "tg/core/entity_record.hpp"
#include "tg/core/math.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <string>
#include <system_error>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

constexpr double kBulgeAngle = core::kPi / 8.0;

/// The shortest text that reads back as `value`, for a DXF group.
std::string groupNumber(double value) {
    std::array<char, 32> buffer{};
    const auto [end, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    return error == std::errc{} ? std::string(buffer.data(), end) : std::string{"0"};
}

bool validNonnegative(double value) {
    return std::isfinite(value) && value >= 0.0;
}

bool validPositive(double value) {
    return std::isfinite(value) && value > 0.0;
}

core::Vec2 scaled(const core::Vec2& value, double amount) {
    return value * amount;
}

}  // namespace

CommandDescription RectangCommand::description() {
    return {
        "RECTANG",
        {"REC", "RECTANGLE"},
        "Draws a rectangular closed polyline.",
        false,
        {{"first-corner",
          "Specify first corner point or [Chamfer/Elevation/Fillet/Thickness/Width]:",
          ValueType::Point,
          {{"Chamfer", "C"},
           {"Elevation", "E"},
           {"Fillet", "F"},
           {"Thickness", "T"},
           {"Width", "W"}},
          false},
         {"other-corner",
          "Specify other corner point or [Area/Dimensions/Rotation]:",
          ValueType::Point,
          {{"Area", "A"}, {"Dimensions", "D"}, {"Rotation", "R"}},
          false},
         {"first-chamfer",
          "Specify first chamfer distance for rectangles <{default}>:",
          ValueType::Distance,
          {},
          true},
         {"second-chamfer",
          "Specify second chamfer distance for rectangles <{default}>:",
          ValueType::Distance,
          {},
          true},
         {"elevation",
          "Specify the elevation for rectangles <{default}>:",
          ValueType::Real,
          {},
          true},
         {"fillet",
          "Specify fillet radius for rectangles <{default}>:",
          ValueType::Distance,
          {},
          true},
         {"thickness", "Specify thickness for rectangles <{default}>:", ValueType::Real, {}, true},
         {"width", "Specify line width for rectangles <{default}>:", ValueType::Distance, {}, true},
         {"area",
          "Enter area of rectangle in current units <{default}>:",
          ValueType::Real,
          {},
          true},
         {"area-basis",
          "Calculate rectangle dimensions based on [Length/Width] <Length>:",
          ValueType::Keyword,
          {{"Length", "L"}, {"Width", "W"}},
          true},
         {"area-length", "Enter rectangle length <{default}>:", ValueType::Distance, {}, true},
         {"area-width", "Enter rectangle width <{default}>:", ValueType::Distance, {}, true},
         {"dimension-length",
          "Specify length for rectangles <{default}>:",
          ValueType::Distance,
          {},
          true},
         {"dimension-width",
          "Specify width for rectangles <{default}>:",
          ValueType::Distance,
          {},
          true},
         {"rotation",
          "Specify rotation angle or [Pick points] <{default}>:",
          ValueType::Angle,
          {{"Pick points", "P"}},
          true},
         {"rotation-first", "Specify first point:", ValueType::Point, {}, false},
         {"rotation-second", "Specify second point:", ValueType::Point, {}, false}}};
}

void RectangCommand::start(CommandContext& context) {
    m_stage = Stage::FirstCorner;
    m_defaults = &context.defaults;
    m_pointer.reset();
    m_preview.clear();
    m_dimensions = false;
    context.editor.beginTransaction("Rectang");
    std::string modes;
    const auto append = [&modes](const std::string& value) {
        modes += modes.empty() ? "" : "  ";
        modes += value;
    };
    if (!m_defaults->rectangUseFillet &&
        (m_defaults->rectangChamferFirst != 0.0 || m_defaults->rectangChamferSecond != 0.0)) {
        append("Chamfer=" + formatLength(m_defaults->rectangChamferFirst) + " x " +
               formatLength(m_defaults->rectangChamferSecond));
    }
    if (m_defaults->rectangUseFillet && m_defaults->rectangFilletRadius != 0.0) {
        append("Fillet=" + formatLength(m_defaults->rectangFilletRadius));
    }
    if (m_defaults->rectangWidth != 0.0) {
        append("Width=" + formatLength(m_defaults->rectangWidth));
    }
    if (m_defaults->rectangElevation != 0.0) {
        append("Elevation=" + formatLength(m_defaults->rectangElevation));
    }
    if (m_defaults->rectangThickness != 0.0) {
        append("Thickness=" + formatLength(m_defaults->rectangThickness));
    }
    if (m_defaults->rectangRotation != 0.0) {
        append("Rotation=" + formatAngle(m_defaults->rectangRotation));
    }
    if (!modes.empty()) {
        context.message("Current rectangle modes:  " + modes);
    }
}

Status RectangCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (m_stage == Stage::FirstCorner) {
            if (keyword->keyword == "Chamfer") {
                m_stage = Stage::ChamferFirst;
            }
            if (keyword->keyword == "Elevation") {
                m_stage = Stage::Elevation;
            }
            if (keyword->keyword == "Fillet") {
                m_stage = Stage::Fillet;
            }
            if (keyword->keyword == "Thickness") {
                m_stage = Stage::Thickness;
            }
            if (keyword->keyword == "Width") {
                m_stage = Stage::Width;
            }
        } else if (m_stage == Stage::OtherCorner) {
            if (keyword->keyword == "Area") {
                m_stage = Stage::Area;
            }
            if (keyword->keyword == "Dimensions") {
                m_dimensions = true;
                m_stage = Stage::DimensionLength;
            }
            if (keyword->keyword == "Rotation") {
                m_stage = Stage::Rotation;
            }
        } else if (m_stage == Stage::AreaBasis) {
            m_stage = keyword->keyword == "Width" ? Stage::AreaWidth : Stage::AreaLength;
        } else if (m_stage == Stage::Rotation && keyword->keyword == "Pick points") {
            m_stage = Stage::RotationFirst;
        }
        return Status::Continue;
    }

    if (const auto* point = std::get_if<PointInput>(&input)) {
        context.lastPoint = point->point;
        if (m_stage == Stage::FirstCorner) {
            m_first = point->point;
            m_stage = Stage::OtherCorner;
        } else if (m_stage == Stage::OtherCorner) {
            auto [dx, dy] = projected(point->point);
            if (m_dimensions) {
                dx = std::copysign(m_defaults->rectangLength, dx);
                dy = std::copysign(m_defaults->rectangOtherWidth, dy);
            }
            return finish(context, dx, dy);
        } else if (m_stage == Stage::RotationFirst) {
            m_rotationFirst = point->point;
            m_stage = Stage::RotationSecond;
        } else if (m_stage == Stage::RotationSecond) {
            const core::Vec2 direction = point->point - m_rotationFirst;
            if (direction.length() > 0.0) {
                m_defaults->rectangRotation = core::atan2(direction.y, direction.x);
                m_stage = Stage::OtherCorner;
            }
        }
        return Status::Continue;
    }

    const auto assign = [](double& target, double value, bool nonnegative) {
        if ((nonnegative && validNonnegative(value)) || (!nonnegative && std::isfinite(value))) {
            target = value;
            return true;
        }
        return false;
    };
    const auto* value = std::get_if<ValueInput>(&input);
    const bool confirm = std::holds_alternative<ConfirmInput>(input);
    const double entered = value != nullptr ? value->value : 0.0;
    switch (m_stage) {
        case Stage::ChamferFirst:
            if (confirm || assign(m_defaults->rectangChamferFirst, entered, true)) {
                if (value != nullptr && entered != 0.0) {
                    m_defaults->rectangChamferSecond = entered;
                }
                m_stage = Stage::ChamferSecond;
            }
            break;
        case Stage::ChamferSecond:
            if (confirm || assign(m_defaults->rectangChamferSecond, entered, true)) {
                m_defaults->rectangUseFillet = false;
                m_stage = Stage::FirstCorner;
            }
            break;
        case Stage::Elevation:
            if (confirm || assign(m_defaults->rectangElevation, entered, false)) {
                m_stage = Stage::FirstCorner;
            }
            break;
        case Stage::Fillet:
            if (confirm) {
                m_defaults->rectangFilletRadius = m_defaults->rectangChamferFirst;
            }
            if (confirm || assign(m_defaults->rectangFilletRadius, entered, true)) {
                m_defaults->rectangChamferFirst = m_defaults->rectangFilletRadius;
                m_defaults->rectangChamferSecond = m_defaults->rectangFilletRadius;
                m_defaults->rectangUseFillet = true;
                m_stage = Stage::FirstCorner;
            }
            break;
        case Stage::Thickness:
            if (confirm || assign(m_defaults->rectangThickness, entered, false)) {
                m_stage = Stage::FirstCorner;
            }
            break;
        case Stage::Width:
            if (confirm || assign(m_defaults->rectangWidth, entered, true)) {
                m_stage = Stage::FirstCorner;
            }
            break;
        case Stage::Area:
            if (confirm || (value != nullptr && validPositive(entered))) {
                if (value != nullptr) {
                    m_area = entered;
                }
                m_stage = Stage::AreaBasis;
            }
            break;
        case Stage::AreaBasis:
            if (confirm) {
                m_stage = Stage::AreaLength;
            }
            break;
        case Stage::AreaLength: {
            const double length = confirm ? m_defaults->rectangLength : entered;
            if (validPositive(length)) {
                m_defaults->rectangLength = length;
                m_defaults->rectangOtherWidth = m_area / length;
                return finish(context, length, m_defaults->rectangOtherWidth);
            }
            break;
        }
        case Stage::AreaWidth: {
            const double width = confirm ? m_defaults->rectangOtherWidth : entered;
            if (validPositive(width)) {
                m_defaults->rectangOtherWidth = width;
                m_defaults->rectangLength = m_area / width;
                return finish(context, m_defaults->rectangLength, width);
            }
            break;
        }
        case Stage::DimensionLength:
            if (confirm || (value != nullptr && validPositive(entered))) {
                if (value != nullptr) {
                    m_defaults->rectangLength = entered;
                }
                m_stage = Stage::DimensionWidth;
            }
            break;
        case Stage::DimensionWidth:
            if (confirm || (value != nullptr && validPositive(entered))) {
                if (value != nullptr) {
                    m_defaults->rectangOtherWidth = entered;
                }
                m_stage = Stage::OtherCorner;
            }
            break;
        case Stage::Rotation:
            if (confirm || (value != nullptr && std::isfinite(entered))) {
                if (value != nullptr) {
                    m_defaults->rectangRotation = entered;
                }
                m_stage = Stage::OtherCorner;
            }
            break;
        default:
            break;
    }
    return Status::Continue;
}

void RectangCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    if (m_stage == Stage::OtherCorner) {
        updatePreview(point);
    }
}

Prompt RectangCommand::prompt() const {
    const auto lengthPrompt = [](const std::string& text, double value, PromptKind kind) {
        return Prompt{text + formatLength(value) + ">:", {}, kind};
    };
    switch (m_stage) {
        case Stage::FirstCorner:
            return {"Specify first corner point or [Chamfer/Elevation/Fillet/Thickness/Width]:",
                    {{"Chamfer", "C"},
                     {"Elevation", "E"},
                     {"Fillet", "F"},
                     {"Thickness", "T"},
                     {"Width", "W"}}};
        case Stage::OtherCorner:
            return {"Specify other corner point or [Area/Dimensions/Rotation]:",
                    {{"Area", "A"}, {"Dimensions", "D"}, {"Rotation", "R"}}};
        case Stage::ChamferFirst:
            return lengthPrompt("Specify first chamfer distance for rectangles <",
                                m_defaults->rectangChamferFirst, PromptKind::Distance);
        case Stage::ChamferSecond:
            return lengthPrompt("Specify second chamfer distance for rectangles <",
                                m_defaults->rectangChamferSecond, PromptKind::Distance);
        case Stage::Elevation:
            return lengthPrompt("Specify the elevation for rectangles <",
                                m_defaults->rectangElevation, PromptKind::Number);
        case Stage::Fillet:
            return lengthPrompt("Specify fillet radius for rectangles <",
                                m_defaults->rectangUseFillet ? m_defaults->rectangFilletRadius
                                                             : m_defaults->rectangChamferFirst,
                                PromptKind::Distance);
        case Stage::Thickness:
            return lengthPrompt("Specify thickness for rectangles <", m_defaults->rectangThickness,
                                PromptKind::Number);
        case Stage::Width:
            return lengthPrompt("Specify line width for rectangles <", m_defaults->rectangWidth,
                                PromptKind::Distance);
        case Stage::Area:
            return lengthPrompt("Enter area of rectangle in current units <", m_area,
                                PromptKind::Number);
        case Stage::AreaBasis:
            return {"Calculate rectangle dimensions based on [Length/Width] <Length>:",
                    {{"Length", "L"}, {"Width", "W"}}};
        case Stage::AreaLength:
            return lengthPrompt("Enter rectangle length <", m_defaults->rectangLength,
                                PromptKind::Distance);
        case Stage::AreaWidth:
            return lengthPrompt("Enter rectangle width <", m_defaults->rectangOtherWidth,
                                PromptKind::Distance);
        case Stage::DimensionLength:
            return lengthPrompt("Specify length for rectangles <", m_defaults->rectangLength,
                                PromptKind::Distance);
        case Stage::DimensionWidth:
            return lengthPrompt("Specify width for rectangles <", m_defaults->rectangOtherWidth,
                                PromptKind::Distance);
        case Stage::Rotation:
            return {"Specify rotation angle or [Pick points] <" +
                        formatAngle(m_defaults->rectangRotation) + ">:",
                    {{"Pick points", "P"}},
                    PromptKind::Angle};
        case Stage::RotationFirst:
            return {"Specify first point:", {}};
        case Stage::RotationSecond:
            return {"Specify second point:", {}};
    }
    return {};
}

std::span<const core::Entity> RectangCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> RectangCommand::basePoint() const {
    if (m_stage == Stage::OtherCorner || m_stage == Stage::Area ||
        m_stage == Stage::DimensionLength || m_stage == Stage::Rotation) {
        return m_first;
    }
    if (m_stage == Stage::RotationSecond) {
        return m_rotationFirst;
    }
    return std::nullopt;
}

std::pair<double, double> RectangCommand::projected(const core::Vec2& point) const {
    const auto direction = core::cardinalSinCos(m_defaults->rectangRotation);
    const core::Vec2 u{direction.cosine, direction.sine};
    const core::Vec2 v{-direction.sine, direction.cosine};
    const core::Vec2 delta = point - m_first;
    return {delta.x * u.x + delta.y * u.y, delta.x * v.x + delta.y * v.y};
}

core::Polyline RectangCommand::rectangle(double dx, double dy) const {
    const auto direction = core::cardinalSinCos(m_defaults->rectangRotation);
    const core::Vec2 u{direction.cosine, direction.sine};
    const core::Vec2 v{-direction.sine, direction.cosine};
    const core::Vec2 ex = scaled(u, dx);
    const core::Vec2 ey = scaled(v, dy);
    const core::Vec2 sx = scaled(u, std::copysign(1.0, dx));
    const core::Vec2 sy = scaled(v, std::copysign(1.0, dy));
    const std::array<core::Vec2, 4> corners{m_first, m_first + ex, m_first + ex + ey, m_first + ey};
    std::vector<core::PolylineVertex> vertices;
    const double radius = m_defaults->rectangUseFillet ? m_defaults->rectangFilletRadius : 0.0;
    const bool filletFits = radius > 0.0 && 2.0 * radius <= std::min(std::abs(dx), std::abs(dy));
    const double first = m_defaults->rectangChamferFirst;
    const double second = m_defaults->rectangChamferSecond;
    const bool chamfer = radius == 0.0 && (first != 0.0 || second != 0.0);
    const bool chamferFits = chamfer && 2.0 * first <= std::abs(dy) && 2.0 * second <= std::abs(dx);
    if ((radius > 0.0 && !filletFits) || (chamfer && !chamferFits)) {
        for (const auto& corner : corners) {
            vertices.push_back({corner});
            vertices.push_back({corner});
        }
    } else if (filletFits) {
        const double bulge = std::copysign(core::tan(kBulgeAngle), dx * dy);
        const std::array<core::Vec2, 4> outgoing{sx, sy, sx * -1.0, sy * -1.0};
        const std::array<core::Vec2, 4> incoming{sy, sx * -1.0, sy * -1.0, sx};
        for (std::size_t i = 0; i < corners.size(); ++i) {
            vertices.push_back({corners.at(i) + incoming.at(i) * radius, bulge});
            vertices.push_back({corners.at(i) + outgoing.at(i) * radius});
        }
    } else if (chamferFits) {
        const std::array<core::Vec2, 4> outgoing{sx, sy, sx * -1.0, sy * -1.0};
        const std::array<core::Vec2, 4> incoming{sy, sx * -1.0, sy * -1.0, sx};
        for (std::size_t i = 0; i < corners.size(); ++i) {
            vertices.push_back({corners.at(i) + incoming.at(i) * first});
            vertices.push_back({corners.at(i) + outgoing.at(i) * second});
        }
    } else {
        for (const auto& corner : corners) {
            vertices.push_back({corner});
        }
    }
    core::Polyline polyline{std::move(vertices), true};
    polyline.setConstantWidth(m_defaults->rectangWidth);
    return polyline;
}

Status RectangCommand::finish(CommandContext& context, double dx, double dy) {
    core::EntityExtras extras;
    if (m_defaults->rectangWidth == 0.0) {
        extras.specific.push_back({43, "0.0"});
    }
    if (m_defaults->rectangElevation != 0.0) {
        extras.placement.push_back({38, groupNumber(m_defaults->rectangElevation)});
    }
    if (m_defaults->rectangThickness != 0.0) {
        extras.placement.push_back({39, groupNumber(m_defaults->rectangThickness)});
    }
    context.editor.addRecord({{}, rectangle(dx, dy), std::move(extras)});
    context.editor.commit();
    const auto direction = core::cardinalSinCos(m_defaults->rectangRotation);
    context.lastPoint = m_first + core::Vec2{direction.cosine * dx - direction.sine * dy,
                                             direction.sine * dx + direction.cosine * dy};
    m_preview.clear();
    return Status::Finished;
}

void RectangCommand::updatePreview(const core::Vec2& other) {
    const auto [dx, dy] = projected(other);
    m_preview.clear();
    m_preview.emplace_back(rectangle(dx, dy));
}

}  // namespace tg::cmd
