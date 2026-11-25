// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "mtext_layout.hpp"

#include "tg/core/mtext_format.hpp"
#include "tg/core/text_metrics.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::core::mtext_layout {
namespace {

constexpr char32_t kReplacement = U'�';

// The baseline of the next row is 5/3 of the height below, times the line
// spacing factor (docs/adr/0013 and docs/adr/0046, point 2, as AutoCAD;
// docs/adr/0054, point 3, rule 4).
constexpr double kBaselineDistance = 5.0 / 3.0;

// "%%" and the character of a DXF control code (docs/adr/0013, point 5).
constexpr std::size_t kPercentCodeLength = 3;

constexpr char32_t kFirstSurrogate = 0xD800;
constexpr char32_t kLastSurrogate = 0xDFFF;
constexpr char32_t kLastCodePoint = 0x10FFFF;

/// The character of "%%<code>", as displayText() converts it, in either case.
std::optional<char32_t> percentCharacter(char32_t code) {
    switch (code) {
        case U'c':
        case U'C':
            return U'⌀';  // diameter sign
        case U'd':
        case U'D':
            return U'°';  // degree sign
        case U'p':
        case U'P':
            return U'±';  // plus-minus sign
        case U'%':
            return U'%';
        default:
            return std::nullopt;
    }
}

/// The `\~` of `unit`: the no-break space that the paragraphs of the layout
/// keep as kNoBreakSpace, unlike a U+00A0 written in the value.
bool isNoBreakCode(std::string_view value, const MTextUnit& unit) {
    return value.substr(unit.sourceBegin, unit.sourceEnd - unit.sourceBegin) == "\\~";
}

/// A stacked fraction shown on one line, as in stage 0.8: the characters
/// between `\S` and ';' as written, with "1^2", "1#2" and "1/2" all "1/2".
void appendFlatStack(std::string_view value, const MTextUnit& unit, std::u32string& out) {
    constexpr std::size_t kOpening = 2;  // "\S"
    constexpr std::size_t kClosing = 1;  // ";"
    const std::size_t length = unit.sourceEnd - unit.sourceBegin - kOpening - kClosing;
    for (const char32_t part : decodeUtf8(value.substr(unit.sourceBegin + kOpening, length))) {
        out += part == U'^' || part == U'#' ? U'/' : part;
    }
}

// The overline over the first row, in units of the height of the highest
// overlined piece (docs/adr/0054, point 3, rule 3).
constexpr double kOverlineExtra = 0.2;

/// One editable unit of a paragraph: a character of a piece, or a whole
/// stacked fraction.
struct Unit {
    std::size_t run{0};
    std::size_t index{0};   ///< in MTextRun::units
    char32_t character{0};  ///< as shown and measured: `\~` a normal space; 0 for a stack
    bool breakable{false};  ///< a normal space, where a row can break
};

std::vector<Unit> unitsOf(std::string_view value, const MTextParagraph& paragraph) {
    std::vector<Unit> units;
    for (std::size_t r = 0; r < paragraph.size(); ++r) {
        const MTextRun& run = paragraph[r];
        if (run.stack) {
            units.push_back({r, 0, 0, false});
            continue;
        }
        for (std::size_t k = 0; k < run.text.size(); ++k) {
            const bool noBreak = isNoBreakCode(value, run.units[k]);
            const char32_t character = noBreak ? U' ' : run.text[k];
            units.push_back({r, k, character, !noBreak && run.text[k] == U' '});
        }
    }
    return units;
}

/// The scale of the advances of a piece: height times width factor, as
/// stage 0.8 multiplies a row, then the tracking.
double advanceScale(const MTextFormat& format) {
    return format.height * format.widthFactor;
}

/// Rule 1: the advances of the face times height, width factor and
/// tracking; a stacked fraction as wide as the wider of its two texts.
double pieceWidth(const MTextRun& run, std::u32string_view text) {
    const FontFace face = faceOf(run.format);
    const double advances =
        run.stack ? std::max(textWidth(run.stack->top, face), textWidth(run.stack->bottom, face))
                  : textWidth(text, face);
    return advances * advanceScale(run.format) * run.format.tracking;
}

/// The width of units [first, last) of a paragraph: the sum of the widths
/// of its pieces, a piece for each run.
double unitsWidth(const MTextParagraph& paragraph, const std::vector<Unit>& units,
                  std::size_t first, std::size_t last) {
    double width = 0.0;
    std::size_t i = first;
    while (i < last) {
        const std::size_t run = units[i].run;
        std::u32string text;
        while (i < last && units[i].run == run) {
            text += units[i].character;
            ++i;
        }
        width += pieceWidth(paragraph[run], text);
    }
    return width;
}

/// The rows of one paragraph, as ranges of its units: one without a positive
/// width, otherwise broken before the word that would carry the row beyond
/// it (PT13, point 3; docs/adr/0054, point 3, rule 2). The spaces where the
/// row breaks stay in neither row; those at the end of the paragraph stay.
std::vector<std::vector<std::size_t>> wrap(const MTextParagraph& paragraph,
                                           const std::vector<Unit>& units, double width) {
    std::vector<std::vector<std::size_t>> rows;
    std::vector<std::size_t> row;
    bool hasWord = false;
    std::size_t i = 0;
    while (i < units.size()) {
        std::vector<std::size_t> spaces;
        while (i < units.size() && units[i].breakable) {
            spaces.push_back(i++);
        }
        std::vector<std::size_t> word;
        while (i < units.size() && !units[i].breakable) {
            word.push_back(i++);
        }
        if (word.empty()) {
            row.insert(row.end(), spaces.begin(), spaces.end());
            continue;
        }
        std::vector<std::size_t> candidate = row;
        candidate.insert(candidate.end(), spaces.begin(), spaces.end());
        candidate.insert(candidate.end(), word.begin(), word.end());
        // A row is a contiguous range of units: its width is that of the
        // range, piece by piece.
        if (width > 0.0 && hasWord &&
            unitsWidth(paragraph, units, candidate.front(), candidate.back() + 1) > width) {
            rows.push_back(std::move(row));
            row = std::move(word);
        } else {
            row = std::move(candidate);
        }
        hasWord = true;
    }
    rows.push_back(std::move(row));
    return rows;
}

/// The row of `indices`, contiguous units of paragraph `p`, its pieces at
/// abscissas from 0.
Row rowOf(const Layout& layout, std::size_t p, const std::vector<Unit>& units,
          const std::vector<std::size_t>& indices, double baseHeight) {
    const MTextParagraph& paragraph = layout.paragraphs[p];
    Row row;
    row.height = 0.0;
    double x = 0.0;
    std::size_t i = 0;
    while (i < indices.size()) {
        const std::size_t run = units[indices[i]].run;
        const MTextRun& source = paragraph[run];
        Piece piece;
        piece.paragraph = p;
        piece.run = run;
        piece.firstUnit = units[indices[i]].index;
        piece.x = x;
        const FontFace face = faceOf(source.format);
        const double scale = advanceScale(source.format);
        piece.boundaries.push_back(x);
        double advances = 0.0;  // summed as textWidth() sums them
        while (i < indices.size() && units[indices[i]].run == run) {
            const Unit& unit = units[indices[i]];
            if (source.stack) {
                piece.blank = false;
                row.text += source.stack->top + U"/" + source.stack->bottom;
            } else {
                piece.text += unit.character;
                piece.blank = piece.blank && unit.character == U' ';
                advances += advanceWidth(unit.character, face);
                piece.boundaries.push_back(x + (advances * scale * source.format.tracking));
                row.text += unit.character;
            }
            ++piece.unitCount;
            ++i;
        }
        piece.width = pieceWidth(source, piece.text);
        if (source.stack) {
            piece.boundaries.push_back(x + piece.width);
        }
        x += piece.width;
        row.blank = row.blank && piece.blank;
        row.height = std::max(row.height, source.format.height);
        row.pieces.push_back(std::move(piece));
    }
    if (row.pieces.empty()) {
        row.height = baseHeight;
    }
    return row;
}

void appendUtf8(char32_t code, std::string& out) {
    constexpr char32_t kOneByte = 0x80;
    constexpr char32_t kTwoBytes = 0x800;
    constexpr char32_t kThreeBytes = 0x10000;
    constexpr unsigned kSixBits = 6;
    constexpr char32_t kLowSix = 0x3F;
    constexpr char32_t kContinuation = 0x80;
    constexpr char32_t kLead2 = 0xC0;
    constexpr char32_t kLead3 = 0xE0;
    constexpr char32_t kLead4 = 0xF0;
    if ((code >= kFirstSurrogate && code <= kLastSurrogate) || code > kLastCodePoint) {
        code = kReplacement;
    }
    const auto put = [&out](char32_t byte) {
        out += static_cast<char>(byte);
    };
    if (code < kOneByte) {
        put(code);
    } else if (code < kTwoBytes) {
        put(kLead2 | (code >> kSixBits));
        put(kContinuation | (code & kLowSix));
    } else if (code < kThreeBytes) {
        put(kLead3 | (code >> (2 * kSixBits)));
        put(kContinuation | ((code >> kSixBits) & kLowSix));
        put(kContinuation | (code & kLowSix));
    } else {
        put(kLead4 | (code >> (3 * kSixBits)));
        put(kContinuation | ((code >> (2 * kSixBits)) & kLowSix));
        put(kContinuation | ((code >> kSixBits) & kLowSix));
        put(kContinuation | (code & kLowSix));
    }
}

}  // namespace

