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
    EXPECT_EQ(SnapModes{}, (SnapModes{true, true, true, true}));
    EXPECT_NE(SnapModes{}, (SnapModes{true, true, true, false}));
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
                                       Arc{{0, 0}, 2, 0, kPi / 2.0}, Text{}};
    for (const Entity& entity : entities) {
        std::vector<SnapPoint> out;
        appendSnapPoints(entity, SnapModes{false, false, false, false}, out);
        EXPECT_TRUE(out.empty());
        appendSnapPoints(entity, SnapModes{}, out);
        for (const SnapPoint& snap : out) {
            std::vector<SnapPoint> alone;
            const SnapModes only{snap.kind == SnapKind::Endpoint, snap.kind == SnapKind::Midpoint,
                                 snap.kind == SnapKind::Center, snap.kind == SnapKind::Insertion};
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

TEST(SnapPoints, TextOffersItsInsertionPoint) {
    Text text;
    text.position = {3.0, -4.0};
    text.rotation = 1.0;
    text.value = "Hello";
    std::vector<SnapPoint> out;
    appendSnapPoints(Entity{text}, {}, out);
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{3.0, -4.0}, SnapKind::Insertion}}));
    // An empty text too: the point is its position, not its box.
    text.value.clear();
    out.clear();
    appendSnapPoints(Entity{text}, {}, out);
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{3.0, -4.0}, SnapKind::Insertion}}));
}

TEST(SnapPoints, TextInsertionFollowsAlignmentEvenForDegenerateFits) {
    for (const auto horizontal : {TextHorizontal::Left, TextHorizontal::Center,
                                  TextHorizontal::Aligned, TextHorizontal::Fit}) {
        for (const bool empty : {false, true}) {
            Text text;
            text.position = {3, -4};
            text.alignmentPoint = {10, 20};
            text.horizontal = horizontal;
            text.vertical = horizontal == TextHorizontal::Center ? TextVertical::Middle
                                                                 : TextVertical::Baseline;
            text.value = empty ? "" : "Hello";
            const Vec2 expected = horizontal == TextHorizontal::Center ? Vec2{10, 20} : Vec2{3, -4};
            std::vector<SnapPoint> out;
            appendSnapPoints(Entity{text}, {}, out);
            EXPECT_EQ(out, (std::vector<SnapPoint>{{expected, SnapKind::Insertion}}));
        }
    }
}

TEST(SnapPoints, InsertionModeOnlyAffectsTexts) {
    Text text;
    text.value = "A";
    std::vector<SnapPoint> out;
    appendSnapPoints(Entity{text}, SnapModes{true, true, true, false}, out);
    EXPECT_TRUE(out.empty());
    appendSnapPoints(Entity{text}, SnapModes{false, false, false, true}, out);
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{0.0, 0.0}, SnapKind::Insertion}}));
    // The other entities have no insertion point.
    out.clear();
    for (const Entity& entity : std::vector<Entity>{Line{{0, 0}, {4, 0}}, Circle{{0, 0}, 2},
                                                    Arc{{0, 0}, 2, 0, kPi / 2.0}}) {
        appendSnapPoints(entity, SnapModes{false, false, false, true}, out);
    }
    EXPECT_TRUE(out.empty());
}

// ---------------------------------------------------------------------------
// Quadrants and perpendiculars (docs/adr/0033)

/// Only the given mode on, all the others off.
SnapModes only(bool SnapModes::*mode) {
    SnapModes modes{false, false, false, false, false, false, false, false};
    modes.*mode = true;
    return modes;
}

void expectPoints(const std::vector<SnapPoint>& actual, const std::vector<SnapPoint>& expected) {
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i].kind, expected[i].kind) << i;
        EXPECT_NEAR(actual[i].point.x, expected[i].point.x, 1e-12) << i;
        EXPECT_NEAR(actual[i].point.y, expected[i].point.y, 1e-12) << i;
    }
}

TEST(SnapPoints, NewModesDefaults) {
    const SnapModes modes;
    EXPECT_TRUE(modes.intersection);
    EXPECT_FALSE(modes.perpendicular);
    EXPECT_FALSE(modes.tangent);
    EXPECT_FALSE(modes.quadrant);
    // The default modes give no quadrant and no perpendicular.
    std::vector<SnapPoint> out;
    appendSnapPoints(Entity{Circle{{1, 2}, 3}}, modes, out, Vec2{9, 9});
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{1, 2}, SnapKind::Center}}));
}

