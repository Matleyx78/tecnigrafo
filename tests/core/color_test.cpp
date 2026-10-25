// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/color.hpp"

#include <gtest/gtest.h>

#include <array>

namespace tg::core {
namespace {
constexpr auto kCompileTimeColor = Color::fromRgb({1, 2, 3});
static_assert(kCompileTimeColor == Color::fromRgb({1, 2, 3}));
static_assert(Color::byLayer() != Color::byBlock());

TEST(Color, KindsAndEquality) {
    EXPECT_EQ(Color::byLayer().kind(), Color::Kind::ByLayer);
    EXPECT_EQ(Color::byBlock().kind(), Color::Kind::ByBlock);
    EXPECT_EQ(Color::fromIndex(42).kind(), Color::Kind::Index);
    EXPECT_EQ(Color::fromIndex(42).index(), 42);
    EXPECT_EQ(kCompileTimeColor.kind(), Color::Kind::Rgb);
    EXPECT_EQ(kCompileTimeColor.rgb(), (Rgb{1, 2, 3}));
    EXPECT_NE(Color::fromIndex(1), Color::fromRgb({255, 0, 0}));
    EXPECT_NE(Color::fromIndex(1), Color::fromIndex(2));
    EXPECT_NE(kCompileTimeColor, Color::fromRgb({1, 2, 4}));
}

#ifndef NDEBUG
TEST(ColorDeathTest, Preconditions) {
    EXPECT_DEATH((void)Color::fromIndex(0), "");
    EXPECT_DEATH((void)Color::fromIndex(256), "");
    EXPECT_DEATH((void)Color::byLayer().index(), "");
    EXPECT_DEATH((void)Color::byBlock().rgb(), "");
    EXPECT_DEATH((void)aciToRgb(0), "");
    EXPECT_DEATH((void)aciToRgb(256), "");
}
#endif

TEST(Color, PaletteGroupsAndDistinctBaseColors) {
    EXPECT_EQ(aciToRgb(1), (Rgb{255, 0, 0}));
    EXPECT_EQ(aciToRgb(7), (Rgb{255, 255, 255}));
    EXPECT_EQ(aciToRgb(10), (Rgb{255, 0, 0}));
    EXPECT_EQ(aciToRgb(11), (Rgb{255, 127, 127}));
    EXPECT_EQ(aciToRgb(249), (Rgb{38, 19, 23}));
    EXPECT_EQ(aciToRgb(250), (Rgb{51, 51, 51}));
    EXPECT_EQ(aciToRgb(255), (Rgb{255, 255, 255}));
    for (int i = 1; i <= 9; ++i) {
        for (int j = i + 1; j <= 9; ++j) {
            EXPECT_NE(aciToRgb(i), aciToRgb(j));
        }
    }
}

TEST(Color, NearestIndexAndTies) {
    EXPECT_EQ(nearestAci({255, 255, 255}), 255);
    EXPECT_EQ(nearestAci({255, 0, 0}), 1);  // Indices 1 and 10 have the same RGB.
    EXPECT_EQ(nearestAci({0, 0, 0}), 18);   // Six darkest saturated hues tie.
    for (int index = 1; index <= 255; ++index) {
        const auto rgb = aciToRgb(index);
        int first = 1;
        while (first == 7 || aciToRgb(first) != rgb) {
            ++first;
        }
        EXPECT_EQ(nearestAci(rgb), first) << index;
    }
}

TEST(Color, TextRoundTrips) {
    EXPECT_EQ(parseColor(" BYLAYER "), Color::byLayer());
    EXPECT_EQ(parseColor("256"), Color::byLayer());
    EXPECT_EQ(parseColor("\tByBlock\n"), Color::byBlock());
    EXPECT_EQ(parseColor("0"), Color::byBlock());
    EXPECT_EQ(formatColor(Color::byLayer()), "bylayer");
    EXPECT_EQ(formatColor(Color::byBlock()), "byblock");
    constexpr std::array<std::string_view, 7> kNames{"RED",  "Yellow",  "green", "CYAN",
                                                     "blue", "Magenta", "WHITE"};
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        EXPECT_EQ(parseColor(kNames.at(i)), Color::fromIndex(static_cast<int>(i) + 1));
    }
    for (int index = 1; index <= 255; ++index) {
        const auto color = Color::fromIndex(index);
        EXPECT_EQ(formatColor(color), std::to_string(index));
        EXPECT_EQ(parseColor(formatColor(color)), color);
    }
    EXPECT_EQ(formatColor(Color::fromRgb({255, 128, 0})), "255,128,0");
    EXPECT_EQ(parseColor(" 255,128,0 "), Color::fromRgb({255, 128, 0}));
    EXPECT_EQ(parseColor("0,0,0"), Color::fromRgb({0, 0, 0}));
    EXPECT_EQ(parseColor("255,255,255"), Color::fromRgb({255, 255, 255}));
}

TEST(Color, RejectsInvalidText) {
    for (const auto* text : {"", " ", "0,0", "256,0,0", "300", "rosso", "-1", "1x", "1,2,3,4",
                             "1,,3", "1,2,", "-1,2,3", "9999999999999999"}) {
        EXPECT_FALSE(parseColor(text)) << text;
    }
}
}  // namespace
}  // namespace tg::core
