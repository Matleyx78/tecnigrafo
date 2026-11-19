// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Angular, radius and diameter dimensions: sector, arc, center marks and
// the fit along the arc (docs/tappe/0.8/PT24-quote-angolari-radiali.md).

#include "tg/core/arc.hpp"
#include "tg/core/dim_style_table.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/entity_record.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/solid.hpp"
#include "tg/core/text_metrics.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-9;

void expectNear(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}

/// Two angles that are the same direction, whatever their turns.
void expectAngle(double actual, double expected) {
    EXPECT_NEAR(std::remainder(actual - expected, 2.0 * kPi), 0.0, kTolerance)
        << actual << " vs " << expected;
}

/// The point at `radius` from the origin in the direction of `degrees`.
Vec2 polar(double radius, double degrees) {
    const SinCos direction = sinCos(degreesToRadians(degrees));
    return {radius * direction.cosine, radius * direction.sine};
}

/// The angular dimension between the lines p13 → p14 and linePoint → p15,
/// with point16 in `p16` (dxfType 2).
Dimension angular(const Vec2& p13, const Vec2& p14, const Vec2& linePoint, const Vec2& p15,
                  const Vec2& p16) {
    Dimension dimension;
    dimension.definition.kind = DimensionKind::Angular;
    dimension.definition.dxfType = 2;
    dimension.definition.point13 = p13;
    dimension.definition.point14 = p14;
    dimension.definition.linePoint = linePoint;
    dimension.definition.point15 = p15;
    dimension.definition.point16 = p16;
    return dimension;
}

/// The angular dimension between the X axis and a line through the origin
/// at `degrees`, both 120 long and centered in the origin.
Dimension fromXAxis(double degrees, const Vec2& p16) {
    return angular({-60, 0}, {60, 0}, polar(-60, degrees), polar(60, degrees), p16);
}

/// A radius dimension with the center `center`, the point on the curve
/// `onCurve` and the text in `text` (dxfType 4).
Dimension radius(const Vec2& center, const Vec2& onCurve, const Vec2& text) {
    Dimension dimension;
    dimension.definition.kind = DimensionKind::Radius;
    dimension.definition.dxfType = 4;
    dimension.definition.linePoint = center;
    dimension.definition.point15 = onCurve;
    dimension.presentation.textPoint = text;
    return dimension;
}

/// A diameter dimension from point15 to linePoint, with the text in `text`
/// (dxfType 3).
Dimension diameter(const Vec2& p15, const Vec2& linePoint, const Vec2& text) {
    Dimension dimension = radius(linePoint, p15, text);
    dimension.definition.kind = DimensionKind::Diameter;
    dimension.definition.dxfType = 3;
    return dimension;
}

Dimension made(const Document& document, Dimension dimension) {
    std::string reason;
    const std::optional<Dimension> result = regenerated(document, std::move(dimension), &reason);
    EXPECT_TRUE(result.has_value()) << reason;
    return result.value_or(Dimension{});
}

/// The reason why `dimension` is not regenerated, empty when it is.
std::string refusalOf(const Document& document, Dimension dimension) {
    std::string reason;
    if (regenerated(document, std::move(dimension), &reason)) {
        return {};
    }
    return reason;
}

/// A new drawing whose ISO-25 is changed by `change`.
Document drawingWith(const std::function<void(DimStyle&)>& change) {
    Document document = Document::newDrawing();
    DimStyleTable table = document.dimStyles();
    DimStyle style = *table.find("ISO-25");
    change(style);
    table.put(std::move(style), document);
    document.replaceDimStyles(std::move(table));
    return document;
}

template <typename T>
std::vector<T> entitiesOf(const Dimension& dimension) {
    std::vector<T> entities;
    for (const EntityRecord& record : dimension.graphics) {
        if (const auto* entity = std::get_if<T>(&record.entity)) {
            entities.push_back(*entity);
        }
    }
    return entities;
}

