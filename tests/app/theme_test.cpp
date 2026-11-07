// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "theme.hpp"

#include "action_registry.hpp"
#include "cad_view.hpp"
#include "main_window.hpp"

#include <QAction>
#include <QApplication>
#include <QFile>
#include <QImage>
#include <QPalette>
#include <QTemporaryDir>
#include <gtest/gtest.h>

#include <cstdint>

namespace tg::app {
namespace {

QApplication& application() {
    return *qobject_cast<QApplication*>(QApplication::instance());
}

render::Color renderColor(const QColor& color) {
    return {static_cast<std::uint8_t>(color.red()), static_cast<std::uint8_t>(color.green()),
            static_cast<std::uint8_t>(color.blue())};
}

TEST(ThemeTest, TokensAreTheValuesOfTheDecision) {
    // docs/adr/0039, point 2, and docs/adr/0040, point 1.
    const ThemeTokens& light = themeTokens(Theme::Light);
    const ThemeTokens& dark = themeTokens(Theme::Dark);
    EXPECT_EQ(light.chrome, QColor(0xdd, 0xe1, 0xe6));
    EXPECT_EQ(dark.chrome, QColor(0x1e, 0x22, 0x28));
    EXPECT_EQ(light.canvas, QColor(0xff, 0xff, 0xff));
    EXPECT_EQ(dark.canvas, QColor(0x1b, 0x1f, 0x24));
    EXPECT_EQ(light.text, QColor(0x1f, 0x23, 0x28));
    EXPECT_EQ(dark.text, QColor(0xe4, 0xe7, 0xeb));
    EXPECT_EQ(light.accent, QColor(0x1f, 0x6f, 0xbf));
    EXPECT_EQ(dark.accent, QColor(0x2a, 0x72, 0xb5));
    EXPECT_EQ(light.selection, QColor(0xdc, 0xe8, 0xf4));
    EXPECT_EQ(dark.selection, QColor(0x30, 0x43, 0x57));
    EXPECT_EQ(light.iconGeo, QColor(0x19, 0x71, 0xc2));
    EXPECT_EQ(dark.iconPt, QColor(0xff, 0x92, 0x2b));
    for (const ThemeTokens* tokens : {&light, &dark}) {
        EXPECT_EQ(tokens->iconEsist, tokens->text);
    }
}

TEST(ThemeTest, PalettesAndSceneBackgroundComeFromTheTokens) {
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        applyTheme(application(), theme);
        const ThemeTokens& tokens = themeTokens(theme);
        const QPalette palette = QApplication::palette();
        EXPECT_EQ(palette.color(QPalette::Text), tokens.text);
        EXPECT_EQ(palette.color(QPalette::WindowText), tokens.text);
        EXPECT_EQ(palette.color(QPalette::ButtonText), tokens.text);
        EXPECT_EQ(palette.color(QPalette::Window), tokens.panel);
        EXPECT_EQ(palette.color(QPalette::Base), tokens.field);
        EXPECT_EQ(palette.color(QPalette::Highlight), tokens.selection);
        EXPECT_EQ(palette.color(QPalette::HighlightedText), tokens.text);
        EXPECT_EQ(palette.color(QPalette::Accent), tokens.accent);
        const render::SceneStyle style = sceneStyle(theme);
        EXPECT_EQ(style.background, renderColor(tokens.canvas));
        // The other colours of the drawing do not follow the tokens.
        const bool dark = theme == Theme::Dark;
        EXPECT_EQ(style.entity, (dark ? render::Color{235, 235, 235} : render::Color{0, 0, 0}));
        EXPECT_EQ(style.devicePixelRatio, 1.0);
    }
    applyTheme(application(), Theme::Light);
}

TEST(ThemeTest, IconWithoutRolesIsDrawnInTheTextColourAndKeepsTransparency) {
    const QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("test.svg"));
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    // An element without a role class is esist, the text of the theme.
    const QByteArray svg =
        R"(<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32"><rect x="8" y="8" width="16" height="16" fill="currentColor"/></svg>)";
    ASSERT_EQ(file.write(svg), svg.size());
    file.close();
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        const QIcon icon = themedIcon(path, theme);
        ASSERT_FALSE(icon.isNull());
        const QImage image = icon.pixmap(32, 32).toImage();
        EXPECT_EQ(image.pixelColor(image.width() / 2, image.height() / 2), themeTokens(theme).text);
        EXPECT_EQ(image.pixelColor(0, 0).alpha(), 0);
    }
    EXPECT_TRUE(themedIcon(QStringLiteral("missing-icon"), Theme::Light).isNull());
    EXPECT_TRUE(themedIcon(QString{}, Theme::Dark).isNull());
}

TEST(ThemeTest, ThemeOfAPaletteIsTheThemeThatBuiltIt) {
    for (const Theme theme : {Theme::Dark, Theme::Light}) {
        applyTheme(application(), theme);
        EXPECT_EQ(themeOf(QApplication::palette()), theme);
    }
    EXPECT_EQ(themeOf(QPalette{}), Theme::Light);
}

TEST(ThemeTest, ActionImmediatelyChangesPaletteAndDrawing) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.setScriptMode();
    window.show();
    auto* action = window.actionRegistry().action(QStringLiteral("ui.theme"));
    ASSERT_NE(action, nullptr);
    for (const Theme theme : {Theme::Dark, Theme::Light}) {
        action->trigger();
        EXPECT_EQ(window.theme(), theme);
        const auto background = sceneStyle(theme).background;
        const QImage image = window.view()->grab().toImage();
        int backgroundPixels = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y) == QColor(background.r, background.g, background.b)) {
                    ++backgroundPixels;
                }
            }
        }
        // Grid lines can cross any particular pixel as the window resizes.
        EXPECT_GT(backgroundPixels, image.width() * image.height() / 2);
        EXPECT_EQ(window.palette().color(QPalette::Text),
                  QApplication::palette().color(QPalette::Text));
    }
}

}  // namespace
}  // namespace tg::app
