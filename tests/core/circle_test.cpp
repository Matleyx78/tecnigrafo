// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/circle.hpp"

#include "tg/core/document.hpp"
#include "tg/core/math.hpp"
#include "tg/core/proximity.hpp"
#include "tg/core/snap_points.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <tuple>
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

TEST(Circle, LengthIsTheCircumference) {
    EXPECT_DOUBLE_EQ((Circle{{0.0, 0.0}, 10.0}).length(), 20.0 * kPi);
    EXPECT_DOUBLE_EQ((Circle{{25.0, 35.0}, 5.0}).length(), 10.0 * kPi);
}

TEST(Circle, ExactBoundingBoxAtAnyScale) {
    const BBox tiny = Circle{{0.0, 0.0}, 1e-6}.boundingBox();
    EXPECT_EQ(tiny.min(), (Vec2{-1e-6, -1e-6}));
    EXPECT_EQ(tiny.max(), (Vec2{1e-6, 1e-6}));
    const BBox huge = Circle{{100.0, 100.0}, 1e6}.boundingBox();
    EXPECT_EQ(huge.min(), (Vec2{-999900.0, -999900.0}));
    EXPECT_EQ(huge.max(), (Vec2{1000100.0, 1000100.0}));
}

TEST(Circle, IsACurveNotARegion) {
    const Entity circle = Circle{{0.0, 0.0}, 10.0};
    EXPECT_DOUBLE_EQ(distance(circle, {0.0, 0.0}), 10.0);  // the centre
    EXPECT_DOUBLE_EQ(distance(circle, {3.0, 4.0}), 5.0);
    EXPECT_DOUBLE_EQ(distance(circle, {12.0, 16.0}), 10.0);
    EXPECT_EQ(distance(circle, {6.0, 8.0}), 0.0);

    // A rectangle inside the circle does not cross it.
    EXPECT_FALSE(isInside(circle, BBox{{-2.0, -2.0}, {2.0, 2.0}}));
    EXPECT_FALSE(intersects(circle, BBox{{-2.0, -2.0}, {2.0, 2.0}}));
    // A rectangle around the circle takes it both ways.
    EXPECT_TRUE(isInside(circle, BBox{{-15.0, -15.0}, {15.0, 15.0}}));
    EXPECT_TRUE(intersects(circle, BBox{{-15.0, -15.0}, {15.0, 15.0}}));
    EXPECT_FALSE(isInside(circle, BBox{{8.0, -2.0}, {12.0, 2.0}}));
    EXPECT_TRUE(intersects(circle, BBox{{8.0, -2.0}, {12.0, 2.0}}));
    EXPECT_FALSE(intersects(circle, BBox{{15.0, 15.0}, {25.0, 25.0}}));
}

TEST(Circle, SnapsOnlyToItsCentre) {
    std::vector<SnapPoint> points;
    appendSnapPoints(Entity{Circle{{25.0, 35.0}, 5.0}}, SnapModes{}, points);
    EXPECT_EQ(points, (std::vector<SnapPoint>{{{25.0, 35.0}, SnapKind::Center}}));
    points.clear();
    appendSnapPoints(Entity{Circle{{25.0, 35.0}, 5.0}}, SnapModes{true, true, false}, points);
    EXPECT_TRUE(points.empty());
}

TEST(CircleThroughPoints, InEitherDirection) {
    for (const auto& [a, b, c] :
         {std::tuple{Vec2{10.0, 0.0}, Vec2{0.0, 10.0}, Vec2{-10.0, 0.0}},
          std::tuple{Vec2{10.0, 0.0}, Vec2{0.0, -10.0}, Vec2{-10.0, 0.0}}}) {
        const Circle circle = valueOf(circleThroughPoints(a, b, c));
        EXPECT_NEAR(circle.center.x, 0.0, kTolerance);
        EXPECT_NEAR(circle.center.y, 0.0, kTolerance);
        EXPECT_NEAR(circle.radius, 10.0, kTolerance);
    }
    const Circle pythagorean =
        valueOf(circleThroughPoints({18.0, 10.0}, {10.0, 22.0}, {-7.0, 15.0}));
    EXPECT_NEAR(pythagorean.center.x, 5.0, kTolerance);
    EXPECT_NEAR(pythagorean.center.y, 10.0, kTolerance);
    EXPECT_NEAR(pythagorean.radius, 13.0, kTolerance);
}

TEST(CircleThroughPoints, CollinearOrCoincidentPointsHaveNone) {
    EXPECT_FALSE(circleThroughPoints({0.0, 0.0}, {5.0, 5.0}, {10.0, 10.0}));
    EXPECT_FALSE(circleThroughPoints({0.0, 0.0}, {0.0, 0.0}, {10.0, 0.0}));
    EXPECT_FALSE(circleThroughPoints({7.0, 7.0}, {7.0, 7.0}, {7.0, 7.0}));
    // Just below and at the threshold of docs/adr/0012.
    EXPECT_FALSE(circleThroughPoints({0.0, 0.0}, {5.0, 9e-9}, {10.0, 0.0}));
    EXPECT_FALSE(circleThroughPoints({0.0, 0.0}, {5.0, 1e-8}, {10.0, 0.0}));
    // Just above: a circle with a huge radius, still exact.
    const Circle flat = valueOf(circleThroughPoints({0.0, 0.0}, {5.0, 2e-8}, {10.0, 0.0}));
    EXPECT_EQ(flat.center, (Vec2{5.0, -625000000.0}));
    EXPECT_EQ(flat.radius, 625000000.0);
}

}  // namespace
}  // namespace tg::core
