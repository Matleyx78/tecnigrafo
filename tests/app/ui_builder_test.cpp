// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "ui_builder.hpp"

#include "action_registry.hpp"
#include "cad_view.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QEvent>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSet>
#include <QSignalSpy>
#include <QSize>
#include <QString>
#include <QTabBar>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <gtest/gtest.h>

#include <SARibbon.h>
#include <cstdlib>

namespace tg::app {
namespace {

/// Runs what the event loop has queued, the deletions and the layouts
/// that the ribbon asks once the events in progress are over.
void settle() {
    for (int round = 0; round < 3; ++round) {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
}

/// The geometry of `widget` in the coordinates of `ribbon`.
QRect inRibbon(const QWidget& widget, const SARibbonBar& ribbon) {
    return {widget.mapTo(&ribbon, QPoint(0, 0)), widget.size()};
}

/// The actions of the quick access bar, in their order.
QList<QAction*> quickAccessActions(const ActionRegistry& registry) {
    return {
        registry.action(QStringLiteral("file.new")), registry.action(QStringLiteral("file.open")),
        registry.action(QStringLiteral("file.save")), registry.action(QStringLiteral("edit.undo")),
        registry.action(QStringLiteral("edit.redo"))};
}

void collectMenu(const QMenu& menu, QSet<QAction*>& found) {
    QList<const QMenu*> pending{&menu};
    QSet<const QMenu*> visited;
    while (!pending.isEmpty()) {
        const QMenu* current = pending.takeLast();
        if (visited.contains(current)) {
            continue;
        }
        visited.insert(current);
        for (QAction* action : current->actions()) {
            found.insert(action);
            if (action->menu() != nullptr) {
                pending.append(action->menu());
            }
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
        const auto remaining =
            window.findChildren<QToolBar*>(QString{}, Qt::FindDirectChildrenOnly);
        EXPECT_TRUE(remaining.isEmpty());
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
    const auto& registry = window.actionRegistry();
    EXPECT_TRUE(standard->actions().contains(registry.action(QStringLiteral("file.save"))));
    EXPECT_TRUE(standard->actions().contains(registry.action(QStringLiteral("edit.undo"))));
}

TEST(UiBuilderTest, CommandsAndStatusButtonSwitchWithoutLosingFocusOrShortcuts) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    window.activateWindow();
    ASSERT_TRUE(QTest::qWaitForWindowActive(&window));
    ASSERT_EQ(QApplication::focusWidget(), window.view());
    events::type(QApplication::focusWidget(), "LINE");
    events::press(QApplication::focusWidget(), Qt::Key_Return);
    EXPECT_TRUE(window.runner().isActive());
    events::press(QApplication::focusWidget(), Qt::Key_Escape);
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
        events::type(input, command.toStdString());
        events::press(input, Qt::Key_Return);
        const bool ribbon = command == QStringLiteral("RIBBON");
        EXPECT_EQ(window.interfaceKind(), ribbon ? InterfaceKind::Ribbon : InterfaceKind::Classic);
        EXPECT_EQ(button->isChecked(), ribbon);
        EXPECT_FALSE(window.runner().isActive());
        EXPECT_EQ(QApplication::focusWidget(), input);
        EXPECT_FALSE(history->toPlainText().contains(QStringLiteral("Unknown command")));
        QAction* zoom = window.actionRegistry().action(QStringLiteral("view.zoom-extents"));
        const QSignalSpy triggered(zoom, &QAction::triggered);
        window.view()->setFocus();
        events::press(window.view(), Qt::Key_E, Qt::ControlModifier | Qt::ShiftModifier);
        EXPECT_EQ(triggered.count(), 1);
    }
    button->click();
    EXPECT_EQ(window.interfaceKind(), InterfaceKind::Ribbon);
    EXPECT_TRUE(button->isChecked());
    button->click();
    EXPECT_EQ(window.interfaceKind(), InterfaceKind::Classic);
    EXPECT_FALSE(button->isChecked());
}

TEST(UiBuilderTest, BothInterfacesHaveTheQuickAccessBar) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    const ActionRegistry& registry = window.actionRegistry();
    const QList<QAction*> expected{
        registry.action(QStringLiteral("file.new")), registry.action(QStringLiteral("file.open")),
        registry.action(QStringLiteral("file.save")), registry.action(QStringLiteral("edit.undo")),
        registry.action(QStringLiteral("edit.redo"))};
    for (int cycle = 0; cycle < 2; ++cycle) {
        window.setInterface(InterfaceKind::Classic);
        // Qt shows a new menu bar from the event loop, and deletes the old one later.
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        EXPECT_EQ(window.ribbonBar(), nullptr);
        // At the left of the menu bar, before the menus.
        auto* quick = qobject_cast<QToolBar*>(window.menuBar()->cornerWidget(Qt::TopLeftCorner));
        ASSERT_NE(quick, nullptr);
        EXPECT_EQ(quick->objectName(), QStringLiteral("quickAccessBar"));
        EXPECT_EQ(quick->actions(), expected);
        EXPECT_EQ(quick->toolButtonStyle(), Qt::ToolButtonIconOnly);
        EXPECT_EQ(quick->iconSize(), QSize(16, 16));
        EXPECT_TRUE(quick->isVisible());

        window.setInterface(InterfaceKind::Ribbon);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        SARibbonBar* ribbon = window.ribbonBar();
        ASSERT_NE(ribbon, nullptr);
        EXPECT_EQ(ribbon->quickAccessBar()->actions(), expected);
    }
}

TEST(UiBuilderTest, TheClassicMenuBarOfAWindowWithoutFrameShowsTheTitle) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.setScriptMode();
    window.show();
    window.setInterface(InterfaceKind::Classic);
    QCoreApplication::processEvents();
    auto* title = window.menuBar()->findChild<QLabel*>(QStringLiteral("classicTitle"));
    if (window.windowButtonBar() == nullptr) {
        // The frame of the system shows the title (Wayland).
        EXPECT_EQ(title, nullptr);
        return;
    }
    ASSERT_NE(title, nullptr);
    EXPECT_TRUE(title->isVisible());
    EXPECT_FALSE(title->text().contains(QStringLiteral("[*]")));
    EXPECT_TRUE(title->text().endsWith(QStringLiteral("— Tecnigrafo")))
        << title->text().toStdString();
    // A change marks the title as the platform does.
    auto* input = window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    ASSERT_NE(input, nullptr);
    events::type(input, "LINE");
    events::press(input, Qt::Key_Return);
    events::type(input, "0,0");
    events::press(input, Qt::Key_Return);
    events::type(input, "10,0");
    events::press(input, Qt::Key_Return);
    events::press(input, Qt::Key_Return);
    ASSERT_TRUE(window.isWindowModified());
    EXPECT_TRUE(title->text().contains(QStringLiteral("* — Tecnigrafo")))
        << title->text().toStdString();
    // The window buttons stay above the bar.
    QWidget* buttons = window.windowButtonBar();
    EXPECT_TRUE(buttons->isVisible());
    const QList<QObject*> siblings = window.children();
    EXPECT_GT(siblings.indexOf(buttons), siblings.indexOf(window.menuBar()));
}

TEST(UiBuilderTest, WithTheFrameOfTheSystemTheQuickAccessBarIsTheFirstOfTheRowOfTheTabs) {
    // docs/tappe/0.7.5/P40, point 1: under Wayland the ribbon has the
    // compact style, whose own quick access bar would be at the right of
    // the tabs; a toolbar takes its place at the top left, before File.
    MainWindow window(MainWindow::InitialDrawing::Empty, nullptr, {}, WindowFrame::System);
    window.setScriptMode();
    window.resize(1200, 800);
    window.show();
    settle();
    ASSERT_EQ(window.windowButtonBar(), nullptr) << "the frame of the system";
    SARibbonBar* ribbon = window.ribbonBar();
    ASSERT_NE(ribbon, nullptr);
    QAbstractButton* file = ribbon->applicationButton();
    ASSERT_NE(file, nullptr);
    for (const int width : {1200, 1500}) {
        SCOPED_TRACE(width);
        // A resize of the bar must not move it: QMenuBar places corner
        // widgets of its own accord.
        window.resize(width, 800);
        settle();
        auto* quick = qobject_cast<QToolBar*>(ribbon->cornerWidget(Qt::TopLeftCorner));
        ASSERT_NE(quick, nullptr);
        EXPECT_EQ(quick->objectName(), QStringLiteral("quickAccessBar"));
        EXPECT_TRUE(quick->isVisible());
        EXPECT_FALSE(ribbon->quickAccessBar()->isVisible());
        EXPECT_EQ(quick->actions(), quickAccessActions(window.actionRegistry()));
        const QRect bar = inRibbon(*quick, *ribbon);
        const QRect button = inRibbon(*file, *ribbon);
        // Its right edge at the left of File (SARibbon lays File out from
        // the last column of the bar), in the same row.
        EXPECT_LE(bar.right(), button.left());
        EXPECT_LT(bar.left(), button.left());
        EXPECT_LE(bar.top(), button.top());
        EXPECT_GT(bar.center().y(), button.top());
        EXPECT_LT(bar.center().y(), button.bottom());
    }
}

TEST(UiBuilderTest, WithTheFrameOfTheSystemTheInterfacesSwitchAndKeepTheirOwnQuickAccessBar) {
    // QMainWindow::setMenuBar() hands the corner widgets of the bar it
    // replaces to the new one: the quick access bar of the compact ribbon
    // must not reach the classic menu bar, nor outlive its ribbon (the crash
    // of the manual test of #651, from the dark ribbon to the light classic
    // menus, and at the next start, which restores the classic menus).
    MainWindow window(MainWindow::InitialDrawing::Empty, nullptr, {}, WindowFrame::System);
    window.setScriptMode();
    window.resize(1200, 800);
    window.show();
    settle();
    const QList<QAction*> expected = quickAccessActions(window.actionRegistry());
    for (int cycle = 0; cycle < 2; ++cycle) {
        SCOPED_TRACE(cycle);
        window.setTheme(Theme::Light);
        window.setInterface(InterfaceKind::Classic);
        settle();
        ASSERT_EQ(window.ribbonBar(), nullptr);
        QMenuBar* bar = window.menuBar();
        auto* quick = qobject_cast<QToolBar*>(bar->cornerWidget(Qt::TopLeftCorner));
        ASSERT_NE(quick, nullptr);
        EXPECT_EQ(quick->parentWidget(), bar) << "the bar of the classic menus";
        EXPECT_EQ(quick->actions(), expected);
        EXPECT_TRUE(quick->isVisible());
        // Moves and resizes after the ribbon is gone lay nothing out.
        window.resize(1300 + 50 * cycle, 820);
        settle();

        window.setTheme(Theme::Dark);
        window.setInterface(InterfaceKind::Ribbon);
        settle();
        SARibbonBar* ribbon = window.ribbonBar();
        ASSERT_NE(ribbon, nullptr);
        auto* corner = qobject_cast<QToolBar*>(ribbon->cornerWidget(Qt::TopLeftCorner));
        ASSERT_NE(corner, nullptr);
        EXPECT_EQ(corner->parentWidget(), ribbon);
        EXPECT_EQ(corner->actions(), expected);
        window.resize(1200, 800);
        settle();
    }
    applyTheme(*qobject_cast<QApplication*>(QApplication::instance()), Theme::Light);
}

TEST(UiBuilderTest, WithItsOwnFrameTheQuickAccessBarStaysInTheTitleRow) {
    MainWindow window(MainWindow::InitialDrawing::Empty, nullptr, {}, WindowFrame::Own);
    window.setScriptMode();
    window.resize(1200, 800);
    window.show();
    settle();
    ASSERT_NE(window.windowButtonBar(), nullptr) << "the ribbon's own frame";
    SARibbonBar* ribbon = window.ribbonBar();
    ASSERT_NE(ribbon, nullptr);
    EXPECT_EQ(ribbon->cornerWidget(Qt::TopLeftCorner), nullptr);
    SARibbonQuickAccessBar* quick = ribbon->quickAccessBar();
    EXPECT_TRUE(quick->isVisible());
    EXPECT_EQ(quick->actions(), quickAccessActions(window.actionRegistry()));
    const QRect bar = inRibbon(*quick, *ribbon);
    // Above the row of the tabs and at the left of the title.
    EXPECT_LE(bar.bottom(), inRibbon(*ribbon->ribbonTabBar(), *ribbon).top());
    const auto* layout = qobject_cast<SARibbonBarLayout*>(ribbon->layout());
    ASSERT_NE(layout, nullptr);
    if (!layout->titleRect().isEmpty()) {
        EXPECT_LE(bar.right(), layout->titleRect().left());
    }
}

TEST(UiBuilderTest, TheTabsOfTheRibbonAreAsHighAsFileAndTheDocumentTabKeepsItsAccent) {
    // docs/tappe/0.7.5/P40, point 2: same top edge and same height as File,
    // within 1 px, with both frames and in both themes; the rules of the
    // tabs of the documents stay theirs.
    constexpr int kTolerance = 1;
    constexpr int kRounding = 8;
    auto* application = qobject_cast<QApplication*>(QApplication::instance());
    ASSERT_NE(application, nullptr);
    for (const WindowFrame frame : {WindowFrame::System, WindowFrame::Own}) {
        for (const Theme theme : {Theme::Light, Theme::Dark}) {
            SCOPED_TRACE(frame == WindowFrame::System ? "system frame" : "own frame");
            SCOPED_TRACE(theme == Theme::Dark ? "dark" : "light");
            MainWindow window(MainWindow::InitialDrawing::Sample, nullptr, {}, frame);
            window.setScriptMode();
            window.resize(1200, 800);
            window.setTheme(theme);
            window.show();
            settle();
            SARibbonBar* ribbon = window.ribbonBar();
            ASSERT_NE(ribbon, nullptr);
            SARibbonTabBar* tabs = ribbon->ribbonTabBar();
            ASSERT_NE(tabs, nullptr);
            ASSERT_GE(tabs->count(), 2) << "Home and View";
            const QRect file = inRibbon(*ribbon->applicationButton(), *ribbon);
            const QPoint origin = tabs->mapTo(ribbon, QPoint(0, 0));
            for (int index = 0; index < tabs->count(); ++index) {
                SCOPED_TRACE(tabs->tabText(index).toStdString());
                const QRect tab = tabs->tabRect(index).translated(origin);
                ASSERT_FALSE(tab.isEmpty());
                EXPECT_LE(std::abs(tab.top() - file.top()), kTolerance)
                    << "tab " << tab.top() << ", File " << file.top();
                EXPECT_LE(std::abs(tab.height() - file.height()), kTolerance)
                    << "tab " << tab.height() << ", File " << file.height();
            }

            // The tab of the document shown has its top edge in the accent.
            auto* documents = window.findChild<QTabBar*>(QStringLiteral("documentTabBar"));
            ASSERT_NE(documents, nullptr);
            ASSERT_GE(documents->currentIndex(), 0);
            const QRect current = documents->tabRect(documents->currentIndex());
            const QImage image = documents->grab().toImage();
            const qreal ratio = image.devicePixelRatio();
            const QColor top = image.pixelColor(qRound(current.center().x() * ratio),
                                                qRound(current.top() * ratio));
            const QColor& accent = themeTokens(theme).accent;
            EXPECT_LE(std::abs(top.red() - accent.red()), kRounding) << top.name().toStdString();
            EXPECT_LE(std::abs(top.green() - accent.green()), kRounding);
            EXPECT_LE(std::abs(top.blue() - accent.blue()), kRounding);
            EXPECT_TRUE(window.close());
        }
    }
    applyTheme(*application, Theme::Light);
}

}  // namespace
}  // namespace tg::app
