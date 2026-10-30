// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/render/viewport.hpp"

#include "tg/core/math.hpp"
#include "tg/core/text.hpp"
#include "tg/render/scene_renderer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <ranges>
#include <utility>
#include <vector>

namespace tg::render {
namespace {

using core::BBox;
using core::Vec2;

constexpr double kEps = 1e-9;

void expectNear(const Vec2& actual, const Vec2& expected, double tolerance = kEps) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
}

Viewport makeViewport() {
    Viewport viewport;
    viewport.setScreenSize(800.0, 600.0);
    viewport.setScale(2.0);
    viewport.setCenter({10.0, 20.0});
    return viewport;
}

TEST(Viewport, CenterMapsToScreenCenterAndYPointsUp) {
    const Viewport viewport = makeViewport();
    expectNear(viewport.worldToScreen({10.0, 20.0}), {400.0, 300.0});
    expectNear(viewport.worldToScreen({11.0, 21.0}), {402.0, 298.0});
}

TEST(Viewport, ScreenToWorldIsTheInverse) {
    const Viewport viewport = makeViewport();
    const Vec2 world{-123.25, 456.5};
    expectNear(viewport.screenToWorld(viewport.worldToScreen(world)), world);
}

TEST(Viewport, TransformMatchesPointMapping) {
    const Viewport viewport = makeViewport();
    const Vec2 world{3.0, -4.0};
    expectNear(viewport.worldToScreenTransform().apply(world), viewport.worldToScreen(world));
}

TEST(Viewport, ZoomKeepsAnchorFixed) {
    Viewport viewport = makeViewport();
    const Vec2 anchor{123.0, 456.0};
    const Vec2 before = viewport.screenToWorld(anchor);
    viewport.zoomAt(anchor, 1.5);
    EXPECT_NEAR(viewport.scale(), 3.0, kEps);
    expectNear(viewport.screenToWorld(anchor), before);
}

TEST(Viewport, PanFollowsTheMouse) {
    Viewport viewport = makeViewport();
    const Vec2 grabbed = viewport.screenToWorld({100.0, 100.0});
    viewport.panByPixels({30.0, -40.0});
    expectNear(viewport.worldToScreen(grabbed), {130.0, 60.0});
}

TEST(Viewport, ZoomToBoxFitsWithMargin) {
    Viewport viewport = makeViewport();
    viewport.zoomToBox(BBox{{0.0, 0.0}, {100.0, 50.0}}, 0.0);
    EXPECT_NEAR(viewport.scale(), 8.0, kEps);  // limited by width: 800 / 100
    expectNear(viewport.center(), {50.0, 25.0});
    const BBox visible = viewport.visibleWorldArea();
    EXPECT_NEAR(visible.width(), 100.0, kEps);
}

TEST(Viewport, ZoomToEmptyBoxChangesNothing) {
    Viewport viewport = makeViewport();
    viewport.zoomToBox(BBox{});
    EXPECT_NEAR(viewport.scale(), 2.0, kEps);
    expectNear(viewport.center(), {10.0, 20.0});
}

TEST(GridSpacing, UsesOneTwoFiveSequence) {
    EXPECT_NEAR(gridSpacing(1.0, 12.0), 20.0, kEps);
    EXPECT_NEAR(gridSpacing(10.0, 12.0), 2.0, kEps);
    EXPECT_NEAR(gridSpacing(3.0, 12.0), 5.0, kEps);
    EXPECT_NEAR(gridSpacing(12.0, 12.0), 1.0, kEps);
    EXPECT_NEAR(gridSpacing(1000.0, 12.0), 0.02, kEps);
}

/// Painter that records calls instead of drawing.
class RecordingPainter final : public Painter {
public:
    void clear(const Color& background) override {
        clearedWith = background;
        ++clears;
    }

    void setPen(const Color& color, double widthPx, LineStyle style) override {
        pen = color;
        penWidth = widthPx;
        penStyle = style;
    }

    void fillPolygon(std::span<const Vec2> points, const Color& color) override {
        fills.emplace_back(points.begin(), points.end());
        fillColor = color;
        polylinesAtFill = polylines.size();
    }

