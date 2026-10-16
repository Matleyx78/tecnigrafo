// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/bbox.hpp"
#include "tg/core/math.hpp"
#include "tg/core/transform2d.hpp"
#include "tg/core/vec2.hpp"

#include <gtest/gtest.h>

namespace tg::core {
namespace {

constexpr double kEps = 1e-12;

void expectNear(const Vec2& actual, const Vec2& expected, double tolerance = kEps) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
}

TEST(Vec2, Arithmetic) {
    const Vec2 a{1.0, 2.0};
    const Vec2 b{3.0, -4.0};
    EXPECT_EQ(a + b, Vec2(4.0, -2.0));
    EXPECT_EQ(a - b, Vec2(-2.0, 6.0));
    EXPECT_EQ(a * 2.0, Vec2(2.0, 4.0));
    EXPECT_EQ(-a, Vec2(-1.0, -2.0));
    EXPECT_DOUBLE_EQ(b.length(), 5.0);
    EXPECT_DOUBLE_EQ(dot(a, b), -5.0);
    EXPECT_DOUBLE_EQ(cross(Vec2{1.0, 0.0}, Vec2{0.0, 1.0}), 1.0);
    EXPECT_EQ(Vec2(1.0, 0.0).perpendicular(), Vec2(0.0, 1.0));
}

TEST(BBox, StartsEmptyAndGrows) {
    BBox box;
    EXPECT_TRUE(box.isEmpty());
    EXPECT_DOUBLE_EQ(box.width(), 0.0);

    box.expand(Vec2{1.0, 2.0});
    EXPECT_FALSE(box.isEmpty());
    EXPECT_DOUBLE_EQ(box.width(), 0.0);

    box.expand(Vec2{-3.0, 5.0});
    EXPECT_EQ(box.min(), Vec2(-3.0, 2.0));
    EXPECT_EQ(box.max(), Vec2(1.0, 5.0));
    EXPECT_EQ(box.center(), Vec2(-1.0, 3.5));
    EXPECT_TRUE(box.contains(Vec2{0.0, 3.0}));
    EXPECT_FALSE(box.contains(Vec2{2.0, 3.0}));
}

TEST(BBox, ExpandingWithEmptyBoxChangesNothing) {
    BBox box(Vec2{0.0, 0.0}, Vec2{1.0, 1.0});
    box.expand(BBox{});
    EXPECT_EQ(box.min(), Vec2(0.0, 0.0));
    EXPECT_EQ(box.max(), Vec2(1.0, 1.0));
}

TEST(Transform2D, CompositionAppliesRightOperandFirst) {
    const Transform2D move = Transform2D::translation({10.0, 0.0});
    const Transform2D scale = Transform2D::scaling(2.0, 2.0);
    expectNear((move * scale).apply({1.0, 1.0}), {12.0, 2.0});
    expectNear((scale * move).apply({1.0, 1.0}), {22.0, 2.0});
}

TEST(Transform2D, RotationIsCounterClockwise) {
    expectNear(Transform2D::rotation(kPi / 2.0).apply({1.0, 0.0}), {0.0, 1.0});
}

TEST(Transform2D, InverseUndoesTransform) {
    const Transform2D t = Transform2D::translation({3.0, -7.0}) *
                          Transform2D::rotation(degreesToRadians(30.0)) *
                          Transform2D::scaling(2.5, -1.5);
    const auto inverse = t.inverted();
    ASSERT_TRUE(inverse.has_value());
    if (inverse) {  // repeated so static analysis sees the check
        const Vec2 p{4.25, -9.5};
        expectNear(inverse->apply(t.apply(p)), p, 1e-9);
    }
}

TEST(Transform2D, SingularTransformHasNoInverse) {
    EXPECT_FALSE(Transform2D::scaling(0.0, 1.0).inverted().has_value());
}

}  // namespace
}  // namespace tg::core
