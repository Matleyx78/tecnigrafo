// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "action_registry.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "theme.hpp"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QImage>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMetaObject>
#include <QPalette>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSet>
#include <QSignalSpy>
#include <QSize>
#include <QStatusBar>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <gtest/gtest.h>

#include <SARibbon.h>
#include <array>
#include <string>
#include <utility>

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

TEST(RibbonSmoke, TheRibbonHasAFileTabThatOpensTheApplicationMenu) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    QCoreApplication::processEvents();
    SARibbonBar* ribbon = window.ribbonBar();
    ASSERT_NE(ribbon, nullptr);
    // Three rows of small buttons in a group (docs/adr/0039, point 3); the
    // title row of its own only in a window without the system's frame.
    EXPECT_TRUE(SARibbonBar::isThreeRowStyle(ribbon->currentRibbonStyle()));
    EXPECT_EQ(SARibbonBar::isLooseStyle(ribbon->currentRibbonStyle()),
              window.windowButtonBar() != nullptr);
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
    // A click opens the menu, which runs its own event loop until closed:
    // the menu closes itself once it is shown. Qt may open it from the
    // event loop rather than inside the click.
    bool shown = false;
    QObject::connect(menu, &QMenu::aboutToShow, menu, [menu, &shown] {
        shown = true;
        QTimer::singleShot(0, menu, &QMenu::close);
    });
    events::click(file, Qt::LeftButton, file->rect().center());
    QCoreApplication::processEvents();
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
    const QRect tabs = ribbon->ribbonTabBar()->geometry();
    const QRect bar = quick->geometry();
    if (window.windowButtonBar() == nullptr) {
        // The frame of the system (Wayland): under its title, in the row of
        // the tabs (docs/adr/0039, point 4).
        EXPECT_LT(bar.top(), tabs.bottom());
        EXPECT_GT(bar.bottom(), tabs.top());
        return;
    }
    // Area 1: at the left of the title row, after the icon of the program;
    // area 2: the tabs right under it, the File tab first.
    EXPECT_LE(bar.bottom(), tabs.top());
    EXPECT_LT(bar.right(), ribbon->width() / 4);
    const QWidget* icon = ribbon->titleIconWidget();
    if (icon != nullptr && icon->isVisible()) {
        EXPECT_GE(bar.left(), icon->geometry().right());
    }
    const QRect file = ribbon->applicationButton()->geometry();
    EXPECT_LE(file.right(), tabs.left());
    EXPECT_LT(file.top(), tabs.bottom());
    EXPECT_GT(file.bottom(), tabs.top());
}

/// The button of the ribbon that runs `action`.
QToolButton* ribbonButtonOf(SARibbonBar& ribbon, const QAction* action) {
    for (auto* button : ribbon.findChildren<QToolButton*>()) {
        if (button->defaultAction() == action) {
            return button;
        }
    }
    return nullptr;
}

