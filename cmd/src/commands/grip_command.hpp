// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/core/document.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace tg::cmd {

/// The stretch of one hot grip (docs/adr/0034, point 3). Internal to the
/// runner: it is not in the registry and has no description, so its prompt
/// belongs to the runner (docs/adr/0026, point 3).
///
/// start() writes "** STRETCH **" and asks "Specify stretch point:". A point
/// moves the grip with core::moveGrip() and replaces the entity in one
/// "Grip edit" transaction; an invalid result changes nothing and writes
/// "Invalid grip edit.". Enter leaves the grip without changes, like Esc.
class GripCommand final : public Command {
public:
    /// Precondition: `index` is valid in core::gripPoints(entity), and `grip`
    /// is that grip point.
    GripCommand(core::EntityId id, core::Entity entity, std::size_t index, const core::Vec2& grip);

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;

    /// The hot grip: snap, ortho and polar start from it.
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override { return m_grip; }

private:
    core::EntityId m_id;
    core::Entity m_entity;  ///< as it was when the grip became hot
    std::size_t m_index{0};
    core::Vec2 m_grip;
    std::vector<core::Entity> m_preview;  ///< the moved entity, or nothing
};

}  // namespace tg::cmd
