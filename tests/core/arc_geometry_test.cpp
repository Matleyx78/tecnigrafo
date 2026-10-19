// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/arc_geometry.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-10;

void expectNear(const Vec2& actual, const Vec2& expected, double tolerance = kTolerance) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
}

ArcGeometry degrees(const Vec2& center, double radius, double start, double sweep) {
    return ArcGeometry{center, radius, degreesToRadians(start), degreesToRadians(sweep)};
}

TEST(NormalizeAngle, BringsAnyAngleIntoOneTurn) {
    EXPECT_EQ(normalizeAngle(0.0), 0.0);
    EXPECT_FALSE(std::signbit(normalizeAngle(-0.0)));
    EXPECT_NEAR(normalizeAngle(-kPi / 2.0), 3.0 * kPi / 2.0, kTolerance);
    EXPECT_NEAR(normalizeAngle(5.0 * kPi), kPi, kTolerance);
    EXPECT_EQ(normalizeAngle(kTwoPi), 0.0);
    // A tiny negative angle must not round up to 2π.
    const double justBelow = normalizeAngle(-std::numeric_limits<double>::denorm_min());
    EXPECT_GE(justBelow, 0.0);
    EXPECT_LT(justBelow, kTwoPi);
}

TEST(ArcGeometry, PointsAndContainedAngles) {
    const ArcGeometry arc = degrees({1.0, 2.0}, 10.0, 300.0, 120.0);  // across 0°
    expectNear(startPoint(arc), {6.0, 2.0 - 8.660254037844386});
    expectNear(endPoint(arc), {6.0, 2.0 + 8.660254037844386});
    expectNear(pointAt(arc, 0.0), {11.0, 2.0});
    EXPECT_TRUE(containsAngle(arc, 0.0));
    EXPECT_TRUE(containsAngle(arc, degreesToRadians(-50.0)));
    EXPECT_FALSE(containsAngle(arc, kPi));
    const ArcGeometry clockwise = degrees({0.0, 0.0}, 1.0, 90.0, -90.0);
    EXPECT_TRUE(containsAngle(clockwise, degreesToRadians(45.0)));
    EXPECT_FALSE(containsAngle(clockwise, degreesToRadians(135.0)));
    const ArcGeometry circle{{0.0, 0.0}, 1.0, 0.0, kTwoPi};
    EXPECT_TRUE(containsAngle(circle, 4.0));
}

TEST(ArcGeometry, ExactBoundingBoxAcrossEveryAxis) {
    struct Case {
        double start{0.0};
        double sweep{0.0};
        Vec2 min;
        Vec2 max;
    };

    const double s = 8.660254037844386;  // 10 · sin 60°
    for (const Case& test : {
             Case{300.0, 120.0, {5.0, -s}, {10.0, s}},    // across 0°
             Case{30.0, 120.0, {-s, 5.0}, {s, 10.0}},     // across 90°
             Case{120.0, 120.0, {-10.0, -s}, {-5.0, s}},  // across 180°
             Case{210.0, 120.0, {-s, -10.0}, {s, -5.0}},  // across 270°
             Case{30.0, 30.0, {5.0, 5.0}, {s, s}},        // within one quadrant
             Case{60.0, -30.0, {5.0, 5.0}, {s, s}},       // the same, clockwise
         }) {
        const BBox box = boundingBox(degrees({0.0, 0.0}, 10.0, test.start, test.sweep));
        expectNear(box.min(), test.min);
        expectNear(box.max(), test.max);
    }
}

TEST(ArcGeometry, AlmostCompleteAndTinyArcs) {
    const BBox almost = boundingBox(degrees({0.0, 0.0}, 10.0, 0.0, 359.9));
    expectNear(almost.min(), {-10.0, -10.0});
    expectNear(almost.max(), {10.0, 10.0});

    const BBox tiny = boundingBox(degrees({0.0, 0.0}, 1e-4, 0.0, 90.0));
    expectNear(tiny.min(), {0.0, 0.0}, 1e-15);
    expectNear(tiny.max(), {1e-4, 1e-4}, 1e-15);

    // An arc of a thousandth of a degree that crosses nothing.
    const BBox sliver = boundingBox(degrees({0.0, 0.0}, 1.0, 45.0, 0.001));
    EXPECT_LT(sliver.width(), 1e-4);
    EXPECT_LT(sliver.height(), 1e-4);
}

