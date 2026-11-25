// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/color.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/mtext_format.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/// The model of the editor of a formatted text (docs/adr/0054, point 5;
/// docs/tappe/0.8.1/PT10-modello-di-modifica.md): the cursor positions of a
/// value with its formatting codes, where each one is drawn, and the edits,
/// each giving the new value with its codes. Without Qt: the editor in the
/// window (PT12) only turns keys and clicks into these calls.
namespace tg::core {

/// What the editor needs besides the MTEXT it opens on.
struct MTextEditOptions {
    /// The width factor and the oblique angle (radians) of the text style of
    /// the MTEXT, as generateOf() takes them: 1 and 0 when the style is not
    /// in the table.
    double widthFactor{1.0};
    double oblique{0.0};
    /// For the text of a dimension: what its first `<>` stands for, already
    /// formatted (TextEditRequest::measurement of PT11); nullopt otherwise.
    std::optional<std::string> measurement;
};

/// An editable unit of the value, in reading order: a character of PT7 (with
/// its escape), a whole stacked fraction, or the first `<>` of the text of a
/// dimension, shown as its measurement.
struct MTextEditUnit {
    enum class Kind : std::uint8_t { Character, Stack, Measurement };

    std::size_t paragraph{0};                ///< index of its paragraph (`\P`) in the value
    std::size_t sourceBegin{}, sourceEnd{};  ///< its bytes in the value
    Kind kind{Kind::Character};

    friend bool operator==(const MTextEditUnit&, const MTextEditUnit&) = default;
};

/// Where a cursor position is drawn.
struct MTextCaret {
    /// On the baseline of its row, in the drawing: the caret goes from here
    /// up by `height`, along the y axis of the text (rotated with it).
    Vec2 point;
    double height{0.0};  ///< the nominal height of its row (PT8)
    std::size_t row{0};  ///< index of its row in the layout, from the top
};

/// A format of docs/adr/0054, point 1, to apply to the selection, or to
/// what is written next at the cursor when nothing is selected.
struct MTextFormatChange {
    enum class Kind : std::uint8_t {
        /// `\fArial|b<0/1>|i<0/1>;`, bold on or off and the italic as it
        /// is: any name is drawn with Liberation Sans (docs/adr/0054, point
        /// 1), and Arial is the one AutoCAD writes
        Bold,
        Italic,     ///< the same code, italic on or off and the bold as it is
        Underline,  ///< `\L` on, `\l` off
        Overline,   ///< `\O` on, `\o` off
        Strike,     ///< `\K` on, `\k` off
        Height,     ///< `\H<number>;`, absolute, in drawing units
        WidthFactor,
        Oblique,  ///< `\Q<number>;`, number in degrees as the code writes them
        Tracking,
        Color,  ///< `\C<aci>;` (`\C0;` ByBlock, `\C256;` ByLayer) or `\c<bgr>;`
        Stack,  ///< on: the selection, with `/`, `#` or `^`, made a `\S…;`;
                ///< off: the stacked fractions of the selection made text
    };

