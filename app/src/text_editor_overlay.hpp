// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/input.hpp"
#include "tg/cmd/text_editing.hpp"
#include "tg/core/mtext_edit.hpp"
#include "tg/core/text.hpp"
#include "tg/core/vec2.hpp"
#include "tg/render/scene_renderer.hpp"
#include "tg/render/viewport.hpp"

#include <QWidget>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::core {
class Document;
}  // namespace tg::core

namespace tg::app {

class TextFormatBar;

/// The value of a TEXT edited in one line, without formats
/// (docs/tappe/0.8.1/PT12-editor.md, point 3). A TEXT has no MTEXT codes:
/// `\`, `{` and `}` are characters like the others. Its units are the
/// characters displayText() shows: a UTF-8 character, or one of %%c, %%d,
/// %%p and %%%, three bytes shown as one character. A line of n units has
/// n + 1 positions, drawn where the TEXT draws them: at the advance of the
/// characters before it from the start of the baseline of core::textFrame(),
/// which follows the value for the alignments Aligned and Fit. Without edits
/// value() is the value it was built with.
class TextLineEditModel {
public:
    /// The model of `text.value`, drawn as `text`. The cursor at the end.
    explicit TextLineEditModel(core::Text text);

    [[nodiscard]] const std::string& value() const { return m_text.value; }

    /// The TEXT drawn: that of the constructor with value().
    [[nodiscard]] const core::Text& shown() const { return m_text; }

    [[nodiscard]] bool isEmpty() const { return m_text.value.empty(); }

    /// The number of positions: at least 1.
    [[nodiscard]] std::size_t positionCount() const { return m_offsets.size(); }

    /// Where `position` is drawn: on the baseline, from there up by the used
    /// height along up(). Precondition: position < positionCount().
    [[nodiscard]] core::MTextCaret caretAt(std::size_t position) const;

    /// The y axis of the text (core::TextFrame::yAxis), oblique and mirrored
    /// with it.
    [[nodiscard]] core::Vec2 up() const;

    /// The position whose caret is nearest to `point`; on a tie the first.
    [[nodiscard]] std::size_t closestPosition(const core::Vec2& point) const;

    [[nodiscard]] std::size_t caret() const { return m_caret; }

    [[nodiscard]] std::size_t anchor() const { return m_anchor; }

    [[nodiscard]] bool hasSelection() const { return m_caret != m_anchor; }

    /// Preconditions: every position < positionCount().
    void setCaret(std::size_t position);
    void select(std::size_t anchor, std::size_t caret);

    /// Writes `text` (UTF-8) in place of the selection, or at the cursor,
    /// without its line breaks: a TEXT is one line. The cursor after it.
    void insertText(std::string_view text);

    /// `Backspace` and `Canc`: the selection, or the unit before or after
    /// the cursor.
    void deleteBackward();
    void deleteForward();

private:
    /// Replaces the bytes from position `from` to position `to` with
    /// `text`; the cursor goes to the first position at or after the end of
    /// what was written.
    void replace(std::size_t from, std::size_t to, std::string_view text);

    core::Text m_text;
    /// The byte offset of every position in the value.
    std::vector<std::size_t> m_offsets;
    std::size_t m_caret{0};
    std::size_t m_anchor{0};
};

/// The editor of a text in the drawing view (docs/adr/0054, point 5;
/// docs/tappe/0.8.1/PT12-editor.md). It lies over the whole view while it
/// is open and is hidden otherwise.
///
/// It opens on the cmd::TextEditRequest of the runner: on an MTEXT (shown
/// an MText: a new MTEXT, TEXTEDIT on an MTEXT or on a dimension, the Mtext
/// option of a dimension) it edits the value with core::MTextEditModel; on
/// a TEXT with TextLineEditModel, in one line. It draws the text as the
/// drawing lays it out, at the scale and rotation of the view and of the
/// text, over a patch of the background that hides the text being edited,
/// with the cursor, the selection and a border as wide as the column.
///
/// Keys: characters, arrows, `Home`/`End` (with `Ctrl` the ends of the
/// text), `Backspace`, `Canc`, `Shift` to select, `Ctrl+A` all; `Invio` a
/// new paragraph (on a TEXT it confirms), `Ctrl+Invio` confirms, `Esc`
/// cancels. The left button moves the cursor and, dragged or with `Shift`,
/// selects; a click outside the border confirms. The wheel and the middle
/// button reach the view below: zoom and pan while writing. The cursor is
/// at the end of the text when it opens, nothing selected.
///
/// It only draws and turns keys and clicks into calls of the model: the
/// view hands the runner what confirmed() and cancelled() mean.
class TextEditorOverlay final : public QWidget {
    Q_OBJECT

public:
    /// `parent` is the drawing view, which must be non-null and outlive the
    /// overlay; `viewport` is the view's, read at every paint and click.
    TextEditorOverlay(QWidget* parent, const render::Viewport& viewport);

