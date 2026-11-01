// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/intersections.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

// Points that solid-space constructs are rounded within 2^-48 of the
// magnitude (solid_space/geom2d/curves.hpp): for these small drawings 1e-12
// is far above that error and far below any distance that matters.
constexpr double kBuilt = 1e-12;

constexpr Extension kNone = Extension::None;
constexpr Extension kExtended = Extension::Extended;

Entity line(Vec2 start, Vec2 end) {
    return Line{start, end};
}

Entity circle(Vec2 center, double radius) {
    return Circle{center, radius};
}

/// An ARC from `startDegrees` to `endDegrees`, counter-clockwise.
Entity arc(Vec2 center, double radius, double startDegrees, double endDegrees) {
    return Arc{center, radius, normalizeAngle(degreesToRadians(startDegrees)),
               normalizeAngle(degreesToRadians(endDegrees))};
}

/// A polyline of (position, bulge) vertices.
Polyline polyline(std::initializer_list<std::pair<Vec2, double>> vertices, bool closed = false) {
    Polyline result;
    for (const auto& [position, bulge] : vertices) {
        result.addVertex(position, bulge);
    }
    result.setClosed(closed);
    return result;
}

CurvePosition at(std::size_t segment, Location location) {
    return CurvePosition{segment, location, 0.0};
}

void expectPoint(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kBuilt);
    EXPECT_NEAR(actual.y, expected.y, kBuilt);
}

/// Segment and location of a position; the station is information only.
void expectPosition(const CurvePosition& actual, const CurvePosition& expected) {
    EXPECT_EQ(actual.segment, expected.segment);
    EXPECT_EQ(actual.location, expected.location);
}

void expectCrossing(const Crossing& actual, const Vec2& point, Contact contact,
                    const CurvePosition& onA, const CurvePosition& onB) {
    expectPoint(actual.point, point);
    EXPECT_EQ(actual.contact, contact);
    expectPosition(actual.onA, onA);
    expectPosition(actual.onB, onB);
}

// ---------------------------------------------------------------------------
// Lines

TEST(Intersect, CrossesTwoLines) {
    const auto found = intersect(line({0, 0}, {10, 0}), kNone, line({5, -5}, {5, 5}), kNone);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {5, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::Inside));
    EXPECT_TRUE(found.shared.empty());
    EXPECT_TRUE(found.complete);
}

TEST(Intersect, FindsNothingBetweenParallelLines) {
    const auto found = intersect(line({0, 0}, {10, 0}), kNone, line({0, 2}, {10, 2}), kNone);
    EXPECT_TRUE(found.crossings.empty());
    EXPECT_TRUE(found.shared.empty());
}

TEST(Intersect, MeetsAtTheEndsOfLines) {
    // A T: the end of the first line on the second (request #20, case 4).
    const auto found = intersect(line({5, -5}, {5, 0}), kNone, line({0, 0}, {10, 0}), kNone);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {5, 0}, Contact::Crossing, at(0, Location::AtEnd),
                   at(0, Location::Inside));
}

TEST(Intersect, ShareAStretchOfTheSameLine) {
    const auto found = intersect(line({0, 0}, {10, 0}), kNone, line({5, 0}, {15, 0}), kNone);
    EXPECT_TRUE(found.crossings.empty());
    ASSERT_EQ(found.shared.size(), 1U);
    EXPECT_FALSE(found.shared[0].whole);
    expectPoint(found.shared[0].from, {5, 0});
    expectPoint(found.shared[0].to, {10, 0});
    expectPosition(found.shared[0].fromOnA, at(0, Location::Inside));
    expectPosition(found.shared[0].toOnA, at(0, Location::AtEnd));
    expectPosition(found.shared[0].fromOnB, at(0, Location::AtStart));
}

