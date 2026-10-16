// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/render/painter.hpp"
#include "tg/render/viewport.hpp"

#include <span>

namespace tg::render {

/// Visual settings of the drawing area.
struct SceneStyle {
    Color background{33, 40, 48};
    Color gridMinor{44, 52, 62};
    Color gridMajor{58, 68, 80};
    Color axisX{170, 60, 60};
    Color axisY{60, 160, 70};
    Color entity{235, 235, 235};
    Color preview{230, 200, 90};
    double minGridSpacingPx{12.0};  ///< finer grids are hidden
    double tessellationTolerancePx{0.25};
    bool showGrid{true};
};

/// Spacing (drawing units) of the minor grid lines: the smallest value of
/// the form {1, 2, 5} * 10^n that is at least `minSpacingPx` on screen.
[[nodiscard]] double gridSpacing(double pixelsPerUnit, double minSpacingPx);

/// Draws grid, axes and every entity of `document` as seen through `viewport`.
void renderScene(const core::Document& document, const Viewport& viewport, const SceneStyle& style,
                 Painter& painter);

/// Draws preview entities without clearing the background or drawing grid and axes.
void renderPreview(std::span<const core::Entity> entities, const Viewport& viewport,
                   const SceneStyle& style, Painter& painter);

}  // namespace tg::render
