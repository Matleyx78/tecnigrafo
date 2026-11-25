// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The model of the editor of a formatted text (docs/adr/0054, point 5;
// docs/tappe/0.8.1/PT10-modello-di-modifica.md).

#include "tg/core/mtext_edit.hpp"

#include "tg/core/color.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/mtext_format.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/core/vec2.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-9;

using Kind = MTextEditUnit::Kind;
using Change = MTextFormatChange::Kind;

MText mtext(std::string value, double width = 0.0) {
    MText result;
    result.height = 1.0;
    result.width = width;
    result.value = std::move(value);
    return result;
}

MTextEditModel model(std::string value, double width = 0.0,
                     std::optional<std::string> measurement = std::nullopt) {
    return MTextEditModel(mtext(std::move(value), width),
                          MTextEditOptions{.measurement = std::move(measurement)});
}

double widthOf(std::u32string_view text) {
    return textWidth(text);  // height 1, width factor 1
}

/// The format of every unit of `value`, in reading order.
std::vector<MTextFormat> unitFormats(std::string_view value) {
    MTextFormat base;
    base.height = 1.0;
    std::vector<MTextFormat> formats;
    for (const MTextParagraph& paragraph : formattedParagraphs(value, base)) {
        for (const MTextRun& run : paragraph) {
            formats.insert(formats.end(), run.units.size(), run.format);
        }
    }
    return formats;
}

/// The values of autocad-mtext-formati.dxf (tests/verifica/sorgenti.json):
/// the MTEXT entities and the text override of its dimension.
const std::vector<std::string>& autocadValues() {
    static const std::vector<std::string> kValues = {
        R"({\C1;50} {\fArial|b1|i0|c0|p34;mm})",
        R"(Normale {\fArial|b1|i0|c0|p34;grassetto} normale)",
        R"(Normale {\fArial|b0|i1|c0|p34;corsivo} normale)",
        R"(Normale {\fArial|b1|i1|c0|p34;grassetto corsivo} normale)",
        R"(Normale {\Lsottolineato} normale)",
        R"(Normale {\Osopralineato} normale)",
        R"(Normale {\Kbarrato} normale)",
        R"(Normale {\H2x;doppia} normale)",
        R"(Normale {\H3.5;tre e mezzo} normale)",
        R"(Normale {\C1;rosso} normale)",
        R"(Normale {\c16744448;colore vero} normale)",
        R"(Normale {\W0.8;stretto} normale)",
        R"(Normale {\Q15;obliquo} normale)",
        R"(Normale {\T1.5;spaziato} normale)",
        R"(Frazione \S1/2; fine)",
        R"(Frazione \S1#2; fine)",
        R"(Frazione \S1^ 2; fine)",
        R"(Normale {\fFontInesistente|b0|i0|c0|p34;font che non c'e'} normale)",
        R"(Normale {\fArial|b0|i0|c0|p34;Arial} normale)",
        R"(Paragrafo con {\fArial|b1|i0|c0|p34;parole in grassetto}, {\H2x;una parola alta}, {\C1;un tratto rosso}, {\Lsottolineato}, {\W0.8;stretto}, {\fArial|b0|i1|c0|p34;corsivo} e una frazione \S3/4; che deve andare a capo dentro la colonna.)",
        R"({\C1;<>} {\fArial|b1|i0|c0|p34;mm})",
    };
    return kValues;
}

// --- Round trip -------------------------------------------------------------------

TEST(MTextEditModel, WithoutEditsTheValueIsTheSame) {
    for (const std::string& value : autocadValues()) {
        // The paragraph of the file is 60 wide; the dimension has 50.
        EXPECT_EQ(model(value, 60.0, std::string{"50"}).value(), value);
        EXPECT_EQ(model(value).value(), value);
    }
}

TEST(MTextEditModel, WritingAndDeletingAtEveryPositionGivesTheSameValue) {
    for (const std::string& value : autocadValues()) {
        MTextEditModel edited = model(value, 60.0, std::string{"50"});
        for (std::size_t position = 0; position < edited.positionCount(); ++position) {
            edited.setCaret(position);
            (void)edited.insertText("X");
            EXPECT_EQ(edited.caret(), position + 1) << value << " at " << position;
            EXPECT_EQ(edited.deleteBackward(), value) << value << " at " << position;
            EXPECT_EQ(edited.caret(), position);
        }
    }
}

