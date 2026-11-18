// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/mtext.hpp"
#include "tg/core/vec2.hpp"

#include <string>
#include <string_view>
#include <vector>

/// The plain text and the layout of an MTEXT (docs/adr/0046, point 2;
/// docs/tappe/0.8/PT13-mtext-core.md, points 2 and 3). Private to core.
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

/// One laid-out row of the column.
struct Row {
    std::u32string text;  ///< plain, with kNoBreakSpace made a normal space
    Vec2 start;           ///< start of the baseline, relative to the insertion
                          ///< point, before the rotation
    bool blank{true};     ///< empty or only spaces: no Text for it
};

/// The rows of `mtext`, in order (PT13, point 3), with `widthFactor` the
/// width factor of its style (1 without a style). The rows are at least one.
[[nodiscard]] std::vector<Row> layout(const MText& mtext, double widthFactor);

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
