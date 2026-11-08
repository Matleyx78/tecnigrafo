// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "action_registry.hpp"
#include "input_events.hpp"
#include "main_window.hpp"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QList>
#include <QMenu>
#include <QRect>
#include <QSignalSpy>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QToolButton>
#include <gtest/gtest.h>

#include <SARibbon.h>

namespace tg::app {
namespace {

TEST(RibbonSmoke, VisibleButtonTriggersItsAction) {
    SARibbonMainWindow window;
    auto* ribbon = window.ribbonBar();
    ASSERT_NE(ribbon, nullptr);
    auto* category = ribbon->addCategoryPage(QStringLiteral("Drawing"));
    ASSERT_NE(category, nullptr);
    auto* panel = category->addPanel(QStringLiteral("Draw"));
    ASSERT_NE(panel, nullptr);
    QAction action(QStringLiteral("Line"), &window);
    const QSignalSpy triggered(&action, &QAction::triggered);
    ASSERT_TRUE(triggered.isValid());
    panel->addLargeAction(&action);

    window.show();
    QApplication::processEvents();
    ASSERT_TRUE(window.isVisible());
    QToolButton* actionButton = nullptr;
    for (auto* button : panel->findChildren<QToolButton*>()) {
        if (button->defaultAction() == &action) {
            actionButton = button;
            break;
        }
    }
    ASSERT_NE(actionButton, nullptr);
    ASSERT_TRUE(actionButton->isVisible());
    ASSERT_TRUE(actionButton->isEnabled());
    EXPECT_EQ(triggered.count(), 0);
    events::click(actionButton, Qt::LeftButton, actionButton->rect().center());
    EXPECT_EQ(triggered.count(), 1);
}

/// The quick access bar of docs/adr/0039, point 1, area 1, in its order.
QList<QAction*> quickAccessActions(const ActionRegistry& registry) {
    return {
        registry.action(QStringLiteral("file.new")), registry.action(QStringLiteral("file.open")),
        registry.action(QStringLiteral("file.save")), registry.action(QStringLiteral("edit.undo")),
        registry.action(QStringLiteral("edit.redo"))};
}

TEST(RibbonSmoke, TheCompactRibbonHasAFileTabThatOpensTheApplicationMenu) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    QCoreApplication::processEvents();
    SARibbonBar* ribbon = window.ribbonBar();
    ASSERT_NE(ribbon, nullptr);
    EXPECT_TRUE(SARibbonBar::isCompactStyle(ribbon->currentRibbonStyle()));
    auto* file = qobject_cast<QToolButton*>(ribbon->applicationButton());
    ASSERT_NE(file, nullptr);
    EXPECT_TRUE(file->isVisible());
    EXPECT_EQ(file->text(), QStringLiteral("File"));
    QMenu* menu = file->menu();
    ASSERT_NE(menu, nullptr);
    EXPECT_EQ(menu->objectName(), QStringLiteral("applicationMenu"));
    const ActionRegistry& registry = window.actionRegistry();
    for (const char* id :
         {"file.new", "file.open", "file.save", "file.saveas", "file.recent", "file.quit"}) {
        EXPECT_TRUE(menu->actions().contains(registry.action(QString::fromLatin1(id)))) << id;
    }
    // A click opens the menu, which runs its own event loop until closed.
    bool shown = false;
    QTimer::singleShot(0, menu, [menu, &shown] {
        shown = menu->isVisible();
        menu->close();
    });
    events::click(file, Qt::LeftButton, file->rect().center());
    EXPECT_TRUE(shown);
}

TEST(RibbonSmoke, TheQuickAccessBarHasItsFiveActionsInOrderAsIcons) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    QCoreApplication::processEvents();
    SARibbonBar* ribbon = window.ribbonBar();
    ASSERT_NE(ribbon, nullptr);
    SARibbonQuickAccessBar* quick = ribbon->quickAccessBar();
    ASSERT_NE(quick, nullptr);
    EXPECT_TRUE(quick->isVisible());
    const QList<QAction*> expected = quickAccessActions(window.actionRegistry());
    EXPECT_EQ(quick->actions(), expected);
    EXPECT_EQ(quick->iconSize(), QSize(16, 16));
    for (QAction* action : expected) {
        auto* button = qobject_cast<QToolButton*>(quick->widgetForAction(action));
        ASSERT_NE(button, nullptr) << action->objectName().toStdString();
        EXPECT_EQ(button->toolButtonStyle(), Qt::ToolButtonIconOnly);
        EXPECT_FALSE(button->icon().isNull());
    }
    // In the title row, with the tabs (the compact style).
    const QRect tabs = ribbon->ribbonTabBar()->geometry();
    EXPECT_LT(quick->geometry().top(), tabs.bottom());
    EXPECT_GT(quick->geometry().bottom(), tabs.top());
}

TEST(RibbonSmoke, TheWindowDrawsItsOwnFrameExceptUnderWayland) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    // SARibbon applies its own style sheet again once the event loop runs.
    QCoreApplication::processEvents();
    const bool wayland = QGuiApplication::platformName().startsWith(QStringLiteral("wayland"));
    EXPECT_EQ(window.windowFlags().testFlag(Qt::FramelessWindowHint), !wayland);
    SARibbonSystemButtonBar* buttons = window.windowButtonBar();
    EXPECT_EQ(buttons != nullptr, !wayland);
    // The colours are those of the application's style sheet, the tokens.
    EXPECT_TRUE(window.styleSheet().isEmpty());
    if (buttons != nullptr) {
        for (QAbstractButton* button :
             {buttons->minimizeButton(), buttons->maximizeButton(), buttons->closeButton()}) {
            ASSERT_NE(button, nullptr);
            EXPECT_FALSE(button->icon().isNull());
        }
    }
}

}  // namespace
}  // namespace tg::app
