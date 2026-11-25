// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/mtext_edit.hpp"

#include "mtext_layout.hpp"
#include "tg/core/color.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/mtext_format.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/core/vec2.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace tg::core {

/// One change of the bytes of the value: [begin, end) replaced by the codes
/// of `prefix`, the characters of `text` (or the stacked fraction `stack`)
/// and the codes of `suffix`.
struct MTextEditModel::Splice {
    std::size_t begin{0}, end{0};
    std::string prefix;
    std::u32string text;
    std::optional<MTextStack> stack;
    std::string suffix;

    /// [begin, end) removed.
    static Splice removal(std::size_t begin, std::size_t end) {
        Splice splice;
        splice.begin = begin;
        splice.end = end;
        return splice;
    }

    /// Codes written at `at`.
    static Splice codes(std::size_t at, std::string prefix, std::string suffix) {
        Splice splice = removal(at, at);
        splice.prefix = std::move(prefix);
        splice.suffix = std::move(suffix);
        return splice;
    }
};

/// A value written from splices, with where the content of each one starts
/// and where its text (or fraction) ends.
struct MTextEditModel::Rendered {
    std::string value;
    std::vector<std::size_t> starts, ends;
};

namespace {

using Kind = MTextEditUnit::Kind;
using ByteRange = std::pair<std::size_t, std::size_t>;

/// What dimensionText() replaces with the measurement: the first one only.
constexpr std::string_view kPlace = "<>";
constexpr std::size_t kPlaceLength = kPlace.size();

constexpr char32_t kReplacement = U'�';
constexpr std::string_view kReplacementUtf8 = "\xEF\xBF\xBD";

/// In a signature (signatureOf()): a paragraph break, and the three marks
/// of a stacked fraction. Outside Unicode, so never a character of a value.
constexpr char32_t kParagraphKey = 0x110001;
constexpr char32_t kStackKey = 0x110002;

/// The font name written by a new `\f` code: the ADR 0054, point 1, draws
/// any name with Liberation Sans, which has the metrics of Arial; Arial is
/// what AutoCAD writes (autocad-mtext-formati.dxf) and finds everywhere.
constexpr std::string_view kFontName = "Arial";

// The ACI numbers of `\C` for ByBlock and ByLayer, as PT7 reads them.
constexpr int kAciByBlock = 0;
constexpr int kAciByLayer = 256;
// `\c` writes blue in the high byte and red in the low one.
constexpr unsigned kByteBits = 8;

/// The codes that end at ';', as PT7 reads them (mtext_format.cpp).
bool takesArgument(char code) {
    switch (code) {
        case 'f':
        case 'F':
        case 'H':
        case 'W':
        case 'Q':
        case 'T':
        case 'A':
        case 'C':
        case 'c':
        case 'p':
        case 'S':
            return true;
        default:
            return false;
    }
}

/// The ';' that ends an argument starting at `from`, or npos when a `\P`
/// or the end of the value comes first, as PT7. ';', '\' and 'P' are ASCII,
/// and a byte of a multibyte UTF-8 sequence never is: bytes suffice.
std::size_t argumentEnd(std::string_view value, std::size_t from) {
    for (std::size_t i = from; i < value.size(); ++i) {
        if (value[i] == ';') {
            return i;
        }
        if (value[i] == '\\' && i + 1 < value.size()) {
            if (value[i + 1] == 'P') {
                return std::string_view::npos;
            }
            ++i;  // an escaped character never ends the argument
        }
    }
    return std::string_view::npos;
}

/// The bytes of each paragraph of `value`, as formattedParagraphs() divides
/// them: from after the `\P` before it (or 0) to the `\P` after it (or the
/// end of the value).
std::vector<ByteRange> paragraphRanges(std::string_view value) {
    std::vector<ByteRange> ranges;
    std::size_t begin = 0;
    std::size_t i = 0;
    while (i < value.size()) {
        if (value[i] != '\\' || i + 1 == value.size()) {
            ++i;
            continue;
        }
        const char code = value[i + 1];
        if (code == 'P') {
            ranges.emplace_back(begin, i);
            i += 2;
            begin = i;
            continue;
        }
        if (code == '\\' || code == '{' || code == '}' || code == '~') {
            i += 2;
            continue;
        }
        if (takesArgument(code)) {
            if (const std::size_t end = argumentEnd(value, i + 2); end != std::string_view::npos) {
                i = end + 1;
                continue;
            }
        }
        ++i;  // a toggle, `\U+XXXX` or a backslash read as text: no `\P` in it
    }
    ranges.emplace_back(begin, value.size());
    return ranges;
}

char32_t separatorOf(StackKind kind) {
    switch (kind) {
        case StackKind::Diagonal:
            return U'#';
        case StackKind::Tolerance:
            return U'^';
        case StackKind::Horizontal:
            break;
    }
    return U'/';
}

/// What a value shows, in a form that two values can be compared with:
/// characters, a mark between paragraphs, and each stacked fraction as its
/// kind and its two texts between marks.
void appendStackKey(const MTextStack& stack, std::u32string& out) {
    out += kStackKey;
    out += stack.top;
    out += kStackKey;
    out += separatorOf(stack.kind);
    out += stack.bottom;
    out += kStackKey;
}

std::u32string signatureOf(std::string_view value) {
    std::u32string signature;
    bool first = true;
    for (const MTextParagraph& paragraph : formattedParagraphs(value, MTextFormat{})) {
        if (!first) {
            signature += kParagraphKey;
        }
        first = false;
        for (const MTextRun& run : paragraph) {
            if (run.stack) {
                appendStackKey(*run.stack, signature);
            } else {
                signature += run.text;
            }
        }
    }
    return signature;
}

/// A character written so that PT7 reads it back as it is: `\`, `{` and `}`
/// escaped, a new line as `\P`; with `safe`, '%' as `%%%`, so that it never
/// forms `%%c`, `%%d` or `%%p` with the characters around it.
void appendCharacter(char32_t c, bool safe, std::string& out) {
    switch (c) {
        case U'\\':
            out += R"(\\)";
            return;
        case U'{':
            out += R"(\{)";
            return;
        case U'}':
            out += R"(\})";
            return;
        case U'\n':
            out += R"(\P)";
            return;
        case U'%':
            out += safe ? "%%%" : "%";
            return;
        default:
            out += mtext_layout::toUtf8(std::u32string_view{&c, 1});
            return;
    }
}

void appendCharacters(std::u32string_view text, bool safe, std::string& out) {
    for (const char32_t c : text) {
        appendCharacter(c, safe, out);
    }
}

/// `\S<top><separator><bottom>;`, a '^' followed by a space as AutoCAD saves
/// it (PT7 skips that space).
void appendStack(const MTextStack& stack, bool safe, std::string& out) {
    out += R"(\S)";
    appendCharacters(stack.top, safe, out);
    switch (stack.kind) {
        case StackKind::Horizontal:
            out += '/';
            break;
        case StackKind::Diagonal:
            out += '#';
            break;
        case StackKind::Tolerance:
            out += "^ ";
            break;
    }
    appendCharacters(stack.bottom, safe, out);
    out += ';';
}

/// `number` in the shortest form that reads back as the same double.
std::string numberText(double number) {
    constexpr std::size_t kLongest = 32;  // more than any shortest double
    std::array<char, kLongest> buffer{};
    const std::to_chars_result result =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), number);
    assert(result.ec == std::errc{});
    return {buffer.data(), result.ptr};
}

