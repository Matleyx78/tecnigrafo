// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/render/scene_renderer.hpp"

#include "tg/core/color.hpp"
#include "tg/core/math.hpp"
#include "tg/core/text.hpp"

#include <gtest/gtest.h>

#include <array>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

namespace tg::render {
namespace {

/// The pen as set by the last setPen().
struct Pen {
    Color color;
    double widthPx{0.0};
    LineStyle style{LineStyle::Solid};
};

/// A line or a polyline, with the pen it was drawn with.
struct Stroke {
    Pen pen;
    std::vector<core::Vec2> points;
    bool line{false};  ///< drawLine, otherwise drawPolyline
    bool closed{false};
};

class RecordingPainter final : public Painter {
public:
    std::vector<TextRun> runs;
    std::vector<Pen> textPens;  ///< the pen at each drawText, in step with runs
    std::vector<std::vector<core::Vec2>> outlines;
    std::vector<bool> closed;
    std::vector<Stroke> strokes;  ///< every drawLine and drawPolyline, in order
    Pen pen;                      ///< as set by the last setPen()

    void clear(const Color& /*background*/) override {}

    void setPen(const Color& color, double widthPx, LineStyle style) override {
        pen = {color, widthPx, style};
    }

    void fillPolygon(std::span<const core::Vec2> /*points*/, const Color& /*color*/) override {}

    void drawText(const TextRun& run) override {
        runs.push_back(run);
        textPens.push_back(pen);
    }

    void drawLine(const core::Vec2& from, const core::Vec2& to) override {
        strokes.push_back({pen, {from, to}, true, false});
    }

