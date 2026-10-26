// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

#include <string>
#include <string_view>

namespace tg::cmd {

/// Every completed option belongs to one transaction, including on Esc.
class LayerCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t {
        Option,
        List,
        Make,
        Set,
        New,
        RenameOld,
        RenameNew,
        Names,
        Color,
        Rgb,
        Linetype,
        Lineweight,
        Plot,
        ConfirmOff,
    };
    enum class Operation : std::uint8_t {
        On,
        Off,
        Freeze,
        Thaw,
        Lock,
        Unlock,
        Color,
        Linetype,
        Lineweight,
        Plot,
    };

    void chooseOption(CommandContext& context, std::string_view text);
    void handleName(CommandContext& context, std::string_view text);
    void handleValue(CommandContext& context, std::string_view text);
    void applyNames(CommandContext& context, std::string_view text);
    void list(CommandContext& context, std::string_view text) const;

    Stage m_stage{Stage::Option};
    Operation m_operation{Operation::On};
    std::string m_currentName;
    std::string m_oldName;
    core::LayerId m_renameId;
    core::LayerId m_offId;
    core::Color m_color{core::Color::fromIndex(7)};
    core::LinetypeId m_linetype;
    std::string m_linetypeName{"Continuous"};
    int m_lineweight{core::lineweight::kDefault};
    bool m_plot{true};
};

class ClayerCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    std::string m_currentName;
};

}  // namespace tg::cmd
