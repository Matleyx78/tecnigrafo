// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/drawing_aids.hpp"

#include <gtest/gtest.h>

namespace tg::cmd {
namespace {

using core::Vec2;

TEST(DrawingAids, DefaultsAreThoseOfANewSession) {
    const DrawingAids aids;
    EXPECT_TRUE(aids.objectSnap);
    EXPECT_EQ(aids.snapModes, (core::SnapModes{true, true, true}));
    EXPECT_FALSE(aids.ortho);
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

}  // namespace
}  // namespace tg::cmd