TEST(RibbonSmoke, ModifyCommandsUseSmallButtonsInStacksAndTheClassicToolbar) {
    MainWindow window;
    window.setScriptMode();
    window.resize(2400, 900);
    window.show();
    QCoreApplication::processEvents();
    SARibbonBar* ribbon = window.ribbonBar();
    ASSERT_NE(ribbon, nullptr);
    auto* panel = ribbon->findChild<SARibbonPanel*>(QStringLiteral("home/modify"));
    ASSERT_NE(panel, nullptr);
    const ActionRegistry& registry = window.actionRegistry();
    const QStringList ids{
        QStringLiteral("edit.move"),    QStringLiteral("edit.copy"),
        QStringLiteral("edit.rotate"),  QStringLiteral("edit.scale"),
        QStringLiteral("edit.mirror"),  QStringLiteral("edit.explode"),
        QStringLiteral("edit.trim"),    QStringLiteral("edit.extend"),
        QStringLiteral("edit.offset"),  QStringLiteral("edit.fillet"),
        QStringLiteral("edit.chamfer"), QStringLiteral("edit.join"),
    };
    for (const QString& id : ids) {
        SCOPED_TRACE(id.toStdString());
        QAction* action = registry.action(id);
        ASSERT_NE(action, nullptr);
        auto* button = dynamic_cast<SARibbonToolButton*>(ribbonButtonOf(*ribbon, action));
        ASSERT_NE(button, nullptr);
        EXPECT_TRUE(panel->isAncestorOf(button));
        EXPECT_TRUE(button->isVisible());
        EXPECT_EQ(button->buttonType(), SARibbonToolButton::SmallButton);
        EXPECT_FALSE(button->icon().isNull());
    }
    // ADR 0039: three small buttons in each column.
    constexpr qsizetype kStackRows = 3;
    for (qsizetype start = 0; start < ids.size(); start += kStackRows) {
        const QToolButton* first = ribbonButtonOf(*ribbon, registry.action(ids[start]));
        ASSERT_NE(first, nullptr);
        for (qsizetype i = start + 1; i < start + kStackRows; ++i) {
            const QToolButton* button = ribbonButtonOf(*ribbon, registry.action(ids[i]));
            ASSERT_NE(button, nullptr);
            EXPECT_EQ(button->x(), first->x());
            EXPECT_GT(button->y(), first->y());
        }
    }
    // No responses are injected: the buttons start the same mode as typing.
    auto* input = window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    ASSERT_NE(input, nullptr);
    for (const QString& id : {QStringLiteral("edit.trim"), QStringLiteral("edit.extend")}) {
        const QString command = registry.spec(id)->command;
        events::type(input, command.toStdString());
        events::press(input, Qt::Key_Return);
        ASSERT_TRUE(window.runner().isActive());
        const std::string prompt = window.runner().promptText();
        events::press(input, Qt::Key_Escape);
        QToolButton* button = ribbonButtonOf(*ribbon, registry.action(id));
        ASSERT_NE(button, nullptr);
        events::click(button, Qt::LeftButton, button->rect().center());
        ASSERT_TRUE(window.runner().isActive());
        EXPECT_EQ(window.runner().promptText(), prompt);
        events::press(input, Qt::Key_Escape);
        ASSERT_FALSE(window.runner().isActive());
    }
    registry.action(QStringLiteral("ui.interface"))->trigger();
    QCoreApplication::processEvents();
    const auto* toolbar = window.findChild<QToolBar*>(QStringLiteral("home/modify"));
    ASSERT_NE(toolbar, nullptr);
    EXPECT_EQ(toolbar->windowTitle(), QStringLiteral("Modify"));
    for (const QString& id : ids) {
        SCOPED_TRACE(id.toStdString());
        QAction* action = registry.action(id);
        EXPECT_TRUE(toolbar->actions().contains(action));
        const QWidget* button = toolbar->widgetForAction(action);
        ASSERT_NE(button, nullptr);
        EXPECT_TRUE(button->isVisible());
    }
}

TEST(RibbonSmoke, CircleAndArcShowTheirVariantsInADropDownWithTheirIcons) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    QCoreApplication::processEvents();
    SARibbonBar* ribbon = window.ribbonBar();
    ASSERT_NE(ribbon, nullptr);
    const ActionRegistry& registry = window.actionRegistry();
    const QList<std::pair<QString, QStringList>> expected{
        {QStringLiteral("draw.circle"),
         {QStringLiteral("Center, &Radius"), QStringLiteral("Center, &Diameter"),
          QStringLiteral("&2-Point"), QStringLiteral("&3-Point"),
          QStringLiteral("&Tan, Tan, Radius")}},
        {QStringLiteral("draw.arc"),
         {QStringLiteral("&3-Point"), QStringLiteral("&Start, Center, End")}}};
    for (const auto& [id, texts] : expected) {
        SCOPED_TRACE(id.toStdString());
        QAction* action = registry.action(id);
        ASSERT_NE(action, nullptr);
        QToolButton* button = ribbonButtonOf(*ribbon, action);
        ASSERT_NE(button, nullptr);
        EXPECT_EQ(button->popupMode(), QToolButton::MenuButtonPopup);
        QMenu* menu = action->menu();
        ASSERT_NE(menu, nullptr);
        EXPECT_EQ(menu->actions(), registry.variants(id));
        QStringList shown;
        QSet<qint64> icons;
        for (const QAction* variant : menu->actions()) {
            shown.append(variant->text());
            EXPECT_FALSE(variant->icon().isNull()) << variant->objectName().toStdString();
            icons.insert(variant->icon().cacheKey());
        }
        EXPECT_EQ(shown, texts);
        EXPECT_EQ(icons.size(), texts.size()) << "each variant has its own icon";
        // The arrow of the button opens that menu; it closes itself once
        // shown, as the File tab's does.
        bool opened = false;
        const QMetaObject::Connection shownOnce =
            QObject::connect(menu, &QMenu::aboutToShow, menu, [menu, &opened] {
                opened = true;
                QTimer::singleShot(0, menu, &QMenu::close);
            });
        button->showMenu();
        QCoreApplication::processEvents();
        QObject::disconnect(shownOnce);
        EXPECT_TRUE(opened);
    }
}

