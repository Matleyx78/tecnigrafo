// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "document_session.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/render/scene_renderer.hpp"
#include "tg/render/viewport.hpp"

#include <QPointF>
#include <QWidget>

namespace tg::app {

class DynamicInputBox;

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

    DocumentSession* m_session{nullptr};
    DynamicInputBox* m_dynamicInput{nullptr};
    render::Viewport m_viewport;
    render::SceneStyle m_style;
    QPointF m_cursorPosition;
    QPointF m_lastPanPos;
    QPointF m_leftPressPos;
    bool m_panning{false};
    bool m_dragSelecting{false};
    bool m_initialFitDone{false};
};

}  // namespace tg::app
