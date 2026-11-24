// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/render/scene_renderer.hpp"

#include "generated_drawing.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/math.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace tg::render {

using core::BBox;
using core::Vec2;

namespace {

// Safety net: never emit more grid lines than this per direction.
constexpr double kMaxGridLines = 4000.0;

constexpr int kAdaptiveFallbackMajor = 5;
constexpr double kUcsIconMarginPx = 12.0;
constexpr double kUcsIconLabelHeightPx = 9.0;
constexpr double kUcsIconLabelGapPx = 3.0;

bool overlaps(const BBox& a, const BBox& b) {
    if (a.isEmpty() || b.isEmpty()) {
        return false;
    }
    return a.min().x <= b.max().x && a.max().x >= b.min().x && a.min().y <= b.max().y &&
           a.max().y >= b.min().y;
}

struct GridAxis {
    double spacing;
    long long majorEvery;
    bool onlyMajor;
};

std::optional<GridAxis> gridAxis(double gridUnit, double snapUnit, int gridDisplay, int gridMajor,
                                 double scale, double minimumPx) {
    const double base = gridUnit > 0.0 ? gridUnit : snapUnit;
    double spacing = base;
    const bool adaptive = (gridDisplay & 2) != 0;
    const bool subdivide = adaptive && (gridDisplay & 4) != 0 && gridMajor > 1;
    if (subdivide) {
        while (spacing / static_cast<double>(gridMajor) >= snapUnit &&
               spacing * scale > static_cast<double>(gridMajor) * minimumPx) {
            spacing /= static_cast<double>(gridMajor);
        }
    }
    if (spacing * scale >= minimumPx) {
        return GridAxis{spacing, gridMajor, false};
    }
    if (!adaptive) {
        return std::nullopt;
    }
    const int factor = gridMajor == 1 ? kAdaptiveFallbackMajor : gridMajor;
    spacing *= static_cast<double>(factor);
    while (spacing * scale < minimumPx) {
        spacing *= static_cast<double>(factor);
    }
    return GridAxis{spacing, 1, true};
}

void drawGridAxis(const GridAxis& grid, double base, double first, double last, bool vertical,
                  const Viewport& viewport, const SceneStyle& style, Painter& painter) {
    const double span =
        std::ceil((last - base) / grid.spacing) - std::floor((first - base) / grid.spacing);
    if (!std::isfinite(span) || span > kMaxGridLines) {
        return;
    }
    const auto begin = static_cast<long long>(std::floor((first - base) / grid.spacing));
    const auto end = static_cast<long long>(std::ceil((last - base) / grid.spacing));
    for (const bool major : std::array{false, true}) {
        if (grid.onlyMajor && !major) {
            continue;
        }
        painter.setPen(major ? style.gridMajor : style.gridMinor, 0.0, LineStyle::Solid);
        for (long long i = begin; i <= end; ++i) {
            if (((i % grid.majorEvery) == 0) != major) {
                continue;
            }
            if (vertical) {
                const double x =
                    viewport.worldToScreen({base + static_cast<double>(i) * grid.spacing, 0}).x;
                painter.drawLine({x, 0.0}, {x, viewport.screenHeight()});
            } else {
                const double y =
                    viewport.worldToScreen({0, base + static_cast<double>(i) * grid.spacing}).y;
                painter.drawLine({0.0, y}, {viewport.screenWidth(), y});
            }
        }
    }
}

void drawGrid(const core::GridSettings& settings, const Viewport& viewport, const SceneStyle& style,
              Painter& painter) {
    const BBox area = viewport.visibleWorldArea();
    // Axes adapt independently: an 8 x 20 px grid can hide or coarsen only X.
    if (const auto x = gridAxis(settings.gridUnit.x, settings.snapUnit.x, settings.gridDisplay,
                                settings.gridMajor, viewport.scale(), style.minGridSpacingPx)) {
        drawGridAxis(*x, settings.snapBase.x, area.min().x, area.max().x, true, viewport, style,
                     painter);
    }
    if (const auto y = gridAxis(settings.gridUnit.y, settings.snapUnit.y, settings.gridDisplay,
                                settings.gridMajor, viewport.scale(), style.minGridSpacingPx)) {
        drawGridAxis(*y, settings.snapBase.y, area.min().y, area.max().y, false, viewport, style,
                     painter);
    }
}

void drawAxesLines(const Viewport& viewport, const SceneStyle& style, Painter& painter) {
    const Vec2 origin = viewport.worldToScreen(style.axisFrame.origin);
    painter.setPen(style.axisX, 0.0, LineStyle::Solid);
    painter.drawLine({0.0, origin.y}, {viewport.screenWidth(), origin.y});
    painter.setPen(style.axisY, 0.0, LineStyle::Solid);
    painter.drawLine({origin.x, 0.0}, {origin.x, viewport.screenHeight()});
}

void drawUcsIcon(int ucsIcon, const Viewport& viewport, const SceneStyle& style, Painter& painter) {
    Vec2 origin = viewport.worldToScreen(style.axisFrame.origin);
    const double size = style.ucsIconSizePx;
    const auto screenDirection = [](const Vec2& direction) {
        return Vec2{direction.x, -direction.y};
    };
    const auto includeAxis = [&](const Vec2& direction, Vec2& minimum, Vec2& maximum) {
        const Vec2 unit = screenDirection(direction);
        const Vec2 tip = origin + unit * size;
        const Vec2 labelCenter = tip + unit * (kUcsIconLabelGapPx + kUcsIconLabelHeightPx * 0.5);
        const double halfLabel = kUcsIconLabelHeightPx * 0.5;
        minimum.x = std::min({minimum.x, tip.x, labelCenter.x - halfLabel});
        minimum.y = std::min({minimum.y, tip.y, labelCenter.y - halfLabel});
        maximum.x = std::max({maximum.x, tip.x, labelCenter.x + halfLabel});
        maximum.y = std::max({maximum.y, tip.y, labelCenter.y + halfLabel});
    };
    Vec2 minimum = origin;
    Vec2 maximum = origin;
    includeAxis(style.axisFrame.xDirection, minimum, maximum);
    includeAxis(style.axisFrame.yDirection, minimum, maximum);
    if ((ucsIcon & 2) == 0 || minimum.x < 0.0 || minimum.y < 0.0 ||
        maximum.x > viewport.screenWidth() || maximum.y > viewport.screenHeight()) {
        origin = {kUcsIconMarginPx, viewport.screenHeight() - kUcsIconMarginPx};
    }
    const auto arrow = [&](const Vec2& direction, const Color& color, const std::string& label) {
        const Vec2 unit = screenDirection(direction);
        const Vec2 tip = origin + unit * size;
        painter.setPen(color, 0.0, LineStyle::Solid);
        painter.drawLine(origin, tip);
        const Vec2 labelCenter = tip + unit * (kUcsIconLabelGapPx + kUcsIconLabelHeightPx * 0.5);
        painter.drawText(
            {std::u32string(label.begin(), label.end()),
             labelCenter + Vec2{-kUcsIconLabelHeightPx * 0.5, kUcsIconLabelHeightPx * 0.5},
             kUcsIconLabelHeightPx});
    };
    arrow(style.axisFrame.xDirection, style.axisX, style.axisFrame.xLabel);
    arrow(style.axisFrame.yDirection, style.axisY, style.axisFrame.yLabel);
}

// The shapes of a preview entity: an entity of a shape type is its own shape,
// whatever its layer; the others are their generated geometry (an insert at
// the pointer, docs/adr/0044, point 2).
std::vector<core::GeneratedShape> previewShapes(const core::Document& document,
                                                const core::Entity& entity) {
    return std::visit(
        [&](const auto& value) -> std::vector<core::GeneratedShape> {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_constructible_v<core::GeneratedShape, const T&>) {
                return {core::GeneratedShape{std::in_place_type<T>, value}};
            } else {
                const core::EntityRecord record{core::EntityId{}, entity};
                std::vector<core::GeneratedShape> shapes;
                for (core::GeneratedItem& item : core::generate(document, record)) {
                    shapes.push_back(std::move(item.shape));
                }
                return shapes;
            }
        },
        entity);
}

