// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/transform2d.hpp"
#include "tg/core/vec2.hpp"

namespace tg::render {

/// Maps the drawing plane (world, Y up, drawing units) onto a screen area
/// (pixels, origin top-left, Y down). Holds zoom and pan state only; it
/// knows nothing about any GUI toolkit.
class Viewport {
public:
    static constexpr double kMinScale = 1e-9;  ///< pixels per drawing unit
    static constexpr double kMaxScale = 1e9;

    /// Size of the screen area in pixels.
    void setScreenSize(double widthPx, double heightPx);

    [[nodiscard]] double screenWidth() const { return m_widthPx; }

    [[nodiscard]] double screenHeight() const { return m_heightPx; }

    /// Pixels per drawing unit.
    [[nodiscard]] double scale() const { return m_scale; }

    void setScale(double pixelsPerUnit);

    /// World point displayed at the center of the screen area.
    [[nodiscard]] const core::Vec2& center() const { return m_center; }

    void setCenter(const core::Vec2& worldPoint) { m_center = worldPoint; }

    [[nodiscard]] core::Vec2 worldToScreen(const core::Vec2& world) const;
    [[nodiscard]] core::Vec2 screenToWorld(const core::Vec2& screen) const;
    [[nodiscard]] core::Transform2D worldToScreenTransform() const;

    /// World area currently visible.
    [[nodiscard]] core::BBox visibleWorldArea() const;

    /// Multiplies the scale by `factor`, keeping the world point under
    /// `screenAnchor` fixed (zoom towards the cursor).
    void zoomAt(const core::Vec2& screenAnchor, double factor);

    /// Moves the view by a screen-space delta (drag with the mouse).
    void panByPixels(const core::Vec2& deltaPx);

    /// Fits `worldBox` in the screen area, leaving `marginFraction` of free
    /// space around it. Does nothing for an empty box.
    void zoomToBox(const core::BBox& worldBox, double marginFraction = 0.05);

private:
    double m_widthPx{1.0};
    double m_heightPx{1.0};
    double m_scale{1.0};
    core::Vec2 m_center;
};

}  // namespace tg::render
