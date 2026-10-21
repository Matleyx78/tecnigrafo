// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <string>
#include <string_view>

namespace tg::core {
/// Bundled font metrics in units of capital-letter height (ADR 0013).
struct FontMetrics {
    double ascent;   ///< > 1
    double descent;  ///< < 0
};

[[nodiscard]] const FontMetrics& fontMetrics();
/// Substitute U+2300/U+2205 with U+00D8 only if absent from the font.
[[nodiscard]] char32_t fontCharacter(char32_t codePoint);
/// Advance of the substituted character, or the missing glyph's advance.
[[nodiscard]] double advanceWidth(char32_t codePoint);
/// Sum of advances, without kerning or DXF control-code expansion.
[[nodiscard]] double textWidth(std::u32string_view text);
/// Decode UTF-8; each ill-formed subsequence becomes U+FFFD.
[[nodiscard]] std::u32string decodeUtf8(std::string_view text);
}  // namespace tg::core