TEST(ArcGeometry, CircleBoundingBoxIsExactAtAnyScale) {
    for (const double radius : {1e-6, 1.0, 1e6}) {
        const BBox box = boundingBox(ArcGeometry{{100.0, 100.0}, radius, 0.0, kTwoPi});
        EXPECT_EQ(box.min(), (Vec2{100.0 - radius, 100.0 - radius}));
        EXPECT_EQ(box.max(), (Vec2{100.0 + radius, 100.0 + radius}));
    }
}

TEST(ArcGeometry, ClosestPoint) {
    const ArcGeometry arc = degrees({0.0, 0.0}, 10.0, 0.0, 90.0);
    expectNear(closestPoint(arc, {12.0, 16.0}), {6.0, 8.0});
    expectNear(closestPoint(arc, {3.0, 4.0}), {6.0, 8.0});
    expectNear(closestPoint(arc, {14.0, -3.0}), {10.0, 0.0});  // outside, near the start
    expectNear(closestPoint(arc, {-5.0, 10.0}), {0.0, 10.0});  // outside, near the end
    expectNear(closestPoint(arc, {0.0, 0.0}), {10.0, 0.0});    // the centre: the start
    const ArcGeometry circle{{0.0, 0.0}, 10.0, 0.0, kTwoPi};
    expectNear(closestPoint(circle, {0.0, -25.0}), {0.0, -10.0});
    expectNear(closestPoint(circle, {0.0, 0.0}), {10.0, 0.0});
}

TEST(ArcGeometry, IntersectsRectangle) {
    const ArcGeometry top = degrees({0.0, 0.0}, 10.0, 30.0, 120.0);
    // The arc enters the rectangle without an end point inside.
    EXPECT_TRUE(intersects(top, BBox{{-2.0, 8.0}, {2.0, 12.0}}));
    EXPECT_FALSE(intersects(top, BBox{{-5.0, 0.0}, {5.0, 4.0}}));
    EXPECT_TRUE(intersects(top, BBox{{8.0, 4.0}, {9.0, 6.0}}));  // the start point

    const ArcGeometry circle{{0.0, 0.0}, 10.0, 0.0, kTwoPi};
    EXPECT_FALSE(intersects(circle, BBox{{-2.0, -2.0}, {2.0, 2.0}}));  // inside the circle
    EXPECT_TRUE(intersects(circle, BBox{{8.0, -2.0}, {12.0, 2.0}}));
    EXPECT_TRUE(intersects(circle, BBox{{-2.0, 9.0}, {2.0, 11.0}}));   // across the top
    EXPECT_TRUE(intersects(circle, BBox{{10.0, -1.0}, {11.0, 1.0}}));  // tangent side
    EXPECT_FALSE(intersects(circle, BBox{{15.0, 15.0}, {25.0, 25.0}}));
}

TEST(ArcGeometry, TessellationEndsOnTheEndPoint) {
    const ArcGeometry arc = degrees({1.0, 1.0}, 5.0, 10.0, 200.0);
    std::vector<Vec2> points;
    tessellate(arc, 0.01, points);
    ASSERT_GT(points.size(), 2U);
    expectNear(points.back(), endPoint(arc));
    for (const Vec2& point : points) {
        EXPECT_NEAR(distance(point, arc.center), arc.radius, kTolerance);
    }
}

/// The largest distance between the arc and a fine sampling of its chords.
double chordError(const ArcGeometry& arc, const std::vector<Vec2>& polyline) {
    double worst = 0.0;
    for (std::size_t i = 0; i + 1 < polyline.size(); ++i) {
        const Vec2 middle = (polyline[i] + polyline[i + 1]) * 0.5;
        worst = std::max(worst, arc.radius - distance(middle, arc.center));
    }
    return worst;
}

