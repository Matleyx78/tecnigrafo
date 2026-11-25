// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "generated_drawing.hpp"

#include "tg/core/document.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/math.hpp"
#include "tg/render/scene_renderer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace tg::render {
namespace {

using core::Vec2;

constexpr double kEps = 1e-9;

/// Records every call, in order.
class ShapePainter final : public Painter {
public:
    struct Stroke {
        std::vector<Vec2> points;
        Color color;
        double widthPx{0.0};
        LineStyle style{LineStyle::Solid};
        bool line{false};  ///< drawLine, otherwise drawPolyline
        bool closed{false};
    };

    std::vector<Stroke> strokes;
    std::vector<std::vector<Vec2>> fills;
    std::vector<Color> fillColors;
    std::vector<std::vector<std::vector<Vec2>>> loopFills;
    std::vector<Color> loopFillColors;
    std::vector<TextRun> runs;
    Color pen;
    double penWidth{0.0};
    LineStyle penStyle{LineStyle::Solid};

    void clear(const Color& /*background*/) override {}

    void setPen(const Color& color, double widthPx, LineStyle style) override {
        pen = color;
        penWidth = widthPx;
        penStyle = style;
    }

    void fillPolygon(std::span<const Vec2> points, const Color& color) override {
        fills.emplace_back(points.begin(), points.end());
        fillColors.push_back(color);
    }

    void fillLoops(std::span<const std::vector<Vec2>> loops, const Color& color) override {
        loopFills.emplace_back(loops.begin(), loops.end());
        loopFillColors.push_back(color);
    }

    void drawText(const TextRun& run) override { runs.push_back(run); }

    void drawLine(const Vec2& from, const Vec2& to) override {
        strokes.push_back({{from, to}, pen, penWidth, penStyle, true, false});
    }

    void drawPolyline(std::span<const Vec2> points, bool closed) override {
        strokes.push_back({{points.begin(), points.end()}, pen, penWidth, penStyle, false, closed});
    }
};

// Screen x = 400 + 2 x, y = 300 - 2 y.
Viewport testViewport() {
    Viewport viewport;
    viewport.setScreenSize(800.0, 600.0);
    viewport.setScale(2.0);
    viewport.setCenter({0.0, 0.0});
    return viewport;
}

void expectPoint(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kEps);
    EXPECT_NEAR(actual.y, expected.y, kEps);
}

core::Polyline square(double half) {
    core::Polyline loop;
    loop.addVertex({-half, -half});
    loop.addVertex({half, -half});
    loop.addVertex({half, half});
    loop.addVertex({-half, half});
    loop.setClosed(true);
    return loop;
}

/// Items with properties pointing to a linetype of this object.
struct Items {
    core::Linetype continuous;
    core::Linetype dashed;

    Items() {
        dashed.name = "Dashed";
        dashed.pattern.elements = {4.0, -2.0};
    }

    [[nodiscard]] core::GeneratedItem item(core::GeneratedShape shape,
                                           bool isDashed = false) const {
        core::EffectiveProperties properties;
        properties.color = {10, 20, 30};
        properties.linetype = isDashed ? &dashed : &continuous;
        return {std::move(shape), properties};
    }
};

TEST(GeneratedDrawing, FilledRegionWithAnIslandFillsTwoLoops) {
    const Items items;
    core::FilledRegion region;
    region.loops = {square(10.0), square(4.0)};
    ShapePainter painter;
    const SceneStyle style;
    drawGeneratedItem(items.item(region), false, testViewport(), style, painter);
    ASSERT_EQ(painter.loopFills.size(), 1U);
    EXPECT_EQ(painter.loopFillColors[0], (Color{10, 20, 30}));
    const auto& loops = painter.loopFills[0];
    ASSERT_EQ(loops.size(), 2U);
    ASSERT_EQ(loops[0].size(), 4U);
    ASSERT_EQ(loops[1].size(), 4U);
    expectPoint(loops[0][0], {380.0, 320.0});
    expectPoint(loops[0][2], {420.0, 280.0});
    expectPoint(loops[1][0], {392.0, 308.0});
    expectPoint(loops[1][2], {408.0, 292.0});
    EXPECT_TRUE(painter.strokes.empty());
    EXPECT_TRUE(painter.fills.empty());
}