bool isPositiveFinite(double number) {
    return std::isfinite(number) && number > 0.0;
}

/// A change PT7 would read back: a height, width factor and tracking
/// positive and finite, an oblique finite.
bool isValid(const MTextFormatChange& change) {
    using enum MTextFormatChange::Kind;
    switch (change.kind) {
        case Height:
        case WidthFactor:
        case Tracking:
            return isPositiveFinite(change.number);
        case Oblique:
            return std::isfinite(change.number);
        default:
            return true;
    }
}

/// `format` with `change`, as PT7 reads its code.
MTextFormat changed(MTextFormat format, const MTextFormatChange& change) {
    using enum MTextFormatChange::Kind;
    switch (change.kind) {
        case Bold:
            format.bold = change.on;
            break;
        case Italic:
            format.italic = change.on;
            break;
        case Underline:
            format.underline = change.on;
            break;
        case Overline:
            format.overline = change.on;
            break;
        case Strike:
            format.strike = change.on;
            break;
        case Height:
            format.height = change.number;
            break;
        case WidthFactor:
            format.widthFactor = change.number;
            break;
        case Oblique:
            format.oblique = degreesToRadians(change.number);
            break;
        case Tracking:
            format.tracking = change.number;
            break;
        case Color:
            format.color = change.color;
            break;
        case Stack:
            break;
    }
    return format;
}

