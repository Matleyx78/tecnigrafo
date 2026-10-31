// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/drawing_aids.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <optional>

namespace tg::cmd {
namespace {

using core::Vec2;

TEST(DrawingAids, DefaultsAreThoseOfANewSession) {
    const DrawingAids aids;
    EXPECT_TRUE(aids.objectSnap);
    EXPECT_EQ(aids.snapModes, (core::SnapModes{true, true, true}));
    EXPECT_FALSE(aids.ortho);
    EXPECT_FALSE(aids.polar);
}

TEST(DrawingAids, ComparesEveryField) {
    const DrawingAids aids;
    DrawingAids other;
    EXPECT_EQ(aids, other);
    other.objectSnap = false;
    EXPECT_NE(aids, other);
    other = {};
    other.snapModes.center = false;
    EXPECT_NE(aids, other);
    other = {};
    other.ortho = true;
    EXPECT_NE(aids, other);
    other = {};
    other.polar = true;
    EXPECT_NE(aids, other);
}

TEST(OrthoPoint, NearerToTheHorizontalInEveryQuadrant) {
    const Vec2 last{10, 20};
    EXPECT_EQ(orthoPoint(last, {15, 22}), (Vec2{15, 20}));  // first quadrant
    EXPECT_EQ(orthoPoint(last, {5, 22}), (Vec2{5, 20}));    // second
    EXPECT_EQ(orthoPoint(last, {5, 18}), (Vec2{5, 20}));    // third
    EXPECT_EQ(orthoPoint(last, {15, 18}), (Vec2{15, 20}));  // fourth
}

TEST(OrthoPoint, NearerToTheVerticalInEveryQuadrant) {
    const Vec2 last{10, 20};
    EXPECT_EQ(orthoPoint(last, {12, 25}), (Vec2{10, 25}));  // first quadrant
    EXPECT_EQ(orthoPoint(last, {8, 25}), (Vec2{10, 25}));   // second
    EXPECT_EQ(orthoPoint(last, {8, 15}), (Vec2{10, 15}));   // third
    EXPECT_EQ(orthoPoint(last, {12, 15}), (Vec2{10, 15}));  // fourth
}

TEST(OrthoPoint, OnTheAxesThePointerStaysWhereItIs) {
    const Vec2 last{10, 20};
    EXPECT_EQ(orthoPoint(last, {30, 20}), (Vec2{30, 20}));
    EXPECT_EQ(orthoPoint(last, {-30, 20}), (Vec2{-30, 20}));
    EXPECT_EQ(orthoPoint(last, {10, 50}), (Vec2{10, 50}));
    EXPECT_EQ(orthoPoint(last, {10, -50}), (Vec2{10, -50}));
}

TEST(OrthoPoint, At45DegreesTheHorizontalWins) {
    const Vec2 last{10, 20};
    EXPECT_EQ(orthoPoint(last, {15, 25}), (Vec2{15, 20}));
    EXPECT_EQ(orthoPoint(last, {5, 25}), (Vec2{5, 20}));
    EXPECT_EQ(orthoPoint(last, {5, 15}), (Vec2{5, 20}));
    EXPECT_EQ(orthoPoint(last, {15, 15}), (Vec2{15, 20}));
}

TEST(OrthoPoint, At45DegreesRoundingDoesNotTipTheBalance) {
    // In binary floating point 0.3 - 0.1 is a little less than 0.4 - 0.2:
    // compared as they are, the vertical would win, yet the pointer is on
    // the diagonal.
    ASSERT_LT(0.3 - 0.1, 0.4 - 0.2);
    EXPECT_EQ(orthoPoint({0.1, 0.2}, {0.3, 0.4}), (Vec2{0.3, 0.2}));
    EXPECT_EQ(orthoPoint({0.1, 0.2}, {1.2, 1.3}), (Vec2{1.2, 0.2}));
    EXPECT_EQ(orthoPoint({0.1, 0.7}, {0.2, 0.8}), (Vec2{0.2, 0.7}));
}

TEST(OrthoPoint, JustOff45DegreesTheNearerAxisWins) {
    const Vec2 last{0, 0};
    EXPECT_EQ(orthoPoint(last, {100, 100.001}), (Vec2{0, 100.001}));
    EXPECT_EQ(orthoPoint(last, {100.001, 100}), (Vec2{100.001, 0}));
}

TEST(OrthoPoint, APointerOnTheLastPointStaysThere) {
    EXPECT_EQ(orthoPoint({3, 4}, {3, 4}), (Vec2{3, 4}));
}

// --- Polar tracking (docs/adr/0033, points 8 and 9) -------------------------

// The aperture of the tests: 10 pixels of one drawing unit each.
constexpr double kAperture = 10.0;

double radians(double degrees) {
    return core::degreesToRadians(degrees);
}

/// The point at `distance` from `base` along the ray at `degrees`, moved by
/// `offset` square to the ray, counter-clockwise.
Vec2 fromRay(const Vec2& base, double degrees, double distance, double offset) {
    const Vec2 unit{std::cos(radians(degrees)), std::sin(radians(degrees))};
    return base + (unit * distance) + (unit.perpendicular() * offset);
}

void expectNear(const std::optional<Vec2>& actual, const Vec2& expected) {
    if (!actual) {
        ADD_FAILURE() << "no polar point, expected " << expected.x << "," << expected.y;
        return;
    }
    constexpr double kTolerance = 1e-9;
    EXPECT_NEAR(actual->x, expected.x, kTolerance);
    EXPECT_NEAR(actual->y, expected.y, kTolerance);
}

TEST(PolarPoint, At90DegreesAPointerOnARayStaysThere) {
    const Vec2 base{10, 20};
    const double increment = radians(90);
    EXPECT_EQ(polarPoint(base, {110, 20}, increment, kAperture), (Vec2{110, 20}));
    EXPECT_EQ(polarPoint(base, {10, 120}, increment, kAperture), (Vec2{10, 120}));
    EXPECT_EQ(polarPoint(base, {-90, 20}, increment, kAperture), (Vec2{-90, 20}));
    EXPECT_EQ(polarPoint(base, {10, -80}, increment, kAperture), (Vec2{10, -80}));
}

TEST(PolarPoint, At90DegreesNinePixelsOffTheRayItEngagesElevenItDoesNot) {
    const Vec2 base{10, 20};
    const double increment = radians(90);
    // Exact on the axes: the coordinate along which the ray does not move is
    // the base point's, as ortho gives it.
    EXPECT_EQ(polarPoint(base, {110, 29}, increment, kAperture), (Vec2{110, 20}));
    EXPECT_EQ(polarPoint(base, {1, 120}, increment, kAperture), (Vec2{10, 120}));
    EXPECT_EQ(polarPoint(base, {-90, 11}, increment, kAperture), (Vec2{-90, 20}));
    EXPECT_EQ(polarPoint(base, {110, 31}, increment, kAperture), std::nullopt);
    EXPECT_EQ(polarPoint(base, {-1, 120}, increment, kAperture), std::nullopt);
}

TEST(PolarPoint, TheApertureItselfStillEngages) {
    EXPECT_EQ(polarPoint({0, 0}, {50, 10}, radians(90), kAperture), (Vec2{50, 0}));
}

TEST(PolarPoint, At45DegreesTheDiagonalsAreRays) {
    const Vec2 base{-5, 7};
    const double increment = radians(45);
    for (const double degrees : {45.0, 135.0, -45.0, -135.0, 180.0}) {
        expectNear(polarPoint(base, fromRay(base, degrees, 100, 0), increment, kAperture),
                   fromRay(base, degrees, 100, 0));
        expectNear(polarPoint(base, fromRay(base, degrees, 100, 9), increment, kAperture),
                   fromRay(base, degrees, 100, 0));
        expectNear(polarPoint(base, fromRay(base, degrees, 100, -9), increment, kAperture),
                   fromRay(base, degrees, 100, 0));
        EXPECT_EQ(polarPoint(base, fromRay(base, degrees, 100, 11), increment, kAperture),
                  std::nullopt)
            << degrees;
    }
}

TEST(PolarPoint, At7DegreesTheRaysAreTheMultiplesOf7) {
    const Vec2 base{0, 0};
    const double increment = radians(7);
    for (const double degrees : {0.0, 7.0, 14.0, 91.0, 175.0, -7.0, -175.0}) {
        expectNear(polarPoint(base, fromRay(base, degrees, 200, 9), increment, kAperture),
                   fromRay(base, degrees, 200, 0));
        EXPECT_EQ(polarPoint(base, fromRay(base, degrees, 200, 11), increment, kAperture),
                  std::nullopt)
            << degrees;
    }
}

TEST(PolarPoint, At7DegreesTheRaysAcrossTheNegativeXAxisAreFound) {
    // 179 degrees: 4 degrees from the ray at 175, 6 from the one at -175.
    const Vec2 pointer = fromRay({0, 0}, 179, 100, 0);
    expectNear(polarPoint({0, 0}, pointer, radians(7), kAperture),
               fromRay({0, 0}, 175, 100 * std::cos(radians(4)), 0));
    // -179 degrees: the other way round.
    const Vec2 below = fromRay({0, 0}, -179, 100, 0);
    expectNear(polarPoint({0, 0}, below, radians(7), kAperture),
               fromRay({0, 0}, -175, 100 * std::cos(radians(4)), 0));
}

TEST(PolarPoint, NearTheBasePointTheNearestRayWins) {
    const Vec2 base{10, 20};
    // 4 from the horizontal ray, 3 from the vertical one.
    EXPECT_EQ(polarPoint(base, {13, 24}, radians(90), kAperture), (Vec2{10, 24}));
    EXPECT_EQ(polarPoint(base, {14, 23}, radians(90), kAperture), (Vec2{14, 20}));
    EXPECT_EQ(polarPoint(base, base, radians(90), kAperture), base);
}

TEST(PolarPoint, TheRayBehindThePointerNeverWins) {
    // Just right of the base, below the X axis: the ray at 270 degrees is 1
    // away, the ray at 90 degrees (behind) only through the base point.
    EXPECT_EQ(polarPoint({0, 0}, {1, -30}, radians(90), kAperture), (Vec2{0, -30}));
    // With 180 degrees only the X axis carries rays, on both sides.
    EXPECT_EQ(polarPoint({0, 0}, {-40, 5}, radians(180), kAperture), (Vec2{-40, 0}));
    EXPECT_EQ(polarPoint({0, 0}, {40, -5}, radians(180), kAperture), (Vec2{40, 0}));
    EXPECT_EQ(polarPoint({0, 0}, {5, 40}, radians(180), kAperture), std::nullopt);
}

TEST(PolarPoint, HalfwayBetweenTwoRaysTheClockwiseOneWins) {
    const Vec2 base{0, 0};
    EXPECT_EQ(polarPoint(base, {5, 5}, radians(90), kAperture), (Vec2{5, 0}));
    EXPECT_EQ(polarPoint(base, {-5, 5}, radians(90), kAperture), (Vec2{0, 5}));
    // Halfway and far: neither ray is within the aperture.
    EXPECT_EQ(polarPoint(base, {100, 100}, radians(90), kAperture), std::nullopt);
    // At 45 degrees, halfway between the rays at 0 and 45, far from both.
    EXPECT_EQ(polarPoint(base, fromRay(base, 22.5, 100, 0), radians(45), kAperture), std::nullopt);
}

TEST(PolarPoint, AZeroToleranceKeepsOnlyPointersOnARay) {
    EXPECT_EQ(polarPoint({0, 0}, {30, 0}, radians(90), 0.0), (Vec2{30, 0}));
    EXPECT_EQ(polarPoint({0, 0}, {30, 0.5}, radians(90), 0.0), std::nullopt);
}

}  // namespace
}  // namespace tg::cmd
