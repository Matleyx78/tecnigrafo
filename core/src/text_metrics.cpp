// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/text_metrics.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>

namespace tg::core {
namespace {
struct CharacterWidth {
    char32_t code;
    double advance;
};

// Each generated table defines the same names, so each gets its own namespace.
namespace regular {
#include "font_metrics_regular.inc"
}  // namespace regular

namespace bold {
#include "font_metrics_bold.inc"
}  // namespace bold

namespace italic {
#include "font_metrics_italic.inc"
}  // namespace italic

namespace bold_italic {
#include "font_metrics_bolditalic.inc"
}  // namespace bold_italic

struct FaceTable {
    const FontMetrics* metrics;
    double missingAdvance;
    std::span<const CharacterWidth> widths;
};

const FaceTable& faceTable(FontFace face) {
    static const std::array<FaceTable, 4> kFaces{{
        {&regular::kMetrics, regular::kMissingAdvance, regular::kWidths},
        {&bold::kMetrics, bold::kMissingAdvance, bold::kWidths},
        {&italic::kMetrics, italic::kMissingAdvance, italic::kWidths},
        {&bold_italic::kMetrics, bold_italic::kMissingAdvance, bold_italic::kWidths},
    }};
    return kFaces.at(static_cast<std::size_t>(face));
}

const CharacterWidth* findCharacter(char32_t code, FontFace face) {
    const auto widths = faceTable(face).widths;
    const auto found = std::ranges::lower_bound(widths, code, {}, &CharacterWidth::code);
    return found != widths.end() && found->code == code ? &*found : nullptr;
}
}  // namespace

const FontMetrics& fontMetrics(FontFace face) {
    return *faceTable(face).metrics;
}

char32_t fontCharacter(char32_t codePoint, FontFace face) {
    if ((codePoint == U'\u2300' || codePoint == U'\u2205') &&
        findCharacter(codePoint, face) == nullptr && findCharacter(U'\u00D8', face) != nullptr) {
        return U'\u00D8';
    }
    return codePoint;
}

double advanceWidth(char32_t codePoint, FontFace face) {
    const auto* character = findCharacter(fontCharacter(codePoint, face), face);
    return character != nullptr ? character->advance : faceTable(face).missingAdvance;
}

double textWidth(std::u32string_view text, FontFace face) {
    double width = 0;
    for (const auto code : text) {
        width += advanceWidth(code, face);
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
