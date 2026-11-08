// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "theme.hpp"

#include <QList>
#include <QPointer>

#include <cstdint>

class QLabel;
class QToolBar;
class QToolButton;
class SARibbonBar;
class SARibbonMainWindow;

namespace tg::app {

class ActionRegistry;

enum class InterfaceKind : std::uint8_t { Ribbon, Classic };

/// Builds the interface from the registry without replacing its actions.
/// The window owns the widgets and must outlive this builder and the registry.
///
/// Both interfaces have the quick access bar (the actions with
/// ActionSpec::quickAccess, in the registry's order; docs/adr/0039, point 1,
/// area 1): the ribbon in its own title row, the classic menus at the left
/// of the menu bar. When the window draws its own frame (it has
/// SARibbonMainWindow::windowButtonBar()), the classic menu bar also shows
/// the title and leaves room for the window buttons.
class UiBuilder {
public:
    UiBuilder(SARibbonMainWindow& window, const ActionRegistry& registry);
    void show(InterfaceKind kind);
    [[nodiscard]] InterfaceKind current() const;

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

    SARibbonMainWindow& m_window;
    const ActionRegistry& m_registry;
    QList<QToolBar*> m_toolbars;
    // Children of the bar on show: Qt deletes a replaced menu bar later,
    // so a search by name could find the one being replaced.
    QPointer<SARibbonBar> m_ribbon;
    QPointer<QToolButton> m_applicationButton;
    QPointer<QLabel> m_title;
    InterfaceKind m_current{InterfaceKind::Ribbon};
    Theme m_theme{Theme::Light};
    bool m_built{false};
};

}  // namespace tg::app
