// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/curve_parts.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
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
    ASSERT_TRUE(result.entity.has_value());
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
    ASSERT_TRUE(result.entity);
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
    ASSERT_TRUE(kept.entity);
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

TEST(CurveParts, CutsStraightPolylineSegmentsAndInterpolatesWidths) {
    Polyline polyline{{{{0, 0}, 0, 2, 6}, {{10, 0}, 0, 8, 12}, {{10, 10}, 0, 3, 4}}};
    polyline.setConstantWidth(7);
    polyline.setContinuousLinetype(true);
    const Vec2 first{2, 1e-15};
    const Vec2 last{10, 5};
    const auto result = partOf(polyline, place(first, 0, Location::Inside, 2),
                               place(last, 1, Location::Inside, 5), PartCourse::Direct);
    expectOutcome(result, PartOutcome::Done);
    ASSERT_TRUE(result.entity);
    const auto& part = std::get<Polyline>(*result.entity);
    EXPECT_FALSE(part.isClosed());
    EXPECT_EQ(part.constantWidth(), 7);
    EXPECT_TRUE(part.continuousLinetype());
    ASSERT_EQ(part.vertexCount(), 3U);
    EXPECT_EQ(part.vertices()[0].position, first);
    EXPECT_EQ(part.vertices()[1].position, (Vec2{10, 0}));
    EXPECT_EQ(part.vertices()[2].position, last);
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
        ASSERT_TRUE(result.entity);
        const auto& part = std::get<Polyline>(*result.entity);
        ASSERT_EQ(part.vertexCount(), 2U);
        EXPECT_EQ(part.vertices()[0].position, (Vec2{0, -5 * sign}));
        EXPECT_EQ(part.vertices()[1], polyline.vertices()[1]);
        EXPECT_NEAR(part.vertices()[0].bulge, sign * std::tan(kPi / 8), kBuilt);
        EXPECT_NEAR(part.vertices()[0].startWidth, 4, kBuilt);
        EXPECT_EQ(part.vertices()[0].endWidth, 6);
    }
}

TEST(CurveParts, DistinguishesTheTwoCoursesOnOneSegmentOfAClosedPolyline) {
    Polyline rectangle{{{{0, 0}}, {{10, 0}}, {{10, 10}}, {{0, 10}}}, true};
    rectangle.setConstantWidth(3);
    const auto shortPart = partOf(rectangle, place({2, 0}), place({8, 0}), PartCourse::Direct);
    expectOutcome(shortPart, PartOutcome::Done);
    ASSERT_TRUE(shortPart.entity);
    const auto& shortLine = std::get<Polyline>(*shortPart.entity);
    ASSERT_EQ(shortLine.vertexCount(), 2U);
    EXPECT_EQ(shortLine.vertices()[0].position, (Vec2{2, 0}));
    EXPECT_EQ(shortLine.vertices()[1].position, (Vec2{8, 0}));
    const auto longPart = partOf(rectangle, place({8, 0}), place({2, 0}), PartCourse::AcrossStart);
    expectOutcome(longPart, PartOutcome::Done);
    ASSERT_TRUE(longPart.entity);
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
    const auto result = partOf(rectangle, place({0, 10}, 2, Location::AtEnd),
                               place({10, 0}, 1, Location::AtStart), PartCourse::AcrossStart);
    expectOutcome(result, PartOutcome::Done);
    ASSERT_TRUE(result.entity);
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
    ASSERT_TRUE(multiple.entity);
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
    ASSERT_TRUE(start.entity);
    ASSERT_TRUE(end.entity);
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

TEST(CurveParts, ExtendsPastPointSegmentsAndPreservesAllTheirOtherData) {
    const Polyline polyline{
        {{{0, 0}, 2, 1, 2}, {{0, 0}, 0, 3, 4}, {{10, 0}, -2, 5, 6}, {{10, 0}, 3, 7, 8}}};
    const auto first = place({-5, 0}, 1, Location::BeforeStart);
    const auto start = extendTo(polyline, EntityEnd::Start, first);
    const auto end = extendTo(polyline, EntityEnd::End, place({15, 0}, 1, Location::AfterEnd));
    for (const auto* result : {&start, &end}) {
        expectOutcome(*result, PartOutcome::Done);
        ASSERT_TRUE(result->entity);
        const auto& part = std::get<Polyline>(*result->entity);
        ASSERT_EQ(part.vertexCount(), polyline.vertexCount());
        for (std::size_t i = 0; i < part.vertexCount(); ++i) {
            EXPECT_EQ(part.vertices()[i].bulge, polyline.vertices()[i].bulge);
            EXPECT_EQ(part.vertices()[i].startWidth, polyline.vertices()[i].startWidth);
            EXPECT_EQ(part.vertices()[i].endWidth, polyline.vertices()[i].endWidth);
            const Vec2 expected = result == &start ? (i < 2 ? Vec2{-5, 0} : Vec2{10, 0})
                                                   : (i < 2 ? Vec2{0, 0} : Vec2{15, 0});
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
            ASSERT_TRUE(result.entity);
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