std::string colorCode(const Color& color) {
    switch (color.kind()) {
        case Color::Kind::ByBlock:
            return R"(\C)" + std::to_string(kAciByBlock) + ';';
        case Color::Kind::ByLayer:
            return R"(\C)" + std::to_string(kAciByLayer) + ';';
        case Color::Kind::Index:
            return R"(\C)" + std::to_string(color.index()) + ';';
        case Color::Kind::Rgb:
            break;
    }
    const Rgb rgb = color.rgb();
    const std::uint32_t bgr = (std::uint32_t{rgb.b} << (2 * kByteBits)) |
                              (std::uint32_t{rgb.g} << kByteBits) | std::uint32_t{rgb.r};
    return R"(\c)" + std::to_string(bgr) + ';';
}

/// The code of `change`, which gives `result`.
std::string codeOf(const MTextFormatChange& change, const MTextFormat& result) {
    using enum MTextFormatChange::Kind;
    switch (change.kind) {
        case Bold:
        case Italic:
            return R"(\f)" + std::string{kFontName} + "|b" + (result.bold ? "1" : "0") + "|i" +
                   (result.italic ? "1" : "0") + ';';
        case Underline:
            return change.on ? R"(\L)" : R"(\l)";
        case Overline:
            return change.on ? R"(\O)" : R"(\o)";
        case Strike:
            return change.on ? R"(\K)" : R"(\k)";
        case Height:
            return R"(\H)" + numberText(change.number) + ';';
        case WidthFactor:
            return R"(\W)" + numberText(change.number) + ';';
        case Oblique:
            return R"(\Q)" + numberText(change.number) + ';';
        case Tracking:
            return R"(\T)" + numberText(change.number) + ';';
        case Color:
            return colorCode(change.color);
        case Stack:
            break;
    }
    return {};
}

/// The codes that make `format` into `format` with `changes`, one per
/// change that makes a difference; `format` becomes the result.
std::string codesOf(const std::vector<MTextFormatChange>& changes, MTextFormat& format) {
    std::string codes;
    for (const MTextFormatChange& change : changes) {
        const MTextFormat next = changed(format, change);
        if (!(next == format)) {
            codes += codeOf(change, next);
            format = next;
        }
    }
    return codes;
}

/// The distance from `point` to the segment [a, b].
double segmentDistance(const Vec2& point, const Vec2& a, const Vec2& b) {
    const Vec2 along = b - a;
    const double length = along.lengthSquared();
    if (length == 0.0) {
        return distance(point, a);
    }
    const double t = std::clamp(dot(point - a, along) / length, 0.0, 1.0);
    return distance(point, a + (along * t));
}

}  // namespace

MTextEditModel::MTextEditModel(MText mtext, MTextEditOptions options)
    : m_shown(std::move(mtext)), m_options(std::move(options)), m_value(m_shown.value) {
    m_base.height = m_shown.height;
    m_base.widthFactor = m_options.widthFactor;
    m_base.oblique = m_options.oblique;
    rebuild();
}