TEST(Intersect, ExtendsALineToMeetAnother) {
    const Entity shortLine = line({0, 0}, {5, 0});
    const Entity target = line({10, -5}, {10, 5});
    EXPECT_TRUE(intersect(shortLine, kNone, target, kNone).crossings.empty());
    const auto found = intersect(shortLine, kExtended, target, kNone);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {10, 0}, Contact::Crossing, at(0, Location::AfterEnd),
                   at(0, Location::Inside));
    const auto backwards = intersect(line({20, 0}, {15, 0}), kExtended, target, kNone);
    ASSERT_EQ(backwards.crossings.size(), 1U);
    expectPosition(backwards.crossings[0].onA, at(0, Location::AfterEnd));
}

TEST(Intersect, ExtendsBothLinesToTheirWholeLine) {
    const auto found = intersect(line({0, 0}, {1, 0}), kExtended, line({3, 0}, {4, 0}), kExtended);
    EXPECT_TRUE(found.crossings.empty());
    ASSERT_EQ(found.shared.size(), 1U);
    EXPECT_TRUE(found.shared[0].whole);
}

TEST(Intersect, MeetsNothingWithALineOfZeroLength) {
    const auto found = intersect(line({5, 0}, {5, 0}), kNone, line({0, 0}, {10, 0}), kNone);
    EXPECT_TRUE(found.crossings.empty());
    EXPECT_TRUE(found.shared.empty());
    EXPECT_TRUE(found.complete);
}

TEST(Intersect, MeetsNothingWithAText) {
    const auto found = intersect(Entity{Text{}}, kNone, line({0, 0}, {10, 0}), kNone);
    EXPECT_TRUE(found.crossings.empty());
    EXPECT_TRUE(found.complete);
}

TEST(Intersect, IsIncompleteBeyondTheDomain) {
    // 2^51 is beyond the coordinates solid-space accepts (2^50).
    const auto found = intersect(line({0, 0}, {0x1p51, 0}), kNone, line({5, -5}, {5, 5}), kNone);
    EXPECT_TRUE(found.crossings.empty());
    EXPECT_FALSE(found.complete);
}

// ---------------------------------------------------------------------------
// Lines and circles (request #20, cases 2 and 6)

TEST(Intersect, CrossesALineAndACircleInOrderAlongTheLine) {
    const auto found = intersect(line({0, 0}, {10, 0}), kNone, circle({5, 0}, 3), kNone);
    ASSERT_EQ(found.crossings.size(), 2U);
    expectCrossing(found.crossings[0], {2, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::Inside));
    expectCrossing(found.crossings[1], {8, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::Inside));
}

TEST(Intersect, CrossesACircleAndALineInOrderAlongTheCircle) {
    // A circle runs counter-clockwise from angle 0: (8,0) comes before (2,0).
    const auto found = intersect(circle({5, 0}, 3), kNone, line({0, 0}, {10, 0}), kNone);
    ASSERT_EQ(found.crossings.size(), 2U);
    expectPoint(found.crossings[0].point, {8, 0});
    expectPoint(found.crossings[1].point, {2, 0});
}

TEST(Intersect, TellsATangentFromNearMisses) {
    const Entity round = circle({0, 0}, 5);
    const auto tangent = intersect(line({-10, 5}, {10, 5}), kNone, round, kNone);
    ASSERT_EQ(tangent.crossings.size(), 1U);
    expectCrossing(tangent.crossings[0], {0, 5}, Contact::Touching, at(0, Location::Inside),
                   at(0, Location::Inside));
    EXPECT_TRUE(intersect(line({-10, 5.000000000001}, {10, 5.000000000001}), kNone, round, kNone)
                    .crossings.empty());
    EXPECT_EQ(intersect(line({-10, 4.999999999999}, {10, 4.999999999999}), kNone, round, kNone)
                  .crossings.size(),
              2U);
}

