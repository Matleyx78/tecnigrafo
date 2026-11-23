// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/render/painter.hpp"
#include "tg/render/viewport.hpp"

#include <optional>
#include <span>
#include <string>

namespace tg::render {

/// Below this height of the capital letters, in pixels, a text is drawn as
/// its box: its characters could not be read (docs/adr/0013, point 4).
inline constexpr double kMinTextHeightPx = 2.0;

struct AxisFrame {
    core::Vec2 origin{0.0, 0.0};
    core::Vec2 xDirection{1.0, 0.0};
    core::Vec2 yDirection{0.0, 1.0};  ///< Unit vectors.
    std::string xLabel{"X"};
    std::string yLabel{"Y"};
};

enum class AxesStyle : std::uint8_t { UcsIcon, Lines, None };

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
    /// AutoCAD GRIPCOLOR.
    Color grip{0, 0, 255};
    /// AutoCAD GRIPHOT.
    Color hotGrip{255, 0, 0};
    /// cmd::CommandRunner::kGripSizePixels.
    double gripSizePx{10.0};
    double minGridSpacingPx{12.0};
    bool showGrid{true};  ///< Program preference; gridOn is the drawing switch.
    AxesStyle axes{AxesStyle::UcsIcon};
    AxisFrame axisFrame;
    double ucsIconSizePx{32.0};
    double tessellationTolerancePx{0.25};
    /// Device pixels per logical pixel of the screen: the width of a pen
    /// drawn with a lineweight is in device pixels (docs/adr/0025, point 6).
    /// Precondition: positive.
    double devicePixelRatio{1.0};
};

/// Draws grid, axes and every visible entity of `document` as seen through
/// `viewport`: those on a layer that is off or frozen are skipped, those on a
/// locked layer are drawn (docs/adr/0023). Each entity is drawn as its
/// generated geometry (core::generate(), docs/adr/0044, point 2): every shape
/// with its effective properties:
/// - colour: its RGB, except index 7, which is style.entity, the main colour of
///   the theme (docs/adr/0024, point 5);
/// - lineweight: with document.lwdisplay() on, a pen of
///   max(1, round(mm * 96 / 25.4 * style.devicePixelRatio)) pixels, otherwise
///   the cosmetic pen of width 0 (docs/adr/0025, point 6);
/// - linetype: the pieces of core::dashes() with the effective scale, each with
///   Painter::drawLine or an open Painter::drawPolyline, and a dot as a stroke
///   of one pixel; continuous when the pattern, scaled and seen on the screen,
///   is shorter than 4 pixels, or when core::dashes() gives no pieces
///   (docs/adr/0025, point 5).
/// The pen is always LineStyle::Solid: the pattern is ours, not the painter's.
/// A text takes only the colour: no lineweight and no linetype.
/// A text is drawn with Painter::drawText using core::textFrame() for its
/// origin, used height, rotation and width factor, together with its oblique
/// angle and mirroring (docs/adr/0022). A text whose used capital height is
/// below kMinTextHeightPx is drawn as its transformed core::textBox().
/// A Solid is filled with Painter::fillPolygon on v0, v1, v3, v2 (docs/adr/0044,
/// point 7); a filled region with Painter::fillLoops, its loops mapped by its
/// transform; both take only the colour. An ellipse is a polyline computed for
/// the screen with the angular step of the arcs, its linetype running across
/// the polyline (docs/adr/0052, point 3). The linetype pattern starts again at
/// the start of every shape.
void renderScene(const core::Document& document, const Viewport& viewport, const SceneStyle& style,
                 Painter& painter);

/// Draws preview entities without clearing the background or drawing grid and
/// axes, in style.preview with the cosmetic pen. An entity of a shape type
/// (line, arc, circle, polyline, text, Solid, ellipse) is drawn as itself,
/// whatever its layer; the others with the shapes of core::generate() in
/// `document`, which resolves their blocks and styles (an insert at the
/// pointer). A Solid or a filled region is filled with style.preview.
void renderPreview(const core::Document& document, std::span<const core::Entity> entities,
                   const Viewport& viewport, const SceneStyle& style, Painter& painter);

/// Draws selected entities dashed without clearing, with the shapes of
/// core::generate(), as the scene; a Solid or a filled region is filled with
/// style.highlight. Absent ids are skipped.
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

/// Draws filled grip squares in screen space; the hot one in hotGrip.
void renderGrips(std::span<const core::Vec2> grips, std::optional<core::Vec2> hot,
                 const Viewport& viewport, const SceneStyle& style, Painter& painter);

}  // namespace tg::render
