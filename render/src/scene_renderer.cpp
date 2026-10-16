// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/render/scene_renderer.hpp"

#include <array>
#include <cmath>
#include <variant>
#include <vector>

namespace tg::render {

using core::BBox;
using core::Vec2;

namespace {

// Safety net: never emit more grid lines than this per direction.
constexpr double kMaxGridLines = 4000.0;

// Every n-th grid line is drawn as a major line.
constexpr long long kMajorEvery = 5;

bool overlaps(const BBox& a, const BBox& b) {
    if (a.isEmpty() || b.isEmpty()) {
        return false;
    }
    return a.min().x <= b.max().x && a.max().x >= b.min().x && a.min().y <= b.max().y &&
           a.max().y >= b.min().y;
}

void drawGrid(const Viewport& viewport, const SceneStyle& style, Painter& painter) {
    const double spacing = gridSpacing(viewport.scale(), style.minGridSpacingPx);
    const BBox area = viewport.visibleWorldArea();

    const double spanX = std::ceil(area.max().x / spacing) - std::floor(area.min().x / spacing);
    const double spanY = std::ceil(area.max().y / spacing) - std::floor(area.min().y / spacing);
    const bool finite = std::isfinite(spanX) && std::isfinite(spanY);
    if (!finite || spanX > kMaxGridLines || spanY > kMaxGridLines) {
        return;
    }
    // Grid lines are numbered with integers: line i lies at i * spacing.
    const long long firstX = std::llround(std::floor(area.min().x / spacing));
    const long long lastX = std::llround(std::ceil(area.max().x / spacing));
    const long long firstY = std::llround(std::floor(area.min().y / spacing));
    const long long lastY = std::llround(std::ceil(area.max().y / spacing));

    const double top = 0.0;
    const double bottom = viewport.screenHeight();
    const double left = 0.0;
    const double right = viewport.screenWidth();

    // Two passes so that major lines are painted over minor ones.
    for (const bool major : std::array{false, true}) {
        painter.setPen(major ? style.gridMajor : style.gridMinor, 0.0);
        for (long long i = firstX; i <= lastX; ++i) {
            if ((i % kMajorEvery == 0) != major) {
                continue;
            }
            const double x = viewport.worldToScreen({static_cast<double>(i) * spacing, 0.0}).x;
            painter.drawLine({x, top}, {x, bottom});
        }
        for (long long i = firstY; i <= lastY; ++i) {
            if ((i % kMajorEvery == 0) != major) {
                continue;
            }
            const double y = viewport.worldToScreen({0.0, static_cast<double>(i) * spacing}).y;
            painter.drawLine({left, y}, {right, y});
        }
    }
}

void drawAxes(const Viewport& viewport, const SceneStyle& style, Painter& painter) {
    const Vec2 origin = viewport.worldToScreen({0.0, 0.0});
    painter.setPen(style.axisX, 0.0);
    painter.drawLine({0.0, origin.y}, {viewport.screenWidth(), origin.y});
    painter.setPen(style.axisY, 0.0);
    painter.drawLine({origin.x, 0.0}, {origin.x, viewport.screenHeight()});
}

class EntityDrawer {
public:
    EntityDrawer(const Viewport& viewport, const SceneStyle& style, Painter& painter)
        : m_viewport(viewport), m_style(style), m_painter(painter) {}

    void operator()(const core::Polyline& polyline) {
        const double toleranceWorld = m_style.tessellationTolerancePx / m_viewport.scale();
        m_screenPoints.clear();
        for (const Vec2& point : polyline.tessellate(toleranceWorld)) {
            m_screenPoints.push_back(m_viewport.worldToScreen(point));
        }
        m_painter.drawPolyline(m_screenPoints, polyline.isClosed());
    }

private:
    const Viewport& m_viewport;
    const SceneStyle& m_style;
    Painter& m_painter;
    std::vector<Vec2> m_screenPoints;  // reused buffer
};

}  // namespace

double gridSpacing(double pixelsPerUnit, double minSpacingPx) {
    if (pixelsPerUnit <= 0.0 || minSpacingPx <= 0.0) {
        return 1.0;
    }
    const double minWorld = minSpacingPx / pixelsPerUnit;
    const double decade = std::pow(10.0, std::floor(std::log10(minWorld)));
    for (const double mantissa : {1.0, 2.0, 5.0, 10.0}) {
        if (mantissa * decade >= minWorld) {
            return mantissa * decade;
        }
    }
    return 10.0 * decade;
}

void renderScene(const core::Document& document, const Viewport& viewport, const SceneStyle& style,
                 Painter& painter) {
    painter.clear(style.background);
    if (style.showGrid) {
        drawGrid(viewport, style, painter);
    }
    drawAxes(viewport, style, painter);

    const BBox visible = viewport.visibleWorldArea();
    EntityDrawer drawer(viewport, style, painter);
    painter.setPen(style.entity, 0.0);
    for (const core::EntityRecord& record : document.entities()) {
        if (overlaps(core::boundingBox(record.entity), visible)) {
            std::visit(drawer, record.entity);
        }
    }
}

void renderPreview(std::span<const core::Entity> entities, const Viewport& viewport,
                   const SceneStyle& style, Painter& painter) {
    const BBox visible = viewport.visibleWorldArea();
    EntityDrawer drawer(viewport, style, painter);
    painter.setPen(style.preview, 0.0);
    for (const core::Entity& entity : entities) {
        if (overlaps(core::boundingBox(entity), visible)) {
            std::visit(drawer, entity);
        }
    }
}

}  // namespace tg::render
