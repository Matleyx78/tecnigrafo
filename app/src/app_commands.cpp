// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "app_commands.hpp"

#include "layer_panel.hpp"
#include "main_window.hpp"
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
}

}  // namespace tg::app
