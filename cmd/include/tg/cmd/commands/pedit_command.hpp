// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/pedit_join.hpp"

namespace tg::cmd {

class PeditCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    enum class Stage : std::uint8_t {
        Source,
        Selection,
        Conversion,
        Options,
        Join,
        Fuzz,
        Type,
        Ltype
    };

    struct Option {
        edit::Editor::Savepoint mark;
        std::vector<core::EntityId> ids;
    };

    Status selected(CommandContext& context);
    Status convert(CommandContext& context, bool accept);
    Status finish(CommandContext& context);
    void remember(CommandContext& context);
    void joinSelected(CommandContext& context, double fuzz);
    void updateOptions(const CommandContext& context);

    Stage m_stage{Stage::Source};
    bool m_multiple{false};
    bool m_closed{false};
    bool m_ltype{false};
    core::JoinType m_joinType{core::JoinType::Extend};
    std::vector<core::EntityId> m_ids;
    std::vector<core::EntityId> m_others;
    std::vector<Option> m_options;
};

class PeditacceptCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    bool m_value{false};
};

}  // namespace tg::cmd
