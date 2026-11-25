// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/mtext.hpp"
#include "tg/core/mtext_format.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

/// The plain text and the layout of an MTEXT (docs/adr/0046, point 2;
/// docs/tappe/0.8/PT13-mtext-core.md, points 2 and 3; docs/adr/0054,
/// point 3; docs/tappe/0.8.1/PT8-impaginazione-formati.md). Private to core.
namespace tg::core::mtext_layout {

/// The no-break space of `\~` inside a decoded paragraph. It lies outside
/// Unicode, so no character of the value can be taken for it (decodeUtf8()
/// gives U+FFFD for what is not a character); it is measured and shown as a
/// normal space.
inline constexpr char32_t kNoBreakSpace = 0x110000;

/// The paragraphs of `value`, its formatting codes decoded once
/// (PT13, point 2): what is left is shown as it is, without a second pass of
/// displayText(). A code with an argument ends at the first ';' before the
/// next `\P`: without one it stays literal, and a `\P` after it still starts
/// a paragraph. There is always at least one paragraph, and one more after
/// every `\P`.
[[nodiscard]] std::vector<std::u32string> paragraphs(std::string_view value);

/// A piece laid out on a row (docs/adr/0054, point 3;
/// docs/tappe/0.8.1/PT8-impaginazione-formati.md, point 3): the part of a
/// piece of PT7 that falls on the row, and where each of its units lies, for
/// the editor (PT10).
struct Piece {
    std::size_t paragraph{0};  ///< index in Layout::paragraphs
    std::size_t run{0};        ///< index of its MTextRun in the paragraph
    std::size_t firstUnit{0};  ///< index of its first unit in MTextRun::units
    std::size_t unitCount{0};  ///< 1 for a stacked fraction
    /// Its characters, `\~` made a normal space; empty for a stack.
    std::u32string text;
    double x{0.0};      ///< start, in the system of Row::start
    double width{0.0};  ///< rule 1 of docs/adr/0054, point 3
    /// unitCount + 1 abscissas, in the system of Row::start: the start of
    /// the piece, every boundary between two units, its end. A stacked
    /// fraction has two: before and after.
    std::vector<double> boundaries;
    bool blank{true};  ///< only spaces: no text for it, only its lines
};

/// One laid-out row of the column.
struct Row {
    /// What the row shows, plain: its pieces joined in reading order, with
    /// kNoBreakSpace made a normal space and a stack as "top/bottom".
    std::u32string text;
    Vec2 start;          ///< start of the baseline, relative to the insertion
                         ///< point, before the rotation
    bool blank{true};    ///< empty or only spaces: nothing drawn for it
    double height{0.0};  ///< nominal: the greatest height of its pieces
    std::vector<Piece> pieces;
};

/// The layout of an MTEXT: the pieces of PT7 it starts from and its rows.
struct Layout {
    std::vector<MTextParagraph> paragraphs;  ///< formattedParagraphs() of the value
    std::vector<Row> rows;                   ///< at least one
    /// From the top of the column to the first baseline: the nominal height
    /// of the first row, plus 0.2 times the height of its highest overlined
    /// piece (docs/adr/0054, point 3, rule 3).
    double top{0.0};

    /// The format of `piece`.
    [[nodiscard]] const MTextRun& runOf(const Piece& piece) const {
        return paragraphs[piece.paragraph][piece.run];
    }
};

/// The layout of `mtext` (docs/adr/0054, point 3, on PT13 of the 0.8), with
/// `widthFactor` and `oblique` those of its style (1 and 0 without a style).
/// A paragraph breaks at the normal spaces before the word that would carry
/// the row beyond the reference width (strictly greater); a word made of
/// pieces with different formats moves whole. A row with one Regular piece
/// is measured as in stage 0.8, so an MTEXT without formats keeps its
/// geometry.
[[nodiscard]] Layout layout(const MText& mtext, double widthFactor, double oblique = 0.0);

/// The width of `row` (rule 1, summed over its pieces): 0 for a blank row.
[[nodiscard]] double rowWidth(const Row& row);

/// The distance from the top of the column to the baseline of its last row
/// (the top is the insertion point of a top attachment): the nominal height
/// of the first row, the overline over it, and the distances between the
/// baselines.
[[nodiscard]] double columnHeight(const Layout& layout);

/// The face of a format: Regular, Bold, Italic or BoldItalic.
[[nodiscard]] FontFace faceOf(const MTextFormat& format);

/// `row` written so that displayText() gives it back unchanged (PT13,
/// point 4): every '%' that forms "%%c", "%%d", "%%p" or "%%%" (in either
/// case) with the two characters after it in the row is written "%%%"; the
/// other characters stay as they are. "%%c" becomes "%%%%c", "%%%" becomes
/// "%%%%%", "%c" and "50%" stay as they are.
[[nodiscard]] std::u32string literalValue(std::u32string_view row);

/// `text` as UTF-8; a code point that is not a character (a surrogate, or
/// beyond U+10FFFF) becomes U+FFFD, as decodeUtf8() does.
[[nodiscard]] std::string toUtf8(std::u32string_view text);

}  // namespace tg::core::mtext_layout
