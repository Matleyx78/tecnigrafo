// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/ellipse_command.hpp"

#include "tg/core/ellipse.hpp"
#include "tg/core/math.hpp"

#include <cmath>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

constexpr double kMaximumRotation = 89.4 * core::kPi / 180.0;

bool validPoint(const core::Vec2& point) {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

double ellipseParameter(double direction, double ratio) {
    const core::SinCos value = core::sinCos(direction);
    return core::atan2(value.sine, value.cosine * ratio);
}

}  // namespace

CommandDescription EllipseCommand::description() {
    return {
        "ELLIPSE",
        {"EL"},
        "Draws an ellipse or an elliptical arc from its axes.",
        false,
        {{"axis-endpoint",
          "Specify axis endpoint of ellipse or [Arc/Center]:",
          ValueType::Point,
          {{"Arc", "A"}, {"Center", "C"}},
          false},
         {"arc-axis-endpoint",
          "Specify axis endpoint of elliptical arc or [Center]:",
          ValueType::Point,
          {{"Center", "C"}},
          false},
         {"other-axis-endpoint", "Specify other endpoint of axis:", ValueType::Point, {}, false},
         {"center", "Specify center of ellipse:", ValueType::Point, {}, false},
         {"arc-center", "Specify center of elliptical arc:", ValueType::Point, {}, false},
         {"axis-endpoint-from-center", "Specify endpoint of axis:", ValueType::Point, {}, false},
         {"other-axis-distance",
          "Specify distance to other axis or [Rotation]:",
          ValueType::Distance,
          {{"Rotation", "R"}},
          false},
         {"rotation", "Specify rotation around major axis:", ValueType::Angle, {}, false},
         {"start-angle", "Specify start angle:", ValueType::Angle, {}, false},
         {"end-angle",
          "Specify end angle or [Included angle]:",
          ValueType::Angle,
          {{"Included angle", "I"}},
          false},
         {"included-angle",
          "Specify included angle for arc <{default}>:",
          ValueType::Angle,
          {},
          true}}};
}

void EllipseCommand::start(CommandContext& context) {
    m_stage = Stage::FirstAxis;
    m_arc = false;
    m_centerMethod = false;
    m_first = {};
    m_center = {};
    m_axis = {};
    m_otherRadius = 0.0;
    m_startDirection = 0.0;
    m_ellipse = {};
    m_pointer.reset();
    m_preview.clear();
    context.editor.beginTransaction("Ellipse");
}

Status EllipseCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (m_stage == Stage::FirstAxis && keyword->keyword == "Arc") {
            m_arc = true;
        } else if (m_stage == Stage::FirstAxis && keyword->keyword == "Center") {
            m_centerMethod = true;
            m_stage = Stage::Center;
        } else if (m_stage == Stage::Distance && keyword->keyword == "Rotation") {
            m_ellipse = ellipseFor(m_axis.length());
            m_stage = Stage::Rotation;
        } else if (m_stage == Stage::EndAngle && keyword->keyword == "Included angle") {
            m_stage = Stage::IncludedAngle;
        }
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        if (m_stage == Stage::IncludedAngle) {
            return setAngle(context, core::kPi);
        }
        context.editor.rollback();
        return Status::Finished;
    } else if (const auto* value = std::get_if<ValueInput>(&input)) {
        if (m_stage == Stage::Distance && setSize(context, value->value)) {
            return afterShape(context);
        }
        if (m_stage == Stage::Rotation && setRotation(context, value->value)) {
            return afterShape(context);
        }
        if (m_stage == Stage::StartAngle || m_stage == Stage::EndAngle ||
            m_stage == Stage::IncludedAngle) {
            return setAngle(context, value->value);
        }
    } else if (const auto* point = std::get_if<PointInput>(&input);
               point != nullptr && validPoint(point->point)) {
        context.lastPoint = point->point;
        switch (m_stage) {
            case Stage::FirstAxis:
                m_first = point->point;
                m_stage = Stage::OtherAxis;
                break;
            case Stage::OtherAxis:
                if (setAxis(context, point->point)) {
                    m_stage = Stage::Distance;
                }
                break;
            case Stage::Center:
                m_center = point->point;
                m_stage = Stage::AxisEndpoint;
                break;
            case Stage::AxisEndpoint:
                if (setAxis(context, point->point)) {
                    m_stage = Stage::Distance;
                }
                break;
            case Stage::Distance:
                if (setSize(context, core::distance(m_center, point->point))) {
                    return afterShape(context);
                }
                break;
            case Stage::Rotation:
            case Stage::StartAngle:
            case Stage::EndAngle:
            case Stage::IncludedAngle:
                break;
        }
    }
    updatePreview();
    return Status::Continue;
}

void EllipseCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt EllipseCommand::prompt() const {
    switch (m_stage) {
        case Stage::FirstAxis:
            if (m_arc) {
                return {"Specify axis endpoint of elliptical arc or [Center]:", {{"Center", "C"}}};
            }
            return {"Specify axis endpoint of ellipse or [Arc/Center]:",
                    {{"Arc", "A"}, {"Center", "C"}}};
        case Stage::OtherAxis:
            return {"Specify other endpoint of axis:", {}};
        case Stage::Center:
            return {m_arc ? "Specify center of elliptical arc:" : "Specify center of ellipse:", {}};
        case Stage::AxisEndpoint:
            return {"Specify endpoint of axis:", {}};
        case Stage::Distance:
            return {"Specify distance to other axis or [Rotation]:",
                    {{"Rotation", "R"}},
                    PromptKind::Distance};
        case Stage::Rotation:
            return {"Specify rotation around major axis:", {}, PromptKind::Angle};
        case Stage::StartAngle:
            return {"Specify start angle:", {}, PromptKind::Angle};
        case Stage::EndAngle:
            return {"Specify end angle or [Included angle]:",
                    {{"Included angle", "I"}},
                    PromptKind::Angle};
        case Stage::IncludedAngle:
            return {"Specify included angle for arc <180>:", {}, PromptKind::Angle};
    }
    return {};
}

std::span<const core::Entity> EllipseCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> EllipseCommand::basePoint() const {
    switch (m_stage) {
        case Stage::OtherAxis:
            return m_first;
        case Stage::AxisEndpoint:
        case Stage::Distance:
            return m_center;
        default:
            return std::nullopt;
    }
}

bool EllipseCommand::setAxis(CommandContext& context, const core::Vec2& point) {
    if (m_centerMethod) {
        m_axis = point - m_center;
    } else {
        m_center = (m_first + point) * 0.5;
        m_axis = m_first - m_center;
    }
    if (!(m_axis.length() > 0.0) || !std::isfinite(m_axis.length())) {
        context.message("Axis length must be positive and nonzero.");
        return false;
    }
    return true;
}

bool EllipseCommand::setSize(CommandContext& context, double distance) {
    if (!(distance > 0.0) || !std::isfinite(distance)) {
        context.message("Distance must be positive and nonzero.");
        return false;
    }
    m_otherRadius = distance;
    m_ellipse = ellipseFor(distance);
    return true;
}

bool EllipseCommand::setRotation(CommandContext& context, double angle) {
    if (!std::isfinite(angle) || angle < 0.0 || angle > kMaximumRotation) {
        context.message("Rotation must be between 0 and 89.4 degrees.");
        return false;
    }
    return setSize(context, m_axis.length() * core::cos(angle));
}

Status EllipseCommand::afterShape(CommandContext& context) {
    if (!m_arc) {
        return finish(context);
    }
    m_stage = Stage::StartAngle;
    updatePreview();
    return Status::Continue;
}

Status EllipseCommand::setAngle(CommandContext& context, double angle) {
    if (!std::isfinite(angle)) {
        context.message("Angle must be finite.");
        return Status::Continue;
    }
    const double directionOffset = m_otherRadius > m_axis.length() ? core::kPi / 2.0 : 0.0;
    if (m_stage == Stage::StartAngle) {
        m_startDirection = angle;
        m_ellipse.startParameter =
            core::normalizeAngle(ellipseParameter(angle - directionOffset, m_ellipse.ratio));
        m_ellipse.endParameter = m_ellipse.startParameter + core::kTwoPi;
        m_stage = Stage::EndAngle;
        updatePreview();
        return Status::Continue;
    }
    const double endDirection = m_stage == Stage::IncludedAngle ? m_startDirection + angle : angle;
    double end = ellipseParameter(endDirection - directionOffset, m_ellipse.ratio);
    while (end <= m_ellipse.startParameter) {
        end += core::kTwoPi;
    }
    m_ellipse.endParameter = end;
    return finish(context);
}

Status EllipseCommand::finish(CommandContext& context) {
    context.editor.add(m_ellipse);
    context.editor.commit();
    context.lastPoint = m_center;
    m_preview.clear();
    return Status::Finished;
}

core::Ellipse EllipseCommand::ellipseFor(double otherRadius) const {
    const double axisRadius = m_axis.length();
    if (otherRadius <= axisRadius) {
        return {m_center, m_axis, otherRadius / axisRadius, 0.0, core::kTwoPi};
    }
    const core::Vec2 major{-m_axis.y * otherRadius / axisRadius,
                           m_axis.x * otherRadius / axisRadius};
    return {m_center, major, axisRadius / otherRadius, 0.0, core::kTwoPi};
}

void EllipseCommand::updatePreview() {
    m_preview.clear();
    if (!m_pointer || !validPoint(*m_pointer)) {
        return;
    }
    if (m_stage == Stage::OtherAxis) {
        const core::Vec2 center = (m_first + *m_pointer) * 0.5;
        const core::Vec2 axis = m_first - center;
        if (axis.length() > 0.0) {
            m_preview.emplace_back(core::Ellipse{center, axis, 1.0, 0.0, core::kTwoPi});
        }
    } else if (m_stage == Stage::AxisEndpoint) {
        const core::Vec2 axis = *m_pointer - m_center;
        if (axis.length() > 0.0) {
            m_preview.emplace_back(core::Ellipse{m_center, axis, 1.0, 0.0, core::kTwoPi});
        }
    } else if (m_stage == Stage::Distance) {
        const double distance = core::distance(m_center, *m_pointer);
        if (distance > 0.0 && std::isfinite(distance)) {
            m_preview.emplace_back(ellipseFor(distance));
        }
    } else if (m_stage == Stage::StartAngle || m_stage == Stage::EndAngle ||
               m_stage == Stage::IncludedAngle || m_stage == Stage::Rotation) {
        m_preview.emplace_back(m_ellipse);
    }
}

}  // namespace tg::cmd
