// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "cad_view.hpp"

#include "dynamic_input_box.hpp"
#include "qt_painter.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/math.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QWheelEvent>

#include <cassert>
#include <cmath>

namespace tg::app {

namespace {

// Zoom factor applied for one standard wheel notch (120 units).
constexpr double kZoomPerNotch = 1.2;
constexpr double kWheelNotch = 120.0;
// Minimum pointer movement to consider a mouse press and release a drag
// gesture (AutoCAD's PICKDRAG = 2, docs/tappe/0.7/P12-finestra-trascinamento.md).
constexpr double kDragThresholdPixels = 5.0;

core::Vec2 toVec2(const QPointF& point) {
    return {point.x(), point.y()};
}

// The pointer over the drawing: a thin cross whose arms stop at a square as
// large as the pick box (kPickBoxPixels of command_runner.cpp, 3 pixels on
// each side of the point), white on a dark background and black on a light one.
constexpr int kCursorArm = 30;
constexpr int kCursorBox = 3;
constexpr double kDarkLuminance = 128.0;

QCursor drawingCursor(const render::Color& background) {
    constexpr int kSide = (2 * kCursorArm) + 1;
    QPixmap pixmap(kSide, kSide);
    pixmap.fill(Qt::transparent);
    const double luminance =
        (0.299 * background.r) + (0.587 * background.g) + (0.114 * background.b);
    QPainter painter(&pixmap);
    painter.setPen(luminance < kDarkLuminance ? Qt::white : Qt::black);
    constexpr int kInner = kCursorArm - kCursorBox;
    constexpr int kOuter = kCursorArm + kCursorBox;
    painter.drawLine(0, kCursorArm, kInner - 1, kCursorArm);
    painter.drawLine(kOuter + 1, kCursorArm, kSide - 1, kCursorArm);
    painter.drawLine(kCursorArm, 0, kCursorArm, kInner - 1);
    painter.drawLine(kCursorArm, kOuter + 1, kCursorArm, kSide - 1);
    painter.drawRect(kInner, kInner, 2 * kCursorBox, 2 * kCursorBox);
    painter.end();
    return QCursor(pixmap, kCursorArm, kCursorArm);
}

}  // namespace

CadView::CadView(QWidget* parent) : QWidget(parent), m_dynamicInput(new DynamicInputBox(this)) {
    setMouseTracking(true);  // cursor coordinates without pressed buttons
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);  // we paint every pixel ourselves
    m_cursor = drawingCursor(m_style.background);
    setCursor(m_cursor);
    setMinimumSize(200, 150);
}

void CadView::setSession(DocumentSession* session) {
    if (session == m_session) {
        return;
    }
    // A view not yet fitted to its drawing has nothing worth keeping.
    if (m_session != nullptr && m_initialFitDone) {
        m_session->setViewState({m_viewport.center(), m_viewport.scale()});
    }
    m_session = session;
    // A gesture in progress belonged to the session left.
    if (m_panning) {
        m_panning = false;
        setCursor(m_cursor);
    }
    m_dragSelecting = false;
    m_dynamicInput->setSession(session);
    if (session == nullptr) {
        update();
        return;
    }
    if (const auto& state = session->viewState()) {
        // The runner of the session kept its own pixel size: it received
        // nothing while another session was shown.
        m_initialFitDone = true;
        m_viewport.setScale(state->scale);
        m_viewport.setCenter(state->center);
        emit scaleChanged(m_viewport.scale());
        emitCursor(m_cursorPosition);
        update();
        return;
    }
    // First shown: fitted to its drawing now, or when the view is shown.
    m_initialFitDone = false;
    if (isVisible()) {
        m_initialFitDone = true;
        m_viewport.setScreenSize(width(), height());
        zoomExtents();
        // The pointer stays, the drawing under it changes: its coordinates
        // are those of the new view.
        emitCursor(m_cursorPosition);
    }
    update();
}

void CadView::zoomExtents() {
    if (m_session == nullptr) {
        return;
    }
    m_viewport.zoomToBox(m_session->document().extents());
    m_session->runner().setPixelSize(1.0 / m_viewport.scale());
    emit scaleChanged(m_viewport.scale());
    update();
}

void CadView::setView(const core::Vec2& center, double unitsPerPixel) {
    assert(unitsPerPixel > 0.0 && "a view needs a positive scale");
    if (m_session == nullptr) {
        return;
    }
    m_initialFitDone = true;
    m_viewport.setScale(1.0 / unitsPerPixel);
    m_viewport.setCenter(center);
    // Not 1 / m_viewport.scale(): the round trip may change the last bit,
    // and the pick and snap tolerances in pixels must be those of level 1.
    m_session->runner().setPixelSize(unitsPerPixel);
    emit scaleChanged(m_viewport.scale());
    update();
}

void CadView::setStyle(const render::SceneStyle& style) {
    m_style = style;
    m_cursor = drawingCursor(m_style.background);
    if (!m_panning) {
        setCursor(m_cursor);
    }
    update();
}

