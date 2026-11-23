// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/mtext_format.hpp"

#include "tg/core/color.hpp"
#include "tg/core/math.hpp"

#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

constexpr char32_t kReplacement = U'�';
constexpr char32_t kNoBreakSpace = U' ';

// `\U+XXXX`: the backslash, 'U', '+' and four hexadecimal digits.
constexpr std::size_t kUnicodeCodeLength = 7;
constexpr std::size_t kUnicodeDigits = 4;
constexpr std::size_t kHexBase = 16;
constexpr std::size_t kDecimalDigits = 10;

// "%%" and the character of a DXF control code (docs/adr/0013, point 5).
constexpr std::size_t kPercentCodeLength = 3;

constexpr char32_t kFirstSurrogate = 0xD800;
constexpr char32_t kLastSurrogate = 0xDFFF;
constexpr char32_t kLastAscii = 0x7F;

// The ACI numbers of `\C`: 0 is ByBlock, 256 ByLayer, 1..255 the colours.
constexpr std::int64_t kAciByBlock = 0;
constexpr std::int64_t kAciByLayer = 256;
// `\c`: 24 bits, blue in the high byte and red in the low one.
constexpr std::int64_t kLastTrueColor = 0xFFFFFF;
constexpr unsigned kByteBits = 8;
constexpr std::int64_t kByteMask = 0xFF;

/// The value decoded as decodeUtf8() does, with the byte where each
/// character starts; `offsets` has one more entry, the size of the value.
struct Decoded {
    std::u32string text;
    std::vector<std::size_t> offsets;
};

/// decodeUtf8() (text_metrics.cpp) with the offsets: each ill-formed
/// subsequence is one U+FFFD, with the bytes it took.
Decoded decodeWithOffsets(std::string_view value) {
    constexpr unsigned char kOneByteEnd = 0x80U;
    constexpr unsigned char kLead2First = 0xC2U;
    constexpr unsigned char kLead2Last = 0xDFU;
    constexpr unsigned char kLead3First = 0xE0U;
    constexpr unsigned char kLead3Last = 0xEFU;
    constexpr unsigned char kLead4First = 0xF0U;
    constexpr unsigned char kLead4Last = 0xF4U;
    constexpr unsigned char kContinuationLast = 0xBFU;
    constexpr unsigned char kSurrogateLead = 0xEDU;
    constexpr unsigned char kBelowSurrogates = 0x9FU;
    constexpr unsigned char kAboveOverlong3 = 0xA0U;
    constexpr unsigned char kAboveOverlong4 = 0x90U;
    constexpr unsigned char kBelowLimit4 = 0x8FU;
    constexpr char32_t kLow5 = 0x1FU;
    constexpr char32_t kLow4 = 0x0FU;
    constexpr char32_t kLow3 = 0x07U;
    constexpr char32_t kLow6 = 0x3FU;
    constexpr unsigned kSixBits = 6;

    Decoded result;
    result.text.reserve(value.size());
    result.offsets.reserve(value.size() + 1);
    std::size_t offset = 0;
    while (offset < value.size()) {
        result.offsets.push_back(offset);
        const auto lead = static_cast<unsigned char>(value[offset++]);
        if (lead < kOneByteEnd) {
            result.text += static_cast<char32_t>(lead);
            continue;
        }
        std::size_t count = 0;
        char32_t code = 0;
        if (lead >= kLead2First && lead <= kLead2Last) {
            count = 1;
            code = lead & kLow5;
        } else if (lead >= kLead3First && lead <= kLead3Last) {
            count = 2;
            code = lead & kLow4;
        } else if (lead >= kLead4First && lead <= kLead4Last) {
            count = 3;
            code = lead & kLow3;
        } else {
            result.text += kReplacement;
            continue;
        }
        std::size_t consumed = 0;
        while (consumed < count && offset < value.size()) {
            const auto next = static_cast<unsigned char>(value[offset]);
            if (next < kOneByteEnd || next > kContinuationLast ||
                (consumed == 0 && ((lead == kLead3First && next < kAboveOverlong3) ||
                                   (lead == kSurrogateLead && next > kBelowSurrogates) ||
                                   (lead == kLead4First && next < kAboveOverlong4) ||
                                   (lead == kLead4Last && next > kBelowLimit4)))) {
                break;
            }
            code = (code << kSixBits) | (next & kLow6);
            ++offset;
            ++consumed;
        }
        result.text += consumed == count ? code : kReplacement;
    }
    result.offsets.push_back(value.size());
    return result;
}

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

