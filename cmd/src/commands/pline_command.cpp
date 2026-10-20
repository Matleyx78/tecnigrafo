// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/pline_command.hpp"

#include <cmath>
#include <numbers>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

// Drawing-unit tolerance, consistent with the coordinate resolver's test
// for a pointer coincident with the last point.
constexpr double kPointTolerance = 1e-9;
// ADR 0012 rejects arcs whose chord points backwards within this angle.
constexpr double kAngleTolerance = 1e-9;

}  // namespace

void PlineCommand::start(CommandContext& context) {
    m_arcMode = false;
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
        if (!m_vertices.empty()) {
            const auto bulge = segmentBulge(point->point);
            if (!bulge) {
                context.message("Invalid end point of arc.");
                return Status::Continue;
            }
            m_vertices.back().bulge = *bulge;
        }
        m_vertices.push_back({point->point, 0.0});
        context.lastPoint = point->point;
        updatePreview();
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        return finish(context, false);
    } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        if ((keyword->keyword == "Close" || keyword->keyword == "CLose") &&
            m_vertices.size() >= 3) {
            const auto bulge = segmentBulge(m_vertices.front().position);
            if (!bulge) {
                context.message("Invalid end point of arc.");
                return Status::Continue;
            }
            m_vertices.back().bulge = *bulge;
            return finish(context, true);
        }
        if (keyword->keyword == "Arc") {
            m_arcMode = true;
            updatePreview();
        } else if (keyword->keyword == "Line") {
            m_arcMode = false;
            updatePreview();
        }
        if (keyword->keyword == "Undo" && !m_vertices.empty()) {
            m_vertices.pop_back();
            if (!m_vertices.empty()) {
                m_vertices.back().bulge = 0.0;
            }
            // Undoing all vertices restores the reference that preceded PLINE.
            context.lastPoint = m_vertices.empty()
                                    ? m_previousPoint
                                    : std::optional<core::Vec2>{m_vertices.back().position};
            updatePreview();
        }
    }
    // CancelInput belongs to CommandRunner, which calls cancel().
    return Status::Continue;
}

void PlineCommand::cancel(CommandContext& context) {
    finish(context, false);
}

void PlineCommand::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    updatePreview();
}

Prompt PlineCommand::prompt() const {
    if (m_vertices.empty()) {
        return {"Specify start point:", {}};
    }
    if (m_arcMode) {
        if (m_vertices.size() < 3) {
            return {"Specify end point of arc or [Line/Undo]:", {{"Line", "L"}, {"Undo", "U"}}};
        }
        return {"Specify end point of arc or [CLose/Line/Undo]:",
                {{"CLose", "CL"}, {"Line", "L"}, {"Undo", "U"}}};
    }
    if (m_vertices.size() < 3) {
        return {"Specify next point or [Arc/Undo]:", {{"Arc", "A"}, {"Undo", "U"}}};
    }
    return {"Specify next point or [Arc/Close/Undo]:",
            {{"Arc", "A"}, {"Close", "C"}, {"Undo", "U"}}};
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
    auto vertices = m_vertices;
    if (m_pointer && !core::nearlyEqual(vertices.back().position, *m_pointer, kPointTolerance)) {
        if (const auto bulge = segmentBulge(*m_pointer)) {
            vertices.back().bulge = *bulge;
            vertices.push_back({*m_pointer, 0.0});
        }
    }
    m_preview.emplace_back(core::Polyline{std::move(vertices)});
}

std::optional<double> PlineCommand::segmentBulge(const core::Vec2& end) const {
    if (!m_arcMode) {
        return 0.0;
    }
    const auto chord = end - m_vertices.back().position;
    if (chord.length() <= kPointTolerance) {
        return std::nullopt;
    }
    double tangentAngle = 0.0;
    if (m_vertices.size() >= 2) {
        const auto& previous = m_vertices[m_vertices.size() - 2];
        const auto previousChord = m_vertices.back().position - previous.position;
        // The end tangent is rotated by half the signed sweep from the chord.
        tangentAngle =
            std::atan2(previousChord.y, previousChord.x) + (2.0 * std::atan(previous.bulge));
    }
    const double alpha =
        std::remainder(std::atan2(chord.y, chord.x) - tangentAngle, 2.0 * std::numbers::pi);
    if (std::numbers::pi - std::abs(alpha) <= kAngleTolerance) {
        return std::nullopt;
    }
    return std::tan(alpha / 2.0);
}

}  // namespace tg::cmd
