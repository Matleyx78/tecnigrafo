// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/vec2.hpp"
#include "tg/edit/editor.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace tg::cmd {

/// TRIM and EXTEND (docs/adr/0035, points 3-8 and 21-23). The two commands
/// differ only in what a pick does without Shift: TRIM trims and EXTEND
/// extends; with Shift each does the other.
///
/// Quick mode (TRIMEXTENDMODE 1, CommandDefaults::trimExtendMode): every
/// line, arc, circle and polyline on a visible layer is an edge. Standard
/// mode (0), and always in the AutoCAD script sequences (docs/adr/0032,
/// point 8): the edges are chosen first, and an empty choice means all of
/// them, as in quick mode. A text is never an edge.
///
/// Trimming takes the cuts from core::cutsAlong(), the edges without the
/// object itself, a polyline among the edges cutting itself; the piece
/// between the last cut not after the pick and the first one after it goes,
/// the rest is rebuilt with core::partOf(): the first piece keeps the entity
/// (a circle becomes an arc), a second one is a copy. Degenerate pieces are
/// dropped, and when nothing is left the object is erased. Without cuts the
/// object is erased in quick mode, while standard mode writes "Object does
/// not intersect an edge.".
///
/// Extending moves the end nearer the pick (the end on a tie) to the first
/// edge it meets along its line or circle, with core::extendTo(). Circles,
/// closed polylines and texts are not extended; without an edge to reach
/// nothing changes.
///
/// When solid-space refused some data and no cut is left, or when a piece
/// cannot be represented, the operation fails: nothing changes and the
/// command writes "Cannot trim that object." or "Cannot extend that object."
/// (docs/adr/0035, point 23).
///
/// Every object trimmed or extended is one operation, with a restore point
/// that Undo goes back to (docs/adr/0031, point 11); the whole command is
/// one undo step, "Trim" or "Extend", and Esc keeps what was done
/// (docs/adr/0015).
class TrimExtendCommand final : public Command {
public:
    enum class Action : std::uint8_t { Trim, Extend };

    explicit TrimExtendCommand(Action action) : m_action(action) {}

    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;
    void cancel(CommandContext& context) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description(Action action);

private:
    enum class Stage : std::uint8_t {
        Edges,   ///< standard mode: the edges, a Selection prompt
        Object,  ///< the object to trim or extend
        Mode     ///< the mOde option: Quick or Standard
    };

    void handleEdges(const Input& input);
    void handleObject(CommandContext& context, const Input& input);
    void handleMode(CommandContext& context, const Input& input);
    /// Enters `quick` mode, or standard mode asking for the edges.
    void enterMode(CommandContext& context, bool quick);
    /// The edges for a pick of `object`, without it, in document order.
    [[nodiscard]] std::vector<core::Entity> edgesFor(const CommandContext& context,
                                                     core::EntityId object,
                                                     bool& objectIsEdge) const;
    void trim(CommandContext& context, core::EntityId id, const core::Vec2& pick);
    void extend(CommandContext& context, core::EntityId id, const core::Vec2& pick);
    /// Marks the start of an operation, for Undo.
    void beginOperation(CommandContext& context);

    Action m_action;
    Stage m_stage{Stage::Object};
    Stage m_beforeMode{Stage::Object};  ///< where the mOde option returns
    bool m_quick{true};
    /// The chosen edges in standard mode; std::nullopt for all of them.
    std::optional<std::vector<core::EntityId>> m_edges;
    std::vector<edit::Editor::Savepoint> m_savepoints;
};

/// TRIMEXTENDMODE: whether TRIM and EXTEND start in quick mode (1) or in
/// standard mode (0). A value of the session, 1 at start-up (docs/adr/0035,
/// point 3), so it opens no undo step.
class TrimextendmodeCommand final : public Command {
public:
    void start(CommandContext& context) override;
    Status handle(CommandContext& context, const Input& input) override;

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] Prompt prompt() const override;

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

    [[nodiscard]] static CommandDescription description();

private:
    int m_value{1};
};

}  // namespace tg::cmd