TEST(GeneratedDrawing, FilledRegionUnderANonUniformScalingIsWidenedInX) {
    const Items items;
    core::FilledRegion region;
    region.loops = {square(10.0), square(4.0)};
    region.transform = core::Transform2D::scaling(2.0, 1.0);
    ShapePainter painter;
    drawGeneratedItem(items.item(region), false, testViewport(), SceneStyle{}, painter);
    ASSERT_EQ(painter.loopFills.size(), 1U);
    const auto& loops = painter.loopFills[0];
    ASSERT_EQ(loops.size(), 2U);
    // x doubled: the outer square spans x from -20 to 20, y from -10 to 10.
    expectPoint(loops[0][0], {360.0, 320.0});
    expectPoint(loops[0][2], {440.0, 280.0});
    expectPoint(loops[1][0], {384.0, 308.0});
    expectPoint(loops[1][2], {416.0, 292.0});
}

TEST(GeneratedDrawing, SolidFillsTheQuadrilateralInTheOrderOfDxf) {
    const Items items;
    const core::Solid solid{{Vec2{0.0, 0.0}, Vec2{10.0, 0.0}, Vec2{0.0, 5.0}, Vec2{10.0, 5.0}}};
    ShapePainter painter;
    drawGeneratedItem(items.item(solid), true, testViewport(), SceneStyle{}, painter);
    ASSERT_EQ(painter.fills.size(), 1U);
    EXPECT_EQ(painter.fillColors[0], (Color{10, 20, 30}));
    const auto& points = painter.fills[0];
    ASSERT_EQ(points.size(), 4U);
    // v0, v1, v3, v2: no butterfly.
    expectPoint(points[0], {400.0, 300.0});
    expectPoint(points[1], {420.0, 300.0});
    expectPoint(points[2], {420.0, 290.0});
    expectPoint(points[3], {400.0, 290.0});
    EXPECT_TRUE(painter.strokes.empty());
}

TEST(GeneratedDrawing, EllipseIsAPolylineWhosePointsLieOnTheEllipse) {
    const Items items;
    // Major axis of 20 turned by 30 degrees, ratio 0.5.
    const auto turn = core::sinCos(core::kPi / 6.0);
    const Vec2 center{3.0, -4.0};
    const Vec2 major = Vec2{turn.cosine, turn.sine} * 20.0;
    const core::Ellipse ellipse{center, major, 0.5, 0.0, core::kTwoPi};
    const auto viewport = testViewport();
    ShapePainter painter;
    drawGeneratedItem(items.item(ellipse), false, viewport, SceneStyle{}, painter);
    ASSERT_EQ(painter.strokes.size(), 1U);
    const auto& stroke = painter.strokes[0];
    EXPECT_FALSE(stroke.line);
    EXPECT_TRUE(stroke.closed);
    ASSERT_GT(stroke.points.size(), 16U);
    const Vec2 minorDirection{-turn.sine, turn.cosine};
    for (const Vec2& screen : stroke.points) {
        const Vec2 offset = viewport.screenToWorld(screen) - center;
        const double u = (offset.x * turn.cosine + offset.y * turn.sine) / 20.0;
        const double v = (offset.x * minorDirection.x + offset.y * minorDirection.y) / 10.0;
        EXPECT_NEAR(u * u + v * v, 1.0, 1e-9);
    }
    // The first point is the end of the major axis (parameter 0).
    expectPoint(stroke.points.front(), viewport.worldToScreen(center + major));
}

TEST(GeneratedDrawing, EllipticalArcRunsFromStartToEndParameter) {
    const core::Ellipse arc{{0.0, 0.0}, {10.0, 0.0}, 0.5, 0.0, core::kPi};
    const auto points = ellipsePoints(arc, 0.01);
    ASSERT_GE(points.size(), 3U);
    expectPoint(points.front(), {10.0, 0.0});
    expectPoint(points.back(), {-10.0, 0.0});
    EXPECT_FALSE(isFullEllipse(arc));
    for (const Vec2& point : points) {
        EXPECT_GE(point.y, -kEps);
    }
}

TEST(GeneratedDrawing, DegenerateEllipseDrawsNothingVisible) {
    const core::Ellipse ellipse{{1.0, 1.0}, {0.0, 0.0}, 0.5, 0.0, core::kTwoPi};
    EXPECT_TRUE(ellipsePoints(ellipse, 0.1).empty());
}