// The circle of a snap marker: 32 edges keep the default 10 px circle's chord
// error below 0.025 px.
constexpr std::size_t kMarkerCircleSegments = 32;

std::array<Vec2, kMarkerCircleSegments> markerCircle(const Vec2& center, double radius) {
    std::array<Vec2, kMarkerCircleSegments> points;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const double angle =
            core::kTwoPi * static_cast<double>(i) / static_cast<double>(points.size());
        const auto direction = core::sinCos(angle);
        points.at(i) = center + Vec2{direction.cosine, direction.sine} * radius;
    }
    return points;
}

}  // namespace

void renderScene(const core::Document& document, const Viewport& viewport, const SceneStyle& style,
                 Painter& painter) {
    painter.clear(style.background);
    if (style.showGrid && document.grid().gridOn) {
        drawGrid(document.grid(), viewport, style, painter);
    }
    if (style.axes == AxesStyle::Lines) {
        drawAxesLines(viewport, style, painter);
    } else if (style.axes == AxesStyle::UcsIcon && (document.grid().ucsIcon & 1) != 0) {
        drawUcsIcon(document.grid().ucsIcon, viewport, style, painter);
    }

    const BBox visible = viewport.visibleWorldArea();
    for (const core::EntityRecord& record : document.entities()) {
        if (!overlaps(core::extent(document, record), visible)) {
            continue;
        }
        // Nothing on a layer that is off or frozen arrives (core::generate()).
        for (const core::GeneratedItem& item : core::generate(document, record)) {
            drawGeneratedItem(item, document.lwdisplay(), viewport, style, painter);
        }
    }
}

