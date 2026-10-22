// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>

namespace tg::io::detail {
namespace {

using CodePageTable = std::array<char16_t, 128>;

// Bytes 0x80-0xFF of Windows code page 1252 (Western European), from the
// code page definition published by Microsoft. The five bytes the code page
// leaves undefined (0x81, 0x8D, 0x8F, 0x90, 0x9D) map to the C1 control
// with the same number, as Windows does, so they survive a round trip.
constexpr CodePageTable kAnsi1252 = {
    0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,  // 0x80
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008D, 0x017D, 0x008F,  // 0x88
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,  // 0x90
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178,  // 0x98
    0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,  // 0xA0
    0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,  // 0xA8
    0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,  // 0xB0
    0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,  // 0xB8
    0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,  // 0xC0
    0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,  // 0xC8
    0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,  // 0xD0
    0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,  // 0xD8
    0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,  // 0xE0
    0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,  // 0xE8
    0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,  // 0xF0
    0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF,  // 0xF8
};

// Bytes 0x80-0xFF of Windows code page 1250 (Central European), from the
// code page definition published by Microsoft; undefined bytes (0x81, 0x83,
// 0x88, 0x90, 0x98) handled as in kAnsi1252.
constexpr CodePageTable kAnsi1250 = {
    0x20AC, 0x0081, 0x201A, 0x0083, 0x201E, 0x2026, 0x2020, 0x2021,  // 0x80
    0x0088, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179,  // 0x88
    0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,  // 0x90
    0x0098, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A,  // 0x98
    0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6, 0x00A7,  // 0xA0
    0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B,  // 0xA8
    0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5, 0x00B6, 0x00B7,  // 0xB0
    0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C,  // 0xB8
    0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,  // 0xC0
    0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,  // 0xC8
    0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,  // 0xD0
    0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,  // 0xD8
    0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,  // 0xE0
    0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,  // 0xE8
    0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,  // 0xF0
    0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9,  // 0xF8
};

/// First byte that is not ASCII.
constexpr unsigned kFirstHighByte = 0x80;

constexpr char32_t kReplacementCharacter = 0xFFFD;
constexpr char32_t kMaxCodePoint = 0x10FFFF;
constexpr char32_t kHighSurrogateFirst = 0xD800;
constexpr char32_t kLowSurrogateFirst = 0xDC00;
constexpr char32_t kSurrogateLast = 0xDFFF;
constexpr char32_t kFirstSupplementary = 0x10000;

/// `\U+XXXX`: the prefix and the number of hexadecimal digits that follow.
constexpr std::string_view kEscapePrefix = "\\U+";
constexpr std::size_t kEscapeDigits = 4;
constexpr std::size_t kEscapeLength = 7;  // kEscapePrefix.size() + kEscapeDigits

const CodePageTable& tableFor(TextEncoding encoding) {
    return encoding == TextEncoding::Ansi1250 ? kAnsi1250 : kAnsi1252;
}

constexpr bool isSurrogate(char32_t c) {
    return c >= kHighSurrogateFirst && c <= kSurrogateLast;
}

void appendUtf8(std::string& out, char32_t c) {
    constexpr char32_t kOneByteMax = 0x7F;
    constexpr char32_t kTwoBytesMax = 0x7FF;
    constexpr char32_t kThreeBytesMax = 0xFFFF;
    constexpr unsigned kContinuation = 0x80;
    constexpr unsigned kSixBits = 0x3F;
    const auto byte = [](char32_t value) {
        return static_cast<char>(value);
    };
    if (c <= kOneByteMax) {
        out += byte(c);
    } else if (c <= kTwoBytesMax) {
        out += byte(0xC0 | (c >> 6));
        out += byte(kContinuation | (c & kSixBits));
    } else if (c <= kThreeBytesMax) {
        out += byte(0xE0 | (c >> 12));
        out += byte(kContinuation | ((c >> 6) & kSixBits));
        out += byte(kContinuation | (c & kSixBits));
    } else {
        out += byte(0xF0 | (c >> 18));
        out += byte(kContinuation | ((c >> 12) & kSixBits));
        out += byte(kContinuation | ((c >> 6) & kSixBits));
        out += byte(kContinuation | (c & kSixBits));
    }
}

/// Decodes the UTF-8 character at `position` and moves past it. Returns
/// std::nullopt, moving one byte, if the bytes there are not well-formed.
std::optional<char32_t> nextCodePoint(std::string_view text, std::size_t& position) {
    const auto lead = static_cast<unsigned char>(text[position]);
    if (lead < kFirstHighByte) {
        ++position;
        return lead;
    }
    std::size_t length = 0;
    char32_t value = 0;
    char32_t minimum = 0;  // smallest value that needs this length (no overlong forms)
    if ((lead & 0xE0U) == 0xC0U) {
        length = 2;
        value = lead & 0x1FU;
        minimum = 0x80;
    } else if ((lead & 0xF0U) == 0xE0U) {
        length = 3;
        value = lead & 0x0FU;
        minimum = 0x800;
    } else if ((lead & 0xF8U) == 0xF0U) {
        length = 4;
        value = lead & 0x07U;
        minimum = kFirstSupplementary;
    } else {
        ++position;
        return std::nullopt;
    }
    if (text.size() - position < length) {
        ++position;
        return std::nullopt;
    }
    for (std::size_t i = 1; i < length; ++i) {
        const auto next = static_cast<unsigned char>(text[position + i]);
        if ((next & 0xC0U) != 0x80U) {
            ++position;
            return std::nullopt;
        }
        value = (value << 6U) | (next & 0x3FU);
    }
    if (value < minimum || value > kMaxCodePoint || isSurrogate(value)) {
        ++position;
        return std::nullopt;
    }
    position += length;
    return value;
}

std::optional<char32_t> hexDigit(char c) {
    constexpr char32_t kTen = 10;
    if (c >= '0' && c <= '9') {
        return static_cast<char32_t>(c - '0');
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<char32_t>(c - 'A') + kTen;
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<char32_t>(c - 'a') + kTen;
    }
    return std::nullopt;
}

/// The value of the `\U+XXXX` that starts at `position`, if there is one.
std::optional<char32_t> escapeAt(std::string_view text, std::size_t position) {
    if (text.size() - position < kEscapeLength ||
        text.substr(position, kEscapePrefix.size()) != kEscapePrefix) {
        return std::nullopt;
    }
    char32_t value = 0;
    for (std::size_t i = 0; i < kEscapeDigits; ++i) {
        const std::optional<char32_t> digit = hexDigit(text[position + kEscapePrefix.size() + i]);
        if (!digit) {
            return std::nullopt;
        }
        value = (value << 4U) | *digit;
    }
    return value;
}

/// `text` with every `\U+XXXX` replaced by its character. A surrogate pair
/// written as two sequences becomes one character; a lone surrogate and
/// `\U+0000` are not characters and stay as written.
std::string replaceEscapes(std::string_view text) {
    if (text.find(kEscapePrefix) == std::string_view::npos) {
        return std::string{text};
    }
    std::string out;
    std::size_t position = 0;
    while (position < text.size()) {
        const std::optional<char32_t> first = escapeAt(text, position);
        if (!first || *first == 0 || (*first >= kLowSurrogateFirst && *first <= kSurrogateLast)) {
            out += text[position];
            ++position;
            continue;
        }
        if (*first < kHighSurrogateFirst || *first > kSurrogateLast) {
            appendUtf8(out, *first);
            position += kEscapeLength;
            continue;
        }
        // A high surrogate: a character only together with a low one.
        const std::optional<char32_t> second = escapeAt(text, position + kEscapeLength);
        if (!second || *second < kLowSurrogateFirst || *second > kSurrogateLast) {
            out += text[position];
            ++position;
            continue;
        }
        constexpr unsigned kSurrogateBits = 10;
        appendUtf8(out, kFirstSupplementary + ((*first - kHighSurrogateFirst) << kSurrogateBits) +
                            (*second - kLowSurrogateFirst));
        position += 2 * kEscapeLength;
    }
    return out;
}

void appendEscape(std::string& out, char32_t c) {
    constexpr std::string_view kHex = "0123456789ABCDEF";
    out += kEscapePrefix;
    for (unsigned shift = 12;; shift -= 4) {
        out += kHex[(c >> shift) & 0xFU];
        if (shift == 0) {
            break;
        }
    }
}

}  // namespace

bool isValidUtf8(std::string_view bytes) {
    std::size_t position = 0;
    while (position < bytes.size()) {
        if (!nextCodePoint(bytes, position)) {
            return false;
        }
    }
    return true;
}

std::optional<std::string> decodeText(std::string_view bytes, TextEncoding encoding) {
    if (encoding == TextEncoding::Utf8) {
        if (!isValidUtf8(bytes)) {
            return std::nullopt;
        }
        return replaceEscapes(bytes);
    }
    const CodePageTable& table = tableFor(encoding);
    std::string utf8;
    for (const char c : bytes) {
        const auto byte = static_cast<unsigned char>(c);
        if (byte < kFirstHighByte) {
            utf8 += c;
        } else {
            appendUtf8(utf8, *std::next(table.begin(), byte - kFirstHighByte));
        }
    }
    return replaceEscapes(utf8);
}

std::string encodeText(std::string_view text, TextEncoding encoding) {
    if (encoding == TextEncoding::Utf8) {
        return std::string{text};
    }
    const CodePageTable& table = tableFor(encoding);
    std::string out;
    std::size_t position = 0;
    while (position < text.size()) {
        const char32_t c = nextCodePoint(text, position).value_or(kReplacementCharacter);
        if (c < kFirstHighByte) {
            out += static_cast<char>(c);
            continue;
        }
        // The position of the character in the code page, or the table size.
        const std::ptrdiff_t index = std::ranges::find(table, c) - table.begin();
        if (index < std::ssize(table)) {
            out += static_cast<char>(kFirstHighByte + static_cast<unsigned>(index));
            continue;
        }
        if (c < kFirstSupplementary) {
            appendEscape(out, c);
        } else {
            constexpr unsigned kSurrogateBits = 10;
            constexpr char32_t kLowTenBits = 0x3FF;
            const char32_t offset = c - kFirstSupplementary;
            appendEscape(out, kHighSurrogateFirst + (offset >> kSurrogateBits));
            appendEscape(out, kLowSurrogateFirst + (offset & kLowTenBits));
        }
    }
    return out;
}

}  // namespace tg::io::detail