// A polyline with an arc and width 2, in an insert scaled (2, 1): core gives
// its pieces, a line and an elliptical arc, without width (docs/DECISIONI.md,
// 2026-11-14); here they come as core will give them.
TEST(GeneratedDrawing, PiecesOfAStretchedPolylineRestartThePatternWithoutWidth) {
    const Items items;
    const core::GeneratedShape line = core::Line{{0.0, 0.0}, {20.0, 0.0}};
    const core::GeneratedShape arc = core::Ellipse{{20.0, 5.0}, {0.0, -5.0}, 1.0, 0.0, core::kPi};
    ShapePainter painter;
    const auto viewport = testViewport();
    for (const auto& shape : {line, arc}) {
        const std::size_t first = painter.strokes.size();
        drawGeneratedItem(items.item(shape, true), false, viewport, SceneStyle{}, painter);
        ASSERT_GT(painter.strokes.size(), first + 1);
        // The first dash starts at the start of the piece: the pattern
        // starts again at every shape.
        const Vec2 start =
            std::holds_alternative<core::Line>(shape) ? Vec2{0.0, 0.0} : Vec2{20.0, 0.0};
        expectPoint(painter.strokes[first].points.front(), viewport.worldToScreen(start));
    }
    for (const auto& stroke : painter.strokes) {
        EXPECT_EQ(stroke.widthPx, 0.0);
        EXPECT_EQ(stroke.style, LineStyle::Solid);
    }
}

TEST(GeneratedDrawing, DashedEllipseFollowsItsPatternAcrossTheVertices) {
    const Items items;
    const core::Ellipse ellipse{{0.0, 0.0}, {30.0, 0.0}, 0.5, 0.0, core::kTwoPi};
    ShapePainter continuous;
    drawGeneratedItem(items.item(ellipse), false, testViewport(), SceneStyle{}, continuous);
    ShapePainter dashed;
    drawGeneratedItem(items.item(ellipse, true), false, testViewport(), SceneStyle{}, dashed);
    ASSERT_EQ(continuous.strokes.size(), 1U);
    // About 145 units of perimeter with a pattern of 6: some 24 dashes, each
    // longer than one edge of the polyline.
    EXPECT_GT(dashed.strokes.size(), 20U);
    EXPECT_LT(dashed.strokes.size(), 60U);
}

// Highlight and preview draw the generated shapes of the scene, in the colour
// of the selection.
TEST(GeneratedDrawing, HighlightAndPreviewDrawTheShapesOfTheScene) {
    core::Document document = core::Document::newDrawing();
    core::Polyline polyline;
    polyline.addVertex({-30.0, -10.0}, 0.5);
    polyline.addVertex({-10.0, -10.0});
    polyline.addVertex({-10.0, 10.0});
    const std::array<core::Entity, 4> entities{core::Line{{0.0, 0.0}, {20.0, 10.0}},
                                               core::Circle{{5.0, 5.0}, 7.0},
                                               core::Arc{{-5.0, 5.0}, 4.0, 0.2, 2.5}, polyline};
    std::vector<core::EntityId> ids;
    ids.reserve(entities.size());
    for (const auto& entity : entities) {
        ids.push_back(document.add(entity));
    }
    SceneStyle style;
    style.showGrid = false;
    style.axes = AxesStyle::Lines;
    style.highlight = {7, 8, 9};
    style.preview = {4, 5, 6};
    const auto viewport = testViewport();
    ShapePainter scene;
    renderScene(document, viewport, style, scene);
    ShapePainter highlight;
    renderHighlight(document, ids, viewport, style, highlight);
    ShapePainter preview;
    renderPreview(document, entities, viewport, style, preview);
    // The scene draws the two axes first.
    ASSERT_EQ(scene.strokes.size(), 2U + entities.size());
    ASSERT_EQ(highlight.strokes.size(), entities.size());
    ASSERT_EQ(preview.strokes.size(), entities.size());
    for (std::size_t i = 0; i < entities.size(); ++i) {
        const auto& expected = scene.strokes[2 + i];
        EXPECT_EQ(highlight.strokes[i].points, expected.points);
        EXPECT_EQ(highlight.strokes[i].closed, expected.closed);
        EXPECT_EQ(highlight.strokes[i].color, style.highlight);
        EXPECT_EQ(highlight.strokes[i].style, LineStyle::Dashed);
        EXPECT_EQ(preview.strokes[i].points, expected.points);
        EXPECT_EQ(preview.strokes[i].color, style.preview);
    }
}

/// An entity of a block, on the layer 0 and ByLayer, as a new drawing gives.
core::EntityRecord blockMember(const core::Document& document, core::Entity entity) {
    core::EntityRecord record{core::EntityId{}, std::move(entity)};
    record.properties.layer = document.layerZero();
    record.properties.linetype = document.byLayerLinetype();
    return record;
}

core::Insert insertOf(std::string name, const Vec2& insertion, double scaleX, double scaleY) {
    core::Insert insert;
    insert.block = std::move(name);
    insert.insertion = insertion;
    insert.scaleX = scaleX;
    insert.scaleY = scaleY;
    return insert;
}

