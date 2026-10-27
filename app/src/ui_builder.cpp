// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "ui_builder.hpp"

#include "action_registry.hpp"
#include "layer_panel.hpp"
#include "properties_panel.hpp"

#include <QAction>
#include <QHash>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QToolBar>
#include <QToolButton>

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

}  // namespace

UiBuilder::UiBuilder(QMainWindow& window, const ActionRegistry& registry)
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

void UiBuilder::buildRibbon() {
    auto* ribbon = new SARibbonBar(&m_window);
    ribbon->setObjectName(QStringLiteral("ribbonBar"));
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
    auto* button = new QToolButton(ribbon);
    button->setText(QObject::tr("File"));
    button->setMenu(application);
    button->setPopupMode(QToolButton::InstantPopup);
    ribbon->setApplicationButton(button);

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
    // QMainWindow disposes of the previous menu widget with deleteLater().
    // Actions are owned by the registry, so rebuilding cannot destroy them.
    m_window.setMenuBar(ribbon);
}

void UiBuilder::buildClassic() {
    auto* bar = new QMenuBar(&m_window);
    bar->setObjectName(QStringLiteral("classicMenuBar"));
    m_registry.buildMenus(bar);
    m_window.setMenuBar(bar);

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
    // toolbar keeps its first item in sight, and the controls are wide.
    if (auto* properties =
            m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"))) {
        toolbarFor(QStringLiteral("home/properties"))
            ->addWidget(new PropertyControls(*properties, &m_window));
    }
}

}  // namespace tg::app