TEST(Intersect, ExtendsALineToACircle) {
    const Entity shortLine = line({0, 0}, {1, 0});
    const Entity round = circle({10, 0}, 2);
    EXPECT_TRUE(intersect(shortLine, kNone, round, kNone).crossings.empty());
    const auto found = intersect(shortLine, kExtended, round, kNone);
    ASSERT_EQ(found.crossings.size(), 2U);
    expectCrossing(found.crossings[0], {8, 0}, Contact::Crossing, at(0, Location::AfterEnd),
                   at(0, Location::Inside));
    expectCrossing(found.crossings[1], {12, 0}, Contact::Crossing, at(0, Location::AfterEnd),
                   at(0, Location::Inside));
}

TEST(Intersect, FindsNothingOnALineOutsideACircle) {
    EXPECT_TRUE(
        intersect(line({0, 4}, {10, 4}), kNone, circle({5, 0}, 3), kNone).crossings.empty());
}

// ---------------------------------------------------------------------------
// Arcs

TEST(Intersect, KeepsOnlyThePointsOnTheArc) {
    // 180 degrees is the double below pi: (2,0) is past the exact end.
    const Entity upper = arc({5, 0}, 3, 0, 180);
    const auto found = intersect(line({0, 0}, {10, 0}), kNone, upper, kNone);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {8, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::AtStart));
}

TEST(Intersect, ExtendsAnArcToItsCircle) {
    const Entity high = arc({5, 0}, 3, 30, 150);
    const Entity axis = line({0, 0}, {10, 0});
    EXPECT_TRUE(intersect(axis, kNone, high, kNone).crossings.empty());
    const auto found = intersect(axis, kNone, high, kExtended);
    ASSERT_EQ(found.crossings.size(), 2U);
    // The rest of the circle of an arc is after its end, never before.
    expectCrossing(found.crossings[0], {2, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::AfterEnd));
    expectCrossing(found.crossings[1], {8, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::AfterEnd));
}

TEST(Intersect, TakesAnArcAcrossAngleZero) {
    // endAngle below startAngle: the arc runs through angle 0.
    const auto found = intersect(line({-10, 0}, {10, 0}), kNone, arc({0, 0}, 5, 300, 60), kNone);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {5, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::Inside));
}

TEST(Intersect, CrossesTwoArcs) {
    const auto found = intersect(arc({0, 0}, 5, 350, 100), kNone, arc({5, 5}, 5, 170, 280), kNone);
    ASSERT_EQ(found.crossings.size(), 2U);
    expectCrossing(found.crossings[0], {5, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::Inside));
    expectCrossing(found.crossings[1], {0, 5}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::Inside));
}

TEST(Intersect, SharesAStretchOfTwoArcsOfOneCircle) {
    // Request #20, case 3: arcs overlapping for 30 degrees.
    const auto found = intersect(arc({0, 0}, 5, 0, 60), kNone, arc({0, 0}, 5, 30, 90), kNone);
    EXPECT_TRUE(found.crossings.empty());
    ASSERT_EQ(found.shared.size(), 1U);
    expectPoint(found.shared[0].from, {4.330127018922193, 2.5});
    expectPoint(found.shared[0].to, {2.5, 4.330127018922194});
}

// ---------------------------------------------------------------------------
// Circles

TEST(Intersect, CrossesTwoCircles) {
    const auto found = intersect(circle({0, 0}, 5), kNone, circle({6, 0}, 5), kNone);
    ASSERT_EQ(found.crossings.size(), 2U);
    expectPoint(found.crossings[0].point, {3, 4});
    expectPoint(found.crossings[1].point, {3, -4});
}

TEST(Intersect, TouchesTangentCircles) {
    const auto outside = intersect(circle({0, 0}, 3), kNone, circle({5, 0}, 2), kNone);
    ASSERT_EQ(outside.crossings.size(), 1U);
    expectCrossing(outside.crossings[0], {3, 0}, Contact::Touching, at(0, Location::Inside),
                   at(0, Location::Inside));
    const auto inside = intersect(circle({0, 0}, 5), kNone, circle({2, 0}, 3), kNone);
    ASSERT_EQ(inside.crossings.size(), 1U);
    expectPoint(inside.crossings[0].point, {5, 0});
    EXPECT_EQ(inside.crossings[0].contact, Contact::Touching);
}

