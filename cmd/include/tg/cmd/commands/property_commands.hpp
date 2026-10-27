// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

namespace tg::cmd {

class ColorCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    std::string m_color;
    bool m_rgb{false};
};

class LinetypeCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t { Option, Load, Set };
    static void load(CommandContext& context, std::string_view names);
    static void list(CommandContext& context);
    Stage m_stage{Stage::Option};
    std::string m_currentName;
};

/// Both scale variables share validation, but retain their own metadata and undo name.
class ScaleCommand final : public Command {
public:
    enum class Variable : std::uint8_t { Global, Current };

    explicit ScaleCommand(Variable variable) : m_variable(variable) {}

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description(Variable variable);

private:
    Variable m_variable;
    double m_scale{1.0};
};

class LweightCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();
};

class LwdisplayCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    bool m_on{false};
};

}  // namespace tg::cmd
