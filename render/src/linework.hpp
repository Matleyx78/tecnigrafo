// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/dashes.hpp"
#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/render/painter.hpp"
#include "tg/render/viewport.hpp"

#include <optional>
#include <span>
#include <vector>

/// Lineweights and linetypes as they appear on the screen (docs/adr/0025,
/// points 5 and 6). Internal to the render module: only the scene renderer
/// and the tests include it.
namespace tg::render {

/// A scaled pattern shorter than this on the screen, in pixels, would not be
/// told apart: the entity is drawn continuous (docs/adr/0025, point 5).
inline constexpr double kMinPatternLengthPx = 4.0;

/// Length of the stroke that stands for a dot of a pattern, along the
/// direction of the line (docs/adr/0025, point 5).
inline constexpr double kDotLengthPx = 1.0;

/// Logical pixels per inch of the screen, and millimetres per inch: the
/// lineweight in millimetres becomes pixels with them (docs/adr/0025, point 6).
inline constexpr double kPixelsPerInch = 96.0;
inline constexpr double kMillimetresPerInch = 25.4;

/// Pen width in device pixels for a lineweight in hundredths of a millimetre.
/// With lineweights off the width is 0, the cosmetic pen: one pixel at every
/// zoom. With them on it is max(1, round(mm * 96 / 25.4 * devicePixelRatio)),
/// independent of the zoom.
/// Precondition: lineweight >= 0 (the effective value, never ByLayer and the
/// like) and devicePixelRatio > 0.
[[nodiscard]] double penWidthPx(int lineweight, bool lwdisplay, double devicePixelRatio);

/// The pieces to draw for `entity` with its effective linetype, in drawing
/// coordinates, or std::nullopt when it is drawn continuous: a text, a
/// continuous linetype, a scaled pattern shorter than kMinPatternLengthPx at
/// `pixelsPerUnit`, or whatever core::dashes() draws continuous (the entity is
/// shorter than a pattern, or it would give more than core::kMaxDashPieces).
/// Precondition: pixelsPerUnit > 0.
[[nodiscard]] std::optional<std::vector<core::DashPiece>> dashPieces(
    const core::Entity& entity, const core::EffectiveProperties& properties, double pixelsPerUnit);

/// Draws each piece with the pen already set: a segment with drawLine, an arc
/// tessellated as the continuous arcs are and drawn with an open drawPolyline,
/// a dot as a stroke of kDotLengthPx along its direction.
void drawDashPieces(std::span<const core::DashPiece> pieces, const Viewport& viewport,
                    double tessellationTolerancePx, Painter& painter);

}  // namespace tg::render