// An insert, with a line, a circle and an arc, under a similarity and under a
// stretch: highlight and preview draw the generated shapes of the scene, in
// the colour of the selection (docs/tappe/0.8/PT16-blocchi-core.md, tests).
TEST(GeneratedDrawing, HighlightAndPreviewOfAnInsertDrawTheShapesOfTheScene) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition block{"B"};
    block.base = {1.0, 1.0};
    block.entities = {blockMember(document, core::Line{{0.0, 0.0}, {20.0, 10.0}}),
                      blockMember(document, core::Circle{{5.0, 5.0}, 7.0}),
                      blockMember(document, core::Arc{{-5.0, 5.0}, 4.0, 0.2, 2.5})};
    document.replaceBlock("B", block);
    const std::array<core::Entity, 2> inserts{insertOf("B", {-20.0, 10.0}, 2.0, 2.0),
                                              insertOf("B", {30.0, -20.0}, 2.0, 1.0)};
    const std::array<core::EntityId, 2> ids{document.add(inserts[0]), document.add(inserts[1])};
    SceneStyle style;
    style.showGrid = false;
    style.axes = AxesStyle::Lines;
    style.highlight = {7, 8, 9};
    style.preview = {4, 5, 6};
    const auto viewport = testViewport();
    ShapePainter scene;
    renderScene(document, viewport, style, scene);
    ShapePainter highlight;
    renderHighlight(document, ids, viewport, style, highlight);
    ShapePainter preview;
    renderPreview(document, inserts, viewport, style, preview);
    // The scene draws the two axes first, then three shapes for each insert.
    ASSERT_EQ(scene.strokes.size(), 2U + 6U);
    ASSERT_EQ(highlight.strokes.size(), 6U);
    ASSERT_EQ(preview.strokes.size(), 6U);
    for (std::size_t i = 0; i < highlight.strokes.size(); ++i) {
        const auto& expected = scene.strokes[2 + i];
        EXPECT_EQ(highlight.strokes[i].points, expected.points);
        EXPECT_EQ(highlight.strokes[i].closed, expected.closed);
        EXPECT_EQ(highlight.strokes[i].color, style.highlight);
        EXPECT_EQ(highlight.strokes[i].style, LineStyle::Dashed);
        EXPECT_EQ(preview.strokes[i].points, expected.points);
        EXPECT_EQ(preview.strokes[i].color, style.preview);
    }
    // The first line of the first insert: (0,0) - (1,1), times 2, plus (-20, 10).
    expectPoint(scene.strokes[2].points.front(), viewport.worldToScreen({-22.0, 8.0}));
}

// A polyline with an arc and width 2, dashed, in an insert scaled (2, 1): its
// pieces, a line and an elliptical arc, reach the painter without width, and
// the pattern starts again at the start of each (docs/DECISIONI.md,
// 2026-11-15, "Test di PT8 con un inserimento vero in PT16").
TEST(GeneratedDrawing, AStretchedPolylineOfAnInsertReachesThePainterInPieces) {
    core::Document document = core::Document::newDrawing();
    core::Linetype dash;
    dash.name = "Dash";
    // A dash of 3 and a gap of 2: the line piece, 4 long, ends in a gap, so
    // a pattern carried on would not start the arc with a dash.
    dash.pattern.elements = {3.0, -2.0};
    const core::LinetypeId dashId = document.addLinetype(dash);
    core::Polyline bent;
    bent.addVertex({6.0, 0.0});
    bent.addVertex({4.0, 0.0}, 1.0);
    bent.addVertex({0.0, 0.0});
    bent.setConstantWidth(2.0);
    core::EntityRecord member = blockMember(document, bent);
    member.properties.linetype = dashId;
    core::BlockDefinition block{"Shapes"};
    block.entities.push_back(member);
    document.replaceBlock("Shapes", block);
    const core::EntityId id = document.add(insertOf("Shapes", {10.0, 20.0}, 2.0, 1.0));
    const auto items = core::generate(document, *document.find(id));
    ASSERT_EQ(items.size(), 2U);
    EXPECT_TRUE(std::holds_alternative<core::Line>(items[0].shape));
    EXPECT_TRUE(std::holds_alternative<core::Ellipse>(items[1].shape));

    SceneStyle style;
    style.showGrid = false;
    style.axes = AxesStyle::Lines;
    const auto viewport = testViewport();
    ShapePainter painter;
    renderScene(document, viewport, style, painter);
    // No width: nothing filled, every stroke a cosmetic solid pen.
    EXPECT_TRUE(painter.fills.empty());
    EXPECT_TRUE(painter.loopFills.empty());
    ASSERT_GT(painter.strokes.size(), 2U);
    for (const auto& stroke : painter.strokes) {
        EXPECT_EQ(stroke.widthPx, 0.0);
        EXPECT_EQ(stroke.style, LineStyle::Solid);
    }
    // The line runs from (22, 20) to (18, 20), the arc from (18, 20): each
    // starts with a dash.
    const auto startsAt = [&](const Vec2& world) {
        const Vec2 screen = viewport.worldToScreen(world);
        return std::ranges::any_of(painter.strokes, [&](const ShapePainter::Stroke& stroke) {
            return std::abs(stroke.points.front().x - screen.x) < kEps &&
                   std::abs(stroke.points.front().y - screen.y) < kEps;
        });
    };
    EXPECT_TRUE(startsAt({22.0, 20.0}));
    EXPECT_TRUE(startsAt({18.0, 20.0}));
}

