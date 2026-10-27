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
#include <QStyle>
#include <QTemporaryDir>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

QApplication& application() {
    return *qobject_cast<QApplication*>(QApplication::instance());
}

TEST(ThemeTest, FusionPalettesAndSceneColoursMatchBothThemes) {
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        applyTheme(application(), theme);
        EXPECT_EQ(QApplication::style()->objectName(), QStringLiteral("fusion"));
        const QPalette palette = QApplication::palette();
        const bool dark = theme == Theme::Dark;
        EXPECT_EQ(palette.color(QPalette::Text), dark ? QColor(235, 235, 235) : QColor(24, 24, 24));
        EXPECT_EQ(palette.color(QPalette::Base), dark ? QColor(33, 40, 48) : QColor(255, 255, 255));
        const render::SceneStyle style = sceneStyle(theme);
        EXPECT_EQ(style.entity, (dark ? render::Color{235, 235, 235} : render::Color{0, 0, 0}));
        EXPECT_EQ(style.background,
                  (dark ? render::Color{33, 40, 48} : render::Color{255, 255, 255}));
        EXPECT_EQ(style.devicePixelRatio, 1.0);
    }
    applyTheme(application(), Theme::Light);
}

TEST(ThemeTest, SvgIsTintedWithPaletteTextAndKeepsTransparency) {
    const QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("test.svg"));
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    const QByteArray svg =
        R"(<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" viewBox="0 0 32 32"><rect x="8" y="8" width="16" height="16" fill="black"/></svg>)";
    ASSERT_EQ(file.write(svg), svg.size());
    file.close();
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        applyTheme(application(), theme);
        const QPalette palette = QApplication::palette();
        const QIcon icon = themedIcon(path, palette);
        ASSERT_FALSE(icon.isNull());
        const QImage image = icon.pixmap(32, 32).toImage();
        EXPECT_EQ(image.pixelColor(image.width() / 2, image.height() / 2),
                  palette.color(QPalette::Text));
        EXPECT_EQ(image.pixelColor(0, 0).alpha(), 0);
    }
    EXPECT_TRUE(themedIcon(QStringLiteral("missing-icon"), QPalette{}).isNull());
    EXPECT_TRUE(themedIcon(QString{}, QPalette{}).isNull());
    applyTheme(application(), Theme::Light);
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
