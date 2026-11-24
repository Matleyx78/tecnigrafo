// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/mtext.hpp"

#include "tg/core/block_table.hpp"
#include "tg/core/color.hpp"
#include "tg/core/document.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/explode.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/text.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/core/text_style_table.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-12;

void expectNear(const Vec2& actual, const Vec2& expected, double tolerance = kTolerance) {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
}

/// The width of `text` in the bundled font at height `height`.
double widthOf(std::u32string_view text, double height = 1.0) {
    return textWidth(text) * height;
}

MText mtext(std::string value, double height = 1.0, double width = 0.0) {
    MText result;
    result.insertion = {0, 0};
    result.height = height;
    result.width = width;
    result.value = std::move(value);
    return result;
}

/// The Texts that `entity` generates, alone in a new drawing (or `document`).
std::vector<Text> rows(const MText& entity, Document document = Document::newDrawing()) {
    const EntityRecord record = *document.find(document.add(entity));
    std::vector<Text> texts;
    for (const GeneratedItem& item : generate(document, record)) {
        const Text* text = std::get_if<Text>(&item.shape);
        EXPECT_NE(text, nullptr);
        if (text != nullptr) {
            texts.push_back(*text);
        }
    }
    return texts;
}

std::vector<std::string> values(const MText& entity) {
    std::vector<std::string> result;
    for (const Text& text : rows(entity)) {
        result.push_back(text.value);
    }
    return result;
}

using Values = std::vector<std::string>;

/// Everything that `entity` generates, alone in a new drawing (or `document`).
std::vector<GeneratedItem> items(const MText& entity, Document document = Document::newDrawing()) {
    const EntityRecord record = *document.find(document.add(entity));
    return generate(document, record);
}

/// The Text of a Text or of a FormattedText; nullptr for a line.
const Text* textOf(const GeneratedItem& item) {
    if (const auto* text = std::get_if<Text>(&item.shape)) {
        return text;
    }
    if (const auto* piece = std::get_if<FormattedText>(&item.shape)) {
        return &piece->text;
    }
    return nullptr;
}

/// The values of the texts of every piece, plain and formatted, in order.
Values pieceValues(const MText& entity) {
    Values result;
    for (const GeneratedItem& item : items(entity)) {
        if (const Text* text = textOf(item)) {
            result.push_back(text->value);
        }
    }
    return result;
}

/// The lines of decoration and of the fractions, in order.
std::vector<Line> textLines(const MText& entity) {
    std::vector<Line> result;
    for (const GeneratedItem& item : items(entity)) {
        if (const auto* line = std::get_if<TextLine>(&item.shape)) {
            result.push_back(line->line);
        }
    }
    return result;
}

TEST(MTextSupport, IsSupported) {
    EXPECT_TRUE(supported(MText{}));
    EXPECT_TRUE(isSupported(Entity{MText{}}));
}

// --- Codes (PT13, point 2) ----------------------------------------------------

TEST(MTextCodes, ParagraphsStartAtEveryP) {
    EXPECT_EQ(values(mtext(R"(A\PB\PC)")), (Values{"A", "B", "C"}));
}

TEST(MTextCodes, NoBreakSpaceIsShownAsASpace) {
    EXPECT_EQ(values(mtext(R"(A\~B)")), (Values{"A B"}));
}

TEST(MTextCodes, EscapedCharacters) {
    EXPECT_EQ(values(mtext(R"(\\\{\})")), (Values{R"(\{})"}));
    EXPECT_EQ(values(mtext(R"(\\P)")), (Values{R"(\P)"}));
}

TEST(MTextCodes, GroupBracesAreRemoved) {
    EXPECT_EQ(values(mtext("{A{B}C}")), (Values{"ABC"}));
}

TEST(MTextCodes, UnicodeCharacters) {
    EXPECT_EQ(values(mtext(R"(\U+00E9\U+03a9\U+2300)")), (Values{"éΩ⌀"}));
    // Without its four hexadecimal digits the code stays as written.
    EXPECT_EQ(values(mtext(R"(\U+00G9)")), (Values{R"(\U+00G9)"}));
    EXPECT_EQ(values(mtext(R"(\U+00)")), (Values{R"(\U+00)"}));
}

TEST(MTextCodes, PercentCodesAsDisplayText) {
    EXPECT_EQ(values(mtext("%%c %%d %%p %%%")), (Values{"⌀ ° ± %"}));
    EXPECT_EQ(values(mtext("%%C %%D %%P")), (Values{"⌀ ° ±"}));
    EXPECT_EQ(values(mtext("%%x")), (Values{"%%x"}));
}

/// The rows of `entity` as they are shown: displayText() of every value.
std::vector<std::u32string> shown(const MText& entity) {
    std::vector<std::u32string> result;
    for (const Text& text : rows(entity)) {
        result.push_back(displayText(text.value));
    }
    return result;
}

using Shown = std::vector<std::u32string>;

TEST(MTextCodes, CodesAreDecodedOnce) {
    EXPECT_EQ(shown(mtext("%%%c")), (Shown{U"%c"}));
    EXPECT_EQ(shown(mtext("%%%%c")), (Shown{U"%%c"}));
}

// --- The value of a row (PT13, point 4) -----------------------------------------