/// The codes with an argument up to ';' other than `\S`.
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

/// The codes that end at ';': those above and `\S`.
bool takesArgument(char32_t code) {
    return isArgumentCode(code) || code == U'S';
}

/// Underline, overline and strike-through on and off.
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

/// No ';' ends the argument: argumentEnd() of a code that stays literal.
constexpr std::size_t kNoEnd = std::u32string_view::npos;

/// The index of the ';' that ends the argument starting at `from`, or kNoEnd
/// when a `\P` or the end of the text comes first.
std::size_t argumentEnd(std::u32string_view text, std::size_t from) {
    for (std::size_t i = from; i < text.size(); ++i) {
        if (text[i] == U';') {
            return i;
        }
        if (text[i] == U'\\' && i + 1 < text.size()) {
            if (text[i + 1] == U'P') {
                return kNoEnd;
            }
            ++i;  // an escaped character never ends the argument
        }
    }
    return kNoEnd;
}

/// `argument` as ASCII, or std::nullopt when a character is not ASCII.
std::optional<std::string> ascii(std::u32string_view argument) {
    std::string result;
    result.reserve(argument.size());
    for (const char32_t c : argument) {
        if (c > kLastAscii) {
            return std::nullopt;
        }
        result += static_cast<char>(c);
    }
    return result;
}