TEST(Intersect, SharesAWholeCircle) {
    const auto found = intersect(circle({1, 2}, 3), kNone, circle({1, 2}, 3), kNone);
    EXPECT_TRUE(found.crossings.empty());
    ASSERT_EQ(found.shared.size(), 1U);
    EXPECT_TRUE(found.shared[0].whole);
}

TEST(Intersect, FindsNothingBetweenConcentricCircles) {
    EXPECT_TRUE(intersect(circle({0, 0}, 5), kNone, circle({0, 0}, 10), kNone).crossings.empty());
}

// ---------------------------------------------------------------------------
// Polylines

TEST(Intersect, MeetsAPolylineWithAnArc) {
    // Segment 1 is a half circle below its chord (positive bulge).
    const Entity bent = polyline({{{0, 0}, 0.0}, {{10, 0}, 1.0}, {{20, 0}, 0.0}});
    const auto straight = intersect(line({5, -5}, {5, 5}), kNone, bent, kNone);
    ASSERT_EQ(straight.crossings.size(), 1U);
    expectCrossing(straight.crossings[0], {5, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::Inside));
    const auto curved = intersect(line({15, -5}, {15, 10}), kNone, bent, kNone);
    ASSERT_EQ(curved.crossings.size(), 1U);
    expectCrossing(curved.crossings[0], {15, -5}, Contact::Crossing, at(0, Location::AtStart),
                   at(1, Location::Inside));
}

TEST(Intersect, ListsAVertexOnceAtTheStartOfTheSegmentThatFollows) {
    const Entity bent = polyline({{{0, 0}, 0.0}, {{10, 0}, 1.0}, {{20, 0}, 0.0}});
    const auto found = intersect(line({5, -5}, {15, 5}), kNone, bent, kNone);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {10, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(1, Location::AtStart));
}

TEST(Intersect, ListsAVertexOfTheFirstEntityOnce) {
    const Entity bent = polyline({{{0, 0}, 0.0}, {{10, 0}, 1.0}, {{20, 0}, 0.0}});
    const auto found = intersect(bent, kNone, line({5, -5}, {15, 5}), kNone);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {10, 0}, Contact::Crossing, at(1, Location::AtStart),
                   at(0, Location::Inside));
}

TEST(Intersect, ListsVertexZeroOfAClosedPolylineAtTheStartOfSegmentZero) {
    const Entity square =
        polyline({{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}, {{0, 10}, 0.0}}, true);
    const auto found = intersect(line({-5, -5}, {5, 5}), kNone, square, kNone);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {0, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(0, Location::AtStart));
    const auto reversed = intersect(square, kNone, line({-5, -5}, {5, 5}), kNone);
    ASSERT_EQ(reversed.crossings.size(), 1U);
    expectPosition(reversed.crossings[0].onA, at(0, Location::AtStart));
}

TEST(Intersect, SkipsARepeatedVertex) {
    // Segment 1 is exactly a point: it meets nothing, and the vertex is on
    // segment 2.
    const Entity repeated = polyline({{{0, 0}, 0.0}, {{5, 0}, 0.0}, {{5, 0}, 0.0}, {{10, 0}, 0.0}});
    const auto found = intersect(line({5, -5}, {5, 5}), kNone, repeated, kNone);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {5, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(2, Location::AtStart));
    EXPECT_TRUE(found.complete);
}

