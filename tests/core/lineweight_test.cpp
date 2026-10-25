// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/lineweight.hpp"

#include <gtest/gtest.h>

#include <limits>

namespace tg::core {
namespace {
TEST(Lineweight, AllowedValuesAndRoundTrips) {
    for (const int value : lineweight::kAllowed) {
        EXPECT_TRUE(isAllowedLineweight(value));
        EXPECT_EQ(nearestLineweight(value), value);
        EXPECT_EQ(parseLineweight(formatLineweight(value)), value);
    }
    for (const int value : {lineweight::kByLayer, lineweight::kByBlock, lineweight::kDefault}) {
        EXPECT_TRUE(isAllowedLineweight(value));
        EXPECT_EQ(parseLineweight(formatLineweight(value)), value);
    }
    for (const int value : {-4, 1, 212, std::numeric_limits<int>::max()}) {
        EXPECT_FALSE(isAllowedLineweight(value));
    }
}

TEST(Lineweight, NearestAndTies) {
    EXPECT_EQ(nearestLineweight(7), 5);
    EXPECT_EQ(nearestLineweight(8), 9);
    EXPECT_EQ(nearestLineweight(52), 53);
    EXPECT_EQ(nearestLineweight(180), 158);
    EXPECT_EQ(nearestLineweight(std::numeric_limits<int>::max()), 211);
}

TEST(Lineweight, TextAndRoundingBoundaries) {
    EXPECT_EQ(formatLineweight(0), "0");
    EXPECT_EQ(formatLineweight(25), "0.25");
    EXPECT_EQ(formatLineweight(211), "2.11");
    EXPECT_EQ(parseLineweight(" ByLayer "), lineweight::kByLayer);
    EXPECT_EQ(parseLineweight("BYBLOCK"), lineweight::kByBlock);
    EXPECT_EQ(parseLineweight("dEfAuLt"), lineweight::kDefault);
    EXPECT_EQ(parseLineweight(".25"), 25);
    EXPECT_EQ(parseLineweight("\t0.25mm\n"), 25);
    EXPECT_EQ(parseLineweight("0.25MM"), 25);
    EXPECT_EQ(parseLineweight("0.245"), 25);
    EXPECT_EQ(parseLineweight("0.255"), 25);
    EXPECT_EQ(parseLineweight("0.005"), 0);
    EXPECT_EQ(parseLineweight("2.115"), 211);
    EXPECT_EQ(parseLineweight("0.135"), 13);
    EXPECT_FALSE(parseLineweight("0.14"));
    EXPECT_FALSE(parseLineweight("0.244999"));
    EXPECT_FALSE(parseLineweight("0.255001"));
    EXPECT_FALSE(parseLineweight("2.115001"));
}

TEST(Lineweight, RejectsInvalidText) {
    for (const auto* text : {"", " ", "-0.01", "nan", "inf", "1e2", "0.25x", "mm", "0,25", ".",
                             "99999999999999999999999999999999999", "0.25mmx"}) {
        EXPECT_FALSE(parseLineweight(text)) << text;
    }
}
#ifndef NDEBUG
TEST(LineweightDeathTest, Preconditions) {
    EXPECT_DEATH((void)nearestLineweight(-1), "");
    EXPECT_DEATH((void)formatLineweight(1), "");
}
#endif
}  // namespace
}  // namespace tg::core
