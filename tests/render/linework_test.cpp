// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "linework.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <utility>
#include <variant>
#include <vector>

namespace tg::render {
namespace {

using core::Vec2;

constexpr double kEps = 1e-9;

void expectPoint(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kEps);
    EXPECT_NEAR(actual.y, expected.y, kEps);
}

/// Records the drawing calls, in order.
class CallPainter final : public Painter {
public:
    struct Call {
        std::vector<Vec2> points;
        bool line{false};  ///< drawLine, otherwise drawPolyline
        bool closed{false};
    };

    std::vector<Call> calls;

    void clear(const Color& /*background*/) override {}

    void setPen(const Color& /*color*/, double /*widthPx*/, LineStyle /*style*/) override {}

    void fillPolygon(std::span<const Vec2> /*points*/, const Color& /*color*/) override {}

    void drawText(const TextRun& /*run*/) override {}

    void drawLine(const Vec2& from, const Vec2& to) override {
        calls.push_back({{from, to}, true, false});
    }

    void drawPolyline(std::span<const Vec2> points, bool closed) override {
        calls.push_back({{points.begin(), points.end()}, false, closed});
    }
};

// Screen x = 400 + 2 x, y = 300 - 2 y.
Viewport testViewport(double scale = 2.0) {
    Viewport viewport;
    viewport.setScreenSize(800.0, 600.0);
    viewport.setScale(scale);
    viewport.setCenter({0.0, 0.0});
    return viewport;
}

// A drawing whose only entity has a linetype with the given pattern.
struct DashedDrawing {
    DashedDrawing(core::Entity entity, std::vector<double> pattern)
        : document(core::Document::newDrawing()) {
        core::Linetype linetype;
        linetype.name = "Test";
        linetype.pattern.elements = std::move(pattern);
        const core::LinetypeId type = document.addLinetype(std::move(linetype));
        id = document.add(std::move(entity));
        core::EntityProperties properties = document.find(id)->properties;
        properties.linetype = type;
        document.setProperties(id, properties);
    }

    [[nodiscard]] const core::Entity& entity() const { return document.find(id)->entity; }

    [[nodiscard]] core::EffectiveProperties properties() const {
        return core::effectiveProperties(document, *document.find(id));
    }