TEST(Intersect, TakesATinyBulgeAsStraight) {
    // A bulge below 1e-12 is drawn straight (docs/adr/0036, point 7).
    const Entity flat = polyline({{{0, 0}, 1e-13}, {{10, 0}, 0.0}});
    const auto found = intersect(line({5, -1}, {5, 1}), kNone, flat, kNone);
    ASSERT_EQ(found.crossings.size(), 1U);
    EXPECT_EQ(found.crossings[0].point.y, 0.0);
    EXPECT_EQ(sideOf(flat, {5, 0}), Side::On);
}

TEST(Intersect, ExtendsAnOpenPolylineAtItsEnds) {
    const Entity open = polyline({{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}});
    // Before the start of the first segment and after the end of the last.
    const auto before = intersect(open, kExtended, line({-5, -5}, {-5, 5}), kNone);
    ASSERT_EQ(before.crossings.size(), 1U);
    expectCrossing(before.crossings[0], {-5, 0}, Contact::Crossing, at(0, Location::BeforeStart),
                   at(0, Location::Inside));
    const auto after = intersect(open, kExtended, line({5, 15}, {15, 15}), kNone);
    ASSERT_EQ(after.crossings.size(), 1U);
    expectPosition(after.crossings[0].onA, at(1, Location::AfterEnd));
    // Not after the end of the first segment, nor before the start of the last.
    EXPECT_TRUE(intersect(open, kExtended, line({15, -5}, {15, 5}), kNone).crossings.empty());
    EXPECT_TRUE(intersect(open, kExtended, line({5, -5}, {15, -5}), kNone).crossings.empty());
}

TEST(Intersect, ExtendsAPolylineOnlyAtItsEndsAsTheSecondEntity) {
    const Entity open = polyline({{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}});
    const auto found = intersect(line({-5, -5}, {-5, 5}), kNone, open, kExtended);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectPosition(found.crossings[0].onB, at(0, Location::BeforeStart));
    EXPECT_TRUE(intersect(line({15, -5}, {15, 5}), kNone, open, kExtended).crossings.empty());
}

TEST(Intersect, SharesStretchesOnlyOnTheSegmentsOfAnExtendedPolyline) {
    // The whole line of the middle segment is not part of the polyline
    // (decision of the maintainer, #357).
    const Entity open = polyline({{{5, 5}, 0.0}, {{10, 0}, 0.0}, {{20, 0}, 0.0}, {{25, 5}, 0.0}});
    const auto found = intersect(line({0, 0}, {30, 0}), kNone, open, kExtended);
    ASSERT_EQ(found.shared.size(), 1U);
    expectPoint(found.shared[0].from, {10, 0});
    expectPoint(found.shared[0].to, {20, 0});
}

TEST(SelfIntersections, FindsAPolylineCrossingItself) {
    const Polyline bow = polyline({{{0, 0}, 0.0}, {{10, 10}, 0.0}, {{10, 0}, 0.0}, {{0, 10}, 0.0}});
    const auto found = selfIntersections(bow);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {5, 5}, Contact::Crossing, at(0, Location::Inside),
                   at(2, Location::Inside));
}

TEST(SelfIntersections, ListsAVertexCrossedByAnotherSegmentOnce) {
    // Request #20, case 5: segment 0 runs through the vertex of segments 2
    // and 3.
    const Polyline closed = polyline(
        {{{-2, 0}, 0.0}, {{2, 0}, 0.0}, {{0, 2}, 0.0}, {{0, 0}, 0.0}, {{0, -2}, 0.0}}, true);
    const auto found = selfIntersections(closed);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {0, 0}, Contact::Crossing, at(0, Location::Inside),
                   at(3, Location::AtStart));
}

TEST(SelfIntersections, LeavesOutTheVerticesOfASimplePolyline) {
    EXPECT_TRUE(selfIntersections(polyline({{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}}))
                    .crossings.empty());
    EXPECT_TRUE(
        selfIntersections(
            polyline({{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}, {{0, 10}, 0.0}}, true))
            .crossings.empty());
}

