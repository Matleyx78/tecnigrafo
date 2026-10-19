// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/line.hpp"

#include "tg/core/document.hpp"
#include "tg/core/proximity.hpp"
#include "tg/core/snap_points.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace tg::core {
namespace {

TEST(Line, LengthAndExactBoundingBox) {
    const Line line{{20.0, 10.0}, {10.0, 30.0}};
    EXPECT_DOUBLE_EQ(line.length(), std::sqrt(500.0));
    EXPECT_EQ(line.boundingBox().min(), (Vec2{10.0, 10.0}));
    EXPECT_EQ(line.boundingBox().max(), (Vec2{20.0, 30.0}));

    const Line point{{50.0, 50.0}, {50.0, 50.0}};
    EXPECT_EQ(point.length(), 0.0);
    EXPECT_FALSE(point.boundingBox().isEmpty());
    EXPECT_EQ(boundingBox(Entity{point}).min(), (Vec2{50.0, 50.0}));
}

TEST(Line, DistanceAndClosestPoint) {
    const Entity line = Line{{0.0, 0.0}, {10.0, 0.0}};
    EXPECT_DOUBLE_EQ(distance(line, {4.0, 3.0}), 3.0);
    EXPECT_DOUBLE_EQ(distance(line, {-4.0, 0.0}), 4.0);  // beyond the start
    EXPECT_DOUBLE_EQ(distance(line, {13.0, 4.0}), 5.0);  // beyond the end
    EXPECT_EQ(distance(line, {4.0, 0.0}), 0.0);
}

TEST(Line, WindowAndCrossing) {
    const Entity line = Line{{0.0, 0.0}, {10.0, 0.0}};
    EXPECT_TRUE(isInside(line, BBox{{-1.0, -1.0}, {11.0, 1.0}}));
    EXPECT_TRUE(intersects(line, BBox{{-1.0, -1.0}, {11.0, 1.0}}));
    EXPECT_FALSE(isInside(line, BBox{{4.0, -1.0}, {6.0, 1.0}}));
    EXPECT_TRUE(intersects(line, BBox{{4.0, -1.0}, {6.0, 1.0}}));  // no end point inside
    EXPECT_FALSE(intersects(line, BBox{{0.0, 2.0}, {10.0, 5.0}}));
    EXPECT_TRUE(intersects(line, BBox{{10.0, 0.0}, {12.0, 2.0}}));  // touches the corner
    const Entity diagonal = Line{{0.0, 0.0}, {10.0, 10.0}};
    EXPECT_TRUE(intersects(diagonal, BBox{{4.0, 4.5}, {5.0, 6.0}}));
    EXPECT_FALSE(intersects(diagonal, BBox{{6.0, 0.0}, {9.0, 3.0}}));
}

TEST(Line, SnapsToEndPointsAndMidpoint) {
    std::vector<SnapPoint> points;
    appendSnapPoints(Entity{Line{{0.0, 0.0}, {3.0, 4.0}}}, SnapModes{}, points);
    EXPECT_EQ(points, (std::vector<SnapPoint>{{{0.0, 0.0}, SnapKind::Endpoint},
                                              {{3.0, 4.0}, SnapKind::Endpoint},
                                              {{1.5, 2.0}, SnapKind::Midpoint}}));
    points.clear();
    appendSnapPoints(Entity{Line{{0.0, 0.0}, {3.0, 4.0}}}, SnapModes{false, true, true}, points);
    EXPECT_EQ(points, (std::vector<SnapPoint>{{{1.5, 2.0}, SnapKind::Midpoint}}));
}

}  // namespace
}  // namespace tg::core
