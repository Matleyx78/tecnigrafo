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
#include <QIcon>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMetaObject>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include <QRect>
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
    if (name == QStringLiteral("annotation")) {
        return QObject::tr("Annotation");
    }
    if (name == QStringLiteral("layers")) {
        return QObject::tr("Layers");
    }
    if (name == QStringLiteral("properties")) {
        return QObject::tr("Properties");
    }
    if (name == QStringLiteral("utilities")) {
        return QObject::tr("Utilities");
    }
    if (name == QStringLiteral("navigate")) {
        return QObject::tr("Navigate");
    }
    if (name == QStringLiteral("aids")) {
        return QObject::tr("Drawing Aids");
    }
    return name;
}

/// The groups of the ribbon in their order: those of the Home tab as in
/// docs/adr/0039, point 1, area 3.
QStringList groups() {
    return {QStringLiteral("home/draw"),       QStringLiteral("home/modify"),
            QStringLiteral("home/annotation"), QStringLiteral("home/layers"),
            QStringLiteral("home/properties"), QStringLiteral("home/utilities"),
            QStringLiteral("view/navigate"),   QStringLiteral("view/aids")};
}

/// The groups whose buttons are all large: icon of 32 px and text under it
/// (docs/adr/0039, point 1, area 3).
bool isLargeGroup(const QString& path) {
    return path == QStringLiteral("home/draw") || path == QStringLiteral("home/annotation") ||
           path == QStringLiteral("home/utilities");
}

/// The actions that open a panel: the large button that comes first in its
/// group, before the controls of the group (area 3: Layer Properties and
/// Properties in the reference).
QStringList panelActions() {
    return {QStringLiteral("layer.panel"), QStringLiteral("properties.panel")};
}