    void drawPolyline(std::span<const core::Vec2> points, bool isClosed) override {
        outlines.emplace_back(points.begin(), points.end());
        closed.push_back(isClosed);
        strokes.push_back({pen, {points.begin(), points.end()}, false, isClosed});
    }
};

Viewport textViewport() {
    Viewport viewport;
    viewport.setScreenSize(1000.0, 800.0);
    viewport.setCenter({10.0, 20.0});
    viewport.setScale(3.0);
    return viewport;
}

void expectPoint(const core::Vec2& actual, const core::Vec2& expected) {
    constexpr double kTolerance = 1e-10;
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}

TEST(SceneRendererText, UsesTextFrameForEveryAlignmentAndTransformation) {
    const auto viewport = textViewport();
    core::Text base;
    base.position = {5.0, 7.0};
    base.alignmentPoint = {40.0, 30.0};
    base.height = 4.0;
    base.rotation = 0.4;
    base.widthFactor = 1.3;
    base.value = "H%%d";
    std::array<core::Text, 7> texts;
    texts.fill(base);
    texts.at(1).horizontal = core::TextHorizontal::Center;
    texts.at(1).vertical = core::TextVertical::Middle;
    texts.at(2).horizontal = core::TextHorizontal::Aligned;
    texts.at(3).horizontal = core::TextHorizontal::Fit;
    texts.at(4).obliqueAngle = std::numbers::pi / 6.0;
    texts.at(5).mirroredX = true;
    texts.at(6).mirroredY = true;
    for (const auto& text : texts) {
        RecordingPainter painter;
        const std::array<core::Entity, 1> entities{text};
        renderPreview(entities, viewport, {}, painter);
        ASSERT_EQ(painter.runs.size(), 1U);
        EXPECT_TRUE(painter.outlines.empty());
        const auto frame = core::textFrame(text);
        const auto& run = painter.runs.front();
        expectPoint(run.origin, viewport.worldToScreen(frame.origin));
        EXPECT_DOUBLE_EQ(run.capHeightPx, frame.height * viewport.scale());
        EXPECT_DOUBLE_EQ(run.angle, frame.rotation);
        EXPECT_DOUBLE_EQ(run.widthFactor, frame.widthFactor);
        EXPECT_DOUBLE_EQ(run.oblique, text.obliqueAngle);
        EXPECT_EQ(run.backward, text.mirroredX);
        EXPECT_EQ(run.upsideDown, text.mirroredY);
        EXPECT_EQ(run.text, U"H\u00b0");
    }
}

TEST(SceneRendererText, TinyTextUsesTransformedBoxAndDerivedHeight) {
    const auto viewport = textViewport();
    core::Text text;
    text.value = "HHHH";
    text.height = 10.0;
    text.position = {10.0, 20.0};
    // Aligned derives a tiny height despite the large stored height.
    text.alignmentPoint = {10.1, 20.1};
    text.horizontal = core::TextHorizontal::Aligned;
    text.obliqueAngle = 0.5;
    text.mirroredX = true;
    text.mirroredY = true;
    RecordingPainter painter;
    const std::array<core::Entity, 1> entities{text};
    renderPreview(entities, viewport, {}, painter);
    EXPECT_TRUE(painter.runs.empty());
    ASSERT_EQ(painter.outlines.size(), 1U);
    ASSERT_TRUE(painter.closed.front());
    const auto box = core::textBox(text);
    ASSERT_EQ(painter.outlines.front().size(), box.size());
    for (std::size_t i = 0; i < box.size(); ++i) {
        expectPoint(painter.outlines.front().at(i), viewport.worldToScreen(box.at(i)));
    }
}

// --- Properties of the entities (docs/adr/0023, 0024, 0025) ---------------

// renderScene() draws the two axes before any entity: this colour tells them
// from the entities' strokes.
constexpr Color kAxisMarker{1, 2, 3, 4};
// The main colour of the theme, as set by propertiesStyle().
constexpr Color kThemeColor{200, 201, 202};

SceneStyle propertiesStyle() {
    SceneStyle style;
    style.showGrid = false;
    style.axisX = kAxisMarker;
    style.axisY = kAxisMarker;
    style.entity = kThemeColor;
    return style;
}

// Screen x = 400 + scale * x, y = 300 - scale * y.
Viewport strokeViewport(double scale = 2.0) {
    Viewport viewport;
    viewport.setScreenSize(800.0, 600.0);
    viewport.setScale(scale);
    viewport.setCenter({0.0, 0.0});
    return viewport;
}

std::vector<Stroke> entityStrokes(const RecordingPainter& painter) {
    std::vector<Stroke> result;
    for (const Stroke& stroke : painter.strokes) {
        if (stroke.pen.color != kAxisMarker) {
            result.push_back(stroke);
        }
    }
    return result;
}

std::vector<Stroke> drawScene(const core::Document& document, const SceneStyle& style,
                              double scale = 2.0) {
    RecordingPainter painter;
    renderScene(document, strokeViewport(scale), style, painter);
    return entityStrokes(painter);
}

Color toRenderColor(const core::Rgb& rgb) {
    return Color{rgb.r, rgb.g, rgb.b};
}

core::Layer makeLayer(const core::Document& document, std::string name) {
    core::Layer layer;
    layer.name = std::move(name);
    layer.linetype = document.continuousLinetype();
    return layer;
}

core::LinetypeId addLinetype(core::Document& document, std::string name,
                             std::vector<double> pattern) {
    core::Linetype linetype;
    linetype.name = std::move(name);
    linetype.pattern.elements = std::move(pattern);
    return document.addLinetype(std::move(linetype));
}

core::EntityId addWith(core::Document& document, core::Entity entity,
                       const core::EntityProperties& properties) {
    const core::EntityId id = document.add(std::move(entity));
    document.setProperties(id, properties);
    return id;
}

// A line on y = 0 from x = 0 to x = length.
core::Line horizontalLine(double length) {
    return core::Line{{0.0, 0.0}, {length, 0.0}};
}

void expectSegment(const Stroke& stroke, const core::Vec2& from, const core::Vec2& to) {
    constexpr double kTolerance = 1e-9;
    ASSERT_TRUE(stroke.line);
    ASSERT_EQ(stroke.points.size(), 2U);
    EXPECT_NEAR(stroke.points.at(0).x, from.x, kTolerance);
    EXPECT_NEAR(stroke.points.at(0).y, from.y, kTolerance);
    EXPECT_NEAR(stroke.points.at(1).x, to.x, kTolerance);
    EXPECT_NEAR(stroke.points.at(1).y, to.y, kTolerance);
}

TEST(SceneRendererProperties, SkipsEntitiesOnLayersOffOrFrozenButNotOnLockedOnes) {
    core::Document document = core::Document::newDrawing();
    core::Layer off = makeLayer(document, "Off");
    off.on = false;
    core::Layer frozen = makeLayer(document, "Frozen");
    frozen.frozen = true;
    core::Layer locked = makeLayer(document, "Locked");
    locked.locked = true;
    const core::Layer plain = makeLayer(document, "Plain");
    double y = 0.0;
    for (const core::Layer& layer : {off, frozen, locked, plain}) {
        core::EntityProperties properties = document.current();
        properties.layer = document.addLayer(layer);
        addWith(document, core::Line{{0.0, y}, {10.0, y}}, properties);
        y += 1.0;
    }

    const auto strokes = drawScene(document, propertiesStyle());

    // The lines at y = 2 (locked) and y = 3 (plain), in the order of the drawing.
    ASSERT_EQ(strokes.size(), 2U);
    EXPECT_NEAR(strokes.at(0).points.front().y, 296.0, 1e-9);
    EXPECT_NEAR(strokes.at(1).points.front().y, 294.0, 1e-9);
}

TEST(SceneRendererProperties, ColourIsTheEffectiveRgbAndOnlyIndexSevenFollowsTheTheme) {
    core::Document document = core::Document::newDrawing();
    core::Layer green = makeLayer(document, "Green");
    green.color = core::Color::fromIndex(3);
    core::Layer blue = makeLayer(document, "Blue");
    blue.color = core::Color::fromRgb({0, 0, 255});
    const core::Layer seven = makeLayer(document, "Seven");  // index 7, like a new layer
    const core::LayerId layerZero = document.current().layer;
    const core::LayerId greenId = document.addLayer(green);
    const core::LayerId blueId = document.addLayer(blue);
    const core::LayerId sevenId = document.addLayer(seven);

    struct Case {
        core::LayerId layer;
        core::Color color;
        bool followsTheme{false};
        Color expected;  // when it does not
    };

    const std::array<Case, 8> cases{{
        {layerZero, core::Color::fromIndex(1), false, toRenderColor(core::aciToRgb(1))},
        {layerZero, core::Color::fromRgb({10, 20, 30}), false, {10, 20, 30}},
        {layerZero, core::Color::fromIndex(7), true, {}},
        {layerZero, core::Color::fromRgb({255, 255, 255}), false, {255, 255, 255}},
        {greenId, core::Color::byLayer(), false, toRenderColor(core::aciToRgb(3))},
        {blueId, core::Color::byLayer(), false, {0, 0, 255}},
        {sevenId, core::Color::byLayer(), true, {}},
        {layerZero, core::Color::byBlock(), true, {}},  // as index 7, until there are blocks
    }};
    double y = 0.0;
    for (const Case& c : cases) {
        core::EntityProperties properties = document.current();
        properties.layer = c.layer;
        properties.color = c.color;
        addWith(document, core::Line{{0.0, y}, {10.0, y}}, properties);
        y += 1.0;
    }

    // The theme is the main colour: black on white, white on black.
    for (const Color theme : {Color{0, 0, 0}, Color{255, 255, 255}}) {
        SceneStyle style = propertiesStyle();
        style.entity = theme;
        const auto strokes = drawScene(document, style);
        ASSERT_EQ(strokes.size(), cases.size());
        for (std::size_t i = 0; i < cases.size(); ++i) {
            const Color expected = cases.at(i).followsTheme ? theme : cases.at(i).expected;
            EXPECT_EQ(strokes.at(i).pen.color, expected) << "case " << i;
        }
    }
}

TEST(SceneRendererProperties, LineweightIsInDevicePixelsWithLwdisplayOnAndCosmeticWhenOff) {
    core::Document document = core::Document::newDrawing();
    core::Layer heavy = makeLayer(document, "Heavy");
    heavy.lineweight = 50;  // 0.5 mm
    const core::LayerId layerZero = document.current().layer;
    const core::LayerId heavyId = document.addLayer(heavy);

    struct Case {
        core::LayerId layer;
        int lineweight{0};
        double widthAtOne{0.0};  // device pixels per logical pixel
        double widthAtTwo{0.0};
    };

    // 0.25 mm, 1 mm, 0 mm, ByLayer on 0.5 mm, then Default and ByBlock, which
    // show LWDEFAULT (0.25 mm for a new drawing).
    const std::array<Case, 6> cases{{
        {layerZero, 25, 1.0, 2.0},
        {layerZero, 100, 4.0, 8.0},
        {layerZero, 0, 1.0, 1.0},
        {heavyId, core::lineweight::kByLayer, 2.0, 4.0},
        {layerZero, core::lineweight::kDefault, 1.0, 2.0},
        {layerZero, core::lineweight::kByBlock, 1.0, 2.0},
    }};
    double y = 0.0;
    for (const Case& c : cases) {
        core::EntityProperties properties = document.current();
        properties.layer = c.layer;
        properties.lineweight = c.lineweight;
        addWith(document, core::Line{{0.0, y}, {10.0, y}}, properties);
        y += 1.0;
    }

    SceneStyle style = propertiesStyle();
    document.setLwdisplay(true);
    for (const double ratio : {1.0, 2.0}) {
        style.devicePixelRatio = ratio;
        const auto strokes = drawScene(document, style);
        ASSERT_EQ(strokes.size(), cases.size());
        for (std::size_t i = 0; i < cases.size(); ++i) {
            const double expected = ratio == 1.0 ? cases.at(i).widthAtOne : cases.at(i).widthAtTwo;
            EXPECT_EQ(strokes.at(i).pen.widthPx, expected) << "case " << i << " at " << ratio;
            EXPECT_EQ(strokes.at(i).pen.style, LineStyle::Solid);
        }
    }

    document.setLwdisplay(false);
    for (const double ratio : {1.0, 2.0}) {
        style.devicePixelRatio = ratio;
        for (const Stroke& stroke : drawScene(document, style)) {
            EXPECT_EQ(stroke.pen.widthPx, 0.0);
        }
    }
}

TEST(SceneRendererProperties, DefaultLineweightFollowsLwdefault) {
    core::Document document = core::Document::newDrawing();
    core::EntityProperties properties = document.current();
    properties.lineweight = core::lineweight::kDefault;
    addWith(document, horizontalLine(10.0), properties);
    document.setLwdisplay(true);
    document.setLwdefault(100);  // 1 mm
    const auto strokes = drawScene(document, propertiesStyle());
    ASSERT_EQ(strokes.size(), 1U);
    EXPECT_EQ(strokes.front().pen.widthPx, 4.0);
}

TEST(SceneRendererProperties, TextTakesTheColourOnlyWithNoLineweightAndNoLinetype) {
    core::Document document = core::Document::newDrawing();
    document.setLwdisplay(true);
    core::EntityProperties properties = document.current();
    properties.color = core::Color::fromIndex(1);
    properties.lineweight = 100;
    properties.linetype = addLinetype(document, "Dashed", {10.0, -5.0});
    core::Text text;
    text.value = "HHHH";
    text.height = 4.0;
    addWith(document, text, properties);
    // Too small to be read: it is drawn as its box, which is a plain outline.
    core::Text tiny = text;
    tiny.position = {20.0, 0.0};
    tiny.height = 0.2;
    addWith(document, tiny, properties);

    RecordingPainter painter;
    renderScene(document, strokeViewport(), propertiesStyle(), painter);

    const Color red = toRenderColor(core::aciToRgb(1));
    ASSERT_EQ(painter.runs.size(), 1U);
    EXPECT_EQ(painter.textPens.front().color, red);
    EXPECT_EQ(painter.textPens.front().widthPx, 0.0);
    EXPECT_EQ(painter.textPens.front().style, LineStyle::Solid);
    const auto strokes = entityStrokes(painter);
    ASSERT_EQ(strokes.size(), 1U);
    EXPECT_FALSE(strokes.front().line);
    EXPECT_TRUE(strokes.front().closed);
    EXPECT_EQ(strokes.front().points.size(), 4U);
    EXPECT_EQ(strokes.front().pen.color, red);
    EXPECT_EQ(strokes.front().pen.widthPx, 0.0);
    EXPECT_EQ(strokes.front().pen.style, LineStyle::Solid);
}

TEST(SceneRendererProperties, DashedLineIsItsPiecesInPixelsDrawnWithASolidPen) {
    core::Document document = core::Document::newDrawing();
    core::EntityProperties properties = document.current();
    properties.color = core::Color::fromIndex(1);
    properties.linetype = addLinetype(document, "Dashed", {10.0, -5.0});
    addWith(document, horizontalLine(30.0), properties);

    const auto strokes = drawScene(document, propertiesStyle());

    // Dashes from 0 to 10 and from 15 to 25; the gap from 25 to 30 leaves nothing.
    ASSERT_EQ(strokes.size(), 2U);
    expectSegment(strokes.at(0), {400.0, 300.0}, {420.0, 300.0});
    expectSegment(strokes.at(1), {430.0, 300.0}, {450.0, 300.0});
    for (const Stroke& stroke : strokes) {
        EXPECT_EQ(stroke.pen.color, toRenderColor(core::aciToRgb(1)));
        EXPECT_EQ(stroke.pen.widthPx, 0.0);
        EXPECT_EQ(stroke.pen.style, LineStyle::Solid);
    }
}

TEST(SceneRendererProperties, EveryPieceOfADashedLineHasTheLineweight) {
    core::Document document = core::Document::newDrawing();
    document.setLwdisplay(true);
    core::EntityProperties properties = document.current();
    properties.lineweight = 50;
    properties.linetype = addLinetype(document, "Dashed", {10.0, -5.0});
    addWith(document, horizontalLine(30.0), properties);
    const auto strokes = drawScene(document, propertiesStyle());
    ASSERT_EQ(strokes.size(), 2U);
    for (const Stroke& stroke : strokes) {
        EXPECT_EQ(stroke.pen.widthPx, 2.0);
    }
}

TEST(SceneRendererProperties, LinetypeComesFromTheLayerOrIsContinuousWhenByBlock) {
    core::Document document = core::Document::newDrawing();
    core::Layer dashed = makeLayer(document, "Dashed");
    dashed.linetype = addLinetype(document, "Dashed", {10.0, -5.0});
    core::EntityProperties properties = document.current();
    properties.layer = document.addLayer(dashed);
    properties.linetype = document.byLayerLinetype();
    addWith(document, horizontalLine(30.0), properties);
    properties.linetype = document.byBlockLinetype();  // continuous until there are blocks
    addWith(document, core::Line{{0.0, 1.0}, {30.0, 1.0}}, properties);

    const auto strokes = drawScene(document, propertiesStyle());

    ASSERT_EQ(strokes.size(), 3U);
    expectSegment(strokes.at(0), {400.0, 300.0}, {420.0, 300.0});
    expectSegment(strokes.at(1), {430.0, 300.0}, {450.0, 300.0});
    EXPECT_FALSE(strokes.at(2).line);
    ASSERT_EQ(strokes.at(2).points.size(), 2U);
    EXPECT_NEAR(strokes.at(2).points.at(1).x - strokes.at(2).points.at(0).x, 60.0, 1e-9);
}

TEST(SceneRendererProperties, PatternIsScaledByLtscaleAndByTheEntityScale) {
    core::Document document = core::Document::newDrawing();
    document.setLtscale(0.5);
    core::EntityProperties properties = document.current();
    properties.linetype = addLinetype(document, "Dashed", {10.0, -5.0});
    properties.linetypeScale = 4.0;  // 0.5 * 4: the pattern is 20 and -10
    addWith(document, horizontalLine(30.0), properties);

    const auto strokes = drawScene(document, propertiesStyle());

    ASSERT_EQ(strokes.size(), 1U);
    expectSegment(strokes.front(), {400.0, 300.0}, {440.0, 300.0});
}

TEST(SceneRendererProperties, DotIsAStrokeOfOnePixelAlongTheLine) {
    core::Document document = core::Document::newDrawing();
    core::EntityProperties properties = document.current();
    properties.linetype = addLinetype(document, "Dashdot", {4.0, -4.0, 0.0, -4.0});
    addWith(document, horizontalLine(14.0), properties);

    const auto strokes = drawScene(document, propertiesStyle());

    // A dash from 0 to 4, a dot at 8, and the next dash from 12, cut at 14.
    ASSERT_EQ(strokes.size(), 3U);
    expectSegment(strokes.at(0), {400.0, 300.0}, {408.0, 300.0});
    expectSegment(strokes.at(1), {416.0, 300.0}, {417.0, 300.0});
    expectSegment(strokes.at(2), {424.0, 300.0}, {428.0, 300.0});
}

TEST(SceneRendererProperties, ArcsAndCirclesAreDashedAsOpenTessellatedArcs) {
    core::Document document = core::Document::newDrawing();
    core::EntityProperties properties = document.current();
    properties.linetype = addLinetype(document, "Dashed", {5.0, -5.0});
    addWith(document, core::Circle{{0.0, 0.0}, 10.0}, properties);

    const auto strokes = drawScene(document, propertiesStyle());

    // 62.8 units of circumference: six dashes of 5 and a last one of 2.8.
    ASSERT_EQ(strokes.size(), 7U);
    for (const Stroke& stroke : strokes) {
        EXPECT_FALSE(stroke.line);
        EXPECT_FALSE(stroke.closed);
        EXPECT_GE(stroke.points.size(), 2U);
    }
    EXPECT_NEAR(strokes.front().points.front().x, 420.0, 1e-9);
    EXPECT_NEAR(strokes.front().points.front().y, 300.0, 1e-9);
    EXPECT_GT(strokes.front().points.size(), 2U);
}

TEST(SceneRendererProperties, PatternShorterThanFourPixelsIsDrawnContinuous) {
    core::Document document = core::Document::newDrawing();
    core::EntityProperties properties = document.current();
    properties.linetype = addLinetype(document, "Fine", {1.0, -1.0});  // 2 units long
    addWith(document, horizontalLine(10.0), properties);

    // 3 pixels: one segment, as a continuous line.
    const auto fine = drawScene(document, propertiesStyle(), 1.5);
    ASSERT_EQ(fine.size(), 1U);
    EXPECT_FALSE(fine.front().line);
    EXPECT_EQ(fine.front().points.size(), 2U);

    // 4 pixels: told apart, five dashes of 1 unit.
    const auto told = drawScene(document, propertiesStyle(), 2.0);
    ASSERT_EQ(told.size(), 5U);
    for (const Stroke& stroke : told) {
        EXPECT_TRUE(stroke.line);
    }
}

TEST(SceneRendererProperties, MoreThanTheMaximumNumberOfPiecesIsDrawnContinuous) {
    core::Document document = core::Document::newDrawing();
    core::EntityProperties properties = document.current();
    properties.linetype = addLinetype(document, "Fine", {1.0, -1.0});
    addWith(document, horizontalLine(1e6), properties);  // 500 000 dashes

    const auto strokes = drawScene(document, propertiesStyle());

    ASSERT_EQ(strokes.size(), 1U);
    EXPECT_FALSE(strokes.front().line);
    EXPECT_EQ(strokes.front().points.size(), 2U);
}

TEST(SceneRendererProperties, HighlightAndPreviewIgnoreTheEntityProperties) {
    core::Document document = core::Document::newDrawing();
    document.setLwdisplay(true);
    core::EntityProperties properties = document.current();
    properties.color = core::Color::fromIndex(1);
    properties.lineweight = 100;
    properties.linetype = addLinetype(document, "Dashed", {10.0, -5.0});
    const core::EntityId id = addWith(document, horizontalLine(30.0), properties);
    const SceneStyle style = propertiesStyle();

    RecordingPainter highlight;
    const std::array ids{id};
    renderHighlight(document, ids, strokeViewport(), style, highlight);
    ASSERT_EQ(highlight.strokes.size(), 1U);
    EXPECT_FALSE(highlight.strokes.front().line);
    EXPECT_EQ(highlight.strokes.front().pen.color, style.highlight);
    EXPECT_EQ(highlight.strokes.front().pen.widthPx, 0.0);
    EXPECT_EQ(highlight.strokes.front().pen.style, LineStyle::Dashed);

    RecordingPainter preview;
    const std::array<core::Entity, 1> entities{horizontalLine(30.0)};
    renderPreview(entities, strokeViewport(), style, preview);
    ASSERT_EQ(preview.strokes.size(), 1U);
    EXPECT_FALSE(preview.strokes.front().line);
    EXPECT_EQ(preview.strokes.front().pen.color, style.preview);
    EXPECT_EQ(preview.strokes.front().pen.widthPx, 0.0);
    EXPECT_EQ(preview.strokes.front().pen.style, LineStyle::Solid);
}

}  // namespace
}  // namespace tg::render
