// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/polygon_command.hpp"

#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"

#include <cmath>
#include <string>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

constexpr int kMinimumSides = 3;
constexpr int kMaximumSides = 1024;

core::Vec2 direction(double angle) {
    const core::SinCos value = core::cardinalSinCos(angle);
    return {value.cosine, value.sine};
}

}  // namespace

CommandDescription PolygonCommand::description() {
    return {"POLYGON",
            {"POL"},
            "Draws a regular closed polygon.",
            false,
            {{"sides", "Enter number of sides <{default}>:", ValueType::Integer, {}, true},
             {"center", "Specify center of polygon or [Edge]:", ValueType::Point,
              {{"Edge", "E"}}, false},
             {"option",
              "Enter an option [Inscribed in circle/Circumscribed about circle] <{default}>:",
              ValueType::Keyword,
              {{"Inscribed in circle", "I"}, {"Circumscribed about circle", "C"}}, true},
             {"radius", "Specify radius of circle:", ValueType::Distance, {}, false},
             {"edge-first", "Specify first endpoint of edge:", ValueType::Point, {}, false},
             {"edge-second", "Specify second endpoint of edge:", ValueType::Point, {}, false}}};
}

void PolygonCommand::start(CommandContext& context) {
    m_stage = Stage::Sides;
    m_defaults = &context.defaults;
    m_pointer.reset();
    m_preview.clear();
    context.editor.beginTransaction("Polygon");
}

Status PolygonCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (m_stage == Stage::Center && keyword->keyword == "Edge") {
            m_stage = Stage::EdgeFirst;
        } else if (m_stage == Stage::Option) {
            m_defaults->polygonCircumscribed = keyword->keyword == "Circumscribed about circle";
            m_stage = Stage::Radius;
        }
        return Status::Continue;
    }
    if (const auto* value = std::get_if<ValueInput>(&input)) {
        if (m_stage == Stage::Sides) {
            const double sides = value->value;
            if (!std::isfinite(sides) || sides != std::floor(sides) || sides < kMinimumSides ||
                sides > kMaximumSides) {
                context.message("Requires an integer between 3 and 1024.");
            } else {
                m_defaults->polygonSides = static_cast<int>(sides);
                m_stage = Stage::Center;
            }
        } else if (m_stage == Stage::Radius) {
            const double angle = core::kPi / 2.0 +
                                 (m_defaults->polygonSides % 2 == 0
                                      ? core::kPi / m_defaults->polygonSides
                                      : 0.0);
            return finishRadius(context, value->value, angle);
        }
        return Status::Continue;
    }
    if (std::holds_alternative<ConfirmInput>(input)) {
        if (m_stage == Stage::Sides) {
            m_stage = Stage::Center;
        } else if (m_stage == Stage::Option) {
            m_stage = Stage::Radius;
        }
        return Status::Continue;
    }
    if (const auto* point = std::get_if<PointInput>(&input)) {
        context.lastPoint = point->point;
        switch (m_stage) {
            case Stage::Center:
                m_center = point->point;
                m_stage = Stage::Option;
                break;
            case Stage::Radius: {
                const core::Vec2 delta = point->point - m_center;
                double angle = core::atan2(delta.y, delta.x);
                if (m_defaults->polygonCircumscribed) {
                    angle += core::kPi / m_defaults->polygonSides;
                }
                return finishRadius(context, delta.length(), angle);
            }
            case Stage::EdgeFirst:
                m_edgeFirst = point->point;
                m_stage = Stage::EdgeSecond;
                break;
            case Stage::EdgeSecond:
                return finishEdge(context, point->point);
            case Stage::Sides:
            case Stage::Option:
                break;
        }
    }
    return Status::Continue;
}

void PolygonCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview(point);
}

Prompt PolygonCommand::prompt() const {
    switch (m_stage) {
        case Stage::Sides:
            return {"Enter number of sides <" + std::to_string(m_defaults->polygonSides) + ">:",
                    {}, PromptKind::Number};
        case Stage::Center:
            return {"Specify center of polygon or [Edge]:", {{"Edge", "E"}}};
        case Stage::Option:
            return {"Enter an option [Inscribed in circle/Circumscribed about circle] <" +
                        std::string(m_defaults->polygonCircumscribed ? "C" : "I") + ">:",
                    {{"Inscribed in circle", "I"}, {"Circumscribed about circle", "C"}},
                    PromptKind::Point};
        case Stage::Radius:
            return {"Specify radius of circle:", {}, PromptKind::Distance};
        case Stage::EdgeFirst:
            return {"Specify first endpoint of edge:", {}};
        case Stage::EdgeSecond:
            return {"Specify second endpoint of edge:", {}};
    }
    return {};
}

std::span<const core::Entity> PolygonCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> PolygonCommand::basePoint() const {
    if (m_stage == Stage::Radius) {
        return m_center;
    }
    if (m_stage == Stage::EdgeSecond) {
        return m_edgeFirst;
    }
    return std::nullopt;
}

Status PolygonCommand::finishRadius(CommandContext& context, double radius, double firstAngle) {
    if (!(radius > 0.0) || !std::isfinite(radius)) {
        context.message("Value must be positive and nonzero.");
        return Status::Continue;
    }
    if (m_defaults->polygonCircumscribed) {
        radius /= core::cos(core::kPi / m_defaults->polygonSides);
    }
    std::vector<core::PolylineVertex> vertices;
    vertices.reserve(static_cast<std::size_t>(m_defaults->polygonSides));
    const double step = 2.0 * core::kPi / m_defaults->polygonSides;
    for (int index = 0; index < m_defaults->polygonSides; ++index) {
        vertices.push_back({m_center + direction(firstAngle + step * index) * radius, 0.0});
    }
    context.editor.add(core::Polyline{std::move(vertices), true});
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

Status PolygonCommand::finishEdge(CommandContext& context, const core::Vec2& second) {
    const core::Vec2 edge = second - m_edgeFirst;
    if (!(edge.length() > 0.0) || !std::isfinite(edge.length())) {
        context.message("Points must be different.");
        return Status::Continue;
    }
    std::vector<core::PolylineVertex> vertices;
    vertices.reserve(static_cast<std::size_t>(m_defaults->polygonSides));
    vertices.push_back({m_edgeFirst, 0.0});
    vertices.push_back({second, 0.0});
    const double firstAngle = core::atan2(edge.y, edge.x);
    const double turn = 2.0 * core::kPi / m_defaults->polygonSides;
    for (int index = 1; index < m_defaults->polygonSides - 1; ++index) {
        vertices.push_back(
            {vertices.back().position + direction(firstAngle + turn * index) * edge.length(), 0.0});
    }
    context.editor.add(core::Polyline{std::move(vertices), true});
    context.editor.commit();
    m_preview.clear();
    return Status::Finished;
}

void PolygonCommand::updatePreview(const core::Vec2&) {
    m_preview.clear();
}

}  // namespace tg::cmd