// --- Writing and deleting near codes ----------------------------------------------

TEST(MTextEditModel, WritingInsideABoldGroupStaysBold) {
    const std::string value = R"(Normale {\fArial|b1|i0|c0|p34;grassetto} normale)";
    MTextEditModel edited = model(value);
    // "Normale " is 8 units: between 'g' and 'r' is position 9.
    edited.setCaret(9);
    EXPECT_TRUE(edited.formatAtCaret().bold);
    EXPECT_EQ(edited.insertText("X"), R"(Normale {\fArial|b1|i0|c0|p34;gXrassetto} normale)");
    EXPECT_TRUE(unitFormats(edited.value())[9].bold);
    // After the last letter of the group the text takes its format.
    edited.setCaret(18);  // after "grassetto" with the X
    EXPECT_EQ(edited.insertText("Y"), R"(Normale {\fArial|b1|i0|c0|p34;gXrassettoY} normale)");
    EXPECT_TRUE(unitFormats(edited.value())[18].bold);
}

TEST(MTextEditModel, DeletingTheLastCharacterOfAGroupLeavesTheGroup) {
    MTextEditModel edited = model(R"(A{\C1;x}B)");
    edited.setCaret(1);
    EXPECT_EQ(edited.deleteForward(), R"(A{\C1;}B)");
    EXPECT_EQ(edited.caret(), 1U);
    EXPECT_EQ(edited.units().size(), 2U);
}

TEST(MTextEditModel, BoldOnHalfAnItalicPieceIsBoldItalic) {
    MTextEditModel edited = model(R"(Normale {\fArial|b0|i1|c0|p34;corsivo} normale)");
    edited.select(8, 11);  // "cor"
    EXPECT_EQ(edited.applyFormat({.kind = Change::Bold, .on = true}),
              R"(Normale {\fArial|b0|i1|c0|p34;{\fArial|b1|i1;cor}sivo} normale)");
    const std::vector<MTextFormat> formats = unitFormats(edited.value());
    for (std::size_t i = 8; i < 11; ++i) {
        EXPECT_TRUE(formats[i].bold && formats[i].italic) << i;
    }
    EXPECT_FALSE(formats[11].bold);
    EXPECT_TRUE(formats[11].italic);
    EXPECT_FALSE(formats[7].bold || formats[7].italic);
    // The selection stays on the same units.
    EXPECT_EQ(edited.anchor(), 8U);
    EXPECT_EQ(edited.caret(), 11U);
}

TEST(MTextEditModel, AHiddenCodeStaysWhenTheCharactersNextToItGo) {
    MTextEditModel a = model(R"(A\A1;B)");
    EXPECT_EQ(a.units(),
              (std::vector<MTextEditUnit>{{0, 0, 1, Kind::Character}, {0, 5, 6, Kind::Character}}));
    a.setCaret(1);
    EXPECT_EQ(a.deleteBackward(), R"(\A1;B)");
    MTextEditModel b = model(R"(A\A1;B)");
    b.setCaret(1);
    EXPECT_EQ(b.deleteForward(), R"(A\A1;)");
    EXPECT_EQ(b.deleteBackward(), R"(\A1;)");
    EXPECT_TRUE(b.isEmpty());
}

TEST(MTextEditModel, DeletingAnEscapedCharacterTakesItsSevenBytes) {
    MTextEditModel edited = model(R"(é\U+0042)");
    EXPECT_EQ(edited.units(),
              (std::vector<MTextEditUnit>{{0, 0, 2, Kind::Character}, {0, 2, 9, Kind::Character}}));
    edited.setCaret(2);
    EXPECT_EQ(edited.deleteBackward(), "é");
}

TEST(MTextEditModel, TextBetweenTwoUnitsGoesBeforeTheHiddenCode) {
    // The choice of the editor: right after the unit before the cursor.
    MTextEditModel edited = model(R"(A\A1;B)");
    edited.setCaret(1);
    EXPECT_EQ(edited.insertText("X"), R"(AX\A1;B)");
    EXPECT_EQ(edited.caret(), 2U);
    // At the start of a paragraph, after the codes that open it.
    MTextEditModel start = model(R"({\C1;A})");
    EXPECT_EQ(start.insertText("X"), R"({\C1;XA})");
}

