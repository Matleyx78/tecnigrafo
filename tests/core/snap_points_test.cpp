// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/snap_points.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace tg::core {
namespace {

TEST(SnapPoints, DefaultsAndEquality) {
    EXPECT_EQ(SnapModes{}, (SnapModes{true, true, true}));
    EXPECT_NE(SnapModes{}, (SnapModes{false, true, true}));
    EXPECT_EQ(SnapPoint{}.kind, SnapKind::Endpoint);
    EXPECT_EQ((SnapPoint{{1, 2}}), (SnapPoint{{1, 2}, SnapKind::Endpoint}));
    EXPECT_NE((SnapPoint{{1, 2}}), (SnapPoint{{1, 2}, SnapKind::Midpoint}));
}

TEST(SnapPoints, StraightSegmentAppendsInOrder) {
    std::vector<SnapPoint> out{{{9, 9}, SnapKind::Center}};
    appendSnapPoints(PolylineSegment{{0, 0}, {4, 6}}, {}, out);
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{9, 9}, SnapKind::Center},
                                           {{0, 0}, SnapKind::Endpoint},
                                           {{4, 6}, SnapKind::Endpoint},
                                           {{2, 3}, SnapKind::Midpoint}}));
}

class ArcSnapTest : public testing::TestWithParam<double> {};

TEST_P(ArcSnapTest, MidpointFollowsSignedSweepAndCenterMatchesCircle) {
    const double bulge = GetParam();
    const PolylineSegment segment{{-2, 0}, {2, 0}, bulge};
    std::vector<SnapPoint> out;
    appendSnapPoints(segment, {}, out);
    ASSERT_EQ(out.size(), 4U);
    EXPECT_EQ(out[0], (SnapPoint{{-2, 0}, SnapKind::Endpoint}));
    EXPECT_EQ(out[1], (SnapPoint{{2, 0}, SnapKind::Endpoint}));
    EXPECT_EQ(out[2], (SnapPoint{{0, -2 * bulge}, SnapKind::Midpoint}));
    EXPECT_EQ(out[3].kind, SnapKind::Center);
    EXPECT_NEAR(out[3].point.x, 0, 1e-12);
    EXPECT_NEAR(out[3].point.y, (1 - bulge * bulge) / bulge, 1e-12);
    const auto arc = arcFromBulge(segment.start, segment.end, bulge);
    const double halfway = arc.startAngle + arc.sweep * 0.5;
    EXPECT_NEAR(out[2].point.x, arc.center.x + arc.radius * std::cos(halfway), 1e-12);
    EXPECT_NEAR(out[2].point.y, arc.center.y + arc.radius * std::sin(halfway), 1e-12);
}

INSTANTIATE_TEST_SUITE_P(MinorSemicircleMajor, ArcSnapTest,
                         testing::Values(0.5, -0.5, 1.0, -1.0, 2.0, -2.0));

class SnapModesTest : public testing::TestWithParam<SnapModes> {};

TEST_P(SnapModesTest, SegmentAndEntityHonorEveryCombination) {
    const auto modes = GetParam();
    const PolylineSegment segment{{-1, 0}, {1, 0}, 1};
    const Entity entity = Polyline{{{{-1, 0}, 1}, {{1, 0}, 0}}};
    std::vector<SnapPoint> expected;
    if (modes.endpoint) {
        expected.push_back({{-1, 0}, SnapKind::Endpoint});
        expected.push_back({{1, 0}, SnapKind::Endpoint});
    }
    if (modes.midpoint) {
        expected.push_back({{0, -1}, SnapKind::Midpoint});
    }
    if (modes.center) {
        expected.push_back({{0, 0}, SnapKind::Center});
    }
    std::vector<SnapPoint> segmentOut;
    std::vector<SnapPoint> entityOut;
    appendSnapPoints(segment, modes, segmentOut);
    appendSnapPoints(entity, modes, entityOut);
    EXPECT_EQ(segmentOut, expected);
    EXPECT_EQ(entityOut, expected);
}

INSTANTIATE_TEST_SUITE_P(
    AllCombinations, SnapModesTest,
    testing::Values(SnapModes{false, false, false}, SnapModes{false, false, true},
                    SnapModes{false, true, false}, SnapModes{false, true, true},
                    SnapModes{true, false, false}, SnapModes{true, false, true},
                    SnapModes{true, true, false}, SnapModes{true, true, true}));

TEST(SnapPoints, NullSegmentHasOnlyOneEndpointEvenWithBulge) {
    for (const double bulge : std::array{0.0, 1.0, -2.0}) {
        std::vector<SnapPoint> out;
        appendSnapPoints(PolylineSegment{{3, 4}, {3, 4}, bulge}, {}, out);
        EXPECT_EQ(out, (std::vector<SnapPoint>{{{3, 4}, SnapKind::Endpoint}}));
        out.clear();
        appendSnapPoints(PolylineSegment{{3, 4}, {3, 4}, bulge}, {false, true, true}, out);
        EXPECT_TRUE(out.empty());
    }
}

