// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/coordinate_input.hpp"

#include <gtest/gtest.h>

#include <numbers>
#include <optional>
#include <ostream>
#include <string_view>

namespace tg::cmd {

// Readable failure messages for PointSpec.
std::ostream& operator<<(std::ostream& out, const PointSpec& spec) {
    return out << "PointSpec{kind " << static_cast<int>(spec.kind) << ", " << spec.a << ", "
               << spec.b << "}";
}

namespace {

using core::Vec2;
using Kind = PointSpec::Kind;

/// Coordinates computed with sin and cos are exact only up to rounding.
constexpr double kTolerance = 1e-9;

/// Any pointer position, for the forms that do not use it.
constexpr Vec2 kAnyPointer{123.0, -456.0};

void expectPoint(const std::optional<Vec2>& actual, const Vec2& expected) {
    if (!actual) {
        ADD_FAILURE() << "no point";
        return;
    }
    EXPECT_NEAR(actual->x, expected.x, kTolerance);
    EXPECT_NEAR(actual->y, expected.y, kTolerance);
}

/// Reads and resolves `text` in one step.
std::optional<Vec2> pointFor(std::string_view text, std::optional<Vec2> lastPoint,
                             Vec2 pointer = kAnyPointer) {
    const std::optional<PointSpec> spec = parseCoordinate(text);
    if (!spec) {
        ADD_FAILURE() << '"' << text << "\" is not a coordinate";
        return std::nullopt;
    }
    return resolve(*spec, lastPoint, pointer);
}

// --- Reading: every form of ADR 0005 ------------------------------------

struct ValidCase {
    std::string_view text;
    PointSpec expected;
};

std::ostream& operator<<(std::ostream& out, const ValidCase& value) {
    return out << '"' << value.text << '"';
}

class ParseValid : public testing::TestWithParam<ValidCase> {};

TEST_P(ParseValid, ReadsTheExpectedSpec) {
    EXPECT_EQ(parseCoordinate(GetParam().text), GetParam().expected);
}

INSTANTIATE_TEST_SUITE_P(CoordinateInput, ParseValid,
                         testing::Values(
                             // One case per form.
                             ValidCase{"1,2", {Kind::Absolute, 1.0, 2.0}},
                             ValidCase{"@3,4", {Kind::Relative, 3.0, 4.0}},
                             ValidCase{"10<45", {Kind::PolarAbsolute, 10.0, 45.0}},
                             ValidCase{"@50<90", {Kind::PolarRelative, 50.0, 90.0}},
                             ValidCase{"25", {Kind::Distance, 25.0, 0.0}},
                             // Signs.
                             ValidCase{"-1.5,-2.25", {Kind::Absolute, -1.5, -2.25}},
                             ValidCase{"+1,+2", {Kind::Absolute, 1.0, 2.0}},
                             ValidCase{"@-3,4", {Kind::Relative, -3.0, 4.0}},
                             ValidCase{"-10<-30", {Kind::PolarAbsolute, -10.0, -30.0}},
                             ValidCase{"@5<-90", {Kind::PolarRelative, 5.0, -90.0}},
                             ValidCase{"-7", {Kind::Distance, -7.0, 0.0}},
                             // Decimals and exponents.
                             ValidCase{".5,5.", {Kind::Absolute, 0.5, 5.0}},
                             ValidCase{"1e3,2.5E-2", {Kind::Absolute, 1000.0, 0.025}},
                             ValidCase{"1.5e+2", {Kind::Distance, 150.0, 0.0}},
                             ValidCase{"@0,0", {Kind::Relative, 0.0, 0.0}},
                             // Leading and trailing white space.
                             ValidCase{"  1,2  ", {Kind::Absolute, 1.0, 2.0}},
                             ValidCase{"\t@1,2\n", {Kind::Relative, 1.0, 2.0}},
                             ValidCase{" 12.5 ", {Kind::Distance, 12.5, 0.0}},
                             // The comma always separates X and Y, whatever the system locale.
                             ValidCase{"1,5", {Kind::Absolute, 1.0, 5.0}}));

// --- Reading: text that is not a coordinate ----------------------------

class ParseInvalid : public testing::TestWithParam<std::string_view> {};

TEST_P(ParseInvalid, IsRejected) {
    EXPECT_EQ(parseCoordinate(GetParam()), std::nullopt);
}

INSTANTIATE_TEST_SUITE_P(CoordinateInput, ParseInvalid,
                         testing::Values(
                             // Required by the package.
                             "", "1,", "@", "1,2,3", "abc", "1;2", "<45",
                             // Missing or extra parts.
                             "   ", ",2", "10<", "@<45", "@5", "@@1,2", "1<2<3", "1,2<3", "1,2@",
                             // Spaces inside.
                             "1, 2", "1 ,2", "@ 1,2", "1 2",
                             // Malformed numbers.
                             ".", "1..2", "1.2.3", "--1", "+-1", "e5", "1e", "1e+", "1,5e", "1.5f",
                             // Numbers that std::strtod would accept but a CAD user never means.
                             "inf", "nan,1", "0x10", "1e999"));

// --- Resolution ----------------------------------------------------------

TEST(CoordinateInput, AbsoluteIgnoresLastPointAndPointer) {
    expectPoint(pointFor("3,4", std::nullopt), {3.0, 4.0});
    expectPoint(pointFor("3,4", Vec2{10.0, 10.0}), {3.0, 4.0});
}

TEST(CoordinateInput, RelativeAddsToTheLastPoint) {
    expectPoint(pointFor("@3,-4", Vec2{10.0, 10.0}), {13.0, 6.0});
}

TEST(CoordinateInput, RelativeNeedsALastPoint) {
    EXPECT_EQ(pointFor("@3,4", std::nullopt), std::nullopt);
}

TEST(CoordinateInput, PolarAbsoluteStartsFromTheOrigin) {
    expectPoint(pointFor("10<0", std::nullopt), {10.0, 0.0});
    expectPoint(pointFor("10<90", Vec2{5.0, 5.0}), {0.0, 10.0});
    expectPoint(pointFor("2<225", std::nullopt), {-std::numbers::sqrt2, -std::numbers::sqrt2});
}

TEST(CoordinateInput, PolarRelativeStartsFromTheLastPoint) {
    expectPoint(pointFor("@50<90", Vec2{50.0, 0.0}), {50.0, 50.0});
    expectPoint(pointFor("@10<-90", Vec2{1.0, 1.0}), {1.0, -9.0});
    expectPoint(pointFor("@-10<0", Vec2{1.0, 1.0}), {-9.0, 1.0});
}

TEST(CoordinateInput, PolarRelativeNeedsALastPoint) {
    EXPECT_EQ(pointFor("@50<90", std::nullopt), std::nullopt);
}

TEST(CoordinateInput, DistanceFollowsThePointerDirection) {
    expectPoint(pointFor("5", Vec2{1.0, 1.0}, Vec2{1.0, 11.0}), {1.0, 6.0});
    expectPoint(pointFor("10", Vec2{0.0, 0.0}, Vec2{3.0, 4.0}), {6.0, 8.0});
    // The pointer gives only the direction, not the length.
    expectPoint(pointFor("10", Vec2{0.0, 0.0}, Vec2{0.3, 0.4}), {6.0, 8.0});
}

TEST(CoordinateInput, NegativeDistanceGoesAgainstThePointer) {
    expectPoint(pointFor("-5", Vec2{0.0, 0.0}, Vec2{10.0, 0.0}), {-5.0, 0.0});
}

TEST(CoordinateInput, DistanceNeedsALastPoint) {
    EXPECT_EQ(pointFor("5", std::nullopt, Vec2{1.0, 1.0}), std::nullopt);
}

TEST(CoordinateInput, DistanceNeedsAPointerAwayFromTheLastPoint) {
    EXPECT_EQ(pointFor("5", Vec2{2.0, 3.0}, Vec2{2.0, 3.0}), std::nullopt);
}

// Step 3 of the manual test of stage 0.2: a closed square of side 50.
TEST(CoordinateInput, ManualTestSquare) {
    std::optional<Vec2> last = pointFor("0,0", std::nullopt);
    expectPoint(last, {0.0, 0.0});
    last = pointFor("@50,0", last);
    expectPoint(last, {50.0, 0.0});
    last = pointFor("@50<90", last);
    expectPoint(last, {50.0, 50.0});
    last = pointFor("0,50", last);
    expectPoint(last, {0.0, 50.0});
}

}  // namespace
}  // namespace tg::cmd