TEST(MTextValue, PercentCodesOfTheRowAreWrittenSoThatDisplayTextGivesItBack) {
    // MTEXT values that decode to the rows %%c, %%%, %%D, %c and 50%.
    const std::array<std::pair<std::string_view, std::string_view>, 5> cases{{
        {"%%%%c", "%%%%c"},
        {"%%%%%%%%%", "%%%%%"},
        {"%%%%%%D", "%%%%D"},
        {"%%%c", "%c"},
        {"50%", "50%"},
    }};
    const std::array<std::u32string_view, 5> rowsShown{U"%%c", U"%%%", U"%%D", U"%c", U"50%"};
    for (std::size_t i = 0; i < cases.size(); ++i) {
        const auto [value, expected] = cases.at(i);
        EXPECT_EQ(values(mtext(std::string{value})), (Values{std::string{expected}})) << i;
        EXPECT_EQ(shown(mtext(std::string{value})), (Shown{std::u32string{rowsShown.at(i)}})) << i;
    }
}

TEST(MTextValue, EveryRowOfPercentSignsAndLettersComesBack) {
    // Every row of up to five characters among %, c, D, p and x: written in
    // the MTEXT with each % as %%%, its Text shows it unchanged.
    constexpr std::u32string_view kAlphabet = U"%cDpx";
    constexpr std::size_t kLongest = 5;
    std::vector<std::u32string> current{U""};
    for (std::size_t length = 1; length <= kLongest; ++length) {
        std::vector<std::u32string> next;
        for (const std::u32string& prefix : current) {
            for (const char32_t c : kAlphabet) {
                next.push_back(prefix + c);
            }
        }
        current = std::move(next);
        for (const std::u32string& row : current) {
            std::string value;
            for (const char32_t c : row) {
                value += c == U'%' ? std::string{"%%%"} : std::string(1, static_cast<char>(c));
            }
            ASSERT_EQ(shown(mtext(value)), (Shown{row})) << value;
        }
    }
}

TEST(MTextValue, TheBoxOfARowIsThatOfItsCharacters) {
    // The row %%c is three characters wide, not the width of the diameter sign.
    const std::vector<Text> texts = rows(mtext("%%%%c", 2.0));
    ASSERT_EQ(texts.size(), 1U);
    const std::array<Vec2, 4> box = textBox(texts[0]);
    EXPECT_NEAR(box[1].x - box[0].x, widthOf(U"%%c", 2.0), kTolerance);
    EXPECT_GT(std::abs(widthOf(U"%%c", 2.0) - widthOf(U"⌀", 2.0)), 0.1);
}

TEST(MTextCodes, CodesWithAnArgumentAreHiddenAndSplitThePieces) {
    // Since the 0.8.1 the codes make pieces (PT8): their texts, in reading
    // order, still show every character and no code.
    std::string joined;
    for (const std::string& value :
         pieceValues(mtext(R"(A\fArial|b1;B\FArial;C\H2.5x;D\W2;E\Q30;F\T2;G\A1;H\C1;I)"
                           R"(\c255;J\px1,qc;K)"))) {
        joined += value;
    }
    EXPECT_EQ(joined, "ABCDEFGHIJK");
}

TEST(MTextCodes, StacksAreFractionsOfTwoTexts) {
    // Since the 0.8.1 a stack is two texts, no longer "1/2" (PT8).
    EXPECT_EQ(pieceValues(mtext(R"(\S1^2; \S3#4; \S5/6;)")),
              (Values{"1", "2", "3", "4", "5", "6"}));
}

TEST(MTextCodes, TogglesKeepTheTextAndAddTheirLines) {
    EXPECT_EQ(pieceValues(mtext(R"(\LA\l\OB\o\KC\k)")), (Values{"A", "B", "C"}));
    EXPECT_EQ(textLines(mtext(R"(\LA\l\OB\o\KC\k)")).size(), 3U);
}

TEST(MTextCodes, AnyOtherCodeStaysAsWritten) {
    EXPECT_EQ(values(mtext(R"(A\XB)")), (Values{R"(A\XB)"}));
    EXPECT_EQ(values(mtext(R"(A\)")), (Values{R"(A\)"}));
}

TEST(MTextCodes, ACodeWithoutItsSemicolonStaysButBreaksAtP) {
    EXPECT_EQ(values(mtext(R"(A\H2\PB)")), (Values{R"(A\H2)", "B"}));
    EXPECT_EQ(values(mtext(R"(A\S1^2)")), (Values{R"(A\S1^2)"}));
    // The ';' after a \P does not end the code before it.
    EXPECT_EQ(values(mtext(R"(A\H2\PB;C)")), (Values{R"(A\H2)", "B;C"}));
}

// --- Formats (docs/adr/0054, points 1, 3 and 4; 0.8.1 PT8) -----------------------

/// The width of `text` in `face` at height `height`.
double faceWidth(std::u32string_view text, FontFace face, double height = 1.0) {
    return textWidth(text, face) * height;
}

/// The item `index` of `all` as a `T`, or a failure.
template <typename T>
T shapeAt(const std::vector<GeneratedItem>& all, std::size_t index) {
    if (index >= all.size() || !std::holds_alternative<T>(all[index].shape)) {
        ADD_FAILURE() << "item " << index << " is not of the expected shape";
        return T{};
    }
    return std::get<T>(all[index].shape);
}

/// The shapes of an MTEXT.
using PieceShape = std::variant<Text, FormattedText, TextLine>;

/// The shapes that `entity` generates, alone in a new drawing.
std::vector<PieceShape> shapes(const MText& entity) {
    std::vector<PieceShape> result;
    for (const GeneratedItem& item : items(entity)) {
        std::visit(
            [&](const auto& shape) {
                using T = std::decay_t<decltype(shape)>;
                if constexpr (std::is_same_v<T, Text> || std::is_same_v<T, FormattedText> ||
                              std::is_same_v<T, TextLine>) {
                    result.emplace_back(shape);
                } else {
                    ADD_FAILURE() << "not a shape of an MTEXT";
                }
            },
            item.shape);
    }
    return result;
}

