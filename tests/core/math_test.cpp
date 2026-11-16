// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/math.hpp"

#include "solid_space/robust/elementary.hpp"
#include "solid_space/robust/trigonometry.hpp"
#include "tg/core/vec2.hpp"

#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace tg::core {
namespace {

std::uint64_t bits(double value) {
    return std::bit_cast<std::uint64_t>(value);
}

TEST(Math, TrigonometryMatchesRobustBitForBit) {
    namespace robust = solid_space::robust;
    for (const double angle :
         {-0.0, 0.0, 0x1p-50, -0.25, 0.5, kPi / 2.0, -kPi, kTwoPi, -30.0, 0x1p20}) {
        SCOPED_TRACE(angle);
        const auto actual = sinCos(angle);
        const auto expected = robust::sinCos(angle);
        EXPECT_EQ(bits(actual.sine), bits(expected.sine));
        EXPECT_EQ(bits(actual.cosine), bits(expected.cosine));
        EXPECT_EQ(bits(atan2(angle, -2.0)), bits(robust::atan2(angle, -2.0)));
        EXPECT_EQ(bits(atan(angle)), bits(atan2(angle, 1.0)));
    }
    const double angle = -2.0 * robust::kMaxTrigArgument;
    const auto reduced = sinCos(std::fmod(angle, kTwoPi));
    EXPECT_EQ(bits(sin(angle)), bits(reduced.sine));
    EXPECT_EQ(bits(cos(angle)), bits(reduced.cosine));
    EXPECT_EQ(sin(0.0), 0.0);
    EXPECT_EQ(cos(0.0), 1.0);
}

TEST(Math, ElementaryFunctionsMatchRobustBitForBit) {
    namespace robust = solid_space::robust;
    for (const double angle : {-0.0, 0.0, 0x1p-50, -0.25, 0.5, kPi / 4.0, -kPi, 30.0}) {
        SCOPED_TRACE(angle);
        EXPECT_EQ(bits(tan(angle)), bits(robust::tan(angle)));
    }
    for (const double value : {-1.0, -0.75, -0.25, 0.0, 0.25, 0.75, 1.0}) {
        SCOPED_TRACE(value);
        EXPECT_EQ(bits(acos(value)), bits(robust::acos(value)));
    }
    for (const auto [x, y] : std::array<std::array<double, 2>, 5>{
             {{0.0, 0.0},
              {3.0, 4.0},
              {-10.0, 2.0},
              {0x1p-500, 0x1p-501},
              {robust::kMaxHypotArgument, robust::kMaxHypotArgument}}}) {
        SCOPED_TRACE(x);
        SCOPED_TRACE(y);
        EXPECT_EQ(bits(hypot(x, y)), bits(robust::hypot(x, y)));
    }

    const double angle = -2.0 * robust::kMaxTrigArgument;
    EXPECT_EQ(bits(tan(angle)), bits(tan(std::fmod(angle, kTwoPi))));
    EXPECT_EQ(acos(1.0), 0.0);
    EXPECT_EQ(bits(acos(-1.0)), bits(kPi));
}

TEST(Math, HypotHandlesLimitsAndNonFiniteArguments) {
    namespace robust = solid_space::robust;
    const double infinity = std::numeric_limits<double>::infinity();
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double aboveLimit = std::nextafter(robust::kMaxHypotArgument, infinity);

    EXPECT_EQ(hypot(3.0, 4.0), 5.0);
    EXPECT_EQ(bits(hypot(-3.0, 0.0)), bits(3.0));
    EXPECT_EQ(bits(hypot(-3.0, 4.0)), bits(hypot(4.0, 3.0)));
    EXPECT_TRUE(std::isfinite(hypot(robust::kMaxHypotArgument, robust::kMaxHypotArgument)));
    EXPECT_EQ(hypot(aboveLimit, -0.0), aboveLimit);
    EXPECT_EQ(hypot(-0.0, aboveLimit), aboveLimit);
    EXPECT_TRUE(
        std::isinf(hypot(std::numeric_limits<double>::max(), std::numeric_limits<double>::max())));
    EXPECT_TRUE(std::isinf(hypot(infinity, nan)));
    EXPECT_TRUE(std::isnan(hypot(1.0, nan)));
    EXPECT_EQ(Vec2(3.0, 4.0).length(), 5.0);
}

TEST(Math, CardinalTrigonometrySnapsOnlyWithinTheBand) {
    struct Direction {
        double angle;
        double cosine;
        double sine;
    };

    const std::array cases{
        Direction{0.0, 1.0, 0.0},
        Direction{kPi / 2.0, 0.0, 1.0},
        Direction{kPi, -1.0, 0.0},
        Direction{3.0 * kPi / 2.0, 0.0, -1.0},
        Direction{-kPi / 2.0, 0.0, -1.0},
        Direction{kPi / 2.0 + kTwoPi, 0.0, 1.0},
        Direction{kPi / 2.0 - 0x1p-50, 0.0, 1.0},
    };
    for (const auto& [angle, cosine, sine] : cases) {
        SCOPED_TRACE(angle);
        const auto direction = cardinalSinCos(angle);
        EXPECT_EQ(bits(direction.cosine), bits(cosine));
        EXPECT_EQ(bits(direction.sine), bits(sine));
    }
    for (const double angle : {kPi / 4.0, kPi / 2.0 - 0x1p-30, kPi / 2.0 - 0x1p-46}) {
        const auto snapped = cardinalSinCos(angle);
        const auto unsnapped = sinCos(angle);
        EXPECT_EQ(bits(snapped.sine), bits(unsnapped.sine));
        EXPECT_EQ(bits(snapped.cosine), bits(unsnapped.cosine));
    }
}

TEST(Math, DegreeReductionPreservesQuarterTurnsAndCanonicalizesZero) {
    for (const double angle : {3600.0, 360.0, 0.0, -0.0, -3600.0, -1e-20}) {
        EXPECT_EQ(bits(reducedDegreesToRadians(angle)), bits(0.0));
    }
    for (const double angle : {3690.0, -270.0, 90.0}) {
        EXPECT_EQ(bits(reducedDegreesToRadians(angle)), bits(degreesToRadians(90.0)));
    }
    const auto direction = cardinalSinCos(reducedDegreesToRadians(3690.0));
    EXPECT_EQ(bits(direction.cosine), bits(0.0));
    EXPECT_EQ(bits(direction.sine), bits(1.0));
}

}  // namespace
}  // namespace tg::core
