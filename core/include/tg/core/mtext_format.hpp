// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/color.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/// The formatting codes of an MTEXT value read into pieces (docs/adr/0054,
/// point 1; docs/tappe/0.8.1/PT7-codici-di-formato.md). The value itself is
/// never changed: every piece and every character says which bytes of it it
/// comes from.
namespace tg::core {

/// The separator of `\S`: '/', '#' or '^'.
enum class StackKind : std::uint8_t { Horizontal, Diagonal, Tolerance };  // / # ^

/// The format of a piece. Two pieces with equal formats and nothing visible
/// between them are one piece.
struct MTextFormat {
    bool bold{false}, italic{false};  // PT8 turns them into a FontFace (PT6)
    double height{};                  // absolute, drawing units
    double widthFactor{1};
    double oblique{0};  // radians
    double tracking{1};
    /// nullopt: the colour of the entity. `\C0;` is Color::byBlock() and
    /// `\C256;` Color::byLayer(), as their ACI numbers.
    std::optional<Color> color;
    bool underline{false}, overline{false}, strike{false};

    friend bool operator==(const MTextFormat&, const MTextFormat&) = default;
};

/// A stacked fraction. Without a separator (`\S12;`, which AutoCAD does not
/// write) all the text is the top, the bottom is empty and the kind is
/// Horizontal.
struct MTextStack {
    std::u32string top, bottom;
    StackKind kind;

    friend bool operator==(const MTextStack&, const MTextStack&) = default;
};

// The bytes of the value one editable unit comes from: a character with
// its escape (\U+0042, %%c, \\, \~), or a whole stacked fraction.
struct MTextUnit {
    std::size_t sourceBegin{}, sourceEnd{};

    friend bool operator==(const MTextUnit&, const MTextUnit&) = default;
};

/// A piece of a paragraph: characters with one format, or one stacked
/// fraction. `sourceBegin` is where the previous piece of the paragraph ends
/// (or where the paragraph starts), so the codes that open the piece are
/// in it; `sourceEnd` is where its last unit ends. The bytes between two
/// units are codes (docs/tappe/0.8.1/PT7-codici-di-formato.md, point 3).
struct MTextRun {
    /// Literal characters, codes resolved. `\~` is U+00A0, the no-break
    /// space, as a U+00A0 written in the value: its unit (two bytes, `\~`)
    /// tells the two apart.
    std::u32string text;
    std::optional<MTextStack> stack;  // a stacked fraction, text empty
    MTextFormat format;
    std::size_t sourceBegin{}, sourceEnd{};  // byte range in the value
    std::vector<MTextUnit> units;            // one per character of text; one for a stack

    friend bool operator==(const MTextRun&, const MTextRun&) = default;
};

using MTextParagraph = std::vector<MTextRun>;

/// The paragraphs of `value` (UTF-8, formatting codes kept), one more after
/// every `\P`, with their pieces in reading order, starting from `base`
/// (the height, width factor and oblique of the entity and its style).
///
/// - A group `{…}` saves the format and its `}` gives it back; a `}`
///   without its `{` does nothing. A format lasts across `\P`.
/// - `\H<n>;` sets the height, `\H<n>x;` multiplies the current one;
///   `\W<n>;` the width factor, `\Q<degrees>;` the oblique, `\T<n>;` the
///   tracking; `\C<aci>;` an indexed colour, `\c<n>;` a true colour written
///   blue-green-red, as AutoCAD (`\c16744448;` is RGB 0080FF).
/// - `\f…;` and `\F…;` set bold from `|b1` and italic from `|i1` (each off
///   without them) and ignore the font name.
/// - `\L` `\l`, `\O` `\o`, `\K` `\k`: underline, overline, strike on and off.
/// - `\S<a>/<b>;`, `\S<a>#<b>;`, `\S<a>^<b>;` (and `^ `, as AutoCAD saves
///   it): a stacked fraction, a piece of its own. In its two texts `\\`,
///   `\{`, `\}`, `\~`, `\U+XXXX` and `%%c`, `%%d`, `%%p`, `%%%` are decoded;
///   any other backslash stays with the character after it.
/// - `\A…;`, `\p…;` and the codes above are hidden.
///
/// As in stage 0.8 (docs/adr/0046, point 2): an unknown code (`\Zx;`), and
/// a code with an argument and no ';' before the next `\P`, are text; a
/// known code with its ';' is hidden even when its argument is not a number
/// (`\Habc;`). An argument outside the domain of its quantity (a height,
/// width factor or tracking that is not a positive finite number, an oblique
/// that is not finite, an ACI outside 0..256, a colour outside 0..FFFFFF)
/// is hidden and has no effect: the format stays as it was
/// (docs/tappe/0.8.1/PT7-codici-di-formato.md, rules of the stage).
///
/// A paragraph with nothing visible (`""`, `{\L}`) has no pieces. Ill-formed
/// UTF-8 gives U+FFFD as decodeUtf8() does, one unit for each ill-formed
/// subsequence.
[[nodiscard]] std::vector<MTextParagraph> formattedParagraphs(std::string_view value,
                                                              const MTextFormat& base);

}  // namespace tg::core