void expectLine(const Line& line, const Vec2& start, const Vec2& end) {
    expectNear(line.start, start);
    expectNear(line.end, end);
}

TEST(MTextFormats, BoldItalicAndBothAreFormattedTextsOnTheirAdvances) {
    // \f with any name: Liberation Sans in the face of |b and |i.
    const std::vector<GeneratedItem> all =
        items(mtext(R"(A{\fArial|b1;B}{\fTimes|i1;C}{\fArial|b1|i1;D}E)"));
    ASSERT_EQ(all.size(), 5U);
    const auto a = shapeAt<Text>(all, 0);
    expectNear(a.position, {0, -1});
    double x = widthOf(U"A");
    const std::array<std::pair<FontFace, std::u32string_view>, 3> formatted{{
        {FontFace::Bold, U"B"},
        {FontFace::Italic, U"C"},
        {FontFace::BoldItalic, U"D"},
    }};
    for (std::size_t i = 0; i < formatted.size(); ++i) {
        const auto [face, character] = formatted.at(i);
        const auto piece = shapeAt<FormattedText>(all, i + 1);
        EXPECT_EQ(piece.face, face) << i;
        EXPECT_EQ(piece.tracking, 1.0) << i;
        EXPECT_EQ(piece.text.height, 1.0) << i;
        EXPECT_EQ(piece.text.widthFactor, 1.0) << i;
        expectNear(piece.text.position, {x, -1});
        x += faceWidth(character, face);
    }
    const auto e = shapeAt<Text>(all, 4);
    EXPECT_EQ(e.value, "E");
    expectNear(e.position, {x, -1});
}

TEST(MTextFormats, RelativeAndAbsoluteHeights) {
    // The first baseline is below the top by the highest piece (rule 3).
    const std::vector<GeneratedItem> relative = items(mtext(R"(A{\H2x;B}C)"));
    ASSERT_EQ(relative.size(), 3U);
    const auto a = shapeAt<Text>(relative, 0);
    const auto b = shapeAt<Text>(relative, 1);
    const auto c = shapeAt<Text>(relative, 2);
    EXPECT_EQ(a.height, 1.0);
    EXPECT_EQ(b.height, 2.0);
    EXPECT_EQ(c.height, 1.0);
    expectNear(a.position, {0, -2});
    expectNear(b.position, {widthOf(U"A"), -2});
    expectNear(c.position, {widthOf(U"A") + widthOf(U"B", 2.0), -2});

    const std::vector<GeneratedItem> absolute = items(mtext(R"(A{\H3;B})", 2.0));
    ASSERT_EQ(absolute.size(), 2U);
    EXPECT_EQ(shapeAt<Text>(absolute, 0).height, 2.0);
    EXPECT_EQ(shapeAt<Text>(absolute, 1).height, 3.0);
    expectNear(shapeAt<Text>(absolute, 1).position, {widthOf(U"A", 2.0), -3});
}

TEST(MTextFormats, WidthFactorObliqueAndTracking) {
    const double a = widthOf(U"A");
    const double b = widthOf(U"B");

    const std::vector<GeneratedItem> wide = items(mtext(R"(A{\W2;B}C)"));
    ASSERT_EQ(wide.size(), 3U);
    EXPECT_EQ(shapeAt<Text>(wide, 1).widthFactor, 2.0);
    expectNear(shapeAt<Text>(wide, 1).position, {a, -1});
    expectNear(shapeAt<Text>(wide, 2).position, {a + (2.0 * b), -1});

    const std::vector<GeneratedItem> oblique = items(mtext(R"(A{\Q30;B}C)"));
    ASSERT_EQ(oblique.size(), 3U);
    EXPECT_NEAR(shapeAt<Text>(oblique, 1).obliqueAngle, degreesToRadians(30.0), kTolerance);
    expectNear(shapeAt<Text>(oblique, 2).position, {a + b, -1});

    // A Regular piece with a tracking other than 1 is a FormattedText
    // (#927, [bloccante] 1): its advances double, its width factor stays 1.
    const std::vector<GeneratedItem> tracked = items(mtext(R"(A{\T2;B}C)"));
    ASSERT_EQ(tracked.size(), 3U);
    const auto piece = shapeAt<FormattedText>(tracked, 1);
    EXPECT_EQ(piece.face, FontFace::Regular);
    EXPECT_EQ(piece.tracking, 2.0);
    EXPECT_EQ(piece.text.widthFactor, 1.0);
    expectNear(piece.text.position, {a, -1});
    expectNear(shapeAt<Text>(tracked, 2).position, {a + (2.0 * b), -1});
    // {\T2;A} alone is a FormattedText too, not a Text.
    const std::vector<GeneratedItem> alone = items(mtext(R"({\T2;A})"));
    ASSERT_EQ(alone.size(), 1U);
    EXPECT_TRUE(std::holds_alternative<FormattedText>(alone[0].shape));
}

TEST(MTextFormats, IndexAndTrueColoursOfThePieces) {
    Document document = Document::newDrawing();
    const EntityRecord record = *document.find(document.add(mtext(R"(A{\C1;B}{\c16744448;C})")));
    const std::vector<GeneratedItem> all = generate(document, record);
    ASSERT_EQ(all.size(), 3U);
    EXPECT_EQ(all[0].properties.color, effectiveProperties(document, record).color);
    EXPECT_EQ(all[1].properties.color, (Rgb{255, 0, 0}));
    // BGR as AutoCAD: 16744448 is 0080FF in RGB.
    EXPECT_EQ(all[2].properties.color, (Rgb{0x00, 0x80, 0xFF}));
    // A colour alone keeps the piece a Text.
    EXPECT_TRUE(std::holds_alternative<Text>(all[1].shape));
    EXPECT_TRUE(std::holds_alternative<Text>(all[2].shape));
}

