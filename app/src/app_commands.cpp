// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "app_commands.hpp"

#include "layer_panel.hpp"
#include "main_window.hpp"
#include "properties_panel.hpp"
#include "tg/cmd/command.hpp"
#include "tg/cmd/command_registry.hpp"
#include "ui_builder.hpp"

#include <QString>

#include <memory>
#include <span>

namespace tg::app {
namespace {

class InterfaceCommand final : public cmd::Command {
public:
    InterfaceCommand(MainWindow& window, InterfaceKind kind) : m_window(window), m_kind(kind) {}

    void start(cmd::CommandContext& /*context*/) override { m_window.setInterface(m_kind); }

    cmd::Status handle(cmd::CommandContext& /*context*/, const cmd::Input& /*input*/) override {
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    MainWindow& m_window;
    InterfaceKind m_kind;
};

/// LAYER shows the layers panel and gives it the keyboard: no message and no
/// question (docs/tappe/0.6/P22).
class LayerPanelCommand final : public cmd::Command {
public:
    explicit LayerPanelCommand(MainWindow& window) : m_window(window) {}

    void start(cmd::CommandContext& /*context*/) override {
        if (auto* panel = m_window.findChild<LayerPanel*>(QStringLiteral("layerPanel"))) {
            panel->showAndFocus();
        }
    }

    cmd::Status handle(cmd::CommandContext& /*context*/, const cmd::Input& /*input*/) override {
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    MainWindow& m_window;
};

/// PROPERTIES shows the properties panel and gives it the keyboard: no
/// message and no question (docs/tappe/0.6/P24).
class PropertiesPanelCommand final : public cmd::Command {
public:
    explicit PropertiesPanelCommand(MainWindow& window) : m_window(window) {}

    void start(cmd::CommandContext& /*context*/) override {
        if (auto* panel = m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"))) {
            panel->showAndFocus();
        }
    }

    cmd::Status handle(cmd::CommandContext& /*context*/, const cmd::Input& /*input*/) override {
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    MainWindow& m_window;
};

/// QSELECT opens the window of the quick selection, with no question in the
/// command line. Starting it cleared the selection (docs/adr/0008): the
/// window gets the one the panel saw before, to search within it.
class QuickSelectCommand final : public cmd::Command {
public:
    explicit QuickSelectCommand(MainWindow& window) : m_window(window) {}

    void start(cmd::CommandContext& /*context*/) override {
        if (auto* panel = m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"))) {
            panel->showQuickSelect(panel->idleSelection());
        }
    }

    cmd::Status handle(cmd::CommandContext& /*context*/, const cmd::Input& /*input*/) override {
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    MainWindow& m_window;
};

}  // namespace

void registerAppCommands(cmd::CommandRegistry& registry, MainWindow& window) {
    registry.add({"RIBBON", {}, "Shows the ribbon interface.", false, {}}, [&window] {
        return std::make_unique<InterfaceCommand>(window, InterfaceKind::Ribbon);
    });
    const cmd::CommandDescription close{
        "RIBBONCLOSE", {}, "Shows the classic menus and toolbars.", false, {}};
    registry.add(close, [&window] {
        return std::make_unique<InterfaceCommand>(window, InterfaceKind::Classic);
    });
    registry.add({"LAYER", {"LA"}, "Shows the layers panel.", false, {}},
                 [&window] { return std::make_unique<LayerPanelCommand>(window); });
    const cmd::CommandDescription properties{
        "PROPERTIES", {"PR", "CH", "MO", "PROPS"}, "Shows the properties panel.", false, {}};
    registry.add(properties,
                 [&window] { return std::make_unique<PropertiesPanelCommand>(window); });
    const cmd::CommandDescription quickSelect{
        "QSELECT", {}, "Selects objects by type and property in a window.", false, {}};
    registry.add(quickSelect, [&window] { return std::make_unique<QuickSelectCommand>(window); });
}

}  // namespace tg::app
