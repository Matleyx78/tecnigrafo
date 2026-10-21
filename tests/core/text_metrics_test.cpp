// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/text_metrics.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace tg::core {
namespace {
TEST(TextMetrics, CapitalHeightUnitsAndSigns) {
    // Independent table fields from Liberation Sans 2.1.5, OS/2 and hhea.
    EXPECT_DOUBLE_EQ(fontMetrics().ascent, 1854.0 / 1409.0);
    EXPECT_DOUBLE_EQ(fontMetrics().descent, -434.0 / 1409.0);
    EXPECT_GT(fontMetrics().ascent, 1.0);
    EXPECT_LT(fontMetrics().descent, 0.0);
    EXPECT_DOUBLE_EQ(advanceWidth(U'A'), 1366.0 / 1409.0);
}

TEST(TextMetrics, EmptyAndAdditiveWithoutKerning) {
    EXPECT_DOUBLE_EQ(textWidth(U""), 0.0);
    EXPECT_DOUBLE_EQ(textWidth(U"AV"), advanceWidth(U'A') + advanceWidth(U'V'));
    EXPECT_DOUBLE_EQ(textWidth(U"Caffè"), textWidth(U"Caf") + textWidth(U"fè"));
    EXPECT_DOUBLE_EQ(textWidth(U"%%c"), 2 * advanceWidth(U'%') + advanceWidth(U'c'));
}

TEST(TextMetrics, SubstitutesAndMissingGlyph) {
    EXPECT_EQ(fontCharacter(U'⌀'), U'Ø');
    EXPECT_EQ(fontCharacter(U'∅'), U'Ø');
    EXPECT_EQ(fontCharacter(U'A'), U'A');
    EXPECT_EQ(fontCharacter(U'中'), U'中');
    EXPECT_DOUBLE_EQ(advanceWidth(U'⌀'), advanceWidth(U'Ø'));
    EXPECT_DOUBLE_EQ(advanceWidth(U'∅'), advanceWidth(U'Ø'));
    EXPECT_DOUBLE_EQ(advanceWidth(U'中'), 1536.0 / 1409.0);
    EXPECT_DOUBLE_EQ(advanceWidth(U'😀'), advanceWidth(U'中'));
    EXPECT_DOUBLE_EQ(advanceWidth(static_cast<char32_t>(0x110000)), advanceWidth(U'中'));
}

TEST(TextMetrics, DecodesAllUtf8LengthsAndEmbeddedNull) {
    EXPECT_EQ(decodeUtf8(""), U"");
    EXPECT_EQ(decodeUtf8("AéØ⌀中😀"), U"AéØ⌀中😀");
    EXPECT_EQ(decodeUtf8(std::string_view{"A\0B", 3}), std::u32string(U"A\0B", 3));
    EXPECT_EQ(decodeUtf8("\xF4\x8F\xBF\xBF"), U"\U0010FFFF");
}

TEST(TextMetrics, ReplacesIllFormedSubsequencesAndPreservesFollowingText) {
    EXPECT_EQ(decodeUtf8("\xC3"), U"\uFFFD");
    EXPECT_EQ(decodeUtf8("\xE2\x82"), U"\uFFFD");
    EXPECT_EQ(decodeUtf8("\xF0\x9F\x98"), U"\uFFFD");
    EXPECT_EQ(decodeUtf8("\xE2\x82Z"), U"\uFFFDZ");
    EXPECT_EQ(decodeUtf8("\x80"), U"\uFFFD");
    EXPECT_EQ(decodeUtf8("\xC0\xAF"), U"\uFFFD\uFFFD");            // overlong
    EXPECT_EQ(decodeUtf8("\xED\xA0\x80"), U"\uFFFD\uFFFD\uFFFD");  // surrogate
    EXPECT_EQ(decodeUtf8("\xF4\x90\x80\x80"), U"\uFFFD\uFFFD\uFFFD\uFFFD");
    EXPECT_EQ(decodeUtf8("\xFF"
                         "A"),
              U"\uFFFDA");
}
}  // namespace
}  // namespace tg::core
