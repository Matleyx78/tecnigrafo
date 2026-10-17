// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/pline_command.hpp"

#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

// Drawing-unit tolerance, consistent with the coordinate resolver's test
// for a pointer coincident with the last point.
constexpr double kPointTolerance = 1e-9;

}  // namespace

void PlineCommand::start(CommandContext& context) {
    m_vertices.clear();
    m_pointer.reset();
    m_preview.clear();
    m_previousPoint = context.lastPoint;
    context.editor.beginTransaction("Polyline");
}

Status PlineCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* point = std::get_if<PointInput>(&input)) {
        if (!m_vertices.empty() &&
            core::nearlyEqual(m_vertices.back().position, point->point, kPointTolerance)) {
            context.message("Point coincides with the previous point.");
            return Status::Continue;
        }
        m_vertices.push_back({point->point, 0.0});
        context.lastPoint = point->point;
        updatePreview();
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        return finish(context, false);
    } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if (keyword->keyword == "Close" && m_vertices.size() >= 3) {
            return finish(context, true);
        }
        if (keyword->keyword == "Undo" && !m_vertices.empty()) {
            m_vertices.pop_back();
            // Undoing all vertices restores the reference that preceded PLINE.
            context.lastPoint = m_vertices.empty()
                                    ? m_previousPoint
                                    : std::optional<core::Vec2>{m_vertices.back().position};
            updatePreview();
        }
    }
    // CancelInput belongs to CommandRunner, which rolls back the transaction.
    return Status::Continue;
}

void PlineCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt PlineCommand::prompt() const {
    if (m_vertices.empty()) {
        return {"Specify start point:", {}};
    }
    if (m_vertices.size() < 3) {
        return {"Specify next point or [Undo]:", {{"Undo", "U"}}};
    }
    return {"Specify next point or [Close/Undo]:", {{"Close", "C"}, {"Undo", "U"}}};
}

std::span<const core::Entity> PlineCommand::preview() const {
    return m_preview;
}

std::optional<core::Vec2> PlineCommand::basePoint() const {
    if (m_vertices.empty()) {
        return std::nullopt;
    }
    return m_vertices.back().position;
}

Status PlineCommand::finish(CommandContext& context, bool closed) {
    if (m_vertices.size() >= 2) {
        context.editor.add(core::Polyline{m_vertices, closed});
    }
    context.editor.commit();
    return Status::Finished;
}

void PlineCommand::updatePreview() {
    m_preview.clear();
    if (m_vertices.empty()) {
        return;
    }
    core::Polyline polyline{m_vertices};
    if (m_pointer && !core::nearlyEqual(m_vertices.back().position, *m_pointer, kPointTolerance)) {
        polyline.addVertex(*m_pointer);
    }
    m_preview.emplace_back(std::move(polyline));
}

}  // namespace tg::cmd
