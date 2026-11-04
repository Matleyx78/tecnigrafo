// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/offset.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <numbers>
#include <string>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

// Points that solid-space constructs are rounded within 2^-48 of the
// magnitude, curves within 2^-46 (solid_space/geom2d/offset.hpp): for these
// small drawings 1e-12 is far above that error and far below any distance
// that matters.
constexpr double kBuilt = 1e-12;
// The same at coordinates of about 1e6 (docs/adr/0036, point 4): the curve
// error 2^-46 * 1e6 is about 1.4e-8.
constexpr double kBuilt1e6 = 2e-8;
// The bulge of a joint arc that turns a quarter clockwise: -tan(pi / 8).
const double kQuarterClockwise = -std::tan(std::numbers::pi / 8.0);

const ToleranceRule kMillimetres{4};

Entity line(Vec2 start, Vec2 end) {
    return Line{start, end};
}

Entity circle(Vec2 center, double radius) {
    return Circle{center, radius};
}

/// An ARC from `startDegrees` to `endDegrees`, counter-clockwise.
Entity arc(Vec2 center, double radius, double startDegrees, double endDegrees) {
    return Arc{center, radius, normalizeAngle(degreesToRadians(startDegrees)),
               normalizeAngle(degreesToRadians(endDegrees))};
}

/// A vertex as the tests write it: position, bulge and widths.
struct V {
    double x{0.0};
    double y{0.0};
    double bulge{0.0};
    double startWidth{0.0};
    double endWidth{0.0};
};

Polyline polyline(std::initializer_list<V> vertices, bool closed = false) {
    std::vector<PolylineVertex> list;
    for (const V& vertex : vertices) {
        list.push_back(PolylineVertex{Vec2{vertex.x, vertex.y}, vertex.bulge, vertex.startWidth,
                                      vertex.endWidth});
    }
    return Polyline{std::move(list), closed};
}

OffsetResult offset(const Entity& entity, double distance, Vec2 side) {
    return offsetEntity(entity, distance, side, kMillimetres);
}

void expectNear(Vec2 actual, Vec2 expected, double tolerance) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
}

/// The copy is a polyline with these vertices, open or closed as said.
void expectPolyline(const OffsetResult& result, std::initializer_list<V> vertices, bool closed,
                    double tolerance = kBuilt) {
    ASSERT_EQ(result.outcome, OffsetOutcome::Done);
    ASSERT_TRUE(result.entity.has_value());
    const auto* copy = std::get_if<Polyline>(&*result.entity);
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->isClosed(), closed);
    ASSERT_EQ(copy->vertexCount(), vertices.size());
    std::size_t i = 0;
    for (const V& expected : vertices) {
        const PolylineVertex& actual = copy->vertices()[i];
        SCOPED_TRACE("vertex " + std::to_string(i));
        expectNear(actual.position, Vec2{expected.x, expected.y}, tolerance);
        EXPECT_NEAR(actual.bulge, expected.bulge, kBuilt);
        EXPECT_EQ(actual.startWidth, expected.startWidth);
        EXPECT_EQ(actual.endWidth, expected.endWidth);
        ++i;
    }
}

// ---------------------------------------------------------------------------
// Lines, circles and arcs (autocad-offset.dxf, cases a, b and c)

TEST(Offset, LineOnBothSides) {
    const Entity l = line({0, 0}, {20, 0});

    const OffsetResult left = offset(l, 5, {10, 10});
    ASSERT_EQ(left.outcome, OffsetOutcome::Done);
    const auto& up = std::get<Line>(*left.entity);
    expectNear(up.start, {0, 5}, kBuilt);
    expectNear(up.end, {20, 5}, kBuilt);

    const OffsetResult right = offset(l, 5, {10, -10});
    ASSERT_EQ(right.outcome, OffsetOutcome::Done);
    const auto& down = std::get<Line>(*right.entity);
    expectNear(down.start, {0, -5}, kBuilt);
    expectNear(down.end, {20, -5}, kBuilt);
}

TEST(Offset, CircleInsideAndOutside) {
    const Entity c = circle({50, 0}, 10);

    const OffsetResult inside = offset(c, 3, {50, 1});
    ASSERT_EQ(inside.outcome, OffsetOutcome::Done);
    const auto& smaller = std::get<Circle>(*inside.entity);
    EXPECT_EQ(smaller.center, (Vec2{50, 0}));
    EXPECT_NEAR(smaller.radius, 7, kBuilt);

    const OffsetResult outside = offset(c, 3, {50, 20});
    ASSERT_EQ(outside.outcome, OffsetOutcome::Done);
    EXPECT_NEAR(std::get<Circle>(*outside.entity).radius, 13, kBuilt);
}

