// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/circle_command.hpp"

#include "tg/cmd/value_format.hpp"
#include "tg/core/fillet.hpp"

#include <cmath>
#include <numeric>
#include <variant>

namespace tg::cmd {
namespace {

bool validCircle(const core::Circle& circle) {
    return circle.radius > 0.0 && std::isfinite(circle.radius) && std::isfinite(circle.center.x) &&
           std::isfinite(circle.center.y);
}

}  // namespace

void CircleCommand::start(CommandContext& context) {
    m_stage = Stage::Center;
    m_first = {};
    m_second = {};
    m_defaultRadius = context.defaults.circleRadius;
    m_pointer.reset();
    m_preview.clear();
    context.editor.beginTransaction("Circle");
}

Status CircleCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (m_stage == Stage::Center && keyword->keyword == "2P") {
            m_stage = Stage::TwoFirst;
        } else if (m_stage == Stage::Center && keyword->keyword == "3P") {
            m_stage = Stage::ThreeFirst;
        } else if (m_stage == Stage::Center && keyword->keyword == "Ttr (tan tan radius)") {
            m_stage = Stage::TangentFirst;
        } else if (m_stage == Stage::Radius && keyword->keyword == "Diameter") {
            m_stage = Stage::Diameter;
        }
    } else if (const auto* object = std::get_if<ObjectInput>(&input)) {
        if (m_stage == Stage::TangentFirst) {
            m_firstTangent = *object;
            m_stage = Stage::TangentSecond;
        } else if (m_stage == Stage::TangentSecond) {
            m_secondTangent = *object;
            m_stage = Stage::TangentRadius;
        }
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        if (m_stage == Stage::TangentRadius && m_defaultRadius) {
            return finishTangent(context, *m_defaultRadius);
        }
        if ((m_stage == Stage::Radius || m_stage == Stage::Diameter) && m_defaultRadius) {
            return finish(context, {m_first, *m_defaultRadius});
        }
        context.editor.rollback();
        return Status::Finished;
    } else if (const auto* value = std::get_if<ValueInput>(&input)) {
        if (m_stage == Stage::TangentRadius) {
            return finishTangent(context, value->value);
        }
        if (m_stage == Stage::Radius || m_stage == Stage::Diameter) {
            return finish(
                context, {m_first, m_stage == Stage::Diameter ? value->value / 2.0 : value->value});
        }
    } else if (const auto* point = std::get_if<PointInput>(&input)) {
        switch (m_stage) {
            case Stage::Center:
            case Stage::TwoFirst:
            case Stage::ThreeFirst:
                m_first = point->point;
                context.lastPoint = point->point;
                if (m_stage == Stage::Center) {
                    m_stage = Stage::Radius;
                } else if (m_stage == Stage::TwoFirst) {
                    m_stage = Stage::TwoSecond;
                } else {
                    m_stage = Stage::ThreeSecond;
                }
                break;
            case Stage::ThreeSecond:
                m_second = point->point;
                context.lastPoint = point->point;
                m_stage = Stage::ThreeThird;
                break;
            case Stage::Radius:
            case Stage::Diameter:
            case Stage::TwoSecond:
            case Stage::ThreeThird:
                if (const auto circle = circleAt(point->point)) {
                    return finish(context, *circle);
                }
                context.message("The three points are collinear.");
                break;
            case Stage::TangentRadius:
                m_first = point->point;
                context.lastPoint = point->point;
                m_stage = Stage::TangentRadiusPoint;
                break;
            case Stage::TangentRadiusPoint:
                return finishTangent(context, core::distance(m_first, point->point));
            case Stage::TangentFirst:
            case Stage::TangentSecond:
                break;
        }
    }
    updatePreview();
    return Status::Continue;
}

void CircleCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt CircleCommand::prompt() const {
    switch (m_stage) {
        case Stage::Center:
            return {"Specify center point for circle or [3P/2P/Ttr (tan tan radius)]:",
                    {{"3P", "3P"},
                     {"2P", "2P"},
                     {"Ttr (tan tan radius)", "T"},
                     {"Ttr (tan tan radius)", "Ttr"}}};
        case Stage::Radius:
            return {m_defaultRadius ? "Specify radius of circle or [Diameter] <" +
                                          formatLength(*m_defaultRadius) + ">:"
                                    : "Specify radius of circle or [Diameter]:",
                    {{"Diameter", "D"}},
                    PromptKind::Distance};
        case Stage::Diameter:
            return {m_defaultRadius ? "Specify diameter of circle <" +
                                          formatLength(*m_defaultRadius * 2.0) + ">:"
                                    : "Specify diameter of circle:",
                    {},
                    PromptKind::Distance};
        case Stage::TwoFirst:
            return {"Specify first end point of circle's diameter:", {}};
        case Stage::TwoSecond:
            return {"Specify second end point of circle's diameter:", {}};
        case Stage::ThreeFirst:
            return {"Specify first point on circle:", {}};
        case Stage::ThreeSecond:
            return {"Specify second point on circle:", {}};
        case Stage::ThreeThird:
            return {"Specify third point on circle:", {}};
        case Stage::TangentFirst:
            return {"Specify point on object for first tangent of circle:", {}, PromptKind::Object};
        case Stage::TangentSecond:
            return {
                "Specify point on object for second tangent of circle:", {}, PromptKind::Object};
        case Stage::TangentRadius:
            if (m_defaultRadius) {
                return {"Specify radius of circle <" + formatLength(*m_defaultRadius) + ">:",
                        {},
                        PromptKind::Distance};
            }
            return {"Specify radius of circle:", {}, PromptKind::Distance};
        case Stage::TangentRadiusPoint:
            return {"Specify second point:", {}};
    }
    return {};
}

std::span<const core::Entity> CircleCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> CircleCommand::basePoint() const {
    switch (m_stage) {
        case Stage::Radius:
        case Stage::Diameter:
        case Stage::TwoSecond:
        case Stage::ThreeSecond:
        case Stage::TangentRadiusPoint:
            return m_first;
        case Stage::ThreeThird:
            return m_second;
        default:
            return std::nullopt;
    }
}

std::optional<core::Circle> CircleCommand::circleAt(const core::Vec2& point) const {
    switch (m_stage) {
        case Stage::Radius:
            return core::Circle{m_first, core::distance(m_first, point)};
        case Stage::Diameter:
            return core::Circle{m_first, core::distance(m_first, point) / 2.0};
        case Stage::TwoSecond:
            return core::Circle{
                {std::midpoint(m_first.x, point.x), std::midpoint(m_first.y, point.y)},
                core::distance(m_first, point) / 2.0};
        case Stage::ThreeThird:
            return core::circleThroughPoints(m_first, m_second, point);
        default:
            return std::nullopt;
    }
}

Status CircleCommand::finish(CommandContext& context, const core::Circle& circle) {
    if (!validCircle(circle)) {
        context.message("Value must be positive and nonzero.");
        return Status::Continue;
    }
    context.editor.add(circle);
    context.editor.commit();
    context.defaults.circleRadius = circle.radius;
    context.lastPoint = circle.center;
    m_preview.clear();
    return Status::Finished;
}

Status CircleCommand::finishTangent(CommandContext& context, double radius) {
    if (!(radius > 0.0) || !std::isfinite(radius)) {
        context.message("Value must be positive and nonzero.");
        m_stage = Stage::TangentRadius;
        return Status::Continue;
    }

    const core::Document& document = context.editor.document();
    const core::EntityRecord* first = document.find(m_firstTangent.id);
    const core::EntityRecord* second = document.find(m_secondTangent.id);
    core::TangentCircleResult result;
    if (first != nullptr && second != nullptr) {
        result = core::circleTangentTangentRadius(first->entity, m_firstTangent.point,
                                                  second->entity, m_secondTangent.point, radius);
    }
    if (result.circle) {
        return finish(context, *result.circle);
    }
    context.message(result.outcome == core::CornerOutcome::OutOfDomain
                        ? "Result is beyond the drawing limits."
                        : "Circle does not exist.");
    context.editor.rollback();
    return Status::Finished;
}

void CircleCommand::updatePreview() {
    m_preview.clear();
    if (m_pointer) {
        if (const auto circle = circleAt(*m_pointer); circle && validCircle(*circle)) {
            m_preview.emplace_back(*circle);
        }
    }
}

}  // namespace tg::cmd