TEST(MTextFormats, UnderlineOverlineAndStrikeWithTheColourOfThePiece) {
    const double a = widthOf(U"A", 2.5);
    const std::vector<GeneratedItem> under = items(mtext(R"({\C1;\LA})", 2.5));
    ASSERT_EQ(under.size(), 2U);
    expectNear(shapeAt<Text>(under, 0).position, {0, -2.5});
    expectLine(shapeAt<TextLine>(under, 1).line, {0, -3}, {a, -3});
    EXPECT_EQ(under[1].properties.color, (Rgb{255, 0, 0}));

    // The overline lowers the first baseline by 0.2 h: 3 for h 2.5 (rule 3).
    const std::vector<GeneratedItem> over = items(mtext(R"({\OA})", 2.5));
    ASSERT_EQ(over.size(), 2U);
    expectNear(shapeAt<Text>(over, 0).position, {0, -3});
    expectLine(shapeAt<TextLine>(over, 1).line, {0, 0}, {a, 0});

    const std::vector<GeneratedItem> strike = items(mtext(R"({\KA})", 2.5));
    ASSERT_EQ(strike.size(), 2U);
    expectLine(shapeAt<TextLine>(strike, 1).line, {0, -1.25}, {a, -1.25});
}

TEST(MTextFormats, TheThreeStackedFractions) {
    // Rule 5: as wide as the wider text, both centred; the numerator 4/3 h
    // above the baseline, the line of '/' at 7/6 h.
    const double wide = widthOf(U"22");
    const double narrow = widthOf(U"1");
    const std::vector<GeneratedItem> horizontal = items(mtext(R"(\S1/22;)"));
    ASSERT_EQ(horizontal.size(), 3U);
    const auto top = shapeAt<Text>(horizontal, 0);
    const auto bottom = shapeAt<Text>(horizontal, 1);
    EXPECT_EQ(top.value, "1");
    EXPECT_EQ(bottom.value, "22");
    expectNear(top.position, {(wide - narrow) / 2.0, -1 + (4.0 / 3.0)});
    expectNear(bottom.position, {0, -1});
    expectLine(shapeAt<TextLine>(horizontal, 2).line, {0, -1 + (7.0 / 6.0)},
               {wide, -1 + (7.0 / 6.0)});

    // '^': no line.
    const std::vector<GeneratedItem> tolerance = items(mtext(R"(\S1^22;)"));
    ASSERT_EQ(tolerance.size(), 2U);
    EXPECT_TRUE(std::holds_alternative<Text>(tolerance[0].shape));
    EXPECT_TRUE(std::holds_alternative<Text>(tolerance[1].shape));
}

TEST(MTextFormats, TheDiagonalFractionReachesTheTopOfTheNumerator) {
    // \S1#2; of height 2.5 after an A: from (x, 0) to (x + w, 7/3 h) of
    // its baseline (#927, [decisione] 3).
    const double x = widthOf(U"A", 2.5);
    const double w = widthOf(U"1", 2.5);
    const std::vector<GeneratedItem> all = items(mtext(R"(A\S1#2;)", 2.5));
    ASSERT_EQ(all.size(), 4U);
    const Line line = shapeAt<TextLine>(all, 3).line;
    const double baseline = -2.5;
    expectNear(line.start, {x, baseline});
    expectNear(line.end, {x + w, baseline + 5.833333333333333});
    // Its middle is that of the line of '/'.
    EXPECT_NEAR((line.start.y + line.end.y) / 2.0 - baseline, (7.0 / 6.0) * 2.5, kTolerance);
}

TEST(MTextFormats, AWordWithTwoFormatsWrapsWhole) {
    // "AA B" fits, "AA BCD" does not: the word B{C}D moves whole (rule 2).
    const double width = widthOf(U"AA B") + 0.01;
    ASSERT_LT(width, widthOf(U"AA BCD"));
    const std::vector<GeneratedItem> all = items(mtext(R"(AA B{\C1;C}D)", 1.0, width));
    ASSERT_EQ(all.size(), 4U);
    const double second = -1 - (5.0 / 3.0);
    EXPECT_EQ(shapeAt<Text>(all, 0).value, "AA");
    expectNear(shapeAt<Text>(all, 0).position, {0, -1});
    EXPECT_EQ(shapeAt<Text>(all, 1).value, "B");
    expectNear(shapeAt<Text>(all, 1).position, {0, second});
    EXPECT_EQ(shapeAt<Text>(all, 2).value, "C");
    expectNear(shapeAt<Text>(all, 2).position, {widthOf(U"B"), second});
    expectNear(shapeAt<Text>(all, 3).position, {widthOf(U"BC"), second});
}

TEST(MTextFormats, TheBreakInTheBandOfTheThreshold) {
    // "A A" at height 3 is exactly 9903/1409 wide: one row, as in the 0.8.
    const double width = 9903.0 / 1409.0;
    EXPECT_EQ(values(mtext("A A", 3.0, width)), (Values{"A A"}));
    // With two pieces the sum may round either way: free, but the same twice.
    const MText coloured = mtext(R"(A {\C1;A})", 3.0, width);
    EXPECT_EQ(shapes(coloured), shapes(coloured));
}

