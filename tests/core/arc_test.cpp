// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/arc.hpp"

#include "tg/core/document.hpp"
#include "tg/core/math.hpp"
#include "tg/core/proximity.hpp"
#include "tg/core/snap_points.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-10;

/// The value of `optional`, or a default value and a failure if it has none.
template <typename T>
T valueOf(const std::optional<T>& optional) {
    EXPECT_TRUE(optional.has_value());
    return optional.value_or(T{});
}

/// An arc with its angles in degrees, as DXF writes them.
Arc arc(const Vec2& center, double radius, double startDegrees, double endDegrees) {
    return Arc{center, radius, degreesToRadians(startDegrees), degreesToRadians(endDegrees)};
}

void expectNear(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}

void expectArc(const std::optional<Arc>& optional, const Vec2& center, double radius,
               double startDegrees, double endDegrees) {
    const Arc actual = valueOf(optional);
    expectNear(actual.center, center);
    EXPECT_NEAR(actual.radius, radius, kTolerance);
    EXPECT_NEAR(radiansToDegrees(actual.startAngle), startDegrees, 1e-9);
    EXPECT_NEAR(radiansToDegrees(actual.endAngle), endDegrees, 1e-9);
}

TEST(Arc, SweepRunsCounterClockwiseAcrossZero) {
    EXPECT_NEAR(arc({0.0, 0.0}, 1.0, 300.0, 60.0).geometry().sweep, degreesToRadians(120.0),
                kTolerance);
    EXPECT_NEAR(arc({0.0, 0.0}, 1.0, 0.0, 359.9).geometry().sweep, degreesToRadians(359.9),
                kTolerance);
    EXPECT_NEAR(arc({0.0, 0.0}, 10.0, 0.0, 90.0).length(), 5.0 * kPi, kTolerance);
    EXPECT_NEAR(arc({0.0, 0.0}, 1e-4, 0.0, 90.0).length(), 1e-4 * kPi / 2.0, 1e-18);
}

TEST(Arc, ExactBoundingBoxAcrossEveryAxis) {
    const double s = 8.660254037844386;
    const BBox across0 = arc({0.0, 0.0}, 10.0, 300.0, 60.0).boundingBox();
    expectNear(across0.min(), {5.0, -s});
    expectNear(across0.max(), {10.0, s});
    const BBox across90 = arc({0.0, 0.0}, 10.0, 30.0, 150.0).boundingBox();
    expectNear(across90.min(), {-s, 5.0});
    expectNear(across90.max(), {s, 10.0});
    const BBox across180 = arc({0.0, 0.0}, 10.0, 120.0, 240.0).boundingBox();
    expectNear(across180.min(), {-10.0, -s});
    expectNear(across180.max(), {-5.0, s});
    const BBox across270 = arc({0.0, 0.0}, 10.0, 210.0, 330.0).boundingBox();
    expectNear(across270.min(), {-s, -10.0});
    expectNear(across270.max(), {s, -5.0});
    const BBox almost = arc({0.0, 0.0}, 10.0, 0.0, 359.9).boundingBox();
    expectNear(almost.min(), {-10.0, -10.0});
    expectNear(almost.max(), {10.0, 10.0});
}

TEST(Arc, DistanceInsideAndOutsideItsAngle) {
    const Entity quarter = arc({0.0, 0.0}, 10.0, 0.0, 90.0);
    EXPECT_NEAR(distance(quarter, {12.0, 16.0}), 10.0, kTolerance);
    EXPECT_NEAR(distance(quarter, {3.0, 4.0}), 5.0, kTolerance);
    EXPECT_NEAR(distance(quarter, {14.0, -3.0}), 5.0, kTolerance);  // to the start
    EXPECT_NEAR(distance(quarter, {-5.0, 10.0}), 5.0, kTolerance);  // to the end
    EXPECT_NEAR(distance(quarter, {0.0, 0.0}), 10.0, kTolerance);   // the centre
}

