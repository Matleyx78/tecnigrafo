// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/solid.hpp"

#include "tg/core/generated.hpp"

#include <gtest/gtest.h>

namespace tg::core {
namespace {

const Solid kButterfly{{Vec2{0, 0}, Vec2{2, 2}, Vec2{2, 0}, Vec2{0, 2}}};

TEST(Solid, BoundsAndEvenOddFilledRegion) {
    const BBox bounds = boundingBox(kButterfly);
    EXPECT_EQ(bounds.min(), (Vec2{0, 0}));
    EXPECT_EQ(bounds.max(), (Vec2{2, 2}));
    EXPECT_DOUBLE_EQ(distanceOf(kButterfly, {1, 0.25}), 0.0);
    EXPECT_GT(distanceOf(kButterfly, {0.25, 1}), 0.0);
    EXPECT_TRUE(intersectsOf(kButterfly, BBox({0.9, 0.2}, {1.1, 0.3})));
    EXPECT_FALSE(intersectsOf(kButterfly, BBox({0.2, 0.9}, {0.3, 1.1})));
    EXPECT_TRUE(isInsideOf(kButterfly, BBox({-1, -1}, {3, 3})));
}

TEST(Solid, TransformationKeepsVertexOrder) {
    const Document document;
    const auto reflection = Similarity::reflection({0, 0}, {0, 1});
    ASSERT_TRUE(reflection.has_value());
    const auto result = transformedOf(document, kButterfly, reflection.value_or(Similarity{}),
                                      MirrorText::Mirrored);
    ASSERT_TRUE(result.entity.has_value());
    const auto transformed = std::get<Solid>(result.entity.value_or(Entity{}));
    EXPECT_EQ(transformed.vertices[0], (Vec2{0, 0}));
    EXPECT_EQ(transformed.vertices[1], (Vec2{-2, 2}));
    EXPECT_EQ(transformed.vertices[2], (Vec2{-2, 0}));
    EXPECT_EQ(transformed.vertices[3], (Vec2{0, 2}));
}

TEST(Solid, CoincidentVerticesShareGripAndCollapseIsRejected) {
    const Document document;
    const Solid triangle{{Vec2{0, 0}, Vec2{0, 0}, Vec2{2, 0}, Vec2{0, 2}}};
    EXPECT_EQ(gripPointsOf(triangle), (std::vector<Vec2>{{0, 0}, {2, 0}, {0, 2}}));
    const auto moved = moveGripOf(document, triangle, 0, {1, 1});
    EXPECT_EQ(moved.outcome, GripOutcome::Moved);
    const auto& result = std::get<Solid>(moved.entity);
    EXPECT_EQ(result.vertices[0], (Vec2{1, 1}));
    EXPECT_EQ(result.vertices[1], (Vec2{1, 1}));

    Solid almostPoint{{Vec2{0, 0}, Vec2{1, 1}, Vec2{1, 1}, Vec2{1, 1}}};
    const auto invalid = moveGripOf(document, almostPoint, 0, {1, 1});
    EXPECT_EQ(invalid.outcome, GripOutcome::Invalid);
    EXPECT_EQ(invalid.entity, Entity{almostPoint});
}

TEST(Solid, EndpointSnapsAreDistinct) {
    const Solid triangle{{Vec2{0, 0}, Vec2{0, 0}, Vec2{2, 0}, Vec2{0, 2}}};
    std::vector<SnapPoint> points;
    appendSnapPointsOf(triangle, SnapModes{}, points, std::nullopt);
    EXPECT_EQ(points, (std::vector<SnapPoint>{{{0, 0}, SnapKind::Endpoint},
                                              {{2, 0}, SnapKind::Endpoint},
                                              {{0, 2}, SnapKind::Endpoint}}));
}

}  // namespace
}  // namespace tg::core
