// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Panels that hide themselves (docs/tappe/0.7.5/P18).

#include "auto_hide.hpp"

#include "action_registry.hpp"
#include "cad_view.hpp"
#include "main_window.hpp"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDockWidget>
#include <QEvent>
#include <QPoint>
#include <QRect>
#include <QSettings>
#include <QStackedWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>
#include <QTreeView>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

class AutoHideTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_directory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("TecnigrafoP18Test"));
        QCoreApplication::setApplicationName(QStringLiteral("AutoHide"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
    }

    void TearDown() override {
        QSettings::setDefaultFormat(m_format);
        QCoreApplication::setOrganizationName(m_organization);
        QCoreApplication::setApplicationName(m_application);
        applyTheme(*qobject_cast<QApplication*>(QApplication::instance()), Theme::Light);
    }

    static QDockWidget* properties(const MainWindow& window) {
        return window.findChild<QDockWidget*>(QStringLiteral("propertiesPanel"));
    }

    static QDockWidget* layers(const MainWindow& window) {
        return window.findChild<QDockWidget*>(QStringLiteral("layerPanel"));
    }

    /// The pin of the title bar of a docked panel.
    static QToolButton* pin(const QDockWidget* panel) {
        QWidget* title = panel->titleBarWidget();
        return title == nullptr ? nullptr
                                : title->findChild<QToolButton*>(QStringLiteral("autoHidePin"));
    }

    /// Runs what the event loop has queued, deletions included.
    static void settle() {
        QApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QApplication::processEvents();
    }

private:
    QTemporaryDir m_directory;
    QString m_organization{QCoreApplication::organizationName()};
    QString m_application{QCoreApplication::applicationName()};
    QSettings::Format m_format{QSettings::defaultFormat()};
};

TEST_F(AutoHideTest, UnpinningPropertiesPutsATabOnTheRightAndPinningDocksItThereAgain) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.show();
    ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
    QDockWidget* panel = properties(window);
    ASSERT_NE(panel, nullptr);
    // Properties alone on the right, so that hiding it frees the edge.
    ASSERT_NE(layers(window), nullptr);
    layers(window)->close();
    settle();
    ASSERT_EQ(window.dockWidgetArea(panel), Qt::RightDockWidgetArea);
    const int before = window.view()->width();

    QToolButton* button = pin(panel);
    ASSERT_NE(button, nullptr);
    EXPECT_FALSE(button->isHidden());
    button->click();
    settle();
    const AutoHide& autoHide = window.autoHide();
    EXPECT_TRUE(autoHide.isAutoHidden(panel));
    EXPECT_FALSE(panel->isVisible());
    const AutoHideBar* right = autoHide.bar(Qt::RightDockWidgetArea);
    EXPECT_TRUE(right->isVisible());
    ASSERT_EQ(right->tabs().size(), 1);
    EXPECT_EQ(right->tabs().front()->panel(), panel);
    EXPECT_FALSE(autoHide.bar(Qt::LeftDockWidgetArea)->isVisible());
    EXPECT_FALSE(autoHide.bar(Qt::BottomDockWidgetArea)->isVisible());
    EXPECT_GT(window.view()->width(), before);

    // The pin of the panel in the overlay docks it again.
    right->tabs().front()->click();
    settle();
    ASSERT_TRUE(autoHide.overlay()->isVisible());
    QWidget* page = autoHide.overlay()->pages()->currentWidget();
    ASSERT_NE(page, nullptr);
    auto* dock = page->findChild<QToolButton*>(QStringLiteral("autoHidePin"));
    ASSERT_NE(dock, nullptr);
    dock->click();
    settle();
    EXPECT_FALSE(autoHide.isAutoHidden(panel));
    EXPECT_FALSE(autoHide.overlay()->isVisible());
    EXPECT_FALSE(right->isVisible());
    EXPECT_TRUE(panel->isVisible());
    EXPECT_EQ(window.dockWidgetArea(panel), Qt::RightDockWidgetArea);
    ASSERT_NE(panel->widget(), nullptr);
    EXPECT_EQ(panel->widget()->parentWidget(), panel);
    EXPECT_NE(panel->findChild<QTreeView*>(QStringLiteral("propertiesGrid")), nullptr);
    EXPECT_TRUE(window.close());
}