TEST(MTextFormats, EmptyRowsKeepTheirPlaceButShowNothing) {
    Document document = Document::newDrawing();
    const EntityRecord record = *document.find(document.add(mtext(R"(A\P\PB\P)")));
    const std::vector<GeneratedItem> all = generate(document, record);
    ASSERT_EQ(all.size(), 2U);
    expectNear(shapeAt<Text>(all, 0).position, {0, -1});
    expectNear(shapeAt<Text>(all, 1).position, {0, -1 - (2.0 * 5.0 / 3.0)});
    EXPECT_EQ(shownLines(document, record), (std::vector<std::u32string>{U"A", U"B"}));
}

TEST(MTextFormats, ShownLinesJoinThePiecesOfARow) {
    Document document = Document::newDrawing();
    const EntityRecord three = *document.find(document.add(mtext(R"(A{\C1;B}{\fArial|b1;C})")));
    ASSERT_EQ(generate(document, three).size(), 3U);
    EXPECT_EQ(shownLines(document, three), (std::vector<std::u32string>{U"ABC"}));
    const EntityRecord stacked = *document.find(document.add(mtext(R"(A \S1/2;\PB)")));
    EXPECT_EQ(shownLines(document, stacked), (std::vector<std::u32string>{U"A 1/2", U"B"}));
    // Nothing for the other types.
    const EntityRecord line = *document.find(document.add(Line{{0, 0}, {1, 0}}));
    EXPECT_TRUE(shownLines(document, line).empty());
}

TEST(MTextFormats, WithoutFormatsOneTextPerRow) {
    for (const GeneratedItem& item : items(mtext(R"(Prima riga\PSeconda)", 2.5, 10.0))) {
        EXPECT_TRUE(std::holds_alternative<Text>(item.shape));
    }
}

// --- Layout (PT13, point 3) -----------------------------------------------------

TEST(MTextLayout, WrapsBeforeTheWordBeyondTheWidth) {
    EXPECT_EQ(values(mtext("A B C", 1.0, 3.0)), (Values{"A B", "C"}));
    EXPECT_EQ(values(mtext("A B C")), (Values{"A B C"}));
}

TEST(MTextLayout, AWordWiderThanTheColumnStandsAlone) {
    const std::vector<Text> texts = rows(mtext("AAAA B CC", 1.0, 2.5));
    ASSERT_EQ(texts.size(), 3U);
    EXPECT_EQ(texts[0].value, "AAAA");
    EXPECT_EQ(texts[1].value, "B");
    EXPECT_EQ(texts[2].value, "CC");
    const double step = 5.0 / 3.0;
    expectNear(texts[0].position, {0, -1});
    expectNear(texts[1].position, {0, -1 - step});
    expectNear(texts[2].position, {0, -1 - (2 * step)});
}

TEST(MTextLayout, ARowAsWideAsTheColumnFits) {
    // At height 1409 the widths are the advances of the font file: 3301.
    const double width = widthOf(U"A B", 1409.0);
    EXPECT_EQ(values(mtext("A B", 1409.0, width)), (Values{"A B"}));
}

TEST(MTextLayout, NoBreakSpaceKeepsTheWordsTogether) {
    const double width = widthOf(U"A B", 1409.0);
    EXPECT_EQ(values(mtext(R"(A\~B C)", 1409.0, width)), (Values{"A B", "C"}));
    // Even in a column narrower than both: one word.
    EXPECT_EQ(values(mtext(R"(A\~B C)", 1.0, 1.5)), (Values{"A B", "C"}));
}

TEST(MTextLayout, SpacesWhereTheRowBreaksAreDropped) {
    EXPECT_EQ(values(mtext("A   B", 1.0, 1.5)), (Values{"A", "B"}));
    // Spaces elsewhere stay.
    EXPECT_EQ(values(mtext(" A  B ")), (Values{" A  B "}));
}

TEST(MTextLayout, StyleWidthFactorWidensTheRows) {
    Document document = Document::newDrawing();
    TextStyleTable styles = document.textStyles();
    TextStyle wide;
    wide.name = "Wide";
    wide.widthFactor = 2.0;
    wide.oblique = degreesToRadians(15.0);
    styles.put(wide, document);
    document.replaceTextStyles(styles);
    MText entity = mtext("A B C", 1.0, 3.0);
    entity.style = "Wide";
    const std::vector<Text> texts = rows(entity, document);
    ASSERT_EQ(texts.size(), 3U);  // "A B" is 4.68 wide with the factor 2
    EXPECT_EQ(texts[0].widthFactor, 2.0);
    EXPECT_EQ(texts[0].obliqueAngle, degreesToRadians(15.0));
    EXPECT_EQ(texts[0].style, "Wide");
}

TEST(MTextLayout, MissingStyleUsesTheDefaults) {
    MText entity = mtext("A");
    entity.style = "Missing";
    const std::vector<Text> texts = rows(entity);
    ASSERT_EQ(texts.size(), 1U);
    EXPECT_EQ(texts[0].widthFactor, 1.0);
    EXPECT_EQ(texts[0].obliqueAngle, 0.0);
    EXPECT_EQ(texts[0].style, "Missing");
}

struct AlignmentCase {
    MTextAttachment attachment{MTextAttachment::TopLeft};
    double firstX{0.0};  ///< times the width of the row
    double firstY{0.0};  ///< baseline of the first row
};

