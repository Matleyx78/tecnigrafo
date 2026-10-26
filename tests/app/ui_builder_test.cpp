// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "action_registry.hpp"
#include "cad_view.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "ui_builder.hpp"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPointer>
#include <QSet>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

void collectMenu(const QMenu& menu, QSet<QAction*>& found) {
    for (QAction* action : menu.actions()) {
        found.insert(action);
        if (action->menu() != nullptr) {
            collectMenu(*action->menu(), found);
        }
    }
}

TEST(UiBuilderTest, EveryActionIsReachableInBothInterfaces) {
    MainWindow window;
    window.setScriptMode();
    for (const InterfaceKind kind : {InterfaceKind::Ribbon, InterfaceKind::Classic}) {
        window.setInterface(kind);
        QSet<QAction*> found;
        if (kind == InterfaceKind::Ribbon) {
            const auto* application = window.findChild<QMenu*>(QStringLiteral("applicationMenu"));
            ASSERT_NE(application, nullptr);
            collectMenu(*application, found);
        } else {
            for (const QAction* action : window.menuBar()->actions()) {
                ASSERT_NE(action->menu(), nullptr);
                collectMenu(*action->menu(), found);
            }
        }
        for (QAction* action : window.actionRegistry().actions()) {
            EXPECT_TRUE(found.contains(action)) << action->objectName().toStdString();
        }
    }
}

TEST(UiBuilderTest, SwitchingPreservesActionsAndDisposesPreviousWidgets) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    const auto actions = window.actionRegistry().actions();
    QAction* ortho = window.actionRegistry().action(QStringLiteral("aids.ortho"));
    ortho->setChecked(true);
    QAction* line = window.actionRegistry().action(QStringLiteral("draw.line"));
    line->setEnabled(false);
    for (int cycle = 0; cycle < 3; ++cycle) {
        const QPointer<QMenuBar> ribbon = window.menuBar();
        window.setInterface(InterfaceKind::Classic);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EXPECT_TRUE(ribbon.isNull());
        EXPECT_EQ(window.actionRegistry().actions(), actions);
        EXPECT_TRUE(ortho->isChecked());
        EXPECT_FALSE(line->isEnabled());
        const auto toolbars = window.findChildren<QToolBar*>(QString{}, Qt::FindDirectChildrenOnly);
        ASSERT_FALSE(toolbars.isEmpty());
        const QPointer<QToolBar> toolbar = toolbars.first();
        const QPointer<QMenuBar> classic = window.menuBar();
        window.setInterface(InterfaceKind::Ribbon);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EXPECT_TRUE(classic.isNull());
        EXPECT_TRUE(toolbar.isNull());
        EXPECT_TRUE(window.findChildren<QToolBar*>(QString{}, Qt::FindDirectChildrenOnly).isEmpty());
        EXPECT_EQ(window.actionRegistry().actions(), actions);
        EXPECT_TRUE(ortho->isChecked());
        EXPECT_FALSE(line->isEnabled());
    }
}

TEST(UiBuilderTest, RibbonButtonsAndClassicToolbarsUseRegisteredActions) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    QAction* line = window.actionRegistry().action(QStringLiteral("draw.line"));
    bool found = false;
    for (QToolButton* button : window.menuBar()->findChildren<QToolButton*>()) {
        if (button->defaultAction() == line) {
            found = true;
            button->click();
            EXPECT_TRUE(window.runner().isActive());
            events::press(window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input")),
                          Qt::Key_Escape);
        }
    }
    EXPECT_TRUE(found);
    window.setInterface(InterfaceKind::Classic);
    auto* draw = window.findChild<QToolBar*>(QStringLiteral("home/draw"));
    ASSERT_NE(draw, nullptr);
    EXPECT_TRUE(draw->actions().contains(line));
    auto* standard = window.findChild<QToolBar*>(QStringLiteral("standard"));
    ASSERT_NE(standard, nullptr);
    EXPECT_TRUE(standard->actions().contains(
        window.actionRegistry().action(QStringLiteral("file.save"))));
    EXPECT_TRUE(standard->actions().contains(
        window.actionRegistry().action(QStringLiteral("edit.undo"))));
}

TEST(UiBuilderTest, CommandsAndStatusButtonSwitchWithoutLosingFocusOrShortcuts) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    window.activateWindow();
    ASSERT_TRUE(QTest::qWaitForWindowActive(&window));
    auto* input = window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    auto* button = window.findChild<QToolButton*>(QStringLiteral("ribbonButton"));
    ASSERT_NE(input, nullptr);
    ASSERT_NE(button, nullptr);
    auto* history = window.findChild<QPlainTextEdit*>(QStringLiteral("history"));
    ASSERT_NE(history, nullptr);
    EXPECT_EQ(window.interfaceKind(), InterfaceKind::Ribbon);
    for (const QString& command : {QStringLiteral("RIBBONCLOSE"), QStringLiteral("RIBBON"),
                                   QStringLiteral("RIBBON"), QStringLiteral("RIBBONCLOSE")}) {
        input->setFocus();
        QTest::keyClicks(input, command);
        events::press(input, Qt::Key_Return);
        const bool ribbon = command == QStringLiteral("RIBBON");
        EXPECT_EQ(window.interfaceKind(), ribbon ? InterfaceKind::Ribbon : InterfaceKind::Classic);
        EXPECT_EQ(button->text(), ribbon ? QStringLiteral("RIBBON") : QStringLiteral("MENU"));
        EXPECT_FALSE(window.runner().isActive());
        EXPECT_EQ(QApplication::focusWidget(), input);
        EXPECT_FALSE(history->toPlainText().contains(QStringLiteral("Unknown command")));
        QAction* zoom = window.actionRegistry().action(QStringLiteral("view.zoom-extents"));
        const QSignalSpy triggered(zoom, &QAction::triggered);
        window.view()->setFocus();
        QTest::keyPress(window.view(), Qt::Key_E, Qt::ControlModifier | Qt::ShiftModifier);
        EXPECT_EQ(triggered.count(), 1);
    }
    button->click();
    EXPECT_EQ(window.interfaceKind(), InterfaceKind::Ribbon);
    EXPECT_EQ(button->text(), QStringLiteral("RIBBON"));
    button->click();
    EXPECT_EQ(window.interfaceKind(), InterfaceKind::Classic);
    EXPECT_EQ(button->text(), QStringLiteral("MENU"));
}

}  // namespace
}  // namespace tg::app