TEST_F(AutoHideTest, AClickShowsTheOverlayInTheWindowEscClosesItAndAFocusedCellKeepsItOpen) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.show();
    window.activateWindow();
    ASSERT_TRUE(QTest::qWaitForWindowActive(&window));
    QDockWidget* panel = properties(window);
    ASSERT_NE(panel, nullptr);
    AutoHide& autoHide = window.autoHide();
    ASSERT_TRUE(autoHide.setAutoHidden(panel, true));
    settle();
    AutoHideTab* tab = autoHide.tab(panel);
    ASSERT_NE(tab, nullptr);
    QWidget* overlay = autoHide.overlay();
    EXPECT_FALSE(overlay->isVisible());

    tab->click();
    settle();
    EXPECT_TRUE(overlay->isVisible());
    EXPECT_TRUE(tab->isActive());
    EXPECT_EQ(autoHide.openPanel(), panel);
    // Inside the main window, never a window of its own, over the drawing.
    EXPECT_FALSE(overlay->isWindow());
    EXPECT_EQ(overlay->window(), &window);
    const QRect drawing(window.view()->mapTo(&window, QPoint(0, 0)), window.view()->size());
    const QRect shown(overlay->mapTo(&window, QPoint(0, 0)), overlay->size());
    EXPECT_TRUE(drawing.contains(shown));
    EXPECT_EQ(shown.height(), drawing.height());
    EXPECT_EQ(shown.right(), drawing.right());
    auto* grid = overlay->findChild<QTreeView*>(QStringLiteral("propertiesGrid"));
    ASSERT_NE(grid, nullptr);
    EXPECT_TRUE(grid->isVisible());

    // Esc closes it, wherever the keyboard is.
    QTest::keyClick(window.view(), Qt::Key_Escape);
    EXPECT_FALSE(overlay->isVisible());
    EXPECT_FALSE(tab->isActive());
    EXPECT_EQ(autoHide.openPanel(), nullptr);

    // A cell holding the keyboard keeps it open when the pointer leaves.
    tab->click();
    settle();
    ASSERT_TRUE(overlay->isVisible());
    grid->setFocus(Qt::OtherFocusReason);
    ASSERT_TRUE(grid->hasFocus());
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(overlay, &leave);
    QTest::qWait(2 * AutoHide::kHideDelayMs);
    EXPECT_TRUE(overlay->isVisible());

    // With the keyboard back in the drawing, it closes.
    window.view()->setFocus(Qt::OtherFocusReason);
    EXPECT_TRUE(
        QTest::qWaitFor([overlay] { return !overlay->isVisible(); }, 4 * AutoHide::kHideDelayMs));
    EXPECT_TRUE(window.view()->hasFocus());
    EXPECT_TRUE(window.close());
}

TEST_F(AutoHideTest, TheActionOfAHiddenPanelShowsItInTheOverlay) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.show();
    ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
    QDockWidget* panel = properties(window);
    ASSERT_NE(panel, nullptr);
    AutoHide& autoHide = window.autoHide();
    ASSERT_TRUE(autoHide.setAutoHidden(panel, true));
    QAction* action = window.actionRegistry().action(QStringLiteral("properties.panel"));
    ASSERT_NE(action, nullptr);
    action->trigger();
    settle();
    EXPECT_FALSE(panel->isVisible());
    EXPECT_TRUE(autoHide.isAutoHidden(panel));
    EXPECT_EQ(autoHide.openPanel(), panel);
    EXPECT_TRUE(autoHide.overlay()->isVisible());
    EXPECT_TRUE(window.close());
}