void MTextEditModel::rebuild() {
    // The units of the value, read by PT7.
    const std::vector<MTextParagraph> paragraphs = formattedParagraphs(m_value, m_base);
    m_paragraphRanges = paragraphRanges(m_value);
    assert(m_paragraphRanges.size() == paragraphs.size());

    // The first <> is the measurement when it is two characters of its own.
    std::optional<std::size_t> place;
    if (m_options.measurement) {
        if (const std::size_t at = m_value.find(kPlace); at != std::string::npos) {
            place = at;
        }
    }
    m_paragraphs.assign(paragraphs.size(), {});
    for (std::size_t p = 0; p < paragraphs.size(); ++p) {
        std::vector<Unit>& units = m_paragraphs[p];
        for (const MTextRun& run : paragraphs[p]) {
            for (std::size_t k = 0; k < run.units.size(); ++k) {
                Unit unit;
                unit.begin = run.units[k].sourceBegin;
                unit.end = run.units[k].sourceEnd;
                unit.format = run.format;
                if (run.stack) {
                    unit.kind = Kind::Stack;
                    unit.stack = run.stack;
                } else {
                    unit.character = run.text[k];
                }
                if (place && unit.kind == Kind::Character && unit.character == U'>' &&
                    unit.begin == *place + 1 && unit.end == *place + kPlaceLength &&
                    !units.empty() && units.back().kind == Kind::Character &&
                    units.back().character == U'<' && units.back().begin == *place) {
                    units.back().kind = Kind::Measurement;
                    units.back().end = unit.end;
                    continue;
                }
                units.push_back(std::move(unit));
            }
        }
    }

    // What the editor draws: the first <> replaced as dimensionText() does.
    m_shown.value = m_value;
    const std::size_t measurementLength = place ? m_options.measurement->size() : 0;
    if (place) {
        m_shown.value.replace(*place, kPlaceLength, *m_options.measurement);
    }
    const mtext_layout::Layout layout =
        mtext_layout::layout(m_shown, m_options.widthFactor, m_options.oblique);
    m_rows.clear();
    for (const mtext_layout::Row& row : layout.rows) {
        m_rows.emplace_back(row.start, row.height);
    }

    // Where each unit of shown() is: from the pieces of PT8; a space where a
    // row breaks, at the end of the row it closes, with no width.
    struct Shown {
        std::optional<std::size_t> row;  ///< the row it is drawn on
        Location left, right;
    };

    std::vector<std::vector<std::vector<Shown>>> shownUnits(layout.paragraphs.size());
    for (std::size_t dp = 0; dp < layout.paragraphs.size(); ++dp) {
        for (const MTextRun& run : layout.paragraphs[dp]) {
            shownUnits[dp].emplace_back(run.units.size());
        }
    }
    for (std::size_t r = 0; r < layout.rows.size(); ++r) {
        for (const mtext_layout::Piece& piece : layout.rows[r].pieces) {
            for (std::size_t j = 0; j < piece.unitCount; ++j) {
                shownUnits[piece.paragraph][piece.run][piece.firstUnit + j] =
                    Shown{r, {r, piece.boundaries[j]}, {r, piece.boundaries[j + 1]}};
            }
        }
    }
    // The first row of each paragraph of shown(): one row for a paragraph
    // without units, otherwise every row has a piece of it.
    std::vector<std::size_t> firstRow(layout.paragraphs.size(), 0);
    std::size_t nextRow = 0;
    for (std::size_t dp = 0; dp < layout.paragraphs.size(); ++dp) {
        firstRow[dp] = std::min(nextRow, layout.rows.size() - 1);
        if (layout.paragraphs[dp].empty()) {
            ++nextRow;
            continue;
        }
        while (nextRow < layout.rows.size() && !layout.rows[nextRow].pieces.empty() &&
               layout.rows[nextRow].pieces.front().paragraph == dp) {
            ++nextRow;
        }
    }

    // A byte of shown() as a byte of the value: inside the measurement, its
    // place.
    const auto toValue = [&](std::size_t offset) {
        if (!place || offset < *place) {
            return offset;
        }
        if (offset >= *place + measurementLength) {
            return offset - measurementLength + kPlaceLength;
        }
        return *place;
    };
    std::vector<Unit*> flat;
    for (std::vector<Unit>& units : m_paragraphs) {
        for (Unit& unit : units) {
            flat.push_back(&unit);
        }
    }
    std::vector<bool> placed(flat.size(), false);
    for (std::size_t dp = 0; dp < layout.paragraphs.size(); ++dp) {
        std::optional<Location> previousEnd;
        for (std::size_t r = 0; r < layout.paragraphs[dp].size(); ++r) {
            const MTextRun& run = layout.paragraphs[dp][r];
            for (std::size_t k = 0; k < run.units.size(); ++k) {
                Shown& shown = shownUnits[dp][r][k];
                if (!shown.row) {
                    const Location end =
                        previousEnd.value_or(Location{firstRow[dp], m_rows[firstRow[dp]].first.x});
                    shown.left = end;
                    shown.right = end;
                }
                previousEnd = shown.right;
                // The unit of the value it belongs to.
                const std::size_t offset = toValue(run.units[k].sourceBegin);
                const auto found = std::ranges::upper_bound(
                    flat, offset, std::less{}, [](const Unit* unit) { return unit->begin; });
                if (found == flat.begin()) {
                    continue;
                }
                const auto index = static_cast<std::size_t>(found - flat.begin()) - 1;
                Unit& unit = *flat[index];
                if (offset >= unit.end) {
                    continue;
                }
                if (!placed[index]) {
                    unit.left = shown.left;
                    placed[index] = true;
                }
                unit.right = shown.right;
                if (shown.row) {
                    unit.rows = unit.rows ? std::pair{std::min(unit.rows->first, *shown.row),
                                                      std::max(unit.rows->second, *shown.row)}
                                          : std::pair{*shown.row, *shown.row};
                }
            }
        }
    }

    // Where each paragraph starts; a unit with nothing shown (an empty
    // measurement) where the unit before it ends.
    const std::vector<ByteRange> shownRanges = paragraphRanges(m_shown.value);
    m_paragraphStarts.assign(m_paragraphs.size(), Location{});
    std::size_t index = 0;
    for (std::size_t p = 0; p < m_paragraphs.size(); ++p) {
        std::size_t begin = m_paragraphRanges[p].first;
        if (place && begin > *place) {
            begin = begin + measurementLength - kPlaceLength;
        }
        std::size_t dp = 0;
        while (dp + 1 < shownRanges.size() && begin > shownRanges[dp].second) {
            ++dp;
        }
        dp = std::min(dp, firstRow.size() - 1);
        m_paragraphStarts[p] = Location{firstRow[dp], m_rows[firstRow[dp]].first.x};
        std::optional<Location> previousEnd;
        for (Unit& unit : m_paragraphs[p]) {
            if (!placed[index]) {
                unit.left = previousEnd.value_or(m_paragraphStarts[p]);
                unit.right = unit.left;
            }
            previousEnd = unit.right;
            ++index;
        }
    }

    m_positions.clear();
    for (std::size_t p = 0; p < m_paragraphs.size(); ++p) {
        for (std::size_t k = 0; k <= m_paragraphs[p].size(); ++k) {
            m_positions.push_back(Position{p, k});
        }
    }
}

