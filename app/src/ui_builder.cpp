// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "ui_builder.hpp"

#include "action_registry.hpp"
#include "environments.hpp"
#include "layer_panel.hpp"
#include "properties_panel.hpp"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMetaObject>
#include <QMouseEvent>
#include <QObject>
#include <QPointer>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

#include <SARibbon.h>
#include <algorithm>
#include <ranges>
#include <utility>

namespace tg::app {
namespace {

QString title(const QString& name) {
    if (name == QStringLiteral("home")) {
        return QObject::tr("Home");
    }
    if (name == QStringLiteral("view")) {
        return QObject::tr("View");
    }
    if (name == QStringLiteral("draw")) {
        return QObject::tr("Draw");
    }
    if (name == QStringLiteral("modify")) {
        return QObject::tr("Modify");
    }
    if (name == QStringLiteral("layers")) {
        return QObject::tr("Layers");
    }
    if (name == QStringLiteral("properties")) {
        return QObject::tr("Properties");
    }
    if (name == QStringLiteral("navigate")) {
        return QObject::tr("Navigate");
    }
    if (name == QStringLiteral("aids")) {
        return QObject::tr("Drawing Aids");
    }
    return name;
}

QStringList groups() {
    return {QStringLiteral("home/draw"),     QStringLiteral("home/modify"),
            QStringLiteral("home/layers"),   QStringLiteral("home/properties"),
            QStringLiteral("view/navigate"), QStringLiteral("view/aids")};
}

/// The tabs of the ribbon that every environment has (docs/adr/0039, point
/// 5): the ones of groups(). The contextual tabs of an environment follow.
QStringList fixedTabs() {
    QStringList tabs;
    for (const QString& path : groups()) {
        const QString tab = path.section(QLatin1Char('/'), 0, 0);
        if (!tabs.contains(tab)) {
            tabs.append(tab);
        }
    }
    return tabs;
}

/// Whether `action` is one of `registry`, not an action Qt made for a menu.
bool isRegistered(const ActionRegistry& registry, const QAction* action) {
    return !action->objectName().isEmpty() && registry.action(action->objectName()) == action;
}

/// Keep Quick Select after the modification commands so their columns of
/// three stay together even though the classic menus register Tools first.
QList<QAction*> toolActions(const ActionRegistry& registry) {
    QList<QAction*> actions = registry.actions();
    QAction* quickSelect = registry.action(QStringLiteral("edit.quick-select"));
    if (actions.removeOne(quickSelect)) {
        actions.append(quickSelect);
    }
    return actions;
}

/// The small icons of the quick access bar: the 16 px grid of docs/adr/0040,
/// point 3 (docs/adr/0039, point 1, area 1).
constexpr int kQuickAccessIconSize = 16;

/// Fills `bar` with the quick access actions of `registry`, icons only.
void fillQuickAccess(QToolBar& bar, const ActionRegistry& registry) {
    bar.setIconSize(QSize(kQuickAccessIconSize, kQuickAccessIconSize));
    bar.setToolButtonStyle(Qt::ToolButtonIconOnly);
    bar.setMovable(false);
    bar.setFloatable(false);
    for (QAction* action : registry.actions()) {
        if (registry.spec(action->objectName())->quickAccess) {
            bar.addAction(action);
        }
    }
}

/// SARibbon's compact style gives the controls of its single row the height
/// of the row less this many pixels, one above and one below
/// (SARibbonBarLayout::resizeInCompactStyle).
constexpr int kCompactRowInset = 2;

/// Keeps the quick access bar of the compact ribbon where SARibbon's layout
/// puts it, first in the row of the tabs (docs/tappe/0.7.5/P40, point 1).
/// SARibbonBar derives from QMenuBar, which also places its corner widgets
/// when it is resized or one of them is shown, in the middle of its whole
/// height: whenever the bar moves, changes size or asks for a new layout,
/// the ribbon lays itself out again, once the events in progress are over.
/// The ribbon is watched, not borrowed: UiBuilder::show() takes the bar off
/// a ribbon it replaces, but a layout asked for while the ribbon goes away
/// finds nothing to lay out.
class CornerKeeper final : public QObject {
public:
    CornerKeeper(SARibbonBar& ribbon, QWidget& corner) : QObject(&corner), m_ribbon(&ribbon) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        switch (event->type()) {
            case QEvent::Move:
            case QEvent::Resize:
            case QEvent::Show:
            case QEvent::LayoutRequest:
                if (!m_pending) {
                    m_pending = true;
                    QMetaObject::invokeMethod(
                        this,
                        [this] {
                            m_pending = false;
                            SARibbonBar* ribbon = m_ribbon.data();
                            if (ribbon == nullptr) {
                                return;
                            }
                            if (QLayout* layout = ribbon->layout(); layout != nullptr) {
                                layout->setGeometry(ribbon->rect());
                            }
                        },
                        Qt::QueuedConnection);
                }
                break;
            default:
                break;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QPointer<SARibbonBar> m_ribbon;
    bool m_pending{false};
};

/// The text of the File tab, on the accent colour: the lighter of the two
/// tokens that text is written in (docs/adr/0039, point 1, area 2, where it
/// is white on the accent in both themes).
QColor lightText(const ThemeTokens& tokens) {
    return tokens.field.lightness() > tokens.text.lightness() ? tokens.field : tokens.text;
}

/// Lets the classic menu bar move and resize a window that draws its own
/// frame, as SARibbonMainWindow does for its ribbon: the bar covers the top
/// of the window, where SAFramelessHelper looks for the pointer. A press on
/// a menu title stays with the bar alone, so that opening a menu never
/// starts moving the window.
class FrameForwarder final : public QObject {
public:
    FrameForwarder(QMenuBar& bar, QWidget& window) : QObject(&bar), m_bar(bar), m_window(window) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        switch (event->type()) {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonDblClick: {
                const auto* mouse = dynamic_cast<const QMouseEvent*>(event);
                if (mouse != nullptr && m_bar.actionAt(mouse->position().toPoint()) == nullptr) {
                    QApplication::sendEvent(&m_window, event);
                }
                break;
            }
            case QEvent::MouseButtonRelease:
            case QEvent::MouseMove:
            case QEvent::HoverMove:
            case QEvent::Leave:
                QApplication::sendEvent(&m_window, event);
                break;
            default:
                break;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QMenuBar& m_bar;
    QWidget& m_window;
};

}  // namespace

UiBuilder::UiBuilder(SARibbonMainWindow& window, const ActionRegistry& registry)
    : m_window(window), m_registry(registry) {
    // Window association keeps shortcuts alive even for menu-only actions
    // while the menu bar is replaced or hidden inside the application menu.
    m_window.addActions(m_registry.actions());
}

void UiBuilder::show(InterfaceKind kind) {
    if (m_built && kind == m_current) {
        return;
    }
    for (QToolBar* toolbar : m_toolbars) {
        m_window.removeToolBar(toolbar);
        delete toolbar;
    }
    m_toolbars.clear();
    if (m_ribbon != nullptr) {
        // QMainWindow::setMenuBar() hands the corner widgets of the bar it
        // replaces to the new one: the quick access bar of the compact
        // ribbon would take the place of the classic one, and outlive the
        // ribbon it lays out. It goes away with its ribbon.
        if (QWidget* corner = m_ribbon->cornerWidget(Qt::TopLeftCorner); corner != nullptr) {
            m_ribbon->setCornerWidget(nullptr, Qt::TopLeftCorner);
            corner->hide();
        }
    }
    m_ribbon = nullptr;
    m_applicationButton = nullptr;
    m_title = nullptr;
    m_menuBar = nullptr;
    m_panels.clear();
    m_categories.clear();
    m_hiddenByEnvironment.clear();
    if (kind == InterfaceKind::Ribbon) {
        buildRibbon();
    } else {
        buildClassic();
    }
    m_current = kind;
    m_built = true;
    applyEnvironment();
}

void UiBuilder::layoutRestored() {
    m_hiddenByEnvironment.clear();
    applyEnvironment();
}

QByteArray UiBuilder::saveWindowState() {
    // The state saved has the toolbars as the user left them: the ones the
    // environment hid are shown for the moment of saveState(), with no
    // event processed in between, so nothing is drawn.
    m_applying = true;
    for (QToolBar* toolbar : std::as_const(m_hiddenByEnvironment)) {
        toolbar->show();
    }
    QByteArray state = m_window.saveState();
    for (QToolBar* toolbar : std::as_const(m_hiddenByEnvironment)) {
        toolbar->hide();
    }
    m_applying = false;
    return state;
}

void UiBuilder::setEnvironment(const Environment* environment) {
    m_environmentSet = true;
    m_environment = environment != nullptr ? environment->id : QString{};
    m_contextualRibbon = environment != nullptr ? environment->contextualRibbon : QStringList{};
    applyEnvironment();
}

void UiBuilder::showMenus(QMenuBar& bar) const {
    // Qt's menus of ActionSpec::menu, each after the menu it is in: those of
    // the bar, then their submenus. An action of the registry with a menu of
    // its own (the recent files, the variants) is offered as itself.
    QList<QAction*> menus;
    for (QAction* action : bar.actions()) {
        if (action->menu() != nullptr) {
            menus.append(action);
        }
    }
    for (qsizetype i = 0; i < menus.size(); ++i) {
        const QList<QAction*> items = menus.at(i)->menu()->actions();
        for (QAction* item : items) {
            if (item->menu() != nullptr && !isRegistered(m_registry, item)) {
                menus.append(item);
            }
        }
    }
    // From the last: a submenu is decided before the menu it is in, which is
    // shown while it has something to offer.
    for (QAction* menu : std::views::reverse(menus)) {
        const QList<QAction*> items = menu->menu()->actions();
        menu->setVisible(std::ranges::any_of(
            items, [](const QAction* item) { return !item->isSeparator() && item->isVisible(); }));
    }
}

void UiBuilder::applyEnvironment() {
    if (!m_environmentSet) {
        return;
    }
    // The groups of the ribbon and the toolbars that offer something.
    QHash<QString, bool> offeredPlaces;
    for (QAction* action : m_registry.actions()) {
        const ActionSpec* spec = m_registry.spec(action->objectName());
        const bool offered = availableIn(*spec, m_environment);
        action->setVisible(offered);
        if (offered && !action->isSeparator() && !spec->ribbon.isEmpty()) {
            offeredPlaces.insert(spec->ribbon, true);
        }
    }
    if (m_menuBar != nullptr) {
        showMenus(*m_menuBar);
    }
    // A toolbar without actions offered is hidden, and shown again only if
    // the environment hid it: one the user hid stays hidden (QMainWindow's
    // menu of the toolbars, restoreState()).
    m_applying = true;
    for (QToolBar* toolbar : m_toolbars) {
        // The controls of the layers and of the properties go with the
        // actions of their toolbar.
        bool offered = false;
        for (const QAction* action : toolbar->actions()) {
            offered = offered || (isRegistered(m_registry, action) && !action->isSeparator() &&
                                  action->isVisible());
        }
        // A toolbar just made is hidden until Qt shows it, unless it was
        // hidden on purpose: only a toolbar hidden on purpose stays hidden.
        const bool hiddenOnPurpose =
            toolbar->isHidden() && toolbar->testAttribute(Qt::WA_WState_ExplicitShowHide);
        if (!offered && !hiddenOnPurpose) {
            toolbar->hide();
            m_hiddenByEnvironment.insert(toolbar);
        } else if (offered && m_hiddenByEnvironment.remove(toolbar)) {
            toolbar->show();
        }
    }
    m_applying = false;
    if (m_ribbon == nullptr) {
        return;
    }
    for (auto panel = m_panels.cbegin(); panel != m_panels.cend(); ++panel) {
        panel.value()->setVisible(offeredPlaces.value(panel.key(), false));
    }
    // The tab chosen, to choose it again once the tabs change.
    SARibbonCategory* chosen = nullptr;
    for (const auto& [name, category] : m_categories) {
        if (m_ribbon->isCategoryVisible(category) &&
            m_ribbon->categoryIndex(category) == m_ribbon->currentIndex()) {
            chosen = category;
        }
    }
    const QStringList fixed = fixedTabs();
    QList<SARibbonCategory*> shown;
    QList<SARibbonCategory*> hidden;
    for (const auto& [name, category] : m_categories) {
        bool offered = false;
        if (fixed.contains(name) || m_contextualRibbon.contains(name)) {
            for (auto panel = m_panels.cbegin(); panel != m_panels.cend(); ++panel) {
                offered = offered || (panel.key().section(QLatin1Char('/'), 0, 0) == name &&
                                      offeredPlaces.value(panel.key(), false));
            }
        }
        if (offered != m_ribbon->isCategoryVisible(category)) {
            (offered ? shown : hidden).append(category);
        }
    }
    if (!hidden.isEmpty()) {
        m_ribbon->setCategoriesVisible(hidden, false);
    }
    if (!shown.isEmpty()) {
        m_ribbon->setCategoriesVisible(shown, true);
    }
    if (chosen == nullptr || !m_ribbon->isCategoryVisible(chosen)) {
        // The first tab shown, if the one chosen went away.
        chosen = nullptr;
        for (const auto& [name, category] : m_categories) {
            if (chosen == nullptr && m_ribbon->isCategoryVisible(category)) {
                chosen = category;
            }
        }
    }
    if (chosen != nullptr && m_ribbon->categoryIndex(chosen) != m_ribbon->currentIndex()) {
        m_ribbon->raiseCategory(chosen);
    }
}

InterfaceKind UiBuilder::current() const {
    return m_current;
}

void UiBuilder::setTheme(Theme theme) {
    m_theme = theme;
    styleRibbon();
}

void UiBuilder::updateTitle() {
    if (m_title == nullptr) {
        return;
    }
    // As the platform does with a window title: [*] marks a modified drawing.
    QString text = m_window.windowTitle();
    text.replace(QStringLiteral("[*]"),
                 m_window.isWindowModified() ? QStringLiteral("*") : QString{});
    m_title->setText(text);
}

void UiBuilder::styleRibbon() {
    if (m_ribbon == nullptr || m_applicationButton == nullptr) {
        return;
    }
    const ThemeTokens& tokens = themeTokens(m_theme);
    // The title row and the row of the tabs on the surface of the title,
    // the pages of the tabs under them on that of the ribbon (docs/adr/0039,
    // point 1, areas 1-3, and point 2).
    m_ribbon->setStyleSheet(
        QStringLiteral("SARibbonBar { background: %1; } SARibbonStackedWidget { background: %2; }")
            .arg(tokens.chrome.name(QColor::HexRgb), tokens.ribbon.name(QColor::HexRgb)));
    // The whole File tab in the accent colour, without the arrow of its
    // menu (area 2). A style sheet of the button's own wins over the
    // application's, which keeps buttons transparent.
    m_applicationButton->setStyleSheet(
        QStringLiteral("QToolButton { background: %1; color: %2; border: none; "
                       "border-radius: 0px; padding: 0px 12px; }"
                       "QToolButton::menu-indicator { image: none; width: 0px; }")
            .arg(tokens.accent.name(QColor::HexRgb), lightText(tokens).name(QColor::HexRgb)));
    // File and the tabs share their top edge and their height (docs/tappe/
    // 0.7.5/P40, point 2). The loose style gives both the height of the row
    // of the tabs. The compact one gives File the height of its single row
    // less kCompactRowInset, and the tabs the height of their own row,
    // aligned to its bottom: the row of the tabs takes the height of File.
    if (!m_ribbon->isLooseStyle()) {
        m_ribbon->setTabBarHeight(m_ribbon->titleBarHeight() - kCompactRowInset);
    }
}

void UiBuilder::buildRibbon() {
    auto* ribbon = new SARibbonBar(&m_window);
    ribbon->setObjectName(QStringLiteral("ribbonBar"));
    // Three rows of small buttons in a group (docs/adr/0039, point 3). A
    // window that draws its own frame has a title row of its own, with the
    // icon and the quick access bar on the left, the title in the middle and
    // the window buttons on the right, and the tabs right under it (areas 1
    // and 2): SARibbon's loose style. Its compact style would put the quick
    // access bar at the right of the title (SARibbonBarLayout::
    // resizeInCompactStyle). With the frame of the system the title row is
    // the system's, and the loose one would be the empty band above the
    // tabs: there the compact style keeps the tabs in the first row (point
    // 4), and the quick access bar goes before File (below).
    const bool ownFrame = m_window.windowButtonBar() != nullptr;
    ribbon->setRibbonStyle(ownFrame ? SARibbonBar::RibbonStyleLooseThreeRow
                                    : SARibbonBar::RibbonStyleCompactThreeRow);
    m_registry.buildMenus(ribbon);
    m_menuBar = ribbon;
    auto* application = new QMenu(ribbon);
    application->setObjectName(QStringLiteral("applicationMenu"));
    // Keep the classic menu actions on the bar as well: the application
    // popup reuses their menus, including the window-owned Recent Files.
    QMenu* classic = application->addMenu(QObject::tr("Classic Menus"));
    for (QAction* menuAction : ribbon->actions()) {
        QMenu* menu = menuAction->menu();
        if (menu == nullptr) {
            continue;
        }
        classic->addMenu(menu);
        if (menu->actions().contains(m_registry.action(QStringLiteral("file.new")))) {
            application->addActions(menu->actions());
        }
    }
    auto* button = new SARibbonApplicationButton(QObject::tr("File"), ribbon);
    button->setObjectName(QStringLiteral("applicationButton"));
    button->setMenu(application);
    ribbon->setApplicationButton(button);
    m_ribbon = ribbon;
    m_applicationButton = button;
    styleRibbon();
    if (ownFrame) {
        fillQuickAccess(*ribbon->quickAccessBar(), m_registry);
    } else {
        // The compact style would put SARibbon's quick access bar at the
        // right of the tabs; the one of the classic menus takes its place, at
        // the top left, as the corner widget that the compact style lays out
        // first, before File (docs/adr/0039, point 1, area 1; docs/tappe/
        // 0.7.5/P40, point 1).
        ribbon->quickAccessBar()->hide();
        auto* quickAccess = new QToolBar(ribbon);
        quickAccess->setObjectName(QStringLiteral("quickAccessBar"));
        // On the surface of the row, without the box of the toolbars.
        quickAccess->setStyleSheet(
            QStringLiteral("QToolBar { background: transparent; border: none; padding: 0px; }"));
        fillQuickAccess(*quickAccess, m_registry);
        ribbon->setCornerWidget(quickAccess, Qt::TopLeftCorner);
        quickAccess->installEventFilter(new CornerKeeper(*ribbon, *quickAccess));
    }

    // Tabs are made in the order of their first group: the fixed ones of
    // groups() first, the contextual ones of the environments after them
    // (docs/adr/0039, point 5).
    QHash<QString, SARibbonCategory*> categories;
    const auto panelFor = [&](const QString& path) {
        SARibbonPanel* panel = m_panels.value(path, nullptr);
        if (panel == nullptr) {
            const QString categoryName = path.section(QLatin1Char('/'), 0, 0);
            SARibbonCategory* category = categories.value(categoryName, nullptr);
            if (category == nullptr) {
                category = ribbon->addCategoryPage(title(categoryName));
                category->setObjectName(categoryName);
                categories.insert(categoryName, category);
                m_categories.append({categoryName, category});
            }
            panel = category->addPanel(title(path.section(QLatin1Char('/'), 1)));
            panel->setObjectName(path);
            m_panels.insert(path, panel);
        }
        return panel;
    };
    for (const QString& path : groups()) {
        panelFor(path);
    }
    // The list of the layers and the state of the current one come first in
    // their group; they follow the layers panel (0.6-P22).
    if (auto* layers = m_window.findChild<LayerPanel*>(QStringLiteral("layerPanel"))) {
        panelFor(QStringLiteral("home/layers"))->addSmallWidget(new LayerControls(*layers, ribbon));
    }
    // So do colour, linetype and lineweight in theirs (0.6-P24).
    if (auto* properties =
            m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"))) {
        panelFor(QStringLiteral("home/properties"))
            ->addSmallWidget(new PropertyControls(*properties, ribbon));
    }
    for (QAction* action : toolActions(m_registry)) {
        const ActionSpec* spec = m_registry.spec(action->objectName());
        if (spec->ribbon.isEmpty() || action->isSeparator()) {
            continue;
        }
        SARibbonPanel* panel = panelFor(spec->ribbon);
        if (action->menu() != nullptr) {
            // An action with variants: a large button that runs the one
            // used last, and the menu of all of them in its drop-down
            // (docs/adr/0039, point 1, area 3).
            panel->addLargeAction(action, QToolButton::MenuButtonPopup);
        } else if (spec->ribbon == QStringLiteral("home/draw")) {
            panel->addLargeAction(action);
        } else {
            panel->addSmallAction(action);
        }
    }
    // SARibbonMainWindow disposes of the previous menu widget with
    // deleteLater(), and gives the new ribbon the title row of a window
    // without frame. Actions are owned by the registry, so rebuilding
    // cannot destroy them.
    m_window.setRibbonBar(ribbon);
}

void UiBuilder::buildClassic() {
    auto* bar = new QMenuBar(&m_window);
    bar->setObjectName(QStringLiteral("classicMenuBar"));
    m_registry.buildMenus(bar);
    m_menuBar = bar;
    auto* quickAccess = new QToolBar(bar);
    quickAccess->setObjectName(QStringLiteral("quickAccessBar"));
    fillQuickAccess(*quickAccess, m_registry);
    bar->setCornerWidget(quickAccess, Qt::TopLeftCorner);
    // A window without frame has no title row but this bar: the bar shows
    // the title and leaves room for the window buttons, which stay where
    // the ribbon had them, and moves the window.
    SARibbonSystemButtonBar* buttons = m_window.windowButtonBar();
    if (buttons != nullptr) {
        auto* corner = new QWidget(bar);
        auto* layout = new QHBoxLayout(corner);
        layout->setContentsMargins(0, 0, 0, 0);
        auto* titleLabel = new QLabel(corner);
        titleLabel->setObjectName(QStringLiteral("classicTitle"));
        layout->addWidget(titleLabel);
        layout->addSpacing(buttons->sizeHint().width());
        bar->setCornerWidget(corner, Qt::TopRightCorner);
        bar->setMinimumHeight(buttons->windowTitleHeight());
        bar->installEventFilter(new FrameForwarder(*bar, m_window));
        m_title = titleLabel;
    }
    m_window.setMenuBar(bar);
    if (buttons != nullptr) {
        // The bar is newer than the buttons, so it would cover them.
        buttons->raise();
    }
    updateTitle();

    QHash<QString, QToolBar*> toolbars;
    const auto toolbarFor = [&](const QString& path) {
        QToolBar* toolbar = toolbars.value(path, nullptr);
        if (toolbar == nullptr) {
            toolbar = new QToolBar(path == QStringLiteral("standard")
                                       ? QObject::tr("Standard")
                                       : title(path.section(QLatin1Char('/'), 1)),
                                   &m_window);
            toolbar->setObjectName(path);
            m_window.addToolBar(toolbar);
            m_toolbars.append(toolbar);
            // A toolbar the user shows is the user's again: the environment
            // will not show it a second time.
            QObject::connect(toolbar, &QToolBar::visibilityChanged, toolbar, [this, toolbar] {
                if (!m_applying) {
                    m_hiddenByEnvironment.remove(toolbar);
                }
            });
            toolbars.insert(path, toolbar);
        }
        return toolbar;
    };
    QToolBar* standard = toolbarFor(QStringLiteral("standard"));
    for (const QString& path : groups()) {
        toolbarFor(path);
    }
    if (auto* layers = m_window.findChild<LayerPanel*>(QStringLiteral("layerPanel"))) {
        toolbarFor(QStringLiteral("home/layers"))->addWidget(new LayerControls(*layers, &m_window));
    }
    for (QAction* action : toolActions(m_registry)) {
        const ActionSpec* spec = m_registry.spec(action->objectName());
        if (action->isSeparator()) {
            continue;
        }
        if (spec->id.startsWith(QStringLiteral("file.")) ||
            spec->id == QStringLiteral("edit.undo") || spec->id == QStringLiteral("edit.redo")) {
            standard->addAction(action);
        } else if (!spec->ribbon.isEmpty()) {
            QToolBar* toolbar = toolbarFor(spec->ribbon);
            toolbar->addAction(action);
            // As in the ribbon: a click runs the variant used last, the
            // arrow shows them all.
            auto* button = qobject_cast<QToolButton*>(toolbar->widgetForAction(action));
            if (action->menu() != nullptr && button != nullptr) {
                button->setPopupMode(QToolButton::MenuButtonPopup);
            }
        }
    }
    // The controls of the properties come after the panel's button: a
    // toolbar keeps its first item in sight, and the controls are wide. The
    // toolbar starts a second row, as in AutoCAD, so that the toolbars do not
    // make the window wider than a small screen.
    if (auto* properties =
            m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"))) {
        QToolBar* toolbar = toolbarFor(QStringLiteral("home/properties"));
        toolbar->addWidget(new PropertyControls(*properties, &m_window));
        m_window.insertToolBarBreak(toolbar);
    }
}

}  // namespace tg::app
