// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/render/painter.hpp"
#include "tg/render/viewport.hpp"

#include <span>

namespace tg::render {

/// Below this height of the capital letters, in pixels, a text is drawn as
/// its box: its characters could not be read (docs/adr/0013, point 4).
inline constexpr double kMinTextHeightPx = 2.0;

/// Visual settings of the drawing area.
struct SceneStyle {
    Color background{33, 40, 48};
    Color gridMinor{44, 52, 62};
    Color gridMajor{58, 68, 80};
    Color axisX{170, 60, 60};
    Color axisY{60, 160, 70};
    Color entity{235, 235, 235};
    Color preview{230, 200, 90};
    Color highlight{90, 160, 255};
    Color windowFill{40, 90, 200, 60};
    Color windowBorder{80, 140, 255};
    Color crossingFill{40, 160, 70, 60};
    Color crossingBorder{90, 210, 110};
    Color snapMarker{255, 170, 40};
    double snapMarkerSizePx{10.0};
    double minGridSpacingPx{12.0};  ///< finer grids are hidden
    double tessellationTolerancePx{0.25};
    bool showGrid{true};
};

/// Spacing (drawing units) of the minor grid lines: the smallest value of
/// the form {1, 2, 5} * 10^n that is at least `minSpacingPx` on screen.
[[nodiscard]] double gridSpacing(double pixelsPerUnit, double minSpacingPx);

/// Draws grid, axes and every entity of `document` as seen through `viewport`.
/// A text is drawn with Painter::drawText from its position, height, rotation
/// and width factor; oblique angle, mirroring and alignments other than left
/// on the baseline are ignored until tappa 0.5 (docs/adr/0013, point 1). A
/// text whose capital letters are lower than kMinTextHeightPx is drawn as its
/// box (docs/adr/0013, point 4).
void renderScene(const core::Document& document, const Viewport& viewport, const SceneStyle& style,
                 Painter& painter);

/// Draws preview entities without clearing the background or drawing grid and axes.
void renderPreview(std::span<const core::Entity> entities, const Viewport& viewport,
                   const SceneStyle& style, Painter& painter);

/// Draws selected entities dashed without clearing; absent ids are skipped.
void renderHighlight(const core::Document& document, std::span<const core::EntityId> ids,
                     const Viewport& viewport, const SceneStyle& style, Painter& painter);

/// Fills and outlines the selection rectangle between drawing-space corners.
/// Window is blue/solid; crossing is green/dashed.
void renderSelectionRectangle(const core::Vec2& first, const core::Vec2& second, bool crossing,
                              const Viewport& viewport, const SceneStyle& style, Painter& painter);

/// Draws a square (endpoint), triangle (midpoint), circle (centre) or two
/// overlapping offset squares (insertion, docs/adr/0013), centred on the snap
/// in screen space. Marker width is snapMarkerSizePx; pen width is 2 px,
/// independent of zoom.
void renderSnapMarker(const core::SnapPoint& snap, const Viewport& viewport,
                      const SceneStyle& style, Painter& painter);

}  // namespace tg::render
