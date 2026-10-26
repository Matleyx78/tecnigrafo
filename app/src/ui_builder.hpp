// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QList>

#include <cstdint>

class QMainWindow;
class QToolBar;

namespace tg::app {

class ActionRegistry;

enum class InterfaceKind : std::uint8_t { Ribbon, Classic };

/// Builds the interface from the registry without replacing its actions.
/// The window owns the widgets and must outlive this builder and the registry.
class UiBuilder {
public:
    UiBuilder(QMainWindow& window, const ActionRegistry& registry);
    void show(InterfaceKind kind);
    [[nodiscard]] InterfaceKind current() const;

private:
    void buildRibbon();
    void buildClassic();

    QMainWindow& m_window;
    const ActionRegistry& m_registry;
    QList<QToolBar*> m_toolbars;
    InterfaceKind m_current{InterfaceKind::Ribbon};
    bool m_built{false};
};

}  // namespace tg::app
