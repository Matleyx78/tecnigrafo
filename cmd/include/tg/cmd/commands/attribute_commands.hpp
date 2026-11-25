// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/drawing_settings.hpp"

#include <cstdint>
#include <span>
#include <string>

namespace tg::cmd {

class AttsyncCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t { Option, Name, Select, Finished };

    static void listBlocks(CommandContext& context);
    Status synchronize(CommandContext& context, std::string_view name);

    Stage m_stage{Stage::Option};
};

class AttdispCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    core::AttributeDisplay m_display{core::AttributeDisplay::Normal};
};

}  // namespace tg::cmd
