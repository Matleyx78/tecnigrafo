// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/snap_points.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

namespace tg::core {
namespace {

TEST(SnapPoints, DefaultsAndEquality) {
    // docs/adr/0033, point 6: the modes of today plus intersection.
    EXPECT_EQ(SnapModes{}, (SnapModes{true, true, true, true, true, false, false, false}));
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

// --- Quadrant and Perpendicular (docs/adr/0033, points 2, 4 and 6) ------------

constexpr SnapModes kQuadrantOnly{false, false, false, false, false, false, false, true};
constexpr SnapModes kPerpendicularOnly{false, false, false, false, false, true, false, false};

// The points of a curve computed with trigonometry are exact only up to
// rounding.
constexpr double kRounding = 1e-12;

std::vector<SnapPoint> pointsOf(const Entity& entity, const SnapModes& modes,
                                const std::optional<Vec2>& from = std::nullopt) {
    std::vector<SnapPoint> out;
    appendSnapPoints(entity, modes, out, from);
    return out;
}

/// `actual` holds exactly the points `expected` of `kind`, in any order, up to
/// rounding.
void expectPoints(const std::vector<SnapPoint>& actual, SnapKind kind,
                  const std::vector<Vec2>& expected) {
    ASSERT_EQ(actual.size(), expected.size());
    for (const Vec2& point : expected) {
        const bool found = std::ranges::any_of(actual, [&](const SnapPoint& snap) {
            return snap.kind == kind && distance(snap.point, point) <= kRounding;
        });
        EXPECT_TRUE(found) << "missing " << point.x << "," << point.y;
    }
}

TEST(SnapPoints, NewModesFollowTheAdrDefaults) {
    const SnapModes modes;
    EXPECT_TRUE(modes.intersection);
    EXPECT_FALSE(modes.perpendicular);
    EXPECT_FALSE(modes.tangent);
    EXPECT_FALSE(modes.quadrant);
    // With the defaults, no quadrant and no perpendicular is offered.
    for (const Entity& entity : std::vector<Entity>{Circle{{0, 0}, 5}, Arc{{0, 0}, 5, 0, kPi}}) {
        for (const SnapPoint& snap : pointsOf(entity, modes, Vec2{9, 9})) {
            EXPECT_NE(snap.kind, SnapKind::Quadrant);
            EXPECT_NE(snap.kind, SnapKind::Perpendicular);
        }
    }
}

TEST(SnapQuadrant, CircleHasTheFourPointsOnTheAxes) {
    // Exact: the offsets are not cos/sin of multiples of π/2.
    EXPECT_EQ(pointsOf(Circle{{3, 4}, 5}, kQuadrantOnly),
              (std::vector<SnapPoint>{{{8, 4}, SnapKind::Quadrant},
                                      {{3, 9}, SnapKind::Quadrant},
                                      {{-2, 4}, SnapKind::Quadrant},
                                      {{3, -1}, SnapKind::Quadrant}}));
}

TEST(SnapQuadrant, ArcHasOnlyTheQuadrantsInsideIt) {
    // From 30° to 200°: 90° and 180°.
    expectPoints(
        pointsOf(Arc{{50, 0}, 10, degreesToRadians(30), degreesToRadians(200)}, kQuadrantOnly),
        SnapKind::Quadrant, {{50, 10}, {40, 0}});
    // An arc between two quadrants has none.
    EXPECT_TRUE(
        pointsOf(Arc{{0, 0}, 10, degreesToRadians(100), degreesToRadians(170)}, kQuadrantOnly)
            .empty());
    // An arc across 0° has that one only.
    expectPoints(
        pointsOf(Arc{{0, 0}, 10, degreesToRadians(350), degreesToRadians(10)}, kQuadrantOnly),
        SnapKind::Quadrant, {{10, 0}});
}

TEST(SnapQuadrant, ArcEndPointsThatAreQuadrantsCount) {
    // From 0° to 90°, as AutoCAD LT does (docs/verifica, snap/quadrante).
    expectPoints(pointsOf(Arc{{100, 0}, 10, 0, kPi / 2.0}, kQuadrantOnly), SnapKind::Quadrant,
                 {{110, 0}, {100, 10}});
}

TEST(SnapQuadrant, ArcSegmentOfAPolylineFollowsItsDirection) {
    // Clockwise over the top, from (10,0) to (30,0) around (20,0): 0°, 90°,
    // 180°; the straight segment before it has none.
    const Entity polyline = Polyline{{{{0, 0}}, {{10, 0}, -1}, {{30, 0}}}};
    expectPoints(pointsOf(polyline, kQuadrantOnly), SnapKind::Quadrant,
                 {{30, 0}, {20, 10}, {10, 0}});
    // The same segment alone.
    std::vector<SnapPoint> out;
    appendSnapPoints(PolylineSegment{{10, 0}, {30, 0}, -1}, kQuadrantOnly, out);
    expectPoints(out, SnapKind::Quadrant, {{30, 0}, {20, 10}, {10, 0}});
}

TEST(SnapQuadrant, OnlyWithTheModeAndOnlyOfCurvesThatAreCircular) {
    EXPECT_TRUE(pointsOf(Circle{{0, 0}, 5}, SnapModes{false, false, false, false}).empty());
    for (const Entity& entity : std::vector<Entity>{Line{{0, 0}, {4, 0}}, Text{}}) {
        EXPECT_TRUE(pointsOf(entity, kQuadrantOnly).empty());
    }
}

TEST(SnapPerpendicular, LineGivesTheFootFromTheBasePoint) {
    const Entity line = Line{{0, 0}, {10, 0}};
    EXPECT_EQ(pointsOf(line, kPerpendicularOnly, Vec2{5, 10}),
              (std::vector<SnapPoint>{{{5, 0}, SnapKind::Perpendicular}}));
    // A foot at an end is that end exactly.
    EXPECT_EQ(pointsOf(line, kPerpendicularOnly, Vec2{10, 10}),
              (std::vector<SnapPoint>{{{10, 0}, SnapKind::Perpendicular}}));
    EXPECT_EQ(pointsOf(line, kPerpendicularOnly, Vec2{0, -3}),
              (std::vector<SnapPoint>{{{0, 0}, SnapKind::Perpendicular}}));
    // The base point on the line is its own foot.
    EXPECT_EQ(pointsOf(line, kPerpendicularOnly, Vec2{4, 0}),
              (std::vector<SnapPoint>{{{4, 0}, SnapKind::Perpendicular}}));
}

TEST(SnapPerpendicular, FootOnTheExtensionIsNotGiven) {
    const Entity line = Line{{0, 0}, {10, 0}};
    EXPECT_TRUE(pointsOf(line, kPerpendicularOnly, Vec2{15, 10}).empty());
    EXPECT_TRUE(pointsOf(line, kPerpendicularOnly, Vec2{-0.001, 10}).empty());
}

TEST(SnapPerpendicular, NothingWithoutTheBasePointOrTheMode) {
    const Entity line = Line{{0, 0}, {10, 0}};
    EXPECT_TRUE(pointsOf(line, kPerpendicularOnly).empty());
    EXPECT_TRUE(pointsOf(line, SnapModes{false, false, false, false}, Vec2{5, 10}).empty());
    // A zero-length line has an Endpoint at most: no direction, no foot.
    EXPECT_TRUE(pointsOf(Line{{2, 2}, {2, 2}}, kPerpendicularOnly, Vec2{5, 10}).empty());
}

TEST(SnapPerpendicular, CircleGivesTheTwoPointsOnTheLineThroughTheCentre) {
    const double side = 10.0 / std::numbers::sqrt2;
    expectPoints(pointsOf(Circle{{0, 0}, 10}, kPerpendicularOnly, Vec2{10, 10}),
                 SnapKind::Perpendicular, {{side, side}, {-side, -side}});
    // From a point of the circle: that point and the opposite one.
    expectPoints(pointsOf(Circle{{0, 0}, 10}, kPerpendicularOnly, Vec2{10, 0}),
                 SnapKind::Perpendicular, {{10, 0}, {-10, 0}});
    // From the centre every direction is perpendicular: none is chosen.
    EXPECT_TRUE(pointsOf(Circle{{0, 0}, 10}, kPerpendicularOnly, Vec2{0, 0}).empty());
    // The one on the side of the base point comes first.
    const std::vector<SnapPoint> both =
        pointsOf(Circle{{0, 0}, 10}, kPerpendicularOnly, Vec2{0, 7});
    ASSERT_EQ(both.size(), 2U);
    EXPECT_NEAR(both[0].point.y, 10.0, kRounding);
    EXPECT_NEAR(both[1].point.y, -10.0, kRounding);
}

TEST(SnapPerpendicular, ArcGivesOnlyTheFeetThatFallOnIt) {
    const Entity arc = Arc{{0, 0}, 10, 0, kPi / 2.0};
    const double side = 10.0 / std::numbers::sqrt2;
    // The foot on the side of the base point.
    expectPoints(pointsOf(arc, kPerpendicularOnly, Vec2{10, 10}), SnapKind::Perpendicular,
                 {{side, side}});
    // The foot opposite to the base point falls on the arc.
    expectPoints(pointsOf(arc, kPerpendicularOnly, Vec2{-10, -10}), SnapKind::Perpendicular,
                 {{side, side}});
    // Neither falls on it: 135° and 315°.
    EXPECT_TRUE(pointsOf(arc, kPerpendicularOnly, Vec2{-10, 10}).empty());
}

TEST(SnapPerpendicular, EverySegmentOfAPolylineHasItsOwnFoot) {
    const Entity polyline = Polyline{{{{0, 0}}, {{10, 0}}, {{10, 10}}}};
    EXPECT_EQ(pointsOf(polyline, kPerpendicularOnly, Vec2{5, 5}),
              (std::vector<SnapPoint>{{{5, 0}, SnapKind::Perpendicular},
                                      {{10, 5}, SnapKind::Perpendicular}}));
}

TEST(SnapPerpendicular, ArcSegmentOfAPolylineUsesItsCircle) {
    // The upper half of the circle around (20,0): only the foot above.
    const Entity polyline = Polyline{{{{10, 0}, -1}, {{30, 0}}}};
    expectPoints(pointsOf(polyline, kPerpendicularOnly, Vec2{20, 20}), SnapKind::Perpendicular,
                 {{20, 10}});
    EXPECT_TRUE(pointsOf(polyline, kPerpendicularOnly, Vec2{20, -20}).size() == 1U);
}

TEST(SnapPerpendicular, TextHasNone) {
    EXPECT_TRUE(pointsOf(Text{}, kPerpendicularOnly, Vec2{1, 1}).empty());
}

}  // namespace
}  // namespace tg::core
