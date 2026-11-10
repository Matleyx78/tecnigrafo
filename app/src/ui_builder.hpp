// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "theme.hpp"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QPair>
#include <QSet>
#include <QString>
#include <QStringList>

#include <cstdint>

class QLabel;
class QMenuBar;
class QToolBar;
class QToolButton;
class SARibbonBar;
class SARibbonCategory;
class SARibbonMainWindow;
class SARibbonPanel;

namespace tg::app {

class ActionRegistry;
struct Environment;

enum class InterfaceKind : std::uint8_t { Ribbon, Classic };

/// Builds the interface from the registry without replacing its actions.
/// The window owns the widgets and must outlive this builder and the registry.
///
/// Both interfaces have the quick access bar (the actions with
/// ActionSpec::quickAccess, in the registry's order; docs/adr/0039, point 1,
/// area 1): the ribbon at the left of its title row, or, when the window
/// keeps the frame of the system, a toolbar named quickAccessBar at the left
/// of the row of the tabs, before File, as the ribbon's corner widget
/// Qt::TopLeftCorner (docs/tappe/0.7.5/P40, point 1); the classic menus at
/// the left of the menu bar. File and the tabs of the ribbon have the same
/// top edge and height, within 1 px (point 2). When the window draws its own frame (it has
/// SARibbonMainWindow::windowButtonBar()), the classic menu bar also shows
/// the title and leaves room for the window buttons.
///
/// Both interfaces offer only the actions of the active environment
/// (setEnvironment(), docs/adr/0041, point 6).
class UiBuilder {
public:
    UiBuilder(SARibbonMainWindow& window, const ActionRegistry& registry);
    void show(InterfaceKind kind);
    [[nodiscard]] InterfaceKind current() const;

    /// Offers only the actions of `environment` and the global ones
    /// (availableIn()); nullptr is no environment, the Start page, where only
    /// the global actions are offered. The others are hidden, which in Qt
    /// also disables them and their shortcuts. A menu, a group of the
    /// ribbon, a toolbar or a tab of the ribbon left without actions is
    /// hidden too; a toolbar comes back when its actions do, unless the user
    /// hid it. The tabs of the ribbon are the fixed ones (Home, View)
    /// and, at their right, the contextual tabs of `environment`
    /// (Environment::contextualRibbon, docs/adr/0039, point 5); the tab
    /// chosen stays chosen while it is shown. Nothing is rebuilt, and the
    /// choice holds for the interfaces shown later. Until the first call
    /// every action is offered.
    void setEnvironment(const Environment* environment);

    /// The toolbars have just been shown and hidden as saved
    /// (QMainWindow::restoreState()): that is the user's choice, and the
    /// environment hides again the ones it does not offer.
    void layoutRestored();

    /// The window's QMainWindow::saveState() with the toolbars as the user
    /// left them, as if no environment had hidden any: restoreState() and
    /// layoutRestored() bring them back with any environment.
    [[nodiscard]] QByteArray saveWindowState();

    /// Colours the title row and the File tab of the ribbon with the tokens
    /// of `theme`; kept for the ribbons built later.
    void setTheme(Theme theme);

    /// Shows the window title, with * for a modified drawing, where the
    /// interface draws it itself: the title of the classic menu bar. The
    /// ribbon draws the window's title on its own.
    void updateTitle();

private:
    void buildRibbon();
    void buildClassic();
    void styleRibbon();
    /// Shows and hides the actions, menus, groups, toolbars and tabs for
    /// the environment of setEnvironment().
    void applyEnvironment();
    /// Shows the menus and submenus of `bar` that have an action offered,
    /// and hides the others.
    void showMenus(QMenuBar& bar) const;

    SARibbonMainWindow& m_window;
    const ActionRegistry& m_registry;
    QList<QToolBar*> m_toolbars;
    // Widgets of the bar on show, or nullptr. Kept rather than searched for
    // by name, because Qt deletes a replaced menu bar later and a search
    // could find the old one; show() forgets them before replacing the bar.
    SARibbonBar* m_ribbon{nullptr};
    QToolButton* m_applicationButton{nullptr};
    QLabel* m_title{nullptr};
    // The bar of the menus on show (the ribbon is one), its groups by their
    // place ("home/draw") and its tabs by their name ("home"), in order.
    QMenuBar* m_menuBar{nullptr};
    QHash<QString, SARibbonPanel*> m_panels;
    QList<QPair<QString, SARibbonCategory*>> m_categories;
    // The environment of setEnvironment(): its id, empty for none, and its
    // contextual tabs; m_environmentSet stays false until the first call.
    bool m_environmentSet{false};
    QString m_environment;
    QStringList m_contextualRibbon;
    // The toolbars the environment hid, to show again when it offers them;
    // a toolbar the user hid is not among them. m_applying is true while
    // the builder itself shows and hides them.
    QSet<QToolBar*> m_hiddenByEnvironment;
    bool m_applying{false};
    InterfaceKind m_current{InterfaceKind::Ribbon};
    Theme m_theme{Theme::Light};
    bool m_built{false};
};

}  // namespace tg::app
