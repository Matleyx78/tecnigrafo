// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/fillet.hpp"

#include "tg/core/math.hpp"
#include "tg/core/text.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <numbers>
#include <optional>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

// Points and arcs that solid-space constructs are rounded within 2^-48 and
// 2^-46 of the magnitude (solid_space/geom2d/tangency.hpp): for these small
// drawings 1e-9 is far above that error and far below any distance that
// matters. Angles are compared in radians with the same bound.
constexpr double kBuilt = 1e-9;

const ToleranceRule kMillimetres{4};

/// tan(pi / 8): the bulge of a quarter turn counter-clockwise.
const double kQuarter = std::tan(std::numbers::pi / 8.0);

Entity line(Vec2 start, Vec2 end) {
    return Line{start, end};
}

/// An ARC from `startDegrees` to `endDegrees`, counter-clockwise.
Arc arcOf(Vec2 center, double radius, double startDegrees, double endDegrees) {
    return Arc{center, radius, normalizeAngle(degreesToRadians(startDegrees)),
               normalizeAngle(degreesToRadians(endDegrees))};
}

struct V {
    double x{0.0};
    double y{0.0};
    double bulge{0.0};
    double startWidth{0.0};
    double endWidth{0.0};
};

Polyline polyline(std::initializer_list<V> vertices, bool closed = false) {
    std::vector<PolylineVertex> list;
    for (const V& vertex : vertices) {
        list.push_back(PolylineVertex{Vec2{vertex.x, vertex.y}, vertex.bulge, vertex.startWidth,
                                      vertex.endWidth});
    }
    return Polyline{std::move(list), closed};
}

void expectPoint(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kBuilt);
    EXPECT_NEAR(actual.y, expected.y, kBuilt);
}

void expectLine(const std::optional<Entity>& actual, const Vec2& start, const Vec2& end) {
    ASSERT_TRUE(actual.has_value());
    const auto* got = std::get_if<Line>(&actual.value());
    ASSERT_NE(got, nullptr);
    expectPoint(got->start, start);
    expectPoint(got->end, end);
}

/// The difference of two angles, as the shortest turn between them.
double angleGap(double a, double b) {
    return std::abs(std::remainder(a - b, kTwoPi));
}

void expectArc(const std::optional<Entity>& actual, const Arc& expected) {
    ASSERT_TRUE(actual.has_value());
    const auto* got = std::get_if<Arc>(&actual.value());
    ASSERT_NE(got, nullptr);
    expectPoint(got->center, expected.center);
    EXPECT_NEAR(got->radius, expected.radius, kBuilt);
    EXPECT_LT(angleGap(got->startAngle, expected.startAngle), kBuilt);
    EXPECT_LT(angleGap(got->endAngle, expected.endAngle), kBuilt);
}

void expectVertices(const Polyline& actual, std::initializer_list<V> expected) {
    ASSERT_EQ(actual.vertexCount(), expected.size());
    std::size_t i = 0;
    for (const V& vertex : expected) {
        const PolylineVertex& got = actual.vertices()[i];
        expectPoint(got.position, {vertex.x, vertex.y});
        EXPECT_NEAR(got.bulge, vertex.bulge, kBuilt) << "vertex " << i;
        EXPECT_NEAR(got.startWidth, vertex.startWidth, kBuilt) << "vertex " << i;
        EXPECT_NEAR(got.endWidth, vertex.endWidth, kBuilt) << "vertex " << i;
        ++i;
    }
}

const Polyline& polylineOf(const std::optional<Entity>& entity) {
    return std::get<Polyline>(entity.value());
}

// --- fillet: two entities ---