void renderPreview(const core::Document& document, std::span<const core::Entity> entities,
                   const Viewport& viewport, const SceneStyle& style, Painter& painter) {
    const BBox visible = viewport.visibleWorldArea();
    painter.setPen(style.preview, 0.0, LineStyle::Solid);
    for (const core::Entity& entity : entities) {
        for (const core::GeneratedShape& shape : previewShapes(document, entity)) {
            if (overlaps(core::shapeBoundingBox(shape), visible)) {
                drawShape(shape, style.preview, viewport, style.tessellationTolerancePx, painter);
            }
        }
    }
}

void renderHighlight(const core::Document& document, std::span<const core::EntityId> ids,
                     const Viewport& viewport, const SceneStyle& style, Painter& painter) {
    const BBox visible = viewport.visibleWorldArea();
    painter.setPen(style.highlight, 0.0, LineStyle::Dashed);
    for (const core::EntityId id : ids) {
        const core::EntityRecord* record = document.find(id);
        if (record == nullptr || !overlaps(core::extent(document, *record), visible)) {
            continue;
        }
        for (const core::GeneratedItem& item : core::generate(document, *record)) {
            drawShape(item.shape, style.highlight, viewport, style.tessellationTolerancePx,
                      painter);
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
            painter.drawPolyline(markerCircle(center, half), true);
            break;
        }
        case core::SnapKind::Intersection: {
            // A cross in the shape of an X (docs/adr/0033, point 1).
            painter.drawLine(center + Vec2{-half, -half}, center + Vec2{half, half});
            painter.drawLine(center + Vec2{-half, half}, center + Vec2{half, -half});
            break;
        }
        case core::SnapKind::Perpendicular: {
            // The symbol ⊥: a base with a stem rising from its middle.
            painter.drawLine(center + Vec2{-half, half}, center + Vec2{half, half});
            painter.drawLine(center + Vec2{0.0, half}, center + Vec2{0.0, -half});
            break;
        }
        case core::SnapKind::Tangent: {
            // A circle with a line tangent to it at the top.
            painter.drawPolyline(markerCircle(center, half), true);
            painter.drawLine(center + Vec2{-half, -half}, center + Vec2{half, -half});
            break;
        }
        case core::SnapKind::Quadrant: {
            // A diamond.
            const std::array points{center + Vec2{0.0, -half}, center + Vec2{half, 0.0},
                                    center + Vec2{0.0, half}, center + Vec2{-half, 0.0}};
            painter.drawPolyline(points, true);
            break;
        }
        case core::SnapKind::Nearest: {
            // An hourglass, as AutoCAD: two triangles meeting at the centre,
            // between the top and bottom sides of the marker square.
            const std::array points{center + Vec2{-half, -half}, center + Vec2{half, -half},
                                    center + Vec2{-half, half}, center + Vec2{half, half}};
            painter.drawPolyline(points, true);
            break;
        }
        case core::SnapKind::Extension:
        case core::SnapKind::Parallel:
            // Guides of the tracking, drawn by the view (docs/adr/0057,
            // point 5): no marker here.
            break;
    }
}

void renderGrips(std::span<const Vec2> grips, std::optional<Vec2> hot, const Viewport& viewport,
                 const SceneStyle& style, Painter& painter) {
    const double half = style.gripSizePx * 0.5;
    const auto square = [&](const Vec2& point, const Color& color) {
        const Vec2 center = viewport.worldToScreen(point);
        const std::array points{center + Vec2{-half, -half}, center + Vec2{half, -half},
                                center + Vec2{half, half}, center + Vec2{-half, half}};
        painter.fillPolygon(points, color);
    };
    for (const Vec2& point : grips) {
        square(point, style.grip);
    }
    // Last, including when several selected entities share the hot point.
    if (hot) {
        square(*hot, style.hotGrip);
    }
}

}  // namespace tg::render
