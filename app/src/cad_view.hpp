// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "document_session.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/render/scene_renderer.hpp"
#include "tg/render/viewport.hpp"

#include <QCursor>
#include <QPointF>
#include <QTimer>
#include <QWidget>

namespace tg::app {

class DynamicInputBox;
class TextEditorOverlay;

/// Drawing area: shows a document, its selection, the command preview and
/// the marker of the object snap under the pointer, and handles zoom and
/// pan. Snap and ortho correct the clicks and the preview in the
/// CommandRunner (docs/adr/0008); the view only draws what it is told.
///
/// Mouse controls (as in AutoCAD):
///   left click            selection gesture or point for the running command
///   Shift + left click    remove from the selection
///   right click           Enter: confirm, or repeat the last command
///   wheel                 zoom towards the cursor
///   middle drag           pan
///   middle double-click   zoom extents
///   left double-click     on a TEXT, an MTEXT, a dimension, an ATTDEF or an
///                         INSERT with attributes, at "Command:": TEXTEDIT
///                         on it (docs/tappe/0.8.1/PT12-editor.md, point 4)
///
/// The editor of a text (TextEditorOverlay, docs/adr/0054, point 5) lies
/// over the view while the runner gives a cmd::TextEditRequest, unless
/// setTextEditorEnabled(false): it takes the keys, and its confirmation or
/// Esc go to the runner. Meanwhile the pointer is not handed to the runner,
/// so the request stays as the editor opened it (the Mtext option of a
/// dimension follows the pointer), while zoom and pan go on.
///
/// The view shows the session of setSession(), the active one
/// (docs/adr/0041, point 2). Without a session it paints only the
/// background and takes no input.
class CadView final : public QWidget {
    Q_OBJECT

public:
    explicit CadView(QWidget* parent = nullptr);

    /// Shows `session` from now on; nullptr for none. The view the session
    /// left keeps its centre and scale in it (DocumentSession::viewState),
    /// and `session` gets its own back; a session never shown is fitted to
    /// its drawing. The session is borrowed: the owner calls setSession()
    /// again, with another session or nullptr, before destroying it.
    void setSession(DocumentSession* session);

    [[nodiscard]] DocumentSession* session() const { return m_session; }

    [[nodiscard]] DynamicInputBox* dynamicInputBox() const { return m_dynamicInput; }

    [[nodiscard]] TextEditorOverlay* textEditor() const { return m_textEditor; }

    /// Opens the editor on the runner's cmd::TextEditRequest, keeps it open
    /// while the request is the same, closes it (dropping what was typed)
    /// when the request changes or ends, and opens it again on a new one.
    /// The view calls it after its own inputs; the owner after the others
    /// (the command line). Without a session the editor is closed.
    void refreshTextEditor();

    /// Off, the editor never opens and the Text prompts stay on the command
    /// line: the form of the scripts (docs/adr/0054, point 5). On at first.
    void setTextEditorEnabled(bool enabled);

    [[nodiscard]] const render::Viewport& viewport() const { return m_viewport; }

    /// Fits the whole drawing in the view. Nothing without a session.
    void zoomExtents();

    /// Shows `center` in the middle of the view, at `unitsPerPixel` drawing
    /// units per pixel: the "!view" of the control mode (docs/adr/0011).
    /// The runner gets exactly `unitsPerPixel` as its pixel size, as at
    /// level 1, and the view is no longer fitted to the drawing when it is
    /// first shown. Nothing without a session. Precondition:
    /// unitsPerPixel > 0.
    void setView(const core::Vec2& center, double unitsPerPixel);

    /// Draws with `style` from now on: the colours of grid, entities,
    /// preview, selection and snap marker.
    void setStyle(const render::SceneStyle& style);

signals:
    /// Cursor position in drawing units.
    void cursorMoved(double worldX, double worldY);
    /// Pixels per drawing unit changed.
    void scaleChanged(double pixelsPerUnit);
    /// A click was sent to the runner: the drawing, the prompt and what can
    /// be undone may have changed.
    void inputHandled();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void emitCursor(const QPointF& screenPos);
    [[nodiscard]] core::Vec2 toWorld(const QPointF& screenPos) const;
    void submit(const cmd::Input& input);
    void closeTextEditor();
    /// The editor's confirmation, or Esc, handed to the runner.
    void confirmText();
    void cancelText();
    /// The left double click at `screenPos`: TEXTEDIT on the text there.
    /// False, and nothing done, when the runner is not at "Command:" or no
    /// text that TEXTEDIT edits is there.
    [[nodiscard]] bool editTextAt(const QPointF& screenPos);

    DocumentSession* m_session{nullptr};
    DynamicInputBox* m_dynamicInput{nullptr};
    render::Viewport m_viewport;
    render::SceneStyle m_style;
    QPointF m_cursorPosition;
    QPointF m_lastPanPos;
    QPointF m_leftPressPos;
    /// The pointer over the drawing, coloured against its background.
    QCursor m_cursor;
    bool m_panning{false};
    bool m_dragSelecting{false};
    bool m_initialFitDone{false};
    bool m_textEditorEnabled{true};
    /// After the viewport, which it keeps a reference to.
    TextEditorOverlay* m_textEditor{nullptr};
    QTimer* m_holdTimer{nullptr};
};

}  // namespace tg::app
