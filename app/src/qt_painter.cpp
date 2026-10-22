// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "qt_painter.hpp"

#include "tg/core/text_metrics.hpp"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QGlyphRun>
#include <QPainter>
#include <QPen>
#include <QRawFont>
#include <QResource>

#include <cmath>
#include <numbers>

// Q_INIT_RESOURCE must resolve its generated symbol in the global namespace.
// Explicitly reference the resource object so static linking retains it.
void initializeFontResources() {
    Q_INIT_RESOURCE(fonts);
}

namespace tg::app {

namespace {

const QRawFont& bundledFont() {
    static const QRawFont kFont = [] {
        initializeFontResources();
        const int id = QFontDatabase::addApplicationFont(
            QStringLiteral(":/tg/fonts/LiberationSans-Regular.ttf"));
        if (id < 0) {
            qFatal("Cannot load the bundled font");
        }
        QFont font(QFontDatabase::applicationFontFamilies(id).first());
        // A fixed, large size keeps the outline metrics independent of zoom.
        constexpr int kReferencePixelSize = 1024;
        font.setPixelSize(kReferencePixelSize);
        font.setKerning(false);
        font.setHintingPreference(QFont::PreferNoHinting);
        font.setStyleStrategy(QFont::NoFontMerging);
        const QRawFont raw = QRawFont::fromFont(font);
        if (!raw.isValid() || raw.capHeight() <= 0.0) {
            qFatal("Cannot obtain the bundled font outlines");
        }
        return raw;
    }();
    return kFont;
}

QColor toQColor(const render::Color& color) {
    return {color.r, color.g, color.b, color.a};
}

}  // namespace

QtPainter::QtPainter(QPainter& painter) : m_painter(painter) {}

void QtPainter::clear(const render::Color& background) {
    m_painter.fillRect(m_painter.viewport(), toQColor(background));
}

void QtPainter::setPen(const render::Color& color, double widthPx, render::LineStyle style) {
    QPen pen(toQColor(color));
    pen.setWidthF(widthPx);
    pen.setStyle(style == render::LineStyle::Dashed ? Qt::DashLine : Qt::SolidLine);
    pen.setCosmetic(true);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::MiterJoin);
    m_painter.setPen(pen);
    m_painter.setBrush(Qt::NoBrush);
}

void QtPainter::fillPolygon(std::span<const core::Vec2> points, const render::Color& color) {
    if (points.size() < 3) {
        return;
    }
    m_buffer.clear();
    m_buffer.reserve(static_cast<qsizetype>(points.size()));
    for (const core::Vec2& point : points) {
        m_buffer.append(QPointF(point.x, point.y));
    }
    // Filling must not change the pen/brush used by subsequent drawing calls.
    m_painter.save();
    m_painter.setPen(Qt::NoPen);
    m_painter.setBrush(QBrush(toQColor(color), Qt::SolidPattern));
    m_painter.setRenderHint(QPainter::Antialiasing, true);
    m_painter.drawPolygon(m_buffer);
    m_painter.restore();
}

void QtPainter::drawText(const render::TextRun& run) {
    if (run.text.empty() || !std::isfinite(run.capHeightPx) || run.capHeightPx <= 0.0 ||
        !std::isfinite(run.widthFactor) || run.widthFactor <= 0.0 || !std::isfinite(run.angle) ||
        !std::isfinite(run.oblique) || !std::isfinite(run.origin.x) ||
        !std::isfinite(run.origin.y)) {
        return;
    }
    m_painter.save();
    m_painter.translate(run.origin.x, run.origin.y);
    // QPainter's positive rotation is clockwise because screen Y points down.
    m_painter.rotate(-run.angle * 180.0 / std::numbers::pi);
    m_painter.scale(run.backward ? -1.0 : 1.0, run.upsideDown ? -1.0 : 1.0);
    // In text coordinates Y points up; screen Y reverses the shear sign.
    // Apply shear before width scaling so it depends on height alone.
    m_painter.shear(-std::tan(run.oblique), 0.0);
    constexpr double kOutlineOnlyHeightPx = 2.0;
    if (run.capHeightPx < kOutlineOnlyHeightPx) {
        const auto& metrics = core::fontMetrics();
        m_painter.setBrush(Qt::NoBrush);
        m_painter.drawRect(QRectF(0.0, -metrics.ascent * run.capHeightPx,
                                  core::textWidth(run.text) * run.capHeightPx * run.widthFactor,
                                  (metrics.ascent - metrics.descent) * run.capHeightPx));
    } else {
        const auto& font = bundledFont();
        QList<quint32> indexes;
        QList<QPointF> positions;
        double advance = 0.0;
        for (const char32_t code : run.text) {
            const char32_t character = core::fontCharacter(code);
            const auto glyphs = font.glyphIndexesForString(QString::fromUcs4(&character, 1));
            // A raw glyph run never asks Qt for a system fallback or shaping.
            indexes.append(glyphs.isEmpty() ? 0U : glyphs.first());
            positions.append(QPointF(advance * font.capHeight(), 0.0));
            advance += core::advanceWidth(code);
        }
        QGlyphRun glyphRun;
        glyphRun.setRawFont(font);
        glyphRun.setGlyphIndexes(indexes);
        glyphRun.setPositions(positions);
        const double scale = run.capHeightPx / font.capHeight();
        m_painter.scale(scale * run.widthFactor, scale);
        m_painter.setRenderHint(QPainter::TextAntialiasing, true);
        m_painter.drawGlyphRun(QPointF(0.0, 0.0), glyphRun);
    }
    m_painter.restore();
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
