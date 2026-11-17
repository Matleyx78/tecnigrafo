// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

#include <cstdint>
#include <string>

namespace tg::cmd {

class BlockCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t { Name, Redefine, Base, Selection };

    static void listBlocks(CommandContext& context);

    Stage m_stage{Stage::Name};
    std::string m_name;
    core::Vec2 m_base;
    bool m_redefining{false};
};

}  // namespace tg::cmd