    core::Document document;
    core::EntityId id;
};

TEST(PenWidth, LineweightsOffGiveTheCosmeticPenWhateverTheLineweight) {
    for (const int lineweight : {0, 25, 100, 211}) {
        for (const double ratio : {1.0, 2.0}) {
            EXPECT_EQ(penWidthPx(lineweight, false, ratio), 0.0);
        }
    }
}

TEST(PenWidth, LineweightsOnConvertMillimetresToDeviceAtLeastOnePixel) {
    struct Case {
        int lineweight;  // hundredths of a millimetre
        double ratio;
        double expected;  // pixels
    };

    // mm * 96 / 25.4 * ratio, rounded, at least 1.
    const std::array<Case, 14> cases{{{0, 1.0, 1.0},
                                      {0, 2.0, 1.0},
                                      {5, 1.0, 1.0},       // 0.19
                                      {25, 1.0, 1.0},      // 0.94
                                      {25, 2.0, 2.0},      // 1.89
                                      {35, 1.0, 1.0},      // 1.32
                                      {35, 2.0, 3.0},      // 2.65
                                      {50, 1.0, 2.0},      // 1.89
                                      {60, 1.0, 2.0},      // 2.27
                                      {80, 1.0, 3.0},      // 3.02
                                      {100, 1.0, 4.0},     // 3.78
                                      {100, 2.0, 8.0},     // 7.56
                                      {211, 1.0, 8.0},     // 7.97
                                      {211, 2.0, 16.0}}};  // 15.95
    for (const Case& c : cases) {
        EXPECT_EQ(penWidthPx(c.lineweight, true, c.ratio), c.expected)
            << c.lineweight << " at " << c.ratio;
    }
}

TEST(DashPieces, ContinuousLinetypeAndTextGiveNoPieces) {
    const DashedDrawing continuous(core::Line{{0.0, 0.0}, {30.0, 0.0}}, {});
    EXPECT_FALSE(dashPieces(continuous.entity(), continuous.properties(), 2.0));

    core::Text text;
    text.value = "HHHH";
    const DashedDrawing dashedText(text, {10.0, -5.0});
    EXPECT_FALSE(dashPieces(dashedText.entity(), dashedText.properties(), 2.0));
}

TEST(DashPieces, ArePiecesOfCoreDashesInDrawingCoordinates) {
    const DashedDrawing drawing(core::Line{{0.0, 0.0}, {30.0, 0.0}}, {10.0, -5.0});
    const auto pieces = dashPieces(drawing.entity(), drawing.properties(), 2.0);
    ASSERT_TRUE(pieces.has_value());
    ASSERT_EQ(pieces->size(), 2U);
    const auto* first = std::get_if<core::DashLine>(&pieces->at(0));
    const auto* second = std::get_if<core::DashLine>(&pieces->at(1));
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    expectPoint(first->start, {0.0, 0.0});
    expectPoint(first->end, {10.0, 0.0});
    expectPoint(second->start, {15.0, 0.0});
    expectPoint(second->end, {25.0, 0.0});
}

TEST(DashPieces, ScaledPatternBelowFourPixelsIsContinuous) {
    const DashedDrawing drawing(core::Line{{0.0, 0.0}, {30.0, 0.0}}, {1.0, -1.0});  // length 2
    const auto properties = drawing.properties();
    EXPECT_FALSE(dashPieces(drawing.entity(), properties, 1.99));  // 3.98 px
    EXPECT_TRUE(dashPieces(drawing.entity(), properties, 2.0));    // 4 px: still told apart
    EXPECT_TRUE(dashPieces(drawing.entity(), properties, 20.0));
}

TEST(DashPieces, ThresholdUsesTheEffectiveScale) {
    DashedDrawing drawing(core::Line{{0.0, 0.0}, {30.0, 0.0}}, {1.0, -1.0});
    drawing.document.setLtscale(0.5);  // the pattern is 1 unit long
    const auto properties = drawing.properties();
    EXPECT_DOUBLE_EQ(properties.linetypeScale, 0.5);
    EXPECT_FALSE(dashPieces(drawing.entity(), properties, 3.9));
    EXPECT_TRUE(dashPieces(drawing.entity(), properties, 4.0));
}

TEST(DashPieces, EntityShorterThanThePatternIsContinuous) {
    const DashedDrawing drawing(core::Line{{0.0, 0.0}, {3.0, 0.0}}, {10.0, -5.0});
    EXPECT_FALSE(dashPieces(drawing.entity(), drawing.properties(), 2.0));
}

TEST(DashPieces, MoreThanTheMaximumNumberOfPiecesIsContinuous) {
    // 1e6 units with a pattern 2 long: 500 000 dashes, over kMaxDashPieces.
    const DashedDrawing drawing(core::Line{{0.0, 0.0}, {1e6, 0.0}}, {1.0, -1.0});
    EXPECT_FALSE(dashPieces(drawing.entity(), drawing.properties(), 2.0));
}

TEST(DrawDashPieces, SegmentIsDrawnWithDrawLineInScreenSpace) {
    CallPainter painter;
    const std::array<core::DashPiece, 2> pieces{core::DashLine{{0.0, 0.0}, {10.0, 0.0}},
                                                core::DashLine{{0.0, 0.0}, {0.0, 10.0}}};
    drawDashPieces(pieces, testViewport(), 0.25, painter);
    ASSERT_EQ(painter.calls.size(), 2U);
    EXPECT_TRUE(painter.calls.at(0).line);
    expectPoint(painter.calls.at(0).points.at(0), {400.0, 300.0});
    expectPoint(painter.calls.at(0).points.at(1), {420.0, 300.0});
    // Y points up in the drawing and down on the screen.
    expectPoint(painter.calls.at(1).points.at(0), {400.0, 300.0});
    expectPoint(painter.calls.at(1).points.at(1), {400.0, 280.0});
}

TEST(DrawDashPieces, DotIsOnePixelAlongItsDirectionAtEveryZoom) {
    for (const double scale : {0.5, 2.0, 40.0}) {
        CallPainter painter;
        const std::array<core::DashPiece, 2> pieces{core::DashPoint{{5.0, 0.0}, {1.0, 0.0}},
                                                    core::DashPoint{{5.0, 0.0}, {0.0, 1.0}}};
        drawDashPieces(pieces, testViewport(scale), 0.25, painter);
        ASSERT_EQ(painter.calls.size(), 2U);
        const Vec2 base{400.0 + (5.0 * scale), 300.0};
        EXPECT_TRUE(painter.calls.at(0).line);
        expectPoint(painter.calls.at(0).points.at(0), base);
        expectPoint(painter.calls.at(0).points.at(1), base + Vec2{1.0, 0.0});
        // Up in the drawing is up on the screen, where Y decreases.
        expectPoint(painter.calls.at(1).points.at(1), base + Vec2{0.0, -1.0});
    }
}

TEST(DrawDashPieces, ArcIsTessellatedAndDrawnOpen) {
    CallPainter painter;
    const std::array<core::DashPiece, 1> pieces{
        core::DashArc{{0.0, 0.0}, 10.0, 0.0, core::kPi / 2.0}};
    constexpr double kTolerancePx = 0.25;
    drawDashPieces(pieces, testViewport(), kTolerancePx, painter);
    ASSERT_EQ(painter.calls.size(), 1U);
    const auto& call = painter.calls.front();
    EXPECT_FALSE(call.line);
    EXPECT_FALSE(call.closed);
    ASSERT_GT(call.points.size(), 4U);
    // From (10, 0) counter-clockwise to (0, 10): radius 20 px around (400, 300).
    expectPoint(call.points.front(), {420.0, 300.0});
    expectPoint(call.points.back(), {400.0, 280.0});
    for (const Vec2& point : call.points) {
        EXPECT_NEAR(core::distance(point, {400.0, 300.0}), 20.0, kEps);
    }
}

TEST(DrawDashPieces, ArcWhoseEndIsBelowItsStartCrossesZeroRadians) {
    CallPainter painter;
    constexpr double kHalfWidth = 0.5;  // radians each side of +X
    const std::array<core::DashPiece, 1> pieces{
        core::DashArc{{0.0, 0.0}, 10.0, core::kTwoPi - kHalfWidth, kHalfWidth}};
    drawDashPieces(pieces, testViewport(), 0.25, painter);
    ASSERT_EQ(painter.calls.size(), 1U);
    const auto& points = painter.calls.front().points;
    ASSERT_GT(points.size(), 2U);
    const double edgeX = 400.0 + (20.0 * std::cos(kHalfWidth));
    const double edgeY = 20.0 * std::sin(kHalfWidth);
    // Below +X in the drawing is lower on the screen, where Y grows.
    expectPoint(points.front(), {edgeX, 300.0 + edgeY});
    expectPoint(points.back(), {edgeX, 300.0 - edgeY});
    // The short way round: no point goes left of the ends.
    for (const Vec2& point : points) {
        EXPECT_GE(point.x, edgeX - kEps);
    }
}

}  // namespace
}  // namespace tg::render