MText textOf(const Dimension& dimension) {
    const std::vector<MText> texts = entitiesOf<MText>(dimension);
    if (texts.size() != 1) {
        ADD_FAILURE() << texts.size() << " texts";
        return {};
    }
    return texts.front();
}

void expectLine(const Line& line, const Vec2& start, const Vec2& end) {
    expectNear(line.start, start);
    expectNear(line.end, end);
}

/// An arrow with the tip in `tip` and the base in `base`.
void expectArrow(const Solid& arrow, const Vec2& tip, const Vec2& base) {
    expectNear(arrow.vertices[2], tip);
    expectNear((arrow.vertices[0] + arrow.vertices[1]) * 0.5, base);
}

void expectArc(const Arc& arc, const Vec2& center, double radius, double startDegrees,
               double endDegrees) {
    expectNear(arc.center, center);
    EXPECT_NEAR(arc.radius, radius, kTolerance);
    expectAngle(arc.startAngle, degreesToRadians(startDegrees));
    expectAngle(arc.endAngle, degreesToRadians(endDegrees));
}

// --- Angular: the sector of point16 --------------------------------------

struct Quadrant {
    double middle{};      ///< the direction of point16, in degrees
    double start{};       ///< the arc, counter-clockwise, in degrees
    double end{};         ///< after `start`, so that end - start is the angle
    Vec2 first;           ///< the ray of the first line, unit
    Vec2 second;          ///< the ray of the second line, unit
    const char* value{};  ///< the text
};

void expectSector(double lineDegrees, const Quadrant& quadrant) {
    const Document document = Document::newDrawing();
    const Dimension dimension = made(document, fromXAxis(lineDegrees, polar(40, quadrant.middle)));
    SCOPED_TRACE(quadrant.middle);
    EXPECT_NEAR(dimension.value.measurement, degreesToRadians(quadrant.end - quadrant.start),
                kTolerance);
    const std::vector<Arc> arcs = entitiesOf<Arc>(dimension);
    ASSERT_EQ(arcs.size(), 1U);
    expectArc(arcs[0], {0, 0}, 40.0, quadrant.start, quadrant.end);
    // The lines reach past the arc: no extension lines, and the arrows fit.
    EXPECT_TRUE(entitiesOf<Line>(dimension).empty());
    const std::vector<Solid> arrows = entitiesOf<Solid>(dimension);
    ASSERT_EQ(arrows.size(), 2U);
    expectNear(arrows[0].vertices[2], quadrant.first * 40.0);
    expectNear(arrows[1].vertices[2], quadrant.second * 40.0);
    EXPECT_EQ(textOf(dimension).value, quadrant.value);
}

TEST(DimensionAngular, RightAngleInEachQuadrant) {
    const std::array<Quadrant, 4> quadrants{{
        {45, 0, 90, {1, 0}, {0, 1}, "90%%d"},
        {135, 90, 180, {-1, 0}, {0, 1}, "90%%d"},
        {225, 180, 270, {-1, 0}, {0, -1}, "90%%d"},
        {315, 270, 360, {1, 0}, {0, -1}, "90%%d"},
    }};
    for (const Quadrant& quadrant : quadrants) {
        expectSector(90.0, quadrant);
    }
}

TEST(DimensionAngular, ThirtyDegreesInEachAngle) {
    const Vec2 ray = polar(1, 30);
    const std::array<Quadrant, 4> quadrants{{
        {15, 0, 30, {1, 0}, ray, "30%%d"},
        {105, 30, 180, {-1, 0}, ray, "150%%d"},
        {195, 180, 210, {-1, 0}, -ray, "30%%d"},
        {285, 210, 360, {1, 0}, -ray, "150%%d"},
    }};
    for (const Quadrant& quadrant : quadrants) {
        expectSector(30.0, quadrant);
    }
}

