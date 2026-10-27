// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "theme.hpp"

#include <QApplication>
#include <QColor>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QStyleFactory>

namespace tg::app {

void applyTheme(QApplication& /*application*/, Theme theme) {
    // Qt exposes these operations as static functions; the argument makes
    // the requirement for a live QApplication explicit.
    // Start from Fusion, never from a native palette which varies by OS.
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QPalette palette(QColor(240, 240, 240));
    const bool dark = theme == Theme::Dark;
    const QColor text = dark ? QColor(235, 235, 235) : QColor(24, 24, 24);
    const QColor window = dark ? QColor(45, 49, 55) : QColor(240, 240, 240);
    const QColor base = dark ? QColor(33, 40, 48) : QColor(255, 255, 255);
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, window);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, window);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::ToolTipBase, base);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::PlaceholderText, QColor(128, 128, 128));
    palette.setColor(QPalette::Accent, QColor(40, 110, 200));
    palette.setColor(QPalette::Highlight, QColor(40, 110, 200));
    palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    palette.setColor(QPalette::Link, dark ? QColor(100, 180, 255) : QColor(0, 80, 180));
    palette.setColor(QPalette::Light, dark ? QColor(75, 80, 88) : QColor(255, 255, 255));
    palette.setColor(QPalette::Midlight, dark ? QColor(60, 65, 72) : QColor(225, 225, 225));
    palette.setColor(QPalette::Mid, dark ? QColor(38, 42, 48) : QColor(180, 180, 180));
    palette.setColor(QPalette::Dark, dark ? QColor(24, 28, 33) : QColor(140, 140, 140));
    palette.setColor(QPalette::Shadow, QColor(0, 0, 0));
    for (const auto role : {QPalette::Text, QPalette::WindowText, QPalette::ButtonText}) {
        palette.setColor(QPalette::Disabled, role, QColor(128, 128, 128));
    }
    QApplication::setPalette(palette);
}

render::SceneStyle sceneStyle(Theme theme) {
    // Grid, sizes and tolerances stay those of the program: only colours
    // change. Entities are black, as AutoCAD draws colour 7 on white; the
    // other colours are darker versions of the dark style's, readable on
    // white.
    render::SceneStyle style;
    if (theme == Theme::Dark) {
        return style;
    }
    style.background = {255, 255, 255};
    style.gridMinor = {236, 238, 241};
    style.gridMajor = {214, 218, 224};
    style.axisX = {200, 40, 40};
    style.axisY = {30, 140, 50};
    style.entity = {0, 0, 0};
    style.preview = {190, 120, 0};
    style.highlight = {0, 100, 230};
    style.windowFill = {40, 90, 200, 40};
    style.windowBorder = {30, 90, 220};
    style.crossingFill = {40, 160, 70, 40};
    style.crossingBorder = {30, 140, 60};
    style.snapMarker = {220, 110, 0};
    return style;
}

QIcon themedIcon(const QString& name, const QPalette& palette) {
    if (name.isEmpty()) {
        return {};
    }
    const QString path = name.startsWith(QLatin1Char(':')) || QFileInfo(name).isAbsolute()
                             ? name
                             : QStringLiteral(":/tg/icons/%1.svg").arg(name);
    if (!QFileInfo::exists(path)) {
        return {};
    }
    const QIcon source(path);
    QIcon result;
    // Rasterize at the sizes used by menus, toolbars and ribbon, including
    // high-DPI variants. SourceIn preserves the SVG's alpha coverage.
    for (const int size : {16, 20, 24, 32, 40, 48, 64, 96, 128}) {
        QPixmap pixmap = source.pixmap(size, size);
        if (pixmap.isNull()) {
            continue;
        }
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), palette.color(QPalette::Text));
        painter.end();
        result.addPixmap(pixmap);
    }
    return result;
}

}  // namespace tg::app