TEST(Offset, ArcKeepsItsAnglesBitForBit) {
    const Entity a = arc({100, 0}, 10, 0, 180);
    const Arc& source = std::get<Arc>(a);

    const OffsetResult inside = offset(a, 4, {100, 1});
    ASSERT_EQ(inside.outcome, OffsetOutcome::Done);
    const auto& smaller = std::get<Arc>(*inside.entity);
    EXPECT_EQ(smaller.center, source.center);
    EXPECT_NEAR(smaller.radius, 6, kBuilt);
    EXPECT_EQ(smaller.startAngle, source.startAngle);
    EXPECT_EQ(smaller.endAngle, source.endAngle);

    const OffsetResult outside = offset(a, 4, {100, 15});
    ASSERT_EQ(outside.outcome, OffsetOutcome::Done);
    const auto& larger = std::get<Arc>(*outside.entity);
    EXPECT_NEAR(larger.radius, 14, kBuilt);
    EXPECT_EQ(larger.startAngle, source.startAngle);
    EXPECT_EQ(larger.endAngle, source.endAngle);
}

TEST(Offset, ArcThroughZeroDegrees) {
    const Entity a = arc({0, 0}, 10, 300, 60);
    const OffsetResult inside = offset(a, 2, {0, 0});
    ASSERT_EQ(inside.outcome, OffsetOutcome::Done);
    const auto& copy = std::get<Arc>(*inside.entity);
    EXPECT_NEAR(copy.radius, 8, kBuilt);
    EXPECT_EQ(copy.startAngle, std::get<Arc>(a).startAngle);
    EXPECT_EQ(copy.endAngle, std::get<Arc>(a).endAngle);

    const OffsetResult outside = offset(a, 2, {20, 0});
    ASSERT_EQ(outside.outcome, OffsetOutcome::Done);
    EXPECT_NEAR(std::get<Arc>(*outside.entity).radius, 12, kBuilt);
}

TEST(Offset, ArcAndCircleVanishAtTheirRadius) {
    EXPECT_EQ(offset(circle({50, 0}, 10), 10, {50, 0}).outcome, OffsetOutcome::Vanished);
    EXPECT_EQ(offset(circle({50, 0}, 10), 12, {50, 0}).outcome, OffsetOutcome::Vanished);
    EXPECT_EQ(offset(arc({100, 0}, 10, 0, 180), 10, {100, 5}).outcome, OffsetOutcome::Vanished);
    const OffsetResult vanished = offset(circle({50, 0}, 10), 10, {50, 0});
    EXPECT_FALSE(vanished.entity.has_value());
}

TEST(Offset, PointExactlyOnTheCurveGivesNoSide) {
    EXPECT_EQ(offset(line({0, 0}, {20, 0}), 5, {10, 0}).outcome, OffsetOutcome::NoSide);
    // On the extension of the line: still on the line.
    EXPECT_EQ(offset(line({0, 0}, {20, 0}), 5, {30, 0}).outcome, OffsetOutcome::NoSide);
    EXPECT_EQ(offset(circle({50, 0}, 10), 3, {60, 0}).outcome, OffsetOutcome::NoSide);
    EXPECT_EQ(offset(arc({100, 0}, 10, 0, 180), 4, {110, 0}).outcome, OffsetOutcome::NoSide);
    // On the circle of the arc, outside the arc itself.
    EXPECT_EQ(offset(arc({100, 0}, 10, 0, 180), 4, {100, -10}).outcome, OffsetOutcome::NoSide);
    const Polyline rectangle =
        polyline({{150, -25}, {250, -25}, {250, 25}, {150, 25}}, /*closed=*/true);
    EXPECT_EQ(offset(rectangle, 10, {200, -25}).outcome, OffsetOutcome::NoSide);
}

TEST(Offset, NoSideAtAllFails) {
    // Decision B5 of #464: a curve that is exactly a point has no side.
    const OffsetResult point = offset(line({0, 0}, {0, 0}), 1, {0, 1});
    EXPECT_EQ(point.outcome, OffsetOutcome::Failed);
    EXPECT_FALSE(point.entity.has_value());
    EXPECT_EQ(offset(Polyline{}, 1, {0, 1}).outcome, OffsetOutcome::Failed);
    EXPECT_EQ(offset(polyline({{3, 4}}), 1, {0, 1}).outcome, OffsetOutcome::Failed);
    EXPECT_EQ(offset(polyline({{3, 4}, {3, 4}}), 1, {0, 1}).outcome, OffsetOutcome::Failed);
}