TEST(SnapPoints, CircleHasTheFourQuadrants) {
    std::vector<SnapPoint> out;
    appendSnapPoints(Entity{Circle{{1, 2}, 3}}, only(&SnapModes::quadrant), out);
    // The exact sums of centre and radius, from 0 degrees counter-clockwise.
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{4, 2}, SnapKind::Quadrant},
                                           {{1, 5}, SnapKind::Quadrant},
                                           {{-2, 2}, SnapKind::Quadrant},
                                           {{1, -1}, SnapKind::Quadrant}}));
}

TEST(SnapPoints, ArcHasOnlyTheQuadrantsInsideIt) {
    // From 45 to 225 degrees: 90 and 180 are inside, 0 and 270 are not.
    std::vector<SnapPoint> out;
    appendSnapPoints(Entity{Arc{{0, 0}, 2, kPi / 4.0, 5.0 * kPi / 4.0}}, only(&SnapModes::quadrant),
                     out);
    EXPECT_EQ(
        out, (std::vector<SnapPoint>{{{0, 2}, SnapKind::Quadrant}, {{-2, 0}, SnapKind::Quadrant}}));
    // A short arc inside one quadrant has none.
    out.clear();
    appendSnapPoints(Entity{Arc{{0, 0}, 2, kPi / 8.0, 3.0 * kPi / 8.0}}, only(&SnapModes::quadrant),
                     out);
    EXPECT_TRUE(out.empty());
}

TEST(SnapPoints, QuadrantOnAnEndPointOfTheArcIsThatEndPoint) {
    // From 0 to 90 degrees: both end points are quadrants, and exactly the
    // same points as the end points.
    std::vector<SnapPoint> out;
    appendSnapPoints(Entity{Arc{{1, 1}, 2, 0, kPi / 2.0}}, only(&SnapModes::quadrant), out);
    EXPECT_EQ(out,
              (std::vector<SnapPoint>{{{3, 1}, SnapKind::Quadrant}, {{1, 3}, SnapKind::Quadrant}}));
}

TEST(SnapPoints, ArcSegmentOfAPolylineHasItsQuadrants) {
    // Bulge 1 from (-2,0) to (2,0): the lower half circle, from 180 to 360
    // degrees. Both end points and the lowest point are quadrants, in the
    // order 0, 180 and 270 degrees.
    std::vector<SnapPoint> out;
    appendSnapPoints(PolylineSegment{{-2, 0}, {2, 0}, 1}, only(&SnapModes::quadrant), out);
    expectPoints(out, {{{2, 0}, SnapKind::Quadrant},
                       {{-2, 0}, SnapKind::Quadrant},
                       {{0, -2}, SnapKind::Quadrant}});
    // The same in a polyline: shared points are listed once.
    out.clear();
    appendSnapPoints(Entity{Polyline{{{{-2, 0}, 1}, {{2, 0}, 0}}}}, only(&SnapModes::quadrant),
                     out);
    EXPECT_EQ(out.size(), 3U);
    // A straight segment has none.
    out.clear();
    appendSnapPoints(PolylineSegment{{0, 0}, {4, 0}}, only(&SnapModes::quadrant), out);
    EXPECT_TRUE(out.empty());
}

TEST(SnapPoints, PerpendicularToALineFallsOnTheLineOnly) {
    const Entity line = Line{{0, 0}, {10, 0}};
    std::vector<SnapPoint> out;
    appendSnapPoints(line, only(&SnapModes::perpendicular), out, Vec2{5, 10});
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{5, 0}, SnapKind::Perpendicular}}));
    // On the extension of the line: nothing, on either side.
    out.clear();
    appendSnapPoints(line, only(&SnapModes::perpendicular), out, Vec2{15, 10});
    appendSnapPoints(line, only(&SnapModes::perpendicular), out, Vec2{-1, 3});
    EXPECT_TRUE(out.empty());
    // Over an end point, the foot is that end point, exactly.
    appendSnapPoints(line, only(&SnapModes::perpendicular), out, Vec2{10, 4});
    appendSnapPoints(line, only(&SnapModes::perpendicular), out, Vec2{0, 4});
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{10, 0}, SnapKind::Perpendicular},
                                           {{0, 0}, SnapKind::Perpendicular}}));
    // A base point on the line is its own foot.
    out.clear();
    appendSnapPoints(line, only(&SnapModes::perpendicular), out, Vec2{3, 0});
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{3, 0}, SnapKind::Perpendicular}}));
}

