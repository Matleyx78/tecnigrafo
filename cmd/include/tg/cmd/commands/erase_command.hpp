// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"

namespace tg::cmd {

/// Removes the selection in one undoable transaction. Selection gestures
/// and confirmation belong to CommandRunner (docs/adr/0008).
class EraseCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] bool needsSelection() const override;
};

}  // namespace tg::cmd
