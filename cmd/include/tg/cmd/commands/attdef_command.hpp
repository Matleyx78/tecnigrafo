// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/attribute_definition.hpp"

namespace tg::cmd {

class AttdefCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;

private:
    enum class Stage : std::uint8_t {
        Modes,
        Tag,
        AttributePrompt,
        Value,
        Start,
        Justify,
        Style,
        FirstPoint,
        SecondPoint,
        Height,
        Rotation
    };

    void reportModes(CommandContext& context) const;
    void reportText(CommandContext& context) const;
    void useStyle(const CommandContext& context);
    void afterPoint(const CommandContext& context);
    Status finish(CommandContext& context);

    Stage m_stage{Stage::Modes};
    core::AttributeDefinition m_definition;
    int m_flags{16};
    std::string m_justify{"Left"};
    core::Vec2 m_first;
};

class AflagsCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    int m_value{16};
};

}  // namespace tg::cmd