TEST(MTextEditModel, ACodeThatIsNotOneIsText) {
    // \Zx; is unknown: four units of text.
    MTextEditModel edited = model(R"(A\Zx;B)");
    EXPECT_EQ(edited.units().size(), 6U);
    edited.setCaret(2);
    EXPECT_EQ(edited.deleteForward(), R"(A\x;B)");
}

TEST(MTextEditModel, CharactersAreWrittenAsTheyAreShown) {
    MTextEditModel edited = model("");
    EXPECT_EQ(edited.insertText(R"(a\b{c})"), R"(a\\b\{c\})");
    EXPECT_EQ(edited.caret(), 6U);
    // "%%c" would be a diameter sign: written so that it shows as typed.
    MTextEditModel percent = model("");
    EXPECT_EQ(percent.insertText("%%c"), "%%%%%%c");
    EXPECT_EQ(percent.units().size(), 3U);
    EXPECT_EQ(percent.caret(), 3U);
}

TEST(MTextEditModel, AnEditThatWouldMakeACodeEscapesTheBackslash) {
    // Deleting Z of \ZL would make \L, an underline.
    MTextEditModel edited = model(R"(\ZL)");
    edited.setCaret(1);
    EXPECT_EQ(edited.deleteForward(), R"(\\L)");
    EXPECT_EQ(edited.units().size(), 2U);
    EXPECT_EQ(edited.caret(), 1U);
}

TEST(MTextEditModel, NewParagraphAndJoiningParagraphs) {
    MTextEditModel edited = model("AB");
    edited.setCaret(1);
    EXPECT_EQ(edited.newParagraph(), R"(A\PB)");
    // Positions: A | in the first paragraph (0, 1), | B | in the second (2, 3).
    EXPECT_EQ(edited.positionCount(), 4U);
    EXPECT_EQ(edited.caret(), 2U);
    EXPECT_EQ(edited.caretAt(2).row, 1U);
    EXPECT_EQ(edited.deleteBackward(), "AB");
    EXPECT_EQ(edited.caret(), 1U);
    (void)edited.newParagraph();
    edited.setCaret(1);
    EXPECT_EQ(edited.deleteForward(), "AB");
}

TEST(MTextEditModel, AnEmptyParagraphTakesTheFormatOfTheCodesBeforeIt) {
    MTextEditModel edited = model(R"({\C1;AB})");
    edited.setCaret(2);
    EXPECT_EQ(edited.newParagraph(), R"({\C1;AB\P})");
    EXPECT_EQ(edited.formatAtCaret().color, Color::fromIndex(1));
    EXPECT_EQ(edited.insertText("C"), R"({\C1;AB\PC})");
}

TEST(MTextEditModel, TextInPlaceOfASelectionTakesItsFormat) {
    MTextEditModel edited = model(R"(A{\C1;BC}D)");
    edited.select(1, 3);
    EXPECT_EQ(edited.insertText("X"), R"(A{\C1;X}D)");
    EXPECT_EQ(edited.caret(), 2U);
    EXPECT_FALSE(edited.hasSelection());
    // Across paragraphs: units and `\P` go, the codes stay.
    MTextEditModel across = model(R"(AB\P{\L}CD)");
    across.select(1, 4);
    EXPECT_EQ(across.deleteBackward(), R"(A{\L}D)");
    EXPECT_EQ(across.caret(), 1U);
}

// --- Formats ----------------------------------------------------------------------

