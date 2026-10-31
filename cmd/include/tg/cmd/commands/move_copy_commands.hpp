// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace tg::cmd {

/// The selection a displacement command works on, as it was when the
/// command started: the preview and the changes are built from these
/// snapshots (docs/adr/0031, point 5).
struct DisplacedEntity {
    core::EntityId id;
    core::Entity entity;
};

/// MOVE: translates the selected entities in place, keeping their identity
/// (docs/adr/0031, point 5). One undo step, "Move".
class MoveCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;
    [[nodiscard]] bool needsSelection() const override;

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t {
        Selection,  ///< before the runner hands over the selection
        Base,
        Second,
        Displacement
    };

    Status apply(CommandContext& context, const core::Vec2& displacement);
    void updatePreview();

    Stage m_stage{Stage::Selection};
    std::vector<DisplacedEntity> m_selected;
    core::Vec2 m_base;
    std::optional<core::Vec2> m_pointer;
    std::vector<core::Entity> m_preview;
};

/// COPY in Multiple mode: every second point adds a copy of the selection
/// (docs/adr/0031, point 6). One undo step, "Copy"; Esc keeps the copies
/// already made (docs/adr/0015). In the sequence of AutoCAD scripts the
/// command ends after the first copy (docs/adr/0032, point 8).
class CopyCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;
    void pointerMoved(const core::Vec2& point) override;
    [[nodiscard]] Prompt prompt() const override;
    [[nodiscard]] std::span<const core::Entity> preview() const override;
    [[nodiscard]] std::optional<core::Vec2> basePoint() const override;
    [[nodiscard]] bool needsSelection() const override;

    [[nodiscard]] static CommandDescription description();

private:
    enum class Stage : std::uint8_t {
        Selection,  ///< before the runner hands over the selection
        Base,
        First,         ///< the second point of the first copy
        Displacement,  ///< one copy by a typed displacement, then the command ends
        More           ///< further copies, with Exit and Undo
    };

    /// Adds one copy of the selection, displaced by `displacement`; ends the
    /// command when the sequence asks for it.
    Status copyBy(CommandContext& context, const core::Vec2& displacement);
    Status finish(CommandContext& context);
    void updatePreview();

    Stage m_stage{Stage::Selection};
    std::vector<DisplacedEntity> m_selected;
    core::Vec2 m_base;
    std::optional<core::Vec2> m_pointer;
    /// One savepoint before each copy, so that Undo removes the last one.
    std::vector<edit::Editor::Savepoint> m_savepoints;
    bool m_scriptSequence{false};
    std::vector<core::Entity> m_preview;
};

}  // namespace tg::cmd