TEST(DimensionAngular, PointOnARayTakesTheNextAngle) {
    const Document document = Document::newDrawing();
    // On the positive X axis: the angle after it counter-clockwise, the
    // first quadrant.
    Dimension dimension = made(document, fromXAxis(90.0, {5, 0}));
    std::vector<Arc> arcs = entitiesOf<Arc>(dimension);
    ASSERT_EQ(arcs.size(), 1U);
    expectArc(arcs[0], {0, 0}, 5.0, 0, 90);
    // On the positive Y axis: the second quadrant.
    dimension = made(document, fromXAxis(90.0, {0, 5}));
    arcs = entitiesOf<Arc>(dimension);
    ASSERT_EQ(arcs.size(), 1U);
    expectArc(arcs[0], {0, 0}, 5.0, 90, 180);
    // In the vertex: no angle.
    EXPECT_EQ(refusalOf(document, fromXAxis(90.0, {0, 0})), kDimensionNotRegenerable);
}

TEST(DimensionAngular, ParallelAndCoincidentLinesAreRefused) {
    const Document document = Document::newDrawing();
    EXPECT_EQ(refusalOf(document, angular({0, 0}, {10, 0}, {0, 5}, {10, 5}, {5, 2})),
              "Lines are parallel.");
    EXPECT_EQ(refusalOf(document, angular({0, 0}, {10, 0}, {30, 0}, {20, 0}, {5, 2})),
              "Lines are parallel.");
}

TEST(DimensionAngular, ExtensionLinesWhenTheLinesStopBeforeTheArc) {
    const Document document = Document::newDrawing();
    // Vertex in the origin, radius 10; the lines end at 5 from it.
    const Dimension dimension = made(document, angular({2, 0}, {5, 0}, {0, 2}, {0, 5}, {6, 8}));
    const std::vector<Line> lines = entitiesOf<Line>(dimension);
    ASSERT_EQ(lines.size(), 2U);
    // From the nearer end plus DIMEXO 0.625 to the arc plus DIMEXE 1.25.
    expectLine(lines[0], {5.625, 0}, {11.25, 0});
    expectLine(lines[1], {0, 5.625}, {0, 11.25});
    const std::vector<Arc> arcs = entitiesOf<Arc>(dimension);
    ASSERT_EQ(arcs.size(), 1U);
    expectArc(arcs[0], {0, 0}, 10.0, 0, 90);
}

TEST(DimensionAngular, TextInsideArrowsOutsideAlongTheArc) {
    // An arc 8 long, arrows of 2 and a text 7 wide with its margins (5.75
    // without): case 2 of the fit of PT23, measured along the arc.
    const Document document = drawingWith([](DimStyle& style) {
        style.vars.arrowSize = 2.0;
        style.vars.textHeight = 5.75 / textWidth(U"MMMM");
    });
    const double r = 16.0 / kPi;  // a quarter of circle 8 long
    Dimension dimension = fromXAxis(90.0, polar(r, 45));
    dimension.presentation.textOverride = "MMMM";
    dimension = made(document, std::move(dimension));
    const std::vector<Solid> arrows = entitiesOf<Solid>(dimension);
    ASSERT_EQ(arrows.size(), 2U);
    expectArrow(arrows[0], {r, 0}, {r, -2});
    expectArrow(arrows[1], {0, r}, {-2, r});
    const std::vector<Line> lines = entitiesOf<Line>(dimension);
    ASSERT_EQ(lines.size(), 2U);
    expectLine(lines[0], {r, -2}, {r, -4});
    expectLine(lines[1], {-2, r}, {-4, r});
    const std::vector<Arc> arcs = entitiesOf<Arc>(dimension);
    ASSERT_EQ(arcs.size(), 1U);
    expectArc(arcs[0], {0, 0}, r, 0, 90);
    // The text above the middle of the arc, along its tangent.
    const MText text = textOf(dimension);
    EXPECT_EQ(text.attachment, MTextAttachment::BottomCenter);
    expectNear(text.insertion, polar(r + 0.625, 45));
    expectAngle(text.rotation, degreesToRadians(315));
}

