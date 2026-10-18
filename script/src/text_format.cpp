// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/text_format.hpp"

#include <cstddef>
#include <cstdint>

namespace tg::script {
namespace {

// Unicode code points end at U+10FFFF; U+D800 to U+DFFF are surrogates,
// which UTF-8 never encodes.
constexpr std::uint32_t kMaxCodePoint = 0x10FFFF;
constexpr std::uint32_t kFirstSurrogate = 0xD800;
constexpr std::uint32_t kLastSurrogate = 0xDFFF;
constexpr std::size_t kMaxHexDigits = 6;

std::optional<std::uint32_t> hexValue(char c) {
    if (c >= '0' && c <= '9') {
        return static_cast<std::uint32_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return static_cast<std::uint32_t>(c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F') {
        return static_cast<std::uint32_t>(c - 'A' + 10);
    }
    return std::nullopt;
}

void appendUtf8(std::uint32_t codePoint, std::string& out) {
    const auto byte = [](std::uint32_t value) {
        return static_cast<char>(value);
    };
    if (codePoint < 0x80U) {
        out += byte(codePoint);
    } else if (codePoint < 0x800U) {
        out += byte(0xC0U | (codePoint >> 6U));
        out += byte(0x80U | (codePoint & 0x3FU));
    } else if (codePoint < 0x10000U) {
        out += byte(0xE0U | (codePoint >> 12U));
        out += byte(0x80U | ((codePoint >> 6U) & 0x3FU));
        out += byte(0x80U | (codePoint & 0x3FU));
    } else {
        out += byte(0xF0U | (codePoint >> 18U));
        out += byte(0x80U | ((codePoint >> 12U) & 0x3FU));
        out += byte(0x80U | ((codePoint >> 6U) & 0x3FU));
        out += byte(0x80U | (codePoint & 0x3FU));
    }
}

}  // namespace

std::string quoteText(std::string_view text) {
    constexpr std::string_view kHexDigits = "0123456789ABCDEF";
    constexpr unsigned char kFirstPrintable = 0x20;
    constexpr unsigned char kDelete = 0x7F;
    std::string out = "\"";
    for (const char c : text) {
        const auto byte = static_cast<unsigned char>(c);
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (byte < kFirstPrintable || byte == kDelete) {
            out += "\\u{";
            out += kHexDigits[byte >> 4U];
            out += kHexDigits[byte & 0x0FU];
            out += '}';
        } else {
            out += c;
        }
    }
    out += '"';
    return out;
}

std::optional<std::string> unquoteText(std::string_view token) {
    if (token.size() < 2 || token.front() != '"' || token.back() != '"') {
        return std::nullopt;
    }
    const std::string_view inner = token.substr(1, token.size() - 2);
    std::string text;
    for (std::size_t i = 0; i < inner.size(); ++i) {
        const char c = inner[i];
        if (c == '"') {
            return std::nullopt;  // an unescaped quote ends the string early
        }
        if (c != '\\') {
            text += c;
            continue;
        }
        if (++i == inner.size()) {
            return std::nullopt;
        }
        if (inner[i] == '"' || inner[i] == '\\') {
            text += inner[i];
            continue;
        }
        // \u{X...}
        if (inner[i] != 'u' || i + 1 == inner.size() || inner[i + 1] != '{') {
            return std::nullopt;
        }
        i += 2;
        std::uint32_t codePoint = 0;
        std::size_t digits = 0;
        while (i < inner.size() && inner[i] != '}') {
            const std::optional<std::uint32_t> value = hexValue(inner[i]);
            if (!value || ++digits > kMaxHexDigits) {
                return std::nullopt;
            }
            codePoint = (codePoint << 4U) | *value;
            ++i;
        }
        if (i == inner.size() || digits == 0 || codePoint > kMaxCodePoint ||
            (codePoint >= kFirstSurrogate && codePoint <= kLastSurrogate)) {
            return std::nullopt;
        }
        appendUtf8(codePoint, text);
    }
    return text;
}

}  // namespace tg::script
