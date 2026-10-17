// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/render/scene_renderer.hpp"
#include "tg/render/viewport.hpp"

#include <QPointF>
#include <QWidget>

namespace tg::app {

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
class CadView final : public QWidget {
    Q_OBJECT

public:
    /// The document and the runner must outlive the view.
    CadView(const core::Document& document, cmd::CommandRunner& runner, QWidget* parent = nullptr);

    [[nodiscard]] const render::Viewport& viewport() const { return m_viewport; }

    /// Fits the whole drawing in the view.
    void zoomExtents();

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

    const core::Document& m_document;
    cmd::CommandRunner& m_runner;
    render::Viewport m_viewport;
    render::SceneStyle m_style;
    QPointF m_lastPanPos;
    bool m_panning{false};
    bool m_initialFitDone{false};
};

}  // namespace tg::app
