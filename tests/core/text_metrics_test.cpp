// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/text_metrics.hpp"

#include <gtest/gtest.h>

#include <array>
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

// Independent hmtx advances of the four Liberation Sans 2.1.5 files; each face
// has its own cap height (1409 in all four).
TEST(TextMetrics, EachFaceHasItsOwnAdvances) {
    struct Expected {
        FontFace face;
        double capitalA;
        double smallA;
        double space;
        double missing;
    };

    constexpr std::array<Expected, 4> kFaces{{
        {FontFace::Regular, 1366.0, 1139.0, 569.0, 1536.0},
        {FontFace::Bold, 1479.0, 1139.0, 569.0, 1536.0},
        {FontFace::Italic, 1366.0, 1139.0, 569.0, 1536.0},
        {FontFace::BoldItalic, 1479.0, 1139.0, 569.0, 1536.0},
    }};
    for (const Expected& expected : kFaces) {
        const auto face = expected.face;
        EXPECT_DOUBLE_EQ(advanceWidth(U'A', face), expected.capitalA / 1409.0);
        EXPECT_DOUBLE_EQ(advanceWidth(U'a', face), expected.smallA / 1409.0);
        EXPECT_DOUBLE_EQ(advanceWidth(U' ', face), expected.space / 1409.0);
        EXPECT_DOUBLE_EQ(advanceWidth(U'☃', face), expected.missing / 1409.0);
        EXPECT_DOUBLE_EQ(fontMetrics(face).ascent, 1854.0 / 1409.0);
        EXPECT_DOUBLE_EQ(fontMetrics(face).descent, -434.0 / 1409.0);
        EXPECT_EQ(fontCharacter(U'⌀', face), U'Ø');
        EXPECT_DOUBLE_EQ(advanceWidth(U'⌀', face), advanceWidth(U'Ø', face));
        EXPECT_DOUBLE_EQ(
            textWidth(U"Aa ", face),
            advanceWidth(U'A', face) + advanceWidth(U'a', face) + advanceWidth(U' ', face));
    }
}

TEST(TextMetrics, RegularIsTheDefaultAndUnchanged) {
    EXPECT_EQ(&fontMetrics(), &fontMetrics(FontFace::Regular));
    for (const char32_t code : std::u32string_view{U"Aa ☃⌀ď"}) {
        EXPECT_EQ(advanceWidth(code), advanceWidth(code, FontFace::Regular));
    }
    EXPECT_DOUBLE_EQ(advanceWidth(U'ď'), 1259.0 / 1409.0);
    EXPECT_DOUBLE_EQ(advanceWidth(U'ď', FontFace::Bold), 1472.0 / 1409.0);
    EXPECT_EQ(textWidth(U"Normale"), textWidth(U"Normale", FontFace::Regular));
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