TEST(MTextEditModel, EachFormatWritesItsCode) {
    const auto applied = [](const MTextFormatChange& change) {
        MTextEditModel edited = model("AB");
        edited.select(0, 1);
        return edited.applyFormat(change);
    };
    EXPECT_EQ(applied({.kind = Change::Italic}), R"({\fArial|b0|i1;A}B)");
    EXPECT_EQ(applied({.kind = Change::Underline}), R"({\LA}B)");
    EXPECT_EQ(applied({.kind = Change::Overline}), R"({\OA}B)");
    EXPECT_EQ(applied({.kind = Change::Strike}), R"({\KA}B)");
    EXPECT_EQ(applied({.kind = Change::Height, .number = 2.5}), R"({\H2.5;A}B)");
    EXPECT_EQ(applied({.kind = Change::WidthFactor, .number = 0.8}), R"({\W0.8;A}B)");
    EXPECT_EQ(applied({.kind = Change::Oblique, .number = 15.0}), R"({\Q15;A}B)");
    EXPECT_EQ(applied({.kind = Change::Tracking, .number = 1.5}), R"({\T1.5;A}B)");
    EXPECT_EQ(applied({.kind = Change::Color, .color = Color::fromIndex(1)}), R"({\C1;A}B)");
    EXPECT_EQ(applied({.kind = Change::Color, .color = Color::byBlock()}), R"({\C0;A}B)");
    EXPECT_EQ(applied({.kind = Change::Color, .color = Color::byLayer()}), R"({\C256;A}B)");
    // Blue in the high byte, as AutoCAD: RGB 0080FF is 16744448.
    EXPECT_EQ(applied({.kind = Change::Color, .color = Color::fromRgb({0x00, 0x80, 0xFF})}),
              R"({\c16744448;A}B)");
    // Outside the domain, or already so: nothing.
    EXPECT_EQ(applied({.kind = Change::Height, .number = 0.0}), "AB");
    EXPECT_EQ(applied({.kind = Change::Underline, .on = false}), "AB");
}

TEST(MTextEditModel, AFormatOverACodeIsWrittenOnEachSideOfIt) {
    // \H2; between B and C lasts after them: each stretch gets its group.
    MTextEditModel edited = model(R"(AB\H2;CD)");
    edited.select(1, 3);
    EXPECT_EQ(edited.applyFormat({.kind = Change::Underline}), R"(A{\LB}\H2;{\LC}D)");
    const std::vector<MTextFormat> formats = unitFormats(edited.value());
    EXPECT_DOUBLE_EQ(formats[3].height, 2.0);
    EXPECT_FALSE(formats[3].underline);
}

TEST(MTextEditModel, RemovingAFormatInsideAGroup) {
    MTextEditModel edited = model(R"({\LABC})");
    edited.select(1, 2);
    EXPECT_EQ(edited.applyFormat({.kind = Change::Underline, .on = false}), R"({\LA{\lB}C})");
    const std::vector<MTextFormat> formats = unitFormats(edited.value());
    EXPECT_TRUE(formats[0].underline);
    EXPECT_FALSE(formats[1].underline);
    EXPECT_TRUE(formats[2].underline);
}

TEST(MTextEditModel, AFormatWithoutSelectionIsForWhatIsWrittenNext) {
    MTextEditModel edited = model("AB");
    edited.setCaret(1);
    EXPECT_EQ(edited.applyFormat({.kind = Change::Bold}), "AB");
    EXPECT_EQ(edited.applyFormat({.kind = Change::Underline}), "AB");
    EXPECT_TRUE(edited.formatAtCaret().bold);
    EXPECT_TRUE(edited.formatAtCaret().underline);
    EXPECT_EQ(edited.insertText("X"), R"(A{\fArial|b1|i0;\LX}B)");
    // What follows is written in the group, after the X.
    EXPECT_EQ(edited.insertText("Y"), R"(A{\fArial|b1|i0;\LXY}B)");
    // Moving the cursor forgets a change not used.
    edited.setCaret(0);
    (void)edited.applyFormat({.kind = Change::Italic});
    edited.setCaret(1);
    EXPECT_FALSE(edited.formatAtCaret().italic);
}