    void drawText(const TextRun& run) override { texts.push_back(run); }

    std::vector<TextRun> texts;

    void drawLine(const Vec2& from, const Vec2& to) override {
        ++lines;
        segments.emplace_back(from, to);
    }

    void drawPolyline(std::span<const Vec2> points, bool closed) override {
        polylines.emplace_back(points.begin(), points.end());
        lastClosed = closed;
        polylinePen = pen;
        polylineStyle = penStyle;
        polylineWidth = penWidth;
    }

    LineStyle penStyle{LineStyle::Solid};
    LineStyle polylineStyle{LineStyle::Solid};
    double penWidth{0.0};
    double polylineWidth{0.0};
    Color fillColor;
    std::size_t polylinesAtFill{0};
    std::vector<std::vector<Vec2>> fills;
    Color clearedWith;
    Color pen;
    Color polylinePen;
    int clears{0};
    int lines{0};
    std::vector<std::pair<Vec2, Vec2>> segments;  ///< every drawLine(), in order
    bool lastClosed{false};
    std::vector<std::vector<Vec2>> polylines;
};

TEST(SceneRenderer, DrawsVisiblePolylinesInScreenSpace) {
    core::Document document;
    core::Polyline polyline;
    polyline.addVertex({10.0, 20.0});
    polyline.addVertex({11.0, 21.0});
    document.add(polyline);

    const SceneStyle style;
    RecordingPainter painter;
    renderScene(document, makeViewport(), style, painter);

    EXPECT_EQ(painter.clearedWith, style.background);
    EXPECT_GT(painter.lines, 0);  // grid and axes
    ASSERT_EQ(painter.polylines.size(), 1U);
    EXPECT_EQ(painter.polylinePen, style.entity);
    ASSERT_EQ(painter.polylines.front().size(), 2U);
    expectNear(painter.polylines.front()[0], {400.0, 300.0});
    expectNear(painter.polylines.front()[1], {402.0, 298.0});
}

TEST(SceneRenderer, DrawsLinesCirclesAndArcs) {
    core::Document document;
    document.add(core::Line{{10.0, 20.0}, {20.0, 20.0}});
    document.add(core::Circle{{10.0, 20.0}, 5.0});
    document.add(core::Arc{{10.0, 20.0}, 5.0, 0.0, core::kPi / 2.0});

    const SceneStyle style;
    RecordingPainter painter;
    renderScene(document, makeViewport(), style, painter);
    ASSERT_EQ(painter.polylines.size(), 3U);

    // The line: its two end points, open.
    const std::vector<Vec2>& line = painter.polylines[0];
    ASSERT_EQ(line.size(), 2U);
    expectNear(line[0], {400.0, 300.0});
    expectNear(line[1], {420.0, 300.0});

    // The circle: a closed polyline whose points lie on it (radius 10 px),
    // without repeating the first point at the end.
    const std::vector<Vec2>& circle = painter.polylines[1];
    ASSERT_GT(circle.size(), 8U);
    for (std::size_t i = 0; i < circle.size(); ++i) {
        EXPECT_NEAR(core::distance(circle[i], {400.0, 300.0}), 10.0, kEps);
        // Every chord, the closing one included, within the tolerance.
        const Vec2 middle = (circle[i] + circle[(i + 1) % circle.size()]) * 0.5;
        EXPECT_LE(10.0 - core::distance(middle, {400.0, 300.0}),
                  style.tessellationTolerancePx + kEps);
    }
    expectNear(circle.front(), {410.0, 300.0});
    EXPECT_GT(core::distance(circle.back(), circle.front()), 0.1);

    // The arc: open, from (15,20) counter-clockwise to (10,25); Y points
    // down on screen.
    const std::vector<Vec2>& arc = painter.polylines[2];
    ASSERT_GT(arc.size(), 4U);
    expectNear(arc.front(), {410.0, 300.0});
    expectNear(arc.back(), {400.0, 290.0});
    EXPECT_FALSE(painter.lastClosed);
}

TEST(SceneRenderer, CircleIsDrawnClosed) {
    core::Document document;
    document.add(core::Circle{{10.0, 20.0}, 5.0});
    RecordingPainter painter;
    renderScene(document, makeViewport(), SceneStyle{}, painter);
    ASSERT_EQ(painter.polylines.size(), 1U);
    EXPECT_TRUE(painter.lastClosed);
}

TEST(PreviewRenderer, DrawsTheNewEntitiesAndSkipsThoseOutsideTheView) {
    const std::array<core::Entity, 3> entities{core::Line{{10.0, 20.0}, {11.0, 21.0}},
                                               core::Circle{{10.0, 20.0}, 1.0},
                                               core::Arc{{1e6, 1e6}, 1.0, 0.0, 1.0}};  // far away
    RecordingPainter painter;
    renderPreview(entities, makeViewport(), SceneStyle{}, painter);
    EXPECT_EQ(painter.polylines.size(), 2U);
    EXPECT_EQ(painter.polylinePen, SceneStyle{}.preview);
}

TEST(HighlightRenderer, DrawsTheNewEntitiesDashed) {
    core::Document document;
    const core::EntityId arc = document.add(core::Arc{{10.0, 20.0}, 5.0, 0.0, 1.0});
    const std::array ids{arc};
    RecordingPainter painter;
    renderHighlight(document, ids, makeViewport(), SceneStyle{}, painter);
    ASSERT_EQ(painter.polylines.size(), 1U);
    EXPECT_EQ(painter.polylineStyle, LineStyle::Dashed);
}

TEST(SceneRenderer, SkipsEntitiesOutsideTheView) {
    core::Document document;
    core::Polyline faraway;
    faraway.addVertex({1e6, 1e6});
    faraway.addVertex({1e6 + 1.0, 1e6});
    document.add(faraway);

    RecordingPainter painter;
    renderScene(document, makeViewport(), SceneStyle{}, painter);
    EXPECT_TRUE(painter.polylines.empty());
}

TEST(PreviewRenderer, DrawsInScreenSpaceWithoutClearingOrDrawingGridAndAxes) {
    core::Polyline polyline;
    polyline.addVertex({10.0, 20.0});
    polyline.addVertex({11.0, 21.0});
    const std::vector<core::Entity> entities{polyline};
    const SceneStyle style;
    const Color expectedPreview{230, 200, 90};
    RecordingPainter painter;

    renderPreview(entities, makeViewport(), style, painter);

    EXPECT_EQ(style.preview, expectedPreview);
    EXPECT_EQ(painter.polylinePen, style.preview);
    EXPECT_EQ(painter.clears, 0);
    EXPECT_EQ(painter.lines, 0);
    EXPECT_FALSE(painter.lastClosed);
    ASSERT_EQ(painter.polylines.size(), 1U);
    ASSERT_EQ(painter.polylines.front().size(), 2U);
    expectNear(painter.polylines.front()[0], {400.0, 300.0});
    expectNear(painter.polylines.front()[1], {402.0, 298.0});
}

TEST(PreviewRenderer, UsesCustomColorAndSharedArcTessellation) {
    core::Polyline polyline;
    polyline.addVertex({10.0, 20.0}, 1.0);
    polyline.addVertex({30.0, 20.0});
    polyline.setClosed(true);
    const std::vector<core::Entity> entities{polyline};
    core::Document document;
    document.add(polyline);
    SceneStyle style;
    style.preview = {100, 150, 200};
    style.tessellationTolerancePx = 0.1;
    RecordingPainter scenePainter;
    RecordingPainter previewPainter;

    renderScene(document, makeViewport(), style, scenePainter);
    renderPreview(entities, makeViewport(), style, previewPainter);

    EXPECT_EQ(previewPainter.polylinePen, style.preview);
    EXPECT_TRUE(previewPainter.lastClosed);
    ASSERT_EQ(previewPainter.polylines.size(), 1U);
    EXPECT_GT(previewPainter.polylines.front().size(), 2U);
    EXPECT_EQ(previewPainter.polylines, scenePainter.polylines);
}

TEST(PreviewRenderer, EmptyPreviewDoesNotDrawOrClear) {
    RecordingPainter painter;
    renderPreview({}, makeViewport(), SceneStyle{}, painter);
    EXPECT_EQ(painter.clears, 0);
    EXPECT_EQ(painter.lines, 0);
    EXPECT_TRUE(painter.polylines.empty());
}

TEST(PreviewRenderer, SkipsEntitiesOutsideTheView) {
    core::Polyline faraway;
    faraway.addVertex({1e6, 1e6});
    faraway.addVertex({1e6 + 1.0, 1e6});
    const std::vector<core::Entity> entities{faraway};
    RecordingPainter painter;
    renderPreview(entities, makeViewport(), SceneStyle{}, painter);
    EXPECT_EQ(painter.clears, 0);
    EXPECT_EQ(painter.lines, 0);
    EXPECT_TRUE(painter.polylines.empty());
}

TEST(HighlightRenderer, UsesSharedTessellationAndSkipsAbsentAndOffscreenIds) {
    core::Document document;
    core::Polyline arc;
    arc.addVertex({10, 20}, 1);
    arc.addVertex({30, 20});
    arc.setClosed(true);
    const auto selected = document.add(arc);
    core::Polyline other;
    other.addVertex({0, 0});
    other.addVertex({1, 1});
    const auto removed = document.add(other);
    document.remove(removed);
    core::Polyline faraway;
    faraway.addVertex({1e6, 1e6});
    faraway.addVertex({1e6 + 1, 1e6});
    const auto outside = document.add(faraway);
    const std::array ids{removed, selected, outside};
    SceneStyle style;
    style.highlight = {1, 2, 3, 140};
    style.tessellationTolerancePx = 0.1;
    RecordingPainter highlight;
    RecordingPainter scene;
    RecordingPainter preview;
    const std::array<core::Entity, 1> entities{arc};
    renderHighlight(document, ids, makeViewport(), style, highlight);
    renderScene(document, makeViewport(), style, scene);
    renderPreview(entities, makeViewport(), style, preview);
    EXPECT_EQ(highlight.polylines, scene.polylines);
    EXPECT_EQ(highlight.polylines, preview.polylines);
    ASSERT_EQ(highlight.polylines.size(), 1U);
    EXPECT_GT(highlight.polylines.front().size(), 2U);
    EXPECT_TRUE(highlight.lastClosed);
    EXPECT_EQ(highlight.polylinePen, style.highlight);
    EXPECT_EQ(highlight.polylineStyle, LineStyle::Dashed);
    EXPECT_EQ(highlight.clears, 0);
    EXPECT_EQ(highlight.lines, 0);
    EXPECT_TRUE(highlight.fills.empty());
}

TEST(HighlightRenderer, EmptySelectionAndAbsentIdsDoNotDraw) {
    const core::Document document;
    RecordingPainter painter;
    renderHighlight(document, {}, makeViewport(), SceneStyle{}, painter);
    const std::array ids{core::EntityId{999}};
    renderHighlight(document, ids, makeViewport(), SceneStyle{}, painter);
    EXPECT_TRUE(painter.polylines.empty());
    EXPECT_TRUE(painter.fills.empty());
    EXPECT_EQ(painter.clears, 0);
    EXPECT_EQ(painter.lines, 0);
}

TEST(SelectionRectangleRenderer, WindowAndCrossingFillBeforeClosedBorder) {
    const Viewport viewport = makeViewport();
    SceneStyle style;
    style.windowFill = {1, 2, 3, 40};
    style.windowBorder = {4, 5, 6};
    style.crossingFill = {7, 8, 9, 50};
    style.crossingBorder = {10, 11, 12};
    for (const bool crossing : {false, true}) {
        for (const bool reverse : {false, true}) {
            const Vec2 first = reverse ? Vec2{30, 40} : Vec2{10, 20};
            const Vec2 second = reverse ? Vec2{10, 20} : Vec2{30, 40};
            RecordingPainter painter;
            renderSelectionRectangle(first, second, crossing, viewport, style, painter);
            ASSERT_EQ(painter.fills.size(), 1U);
            ASSERT_EQ(painter.polylines.size(), 1U);
            EXPECT_EQ(painter.fills.front(), painter.polylines.front());
            ASSERT_EQ(painter.polylines.front().size(), 4U);
            const Vec2 a = viewport.worldToScreen(first);
            const Vec2 b = viewport.worldToScreen(second);
            const std::vector<Vec2> expected{a, {b.x, a.y}, b, {a.x, b.y}};
            EXPECT_EQ(painter.polylines.front(), expected);
            EXPECT_EQ(painter.fillColor, crossing ? style.crossingFill : style.windowFill);
            EXPECT_EQ(painter.polylinePen, crossing ? style.crossingBorder : style.windowBorder);
            EXPECT_EQ(painter.polylineStyle, crossing ? LineStyle::Dashed : LineStyle::Solid);
            EXPECT_EQ(painter.polylinesAtFill, 0U);
            EXPECT_TRUE(painter.lastClosed);
            EXPECT_EQ(painter.clears, 0);
            EXPECT_EQ(painter.lines, 0);
        }
    }
}

TEST(SelectionRectangleRenderer, DegenerateRectanglesRemainFinite) {
    for (const Vec2 second : std::array{Vec2{10, 20}, Vec2{10, 30}, Vec2{20, 20}}) {
        RecordingPainter painter;
        renderSelectionRectangle({10, 20}, second, false, makeViewport(), SceneStyle{}, painter);
        ASSERT_EQ(painter.polylines.size(), 1U);
        for (const Vec2 point : painter.polylines.front()) {
            EXPECT_TRUE(std::isfinite(point.x));
            EXPECT_TRUE(std::isfinite(point.y));
        }
        EXPECT_EQ(painter.clears, 0);
    }
}

TEST(SnapMarkerRenderer, ShapesStayCenteredAndKeepPixelSizeAcrossZoom) {
    const std::array kinds{core::SnapKind::Endpoint, core::SnapKind::Midpoint,
                           core::SnapKind::Center};
    for (const auto kind : kinds) {
        for (const double size : {10.0, 24.0}) {
            for (const double scale : {0.01, 2.0, 1000.0}) {
                Viewport viewport = makeViewport();
                viewport.setScale(scale);
                SceneStyle style;
                style.snapMarkerSizePx = size;
                style.snapMarker = {12, 34, 56, 200};
                const core::SnapPoint snap{{11, 21}, kind};
                const Vec2 center = viewport.worldToScreen(snap.point);
                const double half = size * 0.5;
                RecordingPainter painter;
                renderSnapMarker(snap, viewport, style, painter);
                ASSERT_EQ(painter.polylines.size(), 1U);
                const auto& points = painter.polylines.front();
                BBox box;
                for (const Vec2 point : points) {
                    box.expand(point);
                }
                expectNear(box.min(), center - Vec2{half, half});
                expectNear(box.max(), center + Vec2{half, half});
                if (kind == core::SnapKind::Endpoint) {
                    ASSERT_EQ(points.size(), 4U);
                    expectNear(points[0], center + Vec2{-half, -half});
                    expectNear(points[1], center + Vec2{half, -half});
                    expectNear(points[2], center + Vec2{half, half});
                    expectNear(points[3], center + Vec2{-half, half});
                } else if (kind == core::SnapKind::Midpoint) {
                    ASSERT_EQ(points.size(), 3U);
                    expectNear(points[0], center + Vec2{0, -half});
                    expectNear(points[1], center + Vec2{half, half});
                    expectNear(points[2], center + Vec2{-half, half});
                } else {
                    EXPECT_GE(points.size(), 16U);
                    for (const Vec2 point : points) {
                        EXPECT_NEAR(core::distance(point, center), half, kEps);
                    }
                }
                EXPECT_TRUE(painter.lastClosed);
                EXPECT_EQ(painter.polylinePen, style.snapMarker);
                EXPECT_EQ(painter.polylineStyle, LineStyle::Solid);
                EXPECT_DOUBLE_EQ(painter.polylineWidth, 2.0);
                EXPECT_EQ(painter.clears, 0);
                EXPECT_EQ(painter.lines, 0);
                EXPECT_TRUE(painter.fills.empty());
            }
        }
    }
}

TEST(SceneRenderer, DrawsTextsWithTheirHeightRotationAndWidthFactor) {
    core::Document document;
    core::Text text;
    text.position = {10.0, 20.0};
    text.height = 2.5;
    text.rotation = 0.5;
    text.widthFactor = 0.8;
    text.value = "%%c20";
    // Preserve the original Left/baseline rendering as a regression test.
    document.add(text);

    const SceneStyle style;
    RecordingPainter painter;
    renderScene(document, makeViewport(), style, painter);
    ASSERT_EQ(painter.texts.size(), 1U);
    EXPECT_TRUE(painter.polylines.empty());
    EXPECT_EQ(painter.pen, style.entity);
    const TextRun& run = painter.texts.front();
    EXPECT_EQ(run.text, U"\u230020");  // control codes converted
    expectNear(run.origin, {400.0, 300.0});
    EXPECT_DOUBLE_EQ(run.capHeightPx, 5.0);  // scale 2
    EXPECT_DOUBLE_EQ(run.angle, 0.5);
    EXPECT_DOUBLE_EQ(run.widthFactor, 0.8);
}

TEST(SceneRenderer, SmallTextsAreDrawnAsTheirBox) {
    core::Text text;
    text.position = {10.0, 20.0};
    text.value = "Hello";
    // At scale 2: 0.99 is drawn as a box, 1.0 (2 px) as text.
    text.height = 0.99;
    const std::array<core::Entity, 1> small{text};
    RecordingPainter painter;
    renderPreview(small, makeViewport(), SceneStyle{}, painter);
    EXPECT_TRUE(painter.texts.empty());
    ASSERT_EQ(painter.polylines.size(), 1U);
    EXPECT_TRUE(painter.lastClosed);
    const std::array<Vec2, 4> box = core::textBox(text);
    ASSERT_EQ(painter.polylines.front().size(), box.size());
    for (std::size_t i = 0; i < box.size(); ++i) {
        expectNear(painter.polylines.front()[i], makeViewport().worldToScreen(box.at(i)));
    }

    text.height = 1.0;
    const std::array<core::Entity, 1> readable{text};
    RecordingPainter other;
    renderPreview(readable, makeViewport(), SceneStyle{}, other);
    EXPECT_TRUE(other.polylines.empty());
    ASSERT_EQ(other.texts.size(), 1U);
    EXPECT_DOUBLE_EQ(other.texts.front().capHeightPx, kMinTextHeightPx);
}

TEST(SceneRenderer, TextsOutsideTheViewAndHighlightedTexts) {
    core::Document document;
    core::Text faraway;
    faraway.position = {1e6, 1e6};
    faraway.value = "Far";
    document.add(faraway);
    core::Text near;
    near.position = {10.0, 20.0};
    near.value = "Near";
    const core::EntityId id = document.add(near);
    RecordingPainter scene;
    renderScene(document, makeViewport(), SceneStyle{}, scene);
    ASSERT_EQ(scene.texts.size(), 1U);
    EXPECT_EQ(scene.texts.front().text, U"Near");

    const std::array ids{id};
    RecordingPainter highlight;
    renderHighlight(document, ids, makeViewport(), SceneStyle{}, highlight);
    ASSERT_EQ(highlight.texts.size(), 1U);
    EXPECT_EQ(highlight.pen, SceneStyle{}.highlight);
}

TEST(SnapMarkerRenderer, InsertionIsTwoOverlappingOffsetSquares) {
    for (const double size : {10.0, 24.0}) {
        SceneStyle style;
        style.snapMarkerSizePx = size;
        const Viewport viewport = makeViewport();
        const core::SnapPoint snap{{11, 21}, core::SnapKind::Insertion};
        const Vec2 center = viewport.worldToScreen(snap.point);
        const double half = size * 0.5;
        RecordingPainter painter;
        renderSnapMarker(snap, viewport, style, painter);
        ASSERT_EQ(painter.polylines.size(), 2U);
        BBox all;
        std::array<BBox, 2> squares;
        for (std::size_t i = 0; i < 2; ++i) {
            const auto& points = painter.polylines.at(i);
            ASSERT_EQ(points.size(), 4U);
            for (const Vec2 point : points) {
                squares.at(i).expand(point);
                all.expand(point);
            }
            EXPECT_NEAR(squares.at(i).width(), squares.at(i).height(), kEps);
            EXPECT_LT(squares.at(i).width(), size);  // smaller than the endpoint square
        }
        // Together they fill the marker, centred on the snap point.
        expectNear(all.min(), center - Vec2{half, half});
        expectNear(all.max(), center + Vec2{half, half});
        // Offset, and overlapping.
        EXPECT_NE(squares[0].min(), squares[1].min());
        EXPECT_LT(squares[1].min().x, squares[0].max().x);
        EXPECT_LT(squares[1].min().y, squares[0].max().y);
        EXPECT_TRUE(painter.lastClosed);
        EXPECT_EQ(painter.polylinePen, style.snapMarker);
        EXPECT_DOUBLE_EQ(painter.polylineWidth, 2.0);
    }
}

// The markers of docs/adr/0033, point 1: a cross, the perpendicular symbol, a
// circle with a tangent line, a diamond. Each one is centred on the snap
// point and fills a square of snapMarkerSizePx, whatever the zoom.
TEST(SnapMarkerRenderer, IntersectionIsAnXCrossOfTwoDiagonals) {
    for (const double size : {10.0, 24.0}) {
        SceneStyle style;
        style.snapMarkerSizePx = size;
        const Viewport viewport = makeViewport();
        const core::SnapPoint snap{{11, 21}, core::SnapKind::Intersection};
        const Vec2 center = viewport.worldToScreen(snap.point);
        const double half = size * 0.5;
        RecordingPainter painter;
        renderSnapMarker(snap, viewport, style, painter);
        EXPECT_TRUE(painter.polylines.empty());
        ASSERT_EQ(painter.segments.size(), 2U);
        expectNear(painter.segments[0].first, center + Vec2{-half, -half});
        expectNear(painter.segments[0].second, center + Vec2{half, half});
        expectNear(painter.segments[1].first, center + Vec2{half, -half});
        expectNear(painter.segments[1].second, center + Vec2{-half, half});
        EXPECT_EQ(painter.pen, style.snapMarker);
        EXPECT_DOUBLE_EQ(painter.penWidth, 2.0);
        EXPECT_EQ(painter.penStyle, LineStyle::Solid);
    }
}

TEST(SnapMarkerRenderer, PerpendicularIsABaseWithAStem) {
    for (const double size : {10.0, 24.0}) {
        SceneStyle style;
        style.snapMarkerSizePx = size;
        const Viewport viewport = makeViewport();
        const core::SnapPoint snap{{11, 21}, core::SnapKind::Perpendicular};
        const Vec2 center = viewport.worldToScreen(snap.point);
        const double half = size * 0.5;
        RecordingPainter painter;
        renderSnapMarker(snap, viewport, style, painter);
        EXPECT_TRUE(painter.polylines.empty());
        ASSERT_EQ(painter.segments.size(), 2U);
        // The base along the bottom, the stem from its middle to the top.
        expectNear(painter.segments[0].first, center + Vec2{-half, half});
        expectNear(painter.segments[0].second, center + Vec2{half, half});
        expectNear(painter.segments[1].first, center + Vec2{0.0, half});
        expectNear(painter.segments[1].second, center + Vec2{0.0, -half});
        EXPECT_EQ(painter.pen, style.snapMarker);
        EXPECT_DOUBLE_EQ(painter.penWidth, 2.0);
    }
}

TEST(SnapMarkerRenderer, TangentIsACircleUnderATangentLine) {
    for (const double size : {10.0, 24.0}) {
        SceneStyle style;
        style.snapMarkerSizePx = size;
        const Viewport viewport = makeViewport();
        const core::SnapPoint snap{{11, 21}, core::SnapKind::Tangent};
        const Vec2 center = viewport.worldToScreen(snap.point);
        const double half = size * 0.5;
        RecordingPainter painter;
        renderSnapMarker(snap, viewport, style, painter);
        ASSERT_EQ(painter.polylines.size(), 1U);
        ASSERT_EQ(painter.segments.size(), 1U);
        EXPECT_TRUE(painter.lastClosed);
        const auto& circle = painter.polylines.front();
        EXPECT_GE(circle.size(), 16U);
        for (const Vec2 point : circle) {
            EXPECT_NEAR(core::distance(point, center), half, kEps);
        }
        // The line runs along the top of the marker, touching the circle at
        // its highest point.
        const auto& [from, to] = painter.segments.front();
        expectNear(from, center + Vec2{-half, -half});
        expectNear(to, center + Vec2{half, -half});
        const double top = std::ranges::min(circle | std::views::transform(&Vec2::y));
        EXPECT_NEAR(top, from.y, kEps);
        EXPECT_EQ(painter.polylinePen, style.snapMarker);
        EXPECT_DOUBLE_EQ(painter.polylineWidth, 2.0);
    }
}

TEST(SnapMarkerRenderer, QuadrantIsADiamond) {
    for (const double size : {10.0, 24.0}) {
        SceneStyle style;
        style.snapMarkerSizePx = size;
        const Viewport viewport = makeViewport();
        const core::SnapPoint snap{{11, 21}, core::SnapKind::Quadrant};
        const Vec2 center = viewport.worldToScreen(snap.point);
        const double half = size * 0.5;
        RecordingPainter painter;
        renderSnapMarker(snap, viewport, style, painter);
        EXPECT_EQ(painter.lines, 0);
        ASSERT_EQ(painter.polylines.size(), 1U);
        const auto& points = painter.polylines.front();
        ASSERT_EQ(points.size(), 4U);
        expectNear(points[0], center + Vec2{0.0, -half});
        expectNear(points[1], center + Vec2{half, 0.0});
        expectNear(points[2], center + Vec2{0.0, half});
        expectNear(points[3], center + Vec2{-half, 0.0});
        EXPECT_TRUE(painter.lastClosed);
        EXPECT_EQ(painter.polylinePen, style.snapMarker);
        EXPECT_EQ(painter.polylineStyle, LineStyle::Solid);
        EXPECT_DOUBLE_EQ(painter.polylineWidth, 2.0);
    }
}

TEST(SceneRenderer, SceneAndPreviewResetDashedOverlayPenToSolid) {
    core::Polyline line;
    line.addVertex({10, 20});
    line.addVertex({11, 21});
    core::Document document;
    const auto id = document.add(line);
    const std::array ids{id};
    const std::array<core::Entity, 1> entities{line};
    RecordingPainter painter;
    const SceneStyle style;
    renderHighlight(document, ids, makeViewport(), style, painter);
    renderScene(document, makeViewport(), style, painter);
    EXPECT_EQ(painter.polylineStyle, LineStyle::Solid);
    EXPECT_EQ(painter.polylinePen, style.entity);
    EXPECT_DOUBLE_EQ(painter.polylineWidth, 0.0);
    renderHighlight(document, ids, makeViewport(), style, painter);
    renderPreview(entities, makeViewport(), style, painter);
    EXPECT_EQ(painter.polylineStyle, LineStyle::Solid);
    EXPECT_EQ(painter.polylinePen, style.preview);
    EXPECT_DOUBLE_EQ(painter.polylineWidth, 0.0);
}

TEST(SceneStyle, OverlayDefaultsMatchTheStagePlan) {
    const SceneStyle style;
    EXPECT_EQ(style.highlight, (Color{90, 160, 255}));
    EXPECT_EQ(style.windowFill, (Color{40, 90, 200, 60}));
    EXPECT_EQ(style.windowBorder, (Color{80, 140, 255}));
    EXPECT_EQ(style.crossingFill, (Color{40, 160, 70, 60}));
    EXPECT_EQ(style.crossingBorder, (Color{90, 210, 110}));
    EXPECT_EQ(style.snapMarker, (Color{255, 170, 40}));
    EXPECT_DOUBLE_EQ(style.snapMarkerSizePx, 10.0);
}

}  // namespace
}  // namespace tg::render
