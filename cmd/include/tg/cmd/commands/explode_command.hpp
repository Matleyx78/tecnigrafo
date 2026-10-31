// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

namespace tg::cmd {

/// EXPLODE: turns every selected polyline into lines and arcs, in one undo
/// step, "Explode" (docs/adr/0031, point 8). The new entities are derived
/// from the polyline (docs/adr/0031, point 6) and the polyline is removed.
/// Other entities, and polylines with elevation or thickness, are counted
/// and reported, not exploded.
class ExplodeCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] bool needsSelection() const override;

    [[nodiscard]] static CommandDescription description();
};

}  // namespace tg::cmd
