// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/render/viewport.hpp"

#include "tg/render/scene_renderer.hpp"

#include <gtest/gtest.h>

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
    void clear(const Color& background) override { clearedWith = background; }

    void setPen(const Color& color, double /*widthPx*/) override { pen = color; }

    void drawLine(const Vec2& /*from*/, const Vec2& /*to*/) override { ++lines; }

    void drawPolyline(std::span<const Vec2> points, bool closed) override {
        polylines.emplace_back(points.begin(), points.end());
        lastClosed = closed;
        polylinePen = pen;
    }

    Color clearedWith;
    Color pen;
    Color polylinePen;
    int lines{0};
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

}  // namespace
}  // namespace tg::render
