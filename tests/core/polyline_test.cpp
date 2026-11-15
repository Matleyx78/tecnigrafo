// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/polyline.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace tg::core {
namespace {

constexpr double kEps = 1e-9;

void expectNear(const Vec2& actual, const Vec2& expected, double tolerance = kEps) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
}

Polyline openSquareOfSide(double side) {
    Polyline polyline;
    polyline.addVertex({0.0, 0.0});
    polyline.addVertex({side, 0.0});
    polyline.addVertex({side, side});
    polyline.addVertex({0.0, side});
    return polyline;
}

TEST(Polyline, SegmentCountDependsOnClosure) {
    Polyline polyline = openSquareOfSide(1.0);
    EXPECT_EQ(polyline.segmentCount(), 3U);
    polyline.setClosed(true);
    EXPECT_EQ(polyline.segmentCount(), 4U);
    EXPECT_EQ(Polyline{}.segmentCount(), 0U);
}

TEST(Polyline, LengthOfStraightSegments) {
    Polyline polyline = openSquareOfSide(2.0);
    EXPECT_NEAR(polyline.length(), 6.0, kEps);
    polyline.setClosed(true);
    EXPECT_NEAR(polyline.length(), 8.0, kEps);
}

TEST(Polyline, BulgeOfOneIsAHalfCircle) {
    // Counter-clockwise half circle from (0,0) to (2,0): passes below, through (1,-1).
    const ArcGeometry arc = arcFromBulge({0.0, 0.0}, {2.0, 0.0}, 1.0);
    expectNear(arc.center, {1.0, 0.0});
    EXPECT_NEAR(arc.radius, 1.0, kEps);
    EXPECT_NEAR(arc.sweep, kPi, kEps);

    const PolylineSegment segment{{0.0, 0.0}, {2.0, 0.0}, 1.0};
    EXPECT_NEAR(segment.length(), kPi, kEps);
    const BBox box = segment.boundingBox();
    expectNear(box.min(), {0.0, -1.0});
    expectNear(box.max(), {2.0, 0.0});
}

TEST(Polyline, NegativeBulgeIsClockwise) {
    const PolylineSegment segment{{0.0, 0.0}, {2.0, 0.0}, -1.0};
    const BBox box = segment.boundingBox();
    expectNear(box.min(), {0.0, 0.0});
    expectNear(box.max(), {2.0, 1.0});
}

TEST(Polyline, QuarterArcGeometry) {
    // 90 degree arc, radius 10, centered at the origin, from (10,0) to (0,10).
    const double bulge = std::tan(degreesToRadians(90.0) / 4.0);
    const ArcGeometry arc = arcFromBulge({10.0, 0.0}, {0.0, 10.0}, bulge);
    expectNear(arc.center, {0.0, 0.0});
    EXPECT_NEAR(arc.radius, 10.0, kEps);
    EXPECT_NEAR(arc.startAngle, 0.0, kEps);
    EXPECT_NEAR(arc.sweep, kPi / 2.0, kEps);
}

TEST(Polyline, TessellationRespectsToleranceAndEndPoints) {
    const PolylineSegment segment{{0.0, 0.0}, {2.0, 0.0}, 1.0};
    constexpr double kTolerance = 1e-3;
    std::vector<Vec2> points{segment.start};
    segment.tessellate(kTolerance, points);

    ASSERT_GT(points.size(), 10U);
    EXPECT_EQ(points.back(), segment.end);
    for (const Vec2& point : points) {
        EXPECT_NEAR(distance(point, {1.0, 0.0}), 1.0, kEps);  // every point is on the arc
    }
    // The sagitta of each chord must stay within the tolerance.
    for (std::size_t i = 1; i < points.size(); ++i) {
        const double halfChord = distance(points[i - 1], points[i]) / 2.0;
        const double sagitta = 1.0 - std::sqrt(1.0 - (halfChord * halfChord));
        EXPECT_LE(sagitta, kTolerance + kEps);
    }
}

TEST(Polyline, ClosedTessellationDoesNotRepeatFirstPoint) {
    Polyline polyline = openSquareOfSide(1.0);
    polyline.setClosed(true);
    const std::vector<Vec2> points = polyline.tessellate(0.01);
    ASSERT_EQ(points.size(), 4U);
    EXPECT_EQ(points.front(), Vec2(0.0, 0.0));
    EXPECT_EQ(points.back(), Vec2(0.0, 1.0));
}

TEST(Polyline, BoundingBoxIncludesArcExtremes) {
    // A 2 x 0 "stadium": two half circles of radius 1 joined at their ends.
    const Polyline stadium({{{0.0, 0.0}, 1.0}, {{2.0, 0.0}, 1.0}}, true);
    const BBox box = stadium.boundingBox();
    expectNear(box.min(), {0.0, -1.0});
    expectNear(box.max(), {2.0, 1.0});
    EXPECT_NEAR(stadium.length(), 2.0 * kPi, kEps);
}

TEST(Polyline, WidthsAreZeroByDefault) {
    const Polyline polyline = openSquareOfSide(1.0);
    EXPECT_EQ(polyline.constantWidth(), 0.0);
    for (const PolylineVertex& vertex : polyline.vertices()) {
        EXPECT_EQ(vertex.startWidth, 0.0);
        EXPECT_EQ(vertex.endWidth, 0.0);
    }
}

