// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace tg::core {
/// Bundled font metrics in units of capital-letter height (ADR 0013).
struct FontMetrics {
    double ascent;   ///< > 1
    double descent;  ///< < 0
};

/// The four bundled Liberation Sans files (ADR 0054, point 2). Each face is
/// measured in units of its own file's capital-letter height.
enum class FontFace : std::uint8_t { Regular, Bold, Italic, BoldItalic };

[[nodiscard]] const FontMetrics& fontMetrics(FontFace face = FontFace::Regular);
/// Substitute U+2300/U+2205 with U+00D8 only if absent from the face.
[[nodiscard]] char32_t fontCharacter(char32_t codePoint, FontFace face = FontFace::Regular);
/// Advance of the substituted character, or the face's missing glyph advance.
[[nodiscard]] double advanceWidth(char32_t codePoint, FontFace face = FontFace::Regular);
/// Sum of advances, without kerning or DXF control-code expansion.
[[nodiscard]] double textWidth(std::u32string_view text, FontFace face = FontFace::Regular);
/// Decode UTF-8; each ill-formed subsequence becomes U+FFFD.
[[nodiscard]] std::u32string decodeUtf8(std::string_view text);
}  // namespace tg::core
