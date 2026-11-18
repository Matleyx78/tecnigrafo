// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"

namespace tg::cmd {

/// EXPLODE: turns every selected polyline into lines and arcs, every block
/// reference into the entities of its block (core::explodeInsert()) and
/// every dimension into its graphics (core::explodeDimension()), in one undo
/// step, "Explode" (docs/adr/0031, point 8; docs/adr/0045, point 4;
/// docs/adr/0047, point 4). The new entities get new handles and come last
/// (docs/adr/0031, point 6), in the order of the block or of the graphics,
/// and the exploded entity is removed. A refusal of an insert or of a
/// dimension is written in the history, and that entity stays. Other
/// entities, and polylines with elevation or thickness, are counted and
/// reported, not exploded.
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
