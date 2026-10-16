// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/render/viewport.hpp"

#include <algorithm>

namespace tg::render {

using core::Vec2;

void Viewport::setScreenSize(double widthPx, double heightPx) {
    m_widthPx = std::max(widthPx, 1.0);
    m_heightPx = std::max(heightPx, 1.0);
}

void Viewport::setScale(double pixelsPerUnit) {
    m_scale = std::clamp(pixelsPerUnit, kMinScale, kMaxScale);
}

Vec2 Viewport::worldToScreen(const Vec2& world) const {
    return {((world.x - m_center.x) * m_scale) + (m_widthPx / 2.0),
            (m_heightPx / 2.0) - ((world.y - m_center.y) * m_scale)};
}

Vec2 Viewport::screenToWorld(const Vec2& screen) const {
    return {((screen.x - (m_widthPx / 2.0)) / m_scale) + m_center.x,
            (((m_heightPx / 2.0) - screen.y) / m_scale) + m_center.y};
}

core::Transform2D Viewport::worldToScreenTransform() const {
    // screen = S(scale, -scale) * (world - center) + halfSize
    return core::Transform2D::translation({m_widthPx / 2.0, m_heightPx / 2.0}) *
           core::Transform2D::scaling(m_scale, -m_scale) *
           core::Transform2D::translation(-m_center);
}

core::BBox Viewport::visibleWorldArea() const {
    return {screenToWorld({0.0, m_heightPx}), screenToWorld({m_widthPx, 0.0})};
}

void Viewport::zoomAt(const Vec2& screenAnchor, double factor) {
    if (factor <= 0.0) {
        return;
    }
    const Vec2 anchorWorld = screenToWorld(screenAnchor);
    setScale(m_scale * factor);
    // Shift the center so that anchorWorld is again under screenAnchor.
    m_center += anchorWorld - screenToWorld(screenAnchor);
}

void Viewport::panByPixels(const Vec2& deltaPx) {
    m_center.x -= deltaPx.x / m_scale;
    m_center.y += deltaPx.y / m_scale;
}

void Viewport::zoomToBox(const core::BBox& worldBox, double marginFraction) {
    if (worldBox.isEmpty()) {
        return;
    }
    m_center = worldBox.center();
    const double usable = std::clamp(1.0 - (2.0 * marginFraction), 0.1, 1.0);
    const double width = worldBox.width();
    const double height = worldBox.height();
    if (width <= 0.0 && height <= 0.0) {
        return;  // a single point: keep the current scale, just center it
    }
    const double scaleX = width > 0.0 ? (m_widthPx * usable) / width : kMaxScale;
    const double scaleY = height > 0.0 ? (m_heightPx * usable) / height : kMaxScale;
    setScale(std::min(scaleX, scaleY));
}

}  // namespace tg::render