TEST(SelfIntersections, FindsTwoConsecutiveSegmentsMeetingAgain) {
    // A half circle back to the start of the line before it.
    const Polyline loop = polyline({{{0, 0}, 0.0}, {{10, 0}, 1.0}, {{0, 0}, 0.0}});
    const auto found = selfIntersections(loop);
    ASSERT_EQ(found.crossings.size(), 1U);
    expectCrossing(found.crossings[0], {0, 0}, Contact::Crossing, at(0, Location::AtStart),
                   at(1, Location::AtEnd));
}

TEST(SelfIntersections, SharesTheStretchOfASegmentThatGoesBack) {
    const Polyline back = polyline({{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{4, 0}, 0.0}});
    const auto found = selfIntersections(back);
    EXPECT_TRUE(found.crossings.empty());
    ASSERT_EQ(found.shared.size(), 1U);
    expectPoint(found.shared[0].from, {4, 0});
    expectPoint(found.shared[0].to, {10, 0});
}

// ---------------------------------------------------------------------------
// Cuts along a host

TEST(CutsAlong, MergesCuttersThroughTheSamePoint) {
    const std::vector<Entity> cutters{line({5, -5}, {5, 5}), line({0, -5}, {10, 5}),
                                      line({8, -5}, {8, 5})};
    const auto found = cutsAlong(line({0, 0}, {10, 0}), kNone, cutters);
    ASSERT_EQ(found.cuts.size(), 2U);
    expectPoint(found.cuts[0].point, {5, 0});
    EXPECT_EQ(found.cuts[0].cutters, (std::vector<std::size_t>{0, 1}));
    expectPoint(found.cuts[1].point, {8, 0});
    EXPECT_EQ(found.cuts[1].cutters, (std::vector<std::size_t>{2}));
    EXPECT_TRUE(found.overlapping.empty());
    EXPECT_TRUE(found.complete);
}

TEST(CutsAlong, LeavesOutACutterThatOverlaps) {
    const std::vector<Entity> cutters{line({5, -5}, {5, 5}), line({7, 0}, {20, 0}), Text{}};
    const auto found = cutsAlong(line({0, 0}, {10, 0}), kNone, cutters);
    ASSERT_EQ(found.cuts.size(), 1U);
    expectPoint(found.cuts[0].point, {5, 0});
    EXPECT_EQ(found.cuts[0].cutters, (std::vector<std::size_t>{0}));
    EXPECT_EQ(found.overlapping, (std::vector<std::size_t>{1}));
}

TEST(CutsAlong, CutsNothingWithAText) {
    const std::vector<Entity> cutters{Entity{Text{}}};
    const auto found = cutsAlong(line({0, 0}, {10, 0}), kNone, cutters);
    EXPECT_TRUE(found.cuts.empty());
    EXPECT_TRUE(found.complete);
}

TEST(CutsAlong, CutsARectangleAtItsVerticesWhenItCutsItself) {
    const Entity square =
        polyline({{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}, {{0, 10}, 0.0}}, true);
    const std::vector<Entity> cutters{line({5, -5}, {5, 15})};
    const auto plain = cutsAlong(square, kNone, cutters);
    ASSERT_EQ(plain.cuts.size(), 2U);
    expectPosition(plain.cuts[0].onHost, at(0, Location::Inside));
    expectPosition(plain.cuts[1].onHost, at(2, Location::Inside));

    const auto itself = cutsAlong(square, kNone, cutters, true);
    const std::vector<std::pair<Vec2, CurvePosition>> expected{
        {{0, 0}, at(0, Location::AtStart)},  {{5, 0}, at(0, Location::Inside)},
        {{10, 0}, at(1, Location::AtStart)}, {{10, 10}, at(2, Location::AtStart)},
        {{5, 10}, at(2, Location::Inside)},  {{0, 10}, at(3, Location::AtStart)},
    };
    ASSERT_EQ(itself.cuts.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        expectPoint(itself.cuts[i].point, expected[i].first);
        expectPosition(itself.cuts[i].onHost, expected[i].second);
    }
    EXPECT_TRUE(itself.cuts[0].cutters.empty());
    EXPECT_EQ(itself.cuts[1].cutters, (std::vector<std::size_t>{0}));
}

