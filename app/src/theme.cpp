// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "theme.hpp"

#include "style.hpp"

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QStringList>
#include <QSvgRenderer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

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

// Icons drawn at up to this many pixels use the 16 px grid, the larger ones
// the 32 px grid (docs/adr/0040, point 3).
constexpr int kSmallGridLimit = 24;

/// The colour of the role named in the class attribute `classes`, or null:
/// geo, pt, cost, esist, and the old accent, which is pt (docs/ICONE.md).
const QColor* roleColor(const QString& classes, const ThemeTokens& tokens) {
    for (const QString& token : classes.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts)) {
        if (token == QStringLiteral("geo")) {
            return &tokens.iconGeo;
        }
        if (token == QStringLiteral("pt") || token == QStringLiteral("accent")) {
            return &tokens.iconPt;
        }
        if (token == QStringLiteral("cost")) {
            return &tokens.iconCost;
        }
        if (token == QStringLiteral("esist")) {
            return &tokens.iconEsist;
        }
    }
    return nullptr;
}

/// What an element passes to its children: its role and its fill and stroke
/// as written (empty when neither it nor an ancestor sets them).
struct Inherited {
    QColor role;
    QString fill;
    QString stroke;
};

/// The SVG `data` with every fill and stroke that is currentColor replaced by
/// the colour of the element's role (docs/ICONE.md, "Come si colorano").
/// Each element gets its resolved fill and stroke written on itself, so a
/// child of another role than its group never inherits the group's colour.
/// Empty if `data` is not well-formed.
QByteArray colouredSvg(const QByteArray& data, const ThemeTokens& tokens) {
    const QString currentColor = QStringLiteral("currentColor");
    const QString fillKey = QStringLiteral("fill");
    const QString strokeKey = QStringLiteral("stroke");
    QXmlStreamReader reader(data);
    // The namespace declarations stay ordinary attributes, copied as written.
    reader.setNamespaceProcessing(false);
    QByteArray result;
    QXmlStreamWriter writer(&result);
    std::vector<Inherited> stack;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            Inherited state = stack.empty() ? Inherited{tokens.iconEsist, {}, {}} : stack.back();
            const QXmlStreamAttributes attributes = reader.attributes();
            if (const QColor* role =
                    roleColor(attributes.value(QStringLiteral("class")).toString(), tokens)) {
                state.role = *role;
            }
            if (attributes.hasAttribute(fillKey)) {
                state.fill = attributes.value(fillKey).toString();
            }
            if (attributes.hasAttribute(strokeKey)) {
                state.stroke = attributes.value(strokeKey).toString();
            }
            writer.writeStartElement(reader.qualifiedName().toString());
            for (const QXmlStreamAttribute& attribute : attributes) {
                const QString key = attribute.qualifiedName().toString();
                if (key != fillKey && key != strokeKey) {
                    writer.writeAttribute(key, attribute.value().toString());
                }
            }
            const QString colour = state.role.name(QColor::HexRgb);
            if (!state.fill.isEmpty()) {
                writer.writeAttribute(fillKey, state.fill == currentColor ? colour : state.fill);
            }
            if (!state.stroke.isEmpty()) {
                writer.writeAttribute(strokeKey,
                                      state.stroke == currentColor ? colour : state.stroke);
            }
            stack.push_back(state);
        } else if (reader.isEndElement()) {
            writer.writeEndElement();
            stack.pop_back();
        } else if (reader.isCharacters() && !reader.isWhitespace()) {
            writer.writeCharacters(reader.text().toString());
        }
    }
    if (reader.hasError()) {
        return {};
    }
    return result;
}

QString existingFile(const QString& path) {
    return QFileInfo::exists(path) ? path : QString{};
}

/// The files of an icon: `forSmall` draws the sizes up to kSmallGridLimit,
/// `forLarge` the others; both empty when the icon has no file.
struct IconFiles {
    QString forSmall;
    QString forLarge;
};

using IconKey = std::tuple<QString, QString, Theme>;
using IconCache = std::map<IconKey, QIcon>;

IconCache& iconCache() {
    // Only the UI thread calls themedIcon, so the process-wide cache needs no lock.
    static IconCache cache;
    return cache;
}

IconFiles iconFiles(const QString& name) {
    if (name.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)) {
        const QString file = existingFile(name);
        return {file, file};
    }
    const QString base = name.startsWith(QLatin1Char(':')) || QFileInfo(name).isAbsolute()
                             ? name
                             : QStringLiteral(":/tg/icons/%1").arg(name);
    const QString sixteen = existingFile(base + QStringLiteral("-16.svg"));
    const QString thirtyTwo = existingFile(base + QStringLiteral("-32.svg"));
    return {sixteen.isEmpty() ? thirtyTwo : sixteen, thirtyTwo.isEmpty() ? sixteen : thirtyTwo};
}

QByteArray readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

/// The icon drawn by `renderer` on a transparent square of `size` pixels.
QPixmap drawnIcon(QSvgRenderer& renderer, int size) {
    QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    renderer.render(&painter, QRectF(0, 0, size, size));
    painter.end();
    return QPixmap::fromImage(image);
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

Theme themeOf(const QPalette& palette) {
    return palette.color(QPalette::Window).lightnessF() <
                   palette.color(QPalette::WindowText).lightnessF()
               ? Theme::Dark
               : Theme::Light;
}

QIcon themedIcon(const QString& name, Theme theme) {
    if (name.isEmpty()) {
        return {};
    }
    const IconFiles files = iconFiles(name);
    if (files.forSmall.isEmpty()) {
        return {};
    }
    const bool cacheable = files.forSmall.startsWith(QStringLiteral(":/")) &&
                           files.forLarge.startsWith(QStringLiteral(":/"));
    const IconKey key{files.forSmall, files.forLarge, theme};
    if (cacheable) {
        const auto found = iconCache().find(key);
        if (found != iconCache().end()) {
            return found->second;
        }
    }
    const ThemeTokens& tokens = themeTokens(theme);
    QSvgRenderer smallSizes(colouredSvg(readFile(files.forSmall), tokens));
    QSvgRenderer largeSizes(colouredSvg(readFile(files.forLarge), tokens));
    if (!smallSizes.isValid() || !largeSizes.isValid()) {
        return {};
    }
    QIcon result;
    // The sizes of menus, toolbars and ribbon, with their high-DPI variants.
    for (const int size : {16, 20, 24, 32, 40, 48, 64, 96, 128}) {
        result.addPixmap(drawnIcon(size <= kSmallGridLimit ? smallSizes : largeSizes, size));
    }
    if (cacheable && !result.isNull()) {
        iconCache().emplace(key, result);
    }
    return result;
}

}  // namespace tg::app
