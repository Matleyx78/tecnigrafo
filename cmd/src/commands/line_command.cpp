// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/line_command.hpp"

#include <variant>

namespace tg::cmd {
namespace {

// Drawing units, matching PLINE and the coordinate resolver.
constexpr double kPointTolerance = 1e-9;

}  // namespace

void LineCommand::start(CommandContext& context) {
    m_points.clear();
    m_segments.clear();
    m_pointer.reset();
    m_preview.clear();
    m_previousPoint = context.lastPoint;
    context.editor.beginTransaction("Line");
}

Status LineCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* point = std::get_if<PointInput>(&input)) {
        if (!m_points.empty()) {
            if (core::nearlyEqual(m_points.back(), point->point, kPointTolerance)) {
                context.message("Point coincides with the previous point.");
                return Status::Continue;
            }
            m_segments.push_back(context.editor.add(core::Line{m_points.back(), point->point}));
        }
        m_points.push_back(point->point);
        context.lastPoint = point->point;
        updatePreview();
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        return finish(context);
    } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (keyword->keyword == "Close" && m_points.size() >= 3) {
            // Returning explicitly to the first point has already closed
            // the chain; Close must not add a degenerate segment.
            if (!core::nearlyEqual(m_points.back(), m_points.front(), kPointTolerance)) {
                m_segments.push_back(
                    context.editor.add(core::Line{m_points.back(), m_points.front()}));
            }
            context.lastPoint = m_points.front();
            return finish(context);
        }
        if (keyword->keyword == "Undo" && !m_points.empty()) {
            if (!m_segments.empty()) {
                context.editor.remove(m_segments.back());
                m_segments.pop_back();
            }
            m_points.pop_back();
            context.lastPoint =
                m_points.empty() ? m_previousPoint : std::optional<core::Vec2>{m_points.back()};
            updatePreview();
        }
    }
    return Status::Continue;
}

void LineCommand::cancel(CommandContext& context) {
    finish(context);
}

void LineCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt LineCommand::prompt() const {
    if (m_points.empty()) {
        return {"Specify first point:", {}};
    }
    if (m_points.size() < 3) {
        return {"Specify next point or [Undo]:", {{"Undo", "U"}}};
    }
    return {"Specify next point or [Close/Undo]:", {{"Close", "C"}, {"Undo", "U"}}};
}

std::span<const core::Entity> LineCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> LineCommand::basePoint() const {
    return m_points.empty() ? std::nullopt : std::optional<core::Vec2>{m_points.back()};
}

Status LineCommand::finish(CommandContext& context) {
    if (m_segments.empty()) {
        // Undo may have removed every segment: do not create an empty step
        // or erase the redo history with a net-zero transaction.
        context.editor.rollback();
    } else {
        context.editor.commit();
    }
    return Status::Finished;
}

void LineCommand::updatePreview() {
    m_preview.clear();
    if (!m_points.empty() && m_pointer &&
        !core::nearlyEqual(m_points.back(), *m_pointer, kPointTolerance)) {
        m_preview.emplace_back(core::Line{m_points.back(), *m_pointer});
    }
}

}  // namespace tg::cmd
