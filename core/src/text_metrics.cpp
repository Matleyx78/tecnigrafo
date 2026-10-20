// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/text_metrics.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>

namespace tg::core {
namespace {
struct CharacterWidth {
    char32_t code;
    double advance;
};

#include "font_metrics.inc"

const CharacterWidth* findCharacter(char32_t code) {
    const auto index = static_cast<std::size_t>(
        std::ranges::lower_bound(kWidths, code, {}, &CharacterWidth::code) - kWidths.begin());
    return index < kWidths.size() && kWidths.at(index).code == code ? &kWidths.at(index) : nullptr;
}
}  // namespace

const FontMetrics& fontMetrics() {
    return kMetrics;
}

char32_t fontCharacter(char32_t codePoint) {
    if ((codePoint == U'\u2300' || codePoint == U'\u2205') && findCharacter(codePoint) == nullptr &&
        findCharacter(U'\u00D8') != nullptr) {
        return U'\u00D8';
    }
    return codePoint;
}

double advanceWidth(char32_t codePoint) {
    const auto* character = findCharacter(fontCharacter(codePoint));
    return character != nullptr ? character->advance : kMissingAdvance;
}

double textWidth(std::u32string_view text) {
    double width = 0;
    for (const auto code : text) {
        width += advanceWidth(code);
    }
    return width;
}

std::u32string decodeUtf8(std::string_view text) {
    constexpr char32_t kReplacement = U'\uFFFD';
    std::u32string result;
    std::size_t offset = 0;
    while (offset < text.size()) {
        const auto lead = static_cast<unsigned char>(text[offset++]);
        if (lead < 0x80U) {
            result += static_cast<char32_t>(lead);
            continue;
        }
        std::size_t count = 0;
        char32_t code = 0;
        if (lead >= 0xC2U && lead <= 0xDFU) {
            count = 1;
            code = lead & 0x1FU;
        } else if (lead >= 0xE0U && lead <= 0xEFU) {
            count = 2;
            code = lead & 0x0FU;
        } else if (lead >= 0xF0U && lead <= 0xF4U) {
            count = 3;
            code = lead & 0x07U;
        } else {
            result += kReplacement;
            continue;
        }
        std::size_t consumed = 0;
        while (consumed < count && offset < text.size()) {
            const auto next = static_cast<unsigned char>(text[offset]);
            if (next < 0x80U || next > 0xBFU ||
                (consumed == 0 &&
                 ((lead == 0xE0U && next < 0xA0U) || (lead == 0xEDU && next > 0x9FU) ||
                  (lead == 0xF0U && next < 0x90U) || (lead == 0xF4U && next > 0x8FU)))) {
                break;
            }
            code = (code << 6U) | (next & 0x3FU);
            ++offset;
            ++consumed;
        }
        result += consumed == count ? code : kReplacement;
    }
    return result;
}
}  // namespace tg::core