TEST(Offset, DiagnosisOfOffsetCurveFails) {
    // The radius 2^50 + 1 is beyond solid-space's domain: offsetCurve gives a
    // diagnosis (outOfDomain, input 1), and the copy fails.
    constexpr double kRadius = 0x1p50;
    const OffsetResult beyond = offset(circle({0, 0}, kRadius), 1, {0.8 * kRadius, 0.8 * kRadius});
    EXPECT_EQ(beyond.outcome, OffsetOutcome::Failed);
    EXPECT_FALSE(beyond.entity.has_value());
}

// ---------------------------------------------------------------------------
// Polylines (autocad-offset.dxf, cases d-h; request Matleyx78/solid-space#22)

TEST(Offset, RectangleInsideAndOutside) {
    // Case d, request case 1.
    const Polyline rectangle =
        polyline({{150, -25}, {250, -25}, {250, 25}, {150, 25}}, /*closed=*/true);

    const OffsetResult inside = offset(rectangle, 10, {200, 0});
    expectPolyline(inside, {{160, -15}, {240, -15}, {240, 15}, {160, 15}}, true);
    EXPECT_TRUE(inside.simple);

    const OffsetResult outside = offset(rectangle, 10, {200, -35});
    expectPolyline(outside, {{140, -35}, {260, -35}, {260, 35}, {140, 35}}, true);
    EXPECT_TRUE(outside.simple);
}

TEST(Offset, SlotWithHalfCircles) {
    // Case e, request case 2.
    const Polyline slot =
        polyline({{300, -10}, {340, -10, 1}, {340, 10}, {300, 10, 1}}, /*closed=*/true);
    expectPolyline(offset(slot, 4, {320, 0}), {{300, -6}, {340, -6, 1}, {340, 6}, {300, 6, 1}},
                   true);
    expectPolyline(offset(slot, 4, {320, -20}),
                   {{300, -14}, {340, -14, 1}, {340, 14}, {300, 14, 1}}, true);
}

TEST(Offset, SlotInsideByItsRadiusOrMoreFails) {
    // Request case 2: the half circles vanish and the shifted lines do not meet.
    const Polyline slot = polyline({{0, -5}, {20, -5, 1}, {20, 5}, {0, 5, 1}}, /*closed=*/true);
    EXPECT_EQ(offset(slot, 5, {10, 0}).outcome, OffsetOutcome::Failed);
    EXPECT_EQ(offset(slot, 6, {10, 0}).outcome, OffsetOutcome::Failed);
}

TEST(Offset, NarrowU) {
    // Case f, request case 5: with 4 the bottom turns backwards and the two
    // sides no longer meet. AutoCAD makes nothing either.
    const Polyline u = polyline({{400, 0}, {400, -20}, {406, -20}, {406, 0}});
    expectPolyline(offset(u, 2, {403, -10}), {{402, 0}, {402, -18}, {404, -18}, {404, 0}}, false);
    const OffsetResult tooNarrow = offset(u, 4, {403, -10});
    EXPECT_EQ(tooNarrow.outcome, OffsetOutcome::Failed);
    EXPECT_FALSE(tooNarrow.entity.has_value());
}

TEST(Offset, HollowWithAnArcGetsJointArcsWithoutWidth) {
    // Case g with distance 2: the arc of radius 3 becomes radius 1, and the
    // joints are arcs of radius 2 centred on the vertices, which have no
    // width; the shifted pieces keep the widths of their sources.
    const Polyline hollow =
        polyline({{450, 0, 0, 1, 2}, {470, 0, 1, 3, 4}, {476, 0, 0, 5, 6}, {496, 0}});
    const OffsetResult result = offset(hollow, 2, {480, 10});
    expectPolyline(result,
                   {{450, 2, 0, 1, 2},
                    {470, 2, kQuarterClockwise},
                    {472, 0, 1, 3, 4},
                    {474, 0, kQuarterClockwise},
                    {476, 2, 0, 5, 6},
                    {496, 2}},
                   false);
    EXPECT_TRUE(result.simple);
}

TEST(Offset, HollowWhoseArcVanishesFails) {
    // Case g with distance 5, request case 4: the arc vanishes and the two
    // lines, on the same line y = 5, do not meet (openJoint).
    const Polyline hollow = polyline({{450, 0}, {470, 0, 1}, {476, 0}, {496, 0}});
    EXPECT_EQ(offset(hollow, 5, {480, 10}).outcome, OffsetOutcome::Failed);
}

TEST(Offset, ShortSideOfAnLIsRemoved) {
    // Request case 3: the short side, cut by the joint, turns backwards and is
    // removed; the long side keeps its cut. The copy is not simple, and its
    // only segment takes the widths of segment 1, not of segment 0.
    const Polyline l = polyline({{0, 0, 0, 1, 2}, {4, 0, 0, 3, 4}, {4, 20}});
    const OffsetResult result = offset(l, 5, {2, 10});
    expectPolyline(result, {{-1, 5, 0, 3, 4}, {-1, 20}}, false);
    EXPECT_FALSE(result.simple);
}

