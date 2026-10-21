// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/render/scene_renderer.hpp"

#include "tg/core/arc_geometry.hpp"
#include "tg/core/math.hpp"
#include "tg/core/text.hpp"

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
        painter.setPen(major ? style.gridMajor : style.gridMinor, 0.0, LineStyle::Solid);
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
    painter.setPen(style.axisX, 0.0, LineStyle::Solid);
    painter.drawLine({0.0, origin.y}, {viewport.screenWidth(), origin.y});
    painter.setPen(style.axisY, 0.0, LineStyle::Solid);
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

    void operator()(const core::Line& line) {
        const std::array points{m_viewport.worldToScreen(line.start),
                                m_viewport.worldToScreen(line.end)};
        m_painter.drawPolyline(points, false);
    }

    void operator()(const core::Circle& circle) {
        // A closed polyline: the last point, back on the start, is dropped.
        drawArc(circle.geometry(), true);
    }

    void operator()(const core::Arc& arc) { drawArc(arc.geometry(), false); }

    void operator()(const core::Text& text) {
        const double heightPx = text.height * m_viewport.scale();
        if (heightPx < kMinTextHeightPx) {
            m_screenPoints.clear();
            for (const Vec2& corner : core::textBox(text)) {
                m_screenPoints.push_back(m_viewport.worldToScreen(corner));
            }
            m_painter.drawPolyline(m_screenPoints, true);
            return;
        }
        // The screen flips Y and the angle together: a rotation that is
        // counter-clockwise in the drawing is counter-clockwise as seen.
        m_painter.drawText(TextRun{core::displayText(text.value),
                                   m_viewport.worldToScreen(text.position), heightPx, text.rotation,
                                   text.widthFactor});
    }

private:
    void drawArc(const core::ArcGeometry& arc, bool closed) {
        const double toleranceWorld = m_style.tessellationTolerancePx / m_viewport.scale();
        m_worldPoints.clear();
        m_worldPoints.push_back(core::startPoint(arc));
        core::tessellate(arc, toleranceWorld, m_worldPoints);
        if (closed) {
            m_worldPoints.pop_back();
        }
        m_screenPoints.clear();
        for (const Vec2& point : m_worldPoints) {
            m_screenPoints.push_back(m_viewport.worldToScreen(point));
        }
        m_painter.drawPolyline(m_screenPoints, closed);
    }

    const Viewport& m_viewport;
    const SceneStyle& m_style;
    Painter& m_painter;
    std::vector<Vec2> m_worldPoints;   // reused buffer
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
    painter.setPen(style.entity, 0.0, LineStyle::Solid);
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
    painter.setPen(style.preview, 0.0, LineStyle::Solid);
    for (const core::Entity& entity : entities) {
        if (overlaps(core::boundingBox(entity), visible)) {
            std::visit(drawer, entity);
        }
    }
}

void renderHighlight(const core::Document& document, std::span<const core::EntityId> ids,
                     const Viewport& viewport, const SceneStyle& style, Painter& painter) {
    const BBox visible = viewport.visibleWorldArea();
    EntityDrawer drawer(viewport, style, painter);
    painter.setPen(style.highlight, 0.0, LineStyle::Dashed);
    for (const core::EntityId id : ids) {
        const core::EntityRecord* record = document.find(id);
        if (record != nullptr && overlaps(core::boundingBox(record->entity), visible)) {
            std::visit(drawer, record->entity);
        }
    }
}

void renderSelectionRectangle(const Vec2& first, const Vec2& second, bool crossing,
                              const Viewport& viewport, const SceneStyle& style, Painter& painter) {
    const Vec2 a = viewport.worldToScreen(first);
    const Vec2 b = viewport.worldToScreen(second);
    const std::array points{a, Vec2{b.x, a.y}, b, Vec2{a.x, b.y}};
    painter.fillPolygon(points, crossing ? style.crossingFill : style.windowFill);
    painter.setPen(crossing ? style.crossingBorder : style.windowBorder, 0.0,
                   crossing ? LineStyle::Dashed : LineStyle::Solid);
    painter.drawPolyline(points, true);
}

void renderSnapMarker(const core::SnapPoint& snap, const Viewport& viewport,
                      const SceneStyle& style, Painter& painter) {
    const Vec2 center = viewport.worldToScreen(snap.point);
    const double half = style.snapMarkerSizePx * 0.5;
    // ADR 0008 markers are screen-space overlays, not drawing geometry.
    constexpr double kMarkerPenWidthPx = 2.0;
    painter.setPen(style.snapMarker, kMarkerPenWidthPx, LineStyle::Solid);
    switch (snap.kind) {
        case core::SnapKind::Endpoint: {
            const std::array points{center + Vec2{-half, -half}, center + Vec2{half, -half},
                                    center + Vec2{half, half}, center + Vec2{-half, half}};
            painter.drawPolyline(points, true);
            break;
        }
        case core::SnapKind::Midpoint: {
            // Centre the triangular outline's bounding box on the snap point.
            const std::array points{center + Vec2{0.0, -half}, center + Vec2{half, half},
                                    center + Vec2{-half, half}};
            painter.drawPolyline(points, true);
            break;
        }
        case core::SnapKind::Insertion: {
            // Two squares, each two thirds of the marker, one at the top
            // left and one at the bottom right: together they fill it.
            const double side = style.snapMarkerSizePx * (2.0 / 3.0);
            for (const Vec2 corner :
                 {center + Vec2{-half, -half}, center + Vec2{half - side, half - side}}) {
                const std::array points{corner, corner + Vec2{side, 0.0}, corner + Vec2{side, side},
                                        corner + Vec2{0.0, side}};
                painter.drawPolyline(points, true);
            }
            break;
        }
        case core::SnapKind::Center: {
            // 32 edges keep the default 10 px circle's chord error below 0.025 px.
            constexpr std::size_t kCircleSegments = 32;
            std::array<Vec2, kCircleSegments> points;
            for (std::size_t i = 0; i < points.size(); ++i) {
                const double angle =
                    core::kTwoPi * static_cast<double>(i) / static_cast<double>(points.size());
                points.at(i) = center + Vec2{std::cos(angle), std::sin(angle)} * half;
            }
            painter.drawPolyline(points, true);
            break;
        }
    }
}

}  // namespace tg::render
