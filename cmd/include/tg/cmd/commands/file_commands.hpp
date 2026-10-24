// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"

namespace tg::cmd {

/// SAVE writes a copy unless the chosen name is the current drawing's name.
class SaveCommand : public Command {
public:
    SaveCommand() = default;
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

protected:
    enum class Mode : std::uint8_t { Save, Saveas, Qsave };

    explicit SaveCommand(Mode mode) : m_mode(mode) {}

private:
    enum class Stage : std::uint8_t { Name, Replace, Done };
    Status chooseName(CommandContext& context, std::string name);
    Status save(CommandContext& context);
    Status fail(CommandContext& context, const std::string& reason);

    Mode m_mode{Mode::Save};
    Stage m_stage{Stage::Name};
    std::string m_defaultName;
    std::string m_name;
    bool m_dxf{false};
};

class QsaveCommand final : public SaveCommand {
public:
    QsaveCommand() : SaveCommand(Mode::Qsave) {}
};

class SaveasCommand final : public SaveCommand {
public:
    SaveasCommand() : SaveCommand(Mode::Saveas) {}
};

/// INSUNITS changes the document's units in one undoable transaction.
class InsunitsCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    int m_units{core::Document::kDefaultUnits};
};

}  // namespace tg::cmd