/// The whole of `argument` as a finite number.
std::optional<double> finiteNumber(std::u32string_view argument) {
    const std::optional<std::string> text = ascii(argument);
    if (!text || text->empty()) {
        return std::nullopt;
    }
    double value = 0.0;
    const char* end = text->data() + text->size();
    const auto [ptr, error] = std::from_chars(text->data(), end, value);
    if (error != std::errc{} || ptr != end || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

/// The whole of `argument` as a positive finite number.
std::optional<double> positiveNumber(std::u32string_view argument) {
    const std::optional<double> value = finiteNumber(argument);
    return value && *value > 0.0 ? value : std::nullopt;
}

/// The whole of `argument` as an integer in [first, last].
std::optional<std::int64_t> integerIn(std::u32string_view argument, std::int64_t first,
                                      std::int64_t last) {
    const std::optional<std::string> text = ascii(argument);
    if (!text || text->empty()) {
        return std::nullopt;
    }
    std::int64_t value = 0;
    const char* end = text->data() + text->size();
    const auto [ptr, error] = std::from_chars(text->data(), end, value);
    if (error != std::errc{} || ptr != end || value < first || value > last) {
        return std::nullopt;
    }
    return value;
}

/// The colour of `\C<aci>;`.
std::optional<Color> indexColor(std::u32string_view argument) {
    const std::optional<std::int64_t> aci = integerIn(argument, kAciByBlock, kAciByLayer);
    if (!aci) {
        return std::nullopt;
    }
    if (*aci == kAciByBlock) {
        return Color::byBlock();
    }
    if (*aci == kAciByLayer) {
        return Color::byLayer();
    }
    return Color::fromIndex(static_cast<int>(*aci));
}

/// The colour of `\c<bgr>;`: AutoCAD writes blue in the high byte.
std::optional<Color> trueColor(std::u32string_view argument) {
    const std::optional<std::int64_t> bgr = integerIn(argument, 0, kLastTrueColor);
    if (!bgr) {
        return std::nullopt;
    }
    const auto byte = [&bgr](unsigned shift) {
        return static_cast<std::uint8_t>((*bgr >> shift) & kByteMask);
    };
    return Color::fromRgb(Rgb{byte(0), byte(kByteBits), byte(2 * kByteBits)});
}

/// Bold and italic of `\f<name>|b<0/1>|i<0/1>…;`: the name is ignored, and
/// a flag that is not written is off.
void applyFont(std::u32string_view argument, MTextFormat& format) {
    format.bold = false;
    format.italic = false;
    std::size_t at = argument.find(U'|');
    while (at != std::u32string_view::npos) {
        const std::size_t next = argument.find(U'|', at + 1);
        const std::u32string_view field = argument.substr(at + 1, next - at - 1);
        if (field == U"b1") {
            format.bold = true;
        } else if (field == U"i1") {
            format.italic = true;
        }
        at = next;
    }
}

/// The effect of a hidden code with its argument; an argument outside the
/// domain of its quantity leaves the format as it is.
void applyArgument(char32_t code, std::u32string_view argument, MTextFormat& format) {
    switch (code) {
        case U'f':
        case U'F':
            applyFont(argument, format);
            break;
        case U'H':
            if (!argument.empty() && (argument.back() == U'x' || argument.back() == U'X')) {
                const std::optional<double> factor =
                    positiveNumber(argument.substr(0, argument.size() - 1));
                const double height = factor ? format.height * *factor : 0.0;
                if (std::isfinite(height) && height > 0.0) {
                    format.height = height;
                }
            } else if (const std::optional<double> height = positiveNumber(argument)) {
                format.height = *height;
            }
            break;
        case U'W':
            if (const std::optional<double> factor = positiveNumber(argument)) {
                format.widthFactor = *factor;
            }
            break;
        case U'Q':
            if (const std::optional<double> degrees = finiteNumber(argument)) {
                format.oblique = degreesToRadians(*degrees);
            }
            break;
        case U'T':
            if (const std::optional<double> tracking = positiveNumber(argument)) {
                format.tracking = *tracking;
            }
            break;
        case U'C':
            if (const std::optional<Color> color = indexColor(argument)) {
                format.color = color;
            }
            break;
        case U'c':
            if (const std::optional<Color> color = trueColor(argument)) {
                format.color = color;
            }
            break;
        default:
            break;  // `\A` and `\p`: hidden, without effect
    }
}

void applyToggle(char32_t code, MTextFormat& format) {
    switch (code) {
        case U'L':
            format.underline = true;
            break;
        case U'l':
            format.underline = false;
            break;
        case U'O':
            format.overline = true;
            break;
        case U'o':
            format.overline = false;
            break;
        case U'K':
            format.strike = true;
            break;
        default:  // U'k'
            format.strike = false;
            break;
    }
}

StackKind stackKind(char32_t separator) {
    switch (separator) {
        case U'#':
            return StackKind::Diagonal;
        case U'^':
            return StackKind::Tolerance;
        default:  // U'/'
            return StackKind::Horizontal;
    }
}

/// The stacked fraction of the argument of `\S`.
MTextStack stackOf(std::u32string_view argument) {
    MTextStack stack{.top = {}, .bottom = {}, .kind = StackKind::Horizontal};
    std::u32string* side = &stack.top;
    bool split = false;
    std::size_t i = 0;
    while (i < argument.size()) {
        const char32_t c = argument[i];
        if (!split && (c == U'/' || c == U'#' || c == U'^')) {
            stack.kind = stackKind(c);
            split = true;
            side = &stack.bottom;
            ++i;
            if (c == U'^' && i < argument.size() && argument[i] == U' ') {
                ++i;  // AutoCAD saves a tolerance as "^ "
            }
            continue;
        }
        if (c == U'%' && i + kPercentCodeLength <= argument.size() && argument[i + 1] == U'%') {
            if (const std::optional<char32_t> converted = percentCharacter(argument[i + 2])) {
                *side += *converted;
                i += kPercentCodeLength;
                continue;
            }
        }
        if (c != U'\\' || i + 1 == argument.size()) {
            *side += c;
            ++i;
            continue;
        }
        const char32_t code = argument[i + 1];
        if (code == U'\\' || code == U'{' || code == U'}') {
            *side += code;
            i += 2;
        } else if (code == U'~') {
            *side += kNoBreakSpace;
            i += 2;
        } else if (const std::optional<char32_t> unicode =
                       code == U'U' ? unicodeCode(argument, i) : std::nullopt) {
            *side += *unicode;
            i += kUnicodeCodeLength;
        } else {
            // As argumentEnd(): the character after the backslash is never a
            // separator; both stay.
            *side += c;
            *side += code;
            i += 2;
        }
    }
    return stack;
}

/// One reading of a value: the decoded characters, the format and the
/// paragraphs built so far.
class Reader {
public:
    Reader(std::string_view value, const MTextFormat& base)
        : m_decoded(decodeWithOffsets(value)), m_format(base), m_paragraphs(1) {}

    std::vector<MTextParagraph> read() && {
        const std::u32string_view text = m_decoded.text;
        std::size_t i = 0;
        while (i < text.size()) {
            const char32_t c = text[i];
            if (c == U'{') {
                m_saved.push_back(m_format);
                ++i;
                continue;
            }
            if (c == U'}') {
                if (!m_saved.empty()) {
                    m_format = m_saved.back();
                    m_saved.pop_back();
                }
                ++i;
                continue;
            }
            if (c == U'%' && i + kPercentCodeLength <= text.size() && text[i + 1] == U'%') {
                if (const std::optional<char32_t> converted = percentCharacter(text[i + 2])) {
                    addCharacter(*converted, i, i + kPercentCodeLength);
                    i += kPercentCodeLength;
                    continue;
                }
            }
            if (c != U'\\' || i + 1 == text.size()) {
                addCharacter(c, i, i + 1);
                ++i;
                continue;
            }
            i = readCode(i);
        }
        return std::move(m_paragraphs);
    }

private:
    /// The code whose backslash is at `i`; the index after it.
    std::size_t readCode(std::size_t i) {
        const std::u32string_view text = m_decoded.text;
        const char32_t code = text[i + 1];
        if (code == U'P') {
            m_paragraphs.emplace_back();
            m_runStart = m_decoded.offsets[i + 2];
            return i + 2;
        }
        if (code == U'~') {
            addCharacter(kNoBreakSpace, i, i + 2);
            return i + 2;
        }
        if (code == U'\\' || code == U'{' || code == U'}') {
            addCharacter(code, i, i + 2);
            return i + 2;
        }
        if (isToggleCode(code)) {
            applyToggle(code, m_format);
            return i + 2;
        }
        if (const std::optional<char32_t> unicode =
                code == U'U' ? unicodeCode(text, i) : std::nullopt) {
            addCharacter(*unicode, i, i + kUnicodeCodeLength);
            return i + kUnicodeCodeLength;
        }
        if (const std::size_t end = takesArgument(code) ? argumentEnd(text, i + 2) : kNoEnd;
            end != kNoEnd) {
            const std::u32string_view argument = text.substr(i + 2, end - i - 2);
            if (code == U'S') {
                addStack(stackOf(argument), i, end + 1);
            } else {
                applyArgument(code, argument, m_format);
            }
            return end + 1;
        }
        // Any other code, and one without its ';', stays as written: the
        // characters after the backslash are read again as text.
        addCharacter(U'\\', i, i + 1);
        return i + 1;
    }

    /// A character from the decoded characters [from, to).
    void addCharacter(char32_t c, std::size_t from, std::size_t to) {
        MTextParagraph& paragraph = m_paragraphs.back();
        if (paragraph.empty() || paragraph.back().stack || !(paragraph.back().format == m_format)) {
            paragraph.push_back(MTextRun{.text = {},
                                         .stack = std::nullopt,
                                         .format = m_format,
                                         .sourceBegin = m_runStart,
                                         .sourceEnd = m_runStart,
                                         .units = {}});
        }
        MTextRun& run = paragraph.back();
        run.text += c;
        run.units.push_back(
            MTextUnit{.sourceBegin = m_decoded.offsets[from], .sourceEnd = m_decoded.offsets[to]});
        run.sourceEnd = m_decoded.offsets[to];
        m_runStart = run.sourceEnd;
    }

    /// A stacked fraction from the decoded characters [from, to): always a
    /// piece of its own.
    void addStack(MTextStack stack, std::size_t from, std::size_t to) {
        const std::size_t end = m_decoded.offsets[to];
        m_paragraphs.back().push_back(MTextRun{
            .text = {},
            .stack = std::move(stack),
            .format = m_format,
            .sourceBegin = m_runStart,
            .sourceEnd = end,
            .units = {MTextUnit{.sourceBegin = m_decoded.offsets[from], .sourceEnd = end}}});
        m_runStart = end;
    }

    Decoded m_decoded;
    MTextFormat m_format;
    std::vector<MTextFormat> m_saved;
    std::vector<MTextParagraph> m_paragraphs;
    std::size_t m_runStart{0};  ///< byte where the next piece starts
};

}  // namespace

std::vector<MTextParagraph> formattedParagraphs(std::string_view value, const MTextFormat& base) {
    return Reader(value, base).read();
}

}  // namespace tg::core