TEST(Polyline, WidthsTakePartInEqualityAndCopies) {
    const std::vector<PolylineVertex> vertices{{{0.0, 0.0}, 0.5, 1.0, 2.0},
                                               {{4.0, 0.0}, 0.0, 2.0, 0.0}};
    Polyline wide(vertices);
    wide.setConstantWidth(0.25);
    EXPECT_EQ(wide.constantWidth(), 0.25);
    EXPECT_EQ(wide.vertices()[0].startWidth, 1.0);
    EXPECT_EQ(wide.vertices()[0].endWidth, 2.0);

    const Polyline copy = wide;
    EXPECT_EQ(copy, wide);

    Polyline otherConstant = wide;
    otherConstant.setConstantWidth(0.5);
    EXPECT_NE(otherConstant, wide);

    std::vector<PolylineVertex> otherVertices = vertices;
    otherVertices[1].endWidth = 3.0;
    Polyline otherVertex(otherVertices);
    otherVertex.setConstantWidth(0.25);
    EXPECT_NE(otherVertex, wide);
}

TEST(Polyline, WidthsChangeNeitherLengthNorBoundingBox) {
    const std::vector<PolylineVertex> thin{
        {{0.0, 0.0}, 1.0}, {{10.0, 0.0}, 0.0}, {{10.0, 5.0}, 0.0}};
    std::vector<PolylineVertex> thick = thin;
    for (PolylineVertex& vertex : thick) {
        vertex.startWidth = 3.0;
        vertex.endWidth = 7.0;
    }
    const Polyline plain(thin);
    Polyline wide(thick);
    wide.setConstantWidth(4.0);

    EXPECT_EQ(wide.length(), plain.length());
    EXPECT_EQ(wide.boundingBox().min(), plain.boundingBox().min());
    EXPECT_EQ(wide.boundingBox().max(), plain.boundingBox().max());
    EXPECT_EQ(wide.tessellate(0.01), plain.tessellate(0.01));
}

TEST(Polyline, ToPolylineOfALineIsOneStraightSegment) {
    const Polyline polyline = toPolyline(Line{{-2.0, 3.0}, {4.0, 7.0}});
    EXPECT_EQ(polyline, (Polyline{{PolylineVertex{{-2.0, 3.0}}, PolylineVertex{{4.0, 7.0}}}}));
}

TEST(Polyline, ToPolylineOfAnArcKeepsItsEndsAndBulge) {
    const Polyline quarter = toPolyline(Arc{{0.0, 0.0}, 10.0, 0.0, kPi / 2.0});
    ASSERT_EQ(quarter.vertexCount(), 2U);
    EXPECT_FALSE(quarter.isClosed());
    EXPECT_EQ(quarter.vertices()[0].position, (Vec2{10.0, 0.0}));
    EXPECT_EQ(quarter.vertices()[1].position, (Vec2{0.0, 10.0}));
    EXPECT_NEAR(quarter.vertices()[0].bulge, std::sqrt(2.0) - 1.0, kEps);
    EXPECT_EQ(quarter.vertices()[1].bulge, 0.0);

    // Across angle 0: the sweep is end - start + 2*pi, counter-clockwise.
    const Polyline across = toPolyline(Arc{{1.0, 1.0}, 2.0, 3.0 * kPi / 2.0, kPi / 2.0});
    expectNear(across.vertices()[0].position, {1.0, -1.0});
    expectNear(across.vertices()[1].position, {1.0, 3.0});
    EXPECT_NEAR(across.vertices()[0].bulge, 1.0, kEps);
}

TEST(Polyline, ReversedOpenPolylineMovesBulgesAndWidthsToTheOppositeSegment) {
    const Polyline polyline{{{{0.0, 0.0}, 0.5, 1.0, 2.0},
                             {{10.0, 0.0}, -0.25, 3.0, 4.0},
                             {{20.0, 0.0}, 0.75, 5.0, 6.0}}};
    const Polyline expected{{{{20.0, 0.0}, 0.25, 4.0, 3.0},
                             {{10.0, 0.0}, -0.5, 2.0, 1.0},
                             {{0.0, 0.0}, 0.75, 5.0, 6.0}}};
    EXPECT_EQ(reversed(polyline), expected);
}

TEST(Polyline, ReversedTwiceGivesBackAnOpenPolylineWithDataOnItsLastVertex) {
    Polyline polyline{
        {{{0.0, 0.0}, 1.0, 0.5, 1.5}, {{4.0, 0.0}, 0.0, 2.0, 2.5}, {{4.0, 4.0}, -0.5, 3.0, 3.5}}};
    polyline.setConstantWidth(0.25);
    polyline.setContinuousLinetype(true);
    const Polyline once = reversed(polyline);
    EXPECT_EQ(once.vertices().back(), (PolylineVertex{{0.0, 0.0}, -0.5, 3.0, 3.5}));
    EXPECT_EQ(once.constantWidth(), 0.25);
    EXPECT_TRUE(once.continuousLinetype());
    EXPECT_EQ(reversed(once), polyline);
}

TEST(Polyline, ReversedClosedPolylineKeepsItsFirstVertex) {
    const Polyline polyline{{{{0.0, 0.0}, 0.5, 1.0, 2.0},
                             {{10.0, 0.0}, -0.25, 3.0, 4.0},
                             {{10.0, 10.0}, 1.0, 5.0, 6.0}},
                            true};
    const Polyline expected{{{{0.0, 0.0}, -1.0, 6.0, 5.0},
                             {{10.0, 10.0}, 0.25, 4.0, 3.0},
                             {{10.0, 0.0}, -0.5, 2.0, 1.0}},
                            true};
    EXPECT_EQ(reversed(polyline), expected);
    EXPECT_EQ(reversed(expected), polyline);
}

}  // namespace
}  // namespace tg::core