TEST(MTextEditModel, StackAndUnstack) {
    MTextEditModel edited = model("x1/2y");
    edited.select(1, 4);
    EXPECT_EQ(edited.applyFormat({.kind = Change::Stack}), R"(x\S1/2;y)");
    EXPECT_EQ(edited.anchor(), 1U);
    EXPECT_EQ(edited.caret(), 2U);
    EXPECT_EQ(edited.applyFormat({.kind = Change::Stack, .on = false}), "x1/2y");
    EXPECT_EQ(edited.anchor(), 1U);
    EXPECT_EQ(edited.caret(), 4U);
    // A tolerance is written as AutoCAD saves it.
    MTextEditModel tolerance = model("1^2");
    tolerance.select(0, 3);
    EXPECT_EQ(tolerance.applyFormat({.kind = Change::Stack}), R"(\S1^ 2;)");
    // Without a separator nothing changes.
    MTextEditModel plain = model("12");
    plain.select(0, 2);
    EXPECT_EQ(plain.applyFormat({.kind = Change::Stack}), "12");
}

// --- Stacked fractions ------------------------------------------------------------

TEST(MTextEditModel, AStackedFractionIsOneUnit) {
    MTextEditModel edited = model(R"(x\S1/2;y)");
    EXPECT_EQ(edited.units(),
              (std::vector<MTextEditUnit>{
                  {0, 0, 1, Kind::Character}, {0, 1, 7, Kind::Stack}, {0, 7, 8, Kind::Character}}));
    EXPECT_EQ(edited.positionCount(), 4U);
    const double before = widthOf(U"x");
    const double after = before + widthOf(U"1");  // as wide as the wider text
    EXPECT_NEAR(edited.caretAt(1).point.x, before, kTolerance);
    EXPECT_NEAR(edited.caretAt(2).point.x, after, kTolerance);
    // A point above the middle of the fraction: before or after, never in it.
    const Vec2 above{(before + after) / 2.0, 0.0};
    const std::size_t closest = edited.closestPosition(above);
    EXPECT_TRUE(closest == 1 || closest == 2) << closest;
    edited.setCaret(1);
    EXPECT_EQ(edited.deleteForward(), "xy");
}

// --- Automatic breaks -------------------------------------------------------------

TEST(MTextEditModel, TheCursorOnAnAutomaticBreak) {
    MTextEditModel edited = model("AB CD", widthOf(U"AB"));
    ASSERT_EQ(edited.rowCount(), 2U);
    ASSERT_EQ(edited.positionCount(), 6U);
    // Before the space: at the end of the first row; after it: at the start
    // of the second.
    EXPECT_EQ(edited.caretAt(2).row, 0U);
    EXPECT_NEAR(edited.caretAt(2).point.x, widthOf(U"AB"), kTolerance);
    EXPECT_EQ(edited.caretAt(3).row, 1U);
    EXPECT_NEAR(edited.caretAt(3).point.x, 0.0, kTolerance);
    EXPECT_NEAR(edited.caretAt(3).point.y, edited.caretAt(0).point.y - (5.0 / 3.0), kTolerance);
    // Written after the space, the text goes on the second row.
    edited.setCaret(3);
    EXPECT_EQ(edited.insertText("X"), "AB XCD");
    EXPECT_EQ(edited.caret(), 4U);
    EXPECT_EQ(edited.caretAt(4).row, 1U);
    EXPECT_NEAR(edited.caretAt(4).point.x, widthOf(U"X"), kTolerance);
    // Written before it, on the first row, which no longer fits.
    edited.setCaret(2);
    EXPECT_EQ(edited.insertText("Y"), "ABY XCD");
    EXPECT_EQ(edited.caret(), 3U);
    EXPECT_EQ(edited.caretAt(3).row, 0U);
    EXPECT_NEAR(edited.caretAt(3).point.x, widthOf(U"ABY"), kTolerance);
    // Home and End.
    EXPECT_EQ(edited.rowStart(0), 0U);
    EXPECT_EQ(edited.rowEnd(0), 3U);
    EXPECT_EQ(edited.rowStart(1), 4U);
    EXPECT_EQ(edited.rowEnd(1), 7U);
}

