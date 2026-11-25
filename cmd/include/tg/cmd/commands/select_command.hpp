// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

namespace tg::cmd {

/// SELECT (docs/tappe/0.8.1/PT34-last-previous.md, point 3): one question
/// for objects, asked by CommandRunner. The objects chosen become the
/// previous selection set; the drawing does not change and nothing enters
/// the undo history.
class SelectCommand final : public Command {
public:
    [[nodiscard]] static CommandDescription description();

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] bool needsSelection() const override;
};

}  // namespace tg::cmd
