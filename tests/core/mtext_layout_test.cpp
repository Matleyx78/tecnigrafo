// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The layout of the pieces of an MTEXT (docs/adr/0054, point 3;
// docs/tappe/0.8.1/PT8-impaginazione-formati.md, points 3 to 6).

#include "mtext_layout.hpp"

#include "tg/core/mtext.hpp"
#include "tg/core/text_metrics.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-12;

MText mtext(std::string value, double height = 1.0, double width = 0.0) {
    MText result;
    result.height = height;
    result.width = width;
    result.value = std::move(value);
    return result;
}

double widthOf(std::u32string_view text, double height = 1.0) {
    return textWidth(text) * height;
}

void expectBoundaries(const std::vector<double>& actual, const std::vector<double>& expected) {
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        EXPECT_NEAR(actual[i], expected[i], kTolerance) << i;
    }
}

// --- Units laid out, for the editor (#927, [bloccante] 4) -----------------------

TEST(MTextLayoutUnits, TwoUnitsAndThreeBoundariesAcrossAHiddenCode) {
    // \A1; is hidden and changes no format: A and B are one piece.
    const mtext_layout::Layout layout = mtext_layout::layout(mtext(R"(A\A1;B)"), 1.0);
    ASSERT_EQ(layout.rows.size(), 1U);
    ASSERT_EQ(layout.rows[0].pieces.size(), 1U);
    const mtext_layout::Piece& piece = layout.rows[0].pieces[0];
    EXPECT_EQ(piece.paragraph, 0U);
    EXPECT_EQ(piece.run, 0U);
    EXPECT_EQ(piece.firstUnit, 0U);
    EXPECT_EQ(piece.unitCount, 2U);
    EXPECT_EQ(piece.text, U"AB");
    expectBoundaries(piece.boundaries, {0.0, widthOf(U"A"), widthOf(U"AB")});
    // Each unit is one of the piece of PT7.
    EXPECT_EQ(layout.runOf(piece).units.size(), 2U);
}

TEST(MTextLayoutUnits, AfterAnAutomaticBreakTheFirstUnitOfTheSecondRow) {
    const mtext_layout::Layout layout =
        mtext_layout::layout(mtext(R"(AB C{\C1;D})", 1.0, widthOf(U"AB")), 1.0);
    ASSERT_EQ(layout.rows.size(), 2U);
    const std::vector<mtext_layout::Piece>& second = layout.rows[1].pieces;
    ASSERT_EQ(second.size(), 2U);
    // C is unit 3 of the first piece (A, B, the space where the row breaks).
    EXPECT_EQ(second[0].run, 0U);
    EXPECT_EQ(second[0].firstUnit, 3U);
    EXPECT_EQ(second[0].unitCount, 1U);
    expectBoundaries(second[0].boundaries, {0.0, widthOf(U"C")});
    EXPECT_EQ(second[1].run, 1U);
    EXPECT_EQ(second[1].firstUnit, 0U);
    expectBoundaries(second[1].boundaries, {widthOf(U"C"), widthOf(U"CD")});
    EXPECT_EQ(layout.rows[0].text, U"AB");
    EXPECT_EQ(layout.rows[1].text, U"CD");
}

TEST(MTextLayoutUnits, AStackIsOneUnitWithTwoBoundaries) {
    const mtext_layout::Layout layout = mtext_layout::layout(mtext(R"(A\S1/22;B)"), 1.0);
    ASSERT_EQ(layout.rows.size(), 1U);
    const std::vector<mtext_layout::Piece>& pieces = layout.rows[0].pieces;
    ASSERT_EQ(pieces.size(), 3U);
    const mtext_layout::Piece& stack = pieces[1];
    EXPECT_EQ(stack.unitCount, 1U);
    EXPECT_TRUE(stack.text.empty());
    EXPECT_FALSE(stack.blank);
    // As wide as the wider of its two texts (rule 1).
    EXPECT_NEAR(stack.width, widthOf(U"22"), kTolerance);
    expectBoundaries(stack.boundaries, {widthOf(U"A"), widthOf(U"A") + widthOf(U"22")});
    EXPECT_NEAR(pieces[2].x, widthOf(U"A") + widthOf(U"22"), kTolerance);
    EXPECT_EQ(layout.rows[0].text, U"A1/22B");
}

// --- Rows (rules 1 to 4) --------------------------------------------------------

TEST(MTextLayoutRows, WidthsByFaceWidthFactorAndTracking) {
    const mtext_layout::Layout layout =
        mtext_layout::layout(mtext(R"({\fArial|b1;A}{\W2;B}{\T3;C}{\H2;D})", 1.0), 1.0);
    ASSERT_EQ(layout.rows.size(), 1U);
    const std::vector<mtext_layout::Piece>& pieces = layout.rows[0].pieces;
    ASSERT_EQ(pieces.size(), 4U);
    EXPECT_NEAR(pieces[0].width, textWidth(U"A", FontFace::Bold), kTolerance);
    EXPECT_NEAR(pieces[1].width, 2.0 * widthOf(U"B"), kTolerance);
    EXPECT_NEAR(pieces[2].width, 3.0 * widthOf(U"C"), kTolerance);
    EXPECT_NEAR(pieces[3].width, widthOf(U"D", 2.0), kTolerance);
    EXPECT_EQ(layout.rows[0].height, 2.0);
}