    Kind kind{Kind::Bold};
    bool on{true};       ///< Bold to Strike, and Stack: apply or remove
    double number{0.0};  ///< Height, WidthFactor, Oblique and Tracking
    Color color{Color::byLayer()};
};

/// The value of an MTEXT being edited, its cursor positions and its
/// selection.
///
/// **Positions.** A paragraph of n units has n + 1 positions: before each
/// unit and at its end; the positions of the value are those of its
/// paragraphs in order, numbered from 0. A stacked fraction and the
/// measurement of a dimension are one unit each: two positions, before and
/// after, and none inside. A code that is not shown (`\A1;`, `\p…;`, a known
/// code with an argument outside its domain) is no unit: it stays in the
/// value where it was, and deleting a unit next to it leaves it; a code that
/// is not one (unknown, or without its ';') is text, one unit per character.
///
/// **Where a position is drawn** (choices of the editor, #927, [decisione]
/// 2): a position is drawn at the start of the unit after it, and at the end
/// of its paragraph after the last unit. The spaces where a row breaks are
/// units, but they are not drawn: each is drawn, with no width, at the end
/// of the row it closes. So, at an automatic break, the position before the
/// space is at the end of the row, and the one after it at the start of the
/// next row.
///
/// **Where text goes** (a choice of the editor, which does not change the
/// drawing): right after the unit before the position, before any code that
/// follows it, so it takes the format of the character before the cursor;
/// at the start of a paragraph right before its first unit, after the codes
/// that open it (the format of its first character); in an empty paragraph
/// right after its `\P`. Written in place of a selection, it takes the place
/// of the first unit selected, and its format. In `A\A1;B`, between A and B,
/// text goes before `\A1;`.
///
/// **Characters written** are always shown as written: `\`, `{` and `}` are
/// written `\\`, `\{` and `\}`, and a new line `\P`. When an edit would join
/// bytes into a code that changes what is shown (a `\` read as text followed
/// by a letter, `%%` followed by `c`, `d` or `p`, a code without its `;` that
/// finds one), the new value writes each character read as text that could
/// do so as it is shown: a lone `\` as `\\`, a lone `%` as `%%%`, an
/// ill-formed UTF-8 sequence as U+FFFD; the rest of the value is unchanged.
///
/// Without edits value() is the value it was built with, byte for byte.
class MTextEditModel {
public:
    /// The model of `mtext.value`, laid out as `mtext` with the values of
    /// `options`. For the text of a dimension `mtext` is the MTEXT of its
    /// graphics with its text override as value (`<>` for the measurement),
    /// and `options.measurement` the text of `<>`. The cursor is at position
    /// 0, nothing selected.
    explicit MTextEditModel(MText mtext, MTextEditOptions options = {});

    /// The value with its formatting codes.
    [[nodiscard]] const std::string& value() const { return m_value; }

    /// The MTEXT the editor draws: that of the constructor with the value,
    /// and, with a measurement, the first `<>` of the value replaced by it
    /// as dimensionText() does.
    [[nodiscard]] const MText& shown() const { return m_shown; }

    /// True when no unit is left (nothing shown), even with hidden codes in
    /// the value: `\A1;` and `{\L}` are empty (docs/adr/0054, point 5).
    [[nodiscard]] bool isEmpty() const;

    /// The units of the value in reading order.
    [[nodiscard]] std::vector<MTextEditUnit> units() const;

    /// The number of positions: at least 1.
    [[nodiscard]] std::size_t positionCount() const { return m_positions.size(); }

    /// Where `position` is drawn. Precondition: position < positionCount().
    [[nodiscard]] MTextCaret caretAt(std::size_t position) const;

    /// The position nearest to `point` in the drawing, measured to the caret
    /// of each one (its segment from the baseline up by its height); on a
    /// tie the one first in reading order.
    [[nodiscard]] std::size_t closestPosition(const Vec2& point) const;

    /// The number of rows of the layout of shown(): at least 1.
    [[nodiscard]] std::size_t rowCount() const;

    /// Where `Home` and `End` lead on `row`: the positions before the first
    /// unit drawn on it and after the last one; on a row without units, its
    /// only position. A row drawn only by a unit that takes more rows (a
    /// measurement with `\P`) leads to the positions before and after that
    /// unit. Precondition: row < rowCount().
    [[nodiscard]] std::size_t rowStart(std::size_t row) const;
    [[nodiscard]] std::size_t rowEnd(std::size_t row) const;

    /// The cursor, and the other end of the selection (equal to the cursor
    /// when nothing is selected).
    [[nodiscard]] std::size_t caret() const { return m_caret; }

    [[nodiscard]] std::size_t anchor() const { return m_anchor; }

    [[nodiscard]] bool hasSelection() const { return m_caret != m_anchor; }

    /// Moves the cursor to `position`, nothing selected. Precondition:
    /// position < positionCount().
    void setCaret(std::size_t position);

    /// Selects from `anchor` to `caret`, the cursor at `caret`. Precondition:
    /// both < positionCount().
    void select(std::size_t anchor, std::size_t caret);

    /// Writes `text` (UTF-8; '\n' starts a paragraph) in place of the
    /// selection, or at the cursor; the cursor after it.
    const std::string& insertText(std::string_view text);

    /// A new paragraph at the cursor, in place of the selection.
    const std::string& newParagraph();

    /// `Backspace`: the selection, or the unit before the cursor, or the
    /// `\P` before its paragraph; nothing at position 0.
    const std::string& deleteBackward();