TEST(Offset, RectangleAndLAtCoordinatesOf1e6) {
    // Case h, request case 6: the same answers for the translated data.
    const Polyline rectangle =
        polyline({{1000150, 999975}, {1000250, 999975}, {1000250, 1000025}, {1000150, 1000025}},
                 /*closed=*/true);
    expectPolyline(offset(rectangle, 10, {1000200, 1000000}),
                   {{1000160, 999985}, {1000240, 999985}, {1000240, 1000015}, {1000160, 1000015}},
                   true, kBuilt1e6);

    const Polyline l = polyline({{1e6, 1e6}, {1e6 + 4, 1e6}, {1e6 + 4, 1e6 + 20}});
    expectPolyline(offset(l, 5, {1e6 + 2, 1e6 + 10}), {{1e6 - 1, 1e6 + 5}, {1e6 - 1, 1e6 + 20}},
                   false, kBuilt1e6);
}

TEST(Offset, OpenPolylineWithAnArcAtACorner) {
    // The counter-clockwise half circle from (10,0) to (20,0) passes below
    // the line, so (10,0) is a sharp corner. Moved to the left by 2, the line
    // is y = 2 and the arc keeps its centre (15,0) with radius 3: they meet
    // at (15 - sqrt(5), 2), the one nearer to (10,0) (step 2 of
    // offsetPolyline). The arc reaches it extending backwards by asin(2/3).
    const Polyline open = polyline({{0, 0}, {10, 0, 1}, {20, 0}});
    const double bulge = std::tan((std::numbers::pi + std::asin(2.0 / 3.0)) / 4.0);
    expectPolyline(offset(open, 2, {5, 5}), {{0, 2}, {15 - std::sqrt(5.0), 2, bulge}, {18, 0}},
                   false);
}

TEST(Offset, ClockwiseArcSegmentInsideIsOnTheRight) {
    // A clockwise half circle from (0,0) to (10,0) over the top: its centre
    // (5,0) is on its right, so the signed distance is negative and the
    // radius shrinks from 5 to 3 (offset.hpp: R + distance when clockwise).
    const Polyline half = polyline({{0, 0, -1}, {10, 0}});
    expectPolyline(offset(half, 2, {5, 0}), {{2, 0, -1}, {8, 0}}, false);
    expectPolyline(offset(half, 2, {5, 10}), {{-2, 0, -1}, {12, 0}}, false);
}

TEST(Offset, WidthsAndPropertiesOfThePolyline) {
    Polyline constant = polyline({{0, 0}, {10, 0}, {10, 10}});
    constant.setConstantWidth(0.5);
    constant.setContinuousLinetype(true);
    const OffsetResult result = offset(constant, 2, {5, 5});
    expectPolyline(result, {{0, 2}, {8, 2}, {8, 10}}, false);
    const auto& copy = std::get<Polyline>(*result.entity);
    EXPECT_EQ(copy.constantWidth(), 0.5);
    EXPECT_TRUE(copy.continuousLinetype());

    const Polyline perSegment = polyline({{0, 0, 0, 1, 2}, {10, 0, 0, 3, 4}, {10, 10, 0, 7, 8}});
    // The widths of the last vertex of an open polyline belong to no segment.
    expectPolyline(offset(perSegment, 2, {5, 5}), {{0, 2, 0, 1, 2}, {8, 2, 0, 3, 4}, {8, 10}},
                   false);

    const Polyline closed =
        polyline({{0, 0, 0, 1, 2}, {10, 0, 0, 3, 4}, {10, 10, 0, 5, 6}, {0, 10, 0, 7, 8}},
                 /*closed=*/true);
    expectPolyline(offset(closed, 1, {5, 5}),
                   {{1, 1, 0, 1, 2}, {9, 1, 0, 3, 4}, {9, 9, 0, 5, 6}, {1, 9, 0, 7, 8}}, true);
}

TEST(Offset, AlmostStraightSegmentPassesAsStraight) {
    // A bulge that PolylineSegment::isArc() treats as straight passes to
    // solid-space with bulge 0 (docs/adr/0035, point 1): the copy is straight.
    const Polyline nearlyStraight = polyline({{0, 0, 1e-13}, {10, 0}});
    expectPolyline(offset(nearlyStraight, 1, {5, 5}), {{0, 1}, {10, 1}}, false);
}

}  // namespace
}  // namespace tg::core
