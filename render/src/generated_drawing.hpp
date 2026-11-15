// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/generated.hpp"
#include "tg/render/painter.hpp"
#include "tg/render/scene_renderer.hpp"
#include "tg/render/viewport.hpp"

#include <vector>

/// The generated geometry on the screen (docs/adr/0044, point 2;
/// docs/tappe/0.8/PT8-geometria-generata-render.md). Internal to the render
/// module: only the scene renderer and the tests include it.
namespace tg::render {

/// The points of `ellipse`, in drawing coordinates, from its start parameter
/// to its end parameter, with the angular step of the arcs
/// (core::tessellate() of a circle as large as the major axis): the chord
/// error stays within `maxChordError`. A full ellipse does not repeat its
/// first point at the end. Empty when the major axis has no finite, positive
/// length. Precondition: maxChordError > 0.
[[nodiscard]] std::vector<core::Vec2> ellipsePoints(const core::Ellipse& ellipse,
                                                    double maxChordError);

/// True when `ellipse` runs a full turn: the polyline of ellipsePoints() is
/// closed.
[[nodiscard]] bool isFullEllipse(const core::Ellipse& ellipse);

/// Draws `shape` with the pen already set, without a linetype: lines, arcs,
/// circles, polylines and ellipses as polylines, a text as the scene draws it,
/// a Solid with Painter::fillPolygon on v0, v1, v3, v2 (docs/adr/0044,
/// point 7) and a FilledRegion with Painter::fillLoops, its loops mapped by
/// its transform. Both fills take `fill`.
void drawShape(const core::GeneratedShape& shape, const Color& fill, const Viewport& viewport,
               double tessellationTolerancePx, Painter& painter);

/// Draws `item` with its effective properties, as renderScene() documents it:
/// the colour (index 7 is style.entity), the lineweight with `lwdisplay` and
/// the linetype, the pattern starting again at the start of the shape. A
/// text takes only the colour; a Solid and a FilledRegion are filled with the
/// colour, without lineweight or linetype.
void drawGeneratedItem(const core::GeneratedItem& item, bool lwdisplay, const Viewport& viewport,
                       const SceneStyle& style, Painter& painter);

}  // namespace tg::render
