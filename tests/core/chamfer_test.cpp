// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/chamfer.hpp"

#include "tg/core/line.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

namespace tg::core {
namespace {

const ToleranceRule kRule{4};
const Line kFirst{{0, 0}, {10, 0}};
const Line kSecond{{0, 0}, {0, 10}};

TEST(Chamfer, DistancesKeepThePickedSidesAndEraseAConsumedLine) {
    const ChamferDistances distances{10, 2};
    const auto result = chamfer(kFirst, {8, 0}, kSecond, {0, 5}, distances, true, kRule);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    EXPECT_FALSE(result.first);
    if (!result.second.has_value()) {
        FAIL() << "Expected the kept part of the second line";
    }
    EXPECT_EQ(std::get<Line>(*result.second), (Line{{0, 2}, {0, 10}}));
    if (!result.added.has_value()) {
        FAIL() << "Expected the chamfer line";
    }
    EXPECT_EQ(std::get<Line>(*result.added), (Line{{10, 0}, {0, 2}}));

    const auto corner = chamfer(kFirst, {8, 0}, kSecond, {0, 5}, ChamferDistances{}, true, kRule);
    EXPECT_EQ(corner.outcome, CornerOutcome::Done);
    EXPECT_FALSE(corner.added);
}

TEST(Chamfer, RejectsAnOversizedDistanceButGivesPrecedenceToASingleZero) {
    const ChamferDistances bothTooLong{15, 15};
    const ChamferDistances secondTooLong{5, 15};
    const ChamferDistances firstTooLong{15, 5};
    const auto oversized = {bothTooLong, secondTooLong, firstTooLong};
    for (const auto spec : oversized) {
        EXPECT_EQ(chamfer(kFirst, {8, 0}, kSecond, {0, 5}, spec, true, kRule).outcome,
                  CornerOutcome::DistanceTooLarge);
        const Polyline polyline{{{{10, 0}}, {{0, 0}}, {{0, 10}}}};
        EXPECT_EQ(chamferSegments(polyline, {8, 0}, {0, 5}, spec, true, kRule).outcome,
                  CornerOutcome::DistanceTooLarge);
    }
    const Polyline polyline{{{{10, 0}}, {{0, 0}}, {{0, 10}}}};
    const ChamferDistances firstZero{0, 15};
    const ChamferDistances secondZero{15, 0};
    EXPECT_EQ(chamfer(kFirst, {8, 0}, kSecond, {0, 5}, firstZero, true, kRule).outcome,
              CornerOutcome::None);
    EXPECT_EQ(chamfer(kSecond, {0, 5}, kFirst, {8, 0}, secondZero, true, kRule).outcome,
              CornerOutcome::None);
    EXPECT_EQ(chamferSegments(polyline, {8, 0}, {0, 5}, firstZero, true, kRule).outcome,
              CornerOutcome::None);
    EXPECT_EQ(chamferSegments(polyline, {0, 5}, {8, 0}, secondZero, true, kRule).outcome,
              CornerOutcome::None);
}

TEST(Chamfer, AngleBuildsOnTheKeptSideAndRejectsParallelOrOversizedCuts) {
    const ChamferAngle angle{5, kPi / 4.0};
    const Line shortLine{{0, 0}, {0, 1}};
    const Line reversed{{0, 1}, {0, 0}};
    for (const Line line : {shortLine, reversed}) {
        EXPECT_EQ(chamfer(kFirst, {8, 0}, line, {0, 0.5}, angle, true, kRule).outcome,
                  CornerOutcome::DistanceTooLarge);
    }
    const Polyline shortPolyline{{{{10, 0}}, {{0, 0}}, {{0, 1}}}};
    EXPECT_EQ(chamferSegments(shortPolyline, {8, 0}, {0, 0.5}, angle, true, kRule).outcome,
              CornerOutcome::DistanceTooLarge);
    EXPECT_EQ(chamferSegments(shortPolyline, {0, 0.5}, {8, 0}, angle, true, kRule).outcome,
              CornerOutcome::DistanceTooLarge);
    const ChamferAngle tooLong{15, kPi / 4.0};
    EXPECT_EQ(chamfer(kFirst, {8, 0}, kSecond, {0, 5}, tooLong, true, kRule).outcome,
              CornerOutcome::DistanceTooLarge);
    EXPECT_EQ(chamferSegments(shortPolyline, {8, 0}, {0, 0.5}, tooLong, true, kRule).outcome,
              CornerOutcome::DistanceTooLarge);
    const ChamferAngle parallel{5, kPi / 2.0};
    EXPECT_EQ(chamfer(kFirst, {8, 0}, kSecond, {0, 5}, parallel, true, kRule).outcome,
              CornerOutcome::None);

    const Line longEnough{{0, 0}, {0, 6}};
    const auto result = chamfer(kFirst, {8, 0}, longEnough, {0, 5}, angle, true, kRule);
    ASSERT_EQ(result.outcome, CornerOutcome::Done);
    if (!result.added.has_value()) {
        FAIL() << "Expected the chamfer line";
    }
    const auto& added = std::get<Line>(*result.added);
    EXPECT_EQ(added.start, (Vec2{5, 0}));
    EXPECT_NEAR(added.end.y, 5.0, 1e-12);
}

TEST(Chamfer, OversizedAngleLengthTakesPrecedenceOverAnAbsentSecondCut) {
    const Polyline polyline{{{{10, 0}}, {{0, 0}}, {{0, 10}}}};
    for (const double turn : {0.0, kPi / 2.0, 3.0 * kPi / 4.0}) {
        const ChamferAngle spec{15, turn};
        EXPECT_EQ(chamfer(kFirst, {8, 0}, kSecond, {0, 5}, spec, true, kRule).outcome,
                  CornerOutcome::DistanceTooLarge);
        EXPECT_EQ(chamferSegments(polyline, {8, 0}, {0, 5}, spec, true, kRule).outcome,
                  CornerOutcome::DistanceTooLarge);
    }
}

TEST(Chamfer, AmbiguousPickAndOmittedCornerAreDistinctFromParallelOrDegenerateInputs) {
    const ChamferDistances distances{1, 1};
    EXPECT_EQ(chamfer(kFirst, {0, 2}, kSecond, {0, 5}, distances, true, kRule).outcome,
              CornerOutcome::AmbiguousPick);
    const Polyline polyline{{{{10, 0}}, {{0, 0}}, {{0, 10}}}};
    EXPECT_EQ(chamferSegments(polyline, {0, 0}, {0, 5}, distances, true, kRule).outcome,
              CornerOutcome::AmbiguousPick);
    const Line first{{0, 0}, {1, 0}};
    const Line nearlyParallel{{0, 1}, {1, 1.0000000000000002}};
    EXPECT_EQ(chamfer(first, {0.5, 0}, nearlyParallel, {0.5, 1}, distances, true, kRule).outcome,
              CornerOutcome::OutOfDomain);
    const Line parallel{{0, 1}, {1, 1}};
    EXPECT_EQ(chamfer(first, {0.5, 0}, parallel, {0.5, 1}, distances, true, kRule).outcome,
              CornerOutcome::None);
    EXPECT_EQ(chamfer(Line{}, {0, 0}, kSecond, {0, 5}, distances, true, kRule).outcome,
              CornerOutcome::None);
}

TEST(Chamfer, PolylineVisitsShortenedSegmentsOnlyWithTrim) {
    const Polyline source{{{{0, 2}}, {{0, 0}}, {{1, 0}}, {{1, 2}}}};
    const ChamferDistances distances{0.75, 0.75};
    const auto trimmed = chamferPolyline(source, distances, true, kRule);
    EXPECT_EQ(trimmed.done, 1U);
    EXPECT_EQ(trimmed.tooShort, 1U);
    EXPECT_TRUE(trimmed.added.empty());

    const auto unchanged = chamferPolyline(source, distances, false, kRule);
    EXPECT_EQ(unchanged.done, 2U);
    EXPECT_EQ(unchanged.tooShort, 0U);
    EXPECT_EQ(unchanged.polyline, source);
    ASSERT_EQ(unchanged.added.size(), 2U);
    EXPECT_EQ(std::get<Line>(unchanged.added[0]), (Line{{0, 0.75}, {0.75, 0}}));
    EXPECT_EQ(std::get<Line>(unchanged.added[1]), (Line{{0.25, 0}, {1, 0.75}}));
}

}  // namespace
}  // namespace tg::core
