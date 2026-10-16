// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "qt_painter.hpp"

#include <QColor>
#include <QPainter>
#include <QPen>

namespace tg::app {

namespace {

QColor toQColor(const render::Color& color) {
    return {color.r, color.g, color.b, color.a};
}

}  // namespace

QtPainter::QtPainter(QPainter& painter) : m_painter(painter) {}

void QtPainter::clear(const render::Color& background) {
    m_painter.fillRect(m_painter.viewport(), toQColor(background));
}

void QtPainter::setPen(const render::Color& color, double widthPx) {
    QPen pen(toQColor(color));
    pen.setWidthF(widthPx);
    pen.setCosmetic(true);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
    m_painter.setPen(pen);
    m_painter.setBrush(Qt::NoBrush);
}

void QtPainter::drawLine(const core::Vec2& from, const core::Vec2& to) {
    m_painter.drawLine(QPointF(from.x, from.y), QPointF(to.x, to.y));
}

void QtPainter::drawPolyline(std::span<const core::Vec2> points, bool closed) {
    if (points.size() < 2) {
        return;
    }
    m_buffer.clear();
    m_buffer.reserve(static_cast<qsizetype>(points.size()));
    for (const core::Vec2& point : points) {
        m_buffer.append(QPointF(point.x, point.y));
    }
    // Geometry is antialiased; grid and axes (drawLine) stay crisp.
    m_painter.setRenderHint(QPainter::Antialiasing, true);
    if (closed) {
        m_painter.drawPolygon(m_buffer);
    } else {
        m_painter.drawPolyline(m_buffer);
    }
    m_painter.setRenderHint(QPainter::Antialiasing, false);
}

}  // namespace tg::app