TEST(SnapPoints, OpenPolylineDoesNotRepeatSharedEndpoints) {
    const Entity entity = Polyline{{{{0, 0}}, {{4, 0}}, {{4, 4}}}};
    std::vector<SnapPoint> out;
    appendSnapPoints(entity, {}, out);
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{0, 0}, SnapKind::Endpoint},
                                           {{4, 0}, SnapKind::Endpoint},
                                           {{2, 0}, SnapKind::Midpoint},
                                           {{4, 4}, SnapKind::Endpoint},
                                           {{4, 2}, SnapKind::Midpoint}}));
}

TEST(SnapPoints, ClosedPolylineIncludesClosingArcAndUniqueCenter) {
    const Entity entity = Polyline{{{{-1, 0}, 1}, {{1, 0}, 1}}, true};
    std::vector<SnapPoint> out;
    appendSnapPoints(entity, {}, out);
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{-1, 0}, SnapKind::Endpoint},
                                           {{1, 0}, SnapKind::Endpoint},
                                           {{0, -1}, SnapKind::Midpoint},
                                           {{0, 0}, SnapKind::Center},
                                           {{0, 1}, SnapKind::Midpoint}}));
}

TEST(SnapPoints, EmptyAndSingleVertexOpenOrClosed) {
    for (const bool closed : {false, true}) {
        std::vector<SnapPoint> out;
        appendSnapPoints(Entity{Polyline{{}, closed}}, {}, out);
        EXPECT_TRUE(out.empty());
        const Entity single = Polyline{{{{2, 3}, 1}}, closed};
        appendSnapPoints(single, {false, true, true}, out);
        EXPECT_TRUE(out.empty());
        appendSnapPoints(single, {}, out);
        EXPECT_EQ(out, (std::vector<SnapPoint>{{{2, 3}, SnapKind::Endpoint}}));
    }
}

TEST(SnapPoints, RepeatedVerticesAndRetracedSegmentsAreUniqueByKind) {
    const Entity entity = Polyline{{{{0, 0}}, {{0, 0}, 1}, {{2, 0}}, {{0, 0}}, {{1, 0}}}};
    std::vector<SnapPoint> out;
    appendSnapPoints(entity, {}, out);
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{0, 0}, SnapKind::Endpoint},
                                           {{2, 0}, SnapKind::Endpoint},
                                           {{1, -1}, SnapKind::Midpoint},
                                           {{1, 0}, SnapKind::Center},
                                           {{1, 0}, SnapKind::Midpoint},
                                           {{1, 0}, SnapKind::Endpoint},
                                           {{0.5, 0}, SnapKind::Midpoint}}));
}

TEST(SnapPoints, ExistingOutputIsPreservedAcrossEntities) {
    const Entity entity = Polyline{{{{0, 0}}, {{2, 0}}}};
    std::vector<SnapPoint> out;
    appendSnapPoints(entity, {}, out);
    const auto first = out;
    appendSnapPoints(entity, {}, out);
    auto expected = first;
    expected.insert(expected.end(), first.begin(), first.end());
    EXPECT_EQ(out, expected);
}

TEST(SnapPoints, TinySegmentIsNotCollapsedAndTinyBulgeIsStraight) {
    std::vector<SnapPoint> out;
    appendSnapPoints(PolylineSegment{{0, 0}, {1e-15, 0}, 1e-13}, {}, out);
    ASSERT_EQ(out.size(), 3U);
    EXPECT_EQ(out[2], (SnapPoint{{5e-16, 0}, SnapKind::Midpoint}));
}

TEST(SnapPoints, RotatedTranslatedArcMidpoint) {
    std::vector<SnapPoint> out;
    appendSnapPoints(PolylineSegment{{3, 2}, {3, 6}, -2}, {}, out);
    ASSERT_EQ(out.size(), 4U);
    EXPECT_EQ(out[2], (SnapPoint{{-1, 4}, SnapKind::Midpoint}));
    EXPECT_EQ(out[3], (SnapPoint{{1.5, 4}, SnapKind::Center}));
}

TEST(SnapPoints, EveryEntityTypeHonorsTheModes) {
    const std::vector<Entity> entities{Line{{0, 0}, {4, 0}}, Circle{{0, 0}, 2},
                                       Arc{{0, 0}, 2, 0, kPi / 2.0}};
    for (const Entity& entity : entities) {
        std::vector<SnapPoint> out;
        appendSnapPoints(entity, SnapModes{false, false, false}, out);
        EXPECT_TRUE(out.empty());
        appendSnapPoints(entity, SnapModes{}, out);
        for (const SnapPoint& snap : out) {
            std::vector<SnapPoint> alone;
            const SnapModes only{snap.kind == SnapKind::Endpoint, snap.kind == SnapKind::Midpoint,
                                 snap.kind == SnapKind::Center};
            appendSnapPoints(entity, only, alone);
            EXPECT_NE(std::ranges::find(alone, snap), alone.end());
        }
    }
}

TEST(SnapPoints, DegenerateLineOffersOnlyItsEndpoint) {
    std::vector<SnapPoint> out;
    appendSnapPoints(Entity{Line{{5, 5}, {5, 5}}}, {}, out);
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{5, 5}, SnapKind::Endpoint}}));
}

}  // namespace
}  // namespace tg::core