    /// `Canc`: the selection, or the unit after the cursor, or the `\P`
    /// after its paragraph; nothing at the last position.
    const std::string& deleteForward();

    /// Applies or removes `change` on the units of the selection: each
    /// stretch of units without a code between them whose format changes is
    /// written `{<code>…}`; the codes already there are not touched. Without
    /// a selection the change applies to the next insertText() at the
    /// cursor, until the cursor moves. A height, width factor or tracking
    /// that is not a positive finite number, or an oblique that is not
    /// finite, changes nothing.
    ///
    /// Stack on applies to a selection of characters in one paragraph
    /// without a code between them, a stacked fraction or the measurement,
    /// with a '/', '#' or '^' and without ';': the first of them divides the
    /// two texts (a '^' is written "^ ", as AutoCAD saves it); then the
    /// fraction is selected. Stack off makes each stacked fraction of the
    /// selection its text, "top/bottom", "top#bottom" or "top^bottom", which
    /// is then selected. Otherwise nothing changes.
    const std::string& applyFormat(const MTextFormatChange& change);

    /// The format of what insertText() would write at the cursor: that of the
    /// unit before it, of the first unit at the start of a paragraph, of the
    /// codes before an empty paragraph, with the changes waiting for the next
    /// insertText().
    [[nodiscard]] MTextFormat formatAtCaret() const;

private:
    /// A point of a row of shown(): its index and an abscissa on its
    /// baseline, in the system of the layout (before the rotation).
    struct Location {
        std::size_t row{0};
        double x{0.0};
    };

    /// A unit with what the model needs of it.
    struct Unit {
        std::size_t begin{0}, end{0};  ///< its bytes in the value
        MTextEditUnit::Kind kind{MTextEditUnit::Kind::Character};
        char32_t character{0};  ///< a Character: as PT7 decodes it
        std::optional<MTextStack> stack;
        MTextFormat format;
        Location left, right;  ///< where it starts and ends in shown()
        /// The rows where it is drawn; none for a space where a row breaks.
        std::optional<std::pair<std::size_t, std::size_t>> rows;
    };

    /// A position: paragraph and index of the unit after it.
    struct Position {
        std::size_t paragraph{0};
        std::size_t index{0};
    };

    struct Splice;
    struct Rendered;

    /// Where the cursor goes after commit(): after the text of the first
    /// splice; on the text of all the splices, selected; or where it was.
    enum class CaretAfter : std::uint8_t { FirstSplice, SelectSplices, Same };

    void rebuild();
    void commit(std::vector<Splice> splices, CaretAfter caretAfter);
    [[nodiscard]] Rendered render(const std::vector<Splice>& splices, bool safe) const;
    [[nodiscard]] std::u32string expectedSignature(const std::vector<Splice>& splices) const;
    [[nodiscard]] std::size_t positionIndex(std::size_t paragraph, std::size_t index) const;
    [[nodiscard]] std::size_t insertionOffset(const Position& position) const;
    [[nodiscard]] std::vector<Splice> selectionRemovals() const;
    [[nodiscard]] MTextFormat formatAt(const Position& position) const;
    [[nodiscard]] std::size_t positionAtOffset(std::size_t offset) const;
    [[nodiscard]] Location locationOf(const Position& position) const;
    const std::string& applyStack(bool on);

    MText m_shown;
    MTextEditOptions m_options;
    std::string m_value;
    MTextFormat m_base;
    /// The units of each paragraph of the value, a measurement merged.
    std::vector<std::vector<Unit>> m_paragraphs;
    /// The bytes of each paragraph: from after its `\P` to the next `\P`.
    std::vector<std::pair<std::size_t, std::size_t>> m_paragraphRanges;
    std::vector<Position> m_positions;
    /// The rows of shown(): baseline start and nominal height.
    std::vector<std::pair<Vec2, double>> m_rows;
    /// Where each paragraph starts in shown(): for an empty one, or one
    /// whose first unit is not drawn.
    std::vector<Location> m_paragraphStarts;
    std::size_t m_caret{0};
    std::size_t m_anchor{0};
    std::vector<MTextFormatChange> m_pending;
};

}  // namespace tg::core