TEST_F(AutoHideTest, TheHiddenStateIsSavedRestoredAndKeptAcrossTheInterfaces) {
    {
        MainWindow window(MainWindow::InitialDrawing::Empty);
        window.show();
        ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
        QDockWidget* panel = properties(window);
        ASSERT_NE(panel, nullptr);
        ASSERT_TRUE(window.autoHide().setAutoHidden(panel, true));
        for (const InterfaceKind kind : {InterfaceKind::Classic, InterfaceKind::Ribbon}) {
            window.setInterface(kind);
            settle();
            EXPECT_TRUE(window.autoHide().isAutoHidden(panel));
            EXPECT_FALSE(panel->isVisible());
            EXPECT_EQ(window.autoHide().bar(Qt::RightDockWidgetArea)->tabs().size(), 1);
        }
        EXPECT_TRUE(window.close());
    }
    const QSettings settings;
    const QStringList saved = settings.value(QStringLiteral("ui/autoHide")).toStringList();
    ASSERT_EQ(saved.size(), 1);
    EXPECT_TRUE(saved.front().startsWith(QStringLiteral("propertiesPanel:right:")));

    MainWindow restored(MainWindow::InitialDrawing::Empty);
    restored.show();
    ASSERT_TRUE(QTest::qWaitForWindowExposed(&restored));
    settle();
    QDockWidget* panel = properties(restored);
    ASSERT_NE(panel, nullptr);
    AutoHide& autoHide = restored.autoHide();
    EXPECT_TRUE(autoHide.isAutoHidden(panel));
    EXPECT_FALSE(panel->isVisible());
    ASSERT_EQ(autoHide.bar(Qt::RightDockWidgetArea)->tabs().size(), 1);
    EXPECT_EQ(autoHide.bar(Qt::RightDockWidgetArea)->tabs().front()->panel(), panel);
    EXPECT_FALSE(layers(restored)->isHidden());
    restored.setInterface(InterfaceKind::Classic);
    settle();
    EXPECT_TRUE(autoHide.isAutoHidden(panel));
    restored.setInterface(InterfaceKind::Ribbon);
    settle();
    EXPECT_TRUE(autoHide.isAutoHidden(panel));

    // Pinned again, it docks where it was.
    EXPECT_TRUE(autoHide.setAutoHidden(panel, false));
    settle();
    EXPECT_TRUE(panel->isVisible());
    EXPECT_EQ(restored.dockWidgetArea(panel), Qt::RightDockWidgetArea);
    EXPECT_TRUE(restored.close());
}

TEST_F(AutoHideTest, AFloatingPanelHasNoPin) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.show();
    ASSERT_TRUE(QTest::qWaitForWindowExposed(&window));
    QDockWidget* panel = properties(window);
    ASSERT_NE(panel, nullptr);
    QToolButton* button = pin(panel);
    ASSERT_NE(button, nullptr);
    EXPECT_FALSE(button->isHidden());
    panel->setFloating(true);
    settle();
    EXPECT_TRUE(button->isHidden());
    EXPECT_FALSE(window.autoHide().setAutoHidden(panel, true));
    EXPECT_FALSE(window.autoHide().isAutoHidden(panel));
    panel->setFloating(false);
    settle();
    EXPECT_FALSE(button->isHidden());
    EXPECT_TRUE(window.close());
}

TEST_F(AutoHideTest, AScriptSessionHidesNothing) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.setScriptMode();
    window.show();
    QDockWidget* panel = properties(window);
    ASSERT_NE(panel, nullptr);
    QToolButton* button = pin(panel);
    ASSERT_NE(button, nullptr);
    EXPECT_TRUE(button->isHidden());
    EXPECT_FALSE(window.autoHide().setAutoHidden(panel, true));
    EXPECT_TRUE(window.autoHide().saveState().isEmpty());
    EXPECT_TRUE(window.close());
}

}  // namespace
}  // namespace tg::app
