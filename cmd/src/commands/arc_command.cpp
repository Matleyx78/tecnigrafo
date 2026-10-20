// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/arc_command.hpp"

#include "tg/core/arc.hpp"
#include "tg/core/math.hpp"

#include <cmath>
#include <variant>

namespace tg::cmd {
namespace {

constexpr double kAngleTolerance = 1e-12;

bool validPoint(const core::Vec2& point) {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

}  // namespace

void ArcCommand::start(CommandContext& context) {
    m_stage = Stage::Start;
    m_start = {};
    m_second = {};
    m_pointer.reset();
    m_preview.clear();
    context.editor.beginTransaction("Arc");
}

Status ArcCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (m_stage == Stage::Second && keyword->keyword == "Center") {
            m_stage = Stage::Center;
        }
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        context.editor.rollback();
        return Status::Finished;
    } else if (const auto* point = std::get_if<PointInput>(&input)) {
        if (!validPoint(point->point)) {
            return Status::Continue;
        }
        switch (m_stage) {
            case Stage::Start:
                m_start = point->point;
                context.lastPoint = point->point;
                m_stage = Stage::Second;
                break;
            case Stage::Second:
                m_second = point->point;
                context.lastPoint = point->point;
                m_stage = Stage::Third;
                break;
            case Stage::Center:
                if (!(core::distance(m_start, point->point) > 0.0)) {
                    context.message("Center coincides with the start point.");
                    break;
                }
                m_second = point->point;
                context.lastPoint = point->point;
                m_stage = Stage::End;
                break;
            case Stage::Third:
            case Stage::End:
                if (const auto arc = arcAt(point->point)) {
                    return finish(context, *arc, point->point);
                }
                context.message(m_stage == Stage::Third ? "The three points are collinear."
                                                        : "End point gives a zero-length arc.");
                break;
        }
    }
    updatePreview();
    return Status::Continue;
}

void ArcCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt ArcCommand::prompt() const {
    switch (m_stage) {
        case Stage::Start:
            return {"Specify start point of arc:", {}};
        case Stage::Second:
            return {"Specify second point of arc or [Center]:", {{"Center", "C"}}};
        case Stage::Third:
        case Stage::End:
            return {"Specify end point of arc:", {}};
        case Stage::Center:
            return {"Specify center point of arc:", {}};
    }
    return {};
}

std::span<const core::Entity> ArcCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> ArcCommand::basePoint() const {
    switch (m_stage) {
        case Stage::Start:
            return std::nullopt;
        case Stage::Second:
        case Stage::Center:
            return m_start;
        case Stage::Third:
            return m_second;
        case Stage::End:
            return m_second;
    }
    return std::nullopt;
}

std::optional<core::Arc> ArcCommand::arcAt(const core::Vec2& point) const {
    if (!validPoint(point)) {
        return std::nullopt;
    }
    if (m_stage == Stage::Third) {
        return core::arcThroughPoints(m_start, m_second, point);
    }
    if (m_stage != Stage::End) {
        return std::nullopt;
    }
    const core::Vec2 startDirection = m_start - m_second;
    const core::Vec2 endDirection = point - m_second;
    const double radius = startDirection.length();
    if (!(radius > 0.0) || !(endDirection.length() > 0.0) || !std::isfinite(radius)) {
        return std::nullopt;
    }
    const double start = core::normalizeAngle(std::atan2(startDirection.y, startDirection.x));
    const double end = core::normalizeAngle(std::atan2(endDirection.y, endDirection.x));
    const double sweep = core::normalizeAngle(end - start);
    if (sweep <= kAngleTolerance || core::kTwoPi - sweep <= kAngleTolerance) {
        return std::nullopt;
    }
    return core::Arc{m_second, radius, start, end};
}

Status ArcCommand::finish(CommandContext& context, const core::Arc& arc, const core::Vec2& end) {
    context.editor.add(arc);
    context.editor.commit();
    context.lastPoint = end;
    m_preview.clear();
    return Status::Finished;
}

void ArcCommand::updatePreview() {
    m_preview.clear();
    if (m_pointer) {
        if (const auto arc = arcAt(*m_pointer)) {
            m_preview.emplace_back(*arc);
        }
    }
}

}  // namespace tg::cmd
