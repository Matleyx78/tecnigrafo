// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/mtext_format.hpp"

#include "mtext_layout.hpp"
#include "tg/core/color.hpp"
#include "tg/core/math.hpp"
#include "tg/core/text_metrics.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// docs/tappe/0.8.1/PT7-codici-di-formato.md: the pieces of an MTEXT value.

namespace tg::core {
namespace {

constexpr double kEntityHeight = 2.5;

MTextFormat base() {
    MTextFormat format;
    format.height = kEntityHeight;
    return format;
}

std::vector<MTextParagraph> read(std::string_view value) {
    return formattedParagraphs(value, base());
}

/// The single paragraph of `value`.
MTextParagraph one(std::string_view value) {
    std::vector<MTextParagraph> paragraphs = read(value);
    EXPECT_EQ(paragraphs.size(), 1U) << value;
    return paragraphs.empty() ? MTextParagraph{} : paragraphs.front();
}

std::vector<MTextUnit> units(std::initializer_list<std::array<std::size_t, 2>> ranges) {
    std::vector<MTextUnit> result;
    for (const auto& [begin, end] : ranges) {
        result.push_back(MTextUnit{.sourceBegin = begin, .sourceEnd = end});
    }
    return result;
}

/// Units of one byte each over [begin, end).
std::vector<MTextUnit> byteUnits(std::size_t begin, std::size_t end) {
    std::vector<MTextUnit> result;
    for (std::size_t i = begin; i < end; ++i) {
        result.push_back(MTextUnit{.sourceBegin = i, .sourceEnd = i + 1});
    }
    return result;
}

// ADR 0054, point 1, one row of the table at a time.

TEST(MTextFormat, PlainValueIsOneRegularPiecePerParagraph) {
    const std::vector<MTextParagraph> paragraphs = read("Normale\\Pseconda");
    ASSERT_EQ(paragraphs.size(), 2U);
    ASSERT_EQ(paragraphs[0].size(), 1U);
    ASSERT_EQ(paragraphs[1].size(), 1U);
    const MTextRun& first = paragraphs[0][0];
    EXPECT_EQ(first.text, U"Normale");
    EXPECT_EQ(first.format, base());
    EXPECT_FALSE(first.format.bold);
    EXPECT_FALSE(first.format.italic);
    EXPECT_FALSE(first.stack.has_value());
    EXPECT_EQ(first.sourceBegin, 0U);
    EXPECT_EQ(first.sourceEnd, 7U);
    EXPECT_EQ(first.units, byteUnits(0, 7));
    const MTextRun& second = paragraphs[1][0];
    EXPECT_EQ(second.text, U"seconda");
    EXPECT_EQ(second.format, base());
    EXPECT_EQ(second.sourceBegin, 9U);  // after "\P"
    EXPECT_EQ(second.sourceEnd, 16U);
    EXPECT_EQ(second.units, byteUnits(9, 16));
}

TEST(MTextFormat, NothingVisibleHasNoPieces) {
    const std::vector<MTextParagraph> empty = read("");
    ASSERT_EQ(empty.size(), 1U);
    EXPECT_TRUE(empty[0].empty());
    EXPECT_TRUE(one("{\\L}").empty());
    EXPECT_TRUE(one("\\A1;\\H3;").empty());
    const std::vector<MTextParagraph> breaks = read(R"(A\P\PB\P)");
    ASSERT_EQ(breaks.size(), 4U);
    EXPECT_EQ(breaks[0].size(), 1U);
    EXPECT_TRUE(breaks[1].empty());
    EXPECT_EQ(breaks[2].size(), 1U);
    EXPECT_TRUE(breaks[3].empty());
}

TEST(MTextFormat, FontCodeSetsBoldAndItalicAndIgnoresTheName) {
    const MTextParagraph bold = one("N{\\fArial|b1|i0|c0|p34;G}n");
    ASSERT_EQ(bold.size(), 3U);
    EXPECT_EQ(bold[0].text, U"N");
    EXPECT_EQ(bold[1].text, U"G");
    EXPECT_TRUE(bold[1].format.bold);
    EXPECT_FALSE(bold[1].format.italic);
    EXPECT_EQ(bold[2].format, base());

    const MTextParagraph italic = one("{\\fTimes New Roman|b0|i1|c0|p18;I}");
    ASSERT_EQ(italic.size(), 1U);
    EXPECT_FALSE(italic[0].format.bold);
    EXPECT_TRUE(italic[0].format.italic);

    const MTextParagraph both = one("{\\fNoSuchFont|b1|i1;X}");
    ASSERT_EQ(both.size(), 1U);
    EXPECT_TRUE(both[0].format.bold);
    EXPECT_TRUE(both[0].format.italic);

    // `\F` (a shape font) and a `\f` without flags give Regular.
    const MTextParagraph shape = one(R"({\fArial|b1;A\Fromans.shx|c0;B\fArial|i1;C\fArial;D})");
    ASSERT_EQ(shape.size(), 4U);
    EXPECT_TRUE(shape[0].format.bold);
    EXPECT_FALSE(shape[1].format.bold);
    EXPECT_FALSE(shape[1].format.italic);
    EXPECT_FALSE(shape[2].format.bold);
    EXPECT_TRUE(shape[2].format.italic);
    EXPECT_EQ(shape[3].format, base());

    // Two names, the same variant: one piece.
    const MTextParagraph names = one("\\fArial|b1;A\\fCalibri|b1|i0;B");
    ASSERT_EQ(names.size(), 1U);
    EXPECT_EQ(names[0].text, U"AB");
}

TEST(MTextFormat, AbsoluteAndRelativeHeight) {
    const MTextParagraph absolute = one("A{\\H3.5;B}C");
    ASSERT_EQ(absolute.size(), 3U);
    EXPECT_EQ(absolute[0].format.height, kEntityHeight);
    EXPECT_EQ(absolute[1].format.height, 3.5);
    EXPECT_EQ(absolute[2].format.height, kEntityHeight);

    const MTextParagraph relative = one("A{\\H2x;B}C");
    ASSERT_EQ(relative.size(), 3U);
    EXPECT_EQ(relative[1].format.height, 2 * kEntityHeight);
    EXPECT_EQ(relative[2].format.height, kEntityHeight);
}

TEST(MTextFormat, RelativeHeightInsideAbsoluteHeight) {
    const MTextParagraph pieces = one("{\\H3;A{\\H2x;B}C}D");
    ASSERT_EQ(pieces.size(), 4U);
    EXPECT_EQ(pieces[0].format.height, 3.0);
    EXPECT_EQ(pieces[1].format.height, 6.0);
    EXPECT_EQ(pieces[2].format.height, 3.0);
    EXPECT_EQ(pieces[3].format.height, kEntityHeight);
}

TEST(MTextFormat, WidthObliqueTracking) {
    const MTextParagraph pieces = one(R"({\W0.8;W}{\Q15;Q}{\T1.5;T})");
    ASSERT_EQ(pieces.size(), 3U);
    EXPECT_EQ(pieces[0].format.widthFactor, 0.8);
    EXPECT_EQ(pieces[0].format.oblique, 0.0);
    EXPECT_EQ(pieces[1].format.oblique, degreesToRadians(15.0));
    EXPECT_EQ(pieces[1].format.widthFactor, 1.0);
    EXPECT_EQ(pieces[2].format.tracking, 1.5);
    EXPECT_EQ(pieces[2].format.oblique, 0.0);
}

TEST(MTextFormat, IndexAndTrueColour) {
    const MTextParagraph pieces = one(R"({\C1;R}{\c16744448;B}{\C0;K}{\C256;L}N)");
    ASSERT_EQ(pieces.size(), 5U);
    EXPECT_EQ(pieces[0].format.color, Color::fromIndex(1));
    // BGR, as AutoCAD: 16744448 is FF8000, that is RGB 0080FF.
    EXPECT_EQ(pieces[1].format.color, Color::fromRgb(Rgb{.r = 0x00, .g = 0x80, .b = 0xFF}));
    EXPECT_EQ(pieces[2].format.color, Color::byBlock());
    EXPECT_EQ(pieces[3].format.color, Color::byLayer());
    EXPECT_EQ(pieces[4].format.color, std::nullopt);
}

TEST(MTextFormat, UnderlineOverlineStrike) {
    const MTextParagraph pieces = one(R"(\LU\l\OO\o\KK\kN{\L\O\KG}n)");
    ASSERT_EQ(pieces.size(), 6U);
    EXPECT_TRUE(pieces[0].format.underline);
    EXPECT_FALSE(pieces[0].format.overline);
    EXPECT_TRUE(pieces[1].format.overline);
    EXPECT_FALSE(pieces[1].format.underline);
    EXPECT_TRUE(pieces[2].format.strike);
    EXPECT_EQ(pieces[3].text, U"N");
    EXPECT_EQ(pieces[3].format, base());
    // The end of the group ends all three.
    EXPECT_TRUE(pieces[4].format.underline);
    EXPECT_TRUE(pieces[4].format.overline);
    EXPECT_TRUE(pieces[4].format.strike);
    EXPECT_EQ(pieces[5].format, base());
}

TEST(MTextFormat, ThreeKindsOfStackedFraction) {
    const MTextParagraph pieces = one(R"(\S1/2;\S3#4;\S5^6;)");
    ASSERT_EQ(pieces.size(), 3U);
    const std::array<MTextStack, 3> expected{
        MTextStack{.top = U"1", .bottom = U"2", .kind = StackKind::Horizontal},
        MTextStack{.top = U"3", .bottom = U"4", .kind = StackKind::Diagonal},
        MTextStack{.top = U"5", .bottom = U"6", .kind = StackKind::Tolerance}};
    for (std::size_t i = 0; i < pieces.size(); ++i) {
        EXPECT_TRUE(pieces[i].text.empty());
        EXPECT_EQ(pieces[i].stack, expected.at(i));
        EXPECT_EQ(pieces[i].format, base());
        EXPECT_EQ(pieces[i].units, units({{6 * i, (6 * i) + 6}}));
        EXPECT_EQ(pieces[i].sourceBegin, 6 * i);
        EXPECT_EQ(pieces[i].sourceEnd, (6 * i) + 6);
    }
}

TEST(MTextFormat, ToleranceAsAutoCadSavesIt) {
    const MTextParagraph pieces = one("\\S1^ 2;");
    ASSERT_EQ(pieces.size(), 1U);
    EXPECT_EQ(pieces[0].stack,
              (MTextStack{.top = U"1", .bottom = U"2", .kind = StackKind::Tolerance}));
    EXPECT_EQ(pieces[0].units, units({{0, 7}}));
}

TEST(MTextFormat, StackEscapesAndSeparators) {
    const MTextParagraph escapes = one(R"(\S%%c\U+00B1/\\\~2;)");
    ASSERT_EQ(escapes.size(), 1U);
    EXPECT_EQ(escapes[0].stack,
              (MTextStack{.top = U"⌀±", .bottom = U"\\\u00A02", .kind = StackKind::Horizontal}));
    // Only the first separator splits; a backslash keeps the next character.
    const MTextParagraph second = one(R"(\S1/2/3;\S4\/5#6;)");
    ASSERT_EQ(second.size(), 2U);
    EXPECT_EQ(second[0].stack,
              (MTextStack{.top = U"1", .bottom = U"2/3", .kind = StackKind::Horizontal}));
    EXPECT_EQ(second[1].stack,
              (MTextStack{.top = U"4\\/5", .bottom = U"6", .kind = StackKind::Diagonal}));
    // Without a separator, which AutoCAD does not write: all on top.
    const MTextParagraph none = one("\\S12;");
    ASSERT_EQ(none.size(), 1U);
    EXPECT_EQ(none[0].stack,
              (MTextStack{.top = U"12", .bottom = U"", .kind = StackKind::Horizontal}));
}

TEST(MTextFormat, StackIsOnePieceAndOneUnitWithAllItsBytes) {
    const MTextParagraph pieces = one("x\\S1/2;y");
    ASSERT_EQ(pieces.size(), 3U);
    EXPECT_EQ(pieces[0].text, U"x");
    EXPECT_EQ(pieces[0].units, units({{0, 1}}));
    EXPECT_EQ(pieces[1].units, units({{1, 7}}));
    EXPECT_EQ(pieces[1].sourceBegin, 1U);
    EXPECT_EQ(pieces[1].sourceEnd, 7U);
    EXPECT_EQ(pieces[2].text, U"y");
    EXPECT_EQ(pieces[2].units, units({{7, 8}}));

    // The codes that open a stack are in its piece; its format is the
    // format of the text around it.
    const MTextParagraph coloured = one("a{\\C1;\\S1/2;}b");
    ASSERT_EQ(coloured.size(), 3U);
    EXPECT_EQ(coloured[1].sourceBegin, 1U);
    EXPECT_EQ(coloured[1].units, units({{6, 12}}));
    EXPECT_EQ(coloured[1].format.color, Color::fromIndex(1));
    EXPECT_EQ(coloured[2].sourceBegin, 12U);  // the '}' opens "b"
}

// Point 3: the bytes of the pieces and of their units.

TEST(MTextFormat, HiddenCodeBetweenTwoCharactersOfOnePiece) {
    const MTextParagraph pieces = one("A\\A1;B");
    ASSERT_EQ(pieces.size(), 1U);
    EXPECT_EQ(pieces[0].text, U"AB");
    EXPECT_EQ(pieces[0].sourceBegin, 0U);
    EXPECT_EQ(pieces[0].sourceEnd, 6U);
    EXPECT_EQ(pieces[0].units, units({{0, 1}, {5, 6}}));
}

TEST(MTextFormat, UnitsOfMultiByteCharacterAndUnicodeCode) {
    const MTextParagraph pieces = one("é\\U+0042");
    ASSERT_EQ(pieces.size(), 1U);
    EXPECT_EQ(pieces[0].text, U"éB");
    EXPECT_EQ(pieces[0].units, units({{0, 2}, {2, 9}}));
    EXPECT_EQ(pieces[0].sourceEnd, 9U);
}

TEST(MTextFormat, UnitsOfEscapes) {
    const MTextParagraph pieces = one(R"(\\\{\}\~%%c%%%a)");
    ASSERT_EQ(pieces.size(), 1U);
    EXPECT_EQ(pieces[0].text, U"\\{}\u00A0⌀%a");
    EXPECT_EQ(pieces[0].units,
              units({{0, 2}, {2, 4}, {4, 6}, {6, 8}, {8, 11}, {11, 14}, {14, 15}}));
}

TEST(MTextFormat, OpeningCodesBelongToThePiece) {
    const MTextParagraph pieces = one("A{\\H2;B}C");
    ASSERT_EQ(pieces.size(), 3U);
    EXPECT_EQ(pieces[0].sourceBegin, 0U);
    EXPECT_EQ(pieces[0].sourceEnd, 1U);
    EXPECT_EQ(pieces[1].sourceBegin, 1U);
    EXPECT_EQ(pieces[1].sourceEnd, 7U);
    EXPECT_EQ(pieces[1].units, units({{6, 7}}));
    EXPECT_EQ(pieces[2].sourceBegin, 7U);
    EXPECT_EQ(pieces[2].sourceEnd, 9U);
    EXPECT_EQ(pieces[2].units, units({{8, 9}}));
}

TEST(MTextFormat, EqualAndContiguousPiecesMerge) {
    const MTextParagraph group = one("A{\\H2;}B");
    ASSERT_EQ(group.size(), 1U);
    EXPECT_EQ(group[0].units, units({{0, 1}, {7, 8}}));
    const MTextParagraph same = one("{\\C1;A}{\\C1;B}");
    ASSERT_EQ(same.size(), 1U);
    EXPECT_EQ(same[0].text, U"AB");
    EXPECT_EQ(same[0].units, units({{5, 6}, {12, 13}}));
    // A stack never merges, not even with another stack.
    EXPECT_EQ(one("\\S1/2;\\S1/2;").size(), 2U);
}

TEST(MTextFormat, FormatLastsAcrossParagraphs) {
    const std::vector<MTextParagraph> paragraphs = read("{\\H2;A\\PB}C");
    ASSERT_EQ(paragraphs.size(), 2U);
    ASSERT_EQ(paragraphs[1].size(), 2U);
    EXPECT_EQ(paragraphs[1][0].format.height, 2.0);
    EXPECT_EQ(paragraphs[1][0].sourceBegin, 8U);  // after "\P"
    EXPECT_EQ(paragraphs[1][1].format.height, kEntityHeight);
    EXPECT_EQ(paragraphs[1][1].sourceBegin, 9U);
}

// The rules of the stage: malformed codes and arguments out of domain.

TEST(MTextFormat, UnknownOrUnterminatedCodeIsText) {
    const MTextParagraph unknown = one("\\Zx;");
    ASSERT_EQ(unknown.size(), 1U);
    EXPECT_EQ(unknown[0].text, U"\\Zx;");
    EXPECT_EQ(unknown[0].units, byteUnits(0, 4));
    const MTextParagraph unterminated = one("\\H2");
    ASSERT_EQ(unterminated.size(), 1U);
    EXPECT_EQ(unterminated[0].text, U"\\H2");
    EXPECT_EQ(unterminated[0].format, base());
    EXPECT_EQ(unterminated[0].units, byteUnits(0, 3));
    // A `\P` comes before the ';': the code is text and the paragraph breaks.
    const std::vector<MTextParagraph> beforeBreak = read("\\H2\\P;");
    ASSERT_EQ(beforeBreak.size(), 2U);
    EXPECT_EQ(beforeBreak[0][0].text, U"\\H2");
    EXPECT_EQ(beforeBreak[1][0].text, U";");
}

TEST(MTextFormat, ArgumentOutOfDomainIsHiddenWithoutEffect) {
    for (const std::string_view code :
         {"\\Habc;", "\\H0;", "\\H0x;", "\\H-2;", "\\Hinf;", "\\Hnanx;", "\\W-1;", "\\W0;", "\\T0;",
          "\\Tabc;", "\\Qinf;", "\\Qnan;", "\\C257;", "\\C-1;", "\\C1.5;", "\\c16777216;", "\\c-1;",
          "\\H1e999;"}) {
        std::string value = "A";
        value += code;
        value += "B";
        const MTextParagraph pieces = one(value);
        // The format after the code is the format before it, so the two
        // characters are one piece; the code is between their units.
        ASSERT_EQ(pieces.size(), 1U) << value;
        EXPECT_EQ(pieces[0].text, U"AB") << value;
        EXPECT_EQ(pieces[0].format, base()) << value;
        EXPECT_EQ(pieces[0].units, units({{0, 1}, {value.size() - 1, value.size()}})) << value;
    }
    // Out of domain inside a format: the format before it stays.
    const MTextParagraph kept = one(R"({\H3;\W2;\T1.5;A\H0;\W-1;\T0;B})");
    ASSERT_EQ(kept.size(), 1U);
    EXPECT_EQ(kept[0].format.height, 3.0);
    EXPECT_EQ(kept[0].format.widthFactor, 2.0);
    EXPECT_EQ(kept[0].format.tracking, 1.5);
    // A relative height whose result is not finite has no effect either.
    const MTextParagraph huge = one("{\\H1e300;A\\H1e300x;B}");
    ASSERT_EQ(huge.size(), 1U);
    EXPECT_EQ(huge[0].format.height, 1e300);
}

TEST(MTextFormat, UnbalancedClosingBraceDoesNothing) {
    const MTextParagraph pieces = one("A}B{\\H3;C");
    ASSERT_EQ(pieces.size(), 2U);
    EXPECT_EQ(pieces[0].text, U"AB");
    EXPECT_EQ(pieces[1].format.height, 3.0);
}

TEST(MTextFormat, IllFormedUtf8AsDecodeUtf8) {
    // Each ill-formed subsequence is one U+FFFD with its bytes: a lone
    // continuation byte, a truncated three-byte sequence, an overlong lead.
    const MTextParagraph pieces = one("a\x80\xE2\x82z\xC0\xAF");
    ASSERT_EQ(pieces.size(), 1U);
    EXPECT_EQ(pieces[0].text, decodeUtf8("a\x80\xE2\x82z\xC0\xAF"));
    EXPECT_EQ(pieces[0].units, units({{0, 1}, {1, 2}, {2, 4}, {4, 5}, {5, 6}, {6, 7}}));
}

/// A deterministic generator for the comparisons below (a linear
/// congruential generator; the numbers of Knuth's MMIX).
class Lcg {
public:
    std::size_t below(std::size_t bound) {
        constexpr std::uint64_t kMultiplier = 6364136223846793005ULL;
        constexpr std::uint64_t kIncrement = 1442695040888963407ULL;
        constexpr unsigned kHighBits = 33;
        m_state = (m_state * kMultiplier) + kIncrement;
        return static_cast<std::size_t>((m_state >> kHighBits) % bound);
    }

private:
    std::uint64_t m_state{1};
};

TEST(MTextFormat, PlainBytesGiveDecodeUtf8AndUnitsThatCoverThem) {
    // Without '\', '%', '{' and '}' every byte is in exactly one unit and the
    // text is decodeUtf8() of the value.
    constexpr std::array<char, 12> kBytes{'a',    ' ',    ';',    '/',    '\x80', '\xBF',
                                          '\xC3', '\xA9', '\xE2', '\x8C', '\xF0', '\xED'};
    constexpr std::size_t kValues = 500;
    constexpr std::size_t kMaxLength = 12;
    Lcg random;
    for (std::size_t n = 0; n < kValues; ++n) {
        std::string value;
        const std::size_t length = random.below(kMaxLength);
        for (std::size_t k = 0; k < length; ++k) {
            value += kBytes.at(random.below(kBytes.size()));
        }
        const std::vector<MTextParagraph> paragraphs = read(value);
        ASSERT_EQ(paragraphs.size(), 1U);
        std::u32string text;
        std::size_t next = 0;
        for (const MTextRun& run : paragraphs[0]) {
            ASSERT_EQ(run.units.size(), run.text.size());
            for (const MTextUnit& unit : run.units) {
                EXPECT_EQ(unit.sourceBegin, next);
                EXPECT_LT(unit.sourceBegin, unit.sourceEnd);
                next = unit.sourceEnd;
            }
            text += run.text;
        }
        EXPECT_EQ(next, value.size());
        EXPECT_EQ(text, decodeUtf8(value));
    }
}

// paragraphs() of stage 0.8, before PT7, kept here as it was
// (core/src/mtext_layout.cpp at 0.8.1 before PT7): the new reading must give
// the same paragraphs.
namespace legacy {

using mtext_layout::kNoBreakSpace;
constexpr char32_t kReplacement = U'\uFFFD';
constexpr std::size_t kUnicodeCodeLength = 7;
constexpr std::size_t kNoEnd = std::u32string_view::npos;

std::optional<char32_t> percentCharacter(char32_t code) {
    switch (code) {
        case U'c':
        case U'C':
            return U'⌀';
        case U'd':
        case U'D':
            return U'°';
        case U'p':
        case U'P':
            return U'±';
        case U'%':
            return U'%';
        default:
            return std::nullopt;
    }
}

bool takesArgument(char32_t code) {
    return std::u32string_view{U"fFHWQTACcpS"}.find(code) != std::u32string_view::npos;
}

bool isToggleCode(char32_t code) {
    return std::u32string_view{U"LlOoKk"}.find(code) != std::u32string_view::npos;
}

std::optional<char32_t> unicodeCode(std::u32string_view text, std::size_t at) {
    constexpr std::size_t kDigits = 4;
    constexpr std::uint32_t kBase = 16;
    if (at + kUnicodeCodeLength > text.size() || text[at + 2] != U'+') {
        return std::nullopt;
    }
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < kDigits; ++i) {
        const char32_t c = text[at + 3 + i];
        std::uint32_t digit = 0;
        if (c >= U'0' && c <= U'9') {
            digit = c - U'0';
        } else if (c >= U'a' && c <= U'f') {
            digit = c - U'a' + 10;
        } else if (c >= U'A' && c <= U'F') {
            digit = c - U'A' + 10;
        } else {
            return std::nullopt;
        }
        value = (value * kBase) + digit;
    }
    const auto code = static_cast<char32_t>(value);
    return code >= 0xD800 && code <= 0xDFFF ? kReplacement : code;
}

std::size_t argumentEnd(std::u32string_view text, std::size_t from) {
    for (std::size_t i = from; i < text.size(); ++i) {
        if (text[i] == U';') {
            return i;
        }
        if (text[i] == U'\\' && i + 1 < text.size()) {
            if (text[i + 1] == U'P') {
                return kNoEnd;
            }
            ++i;
        }
    }
    return kNoEnd;
}

std::vector<std::u32string> paragraphs(std::string_view value) {
    const std::u32string text = decodeUtf8(value);
    std::vector<std::u32string> result(1);
    std::size_t i = 0;
    while (i < text.size()) {
        std::u32string& current = result.back();
        const char32_t c = text[i];
        if (c == U'{' || c == U'}') {
            ++i;
            continue;
        }
        if (c == U'%' && i + 3 <= text.size() && text[i + 1] == U'%') {
            if (const std::optional<char32_t> converted = percentCharacter(text[i + 2])) {
                current += *converted;
                i += 3;
                continue;
            }
        }
        if (c != U'\\' || i + 1 == text.size()) {
            current += c;
            ++i;
            continue;
        }
        const char32_t code = text[i + 1];
        if (code == U'P') {
            result.emplace_back();
            i += 2;
        } else if (code == U'~') {
            current += kNoBreakSpace;
            i += 2;
        } else if (code == U'\\' || code == U'{' || code == U'}') {
            current += code;
            i += 2;
        } else if (isToggleCode(code)) {
            i += 2;
        } else if (const std::optional<char32_t> unicode =
                       code == U'U' ? unicodeCode(text, i) : std::nullopt) {
            current += *unicode;
            i += kUnicodeCodeLength;
        } else if (const std::size_t end = takesArgument(code) ? argumentEnd(text, i + 2) : kNoEnd;
                   end != kNoEnd) {
            if (code == U'S') {
                for (std::size_t k = i + 2; k < end; ++k) {
                    const char32_t part = text[k];
                    current += part == U'^' || part == U'#' ? U'/' : part;
                }
            }
            i = end + 1;
        } else {
            current += c;
            ++i;
        }
    }
    return result;
}

}  // namespace legacy

TEST(MTextFormat, ParagraphsOfStageZeroEightDoNotChange) {
    // The values of the verification cases with codes, and harder ones.
    for (const std::string_view value :
         {R"(A\P\PB\P)",
          "{A{\\LBC\\l}D}",
          R"(\\\{\})",
          R"(\U+00E9\U+03A9\U+2300)",
          "%%c %%d %%p %%%",
          R"(\LA\l\OB\o\KC\k)",
          R"(A\fArial;B\FArial;C\H2;D\W2;E\Q30;F\T2;G\A1;H\C1;I\c255;J\px1;K)",
          R"(\S1^2; \S3#4; \S5/6;)",
          "%%%c",
          "%%%%c",
          "A\\XB",
          "A\\H2\\PB",
          "\\\\P",
          "Seconda {\\H2x;grande} riga",
          "\\S1^ 2;",
          R"(\S%%c\U+00B1/\\\~2;)",
          "\\S1\\;2;",
          "a\\~b c\\~",
          "\xC2\xA0\\~",
          "\\U+D800\\U+12",
          "\\",
          "a\\",
          "%%",
          "%%x",
          "\\S\\P1/2;",
          R"({\H0;\W-1;\T0;\Habc;x})",
          "\\Zx;",
          "\x80\xE2\x82\\S\xE2\x82/\xFF;",
          "}{}{",
          R"(\pxqc;\A1;\px1;text)"}) {
        EXPECT_EQ(mtext_layout::paragraphs(value), legacy::paragraphs(value)) << value;
    }
}

TEST(MTextFormat, ParagraphsOfStageZeroEightDoNotChangeOnGeneratedValues) {
    constexpr std::array<std::string_view, 40> kPieces{
        "a",     "B",     " ",       ";",   "/",      "#",      "^",      "^ ",
        "{",     "}",     "\\",      "\\P", "\\~",    "\\\\",   "\\{",    "\\}",
        "\\L",   "\\l",   "\\O",     "\\K", "\\H2;",  "\\H2x;", "\\H",    "\\S",
        "\\S1/", "\\C1;", "\\c255;", "\\f", "Arial|", "b1;",    "\\U+00", "\\U+0042",
        "%%",    "%%c",   "%",       "é",   "\xE2",   "\x82",   "\\A1;",  "\\Zx;"};
    constexpr std::size_t kValues = 3000;
    constexpr std::size_t kMaxPieces = 10;
    Lcg random;
    for (std::size_t n = 0; n < kValues; ++n) {
        std::string value;
        const std::size_t count = random.below(kMaxPieces);
        for (std::size_t k = 0; k < count; ++k) {
            value += kPieces.at(random.below(kPieces.size()));
        }
        ASSERT_EQ(mtext_layout::paragraphs(value), legacy::paragraphs(value)) << value;
    }
}

}  // namespace
}  // namespace tg::core