bool MTextEditModel::isEmpty() const {
    return std::ranges::all_of(m_paragraphs,
                               [](const std::vector<Unit>& units) { return units.empty(); });
}

std::vector<MTextEditUnit> MTextEditModel::units() const {
    std::vector<MTextEditUnit> result;
    for (std::size_t p = 0; p < m_paragraphs.size(); ++p) {
        for (const Unit& unit : m_paragraphs[p]) {
            result.push_back(MTextEditUnit{.paragraph = p,
                                           .sourceBegin = unit.begin,
                                           .sourceEnd = unit.end,
                                           .kind = unit.kind});
        }
    }
    return result;
}

MTextEditModel::Location MTextEditModel::locationOf(const Position& position) const {
    const std::vector<Unit>& units = m_paragraphs[position.paragraph];
    if (units.empty()) {
        return m_paragraphStarts[position.paragraph];
    }
    // At the start of the unit after it; at the end of the paragraph, at the
    // end of the last unit.
    return position.index < units.size() ? units[position.index].left : units.back().right;
}

MTextCaret MTextEditModel::caretAt(std::size_t position) const {
    assert(position < m_positions.size());
    const Location location = locationOf(m_positions[position]);
    const SinCos direction = sinCos(m_shown.rotation);
    const Vec2 xAxis{direction.cosine, direction.sine};
    const Vec2 yAxis{-direction.sine, direction.cosine};
    const auto& [start, height] = m_rows[location.row];
    return MTextCaret{.point = m_shown.insertion + (xAxis * location.x) + (yAxis * start.y),
                      .height = height,
                      .row = location.row};
}

std::size_t MTextEditModel::closestPosition(const Vec2& point) const {
    const SinCos direction = sinCos(m_shown.rotation);
    const Vec2 yAxis{-direction.sine, direction.cosine};
    std::size_t best = 0;
    double bestDistance = 0.0;
    for (std::size_t i = 0; i < m_positions.size(); ++i) {
        const MTextCaret caret = caretAt(i);
        const double d = segmentDistance(point, caret.point, caret.point + (yAxis * caret.height));
        if (i == 0 || d < bestDistance) {
            best = i;
            bestDistance = d;
        }
    }
    return best;
}

std::size_t MTextEditModel::rowCount() const {
    return m_rows.size();
}

std::size_t MTextEditModel::positionIndex(std::size_t paragraph, std::size_t index) const {
    std::size_t result = 0;
    for (std::size_t p = 0; p < paragraph; ++p) {
        result += m_paragraphs[p].size() + 1;
    }
    return result + index;
}

std::size_t MTextEditModel::rowStart(std::size_t row) const {
    assert(row < m_rows.size());
    for (std::size_t p = 0; p < m_paragraphs.size(); ++p) {
        for (std::size_t k = 0; k < m_paragraphs[p].size(); ++k) {
            const auto& rows = m_paragraphs[p][k].rows;
            if (rows && rows->first <= row && row <= rows->second) {
                return positionIndex(p, k);
            }
        }
    }
    for (std::size_t i = 0; i < m_positions.size(); ++i) {
        if (locationOf(m_positions[i]).row >= row) {
            return i;
        }
    }
    return m_positions.size() - 1;
}

std::size_t MTextEditModel::rowEnd(std::size_t row) const {
    assert(row < m_rows.size());
    for (std::size_t p = m_paragraphs.size(); p-- > 0;) {
        for (std::size_t k = m_paragraphs[p].size(); k-- > 0;) {
            const auto& rows = m_paragraphs[p][k].rows;
            if (rows && rows->first <= row && row <= rows->second) {
                return positionIndex(p, k + 1);
            }
        }
    }
    for (std::size_t i = m_positions.size(); i-- > 0;) {
        if (locationOf(m_positions[i]).row <= row) {
            return i;
        }
    }
    return 0;
}