TEST(CutsAlong, CutsAnOpenPolylineAtBothEndsWhenItCutsItself) {
    const Entity open = polyline({{{0, 0}, 0.0}, {{10, 0}, 0.0}});
    const auto found = cutsAlong(open, kNone, {}, true);
    ASSERT_EQ(found.cuts.size(), 2U);
    expectPosition(found.cuts[0].onHost, at(0, Location::AtStart));
    expectPosition(found.cuts[1].onHost, at(0, Location::AtEnd));
}

TEST(CutsAlong, CutsAPolylineWhereItCrossesItself) {
    const Entity bow = polyline({{{0, 0}, 0.0}, {{10, 10}, 0.0}, {{10, 0}, 0.0}, {{0, 10}, 0.0}});
    const auto found = cutsAlong(bow, kNone, {}, true);
    // The four vertices and the crossing, met twice: on segments 0 and 2.
    ASSERT_EQ(found.cuts.size(), 6U);
    expectPoint(found.cuts[1].point, {5, 5});
    expectPosition(found.cuts[1].onHost, at(0, Location::Inside));
    expectPoint(found.cuts[4].point, {5, 5});
    expectPosition(found.cuts[4].onHost, at(2, Location::Inside));
}

TEST(CutsAlong, ExtendsAnOpenPolylineWhoseFirstSegmentIsCurved) {
    // Segment 0: a half circle below (0,0)-(10,0); extended, the rest of
    // its circle, after its end as in solid-space.
    const Entity open = polyline({{{0, 0}, 1.0}, {{10, 0}, 0.0}, {{20, 0}, 0.0}});
    const std::vector<Entity> cutters{line({5, -10}, {5, 10})};
    const auto plain = cutsAlong(open, kNone, cutters);
    ASSERT_EQ(plain.cuts.size(), 1U);
    expectPoint(plain.cuts[0].point, {5, -5});
    const auto extended = cutsAlong(open, kExtended, cutters);
    ASSERT_EQ(extended.cuts.size(), 2U);
    expectPoint(extended.cuts[0].point, {5, -5});
    expectPosition(extended.cuts[0].onHost, at(0, Location::Inside));
    expectPoint(extended.cuts[1].point, {5, 5});
    expectPosition(extended.cuts[1].onHost, at(0, Location::AfterEnd));
}

TEST(CutsAlong, ExtendsALineToItsCutters) {
    const std::vector<Entity> cutters{line({-5, -5}, {-5, 5}), line({15, -5}, {15, 5})};
    const auto found = cutsAlong(line({0, 0}, {10, 0}), kExtended, cutters);
    ASSERT_EQ(found.cuts.size(), 2U);
    expectPosition(found.cuts[0].onHost, at(0, Location::BeforeStart));
    EXPECT_EQ(found.cuts[0].cutters, (std::vector<std::size_t>{0}));
    expectPosition(found.cuts[1].onHost, at(0, Location::AfterEnd));
    EXPECT_EQ(found.cuts[1].cutters, (std::vector<std::size_t>{1}));
}

// ---------------------------------------------------------------------------
// Side and nearest position

TEST(SideOf, LooksAlongALine) {
    // Request #20, case 7: 1e-13 from the line.
    const Entity axis = line({0, 0}, {10, 0});
    EXPECT_EQ(sideOf(axis, {5, 1e-13}), Side::Left);
    EXPECT_EQ(sideOf(axis, {5, -1e-13}), Side::Right);
    EXPECT_EQ(sideOf(axis, {15, 0}), Side::On);
}

