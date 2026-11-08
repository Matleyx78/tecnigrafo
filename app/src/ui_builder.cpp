// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "ui_builder.hpp"

#include "action_registry.hpp"
#include "layer_panel.hpp"
#include "properties_panel.hpp"

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QObject>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QToolBar>
#include <QToolButton>
#include <QWidget>

#include <SARibbon.h>

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
    m_ribbon.clear();
    m_applicationButton.clear();
    m_title.clear();
    if (kind == InterfaceKind::Ribbon) {
        buildRibbon();
    } else {
        buildClassic();
    }
    m_current = kind;
    m_built = true;
}

InterfaceKind UiBuilder::current() const {
    return m_current;
}

void UiBuilder::setTheme(Theme theme) {
    m_theme = theme;
    styleRibbon();
}

void UiBuilder::updateTitle() {
    if (m_title.isNull()) {
        return;
    }
    // As the platform does with a window title: [*] marks a modified drawing.
    QString text = m_window.windowTitle();
    text.replace(QStringLiteral("[*]"),
                 m_window.isWindowModified() ? QStringLiteral("*") : QString{});
    m_title->setText(text);
}

void UiBuilder::styleRibbon() {
    if (m_ribbon.isNull() || m_applicationButton.isNull()) {
        return;
    }
    const ThemeTokens& tokens = themeTokens(m_theme);
    // In the compact style the title row is the row of the tabs: all of it
    // on the surface of the title, the pages of the tabs under it on that of
    // the ribbon (docs/adr/0039, point 1, areas 1-3, and point 2).
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
}

void UiBuilder::buildRibbon() {
    auto* ribbon = new SARibbonBar(&m_window);
    ribbon->setObjectName(QStringLiteral("ribbonBar"));
    // The compact style puts the tabs in the title row: no empty band above
    // them (docs/adr/0039, point 1, areas 1 and 2); three rows of small
    // buttons in a group (point 3).
    ribbon->setRibbonStyle(SARibbonBar::RibbonStyleCompactThreeRow);
    m_registry.buildMenus(ribbon);
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
    fillQuickAccess(*ribbon->quickAccessBar(), m_registry);

    QHash<QString, SARibbonCategory*> categories;
    QHash<QString, SARibbonPanel*> panels;
    const auto panelFor = [&](const QString& path) {
        SARibbonPanel* panel = panels.value(path, nullptr);
        if (panel == nullptr) {
            const QString categoryName = path.section(QLatin1Char('/'), 0, 0);
            SARibbonCategory* category = categories.value(categoryName, nullptr);
            if (category == nullptr) {
                category = ribbon->addCategoryPage(title(categoryName));
                categories.insert(categoryName, category);
            }
            panel = category->addPanel(title(path.section(QLatin1Char('/'), 1)));
            panel->setObjectName(path);
            panels.insert(path, panel);
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
    for (QAction* action : m_registry.actions()) {
        const ActionSpec* spec = m_registry.spec(action->objectName());
        if (spec->ribbon.isEmpty() || action->isSeparator()) {
            continue;
        }
        SARibbonPanel* panel = panelFor(spec->ribbon);
        if (spec->ribbon == QStringLiteral("home/draw")) {
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
    for (QAction* action : m_registry.actions()) {
        const ActionSpec* spec = m_registry.spec(action->objectName());
        if (action->isSeparator()) {
            continue;
        }
        if (spec->id.startsWith(QStringLiteral("file.")) ||
            spec->id == QStringLiteral("edit.undo") || spec->id == QStringLiteral("edit.redo")) {
            standard->addAction(action);
        } else if (!spec->ribbon.isEmpty()) {
            toolbarFor(spec->ribbon)->addAction(action);
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