TEST(RibbonSmoke, TheButtonOfCircleRemembersTheLastVariantUsed) {
    MainWindow window;
    window.setScriptMode();
    window.show();
    QCoreApplication::processEvents();
    SARibbonBar* ribbon = window.ribbonBar();
    ASSERT_NE(ribbon, nullptr);
    const ActionRegistry& registry = window.actionRegistry();
    QAction* circle = registry.action(QStringLiteral("draw.circle"));
    QAction* twoPoints = registry.action(QStringLiteral("draw.circle.2-point"));
    ASSERT_NE(circle, nullptr);
    ASSERT_NE(twoPoints, nullptr);
    QToolButton* button = ribbonButtonOf(*ribbon, circle);
    ASSERT_NE(button, nullptr);
    auto* input = window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    ASSERT_NE(input, nullptr);
    const std::string center = "Specify center point for circle or [3P/2P/Ttr (tan tan radius)]:";

    // At first the button runs Center, Radius, and shows its icon.
    QAction* first = registry.variants(QStringLiteral("draw.circle")).value(0);
    EXPECT_EQ(registry.currentVariant(QStringLiteral("draw.circle")), first);
    EXPECT_EQ(button->icon().cacheKey(), first->icon().cacheKey());
    button->click();
    EXPECT_EQ(window.runner().promptText(), center);
    events::press(input, Qt::Key_Escape);
    ASSERT_FALSE(window.runner().isActive());

    // 2-Point, chosen from the menu, becomes the button's.
    twoPoints->trigger();
    EXPECT_EQ(window.runner().promptText(), "Specify first end point of circle's diameter:");
    events::press(input, Qt::Key_Escape);
    ASSERT_FALSE(window.runner().isActive());
    EXPECT_EQ(registry.currentVariant(QStringLiteral("draw.circle")), twoPoints);
    EXPECT_EQ(button->icon().cacheKey(), twoPoints->icon().cacheKey());
    button->click();
    EXPECT_EQ(window.runner().promptText(), "Specify first end point of circle's diameter:");
    events::press(input, Qt::Key_Escape);

    // A new theme redraws the icons: the button keeps the variant's.
    window.setTheme(Theme::Dark);
    EXPECT_EQ(button->icon().cacheKey(), twoPoints->icon().cacheKey());
    EXPECT_EQ(twoPoints->icon().cacheKey(),
              themedIcon(QStringLiteral("draw-circle-2p"), Theme::Dark).cacheKey());

    // The classic interface has the same in its toolbar.
    window.setInterface(InterfaceKind::Classic);
    auto* draw = window.findChild<QToolBar*>(QStringLiteral("home/draw"));
    ASSERT_NE(draw, nullptr);
    auto* classic = qobject_cast<QToolButton*>(draw->widgetForAction(circle));
    ASSERT_NE(classic, nullptr);
    EXPECT_EQ(classic->popupMode(), QToolButton::MenuButtonPopup);
    EXPECT_EQ(classic->icon().cacheKey(), twoPoints->icon().cacheKey());
}

TEST(RibbonSmoke, TheWholeWindowTakesEachThemeAfterTheEventLoop) {
    // The gallery's order: shown, then the theme, then the event loop, in
    // which SARibbon would apply its own theme again (docs/adr/0039,
    // point 2). Every surface outside the ribbon is the panel of the theme.
    MainWindow window;
    window.setScriptMode();
    window.show();
    for (const InterfaceKind kind : {InterfaceKind::Ribbon, InterfaceKind::Classic}) {
        for (const Theme theme : {Theme::Dark, Theme::Light, Theme::Dark}) {
            SCOPED_TRACE(testing::Message()
                         << (kind == InterfaceKind::Ribbon ? "ribbon " : "classic ")
                         << (theme == Theme::Dark ? "dark" : "light"));
            window.setTheme(theme);
            window.setInterface(kind);
            QCoreApplication::processEvents();
            EXPECT_TRUE(window.styleSheet().isEmpty());
            const QColor panel = themeTokens(theme).panel;
            QStatusBar* status = window.statusBar();
            auto* layers = window.findChild<QWidget*>(QStringLiteral("layerPanel"));
            ASSERT_NE(layers, nullptr);
            const std::array<const QWidget*, 4> surfaces{&window, status, window.commandLine(),
                                                         layers};
            for (const QWidget* widget : surfaces) {
                EXPECT_EQ(widget->palette().color(QPalette::Window), panel)
                    << widget->objectName().toStdString();
            }
            // As drawn in the whole window, not only in the widget alone: the
            // middle of the status bar shows the window's background.
            const QImage image = window.grab().toImage();
            const QPoint middle =
                status->mapTo(&window, QPoint(status->width() / 2, status->height() / 2));
            const qreal ratio = image.devicePixelRatio();
            EXPECT_EQ(image.pixelColor(QPointF(middle * ratio).toPoint()), panel);
        }
    }
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
