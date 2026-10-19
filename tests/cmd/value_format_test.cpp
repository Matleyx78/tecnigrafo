// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/value_format.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <locale>
#include <memory>
#include <string>

namespace tg::cmd {
namespace {

TEST(FormatLength, UsesFourDecimalPlaces) {
    EXPECT_EQ(formatLength(2.5), "2.5000");
    EXPECT_EQ(formatLength(50.0), "50.0000");
    EXPECT_EQ(formatLength(-12.25), "-12.2500");
    EXPECT_EQ(formatLength(0.0001), "0.0001");
}

TEST(FormatLength, RoundsToFourDecimalPlaces) {
    EXPECT_EQ(formatLength(1.23454), "1.2345");
    EXPECT_EQ(formatLength(1.23456), "1.2346");
    EXPECT_EQ(formatLength(-1.23456), "-1.2346");
    EXPECT_EQ(formatLength(9.99996), "10.0000");
}

TEST(FormatLength, RoundedZeroHasNoMinusSign) {
    EXPECT_EQ(formatLength(0.0), "0.0000");
    EXPECT_EQ(formatLength(-0.0), "0.0000");
    EXPECT_EQ(formatLength(-0.00004), "0.0000");
    EXPECT_EQ(formatLength(-0.00006), "-0.0001");
}

TEST(FormatAngle, ConvertsRadiansToDegreesWithoutTrailingZeros) {
    EXPECT_EQ(formatAngle(0.0), "0");
    EXPECT_EQ(formatAngle(core::kPi / 2.0), "90");
    EXPECT_EQ(formatAngle(core::kPi), "180");
    EXPECT_EQ(formatAngle(core::degreesToRadians(37.5)), "37.5");
    EXPECT_EQ(formatAngle(core::degreesToRadians(-45.25)), "-45.25");
    EXPECT_EQ(formatAngle(core::degreesToRadians(450.0)), "450");
}

TEST(FormatAngle, RoundsToAtMostFourDecimals) {
    EXPECT_EQ(formatAngle(core::degreesToRadians(12.34564)), "12.3456");
    EXPECT_EQ(formatAngle(core::degreesToRadians(12.34566)), "12.3457");
    EXPECT_EQ(formatAngle(core::degreesToRadians(-12.34566)), "-12.3457");
    EXPECT_EQ(formatAngle(core::degreesToRadians(89.99996)), "90");
    EXPECT_EQ(formatAngle(-0.0), "0");
    EXPECT_EQ(formatAngle(core::degreesToRadians(-0.00004)), "0");
    EXPECT_EQ(formatAngle(core::degreesToRadians(-0.00006)), "-0.0001");
}

class CommaPunctuation final : public std::numpunct<char> {
protected:
    [[nodiscard]] char do_decimal_point() const override { return ','; }

    [[nodiscard]] char do_thousands_sep() const override { return '.'; }

    [[nodiscard]] std::string do_grouping() const override { return "\3"; }
};

TEST(ValueFormat, IgnoresTheSystemLocale) {
    const std::locale previous;
    // std::locale owns the facet, whose lifetime extends through both calls.
    auto punctuation = std::make_unique<CommaPunctuation>();
    std::locale::global(std::locale{previous, punctuation.release()});
    const std::string length = formatLength(1234.5);
    const std::string angle = formatAngle(core::degreesToRadians(1234.5));
    std::locale::global(previous);

    EXPECT_EQ(length, "1234.5000");
    EXPECT_EQ(angle, "1234.5");
}

}  // namespace
}  // namespace tg::cmd
