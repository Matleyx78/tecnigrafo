// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/tolerance.hpp"

#include "tg/core/document.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>

namespace tg::core {
namespace {

// The drawing units of INSUNITS in metres (docs/adr/0036, point 1).
constexpr int kNoUnit = 0;
constexpr int kMillimetres = 4;
constexpr int kMetres = 6;
constexpr int kKilometres = 7;
constexpr int kYards = 10;

TEST(UnitInMeters, GivesEveryUnitOfInsunits) {
    // The table of docs/adr/0036, point 1, value by value.
    const std::array<double, Document::kMaxUnits + 1> expected{
        0.001,
        0.0254,
        0.3048,
        1609.344,
        0.001,
        0.01,
        1.0,
        1000.0,
        2.54e-8,
        2.54e-5,
        0.9144,
        1e-10,
        1e-9,
        1e-6,
        0.1,
        10.0,
        100.0,
        1e9,
        149597870700.0,
        9460730472580800.0,
        3.0856775814913673e16,
        1200.0 / 3937.0,
        100.0 / 3937.0,
        3600.0 / 3937.0,
        6336000.0 / 3937.0,
    };
    for (int units = 0; units <= Document::kMaxUnits; ++units) {
        EXPECT_EQ(unitInMeters(units), expected.at(static_cast<std::size_t>(units)))
            << "INSUNITS " << units;
    }
}

TEST(UnitInMeters, TakesNoUnitAsMillimetres) {
    EXPECT_EQ(unitInMeters(kNoUnit), unitInMeters(kMillimetres));
}

TEST(UnitInMeters, GivesTheParsecFromTheAstronomicalUnit) {
    // 648000/pi astronomical units, within a few ulps of the double nearest.
    EXPECT_NEAR(unitInMeters(20), 648000.0 / kPi * unitInMeters(18), 16.0);
}

TEST(ToleranceRule, KeepsItsUnits) {
    EXPECT_EQ(ToleranceRule{kKilometres}.units(), kKilometres);
}

TEST(ToleranceRule, TakesTheUnitsOfTheDocument) {
    Document document = Document::newDrawing();
    document.setUnits(kKilometres);
    EXPECT_EQ(ToleranceRule::of(document).units(), kKilometres);
}

TEST(ToleranceRule, HasTheNanometreFloorInMillimetres) {
    // solid-space ADR 0009: epsilon(M) = max(1e-9 m in drawing units, 2^-42 M).
    const ToleranceRule rule{kMillimetres};
    EXPECT_DOUBLE_EQ(rule.epsilon(0.0), 1e-6);
    EXPECT_DOUBLE_EQ(rule.epsilon(10.0), 1e-6);
}

TEST(ToleranceRule, GrowsWithTheMagnitude) {
    const ToleranceRule rule{kMillimetres};
    EXPECT_DOUBLE_EQ(rule.epsilon(1e9), 0x1p-42 * 1e9);
}

TEST(ToleranceRule, HasTheNanometreFloorInKilometres) {
    EXPECT_DOUBLE_EQ(ToleranceRule{kKilometres}.epsilon(0.0), 1e-12);
}

TEST(ToleranceRule, MergesPointsWithinTheFloorInMillimetres) {
    // Case 1 of solid-space request #23: 1e-10 mm apart.
    const ToleranceRule rule{kMillimetres};
    EXPECT_TRUE(rule.coincident({10.0, 0.0}, {10.0, 1e-10}));
    EXPECT_FALSE(rule.coincident({0.0, 0.0}, {1.000001e-6, 0.0}));
    EXPECT_FALSE(rule.coincident({0.0, 0.0}, {0.0, 1e-3}));
}

TEST(ToleranceRule, MergesPointsWithinTheFloorInKilometres) {
    // Case 4 of solid-space request #23.
    const ToleranceRule rule{kKilometres};
    EXPECT_TRUE(rule.coincident({100000.00001, 100000.0}, {100000.00001, 100000.0}));
    EXPECT_TRUE(rule.coincident({0.0, 0.0}, {1e-12, 0.0}));
    EXPECT_FALSE(rule.coincident({0.0, 0.0}, {1e-11, 0.0}));
}

TEST(ToleranceRule, DependsOnTheUnit) {
    // 1e-11 apart: a yard is about a metre, so the floor is about 1e-9.
    EXPECT_TRUE(ToleranceRule{kYards}.coincident({0.0, 0.0}, {1e-11, 0.0}));
    EXPECT_TRUE(ToleranceRule{kMetres}.coincident({0.0, 0.0}, {1e-9, 0.0}));
    EXPECT_FALSE(ToleranceRule{kMetres}.coincident({0.0, 0.0}, {2e-9, 0.0}));
}

TEST(IsDegenerate, FindsAShortLineDegenerateInEveryUnit) {
    // Case 5 of solid-space request #23: a piece 1e-14 long.
    const Entity line = Line{{0.0, 0.0}, {1e-14, 0.0}};
    EXPECT_TRUE(isDegenerate(line, ToleranceRule{kMillimetres}));
    EXPECT_TRUE(isDegenerate(line, ToleranceRule{kKilometres}));
}

TEST(IsDegenerate, KeepsAnOrdinaryLine) {
    EXPECT_FALSE(isDegenerate(Entity{Line{{0.0, 0.0}, {10.0, 0.0}}}, ToleranceRule{kMillimetres}));
}

TEST(IsDegenerate, FindsAZeroLengthLineDegenerate) {
    EXPECT_TRUE(isDegenerate(Entity{Line{{3.0, 4.0}, {3.0, 4.0}}}, ToleranceRule{kMillimetres}));
}

TEST(IsDegenerate, JudgesCirclesAndArcsByTheirSize) {
    const ToleranceRule rule{kMillimetres};
    EXPECT_TRUE(isDegenerate(Entity{Circle{{0.0, 0.0}, 1e-14}}, rule));
    EXPECT_FALSE(isDegenerate(Entity{Circle{{0.0, 0.0}, 10.0}}, rule));
    EXPECT_TRUE(isDegenerate(Entity{Arc{{0.0, 0.0}, 1e-14, 0.0, kPi / 2.0}}, rule));
    EXPECT_FALSE(isDegenerate(Entity{Arc{{0.0, 0.0}, 10.0, 0.0, kPi / 2.0}}, rule));
}

TEST(IsDegenerate, NeverFindsAPolylineOrATextDegenerate) {
    const ToleranceRule rule{kMillimetres};
    Polyline polyline;
    polyline.addVertex({0.0, 0.0});
    polyline.addVertex({1e-14, 0.0});
    EXPECT_FALSE(isDegenerate(Entity{polyline}, rule));
    EXPECT_FALSE(isDegenerate(Entity{Text{}}, rule));
}

TEST(IsDegenerate, JudgesOnePolylineSegment) {
    const ToleranceRule rule{kMillimetres};
    EXPECT_TRUE(isDegenerate(PolylineSegment{{10.0, 0.0}, {10.00000000000001, 0.0}}, rule));
    EXPECT_FALSE(isDegenerate(PolylineSegment{{0.0, 0.0}, {10.0, 0.0}}, rule));
    EXPECT_TRUE(isDegenerate(PolylineSegment{{5.0, 5.0}, {5.0, 5.0}, 1.0}, rule));
    EXPECT_FALSE(isDegenerate(PolylineSegment{{0.0, 0.0}, {10.0, 0.0}, 1.0}, rule));
}

}  // namespace
}  // namespace tg::core