    /// Opens on `request`, laid out with the text styles of `document`,
    /// which must outlive the editor or be followed by close(); an editor
    /// already open is dropped. Shows the overlay over its parent.
    void open(const cmd::TextEditRequest& request, const core::Document& document);

    /// Drops the editor without handing anything over, and hides it.
    void close();

    [[nodiscard]] bool isOpen() const { return m_request.has_value(); }

    /// The request the editor was opened on; nullopt when closed.
    [[nodiscard]] const std::optional<cmd::TextEditRequest>& request() const { return m_request; }

    /// The value being edited, with its codes; empty when closed.
    [[nodiscard]] std::string value() const;

    /// The model of an MTEXT or of the text of a dimension; nullptr on a
    /// TEXT and when closed.
    [[nodiscard]] const core::MTextEditModel* model() const;

    /// The model of a TEXT; nullptr otherwise.
    [[nodiscard]] const TextLineEditModel* lineModel() const;

    /// What confirming hands the runner now, in order
    /// (docs/tappe/0.8.1/PT12-editor.md, point 2):
    /// - the value as it was opened (nothing changed, or changed back):
    ///   ConfirmInput, which keeps it;
    /// - emptied, nothing left that shows (core::MTextEditModel::isEmpty(),
    ///   hidden codes included): on the text of a dimension TextInput{""},
    ///   back to the measurement, otherwise ConfirmInput, which leaves a
    ///   TEXT or an MTEXT as it was and creates no new MTEXT;
    /// - otherwise TextInput with the value on one line, codes kept, and for
    ///   a new MTEXT then ConfirmInput, the empty line that ends its lines.
    /// Nothing when closed.
    [[nodiscard]] std::vector<cmd::Input> confirmation() const;

    /// Applies or removes a format on the selection, or on what is written
    /// next (core::MTextEditModel::applyFormat()): the format bar of PT13.
    /// Nothing on a TEXT, which has no formats, or when closed.
    void applyFormat(const core::MTextFormatChange& change);

    /// Draws with the colours of `style`: the background of the view under
    /// the text, the main colour for the text and the cursor, the colour of
    /// the highlight for the selection and the border.
    void setStyle(const render::SceneStyle& style);

    /// Corners of the border in the drawing, counter-clockwise from the
    /// bottom left of the text when it is not mirrored; all at the origin
    /// when closed.
    [[nodiscard]] std::array<core::Vec2, 4> border() const;

signals:
    /// `Ctrl+Invio`, `Invio` on a TEXT, or a click outside the border: hand
    /// the runner confirmation(). The editor is still open.
    void confirmed();
    /// `Esc`: the runner gets a CancelInput. The editor is still open.
    void cancelled();
    /// The cursor or the selection moved, or the value changed: the format
    /// bar of PT13 follows the format under the cursor.
    void selectionChanged();

protected:
    bool event(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    /// A box in the system of the text: origin + x * xAxis + y * yAxis, for
    /// x in [left, right] and y in [bottom, top].
    struct Box {
        core::Vec2 origin;
        core::Vec2 xAxis{1.0, 0.0};
        core::Vec2 yAxis{0.0, 1.0};
        double left{0.0};
        double right{0.0};
        double bottom{0.0};
        double top{0.0};
    };

    [[nodiscard]] Box currentBox() const;
    /// The corners of `box`, grown by `margin` drawing units on every side.
    [[nodiscard]] static std::array<core::Vec2, 4> corners(const Box& box, double margin);
    [[nodiscard]] bool insideBorder(const core::Vec2& point) const;
    /// The margin of the border around the text, in drawing units at the
    /// scale of the view.
    [[nodiscard]] double margin() const;

    [[nodiscard]] std::size_t positionCount() const;
    [[nodiscard]] std::size_t caret() const;
    [[nodiscard]] std::size_t anchor() const;
    [[nodiscard]] core::MTextCaret caretAt(std::size_t position) const;
    /// The direction the cursor rises along, as a unit of the height.
    [[nodiscard]] core::Vec2 up() const;
    [[nodiscard]] std::size_t closestPosition(const core::Vec2& point) const;
    /// Moves the cursor to `position`, extending the selection with `extend`.
    void moveCaret(std::size_t position, bool extend);
    /// The position on the row above (-1) or below (+1) nearest to the
    /// cursor; the cursor itself on the first or last row.
    [[nodiscard]] std::size_t verticalPosition(int direction) const;
    void insertText(std::string_view text);
    void newParagraph();
    void deleteBackward();
    void deleteForward();
    /// After every change: repaint and tell the format bar.
    void changed();

    const render::Viewport& m_viewport;
    render::SceneStyle m_style;
    std::optional<cmd::TextEditRequest> m_request;
    const core::Document* m_document{nullptr};
    std::optional<core::MTextEditModel> m_mtext;
    std::optional<TextLineEditModel> m_line;
    /// The box of the text as it was opened: its patch of background hides
    /// the text being edited, which the view still draws below.
    Box m_openedBox;
    bool m_selecting{false};
    TextFormatBar* m_formatBar;
};

}  // namespace tg::app