void CadView::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    m_style.devicePixelRatio = devicePixelRatioF();
    QtPainter backend(painter);
    if (m_session == nullptr) {
        backend.clear(m_style.background);
        return;
    }
    const core::Document& document = m_session->document();
    const cmd::CommandRunner& runner = m_session->runner();
    render::renderScene(document, m_viewport, m_style, backend);
    render::renderHighlight(document, runner.selection().ids(), m_viewport, m_style, backend);
    render::renderPreview(runner.preview(), m_viewport, m_style, backend);
    if (const auto rectangle = runner.selectionRectangle()) {
        render::renderSelectionRectangle(rectangle->first, rectangle->second, rectangle->crossing,
                                         m_viewport, m_style, backend);
    }
    if (const auto polar = runner.activePolar()) {
        const core::Vec2 direction = polar->point - polar->base;
        const double distance = direction.length();
        // A zero-length ray has no visible direction or angle.
        if (distance > 0.0) {
            const core::Vec2 base = m_viewport.worldToScreen(polar->base);
            const core::Vec2 point = m_viewport.worldToScreen(polar->point);
            const core::Vec2 screenDirection = point - base;
            // Long enough to cover the visible part, even with an offscreen base.
            const double extent = std::hypot(width(), height()) +
                                  (base - core::Vec2{width() * 0.5, height() * 0.5}).length();
            const core::Vec2 end = base + screenDirection * (extent / screenDirection.length());
            backend.setPen(m_style.snapMarker, 0.0, render::LineStyle::Dashed);
            backend.drawLine(base, end);
            double angle = std::atan2(direction.y, direction.x);
            if (angle < 0.0) {
                angle += core::kTwoPi;
            }
            const QString text = tr("Polar: %1 < %2°")
                                     .arg(QString::fromStdString(cmd::formatLength(distance)))
                                     .arg(QString::fromStdString(cmd::formatAngle(angle)));
            constexpr double kHintOffsetPx = 14.0;
            painter.drawText(QPointF(point.x + kHintOffsetPx, point.y - kHintOffsetPx), text);
        }
    }
    render::renderGrips(runner.grips(), runner.hotGrip(), m_viewport, m_style, backend);
    // Above everything, so that the preview never hides it.
    if (const auto snap = runner.activeSnap()) {
        render::renderSnapMarker(*snap, m_viewport, m_style, backend);
    }
}

void CadView::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_viewport.setScreenSize(width(), height());
    emitCursor(m_cursorPosition);
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
    if (notches == 0.0 || m_session == nullptr) {
        event->ignore();
        return;
    }
    m_viewport.zoomAt(toVec2(event->position()), std::pow(kZoomPerNotch, notches));
    m_session->runner().setPixelSize(1.0 / m_viewport.scale());
    emit scaleChanged(m_viewport.scale());
    emitCursor(event->position());
    update();
    event->accept();
}

void CadView::mousePressEvent(QMouseEvent* event) {
    if (m_session == nullptr) {
        QWidget::mousePressEvent(event);
        return;
    }
    cmd::CommandRunner& runner = m_session->runner();
    if (event->button() == Qt::LeftButton) {
        const bool hadRectangle = runner.selectionRectangle().has_value();
        const cmd::PickAction action = event->modifiers().testFlag(Qt::ShiftModifier)
                                           ? cmd::PickAction::Remove
                                           : cmd::PickAction::Add;
        runner.click(toWorld(event->position()), action);
        m_leftPressPos = event->position();
        m_dragSelecting = !hadRectangle && runner.selectionRectangle().has_value();
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
    if (m_session != nullptr) {
        cmd::CommandRunner& runner = m_session->runner();
        runner.pointerMoved(toWorld(event->position()));
        if (runner.isActive() || runner.selectionRectangle()) {
            update();  // the preview or selection rectangle follows the pointer
        }
    }
    emitCursor(event->position());
    QWidget::mouseMoveEvent(event);
}

void CadView::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_dragSelecting && m_session != nullptr) {
        m_dragSelecting = false;
        const double dx = event->position().x() - m_leftPressPos.x();
        const double dy = event->position().y() - m_leftPressPos.y();
        const double distance = std::hypot(dx, dy);
        cmd::CommandRunner& runner = m_session->runner();
        if (distance > kDragThresholdPixels && runner.selectionRectangle()) {
            const cmd::PickAction action = event->modifiers().testFlag(Qt::ShiftModifier)
                                               ? cmd::PickAction::Remove
                                               : cmd::PickAction::Add;
            runner.click(toWorld(event->position()), action);
            update();
            emit inputHandled();
            event->accept();
            return;
        }
    }
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        setCursor(m_cursor);
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
    m_cursorPosition = screenPos;
    const core::Vec2 world = toWorld(screenPos);
    m_dynamicInput->updatePointer(world, screenPos.toPoint());
    emit cursorMoved(world.x, world.y);
}

core::Vec2 CadView::toWorld(const QPointF& screenPos) const {
    return m_viewport.screenToWorld(toVec2(screenPos));
}

void CadView::submit(const cmd::Input& input) {
    m_session->runner().submit(input);
    update();
    emit inputHandled();
}

}  // namespace tg::app