TEST(SnapPoints, PerpendicularNeedsABasePointAndTheMode) {
    const Entity line = Line{{0, 0}, {10, 0}};
    std::vector<SnapPoint> out;
    appendSnapPoints(line, only(&SnapModes::perpendicular), out);
    EXPECT_TRUE(out.empty());
    appendSnapPoints(line, SnapModes{}, out, Vec2{5, 10});
    EXPECT_EQ(std::ranges::count(out, SnapKind::Perpendicular, &SnapPoint::kind), 0);
}

TEST(SnapPoints, PerpendicularToACircleIsOnTheLineThroughTheCentre) {
    std::vector<SnapPoint> out;
    appendSnapPoints(Entity{Circle{{0, 0}, 2}}, only(&SnapModes::perpendicular), out, Vec2{5, 0});
    // Both feet, the nearer first.
    expectPoints(out, {{{2, 0}, SnapKind::Perpendicular}, {{-2, 0}, SnapKind::Perpendicular}});
    // From the centre every direction is as good: none.
    out.clear();
    appendSnapPoints(Entity{Circle{{0, 0}, 2}}, only(&SnapModes::perpendicular), out, Vec2{0, 0});
    EXPECT_TRUE(out.empty());
}

TEST(SnapPoints, PerpendicularToAnArcFallsOnTheArcOnly) {
    // The arc from 0 to 90 degrees, radius 2.
    const Entity arc = Arc{{0, 0}, 2, 0, kPi / 2.0};
    const double root = std::sqrt(2.0);
    std::vector<SnapPoint> out;
    appendSnapPoints(arc, only(&SnapModes::perpendicular), out, Vec2{3, 3});
    expectPoints(out, {{{root, root}, SnapKind::Perpendicular}});
    // From the opposite side the near foot is outside the arc, the far one is on it.
    out.clear();
    appendSnapPoints(arc, only(&SnapModes::perpendicular), out, Vec2{-3, -3});
    expectPoints(out, {{{root, root}, SnapKind::Perpendicular}});
    // From a direction that meets the circle outside the arc at both ends.
    out.clear();
    appendSnapPoints(arc, only(&SnapModes::perpendicular), out, Vec2{-3, 3});
    EXPECT_TRUE(out.empty());
}

TEST(SnapPoints, PerpendicularToTheSegmentsOfAPolyline) {
    // (0,0)-(10,0)-(10,10): from (5,5) the feet are (5,0) and (10,5).
    const Entity polyline = Polyline{{{{0, 0}, 0}, {{10, 0}, 0}, {{10, 10}, 0}}};
    std::vector<SnapPoint> out;
    appendSnapPoints(polyline, only(&SnapModes::perpendicular), out, Vec2{5, 5});
    EXPECT_EQ(out, (std::vector<SnapPoint>{{{5, 0}, SnapKind::Perpendicular},
                                           {{10, 5}, SnapKind::Perpendicular}}));
    // An arc segment gives the foot on its circle.
    out.clear();
    appendSnapPoints(PolylineSegment{{-2, 0}, {2, 0}, 1}, only(&SnapModes::perpendicular), out,
                     Vec2{0, -5});
    expectPoints(out, {{{0, -2}, SnapKind::Perpendicular}});
}

TEST(SnapPoints, TextHasNeitherQuadrantsNorPerpendiculars) {
    Text text;
    text.value = "A";
    std::vector<SnapPoint> out;
    appendSnapPoints(Entity{text}, SnapModes{false, false, false, false, false, true, true, true},
                     out, Vec2{1, 1});
    EXPECT_TRUE(out.empty());
}

}  // namespace
}  // namespace tg::core