void MTextEditModel::setCaret(std::size_t position) {
    select(position, position);
}

void MTextEditModel::select(std::size_t anchor, std::size_t caret) {
    assert(anchor < m_positions.size() && caret < m_positions.size());
    m_anchor = anchor;
    m_caret = caret;
    m_pending.clear();
}

std::size_t MTextEditModel::insertionOffset(const Position& position) const {
    const std::vector<Unit>& units = m_paragraphs[position.paragraph];
    if (position.index > 0) {
        return units[position.index - 1].end;
    }
    return units.empty() ? m_paragraphRanges[position.paragraph].first : units.front().begin;
}

MTextFormat MTextEditModel::formatAt(const Position& position) const {
    const std::vector<Unit>& units = m_paragraphs[position.paragraph];
    if (position.index > 0) {
        return units[position.index - 1].format;
    }
    if (!units.empty()) {
        return units.front().format;
    }
    // An empty paragraph: the format of a character written at its start,
    // which no code before it can join (it starts after a `\P`, or at 0).
    const std::string probe = m_value.substr(0, m_paragraphRanges[position.paragraph].first) + "x";
    const std::vector<MTextParagraph> paragraphs = formattedParagraphs(probe, m_base);
    return paragraphs.back().back().format;
}

MTextFormat MTextEditModel::formatAtCaret() const {
    MTextFormat format = formatAt(m_positions[m_caret]);
    (void)codesOf(m_pending, format);
    return format;
}

std::size_t MTextEditModel::positionAtOffset(std::size_t offset) const {
    for (std::size_t p = 0; p < m_paragraphs.size(); ++p) {
        const auto [begin, end] = m_paragraphRanges[p];
        if (offset < begin || offset > end) {
            continue;
        }
        const auto before = std::ranges::count_if(
            m_paragraphs[p], [offset](const Unit& unit) { return unit.begin < offset; });
        return positionIndex(p, static_cast<std::size_t>(before));
    }
    return m_positions.size() - 1;
}

std::vector<MTextEditModel::Splice> MTextEditModel::selectionRemovals() const {
    const Position from = m_positions[std::min(m_anchor, m_caret)];
    const Position to = m_positions[std::max(m_anchor, m_caret)];
    std::vector<Splice> splices;
    for (std::size_t p = from.paragraph; p <= to.paragraph; ++p) {
        const std::vector<Unit>& units = m_paragraphs[p];
        const std::size_t first = p == from.paragraph ? from.index : 0;
        const std::size_t last = p == to.paragraph ? to.index : units.size();
        for (std::size_t k = first; k < last; ++k) {
            splices.push_back(Splice::removal(units[k].begin, units[k].end));
        }
        if (p < to.paragraph) {
            const std::size_t at = m_paragraphRanges[p].second;
            splices.push_back(Splice::removal(at, at + 2));  // its `\P`
        }
    }
    return splices;
}

MTextEditModel::Rendered MTextEditModel::render(const std::vector<Splice>& splices,
                                                bool safe) const {
    Rendered result;
    std::vector<const Unit*> units;
    if (safe) {
        for (const std::vector<Unit>& paragraph : m_paragraphs) {
            for (const Unit& unit : paragraph) {
                units.push_back(&unit);
            }
        }
    }
    // The bytes [from, to) of the value; with `safe`, each character read as
    // text that could join the bytes around it written as it is shown.
    const auto copy = [&](std::size_t from, std::size_t to) {
        std::size_t at = from;
        for (const Unit* unit : units) {
            if (unit->begin < from || unit->end > to || unit->kind != Kind::Character) {
                continue;
            }
            const std::string_view bytes =
                std::string_view{m_value}.substr(unit->begin, unit->end - unit->begin);
            std::string_view written = bytes;
            if (bytes == "\\") {
                written = R"(\\)";
            } else if (bytes == "%") {
                written = "%%%";
            } else if (unit->character == kReplacement && bytes.front() != '\\' &&
                       bytes.front() != '%') {
                // An ill-formed UTF-8 sequence, which PT7 reads as U+FFFD.
                written = kReplacementUtf8;
            }
            result.value.append(m_value, at, unit->begin - at);
            result.value += written;
            at = unit->end;
        }
        result.value.append(m_value, at, to - at);
    };
    std::size_t at = 0;
    for (const Splice& splice : splices) {
        copy(at, splice.begin);
        result.starts.push_back(result.value.size());
        result.value += splice.prefix;
        if (splice.stack) {
            appendStack(*splice.stack, safe, result.value);
        } else {
            appendCharacters(splice.text, safe, result.value);
        }
        result.ends.push_back(result.value.size());
        result.value += splice.suffix;
        at = splice.end;
    }
    copy(at, m_value.size());
    return result;
}