TEST(MTextLayout, NineAttachmentsWithTwoRows) {
    // h = 3, two rows: L = 5. Top: -3; middle: (3 + 5) / 2 - 3 = 1; bottom: 5.
    const std::array cases{
        AlignmentCase{MTextAttachment::TopLeft, 0.0, -3.0},
        AlignmentCase{MTextAttachment::TopCenter, -0.5, -3.0},
        AlignmentCase{MTextAttachment::TopRight, -1.0, -3.0},
        AlignmentCase{MTextAttachment::MiddleLeft, 0.0, 1.0},
        AlignmentCase{MTextAttachment::MiddleCenter, -0.5, 1.0},
        AlignmentCase{MTextAttachment::MiddleRight, -1.0, 1.0},
        AlignmentCase{MTextAttachment::BottomLeft, 0.0, 5.0},
        AlignmentCase{MTextAttachment::BottomCenter, -0.5, 5.0},
        AlignmentCase{MTextAttachment::BottomRight, -1.0, 5.0},
    };
    for (const double width : {0.0, 12.0}) {
        for (const AlignmentCase& alignment : cases) {
            MText entity = mtext(R"(A\PBB)", 3.0, width);
            entity.insertion = {20, 30};
            entity.attachment = alignment.attachment;
            const std::vector<Text> texts = rows(entity);
            ASSERT_EQ(texts.size(), 2U);
            const double a = widthOf(U"A", 3.0);
            const double bb = widthOf(U"BB", 3.0);
            expectNear(texts[0].position, {20 + (alignment.firstX * a), 30 + alignment.firstY});
            expectNear(texts[1].position,
                       {20 + (alignment.firstX * bb), 30 + alignment.firstY - 5.0});
            for (const Text& text : texts) {
                EXPECT_EQ(text.horizontal, TextHorizontal::Left);
                EXPECT_EQ(text.vertical, TextVertical::Baseline);
                EXPECT_EQ(text.height, 3.0);
            }
        }
    }
}

TEST(MTextLayout, LineSpacing) {
    for (const bool exact : {false, true}) {
        MText entity = mtext(R"(A\PB)", 3.0);
        entity.insertion = {10, 20};
        entity.lineSpacing = 1.5;
        entity.exactSpacing = exact;
        const std::vector<Text> texts = rows(entity);
        ASSERT_EQ(texts.size(), 2U);
        expectNear(texts[0].position, {10, 17});
        expectNear(texts[1].position, {10, 9.5});
    }
}

TEST(MTextLayout, RotationTurnsTheColumn) {
    MText entity = mtext(R"(A\PB)", 3.0);
    entity.insertion = {10, 20};
    entity.rotation = degreesToRadians(30.0);
    const std::vector<Text> texts = rows(entity);
    ASSERT_EQ(texts.size(), 2U);
    // x' = 10 + sqrt(3)/2 x - y/2, y' = 20 + x/2 + sqrt(3)/2 y.
    const double root3 = std::numbers::sqrt3;
    expectNear(texts[0].position, {11.5, 20 - (1.5 * root3)});
    expectNear(texts[1].position, {14, 20 - (4 * root3)});
    EXPECT_EQ(texts[0].rotation, entity.rotation);
}

TEST(MTextLayout, BlankRowsTakeABaselineButNoText) {
    const std::vector<Text> texts = rows(mtext(R"(  \PZ)", 3.0));
    ASSERT_EQ(texts.size(), 1U);
    EXPECT_EQ(texts[0].value, "Z");
    expectNear(texts[0].position, {0, -8});

    // A trailing \P makes an empty last row: Z on the middle baseline.
    MText bottom = mtext(R"(\PZ\P)", 3.0);
    bottom.attachment = MTextAttachment::BottomLeft;
    const std::vector<Text> lifted = rows(bottom);
    ASSERT_EQ(lifted.size(), 1U);
    expectNear(lifted[0].position, {0, 5});
}

TEST(MTextLayout, BlankRowsDoNotWidenTheColumn) {
    MText entity = mtext(R"(          \PA)", 1.0);
    entity.attachment = MTextAttachment::TopRight;
    const std::vector<Text> texts = rows(entity);
    ASSERT_EQ(texts.size(), 1U);
    expectNear(texts[0].position, {-widthOf(U"A"), -1 - (5.0 / 3.0)});
}

TEST(MTextLayout, DirectionByStyleFollowsTheStyle) {
    Document document = Document::newDrawing();
    TextStyleTable styles = document.textStyles();
    TextStyle mirrored;
    mirrored.name = "Mirrored";
    mirrored.backwards = true;
    mirrored.upsideDown = true;
    styles.put(mirrored, document);
    document.replaceTextStyles(styles);
    MText entity = mtext("A");
    entity.style = "Mirrored";
    const std::vector<Text> plain = rows(entity, document);
    ASSERT_EQ(plain.size(), 1U);
    EXPECT_FALSE(plain[0].mirroredX);
    EXPECT_FALSE(plain[0].mirroredY);
    entity.directionByStyle = true;
    const std::vector<Text> byStyle = rows(entity, document);
    ASSERT_EQ(byStyle.size(), 1U);
    EXPECT_TRUE(byStyle[0].mirroredX);
    EXPECT_TRUE(byStyle[0].mirroredY);
}

// --- Selection, snap and grips (PT13, points 4 and 6) ---------------------------

