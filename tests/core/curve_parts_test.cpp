// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/curve_parts.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

// Angle reconstruction and bulges involve trigonometry, unlike the exact
// endpoint copies. This allowance is well above their roundoff here.
constexpr double kBuilt = 1e-12;

CurvePoint place(Vec2 point, std::size_t segment = 0, Location location = Location::Inside,
                 double station = 0.0) {
    return {point, {segment, location, station}};
}

void expectOutcome(const PartResult& result, PartOutcome outcome) {
    EXPECT_EQ(result.outcome, outcome);
    EXPECT_EQ(result.entity.has_value(), outcome == PartOutcome::Done);
}

void expectArc(const PartResult& result, double start, double end) {
    ASSERT_EQ(result.outcome, PartOutcome::Done);
    if (!result.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    const auto* arc = std::get_if<Arc>(&*result.entity);
    ASSERT_NE(arc, nullptr);
    EXPECT_EQ(arc->center, (Vec2{0, 0}));
    EXPECT_EQ(arc->radius, 5.0);
    EXPECT_NEAR(arc->startAngle, degreesToRadians(start), kBuilt);
    EXPECT_NEAR(arc->endAngle, degreesToRadians(end), kBuilt);
}

TEST(CurveParts, CutsALineAtBothEndsAndKeepsTheGivenPointsExactly) {
    const Line line{{0, 0}, {10, 0}};
    const Vec2 first{2, 1e-15};
    const Vec2 last{8, -1e-15};
    const auto result = partOf(line, place(first), place(last), PartCourse::Direct);
    expectOutcome(result, PartOutcome::Done);
    if (!result.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    EXPECT_EQ(std::get<Line>(*result.entity), (Line{first, last}));
    const auto unchanged = partOf(line, std::nullopt, std::nullopt, PartCourse::Direct);
    EXPECT_EQ(unchanged.entity, std::optional<Entity>{line});
}

TEST(CurveParts, SamePlaceIsEmptyButEqualRoundedPlacesCanGiveAZeroLengthLine) {
    const Line line{{0, 0}, {2, 0}};
    // #359: distinct exact cuts can have identical rounded points/stations.
    const CurvePoint rounded = place({1, 0}, 0, Location::Inside, 1);
    const auto kept = partOf(line, rounded, rounded, PartCourse::Direct);
    expectOutcome(kept, PartOutcome::Done);
    if (!kept.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    EXPECT_EQ(std::get<Line>(*kept.entity), (Line{{1, 0}, {1, 0}}));
    expectOutcome(partOf(line, rounded, rounded, PartCourse::SamePlace), PartOutcome::Empty);
}

TEST(CurveParts, CutsAnArcAndKeepsItsCenterAndRadius) {
    const Arc arc{{0, 0}, 5, 0, degreesToRadians(180)};
    expectArc(partOf(arc, place({3, 4}), place({-3, 4}), PartCourse::Direct),
              radiansToDegrees(std::atan2(4.0, 3.0)), radiansToDegrees(std::atan2(4.0, -3.0)));
    expectArc(partOf(arc, std::nullopt, place({0, 5}), PartCourse::Direct), 0, 90);
    expectArc(partOf(arc, place({0, 5}), std::nullopt, PartCourse::Direct), 90, 180);
}

TEST(CurveParts, TurnsACircleIntoAnArcInEitherCourse) {
    const Circle circle{{0, 0}, 5};
    expectArc(partOf(circle, place({5, 0}), place({0, 5}), PartCourse::Direct), 0, 90);
    expectArc(partOf(circle, place({0, 5}), place({5, 0}), PartCourse::AcrossStart), 90, 0);
}

TEST(CurveParts, EqualRoundedAnglesDoNotTurnAShortArcIntoAWholeCircle) {
    const Arc arc{{0, 0}, 5, 0, degreesToRadians(90)};
    const CurvePoint rounded = place({3, 4});
    expectOutcome(partOf(arc, rounded, rounded, PartCourse::Direct), PartOutcome::Empty);
    const Circle circle{{0, 0}, 5};
    expectOutcome(partOf(circle, rounded, rounded, PartCourse::Direct), PartOutcome::Empty);
    expectOutcome(partOf(circle, rounded, rounded, PartCourse::AcrossStart),
                  PartOutcome::NotRepresentable);
    expectOutcome(partOf(circle, rounded, rounded, PartCourse::SamePlace), PartOutcome::Empty);
}

TEST(CurveParts, InvertedRoundedAnglesDiscardTheShortPartOfAnArcOrCircle) {
    // Review #487: the exact cuts are in order, their received points are not.
    const CurvePoint first = place({3, 4});
    const CurvePoint last = place({3.000000000000001, 4});
    const Arc quarter{{0, 0}, 5, 0, kPi / 2};
    expectOutcome(partOf(quarter, first, last, PartCourse::Direct), PartOutcome::Empty);
    // The same inversion on an arc whose original sweep crosses angle zero.
    const Arc wrapping{{0, 0}, 5, 3 * kPi / 2, kPi / 2};
    expectOutcome(partOf(wrapping, first, last, PartCourse::Direct), PartOutcome::Empty);
    const Circle circle{{0, 0}, 5};
    expectOutcome(partOf(circle, first, last, PartCourse::Direct), PartOutcome::Empty);
    expectOutcome(partOf(circle, last, first, PartCourse::AcrossStart),
                  PartOutcome::NotRepresentable);
}

TEST(CurveParts, InvertedRoundedAnglesDiscardACurvedSegmentInEitherDirection) {
    for (const double sign : {1.0, -1.0}) {
        const Polyline polyline{{{{-5, 0}, sign}, {{5, 0}}, {{10, 0}}}};
        const CurvePoint first = place({0, -5 * sign}, 0, Location::Inside, 5 * kPi / 2);
        const CurvePoint last = place({-1e-15, -5 * sign}, 0, Location::Inside, 5 * kPi / 2);
        expectOutcome(partOf(polyline, first, last, PartCourse::Direct), PartOutcome::Empty);
        const Polyline multiCurve{{{{-3, -4 * sign}, sign}, {{3, 4 * sign}}, {{10, 0}}}};
        const Vec2 invertedStart{2.999999999999999, 4 * sign};
        const auto multiple =
            partOf(multiCurve, place(invertedStart), std::nullopt, PartCourse::Direct);
        expectOutcome(multiple, PartOutcome::Done);
        if (!multiple.entity) {
            ADD_FAILURE() << "missing constructed entity";
            return;
        }
        const auto& part = std::get<Polyline>(*multiple.entity);
        ASSERT_EQ(part.vertexCount(), 3U);
        EXPECT_EQ(part.vertices()[0].position, invertedStart);
        EXPECT_EQ(part.vertices()[1].position, (Vec2{3, 4 * sign}));
        EXPECT_EQ(part.vertices()[0].bulge, 0);
    }
}

TEST(CurveParts, MinimumCorrectionKeepsAlmostTheWholeOriginalArc) {
    // Second review, example 1, and its start/end and zero-crossing mirrors.
    const Arc quarter{{0, 0}, 5, 0, kPi / 2};
    expectArc(partOf(quarter, std::nullopt, place({-1e-15, 5}), PartCourse::Direct), 0, 90);
    expectArc(partOf(quarter, place({5, -1e-14}), std::nullopt, PartCourse::Direct), 0, 90);
    expectArc(partOf(quarter, place({5, -1e-14}), place({-1e-15, 5}), PartCourse::Direct), 0, 90);
    const Arc wrapping{{0, 0}, 5, 3 * kPi / 2, kPi / 2};
    expectArc(partOf(wrapping, place({-1e-15, -5}), std::nullopt, PartCourse::Direct), 270, 90);
}

TEST(CurveParts, MinimumCorrectionPrefersTheOriginalStartOnAnExactTie) {
    // #487: the missing gap is two ulps; the supplied ray bisects it.
    constexpr double kUnit = 0x1p-50;
    const Arc almostClosed{{0, 0}, 5, 0, kTwoPi - 2 * kUnit};
    const auto middle = place({5, -5 * kUnit});
    expectOutcome(partOf(almostClosed, std::nullopt, middle, PartCourse::Direct), PartOutcome::Empty);
    const auto kept = partOf(almostClosed, middle, std::nullopt, PartCourse::Direct);
    ASSERT_TRUE(kept.entity);
    EXPECT_EQ(std::get<Arc>(*kept.entity), almostClosed);
}

TEST(CurveParts, MinimumCorrectionUsesEitherSideOfTheCircleCutInBothCourses) {
    // Second review, example 2. Reverse the exact order for AcrossStart;
    // also move the other endpoint across zero in the opposite direction.
    const Circle circle{{0, 0}, 5};
    const auto below = place({5, -1e-14});
    const auto above = place({5, 1e-14});
    const auto quarter = place({0, 5});
    const auto threeQuarters = place({0, -5});
    expectArc(partOf(circle, below, quarter, PartCourse::Direct), 0, 90);
    expectArc(partOf(circle, quarter, below, PartCourse::AcrossStart), 90, 0);
    expectArc(partOf(circle, threeQuarters, above, PartCourse::Direct), 270, 0);
    expectArc(partOf(circle, above, threeQuarters, PartCourse::AcrossStart), 0, 270);
}

TEST(CurveParts, MinimumCorrectionKeepsCurvedSegmentsInEitherDirection) {
    // Second review, example 3, mirrored for clockwise segments and both ends.
    for (const double sign : {1.0, -1.0}) {
        const Polyline polyline{{{{-5, 0}, sign, 2, 6}, {{5, 0}}, {{10, 0}}}};
        for (const bool atStart : {true, false}) {
            const auto from = atStart ? std::optional{place({-5, sign * 1e-14})} : std::nullopt;
            const auto to = atStart ? std::nullopt : std::optional{place({5, sign * 1e-14})};
            const auto result = partOf(polyline, from, to, PartCourse::Direct);
            ASSERT_EQ(result.outcome, PartOutcome::Done);
            ASSERT_TRUE(result.entity);
            const auto& part = std::get<Polyline>(*result.entity);
            EXPECT_NEAR(part.vertices()[0].bulge, sign, kBuilt);
            EXPECT_EQ(part.vertices()[0].position, from ? from->point : (Vec2{-5, 0}));
            EXPECT_EQ(part.vertices()[1].position, to ? to->point : (Vec2{5, 0}));
        }
    }
}

TEST(CurveParts, MinimumCorrectionKeepsMinimalExtensionsAtEitherEndAndDirection) {
    // Second review, example 4, its start mirror, and curved polyline ends.
    const Arc quarter{{0, 0}, 5, 0, kPi / 2};
    expectArc(extendTo(quarter, EntityEnd::End, place({1e-15, 5}, 0, Location::AfterEnd)), 0, 90);
    expectArc(extendTo(quarter, EntityEnd::Start, place({5, 1e-14}, 0, Location::AfterEnd)), 0, 90);
    for (const double sign : {1.0, -1.0}) {
        const Polyline polyline{{{{-5, 0}, sign, 2, 6}, {{5, 0}}}};
        for (const EntityEnd end : {EntityEnd::Start, EntityEnd::End}) {
            const Vec2 point{end == EntityEnd::Start ? -5.0 : 5.0, -sign * 1e-14};
            const auto result = extendTo(polyline, end, place(point, 0, Location::AfterEnd));
            ASSERT_EQ(result.outcome, PartOutcome::Done);
            ASSERT_TRUE(result.entity);
            const auto& part = std::get<Polyline>(*result.entity);
            EXPECT_NEAR(part.vertices()[0].bulge, sign, kBuilt);
            EXPECT_EQ(part.vertices()[end == EntityEnd::Start ? 0 : 1].position, point);
            EXPECT_EQ(part.vertices()[0].startWidth, 2);
            EXPECT_EQ(part.vertices()[0].endWidth, 6);
        }
    }
}

TEST(CurveParts, RebuildsSuppliedExistingArcEndpointsButKeepsOmittedAngles) {
    const Arc arc{{0, 0}, 5, 0, kPi / 2};
    const auto result = partOf(arc, place({5, 1e-15}, 0, Location::AtStart),
                               place({1e-15, 5}, 0, Location::AtEnd), PartCourse::Direct);
    expectOutcome(result, PartOutcome::Done);
    if (!result.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    const auto& cut = std::get<Arc>(*result.entity);
    EXPECT_GT(cut.startAngle, arc.startAngle);
    EXPECT_LT(cut.endAngle, arc.endAngle);
    const auto unchanged = partOf(arc, std::nullopt, std::nullopt, PartCourse::Direct);
    EXPECT_EQ(unchanged.entity, std::optional<Entity>{arc});
}

TEST(CurveParts, CutsStraightPolylineSegmentsAndInterpolatesWidths) {
    Polyline polyline{{{{0, 0}, 0, 2, 6}, {{10, 0}, 0, 8, 12}, {{10, 10}, -0.5, 3, 4}}};
    polyline.setConstantWidth(7);
    polyline.setContinuousLinetype(true);
    const Vec2 first{2, 1e-15};
    const Vec2 last{10, 5};
    const auto result = partOf(polyline, place(first, 0, Location::Inside, 2),
                               place(last, 1, Location::Inside, 5), PartCourse::Direct);
    expectOutcome(result, PartOutcome::Done);
    if (!result.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    const auto& part = std::get<Polyline>(*result.entity);
    EXPECT_FALSE(part.isClosed());
    EXPECT_EQ(part.constantWidth(), 7);
    EXPECT_TRUE(part.continuousLinetype());
    ASSERT_EQ(part.vertexCount(), 3U);
    EXPECT_EQ(part.vertices()[0].position, first);
    EXPECT_EQ(part.vertices()[1].position, (Vec2{10, 0}));
    EXPECT_EQ(part.vertices()[2].position, last);
    EXPECT_EQ(part.vertices()[2].bulge, polyline.vertices()[2].bulge);
    EXPECT_EQ(part.vertices()[2].startWidth, 3);
    EXPECT_EQ(part.vertices()[2].endWidth, 4);
    EXPECT_NEAR(part.vertices()[0].startWidth, 2.8, kBuilt);
    EXPECT_EQ(part.vertices()[0].endWidth, 6);
    EXPECT_EQ(part.vertices()[1].startWidth, 8);
    EXPECT_EQ(part.vertices()[1].endWidth, 10);
    const auto unchanged = partOf(polyline, std::nullopt, std::nullopt, PartCourse::Direct);
    EXPECT_EQ(unchanged.entity, std::optional<Entity>{polyline});
}

TEST(CurveParts, CutsPositiveAndNegativeBulgesByArcLength) {
    for (const double sign : {1.0, -1.0}) {
        const Polyline polyline{{{{-5, 0}, sign, 2, 6}, {{5, 0}, 0, 11, 12}}};
        const auto cut = place({0, -5 * sign}, 0, Location::Inside, 5 * kPi / 2);
        const auto result = partOf(polyline, cut, std::nullopt, PartCourse::Direct);
        expectOutcome(result, PartOutcome::Done);
        if (!result.entity) {
            ADD_FAILURE() << "missing constructed entity";
            return;
        }
        const auto& part = std::get<Polyline>(*result.entity);
        ASSERT_EQ(part.vertexCount(), 2U);
        EXPECT_EQ(part.vertices()[0].position, (Vec2{0, -5 * sign}));
        EXPECT_EQ(part.vertices()[1], polyline.vertices()[1]);
        EXPECT_NEAR(part.vertices()[0].bulge, sign * std::tan(kPi / 8), kBuilt);
        EXPECT_NEAR(part.vertices()[0].startWidth, 4, kBuilt);
        EXPECT_EQ(part.vertices()[0].endWidth, 6);
    }
}

TEST(CurveParts, InterpolatesClockwiseWidthsFromTheStartAwayFromTheMidpoint) {
    const Polyline polyline{{{{-5, 0}, -1, 2, 10}, {{5, 0}, 0.25, 11, 12}}};
    // Clockwise from angle pi to 3*pi/4: one quarter of the semicircle.
    const double coordinate = 5 / std::numbers::sqrt2;
    const auto cut = place({-coordinate, coordinate}, 0, Location::Inside, 5 * kPi / 4);
    for (const bool keepStart : {true, false}) {
        const auto from = keepStart ? std::nullopt : std::optional{cut};
        const auto to = keepStart ? std::optional{cut} : std::nullopt;
        const auto result = partOf(polyline, from, to, PartCourse::Direct);
        expectOutcome(result, PartOutcome::Done);
        if (!result.entity) {
            ADD_FAILURE() << "missing constructed entity";
            return;
        }
        const auto& part = std::get<Polyline>(*result.entity);
        ASSERT_EQ(part.vertexCount(), 2U);
        EXPECT_NEAR(part.vertices()[0].startWidth, keepStart ? 2 : 4, kBuilt);
        EXPECT_NEAR(part.vertices()[0].endWidth, keepStart ? 4 : 10, kBuilt);
        const double expectedBulge = -std::tan(keepStart ? kPi / 16 : 3 * kPi / 16);
        EXPECT_NEAR(part.vertices()[0].bulge, expectedBulge, kBuilt);
        EXPECT_EQ(part.vertices()[1].bulge, 0.25);
        EXPECT_EQ(part.vertices()[1].startWidth, 11);
        EXPECT_EQ(part.vertices()[1].endWidth, 12);
    }
}

TEST(CurveParts, ReplacesSuppliedExistingPolylineVerticesIncludingTheOpenEnd) {
    const Polyline polyline{{{{0, 0}}, {{10, 0}}, {{10, 10}}}};
    const Vec2 first{1e-15, 0};
    const Vec2 last{10, 10 - 2e-15};
    const auto result = partOf(polyline, place(first, 0, Location::AtStart),
                               place(last, 1, Location::AtEnd), PartCourse::Direct);
    expectOutcome(result, PartOutcome::Done);
    if (!result.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    const auto& part = std::get<Polyline>(*result.entity);
    ASSERT_EQ(part.vertexCount(), 3U);
    EXPECT_EQ(part.vertices()[0].position, first);
    EXPECT_EQ(part.vertices()[1], polyline.vertices()[1]);
    EXPECT_EQ(part.vertices()[2].position, last);
}

TEST(CurveParts, UsesTheFollowingSegmentForVertexZeroOfAClosedPolyline) {
    const Polyline rectangle{{{{0, 0}}, {{10, 0}}, {{10, 10}}, {{0, 10}}}, true};
    const Vec2 suppliedZero{1e-15, 0};
    const auto zero = place(suppliedZero, 0, Location::AtStart);
    const auto next = place({10, 0}, 1, Location::AtStart);
    const auto direct = partOf(rectangle, zero, next, PartCourse::Direct);
    const auto across = partOf(rectangle, next, zero, PartCourse::AcrossStart);
    expectOutcome(direct, PartOutcome::Done);
    expectOutcome(across, PartOutcome::Done);
    if (!direct.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    if (!across.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    const auto& shortPart = std::get<Polyline>(*direct.entity);
    ASSERT_EQ(shortPart.vertexCount(), 2U);
    EXPECT_EQ(shortPart.vertices()[0].position, suppliedZero);
    EXPECT_EQ(shortPart.vertices()[1].position, (Vec2{10, 0}));
    const auto& longPart = std::get<Polyline>(*across.entity);
    ASSERT_EQ(longPart.vertexCount(), 4U);
    EXPECT_EQ(longPart.vertices()[0].position, (Vec2{10, 0}));
    EXPECT_EQ(longPart.vertices()[3].position, suppliedZero);
}

TEST(CurveParts, DistinguishesTheTwoCoursesOnOneSegmentOfAClosedPolyline) {
    Polyline rectangle{{{{0, 0}}, {{10, 0}}, {{10, 10}}, {{0, 10}}}, true};
    rectangle.setConstantWidth(3);
    const auto shortPart = partOf(rectangle, place({2, 0}), place({8, 0}), PartCourse::Direct);
    expectOutcome(shortPart, PartOutcome::Done);
    if (!shortPart.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    const auto& shortLine = std::get<Polyline>(*shortPart.entity);
    ASSERT_EQ(shortLine.vertexCount(), 2U);
    EXPECT_EQ(shortLine.vertices()[0].position, (Vec2{2, 0}));
    EXPECT_EQ(shortLine.vertices()[1].position, (Vec2{8, 0}));
    const auto longPart = partOf(rectangle, place({8, 0}), place({2, 0}), PartCourse::AcrossStart);
    expectOutcome(longPart, PartOutcome::Done);
    if (!longPart.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    const auto& opened = std::get<Polyline>(*longPart.entity);
    ASSERT_EQ(opened.vertexCount(), 6U);
    EXPECT_FALSE(opened.isClosed());
    EXPECT_EQ(opened.constantWidth(), 3);
    EXPECT_EQ(opened.vertices()[0].position, (Vec2{8, 0}));
    for (std::size_t i = 1; i < 5; ++i) {
        EXPECT_EQ(opened.vertices()[i].position, rectangle.vertices()[i % 4].position);
    }
    EXPECT_EQ(opened.vertices()[5].position, (Vec2{2, 0}));
}

TEST(CurveParts, UsesExactVertexLocationsWithoutDuplicatingBoundaryVertices) {
    const Polyline rectangle{{{{0, 0}}, {{10, 0}}, {{10, 10}}, {{0, 10}}}, true};
    const auto result = partOf(rectangle, place({0, 10}, 3, Location::AtStart),
                               place({10, 0}, 1, Location::AtStart), PartCourse::AcrossStart);
    expectOutcome(result, PartOutcome::Done);
    if (!result.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    const auto& part = std::get<Polyline>(*result.entity);
    ASSERT_EQ(part.vertexCount(), 3U);
    EXPECT_EQ(part.vertices()[0].position, (Vec2{0, 10}));
    EXPECT_EQ(part.vertices()[1].position, (Vec2{0, 0}));
    EXPECT_EQ(part.vertices()[2].position, (Vec2{10, 0}));
}

TEST(CurveParts, DiscardsASingleRoundedPointArcButRetainsItWithinSeveralSegments) {
    const Polyline polyline{{{{-5, 0}, 1}, {{5, 0}}, {{10, 0}}}};
    const CurvePoint cut = place({5, 0}, 0, Location::Inside, 5 * kPi);
    const auto vertex = place({5, 0}, 1, Location::AtStart);
    expectOutcome(partOf(polyline, cut, vertex, PartCourse::Direct), PartOutcome::Empty);
    const auto multiple = partOf(polyline, cut, std::nullopt, PartCourse::Direct);
    expectOutcome(multiple, PartOutcome::Done);
    if (!multiple.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    const auto& part = std::get<Polyline>(*multiple.entity);
    ASSERT_EQ(part.vertexCount(), 3U);
    EXPECT_EQ(part.vertices()[0].position, part.vertices()[1].position);
    EXPECT_EQ(part.vertices()[0].bulge, 0);
    EXPECT_EQ(part.vertices()[2].position, (Vec2{10, 0}));
}

TEST(CurveParts, ExtendsALineAtEitherEndWithTheGivenPoint) {
    const Line line{{0, 0}, {10, 0}};
    const Vec2 first{-5, 1e-15};
    const Vec2 last{15, -1e-15};
    const auto start = extendTo(line, EntityEnd::Start, place(first, 0, Location::BeforeStart));
    const auto end = extendTo(line, EntityEnd::End, place(last, 0, Location::AfterEnd));
    expectOutcome(start, PartOutcome::Done);
    expectOutcome(end, PartOutcome::Done);
    if (!start.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    if (!end.entity) {
        ADD_FAILURE() << "missing constructed entity";
        return;
    }
    EXPECT_EQ(std::get<Line>(*start.entity), (Line{first, line.end}));
    EXPECT_EQ(std::get<Line>(*end.entity), (Line{line.start, last}));
}

TEST(CurveParts, ExtendsAnArcBeyondZeroAndBackwardsFromItsStart) {
    const Arc wrapping{{0, 0}, 5, degreesToRadians(270), 0};
    expectArc(extendTo(wrapping, EntityEnd::End, place({0, 5}, 0, Location::AfterEnd)), 270, 90);
    const Arc quarter{{0, 0}, 5, 0, degreesToRadians(90)};
    expectArc(extendTo(quarter, EntityEnd::Start, place({0, -5}, 0, Location::AfterEnd)), 270, 90);
    expectOutcome(extendTo(quarter, EntityEnd::End, place({5, 0}, 0, Location::AfterEnd)),
                  PartOutcome::NotRepresentable);
    expectOutcome(extendTo(quarter, EntityEnd::Start, place({0, 5}, 0, Location::AfterEnd)),
                  PartOutcome::NotRepresentable);
}

TEST(CurveParts, RejectsExtensionsWhoseRoundedAnglesPassTheOtherEnd) {
    const double boundary = std::atan2(4.0, 3.0);
    const Arc fromBoundary{{0, 0}, 5, boundary, kPi};
    const Arc toBoundary{{0, 0}, 5, 0, boundary};
    expectOutcome(extendTo(fromBoundary, EntityEnd::End,
                           place({2.999999999999999, 4}, 0, Location::AfterEnd)),
                  PartOutcome::NotRepresentable);
    expectOutcome(extendTo(toBoundary, EntityEnd::Start,
                           place({3.000000000000001, 4}, 0, Location::AfterEnd)),
                  PartOutcome::NotRepresentable);
    for (const double sign : {1.0, -1.0}) {
        const Polyline polyline{{{{-3, -4 * sign}, sign}, {{3, 4 * sign}}}};
        expectOutcome(extendTo(polyline, EntityEnd::End,
                               place({-2.999999999999999, -4 * sign}, 0, Location::AfterEnd)),
                      PartOutcome::NotRepresentable);
        expectOutcome(extendTo(polyline, EntityEnd::Start,
                               place({3.000000000000001, 4 * sign}, 0, Location::AfterEnd)),
                      PartOutcome::NotRepresentable);
    }
}

TEST(CurveParts, ExtendsPastPointSegmentsAndPreservesAllTheirOtherData) {
    const Polyline polyline{
        {{{0, 0}, 2, 1, 2}, {{0, 0}, 0, 3, 4}, {{10, 0}, -2, 5, 6}, {{10, 0}, 3, 7, 8}}};
    const auto first = place({-5, 0}, 1, Location::BeforeStart);
    const auto start = extendTo(polyline, EntityEnd::Start, first);
    const auto end = extendTo(polyline, EntityEnd::End, place({15, 0}, 1, Location::AfterEnd));
    for (const auto* result : {&start, &end}) {
        expectOutcome(*result, PartOutcome::Done);
        if (!result->entity) {
            ADD_FAILURE() << "missing constructed entity";
            return;
        }
        const auto& part = std::get<Polyline>(*result->entity);
        ASSERT_EQ(part.vertexCount(), polyline.vertexCount());
        for (std::size_t i = 0; i < part.vertexCount(); ++i) {
            EXPECT_EQ(part.vertices()[i].bulge, polyline.vertices()[i].bulge);
            EXPECT_EQ(part.vertices()[i].startWidth, polyline.vertices()[i].startWidth);
            EXPECT_EQ(part.vertices()[i].endWidth, polyline.vertices()[i].endWidth);
            Vec2 expected;
            if (result == &start) {
                expected = i < 2 ? Vec2{-5, 0} : Vec2{10, 0};
            } else {
                expected = i < 2 ? Vec2{0, 0} : Vec2{15, 0};
            }
            EXPECT_EQ(part.vertices()[i].position, expected);
        }
    }
}

TEST(CurveParts, ExtendsCurvedPolylineEndsInBothDirections) {
    for (const double sign : {1.0, -1.0}) {
        Polyline polyline{{{{-5, 0}, sign, 2, 6}, {{5, 0}, 0, 11, 12}}};
        polyline.setConstantWidth(7);
        polyline.setContinuousLinetype(true);
        for (const EntityEnd end : {EntityEnd::Start, EntityEnd::End}) {
            const auto cut = place({0, 5 * sign}, 0, Location::AfterEnd);
            const auto result = extendTo(polyline, end, cut);
            expectOutcome(result, PartOutcome::Done);
            if (!result.entity) {
                ADD_FAILURE() << "missing constructed entity";
                return;
            }
            const auto& part = std::get<Polyline>(*result.entity);
            ASSERT_EQ(part.vertexCount(), 2U);
            EXPECT_EQ(part.vertices()[end == EntityEnd::Start ? 0 : 1].position,
                      (Vec2{0, 5 * sign}));
            EXPECT_NEAR(part.vertices()[0].bulge, sign * std::tan(3 * kPi / 8), kBuilt);
            EXPECT_EQ(part.vertices()[0].startWidth, 2);
            EXPECT_EQ(part.vertices()[0].endWidth, 6);
            EXPECT_EQ(part.constantWidth(), 7);
            EXPECT_TRUE(part.continuousLinetype());
        }
        const auto closing = place({-5, 0}, 0, Location::AfterEnd);
        expectOutcome(extendTo(polyline, EntityEnd::End, closing), PartOutcome::NotRepresentable);
    }
}

}  // namespace
}  // namespace tg::core