TEST(DimensionAngular, DimlfacAndDimrndDoNotApply) {
    const Document document = Document::newDrawing();
    Dimension dimension = fromXAxis(90.0, polar(20, 45));
    dimension.presentation.overrides = {{"DIMLFAC", "2"}, {"DIMRND", "4"}};
    dimension = made(document, std::move(dimension));
    EXPECT_EQ(textOf(dimension).value, "90%%d");
}

TEST(DimensionAngular, GripsOnEveryDefinitionPointAndTheText) {
    const Document document = Document::newDrawing();
    const Dimension dimension = made(document, fromXAxis(90.0, polar(20, 45)));
    const std::vector<Vec2> grips = gripPointsOf(dimension);
    ASSERT_EQ(grips.size(), 6U);
    expectNear(grips[3], {0, 60});
    expectNear(grips[4], polar(20, 45));
    // point16 into the second quadrant regenerates there.
    const GripResult moved = moveGripOf(document, dimension, 4, {-10, 10});
    ASSERT_EQ(moved.outcome, GripOutcome::Moved);
    const auto& result = std::get<Dimension>(moved.entity);
    const std::vector<Arc> arcs = entitiesOf<Arc>(result);
    ASSERT_EQ(arcs.size(), 1U);
    expectArc(arcs[0], {0, 0}, std::sqrt(200.0), 90, 180);
    // point16 into the vertex is refused.
    const GripResult refused = moveGripOf(document, dimension, 4, {0, 0});
    EXPECT_EQ(refused.outcome, GripOutcome::Invalid);
    EXPECT_EQ(refused.refusal, kDimensionNotRegenerable);
}

// --- Radius and diameter ------------------------------------------------

TEST(DimensionRadius, TextOutside) {
    const Document document = Document::newDrawing();
    const Dimension dimension = made(document, radius({0, 0}, {5, 0}, {10, 0}));
    EXPECT_DOUBLE_EQ(dimension.value.measurement, 5.0);
    const std::vector<Line> lines = entitiesOf<Line>(dimension);
    ASSERT_EQ(lines.size(), 3U);
    // The center mark of DIMCEN 2.5, then the line from the curve to the
    // text.
    expectLine(lines[0], {-2.5, 0}, {2.5, 0});
    expectLine(lines[1], {0, -2.5}, {0, 2.5});
    expectLine(lines[2], {5, 0}, {10, 0});
    const std::vector<Solid> arrows = entitiesOf<Solid>(dimension);
    ASSERT_EQ(arrows.size(), 1U);
    expectArrow(arrows[0], {5, 0}, {7.5, 0});
    const MText text = textOf(dimension);
    EXPECT_EQ(text.value, "R5");
    expectNear(text.insertion, {10, 0});
}

TEST(DimensionRadius, TextInside) {
    const Document document = Document::newDrawing();
    // In (2, 0) and on the curve, in (5, 0): both inside.
    for (const Vec2& place : {Vec2{2, 0}, Vec2{5, 0}}) {
        const Dimension dimension = made(document, radius({0, 0}, {5, 0}, place));
        const std::vector<Line> lines = entitiesOf<Line>(dimension);
        ASSERT_EQ(lines.size(), 3U);
        expectLine(lines[2], {0, 0}, {5, 0});
        const std::vector<Solid> arrows = entitiesOf<Solid>(dimension);
        ASSERT_EQ(arrows.size(), 1U);
        expectArrow(arrows[0], {5, 0}, {2.5, 0});
        expectNear(textOf(dimension).insertion, place);
    }
}

TEST(DimensionRadius, TextInTheCenterIsRefused) {
    const Document document = Document::newDrawing();
    EXPECT_EQ(refusalOf(document, radius({0, 0}, {5, 0}, {0, 0})), kDimensionNotRegenerable);
    // Nor can the grip of the text put it there.
    const Dimension dimension = made(document, radius({0, 0}, {5, 0}, {10, 0}));
    const std::vector<Vec2> grips = gripPointsOf(dimension);
    ASSERT_EQ(grips.size(), 3U);
    const GripResult result = moveGripOf(document, dimension, 2, {0, 0});
    EXPECT_EQ(result.outcome, GripOutcome::Invalid);
    EXPECT_EQ(result.refusal, kDimensionNotRegenerable);
}