TEST(ArcGeometry, AgreesWithADenseTessellationOnRandomArcsAndCircles) {
    // A fixed seed makes geometry regressions reproducible; this is not security-sensitive.
    // NOLINTNEXTLINE(cert-msc32-c,cert-msc51-cpp)
    std::mt19937 generator{20261019U};
    // Not std::uniform_real_distribution, whose values differ between
    // standard libraries.
    const auto unit = [&generator] {
        return static_cast<double>(generator()) / static_cast<double>(std::mt19937::max());
    };
    const auto coordinate = [&unit] {
        return (unit() * 2000.0) - 1000.0;
    };
    const auto angle = [&unit] {
        return unit() * kTwoPi;
    };
    constexpr int kArcCount = 200;
    for (int i = 0; i < kArcCount; ++i) {
        const bool circle = i % 4 == 0;
        const double radius = 0.01 + (unit() * 500.0);
        const double direction = unit() < 0.5 ? -1.0 : 1.0;
        const double sweep = circle ? kTwoPi : direction * std::max(angle(), 1e-3);
        const ArcGeometry arc{{coordinate(), coordinate()}, radius, angle(), sweep};
        const double fine = radius * 1e-6;

        std::vector<Vec2> dense{startPoint(arc)};
        tessellate(arc, fine, dense);
        BBox denseBox;
        for (const Vec2& point : dense) {
            denseBox.expand(point);
        }
        const BBox box = boundingBox(arc);
        // The exact box contains the samples and exceeds them by no more than
        // the chord error.
        const double slack = 1e-9 * (radius + 1000.0);
        EXPECT_LE(box.min().x, denseBox.min().x + slack);
        EXPECT_GE(box.max().x, denseBox.max().x - slack);
        EXPECT_NEAR(box.min().x, denseBox.min().x, fine + slack);
        EXPECT_NEAR(box.max().y, denseBox.max().y, fine + slack);
        EXPECT_LE(chordError(arc, dense), fine + slack);

        // The closest point is no farther than the nearest dense sample.
        const Vec2 probe{coordinate(), coordinate()};
        double nearestSample = std::numeric_limits<double>::infinity();
        for (const Vec2& point : dense) {
            nearestSample = std::min(nearestSample, distance(point, probe));
        }
        const double exact = distance(closestPoint(arc, probe), probe);
        EXPECT_LE(exact, nearestSample + slack);
        EXPECT_GE(exact, nearestSample - (radius * 2e-3) - slack);
    }
}

TEST(AreCollinear, FollowsTheRelativeCriterionOfAdr0012) {
    // a = (0,0), c = (10,0), b = (5,h): threshold at h = 1e-8.
    EXPECT_TRUE(areCollinear({0.0, 0.0}, {5.0, 9e-9}, {10.0, 0.0}));
    EXPECT_TRUE(areCollinear({0.0, 0.0}, {5.0, 1e-8}, {10.0, 0.0}));
    EXPECT_FALSE(areCollinear({0.0, 0.0}, {5.0, 2e-8}, {10.0, 0.0}));
    // The same shape scaled and moved gives the same answer.
    EXPECT_TRUE(areCollinear({1e6, 1e6}, {1e6 + 5e3, 1e6 + 9e-6}, {1e6 + 1e4, 1e6}));
    EXPECT_FALSE(areCollinear({1e6, 1e6}, {1e6 + 5e3, 1e6 + 2e-5}, {1e6 + 1e4, 1e6}));
    // Coincident points.
    EXPECT_TRUE(areCollinear({7.0, 7.0}, {7.0, 7.0}, {7.0, 7.0}));
    EXPECT_TRUE(areCollinear({0.0, 0.0}, {0.0, 0.0}, {10.0, 0.0}));
    EXPECT_TRUE(areCollinear({5.0, 5.0}, {10.0, 10.0}, {5.0, 5.0}));
    EXPECT_FALSE(areCollinear({10.0, 0.0}, {0.0, 10.0}, {-10.0, 0.0}));
}

}  // namespace
}  // namespace tg::core
