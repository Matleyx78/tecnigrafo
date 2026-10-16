// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/render/scene_renderer.hpp"
#include "tg/render/viewport.hpp"

#include <QPointF>
#include <QWidget>

namespace tg::app {

/// Drawing area: shows a document and handles zoom and pan.
///
/// Mouse controls (as in AutoCAD):
///   wheel                 zoom towards the cursor
///   middle drag           pan
///   middle double-click   zoom extents
class CadView final : public QWidget {
    Q_OBJECT

public:
    explicit CadView(const core::Document& document, QWidget* parent = nullptr);

    [[nodiscard]] const render::Viewport& viewport() const { return m_viewport; }

    /// Fits the whole drawing in the view.
    void zoomExtents();

signals:
    /// Cursor position in drawing units.
    void cursorMoved(double worldX, double worldY);
    /// Pixels per drawing unit changed.
    void scaleChanged(double pixelsPerUnit);

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

    const core::Document& m_document;
    render::Viewport m_viewport;
    render::SceneStyle m_style;
    QPointF m_lastPanPos;
    bool m_panning{false};
    bool m_initialFitDone{false};
};

}  // namespace tg::app