TEST(Fillet, KeepsThePartThatContinuesTheArc) {
    // B2 of #367: the picks lie on the parts that do not continue the arc.
    const Entity first = line({0, 0}, {30, 0});
    const Entity second = line({0, 0}, {0, 30});
    const CornerResult result = fillet(first, {5, 0}, second, {0, 5}, 10.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    expectLine(result.first, {10, 0}, {30, 0});
    expectLine(result.second, {0, 10}, {0, 30});
    expectArc(result.added, arcOf({10, 10}, 10, 180, 270));
}

TEST(Fillet, TheSameStretchesWithReversedOrSwappedCurves) {
    const Entity first = line({30, 0}, {0, 0});
    const Entity second = line({0, 30}, {0, 0});
    const CornerResult reversed = fillet(first, {5, 0}, second, {0, 5}, 10.0, true, kMillimetres);
    ASSERT_EQ(reversed.outcome, CornerOutcome::Done);
    expectLine(reversed.first, {30, 0}, {10, 0});
    expectLine(reversed.second, {0, 30}, {0, 10});
    expectArc(reversed.added, arcOf({10, 10}, 10, 180, 270));

    const CornerResult swapped = fillet(line({0, 0}, {0, 30}), {0, 5}, line({0, 0}, {30, 0}),
                                        {5, 0}, 10.0, true, kMillimetres);
    ASSERT_EQ(swapped.outcome, CornerOutcome::Done);
    expectLine(swapped.first, {0, 10}, {0, 30});
    expectLine(swapped.second, {10, 0}, {30, 0});
    expectArc(swapped.added, arcOf({10, 10}, 10, 180, 270));
}

TEST(Fillet, ExtendsTheStartOfAnArcBackwards) {
    // Case e of autocad-raccordi.dxf: the arc is extended from its start, to
    // about 351.15 degrees, and keeps its end at 180 degrees.
    const Entity arc = arcOf({230, 0}, 10, 0, 180);
    const Entity bottom = line({215, -5}, {250, -5});
    const CornerResult result =
        fillet(arc, {238.6603, 5}, bottom, {245, -5}, 3.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    // The fillet circle touches the circle of the arc from outside: its centre
    // is at 13 from (230,0), on y = -2.
    const double cx = 230.0 + std::sqrt(169.0 - 4.0);
    const double angle = std::atan2(-2.0, cx - 230.0);
    expectArc(result.first, Arc{{230, 0}, 10, normalizeAngle(angle), kPi});
    expectLine(result.second, {cx, -5}, {250, -5});

    const CornerResult swapped =
        fillet(bottom, {245, -5}, arc, {238.6603, 5}, 3.0, true, kMillimetres);
    ASSERT_EQ(swapped.outcome, CornerOutcome::Done);
    expectArc(swapped.second, Arc{{230, 0}, 10, normalizeAngle(angle), kPi});
    expectLine(swapped.first, {cx, -5}, {250, -5});
}

TEST(Fillet, RadiusTooLargeChangesNothing) {
    // The point of tangency on L1 is (10,0), past its end: keeping the part
    // that continues the arc would turn L1 back past (5,0).
    for (const bool trim : {true, false}) {
        for (const Entity& first : {line({2, 0}, {5, 0}), line({5, 0}, {2, 0})}) {
            const CornerResult result =
                fillet(first, {3, 0}, line({0, 0}, {0, 30}), {0, 20}, 10.0, trim, kMillimetres);
            EXPECT_EQ(result.outcome, CornerOutcome::RadiusTooLarge);
            EXPECT_FALSE(result.first || result.second || result.added);
        }
    }
}

TEST(Fillet, ExtendsALineToItsPointOfTangency) {
    const CornerResult result = fillet(line({15, 0}, {30, 0}), {20, 0}, line({0, 0}, {0, 30}),
                                       {0, 20}, 10.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    expectLine(result.first, {10, 0}, {30, 0});
    expectLine(result.second, {0, 10}, {0, 30});
}

TEST(Fillet, WithoutTrimTheEntitiesStay) {
    const Entity first = line({0, 0}, {30, 0});
    const Entity second = line({0, 0}, {0, 30});
    const CornerResult result = fillet(first, {20, 0}, second, {0, 20}, 10.0, false, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    EXPECT_EQ(result.first, first);
    EXPECT_EQ(result.second, second);
    expectArc(result.added, arcOf({10, 10}, 10, 180, 270));
}

TEST(Fillet, RadiusZeroGivesTheCorner) {
    // Case f of autocad-raccordi.dxf.
    const CornerResult result = fillet(line({300, -10}, {300, 20}), {300, 10},
                                       line({290, 0}, {330, 0}), {320, 0}, 0.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    expectLine(result.first, {300, 0}, {300, 20});
    expectLine(result.second, {300, 0}, {330, 0});
    EXPECT_FALSE(result.added);
}

TEST(Fillet, ParallelLinesGiveTheSemicircle) {
    // Case c of autocad-raccordi.dxf: radius 10 whatever the radius asked.
    const Entity first = line({100, 0}, {130, 0});
    const Entity second = line({100, 20}, {130, 20});
    const CornerResult result = fillet(first, {120, 0}, second, {120, 20}, 5.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    EXPECT_EQ(result.first, first);
    EXPECT_EQ(result.second, second);
    expectArc(result.added, arcOf({130, 10}, 10, 270, 90));
}

TEST(Fillet, ACircleIsNeverCut) {
    // Case d of autocad-raccordi.dxf.
    const Entity circle = Circle{{170, 10}, 10};
    const CornerResult result =
        fillet(circle, {170, 0}, line({150, -5}, {200, -5}), {185, -5}, 3.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    EXPECT_EQ(result.first, circle);
    expectLine(result.second, {175, -5}, {200, -5});
    expectArc(result.added, arcOf({175, -2}, 3, 112.61986494804044, 270));
}

TEST(Fillet, EmptyKeptPartsEraseTheEntities) {
    // B14 of #464: both lines are consumed, only the arc remains.
    const CornerResult result =
        fillet(line({0, 0}, {1, 0}), {1, 0}, line({0, 0}, {0, 1}), {0, 1}, 1.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    EXPECT_FALSE(result.first);
    EXPECT_FALSE(result.second);
    expectArc(result.added, arcOf({1, 1}, 1, 180, 270));
}

TEST(Fillet, ADegenerateArcGivesTheCorner) {
    // An arc of radius 1e-9 mm is degenerate for the rule of millimetres
    // (docs/adr/0036, point 5): the result of radius 0.
    const Entity first = line({-10, 0}, {0, 0});
    const Entity second = line({0, 0}, {10, 1e-9});
    const CornerResult result =
        fillet(first, {-5, 0}, second, {5, 5e-10}, 1e-9, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    EXPECT_EQ(result.first, first);
    EXPECT_EQ(result.second, second);
    EXPECT_FALSE(result.added);
}

TEST(Fillet, OutcomesWithoutAFillet) {
    // No circle of radius 5 fits between them.
    EXPECT_EQ(fillet(Circle{{0, 0}, 10}, {0, 10}, line({-20, 40}, {20, 40}), {0, 40}, 5.0, true,
                     kMillimetres)
                  .outcome,
              CornerOutcome::None);
    // Concentric circles whose radii differ by 2 * radius.
    EXPECT_EQ(
        fillet(Circle{{0, 0}, 10}, {10, 0}, Circle{{0, 0}, 20}, {20, 0}, 5.0, true, kMillimetres)
            .outcome,
        CornerOutcome::InfinitelyMany);
    // The pick of the line lies exactly on the circle.
    EXPECT_EQ(fillet(Circle{{0, 0}, 10}, {0, -15}, line({-20, -10}, {20, -10}), {0, -10}, 5.0, true,
                     kMillimetres)
                  .outcome,
              CornerOutcome::AmbiguousPick);
    // A text, and a polyline with another entity.
    const Entity text = Text{};
    EXPECT_EQ(fillet(text, {0, 0}, line({0, 0}, {10, 0}), {5, 0}, 2.0, true, kMillimetres).outcome,
              CornerOutcome::NotAllowed);
    EXPECT_EQ(fillet(polyline({{0, 5}, {10, 5}}), {5, 5}, line({0, 0}, {10, 0}), {5, 0}, 2.0, true,
                     kMillimetres)
                  .outcome,
              CornerOutcome::NotAllowed);
}

TEST(Fillet, BeyondTheDomain) {
    // Slope 2^-52: the lines meet at x = -2^52, beyond solid-space's domain.
    const Entity low = line({0, 0}, {1, 0});
    const Entity high = line({0, 1}, {1, 1.0000000000000002});
    for (const double radius : {1.0, 0.0}) {
        EXPECT_EQ(fillet(low, {0.5, 0}, high, {0.5, 1}, radius, true, kMillimetres).outcome,
                  CornerOutcome::OutOfDomain)
            << "radius " << radius;
    }
    EXPECT_EQ(circleTangentTangentRadius(low, {0.5, 0}, high, {0.5, 1}, 1.0).outcome,
              CornerOutcome::OutOfDomain);
    // Two parallel lines far away (solid-space#137).
    const double far = 0x1p50;
    EXPECT_EQ(fillet(line({far, -far}, {0, 0}), {far, -far}, line({0, far}, {far, 0}), {0, far},
                     1.0, true, kMillimetres)
                  .outcome,
              CornerOutcome::OutOfDomain);
    // A radius beyond kMaxCoordinate is data beyond the domain.
    const Entity across = line({0, 0}, {10, 0});
    const Entity up = line({0, 0}, {0, 10});
    EXPECT_EQ(fillet(across, {5, 0}, up, {0, 5}, 2e15, true, kMillimetres).outcome,
              CornerOutcome::None);
    EXPECT_EQ(circleTangentTangentRadius(across, {5, 0}, up, {0, 5}, 2e15).outcome,
              CornerOutcome::None);
}

// --- fillet: two segments of one polyline ---

TEST(FilletSegments, TwoConsecutiveSegments) {
    const Polyline open = polyline({{0, 0}, {20, 0}, {20, 20}, {0, 20}});
    const CornerResult result = filletSegments(open, {10, 0}, {20, 10}, 5.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    expectVertices(polylineOf(result.first),
                   {{0, 0}, {15, 0, kQuarter}, {20, 5}, {20, 20}, {0, 20}});
    EXPECT_FALSE(result.second);
    EXPECT_FALSE(result.added);
    // The same corner with the picks swapped, within the errors of
    // solid-space (swapping the curves need not give the same bits).
    const CornerResult swapped = filletSegments(open, {20, 10}, {10, 0}, 5.0, true, kMillimetres);
    ASSERT_EQ(swapped.outcome, CornerOutcome::Done);
    expectVertices(polylineOf(swapped.first),
                   {{0, 0}, {15, 0, kQuarter}, {20, 5}, {20, 20}, {0, 20}});
}

TEST(FilletSegments, AcrossTheClosingSegment) {
    const Polyline square = polyline({{0, 0}, {40, 0}, {40, 40}, {0, 40}}, true);
    const CornerResult result = filletSegments(square, {0, 20}, {20, 0}, 5.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    expectVertices(polylineOf(result.first),
                   {{0, 5, kQuarter}, {5, 0}, {40, 0}, {40, 40}, {0, 40}});
    EXPECT_TRUE(polylineOf(result.first).isClosed());
}

TEST(FilletSegments, SegmentsThatAreNotConsecutiveOrCurved) {
    const Polyline open = polyline({{0, 0}, {20, 0}, {20, 20}, {0, 20}});
    EXPECT_EQ(filletSegments(open, {10, 0}, {10, 20}, 5.0, true, kMillimetres).outcome,
              CornerOutcome::NotAllowed);
    EXPECT_EQ(filletSegments(open, {10, 0}, {12, 0}, 5.0, true, kMillimetres).outcome,
              CornerOutcome::NotAllowed);
    // FILLET on the arcs of a polyline is left out (docs/adr/0035).
    const Polyline curved = polyline({{0, 0}, {20, 0, 0.5}, {20, 20}});
    EXPECT_EQ(filletSegments(curved, {10, 0}, {20, 10}, 5.0, true, kMillimetres).outcome,
              CornerOutcome::NotAllowed);
}

TEST(FilletSegments, WithoutTrimThePolylineStaysAndTheArcIsApart) {
    // B6 of #464, the case of the test by hand in AutoCAD.
    const Polyline corner = polyline({{0, 10}, {0, 0}, {10, 0}});
    const CornerResult result = filletSegments(corner, {0, 5}, {5, 0}, 1.0, false, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    EXPECT_EQ(result.first, Entity{corner});
    EXPECT_FALSE(result.second);
    expectArc(result.added, arcOf({1, 1}, 1, 180, 270));

    const CornerResult zero = filletSegments(corner, {0, 5}, {5, 0}, 0.0, false, kMillimetres);
    ASSERT_EQ(zero.outcome, CornerOutcome::Done);
    EXPECT_EQ(zero.first, Entity{corner});
    EXPECT_FALSE(zero.added);
}

TEST(FilletSegments, RadiusZeroKeepsTheVertex) {
    const Polyline corner = polyline({{0, 10}, {0, 0}, {10, 0}});
    const CornerResult result = filletSegments(corner, {0, 5}, {5, 0}, 0.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    EXPECT_EQ(result.first, Entity{corner});
}

TEST(FilletSegments, AHairpinBeyondTheDomain) {
    // The centre is at about 2e16 from the vertex.
    const Polyline hairpin = polyline({{10, 0}, {0, 0}, {10, 1e-15}});
    EXPECT_EQ(filletSegments(hairpin, {10, 0}, {10, 1e-15}, 1.0, true, kMillimetres).outcome,
              CornerOutcome::OutOfDomain);
    const PolylineCorners corners = filletPolyline(hairpin, 1.0, true, kMillimetres);
    EXPECT_EQ(corners.done, 0U);
    EXPECT_EQ(corners.tooShort, 1U);
    EXPECT_EQ(corners.polyline, hairpin);
}

TEST(FilletSegments, WidthsAreInterpolatedOnTheCutSegment) {
    const Polyline corner = polyline({{0, 10, 0, 1, 3}, {0, 0, 0, 2, 2}, {10, 0}});
    const CornerResult result = filletSegments(corner, {0, 8}, {8, 0}, 5.0, true, kMillimetres);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    expectVertices(polylineOf(result.first),
                   {{0, 10, 0, 1, 2}, {0, 5, kQuarter, 2, 2}, {5, 0, 0, 2, 2}, {10, 0}});
}

// --- fillet Polyline ---

TEST(FilletPolyline, ARectangle) {
    const Polyline square = polyline({{0, 0}, {40, 0}, {40, 40}, {0, 40}}, true);
    const PolylineCorners corners = filletPolyline(square, 5.0, true, kMillimetres);
    EXPECT_EQ(corners.done, 4U);
    EXPECT_EQ(corners.tooShort, 0U);
    EXPECT_TRUE(corners.added.empty());
    expectVertices(corners.polyline, {{0, 5, kQuarter},
                                      {5, 0},
                                      {35, 0, kQuarter},
                                      {40, 5},
                                      {40, 35, kQuarter},
                                      {35, 40},
                                      {5, 40, kQuarter},
                                      {0, 35}});
}

TEST(FilletPolyline, ASegmentConsumedWholeIsNotCreated) {
    // 20 by 10 with radius 5: the short sides are consumed by their two
    // fillets, and no point segment enters the polyline (docs/adr/0036).
    const Polyline slot = polyline({{0, 0}, {20, 0}, {20, 10}, {0, 10}}, true);
    const PolylineCorners corners = filletPolyline(slot, 5.0, true, kMillimetres);
    EXPECT_EQ(corners.done, 4U);
    EXPECT_EQ(corners.tooShort, 0U);
    expectVertices(corners.polyline, {{0, 5, kQuarter},
                                      {5, 0},
                                      {15, 0, kQuarter},
                                      {20, 5, kQuarter},
                                      {15, 10},
                                      {5, 10, kQuarter}});
}

TEST(FilletPolyline, SegmentsTooShort) {
    const Polyline step = polyline({{0, 0}, {20, 0}, {20, 2}, {40, 2}});
    const PolylineCorners corners = filletPolyline(step, 5.0, true, kMillimetres);
    EXPECT_EQ(corners.done, 0U);
    EXPECT_EQ(corners.tooShort, 2U);
    EXPECT_EQ(corners.polyline, step);
}

TEST(FilletPolyline, EachVertexOnThePolylineAsTheOthersLeftIt) {
    // B15 of #464.
    const Polyline cup = polyline({{0, 2}, {0, 0}, {1, 0}, {1, 2}});
    const PolylineCorners trimmed = filletPolyline(cup, 0.75, true, kMillimetres);
    EXPECT_EQ(trimmed.done, 1U);
    EXPECT_EQ(trimmed.tooShort, 1U);
    expectVertices(trimmed.polyline, {{0, 2}, {0, 0.75, kQuarter}, {0.75, 0}, {1, 0}, {1, 2}});

    // Without trim each vertex is judged on the original segments (B6).
    const PolylineCorners kept = filletPolyline(cup, 0.75, false, kMillimetres);
    EXPECT_EQ(kept.done, 2U);
    EXPECT_EQ(kept.tooShort, 0U);
    EXPECT_EQ(kept.polyline, cup);
    ASSERT_EQ(kept.added.size(), 2U);
    expectArc(kept.added[0], arcOf({0.75, 0.75}, 0.75, 180, 270));
    expectArc(kept.added[1], arcOf({0.25, 0.75}, 0.75, 270, 0));
}

TEST(FilletPolyline, ARectangleWithoutTrim) {
    const Polyline square = polyline({{0, 0}, {40, 0}, {40, 40}, {0, 40}}, true);
    const PolylineCorners corners = filletPolyline(square, 5.0, false, kMillimetres);
    EXPECT_EQ(corners.done, 4U);
    EXPECT_EQ(corners.polyline, square);
    ASSERT_EQ(corners.added.size(), 4U);
    // The visit starts from vertex 0, the closing vertex.
    expectArc(corners.added[0], arcOf({5, 5}, 5, 180, 270));
    expectArc(corners.added[1], arcOf({35, 5}, 5, 270, 0));
    expectArc(corners.added[2], arcOf({35, 35}, 5, 0, 90));
    expectArc(corners.added[3], arcOf({5, 35}, 5, 90, 180));
}

TEST(FilletPolyline, ACurvedSegmentLeavesItsVerticesAsTheyAre) {
    const Polyline curved = polyline({{0, 0}, {20, 0, 0.5}, {20, 20}, {0, 20}});
    const PolylineCorners corners = filletPolyline(curved, 2.0, true, kMillimetres);
    EXPECT_EQ(corners.done, 0U);
    EXPECT_EQ(corners.tooShort, 2U);
    EXPECT_EQ(corners.polyline, curved);
}

// --- CIRCLE Ttr ---

TEST(CircleTangentTangentRadius, BetweenTwoLinesAndACircleAndALine) {
    // Cases m and n of autocad-raccordi.dxf.
    const TangentCircleResult lines = circleTangentTangentRadius(
        line({0, 200}, {40, 200}), {10, 200}, line({0, 200}, {0, 240}), {0, 210}, 8.0);
    ASSERT_EQ(lines.outcome, CornerOutcome::Done);
    ASSERT_TRUE(lines.circle.has_value());
    expectPoint(lines.circle.value().center, {8, 208});
    EXPECT_EQ(lines.circle.value().radius, 8.0);

    const TangentCircleResult mixed = circleTangentTangentRadius(
        Circle{{80, 220}, 10}, {74, 212}, line({60, 200}, {110, 200}), {90, 200}, 6.0);
    ASSERT_EQ(mixed.outcome, CornerOutcome::Done);
    expectPoint(mixed.circle.value().center, {80.0 + std::sqrt(60.0), 206});
}

TEST(CircleTangentTangentRadius, APolylineSegmentAndATextNotAllowed) {
    const Entity corner = polyline({{0, 240}, {0, 200}, {40, 200}});
    const TangentCircleResult result =
        circleTangentTangentRadius(corner, {10, 200}, corner, {0, 210}, 8.0);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    expectPoint(result.circle.value().center, {8, 208});
    EXPECT_EQ(circleTangentTangentRadius(Text{}, {0, 0}, corner, {0, 210}, 8.0).outcome,
              CornerOutcome::NotAllowed);
}

// --- Tangent points ---

TEST(TangentPoints, FromOutsideOnAndInsideACircle) {
    const Entity circle = Circle{{0, 0}, 5};
    const double y = 5.0 * std::sqrt(75.0) / 10.0;
    const std::vector<Vec2> outside = tangentPoints({10, 0}, circle);
    ASSERT_EQ(outside.size(), 2U);
    expectPoint(outside[0], {2.5, y});
    expectPoint(outside[1], {2.5, -y});
    EXPECT_EQ(tangentPoints({5, 0}, circle), (std::vector<Vec2>{{5, 0}}));
    EXPECT_TRUE(tangentPoints({2, 0}, circle).empty());
    EXPECT_TRUE(tangentPoints({5, 5}, line({0, 0}, {10, 0})).empty());
    // Only the points on an arc.
    const std::vector<Vec2> onArc = tangentPoints({10, 0}, Entity{arcOf({0, 0}, 5, 0, 90)});
    ASSERT_EQ(onArc.size(), 1U);
    expectPoint(onArc[0], {2.5, y});
}

TEST(TangentPoints, APolylineListsTheCommonVertexOnce) {
    // B7 of #464.
    const Entity halves = polyline({{0, 0, 1}, {2, 0, 1}, {4, 0}});
    const std::vector<Vec2> points = tangentPoints({2, 1}, halves);
    ASSERT_EQ(points.size(), 1U);
    expectPoint(points[0], {2, 0});
}

TEST(TangentPoints, ARefusedSegmentKeepsThePointsOfTheOthers) {
    // The last segment is curved and reaches beyond solid-space's domain.
    const Entity halves = polyline({{0, 0, 1}, {2, 0, 1}, {4, 0, 1}, {0x1p51, 0}});
    const std::vector<Vec2> points = tangentPoints({2, 1}, halves);
    ASSERT_FALSE(points.empty());
    expectPoint(points.front(), {2, 0});
}

}  // namespace
}  // namespace tg::core