// The five entities of before the 0.8 come from core::generate() as they are:
// the scene draws them as it drew them before.
TEST(GeneratedDrawing, TheFiveEntitiesOfBeforeAreDrawnAsBefore) {
    core::Document document = core::Document::newDrawing();
    core::Text text;
    text.position = {1.0, 2.0};
    text.height = 5.0;
    text.value = "AB";
    document.add(core::Line{{0.0, 0.0}, {20.0, 10.0}});
    document.add(core::Circle{{5.0, 5.0}, 7.0});
    document.add(text);
    SceneStyle style;
    style.showGrid = false;
    style.axes = AxesStyle::Lines;
    const auto viewport = testViewport();
    ShapePainter painter;
    renderScene(document, viewport, style, painter);
    ASSERT_EQ(painter.strokes.size(), 4U);
    EXPECT_EQ(painter.strokes[2].points, (std::vector<Vec2>{viewport.worldToScreen({0.0, 0.0}),
                                                            viewport.worldToScreen({20.0, 10.0})}));
    EXPECT_FALSE(painter.strokes[2].closed);
    EXPECT_TRUE(painter.strokes[3].closed);
    EXPECT_EQ(painter.strokes[3].color, style.entity);
    ASSERT_EQ(painter.runs.size(), 1U);
    EXPECT_NEAR(painter.runs[0].capHeightPx, 10.0, kEps);
    EXPECT_TRUE(painter.fills.empty());
    EXPECT_TRUE(painter.loopFills.empty());
}

TEST(GeneratedDrawing, FormattedTextPassesItsFaceAndTrackingToThePainter) {
    const Items items;
    core::Text text;
    text.position = {1.0, 2.0};
    text.height = 5.0;
    text.widthFactor = 2.0;
    text.value = "AB";
    ShapePainter painter;
    drawGeneratedItem(items.item(core::FormattedText{text, core::FontFace::Bold, 1.0}), false,
                      testViewport(), SceneStyle{}, painter);
    drawGeneratedItem(items.item(core::FormattedText{text, core::FontFace::Italic, 2.0}), false,
                      testViewport(), SceneStyle{}, painter);
    ASSERT_EQ(painter.runs.size(), 2U);
    EXPECT_EQ(painter.runs[0].face, core::FontFace::Bold);
    EXPECT_DOUBLE_EQ(painter.runs[0].tracking, 1.0);
    EXPECT_DOUBLE_EQ(painter.runs[0].widthFactor, 2.0);
    EXPECT_EQ(painter.runs[1].face, core::FontFace::Italic);
    EXPECT_DOUBLE_EQ(painter.runs[1].tracking, 2.0);
    EXPECT_DOUBLE_EQ(painter.runs[1].widthFactor, 2.0);
}

TEST(GeneratedDrawing, TinyFormattedTextUsesItsFormattedBox) {
    const Items items;
    core::Text text;
    text.position = {1.0, 2.0};
    text.height = 0.5;
    text.value = "AB";
    const core::FormattedText piece{text, core::FontFace::Bold, 2.0};
    ShapePainter painter;
    const auto viewport = testViewport();
    drawGeneratedItem(items.item(piece), false, viewport, SceneStyle{}, painter);
    EXPECT_TRUE(painter.runs.empty());
    ASSERT_EQ(painter.strokes.size(), 1U);
    ASSERT_EQ(painter.strokes[0].points.size(), 4U);
    const auto box = core::formattedTextBox(piece);
    for (std::size_t i = 0; i < box.size(); ++i) {
        expectPoint(painter.strokes[0].points[i], viewport.worldToScreen(box[i]));
    }
}

}  // namespace
}  // namespace tg::render
