// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "cad_view.hpp"

#include "qt_painter.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <cmath>

namespace tg::app {

namespace {

// Zoom factor applied for one standard wheel notch (120 units).
constexpr double kZoomPerNotch = 1.2;
constexpr double kWheelNotch = 120.0;

core::Vec2 toVec2(const QPointF& point) {
    return {point.x(), point.y()};
}

}  // namespace

CadView::CadView(const core::Document& document, QWidget* parent)
    : QWidget(parent), m_document(document) {
    setMouseTracking(true);  // cursor coordinates without pressed buttons
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);  // we paint every pixel ourselves
    setCursor(Qt::CrossCursor);
    setMinimumSize(200, 150);
}

void CadView::zoomExtents() {
    m_viewport.zoomToBox(m_document.extents());
    emit scaleChanged(m_viewport.scale());
    update();
}

void CadView::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    QtPainter backend(painter);
    render::renderScene(m_document, m_viewport, m_style, backend);
}

void CadView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_viewport.setScreenSize(width(), height());
}

void CadView::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!m_initialFitDone) {
        m_initialFitDone = true;
        m_viewport.setScreenSize(width(), height());
        zoomExtents();
    }
}

void CadView::wheelEvent(QWheelEvent* event) {
    const double notches = event->angleDelta().y() / kWheelNotch;
    if (notches == 0.0) {
        event->ignore();
        return;
    }
    m_viewport.zoomAt(toVec2(event->position()), std::pow(kZoomPerNotch, notches));
    emit scaleChanged(m_viewport.scale());
    emitCursor(event->position());
    update();
    event->accept();
}

void CadView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPos = event->position();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void CadView::mouseMoveEvent(QMouseEvent* event) {
    if (m_panning) {
        const QPointF delta = event->position() - m_lastPanPos;
        m_lastPanPos = event->position();
        m_viewport.panByPixels(toVec2(delta));
        update();
    }
    emitCursor(event->position());
    QWidget::mouseMoveEvent(event);
}

void CadView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        setCursor(Qt::CrossCursor);
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void CadView::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        zoomExtents();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}

void CadView::emitCursor(const QPointF& screenPos) {
    const core::Vec2 world = m_viewport.screenToWorld(toVec2(screenPos));
    emit cursorMoved(world.x, world.y);
}

}  // namespace tg::app
