// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "mtext_layout.hpp"

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

// `\U+XXXX`: the backslash, 'U', '+' and four hexadecimal digits.
constexpr std::size_t kUnicodeCodeLength = 7;
constexpr std::size_t kUnicodeDigits = 4;
constexpr std::size_t kHexBase = 16;
constexpr std::size_t kDecimalDigits = 10;

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

/// The codes with an argument up to ';' that are removed (PT13, point 2).
bool isArgumentCode(char32_t code) {
    switch (code) {
        case U'f':
        case U'F':
        case U'H':
        case U'W':
        case U'Q':
        case U'T':
        case U'A':
        case U'C':
        case U'c':
        case U'p':
            return true;
        default:
            return false;
    }
}

/// The codes that end at ';': those removed and `\S`, which keeps its text.
bool takesArgument(char32_t code) {
    return isArgumentCode(code) || code == U'S';
}

/// Underline, overline and strike-through on and off, removed.
bool isToggleCode(char32_t code) {
    switch (code) {
        case U'L':
        case U'l':
        case U'O':
        case U'o':
        case U'K':
        case U'k':
            return true;
        default:
            return false;
    }
}

std::optional<std::size_t> hexDigit(char32_t c) {
    if (c >= U'0' && c <= U'9') {
        return static_cast<std::size_t>(c - U'0');
    }
    if (c >= U'a' && c <= U'f') {
        return static_cast<std::size_t>(c - U'a') + kDecimalDigits;
    }
    if (c >= U'A' && c <= U'F') {
        return static_cast<std::size_t>(c - U'A') + kDecimalDigits;
    }
    return std::nullopt;
}

/// The character of `\U+XXXX` at `at`, or std::nullopt when the four digits
/// are not there. A surrogate is not a character: U+FFFD, as decodeUtf8().
std::optional<char32_t> unicodeCode(std::u32string_view text, std::size_t at) {
    if (at + kUnicodeCodeLength > text.size() || text[at + 2] != U'+') {
        return std::nullopt;
    }
    std::size_t value = 0;
    for (std::size_t i = 0; i < kUnicodeDigits; ++i) {
        const std::optional<std::size_t> digit = hexDigit(text[at + 3 + i]);
        if (!digit) {
            return std::nullopt;
        }
        value = (value * kHexBase) + *digit;
    }
    const auto code = static_cast<char32_t>(value);
    return code >= kFirstSurrogate && code <= kLastSurrogate ? kReplacement : code;
}

/// The index of the ';' that ends the argument starting at `from`, or
/// std::nullopt when a `\P` or the end of the text comes first.
std::optional<std::size_t> argumentEnd(std::u32string_view text, std::size_t from) {
    for (std::size_t i = from; i < text.size(); ++i) {
        if (text[i] == U';') {
            return i;
        }
        if (text[i] == U'\\' && i + 1 < text.size()) {
            if (text[i + 1] == U'P') {
                return std::nullopt;
            }
            ++i;  // an escaped character never ends the argument
        }
    }
    return std::nullopt;
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
    const std::u32string text = decodeUtf8(value);
    std::vector<std::u32string> result(1);
    std::size_t i = 0;
    while (i < text.size()) {
        std::u32string& current = result.back();
        const char32_t c = text[i];
        if (c == U'{' || c == U'}') {
            ++i;  // a group
            continue;
        }
        if (c == U'%' && i + kPercentCodeLength <= text.size() && text[i + 1] == U'%') {
            if (const std::optional<char32_t> converted = percentCharacter(text[i + 2])) {
                current += *converted;
                i += kPercentCodeLength;
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
        } else if (const std::optional<std::size_t> end =
                       takesArgument(code) ? argumentEnd(text, i + 2) : std::nullopt) {
            if (code == U'S') {
                // A stacked fraction, shown on one line: "1^2", "1#2" and
                // "1/2" all become "1/2".
                for (std::size_t k = i + 2; k < *end; ++k) {
                    const char32_t part = text[k];
                    current += part == U'^' || part == U'#' ? U'/' : part;
                }
            }
            i = *end + 1;
        } else {
            // Any other code, and one without its ';', stays as written: the
            // characters after the backslash are read again as text.
            current += c;
            ++i;
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

std::string toUtf8(std::u32string_view text) {
    std::string result;
    result.reserve(text.size());
    for (const char32_t code : text) {
        appendUtf8(code, result);
    }
    return result;
}

}  // namespace tg::core::mtext_layout