TEST(MTextDocument, EmptyMTextIsItsInsertionPoint) {
    for (const std::string_view value : {"", "   ", R"(\P\P)"}) {
        Document document = Document::newDrawing();
        MText entity = mtext(std::string{value});
        entity.insertion = {10, 20};
        const EntityId id = document.add(entity);
        const EntityRecord& record = *document.find(id);
        EXPECT_TRUE(generate(document, record).empty());
        const BBox box = extent(document, record);
        EXPECT_EQ(box.min(), (Vec2{10, 20}));
        EXPECT_EQ(box.max(), (Vec2{10, 20}));
        EXPECT_EQ(distance(document, record, {13, 24}), 5.0);
        EXPECT_EQ(pickEntity(document, {10, 20}, 0.0), std::optional<EntityId>{id});
        EXPECT_EQ(pickEntity(document, {11, 20}, 0.1), std::nullopt);
        EXPECT_EQ(selectInRectangle(document, BBox{{9, 19}, {11, 21}}, RectangleMode::Window),
                  std::vector<EntityId>{id});
        EXPECT_TRUE(intersects(document, record, BBox{{10, 20}, {10, 20}}));
        EXPECT_FALSE(intersects(document, record, BBox{{11, 21}, {12, 22}}));
    }
}

TEST(MTextDocument, EmptyMTextOnALayerOffIsNotPicked) {
    Document document = Document::newDrawing();
    Layer off;
    off.name = "Off";
    off.linetype = document.continuousLinetype();
    off.on = false;
    const LayerId layer = document.addLayer(off);
    MText entity = mtext("");
    entity.insertion = {10, 20};
    const EntityId id = document.add(entity);
    EntityRecord record = *document.find(id);
    record.properties.layer = layer;
    EXPECT_EQ(distance(document, record, {10, 20}), std::numeric_limits<double>::infinity());
    EXPECT_FALSE(isInside(document, record, BBox{{9, 19}, {11, 21}}));
    EXPECT_EQ(extent(document, record).min(), (Vec2{10, 20}));
}

TEST(MTextDocument, NothingOnAFrozenLayer) {
    Document document = Document::newDrawing();
    Layer frozen;
    frozen.name = "Frozen";
    frozen.linetype = document.continuousLinetype();
    frozen.frozen = true;
    const LayerId layer = document.addLayer(frozen);
    for (const std::string_view value : {"", "A"}) {
        EntityRecord record = *document.find(document.add(mtext(std::string{value})));
        record.properties.layer = layer;
        EXPECT_TRUE(generate(document, record).empty());
        EXPECT_TRUE(extent(document, record).isEmpty());
    }
}

TEST(MTextDocument, RowsAreSelectedAsTexts) {
    Document document = Document::newDrawing();
    MText entity = mtext(R"(A\PB)", 3.0);
    entity.insertion = {30, 20};
    const EntityId id = document.add(entity);
    const EntityRecord& record = *document.find(id);
    EXPECT_EQ(distance(document, record, {31, 17}), 0.0);
    EXPECT_NEAR(distance(document, record, {29, 17}), 1.0, kTolerance);
    EXPECT_EQ(pickEntity(document, {31, 17}, 0.1), std::optional<EntityId>{id});
    EXPECT_TRUE(isInside(document, record, BBox{{29, 10}, {34, 22}}));
    EXPECT_FALSE(isInside(document, record, BBox{{30, 16}, {32, 18}}));
    EXPECT_TRUE(intersects(document, record, BBox{{30, 16}, {32, 18}}));
}

TEST(MTextDocument, InsertionSnapAndGrip) {
    Document document = Document::newDrawing();
    MText entity = mtext(R"(A\PB)", 3.0, 12.0);
    entity.insertion = {10, 20};
    entity.attachment = MTextAttachment::MiddleCenter;
    const EntityRecord& record = *document.find(document.add(entity));
    SnapModes insertion{};
    insertion.endpoint = false;
    insertion.midpoint = false;
    insertion.center = false;
    insertion.intersection = false;
    std::vector<SnapPoint> own;
    appendOwnSnapPointsOf(document, record, entity, insertion, own, std::nullopt);
    ASSERT_EQ(own.size(), 1U);
    EXPECT_EQ(own[0].point, (Vec2{10, 20}));
    EXPECT_EQ(own[0].kind, SnapKind::Insertion);
    SnapModes none = insertion;
    none.insertion = false;
    own.clear();
    appendOwnSnapPointsOf(document, record, entity, none, own, std::nullopt);
    EXPECT_TRUE(own.empty());

    EXPECT_EQ(gripPoints(Entity{entity}), std::vector<Vec2>{(Vec2{10, 20})});
    const GripResult moved = moveGrip(Entity{entity}, 0, {25, 35});
    EXPECT_EQ(moved.outcome, GripOutcome::Moved);
    MText expected = entity;
    expected.insertion = {25, 35};
    EXPECT_EQ(std::get<MText>(moved.entity), expected);
}

// --- Transformation (PT13, point 5) ---------------------------------------------

MText transformedMText(const MText& entity, const Similarity& similarity, MirrorText mirrorText) {
    const TransformOutcome outcome =
        transformedIn(Document::newDrawing(), Entity{entity}, similarity, mirrorText);
    EXPECT_TRUE(outcome.refusal.empty());
    EXPECT_TRUE(outcome.entity.has_value());
    return std::get<MText>(outcome.entity.value_or(Entity{entity}));
}

MText sampleColumn() {
    MText entity = mtext(R"(A\PB)", 3.0, 12.0);
    entity.insertion = {10, 20};
    entity.attachment = MTextAttachment::MiddleCenter;
    entity.rotation = degreesToRadians(30.0);
    entity.lineSpacing = 1.5;
    return entity;
}