TEST(Arc, WindowAndCrossing) {
    const Entity top = arc({0.0, 0.0}, 10.0, 30.0, 150.0);
    EXPECT_FALSE(isInside(top, BBox{{-2.0, 8.0}, {2.0, 12.0}}));
    EXPECT_TRUE(intersects(top, BBox{{-2.0, 8.0}, {2.0, 12.0}}));  // no end point inside
    EXPECT_TRUE(isInside(top, BBox{{-10.0, 0.0}, {10.0, 12.0}}));
    EXPECT_TRUE(intersects(top, BBox{{-10.0, 0.0}, {10.0, 12.0}}));
    EXPECT_FALSE(intersects(top, BBox{{-5.0, 0.0}, {5.0, 4.0}}));
    // Below the arc, inside its circle: not touched.
    EXPECT_FALSE(intersects(top, BBox{{-2.0, -9.0}, {2.0, -8.0}}));
}

TEST(Arc, SnapsToEndPointsMidpointAndCentre) {
    std::vector<SnapPoint> points;
    appendSnapPoints(Entity{arc({0.0, 0.0}, 10.0, 300.0, 60.0)}, SnapModes{}, points);
    ASSERT_EQ(points.size(), 4U);
    EXPECT_EQ(points[0].kind, SnapKind::Endpoint);
    expectNear(points[0].point, {5.0, -8.660254037844386});
    EXPECT_EQ(points[1].kind, SnapKind::Endpoint);
    expectNear(points[1].point, {5.0, 8.660254037844386});
    EXPECT_EQ(points[2].kind, SnapKind::Midpoint);
    expectNear(points[2].point, {10.0, 0.0});  // halfway along the arc, across 0°
    EXPECT_EQ(points[3], (SnapPoint{{0.0, 0.0}, SnapKind::Center}));
}

TEST(ArcThroughPoints, StoredCounterClockwiseWhateverTheDirection) {
    expectArc(arcThroughPoints({10.0, 0.0}, {0.0, 10.0}, {-10.0, 0.0}), {0.0, 0.0}, 10.0, 0.0,
              180.0);
    expectArc(arcThroughPoints({10.0, 0.0}, {0.0, -10.0}, {-10.0, 0.0}), {0.0, 0.0}, 10.0, 180.0,
              0.0);
    expectArc(arcThroughPoints({0.0, -10.0}, {10.0, 0.0}, {0.0, 10.0}), {0.0, 0.0}, 10.0, 270.0,
              90.0);
    expectArc(arcThroughPoints({0.0, 10.0}, {10.0, 0.0}, {0.0, -10.0}), {0.0, 0.0}, 10.0, 270.0,
              90.0);
    expectArc(arcThroughPoints({0.0, 0.0}, {2.0, 2.0}, {4.0, 0.0}), {2.0, 0.0}, 2.0, 0.0, 180.0);
    expectArc(arcThroughPoints({4.0, 0.0}, {2.0, 2.0}, {0.0, 0.0}), {2.0, 0.0}, 2.0, 0.0, 180.0);
}

TEST(ArcThroughPoints, MiddlePointLiesOnTheStoredArc) {
    const ArcGeometry geometry =
        valueOf(arcThroughPoints({1.0, 0.0}, {-1.0, 0.0}, {0.0, -1.0})).geometry();
    EXPECT_GT(geometry.sweep, kPi);  // three quarters of the circle
    EXPECT_TRUE(containsAngle(geometry, kPi));
}

TEST(ArcThroughPoints, CollinearNearlyCollinearAndCoincident) {
    EXPECT_FALSE(arcThroughPoints({0.0, 0.0}, {5.0, 5.0}, {10.0, 10.0}));
    EXPECT_FALSE(arcThroughPoints({0.0, 0.0}, {10.0, 0.0}, {10.0, 0.0}));
    EXPECT_FALSE(arcThroughPoints({0.0, 0.0}, {5.0, 9e-9}, {10.0, 0.0}));
    EXPECT_FALSE(arcThroughPoints({0.0, 0.0}, {5.0, 1e-8}, {10.0, 0.0}));
    const Arc flat = valueOf(arcThroughPoints({0.0, 0.0}, {5.0, 2e-8}, {10.0, 0.0}));
    EXPECT_EQ(flat.center, (Vec2{5.0, -625000000.0}));
    EXPECT_NEAR(radiansToDegrees(flat.startAngle), 89.99999954163377, 1e-9);
    EXPECT_NEAR(radiansToDegrees(flat.endAngle), 90.00000045836623, 1e-9);
}

}  // namespace
}  // namespace tg::core
