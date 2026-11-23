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
// spacing factor (docs/adr/0013 and docs/adr/0046, point 2, as AutoCAD).
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

/// A word and the normal spaces before it; the word is empty only for the
/// spaces at the end of a paragraph.
struct Token {
    std::u32string spaces;
    std::u32string word;  ///< kNoBreakSpace already a normal space
};

std::vector<Token> tokens(std::u32string_view paragraph) {
    std::vector<Token> result;
    std::size_t i = 0;
    while (i < paragraph.size()) {
        Token token;
        while (i < paragraph.size() && paragraph[i] == U' ') {
            token.spaces += U' ';
            ++i;
        }
        while (i < paragraph.size() && paragraph[i] != U' ') {
            token.word += paragraph[i] == kNoBreakSpace ? U' ' : paragraph[i];
            ++i;
        }
        result.push_back(std::move(token));
    }
    return result;
}

/// The rows of one paragraph: one without a positive width, otherwise broken
/// before the word that would carry the row beyond it (PT13, point 3).
std::vector<std::u32string> wrap(std::u32string_view paragraph, double width, double scale) {
    const std::vector<Token> words = tokens(paragraph);
    std::vector<std::u32string> rows;
    std::u32string row;
    bool hasWord = false;
    for (const Token& token : words) {
        if (token.word.empty()) {
            row += token.spaces;  // the spaces at the end of the paragraph
            continue;
        }
        std::u32string candidate = row + token.spaces + token.word;
        if (width > 0.0 && hasWord && textWidth(candidate) * scale > width) {
            // The spaces where the row breaks stay in neither row.
            rows.push_back(std::move(row));
            row = token.word;
        } else {
            row = std::move(candidate);
        }
        hasWord = true;
    }
    rows.push_back(std::move(row));
    return rows;
}

bool isBlank(std::u32string_view row) {
    return std::ranges::all_of(row, [](char32_t c) { return c == U' '; });
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

std::vector<Row> layout(const MText& mtext, double widthFactor) {
    const double height = mtext.height;
    const double scale = height * widthFactor;
    std::vector<Row> rows;
    for (const std::u32string& paragraph : paragraphs(mtext.value)) {
        for (std::u32string& text : wrap(paragraph, mtext.width, scale)) {
            Row row;
            row.blank = isBlank(text);
            row.text = std::move(text);
            rows.push_back(std::move(row));
        }
    }

    // Vertical: the first baseline, in the system of the column.
    const double step = kBaselineDistance * height * mtext.lineSpacing;
    const double span = static_cast<double>(rows.size() - 1) * step;
    double first = -height;  // top
    switch (mtext.attachment) {
        case MTextAttachment::MiddleLeft:
        case MTextAttachment::MiddleCenter:
        case MTextAttachment::MiddleRight:
            first = ((height + span) / 2.0) - height;
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
        // A blank row counts as an empty one (PT13, point 3).
        const double rowWidth = row.blank ? 0.0 : textWidth(row.text) * scale;
        double x = 0.0;
        switch (mtext.attachment) {
            case MTextAttachment::TopCenter:
            case MTextAttachment::MiddleCenter:
            case MTextAttachment::BottomCenter:
                x = -rowWidth / 2.0;
                break;
            case MTextAttachment::TopRight:
            case MTextAttachment::MiddleRight:
            case MTextAttachment::BottomRight:
                x = -rowWidth;
                break;
            default:
                break;
        }
        row.start = {x, first - (static_cast<double>(i) * step)};
    }
    return rows;
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