std::u32string MTextEditModel::expectedSignature(const std::vector<Splice>& splices) const {
    std::u32string signature;
    std::size_t next = 0;
    const auto flush = [&](std::size_t offset) {
        while (next < splices.size() && splices[next].begin <= offset) {
            const Splice& splice = splices[next++];
            if (splice.stack) {
                appendStackKey(*splice.stack, signature);
                continue;
            }
            for (const char32_t c : splice.text) {
                signature += c == U'\n' ? kParagraphKey : c;
            }
        }
    };
    const auto removed = [&](std::size_t offset) {
        return std::ranges::any_of(splices, [offset](const Splice& splice) {
            return splice.begin <= offset && offset < splice.end;
        });
    };
    for (std::size_t p = 0; p < m_paragraphs.size(); ++p) {
        if (p > 0) {
            const std::size_t at = m_paragraphRanges[p - 1].second;
            flush(at);
            if (!removed(at)) {
                signature += kParagraphKey;
            }
        }
        for (const Unit& unit : m_paragraphs[p]) {
            flush(unit.begin);
            if (removed(unit.begin)) {
                continue;
            }
            switch (unit.kind) {
                case Kind::Character:
                    signature += unit.character;
                    break;
                case Kind::Stack:
                    if (unit.stack) {
                        appendStackKey(*unit.stack, signature);
                    }
                    break;
                case Kind::Measurement:
                    signature += U"<>";
                    break;
            }
        }
    }
    flush(m_value.size());
    return signature;
}

void MTextEditModel::commit(std::vector<Splice> splices, CaretAfter caretAfter) {
    std::ranges::stable_sort(splices, std::less{}, &Splice::begin);
    Rendered rendered = render(splices, false);
    if (signatureOf(rendered.value) != expectedSignature(splices)) {
        // Bytes joined into a code: write the characters as they are shown.
        rendered = render(splices, true);
    }
    const std::size_t anchor = m_anchor;
    const std::size_t caret = m_caret;
    m_value = std::move(rendered.value);
    rebuild();
    m_pending.clear();
    const std::size_t last = m_positions.size() - 1;
    switch (caretAfter) {
        case CaretAfter::FirstSplice:
            m_caret = positionAtOffset(rendered.ends.front());
            m_anchor = m_caret;
            break;
        case CaretAfter::SelectSplices:
            m_anchor = positionAtOffset(rendered.starts.front());
            m_caret = positionAtOffset(rendered.ends.back());
            break;
        case CaretAfter::Same:
            m_anchor = std::min(anchor, last);
            m_caret = std::min(caret, last);
            break;
    }
}

const std::string& MTextEditModel::insertText(std::string_view text) {
    Splice insertion;
    insertion.text = decodeUtf8(text);
    if (!m_pending.empty() && !hasSelection()) {
        MTextFormat format = formatAt(m_positions[m_caret]);
        const std::string codes = codesOf(m_pending, format);
        if (!codes.empty()) {
            insertion.prefix = "{" + codes;
            insertion.suffix = "}";
        }
    }
    std::vector<Splice> splices;
    if (hasSelection()) {
        // In place of the first unit selected (or `\P`), with its format.
        splices = selectionRemovals();
        insertion.begin = splices.front().begin;
        insertion.end = splices.front().end;
        splices.front() = std::move(insertion);
    } else {
        const std::size_t at = insertionOffset(m_positions[m_caret]);
        insertion.begin = at;
        insertion.end = at;
        splices.push_back(std::move(insertion));
    }
    commit(std::move(splices), CaretAfter::FirstSplice);
    return m_value;
}

const std::string& MTextEditModel::newParagraph() {
    return insertText("\n");
}

const std::string& MTextEditModel::deleteBackward() {
    if (hasSelection()) {
        commit(selectionRemovals(), CaretAfter::FirstSplice);
        return m_value;
    }
    const Position position = m_positions[m_caret];
    if (position.index > 0) {
        const Unit& unit = m_paragraphs[position.paragraph][position.index - 1];
        commit({Splice::removal(unit.begin, unit.end)}, CaretAfter::FirstSplice);
    } else if (position.paragraph > 0) {
        const std::size_t at = m_paragraphRanges[position.paragraph - 1].second;
        commit({Splice::removal(at, at + 2)}, CaretAfter::FirstSplice);
    }
    return m_value;
}