TEST(MTextLayoutRows, ARowOfOneRegularPieceIsMeasuredAsInStage08) {
    // textWidth() of the row times height and width factor, bit for bit.
    const mtext_layout::Layout layout = mtext_layout::layout(mtext("Normale testo", 2.5), 1.3);
    ASSERT_EQ(layout.rows.size(), 1U);
    EXPECT_EQ(mtext_layout::rowWidth(layout.rows[0]), textWidth(U"Normale testo") * (2.5 * 1.3));
}

TEST(MTextLayoutRows, FirstBaselineAndLineSpacingWithMixedHeights) {
    // Rule 3: below the top by the highest piece of the first row; rule 4:
    // 5/3 of the highest piece of the row below.
    const mtext_layout::Layout layout =
        mtext_layout::layout(mtext(R"(A{\H2x;B}\PC\P{\H3;D})", 1.0), 1.0);
    ASSERT_EQ(layout.rows.size(), 3U);
    EXPECT_NEAR(layout.rows[0].start.y, -2.0, kTolerance);
    EXPECT_NEAR(layout.rows[1].start.y, -2.0 - (5.0 / 3.0), kTolerance);
    EXPECT_NEAR(layout.rows[2].start.y, -2.0 - (5.0 / 3.0) - 5.0, kTolerance);
    // From the top to the last baseline, as the dimensions measure it.
    EXPECT_NEAR(mtext_layout::columnHeight(layout), 2.0 + (5.0 / 3.0) + 5.0, kTolerance);
}

TEST(MTextLayoutRows, AFractionCountsItsNominalHeight) {
    const mtext_layout::Layout layout = mtext_layout::layout(mtext(R"(\S1#2;)", 2.5), 1.0);
    ASSERT_EQ(layout.rows.size(), 1U);
    EXPECT_EQ(layout.rows[0].height, 2.5);
    EXPECT_EQ(mtext_layout::columnHeight(layout), 2.5);
}

TEST(MTextLayoutRows, TheOverlineOfTheFirstRowLowersTheBaseline) {
    const mtext_layout::Layout layout = mtext_layout::layout(mtext(R"(A{\OB}\PC)", 2.5), 1.0);
    ASSERT_EQ(layout.rows.size(), 2U);
    EXPECT_NEAR(layout.top, 3.0, kTolerance);
    EXPECT_NEAR(layout.rows[0].start.y, -3.0, kTolerance);
    // The rows below keep the distance of the 0.8.
    EXPECT_NEAR(layout.rows[1].start.y, -3.0 - (5.0 / 3.0 * 2.5), kTolerance);
}

TEST(MTextLayoutRows, EmptyRowsStayInTheLayout) {
    const mtext_layout::Layout layout = mtext_layout::layout(mtext(R"(A\P\PB\P)"), 1.0);
    ASSERT_EQ(layout.rows.size(), 4U);
    EXPECT_FALSE(layout.rows[0].blank);
    EXPECT_TRUE(layout.rows[1].blank);
    EXPECT_FALSE(layout.rows[2].blank);
    EXPECT_TRUE(layout.rows[3].blank);
    EXPECT_NEAR(layout.rows[2].start.y, -1.0 - (2.0 * 5.0 / 3.0), kTolerance);
}

TEST(MTextLayoutRows, TheBreakInTheBandIsDeterministic) {
    // "A A" at height 3 is exactly 9903/1409 wide: one row, as in the 0.8.
    const double width = 9903.0 / 1409.0;
    EXPECT_EQ(mtext_layout::layout(mtext("A A", 3.0, width), 1.0).rows.size(), 1U);
    // Two pieces: within 4 epsilon of the width, either way, the same twice.
    const MText coloured = mtext(R"(A {\C1;A})", 3.0, width);
    const mtext_layout::Layout first = mtext_layout::layout(coloured, 1.0);
    const mtext_layout::Layout second = mtext_layout::layout(coloured, 1.0);
    ASSERT_EQ(first.rows.size(), second.rows.size());
    for (std::size_t i = 0; i < first.rows.size(); ++i) {
        EXPECT_EQ(first.rows[i].text, second.rows[i].text);
        EXPECT_EQ(first.rows[i].start, second.rows[i].start);
    }
}

TEST(MTextLayoutRows, FaceOfAFormat) {
    MTextFormat format;
    EXPECT_EQ(mtext_layout::faceOf(format), FontFace::Regular);
    format.bold = true;
    EXPECT_EQ(mtext_layout::faceOf(format), FontFace::Bold);
    format.italic = true;
    EXPECT_EQ(mtext_layout::faceOf(format), FontFace::BoldItalic);
    format.bold = false;
    EXPECT_EQ(mtext_layout::faceOf(format), FontFace::Italic);
}

}  // namespace
}  // namespace tg::core