TEST(MTextEditModel, TheSpaceOfABreakIsDeletedFromBothSides) {
    const double width = widthOf(U"A");
    MTextEditModel forward = model("A B", width);
    ASSERT_EQ(forward.rowCount(), 2U);
    forward.setCaret(1);
    EXPECT_EQ(forward.deleteForward(), "AB");
    MTextEditModel backward = model("A B", width);
    backward.setCaret(2);
    EXPECT_EQ(backward.deleteBackward(), "AB");
    // Two spaces: one per key, either way.
    MTextEditModel twoForward = model("A  B", width);
    twoForward.setCaret(1);
    EXPECT_EQ(twoForward.deleteForward(), "A B");
    MTextEditModel twoBackward = model("A  B", width);
    twoBackward.setCaret(3);
    EXPECT_EQ(twoBackward.deleteBackward(), "A B");
    // A selection across them takes both.
    MTextEditModel both = model("A  B", width);
    both.select(1, 3);
    EXPECT_EQ(both.deleteForward(), "AB");
}

// --- The measurement of a dimension -----------------------------------------------

TEST(MTextEditModel, TheMeasurementIsOneUnit) {
    MTextEditModel edited = model("<> mm", 0.0, std::string{"1234"});
    EXPECT_EQ(edited.shown().value, "1234 mm");
    EXPECT_EQ(edited.units(), (std::vector<MTextEditUnit>{{0, 0, 2, Kind::Measurement},
                                                          {0, 2, 3, Kind::Character},
                                                          {0, 3, 4, Kind::Character},
                                                          {0, 4, 5, Kind::Character}}));
    // Two positions at the edges of 1234, then between the letters of " mm".
    ASSERT_EQ(edited.positionCount(), 5U);
    EXPECT_NEAR(edited.caretAt(0).point.x, 0.0, kTolerance);
    EXPECT_NEAR(edited.caretAt(1).point.x, widthOf(U"1234"), kTolerance);
    EXPECT_NEAR(edited.caretAt(2).point.x, widthOf(U"1234 "), kTolerance);
    // A click between 2 and 3 goes to the nearer edge.
    const std::size_t click = edited.closestPosition(Vec2{widthOf(U"12"), 0.5});
    EXPECT_TRUE(click == 0 || click == 1) << click;
    EXPECT_EQ(edited.value(), "<> mm");
    EXPECT_EQ(edited.deleteForward(), " mm");
    EXPECT_EQ(edited.shown().value, " mm");
}

TEST(MTextEditModel, OnlyTheFirstPlaceIsTheMeasurement) {
    const MTextEditModel edited = model("<> / <>", 0.0, std::string{"30"});
    EXPECT_EQ(edited.shown().value, "30 / <>");
    const std::vector<MTextEditUnit> units = edited.units();
    ASSERT_EQ(units.size(), 6U);
    EXPECT_EQ(units[0], (MTextEditUnit{0, 0, 2, Kind::Measurement}));
    EXPECT_EQ(units[4], (MTextEditUnit{0, 5, 6, Kind::Character}));
    EXPECT_EQ(units[5], (MTextEditUnit{0, 6, 7, Kind::Character}));
}

TEST(MTextEditModel, WritingThePlaceShowsTheMeasurement) {
    MTextEditModel edited = model("FISSO", 0.0, std::string{"30"});
    EXPECT_EQ(edited.shown().value, "FISSO");
    edited.select(0, 5);
    EXPECT_EQ(edited.insertText("<>"), "<>");
    EXPECT_EQ(edited.shown().value, "30");
    EXPECT_EQ(edited.units(), (std::vector<MTextEditUnit>{{0, 0, 2, Kind::Measurement}}));
    EXPECT_EQ(edited.caret(), 1U);
}

TEST(MTextEditModel, AMeasurementOnThreeRowsHasTwoPositions) {
    // DIMPOST "<> mm\Pnominale\Pquota" with the value <>.
    MTextEditModel edited = model("<>", 0.0, std::string{R"(30 mm\Pnominale\Pquota)"});
    ASSERT_EQ(edited.rowCount(), 3U);
    ASSERT_EQ(edited.positionCount(), 2U);
    EXPECT_EQ(edited.caretAt(0).row, 0U);
    EXPECT_NEAR(edited.caretAt(0).point.x, 0.0, kTolerance);
    EXPECT_EQ(edited.caretAt(1).row, 2U);
    EXPECT_NEAR(edited.caretAt(1).point.x, widthOf(U"quota"), kTolerance);
    // A click on the row "nominale", and Home and End on it.
    const double middle = edited.caretAt(0).point.y - (5.0 / 3.0);
    const std::size_t click = edited.closestPosition(Vec2{widthOf(U"nomi"), middle + 0.5});
    EXPECT_TRUE(click == 0 || click == 1) << click;
    EXPECT_EQ(edited.rowStart(1), 0U);
    EXPECT_EQ(edited.rowEnd(1), 1U);
    // Written after and before.
    edited.setCaret(1);
    EXPECT_EQ(edited.insertText("X"), "<>X");
    MTextEditModel before = model("<>", 0.0, std::string{R"(30 mm\Pnominale\Pquota)"});
    EXPECT_EQ(before.insertText("X"), "X<>");
}

