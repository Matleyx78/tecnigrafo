// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "theme.hpp"

#include "style.hpp"

#include <QApplication>
#include <QColor>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>

#include <cstdint>

namespace tg::app {
namespace {

QColor rgb(QRgb value) {
    return QColor::fromRgb(value);
}

render::Color toRenderColor(const QColor& color) {
    return {static_cast<std::uint8_t>(color.red()), static_cast<std::uint8_t>(color.green()),
            static_cast<std::uint8_t>(color.blue()), static_cast<std::uint8_t>(color.alpha())};
}

QPalette paletteFrom(const ThemeTokens& tokens) {
    // QPalette derives the roles that the interface does not show (light,
    // midlight, dark, shadow, bright text) from button and window; every
    // role that the interface shows comes from a token.
    QPalette palette(tokens.panel, tokens.panel);
    palette.setColor(QPalette::Window, tokens.panel);
    palette.setColor(QPalette::WindowText, tokens.text);
    palette.setColor(QPalette::Base, tokens.field);
    palette.setColor(QPalette::AlternateBase, tokens.group);
    palette.setColor(QPalette::Text, tokens.text);
    palette.setColor(QPalette::Button, tokens.panel);
    palette.setColor(QPalette::ButtonText, tokens.text);
    palette.setColor(QPalette::ToolTipBase, tokens.field);
    palette.setColor(QPalette::ToolTipText, tokens.text);
    palette.setColor(QPalette::PlaceholderText, tokens.textSecondary);
    palette.setColor(QPalette::Accent, tokens.accent);
    // The selection is a light fill under the normal text (docs/adr/0039,
    // point 2), not white text on the accent colour.
    palette.setColor(QPalette::Highlight, tokens.selection);
    palette.setColor(QPalette::HighlightedText, tokens.text);
    palette.setColor(QPalette::Link, tokens.accent);
    palette.setColor(QPalette::LinkVisited, tokens.accent);
    palette.setColor(QPalette::Mid, tokens.border);
    for (const auto role : {QPalette::Text, QPalette::WindowText, QPalette::ButtonText}) {
        palette.setColor(QPalette::Disabled, role, tokens.textSecondary);
    }
    return palette;
}

}  // namespace

const ThemeTokens& themeTokens(Theme theme) {
    // The values of the tables of docs/adr/0039, point 2, and docs/adr/0040,
    // point 1; iconEsist is the text of the theme.
    static const ThemeTokens kLight{
        .chrome = rgb(0xdde1e6U),
        .ribbon = rgb(0xf4f5f7U),
        .group = rgb(0xe9ecefU),
        .tabs = rgb(0xe4e7ebU),
        .panel = rgb(0xf7f8faU),
        .field = rgb(0xffffffU),
        .canvas = rgb(0xffffffU),
        .border = rgb(0xcdd3daU),
        .text = rgb(0x1f2328U),
        .textSecondary = rgb(0x68707aU),
        .accent = rgb(0x1f6fbfU),
        .selection = rgb(0xdce8f4U),
        .iconGeo = rgb(0x1971c2U),
        .iconPt = rgb(0xe8590cU),
        .iconCost = rgb(0x868e96U),
        .iconEsist = rgb(0x1f2328U),
    };
    static const ThemeTokens kDark{
        .chrome = rgb(0x1e2228U),
        .ribbon = rgb(0x2b3038U),
        .group = rgb(0x262a31U),
        .tabs = rgb(0x23272dU),
        .panel = rgb(0x2b3038U),
        .field = rgb(0x20242aU),
        .canvas = rgb(0x1b1f24U),
        .border = rgb(0x3b424dU),
        .text = rgb(0xe4e7ebU),
        .textSecondary = rgb(0x9098a3U),
        .accent = rgb(0x2a72b5U),
        .selection = rgb(0x304357U),
        .iconGeo = rgb(0x4dabf7U),
        .iconPt = rgb(0xff922bU),
        .iconCost = rgb(0x8b95a1U),
        .iconEsist = rgb(0xe4e7ebU),
    };
    return theme == Theme::Dark ? kDark : kLight;
}

void applyTheme(QApplication& application, Theme theme) {
    const ThemeTokens& tokens = themeTokens(theme);
    // Every main window applies its theme when it is built. Installing the
    // same style, palette or style sheet again would repolish every widget
    // of every window, which makes building a window several times slower.
    const Style* current = installedStyle();
    // Decided before setStyle, which deletes the current style.
    const bool newStyle = current == nullptr || current->tokens() != tokens;
    if (newStyle) {
        // Fusion under the proxy style, never a native style or palette
        // which vary by OS (docs/adr/0009, point 5). The style comes
        // first: setting a style resets the palettes that it polishes. The
        // application owns the style, and QApplication deletes the
        // previous one.
        // clang-analyzer does not see either owner: Qt's headers are system
        // headers, whose functions it assumes never take a pointer, and the
        // parent is set in Style's constructor, in another file. QObject
        // ownership is the documented contract of QApplication::setStyle.
        // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
        QApplication::setStyle(new Style(tokens, &application));
    }
    const QPalette palette = paletteFrom(tokens);
    if (newStyle || QApplication::palette() != palette) {
        QApplication::setPalette(palette);
    }
    // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
    const QString sheet = styleSheet(tokens);
    if (application.styleSheet() != sheet) {
        application.setStyleSheet(sheet);
    }
}

render::SceneStyle sceneStyle(Theme theme) {
    // Grid, sizes and tolerances stay those of the program, and only the
    // background follows the tokens (docs/adr/0039, point 2).
    render::SceneStyle style;
    style.background = toRenderColor(themeTokens(theme).canvas);
    if (theme == Theme::Dark) {
        return style;
    }
    // Entities are black, as AutoCAD draws colour 7 on white; the other
    // colours are darker versions of the dark style's, readable on white.
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