std::vector<std::u32string> paragraphs(std::string_view value) {
    // The pieces of PT7 joined again: the formats do not count here yet.
    std::vector<std::u32string> result;
    for (const MTextParagraph& paragraph : formattedParagraphs(value, MTextFormat{})) {
        std::u32string& current = result.emplace_back();
        for (const MTextRun& run : paragraph) {
            if (run.stack) {
                appendFlatStack(value, run.units.front(), current);
                continue;
            }
            for (std::size_t k = 0; k < run.text.size(); ++k) {
                current += isNoBreakCode(value, run.units[k]) ? kNoBreakSpace : run.text[k];
            }
        }
    }
    return result;
}

FontFace faceOf(const MTextFormat& format) {
    if (format.bold) {
        return format.italic ? FontFace::BoldItalic : FontFace::Bold;
    }
    return format.italic ? FontFace::Italic : FontFace::Regular;
}

double rowWidth(const Row& row) {
    if (row.blank) {
        return 0.0;  // a blank row counts as an empty one (PT13, point 3)
    }
    double width = 0.0;
    for (const Piece& piece : row.pieces) {
        width += piece.width;
    }
    return width;
}

double columnHeight(const Layout& layout) {
    return layout.rows.front().start.y - layout.rows.back().start.y + layout.top;
}

