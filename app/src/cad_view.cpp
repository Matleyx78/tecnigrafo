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

CadView::CadView(const core::Document& document, cmd::CommandRunner& runner, QWidget* parent)
    : QWidget(parent), m_document(document), m_runner(runner) {
    setMouseTracking(true);  // cursor coordinates without pressed buttons
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);  // we paint every pixel ourselves
    setCursor(Qt::CrossCursor);
    setMinimumSize(200, 150);
}

void CadView::zoomExtents() {
    m_viewport.zoomToBox(m_document.extents());
    m_runner.setPixelSize(1.0 / m_viewport.scale());
    emit scaleChanged(m_viewport.scale());
    update();
}

void CadView::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    QtPainter backend(painter);
    render::renderScene(m_document, m_viewport, m_style, backend);
    render::renderHighlight(m_document, m_runner.selection().ids(), m_viewport, m_style, backend);
    render::renderPreview(m_runner.preview(), m_viewport, m_style, backend);
    if (const auto rectangle = m_runner.selectionRectangle()) {
        render::renderSelectionRectangle(rectangle->first, rectangle->second, rectangle->crossing,
                                         m_viewport, m_style, backend);
    }
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
    m_runner.setPixelSize(1.0 / m_viewport.scale());
    emit scaleChanged(m_viewport.scale());
    emitCursor(event->position());
    update();
    event->accept();
}

void CadView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        const cmd::PickAction action = event->modifiers().testFlag(Qt::ShiftModifier)
                                           ? cmd::PickAction::Remove
                                           : cmd::PickAction::Add;
        m_runner.click(toWorld(event->position()), action);
        update();
        emit inputHandled();
        event->accept();
        return;
    }
    if (event->button() == Qt::RightButton) {
        submit(cmd::ConfirmInput{});
        event->accept();
        return;
    }
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
    m_runner.pointerMoved(toWorld(event->position()));
    if (m_runner.isActive() || m_runner.selectionRectangle()) {
        update();  // the preview or selection rectangle follows the pointer
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
    const core::Vec2 world = toWorld(screenPos);
    emit cursorMoved(world.x, world.y);
}

core::Vec2 CadView::toWorld(const QPointF& screenPos) const {
    return m_viewport.screenToWorld(toVec2(screenPos));
}

void CadView::submit(const cmd::Input& input) {
    m_runner.submit(input);
    update();
    emit inputHandled();
}

}  // namespace tg::app
