// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/number_format.hpp"

#include <gtest/gtest.h>

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <random>
#include <string>

namespace tg::script {
namespace {

TEST(FormatNumber, ShortestTextThatReadsBack) {
    EXPECT_EQ(formatNumber(0.1), "0.1");
    EXPECT_EQ(formatNumber(1.0 / 3.0), "0.3333333333333333");
    EXPECT_EQ(formatNumber(2.0 / 3.0), "0.6666666666666666");
    EXPECT_EQ(formatNumber(0.1 + 0.2), "0.30000000000000004");
    EXPECT_EQ(formatNumber(2.5), "2.5");
    EXPECT_EQ(formatNumber(-2.5), "-2.5");
    EXPECT_EQ(formatNumber(123.456), "123.456");
    EXPECT_EQ(formatNumber(std::sqrt(50.0)), "7.0710678118654755");
}

TEST(FormatNumber, IntegersHaveNoDecimalPoint) {
    EXPECT_EQ(formatNumber(1.0), "1");
    EXPECT_EQ(formatNumber(42.0), "42");
    EXPECT_EQ(formatNumber(-7.0), "-7");
    EXPECT_EQ(formatNumber(100.0), "100");
    EXPECT_EQ(formatNumber(1e15), "1000000000000000");
    EXPECT_EQ(formatNumber(9007199254740992.0), "9007199254740992");  // 2^53
}

TEST(FormatNumber, ZeroOfEitherSignIsZero) {
    EXPECT_EQ(formatNumber(0.0), "0");
    EXPECT_EQ(formatNumber(-0.0), "0");
}

TEST(FormatNumber, LargeAndSmallNumbersUseAnExponent) {
    EXPECT_EQ(formatNumber(1e300), "1e+300");
    EXPECT_EQ(formatNumber(-1e300), "-1e+300");
    EXPECT_EQ(formatNumber(1e-300), "1e-300");
    EXPECT_EQ(formatNumber(1e16), "1e+16");
    EXPECT_EQ(formatNumber(1.5e17), "1.5e+17");
    EXPECT_EQ(formatNumber(1e-5), "1e-05");
    EXPECT_EQ(formatNumber(1.5e-7), "1.5e-07");
    EXPECT_EQ(formatNumber(3.061616997868383e-15), "3.061616997868383e-15");
    EXPECT_EQ(formatNumber(std::numeric_limits<double>::max()), "1.7976931348623157e+308");
}

TEST(FormatNumber, FixedNotationFromTenToTheMinusFour) {
    EXPECT_EQ(formatNumber(0.0001), "0.0001");
    EXPECT_EQ(formatNumber(0.00012), "0.00012");
    EXPECT_EQ(formatNumber(-0.0005), "-0.0005");
}

TEST(FormatNumber, NonFiniteValues) {
    EXPECT_EQ(formatNumber(std::numeric_limits<double>::infinity()), "inf");
    EXPECT_EQ(formatNumber(-std::numeric_limits<double>::infinity()), "-inf");
    EXPECT_EQ(formatNumber(std::numeric_limits<double>::quiet_NaN()), "nan");
}

/// Random bits reproducible on every platform: std::mt19937_64 is fully
/// specified by the standard, its distributions are not.
std::mt19937_64 randomBits(std::uint64_t seed) {
    return std::mt19937_64{seed};
}

TEST(FormatNumber, EveryValueReadsBackExactly) {
    std::mt19937_64 random = randomBits(20261018);
    int checked = 0;
    while (checked < 2000) {
        const auto value = std::bit_cast<double>(random());
        // Subnormal numbers are left out: no drawing reaches 1e-308, and a
        // stream may refuse to read them back on some platforms.
        if (!std::isnormal(value)) {
            continue;
        }
        ++checked;
        const std::string text = formatNumber(value);
        const std::optional<double> back = parseNumber(text);
        if (!back) {
            ADD_FAILURE() << "does not read back: " << text;
            continue;
        }
        EXPECT_EQ(std::bit_cast<std::uint64_t>(*back), std::bit_cast<std::uint64_t>(value)) << text;
    }
}

TEST(FormatNumber, PointIsTwoNumbersAndAComma) {
    EXPECT_EQ(formatPoint({1.5, -2.0}), "1.5,-2");
    EXPECT_EQ(formatPoint({0.0, -0.0}), "0,0");
}

TEST(ParseNumber, AcceptsTheSyntaxOfCoordinates) {
    EXPECT_EQ(parseNumber("1"), 1.0);
    EXPECT_EQ(parseNumber("-1.5"), -1.5);
    EXPECT_EQ(parseNumber("+2"), 2.0);
    EXPECT_EQ(parseNumber(".5"), 0.5);
    EXPECT_EQ(parseNumber("5."), 5.0);
    EXPECT_EQ(parseNumber("1e3"), 1000.0);
    EXPECT_EQ(parseNumber("1E-3"), 0.001);
    EXPECT_EQ(parseNumber("2.5e+2"), 250.0);
}

TEST(ParseNumber, RejectsAnythingElse) {
    for (const char* text : {"", "-", ".", "abc", "1,5", " 1", "1 ", "inf", "nan", "0x10", "1e",
                             "1e+", "--1", "1.2.3", "1e400"}) {
        EXPECT_FALSE(parseNumber(text).has_value()) << text;
    }
}

TEST(ParsePoint, ReadsTwoNumbersSeparatedByAComma) {
    EXPECT_EQ(parsePoint("1.5,-2"), (core::Vec2{1.5, -2.0}));
    EXPECT_EQ(parsePoint("0,0"), (core::Vec2{0.0, 0.0}));
    for (const char* text : {"", "1", "1,", ",1", "1, 2", "1,2,3", "1;2", "a,b"}) {
        EXPECT_FALSE(parsePoint(text).has_value()) << text;
    }
}

}  // namespace
}  // namespace tg::script
