// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/join.hpp"

namespace tg::cmd {

class JoinCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    enum class Stage : std::uint8_t { Interactive, Source, Lines, Arcs, Objects, Circle };

    Status joinSelected(CommandContext& context, bool close = false);
    Status apply(CommandContext& context, core::JoinResult result);
    [[nodiscard]] std::string successMessage(const core::JoinResult& result,
                                             const core::Entity& source) const;

    Stage m_stage{Stage::Interactive};
    bool m_script{false};
    std::vector<core::EntityId> m_ids;
    std::optional<core::JoinResult> m_pending;
};

}  // namespace tg::cmd