TEST(DimensionDiameter, TextOutsideAtEqualDistanceGoesWithPoint15) {
    const Document document = Document::newDrawing();
    const Dimension dimension = made(document, diameter({5, 0}, {-5, 0}, {0, 6}));
    EXPECT_DOUBLE_EQ(dimension.value.measurement, 10.0);
    const std::vector<Line> lines = entitiesOf<Line>(dimension);
    ASSERT_EQ(lines.size(), 3U);
    expectLine(lines[2], {5, 0}, {0, 6});
    const std::vector<Solid> arrows = entitiesOf<Solid>(dimension);
    ASSERT_EQ(arrows.size(), 1U);
    expectArrow(arrows[0], {5, 0}, {7.5, 0});
    EXPECT_EQ(textOf(dimension).value, "%%c10");
}

TEST(DimensionDiameter, TextInside) {
    const Document document = Document::newDrawing();
    const Dimension dimension = made(document, diameter({5, 0}, {-5, 0}, {0, 1}));
    const std::vector<Line> lines = entitiesOf<Line>(dimension);
    ASSERT_EQ(lines.size(), 3U);
    expectLine(lines[2], {-5, 0}, {5, 0});
    const std::vector<Solid> arrows = entitiesOf<Solid>(dimension);
    ASSERT_EQ(arrows.size(), 2U);
    expectArrow(arrows[0], {-5, 0}, {-2.5, 0});
    expectArrow(arrows[1], {5, 0}, {2.5, 0});
}

// --- Center marks -------------------------------------------------------

std::vector<Line> markOf(const Document& document, Dimension dimension, const char* dimcen) {
    dimension.presentation.overrides = {{"DIMCEN", dimcen}};
    std::vector<Line> lines = entitiesOf<Line>(made(document, std::move(dimension)));
    // The last line is the dimension line.
    if (!lines.empty()) {
        lines.pop_back();
    }
    return lines;
}

TEST(DimensionCenterMark, PositiveNegativeAndZero) {
    const Document document = Document::newDrawing();
    const Dimension outside = radius({0, 0}, {5, 0}, {10, 0});
    EXPECT_TRUE(markOf(document, outside, "0").empty());
    std::vector<Line> mark = markOf(document, outside, "2");
    ASSERT_EQ(mark.size(), 2U);
    expectLine(mark[0], {-2, 0}, {2, 0});
    expectLine(mark[1], {0, -2}, {0, 2});
    // DIMCEN -2 on radius 5: the cross from -2 to 2, the center lines from 4
    // to 7 on each side.
    mark = markOf(document, outside, "-2");
    ASSERT_EQ(mark.size(), 6U);
    expectLine(mark[0], {-2, 0}, {2, 0});
    expectLine(mark[1], {0, -2}, {0, 2});
    expectLine(mark[2], {4, 0}, {7, 0});
    expectLine(mark[3], {-4, 0}, {-7, 0});
    expectLine(mark[4], {0, 4}, {0, 7});
    expectLine(mark[5], {0, -4}, {0, -7});
}

TEST(DimensionCenterMark, OnlyTheCrossOnSmallCircles) {
    const Document document = Document::newDrawing();
    // DIMCEN -2 on radius 1 and 2, the text inside: the center lines would
    // end where they begin, or before.
    for (const double r : {1.0, 2.0}) {
        SCOPED_TRACE(r);
        for (const Dimension& dimension :
             {radius({0, 0}, {r, 0}, {r / 2.0, 0}), diameter({r, 0}, {-r, 0}, {0, r / 2.0})}) {
            const std::vector<Line> mark = markOf(document, dimension, "-2");
            ASSERT_EQ(mark.size(), 2U);
            expectLine(mark[0], {-2, 0}, {2, 0});
            expectLine(mark[1], {0, -2}, {0, 2});
        }
    }
}

}  // namespace
}  // namespace tg::core