/// The small buttons of Modify, column by column, three to a column (area
/// 3); the ones not listed follow in the order of the registry.
QStringList modifyOrder() {
    return {QStringLiteral("edit.move"),    QStringLiteral("edit.copy"),
            QStringLiteral("edit.rotate"),  QStringLiteral("edit.mirror"),
            QStringLiteral("edit.scale"),   QStringLiteral("edit.offset"),
            QStringLiteral("edit.trim"),    QStringLiteral("edit.extend"),
            QStringLiteral("edit.join"),    QStringLiteral("edit.fillet"),
            QStringLiteral("edit.chamfer"), QStringLiteral("edit.explode"),
            QStringLiteral("edit.erase")};
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

/// The actions of `registry` in the order of the ribbon and the toolbars:
/// those of modifyOrder() in that order, where the first of them is, the
/// others as registered.
QList<QAction*> toolActions(const ActionRegistry& registry) {
    QList<QAction*> actions = registry.actions();
    QList<QAction*> modify;
    for (const QString& id : modifyOrder()) {
        if (QAction* action = registry.action(id); action != nullptr) {
            modify.append(action);
        }
    }
    const auto first = std::ranges::find_first_of(actions, modify);
    const qsizetype at = first - actions.begin();
    for (QAction* action : modify) {
        actions.removeOne(action);
    }
    const qsizetype place = std::min(at, actions.size());
    for (qsizetype i = 0; i < modify.size(); ++i) {
        actions.insert(place + i, modify.at(i));
    }
    return actions;
}

/// The small icons of the quick access bar: the 16 px grid of docs/adr/0040,
/// point 3 (docs/adr/0039, point 1, area 1).
constexpr int kQuickAccessIconSize = 16;
/// The logo of the program fills the title row less one pixel above and one
/// below, as SARibbon's own title icon.
constexpr int kLogoInset = 2;

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

/// Gives a quick access bar that is the corner widget of a menu bar the size
/// of its contents. QMenuBar places its corner widgets again whenever the
/// layout of the window asks its height (QMenuBar::heightForWidth()), at
/// their size hint, and the compact ribbon places its own at the height of
/// its row (SARibbonBarLayout::resizeInCompactStyle()): each resize made the
/// bar show its buttons, which asked both layouts again, and this did not
/// end (a stack overflow with the frame of the system, the crash of the
/// manual test of 0.7.5-P40). With a fixed size neither resizes it, and
/// placing it again changes nothing. Hidden actions take no room (QToolBar),
/// so the size follows them.
void fitQuickAccess(QToolBar& bar) {
    bar.ensurePolished();
    bar.setFixedSize(bar.sizeHint());
}

/// Icons of the buttons of the ribbon: small on the 16 px grid, large on the
/// 32 px one (docs/adr/0039, point 3; docs/adr/0040, point 3).
constexpr int kSmallIconSize = 16;
constexpr int kLargeIconSize = 32;
/// The band of the names of the groups under them (area 3).
constexpr int kPanelTitleHeight = 22;
/// The page of a tab of the ribbon, with that band: 96 px in the reference
/// (area 3), room for an icon of 32 px with two lines of text under it.
constexpr int kCategoryHeight = 96;
/// Space between the buttons of a group, and so between its columns of
/// small buttons (area 3).
constexpr int kButtonSpacing = 6;

/// The actions of the second row of the Layers group, under the list of the
/// layers (area 3).
QStringList layerRowActions() {
    return {QStringLiteral("layer.new"), QStringLiteral("layer.delete"),
            QStringLiteral("layer.current")};
}

QToolButton* rowButton(QAction* action, Qt::ToolButtonStyle style, QWidget* parent) {
    auto* button = new QToolButton(parent);
    button->setDefaultAction(action);
    button->setToolButtonStyle(style);
    button->setIconSize(QSize(kSmallIconSize, kSmallIconSize));
    button->setAutoRaise(true);
    // A click must not take the keyboard away from the drawing and the
    // command line.
    button->setFocusPolicy(Qt::NoFocus);
    return button;
}

/// The second row of the Layers group as in the reference (area 3): New and
/// Delete as icons, with their name in the tooltip, then Make Current with
/// its text. Nullptr if the registry has none of them.
QWidget* layerButtonRow(const ActionRegistry& registry, QWidget* parent) {
    QAction* add = registry.action(QStringLiteral("layer.new"));
    QAction* remove = registry.action(QStringLiteral("layer.delete"));
    QAction* current = registry.action(QStringLiteral("layer.current"));
    if (add == nullptr && remove == nullptr && current == nullptr) {
        return nullptr;
    }
    auto* row = new QWidget(parent);
    row->setObjectName(QStringLiteral("layerButtonRow"));
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    for (QAction* action : {add, remove}) {
        if (action != nullptr) {
            layout->addWidget(rowButton(action, Qt::ToolButtonIconOnly, row));
        }
    }
    if (current != nullptr) {
        if (layout->count() > 0) {
            layout->addWidget(new SARibbonSeparatorWidget(row));
        }
        layout->addWidget(rowButton(current, Qt::ToolButtonTextBesideIcon, row));
    }
    layout->addStretch(1);
    return row;
}

/// The arrow at the corner of a group that opens its panel (area 3). SARibbon
/// gives its button, and the icon, a square as high as the band of the
/// names; the arrow, on the 16 px grid, goes in the middle of it, not
/// stretched to the square.
QIcon launcherIcon(Theme theme) {
    const QIcon arrow = themedIcon(QStringLiteral("ui-panel-launcher"), theme);
    const int offset = (kPanelTitleHeight - kSmallIconSize) / 2;
    QIcon icon;
    for (const qreal ratio : {1.0, 2.0}) {
        QPixmap pixmap(QSize(kPanelTitleHeight, kPanelTitleHeight) * ratio);
        pixmap.setDevicePixelRatio(ratio);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        arrow.paint(&painter, QRect(offset, offset, kSmallIconSize, kSmallIconSize));
        painter.end();
        icon.addPixmap(pixmap);
    }
    return icon;
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
/// The bar is a child of the ribbon, and the keeper a child of the bar, so
/// the keeper never outlives its ribbon. UiBuilder::show() takes the bar off
/// a ribbon it replaces before QMainWindow::setMenuBar() could hand it to
/// the classic menus; if it ever had another parent, the keeper would lay
/// nothing out.
class CornerKeeper final : public QObject {
public:
    CornerKeeper(SARibbonBar& ribbon, QWidget& corner)
        : QObject(&corner), m_ribbon(ribbon), m_corner(corner) {}

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
                            // Only the parent is compared: no other ribbon
                            // is reached.
                            if (m_corner.parentWidget() != &m_ribbon) {
                                return;
                            }
                            if (QLayout* layout = m_ribbon.layout(); layout != nullptr) {
                                layout->setGeometry(m_ribbon.rect());
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
    SARibbonBar& m_ribbon;
    QWidget& m_corner;
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
    m_cornerQuickAccess = nullptr;
    m_menuBar = nullptr;
    m_panels.clear();
    m_categories.clear();
    m_launchers.clear();
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
    if (m_cornerQuickAccess != nullptr) {
        fitQuickAccess(*m_cornerQuickAccess);
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
    // With every tab hidden (the Start page), the first tab shown again
    // becomes the current one before SARibbon links it to its page, and the
    // page last shown stays under it: Home chosen over the page of View.
    SARibbonStackedWidget* pages = m_ribbon->ribbonStackedWidget();
    if (chosen != nullptr && pages->currentWidget() != chosen) {
        pages->setCurrentWidget(chosen);
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
    for (QAction* launcher : std::as_const(m_launchers)) {
        launcher->setIcon(launcherIcon(m_theme));
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
    ribbon->setPanelToolButtonIconSize(QSize(kSmallIconSize, kSmallIconSize),
                                       QSize(kLargeIconSize, kLargeIconSize));
    ribbon->setPanelTitleHeight(kPanelTitleHeight);
    ribbon->setPanelSpacing(kButtonSpacing);
    ribbon->setCategoryHeight(kCategoryHeight);
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
        // The logo before the quick access bar, as in the reference: the
        // compact style lays out the corner widget before its own title
        // icon, which would put the logo between the bar and File.
        ribbon->setTitleIconVisible(false);
        auto* logo = new SARibbonTitleIconWidget(quickAccess);
        logo->setObjectName(QStringLiteral("programLogo"));
        logo->setIcon(programIcon());
        const int logoSide = ribbon->titleBarHeight() - kLogoInset;
        logo->setFixedSize(logoSide, logoSide);
        quickAccess->addWidget(logo);
        fillQuickAccess(*quickAccess, m_registry);
        fitQuickAccess(*quickAccess);
        m_cornerQuickAccess = quickAccess;
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
    // The button of a panel comes first in its group, large, and the arrow
    // at the corner of the group opens the panel too (area 3).
    for (const QString& id : panelActions()) {
        QAction* action = m_registry.action(id);
        const ActionSpec* spec = m_registry.spec(id);
        if (action != nullptr && spec != nullptr && !spec->ribbon.isEmpty()) {
            SARibbonPanel* panel = panelFor(spec->ribbon);
            panel->addLargeAction(action);
            auto* launcher = new QAction(launcherIcon(m_theme), QString{}, panel);
            launcher->setToolTip(action->toolTip());
            QObject::connect(launcher, &QAction::triggered, action, &QAction::trigger);
            panel->setOptionAction(launcher);
            m_launchers.append(launcher);
        }
    }
    // The list of the layers and the state of the current one follow the
    // button of the layers panel (0.6-P22), with New, Delete and Make
    // Current under them (area 3).
    if (auto* layers = m_window.findChild<LayerPanel*>(QStringLiteral("layerPanel"))) {
        SARibbonPanel* panel = panelFor(QStringLiteral("home/layers"));
        panel->addSmallWidget(new LayerControls(*layers, ribbon));
        if (QWidget* row = layerButtonRow(m_registry, ribbon)) {
            panel->addSmallWidget(row);
        }
    }
    // Colour, linetype and lineweight, one under the other, follow that of
    // the properties panel (0.6-P24; area 3).
    if (auto* properties =
            m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"))) {
        panelFor(QStringLiteral("home/properties"))
            ->addLargeWidget(new PropertyControls(*properties, Qt::Vertical, ribbon));
    }
    for (QAction* action : toolActions(m_registry)) {
        const ActionSpec* spec = m_registry.spec(action->objectName());
        if (spec->ribbon.isEmpty() || action->isSeparator() || panelActions().contains(spec->id) ||
            layerRowActions().contains(spec->id)) {
            continue;
        }
        SARibbonPanel* panel = panelFor(spec->ribbon);
        if (action->menu() != nullptr) {
            // An action with variants: a large button that runs the one
            // used last, and the menu of all of them in its drop-down
            // (docs/adr/0039, point 1, area 3).
            panel->addLargeAction(action, QToolButton::MenuButtonPopup);
        } else if (isLargeGroup(spec->ribbon)) {
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
    fitQuickAccess(*quickAccess);
    m_cornerQuickAccess = quickAccess;
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
        toolbar->addWidget(new PropertyControls(*properties, Qt::Horizontal, &m_window));
        m_window.insertToolBarBreak(toolbar);
    }
}

}  // namespace tg::app