const std::string& MTextEditModel::deleteForward() {
    if (hasSelection()) {
        commit(selectionRemovals(), CaretAfter::FirstSplice);
        return m_value;
    }
    const Position position = m_positions[m_caret];
    const std::vector<Unit>& units = m_paragraphs[position.paragraph];
    if (position.index < units.size()) {
        const Unit& unit = units[position.index];
        commit({Splice::removal(unit.begin, unit.end)}, CaretAfter::FirstSplice);
    } else if (position.paragraph + 1 < m_paragraphs.size()) {
        const std::size_t at = m_paragraphRanges[position.paragraph].second;
        commit({Splice::removal(at, at + 2)}, CaretAfter::FirstSplice);
    }
    return m_value;
}

const std::string& MTextEditModel::applyFormat(const MTextFormatChange& change) {
    if (change.kind == MTextFormatChange::Kind::Stack) {
        return applyStack(change.on);
    }
    if (!isValid(change)) {
        return m_value;
    }
    if (!hasSelection()) {
        // For what is written next here: one change of each kind.
        std::erase_if(m_pending, [&change](const MTextFormatChange& pending) {
            return pending.kind == change.kind;
        });
        m_pending.push_back(change);
        return m_value;
    }
    // Each stretch of units with no code between them, which therefore share
    // their format, wrapped in a group with the code.
    const Position from = m_positions[std::min(m_anchor, m_caret)];
    const Position to = m_positions[std::max(m_anchor, m_caret)];
    std::vector<Splice> splices;
    for (std::size_t p = from.paragraph; p <= to.paragraph; ++p) {
        const std::vector<Unit>& units = m_paragraphs[p];
        const std::size_t first = p == from.paragraph ? from.index : 0;
        const std::size_t last = p == to.paragraph ? to.index : units.size();
        std::size_t k = first;
        while (k < last) {
            std::size_t end = k + 1;
            while (end < last && units[end].begin == units[end - 1].end &&
                   units[end].format == units[k].format) {
                ++end;
            }
            const MTextFormat format = changed(units[k].format, change);
            if (!(format == units[k].format)) {
                splices.push_back(Splice::codes(units[k].begin, "{" + codeOf(change, format), {}));
                splices.push_back(Splice::codes(units[end - 1].end, {}, "}"));
            }
            k = end;
        }
    }
    if (!splices.empty()) {
        commit(std::move(splices), CaretAfter::Same);
    }
    return m_value;
}

const std::string& MTextEditModel::applyStack(bool on) {
    if (!hasSelection()) {
        return m_value;
    }
    const Position from = m_positions[std::min(m_anchor, m_caret)];
    const Position to = m_positions[std::max(m_anchor, m_caret)];
    std::vector<Splice> splices;
    if (on) {
        if (from.paragraph != to.paragraph) {
            return m_value;
        }
        const std::vector<Unit>& units = m_paragraphs[from.paragraph];
        std::u32string text;
        for (std::size_t k = from.index; k < to.index; ++k) {
            const bool joined = k == from.index || units[k].begin == units[k - 1].end;
            if (units[k].kind != Kind::Character || !joined || units[k].character == U';') {
                return m_value;
            }
            text += units[k].character;
        }
        const std::size_t at = text.find_first_of(U"/#^");
        if (at == std::u32string::npos) {
            return m_value;
        }
        StackKind kind = StackKind::Horizontal;
        if (text[at] == U'#') {
            kind = StackKind::Diagonal;
        } else if (text[at] == U'^') {
            kind = StackKind::Tolerance;
        }
        Splice stack = Splice::removal(units[from.index].begin, units[to.index - 1].end);
        stack.stack =
            MTextStack{.top = text.substr(0, at), .bottom = text.substr(at + 1), .kind = kind};
        splices.push_back(std::move(stack));
    } else {
        for (std::size_t p = from.paragraph; p <= to.paragraph; ++p) {
            const std::vector<Unit>& units = m_paragraphs[p];
            const std::size_t first = p == from.paragraph ? from.index : 0;
            const std::size_t last = p == to.paragraph ? to.index : units.size();
            for (std::size_t k = first; k < last; ++k) {
                if (const std::optional<MTextStack>& fraction = units[k].stack) {
                    const MTextStack& stack = *fraction;
                    Splice text = Splice::removal(units[k].begin, units[k].end);
                    text.text = stack.top + separatorOf(stack.kind) + stack.bottom;
                    splices.push_back(std::move(text));
                }
            }
        }
        if (splices.empty()) {
            return m_value;
        }
    }
    commit(std::move(splices), CaretAfter::SelectSplices);
    return m_value;
}

}  // namespace tg::core