TEST(MTextEditModel, TheDimensionOfTheFileKeepsItsCodes) {
    MTextEditModel edited = model(R"({\C1;<>} {\fArial|b1|i0|c0|p34;mm})", 0.0, std::string{"50"});
    EXPECT_EQ(edited.shown().value, R"({\C1;50} {\fArial|b1|i0|c0|p34;mm})");
    EXPECT_EQ(edited.units().front(), (MTextEditUnit{0, 5, 7, Kind::Measurement}));
    // Deleting the measurement leaves its group.
    EXPECT_EQ(edited.deleteForward(), R"({\C1;} {\fArial|b1|i0|c0|p34;mm})");
}

// --- Empty ------------------------------------------------------------------------

TEST(MTextEditModel, EmptyMeansNoUnitLeft) {
    MTextEditModel edited = model(R"(A\A1;B)");
    EXPECT_FALSE(edited.isEmpty());
    edited.select(0, 2);
    EXPECT_EQ(edited.deleteBackward(), R"(\A1;)");
    EXPECT_TRUE(edited.isEmpty());
    EXPECT_TRUE(model("").isEmpty());
    EXPECT_TRUE(model(R"({\L})").isEmpty());
    EXPECT_FALSE(model(" ").isEmpty());
    // The measurement and " mm" deleted: the hidden code stays.
    MTextEditModel dimension = model(R"(<>\A1; mm)", 0.0, std::string{"30"});
    EXPECT_FALSE(dimension.isEmpty());
    dimension.select(0, 4);
    EXPECT_EQ(dimension.deleteForward(), R"(\A1;)");
    EXPECT_TRUE(dimension.isEmpty());
}

// --- Where the positions are ------------------------------------------------------

TEST(MTextEditModel, ThePositionsTurnWithTheText) {
    MText rotated = mtext("AB");
    rotated.insertion = {10.0, 20.0};
    rotated.rotation = degreesToRadians(90.0);
    const MTextEditModel edited(rotated);
    // Top left: the baseline 1 below the insertion point, along -x once turned.
    const MTextCaret caret = edited.caretAt(1);
    EXPECT_NEAR(caret.point.x, 10.0 + 1.0, kTolerance);
    EXPECT_NEAR(caret.point.y, 20.0 + widthOf(U"A"), kTolerance);
    EXPECT_DOUBLE_EQ(caret.height, 1.0);
    EXPECT_EQ(edited.closestPosition(caret.point), 1U);
}

TEST(MTextEditModel, ThePositionsFollowTheAttachment) {
    MText centred = mtext(R"(AB\P)");
    centred.attachment = MTextAttachment::TopCenter;
    const MTextEditModel edited(centred);
    EXPECT_NEAR(edited.caretAt(0).point.x, -widthOf(U"AB") / 2.0, kTolerance);
    EXPECT_NEAR(edited.caretAt(2).point.x, widthOf(U"AB") / 2.0, kTolerance);
    // The empty paragraph after it: in the middle of its row.
    EXPECT_EQ(edited.caretAt(3).row, 1U);
    EXPECT_NEAR(edited.caretAt(3).point.x, 0.0, kTolerance);
}

TEST(MTextEditModel, AtEqualDistanceTheFirstPositionWins) {
    const MTextEditModel edited = model("AA");
    // Halfway between the two positions around the first A.
    EXPECT_EQ(edited.closestPosition(Vec2{widthOf(U"A") / 2.0, 0.0}), 0U);
}

}  // namespace
}  // namespace tg::core