TEST(SideOf, HasTheInsideOfCirclesAndArcsOnTheLeft) {
    EXPECT_EQ(sideOf(circle({0, 0}, 5), {0, 0}), Side::Left);
    EXPECT_EQ(sideOf(circle({0, 0}, 5), {10, 0}), Side::Right);
    EXPECT_EQ(sideOf(circle({0, 0}, 5), {5, 0}), Side::On);
    const Entity quarter = arc({0, 0}, 5, 0, 90);
    EXPECT_EQ(sideOf(quarter, {1, 1}), Side::Left);
    EXPECT_EQ(sideOf(quarter, {-5, 0}), Side::On);
    EXPECT_EQ(sideOf(quarter, {-6, 0}), Side::Right);
}

TEST(SideOf, TakesTheNearestSegmentOfAPolyline) {
    const Entity corner = polyline({{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}});
    EXPECT_EQ(sideOf(corner, {5, 1}), Side::Left);
    EXPECT_EQ(sideOf(corner, {11, 5}), Side::Right);
    EXPECT_EQ(sideOf(corner, {9, 5}), Side::Left);
    // A clockwise half circle has its inside on the right.
    const Entity clockwise = polyline({{{0, 0}, -1.0}, {{10, 0}, 0.0}});
    EXPECT_EQ(sideOf(clockwise, {5, 1}), Side::Right);
}

TEST(SideOf, HasNoAnswerForATextOrAPoint) {
    EXPECT_FALSE(sideOf(Entity{Text{}}, {0, 0}).has_value());
    EXPECT_FALSE(sideOf(line({1, 1}, {1, 1}), {0, 0}).has_value());
    EXPECT_FALSE(sideOf(polyline({{{1, 1}, 0.0}}), {0, 0}).has_value());
}

TEST(NearestPosition, FindsTheEndsAndTheInsideOfALine) {
    const Entity axis = line({0, 0}, {10, 0});
    expectPosition(nearestPosition(axis, {-3, 2}), at(0, Location::AtStart));
    expectPosition(nearestPosition(axis, {13, 2}), at(0, Location::AtEnd));
    const CurvePosition inside = nearestPosition(axis, {4, 2});
    expectPosition(inside, at(0, Location::Inside));
    EXPECT_NEAR(inside.station, 4.0, kBuilt);
}

TEST(NearestPosition, MeasuresArcsAndCirclesCounterClockwise) {
    const Entity quarter = arc({0, 0}, 2, 0, 90);
    const CurvePosition inside = nearestPosition(quarter, {3, 3});
    expectPosition(inside, at(0, Location::Inside));
    EXPECT_NEAR(inside.station, kPi / 2.0, 1e-9);
    expectPosition(nearestPosition(quarter, {3, -1}), at(0, Location::AtStart));
    expectPosition(nearestPosition(quarter, {-1, 3}), at(0, Location::AtEnd));
    const CurvePosition onCircle = nearestPosition(circle({0, 0}, 2), {0, -5});
    expectPosition(onCircle, at(0, Location::Inside));
    EXPECT_NEAR(onCircle.station, 3.0 * kPi, 1e-9);
}

TEST(NearestPosition, TakesTheNearestSegmentOfAPolyline) {
    const Entity corner = polyline({{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}});
    expectPosition(nearestPosition(corner, {12, 5}), at(1, Location::Inside));
    // The corner is at the same distance from both: the first segment.
    expectPosition(nearestPosition(corner, {11, -1}), at(0, Location::AtEnd));
    const Entity bent = polyline({{{0, 0}, 1.0}, {{10, 0}, 0.0}});
    const CurvePosition low = nearestPosition(bent, {5, -7});
    expectPosition(low, at(0, Location::Inside));
    EXPECT_NEAR(low.station, 5.0 * kPi / 2.0, 1e-9);
}

}  // namespace
}  // namespace tg::core