Layout layout(const MText& mtext, double widthFactor, double oblique) {
    MTextFormat base;
    base.height = mtext.height;
    base.widthFactor = widthFactor;
    base.oblique = oblique;
    Layout result;
    result.paragraphs = formattedParagraphs(mtext.value, base);
    std::vector<Row>& rows = result.rows;
    for (std::size_t p = 0; p < result.paragraphs.size(); ++p) {
        const std::vector<Unit> units = unitsOf(mtext.value, result.paragraphs[p]);
        for (const std::vector<std::size_t>& indices :
             wrap(result.paragraphs[p], units, mtext.width)) {
            rows.push_back(rowOf(result, p, units, indices, mtext.height));
        }
    }

    // Vertical (rules 3 and 4): the first baseline below the top by the
    // height of the first row, and the overline over it; then 5/3 of the
    // height of the row below times the line spacing factor. With rows all
    // as high, the arithmetic of stage 0.8.
    result.top = rows.front().height;
    if (!rows.front().blank) {
        double overlined = 0.0;
        for (const Piece& piece : rows.front().pieces) {
            const MTextFormat& format = result.runOf(piece).format;
            if (format.overline) {
                overlined = std::max(overlined, format.height);
            }
        }
        if (overlined > 0.0) {
            result.top += kOverlineExtra * overlined;
        }
    }
    const bool uniform = std::ranges::all_of(
        rows, [&](const Row& row) { return row.height == rows.front().height; });
    const double step = kBaselineDistance * rows.front().height * mtext.lineSpacing;
    std::vector<double> offsets(rows.size(), 0.0);
    for (std::size_t i = 1; i < rows.size(); ++i) {
        offsets[i] =
            uniform ? static_cast<double>(i) * step
                    : offsets[i - 1] + (kBaselineDistance * rows[i].height * mtext.lineSpacing);
    }
    const double span = offsets.back();
    const double top = result.top;
    double first = -top;  // top
    switch (mtext.attachment) {
        case MTextAttachment::MiddleLeft:
        case MTextAttachment::MiddleCenter:
        case MTextAttachment::MiddleRight:
            first = ((top + span) / 2.0) - top;
            break;
        case MTextAttachment::BottomLeft:
        case MTextAttachment::BottomCenter:
        case MTextAttachment::BottomRight:
            first = span;
            break;
        default:
            break;
    }

    // Horizontal: the column starts at 0, -W/2 or -W and each row lies left,
    // centred or right in it, so a row starts at 0, -w/2 or -w, whatever W.
    for (std::size_t i = 0; i < rows.size(); ++i) {
        Row& row = rows[i];
        const double width = rowWidth(row);
        double x = 0.0;
        switch (mtext.attachment) {
            case MTextAttachment::TopCenter:
            case MTextAttachment::MiddleCenter:
            case MTextAttachment::BottomCenter:
                x = -width / 2.0;
                break;
            case MTextAttachment::TopRight:
            case MTextAttachment::MiddleRight:
            case MTextAttachment::BottomRight:
                x = -width;
                break;
            default:
                break;
        }
        row.start = {x, first - offsets[i]};
        for (Piece& piece : row.pieces) {
            piece.x += x;
            for (double& boundary : piece.boundaries) {
                boundary += x;
            }
        }
    }
    return result;
}

std::u32string literalValue(std::u32string_view row) {
    std::u32string result;
    result.reserve(row.size());
    for (std::size_t i = 0; i < row.size(); ++i) {
        const bool formsCode = row[i] == U'%' && i + kPercentCodeLength <= row.size() &&
                               row[i + 1] == U'%' && percentCharacter(row[i + 2]).has_value();
        result += formsCode ? std::u32string_view{U"%%%"} : std::u32string_view{&row[i], 1};
    }
    return result;
}

std::string toUtf8(std::u32string_view text) {
    std::string result;
    result.reserve(text.size());
    for (const char32_t code : text) {
        appendUtf8(code, result);
    }
    return result;
}

}  // namespace tg::core::mtext_layout