TEST(MTextTransform, MoveRotateScale) {
    const MText entity = sampleColumn();
    const MText moved =
        transformedMText(entity, Similarity::translation({5, -7}), MirrorText::Readable);
    expectNear(moved.insertion, {15, 13});
    EXPECT_EQ(moved.rotation, entity.rotation);

    const MText turned = transformedMText(entity, Similarity::rotation(degreesToRadians(90.0), {}),
                                          MirrorText::Readable);
    expectNear(turned.insertion, {-20, 10});
    EXPECT_NEAR(turned.rotation, degreesToRadians(120.0), kTolerance);

    const std::optional<Similarity> scaling = Similarity::scaling(2.0, {5, 10});
    ASSERT_TRUE(scaling.has_value());
    const MText scaled =
        transformedMText(entity, scaling.value_or(Similarity{}), MirrorText::Readable);
    expectNear(scaled.insertion, {15, 30});
    EXPECT_EQ(scaled.height, 6.0);
    EXPECT_EQ(scaled.width, 24.0);
    EXPECT_EQ(scaled.lineSpacing, 1.5);
    EXPECT_EQ(scaled.value, entity.value);
}

TEST(MTextTransform, AbsoluteHeightsStayUnderASimilarity) {
    // As AutoCAD (LIST before and after SCALE, 2026-11-22): the height of
    // the entity doubles, the value and its \H3; do not change.
    const MText entity = mtext(R"(A{\H3;B})", 2.0);
    const std::optional<Similarity> scaling = Similarity::scaling(2.0, {0, 0});
    ASSERT_TRUE(scaling.has_value());
    const MText scaled =
        transformedMText(entity, scaling.value_or(Similarity{}), MirrorText::Readable);
    EXPECT_EQ(scaled.height, 4.0);
    EXPECT_EQ(scaled.value, entity.value);
    const std::vector<GeneratedItem> all = items(scaled);
    ASSERT_EQ(all.size(), 2U);
    EXPECT_EQ(shapeAt<Text>(all, 0).height, 4.0);
    EXPECT_EQ(shapeAt<Text>(all, 1).height, 3.0);

    // EXPLODE of a block in scale 2 goes through the same transformation.
    Document document = Document::newDrawing();
    BlockDefinition definition{"Note"};
    EntityRecord member{EntityId{}, Entity{entity}};
    member.properties.layer = document.layerZero();
    member.properties.linetype = document.byLayerLinetype();
    definition.entities.push_back(member);
    document.replaceBlock("Note", std::move(definition));
    Insert insert;
    insert.block = "Note";
    insert.scaleX = 2.0;
    insert.scaleY = 2.0;
    const Exploded exploded = explodeInsert(document, insert);
    ASSERT_TRUE(exploded.refusal.empty()) << exploded.refusal;
    ASSERT_EQ(exploded.entities.size(), 1U);
    const auto* result = std::get_if<MText>(&exploded.entities[0].entity);
    ASSERT_NE(result, nullptr);
    EXPECT_EQ(result->height, 4.0);
    EXPECT_EQ(result->value, entity.value);
}

struct MirrorCase {
    Vec2 a;
    Vec2 b;
    Vec2 insertion;
    double degrees{0.0};
};

TEST(MTextTransform, ReflectionStaysReadableWhateverMirrtext) {
    const std::array cases{
        MirrorCase{{0, 0}, {1, 0}, {10, -20}, 330.0},
        MirrorCase{{0, 0}, {0, 1}, {-10, 20}, 330.0},
        MirrorCase{{0, 0}, {1, 1}, {20, 10}, 60.0},
    };
    for (const bool backwards : {false, true}) {
        Document document = Document::newDrawing();
        TextStyleTable styles = document.textStyles();
        TextStyle style;
        style.name = "Back";
        style.backwards = backwards;
        styles.put(style, document);
        document.replaceTextStyles(styles);
        MText entity = sampleColumn();
        entity.style = "Back";
        entity.directionByStyle = true;
        for (const MirrorCase& mirror : cases) {
            const std::optional<Similarity> reflection = Similarity::reflection(mirror.a, mirror.b);
            ASSERT_TRUE(reflection.has_value());
            const Similarity similarity = reflection.value_or(Similarity{});
            const MText readable = transformedMText(entity, similarity, MirrorText::Readable);
            const MText mirrored = transformedMText(entity, similarity, MirrorText::Mirrored);
            EXPECT_EQ(readable, mirrored);
            expectNear(readable.insertion, mirror.insertion);
            EXPECT_NEAR(readable.rotation, degreesToRadians(mirror.degrees), 1e-12);
            EXPECT_EQ(readable.directionByStyle, true);
            EXPECT_EQ(readable.style, "Back");
            // The rows keep the signals of the style: readable as before.
            const std::vector<Text> texts = rows(readable, document);
            ASSERT_EQ(texts.size(), 2U);
            EXPECT_EQ(texts[0].mirroredX, backwards);
            EXPECT_FALSE(texts[0].mirroredY);
        }
    }
}

TEST(MTextTransform, ReflectionOfAnObliqueColumn) {
    MText entity = mtext("A");
    entity.insertion = {2, 3};
    entity.rotation = degreesToRadians(120.0);
    const std::optional<Similarity> acrossX = Similarity::reflection({0, 0}, {1, 0});
    ASSERT_TRUE(acrossX.has_value());
    for (const MirrorText mirrorText : {MirrorText::Readable, MirrorText::Mirrored}) {
        const MText result = transformedMText(entity, acrossX.value_or(Similarity{}), mirrorText);
        expectNear(result.insertion, {2, -3});
        EXPECT_NEAR(result.rotation, degreesToRadians(60.0), 1e-12);
    }
}

}  // namespace
}  // namespace tg::core
