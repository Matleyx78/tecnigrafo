// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/entity_id.hpp"
#include "tg/core/vec2.hpp"

#include <string>
#include <variant>
#include <vector>

namespace tg::cmd {

/// A point of the drawing: a click, or coordinates typed on the command line.
struct PointInput {
    core::Vec2 point;

    friend constexpr bool operator==(const PointInput&, const PointInput&) = default;
};

/// An option of the current prompt, always in its canonical form ("Close"),
/// whether the user typed the name or the shortcut.
struct KeywordInput {
    std::string keyword;

    friend bool operator==(const KeywordInput&, const KeywordInput&) = default;
};

/// Enter, space or right click: accept what has been done so far.
struct ConfirmInput {
    friend constexpr bool operator==(const ConfirmInput&, const ConfirmInput&) = default;
};

/// Esc: abandon the command. Handled by CommandRunner, never passed to a
/// command.
struct CancelInput {
    friend constexpr bool operator==(const CancelInput&, const CancelInput&) = default;
};

/// The objects a command works on (docs/adr/0008 and docs/adr/0032). Only
/// CommandRunner sends it: once, right after start(), to commands whose
/// needsSelection() is true, and then never empty; or as the answer to a
/// Selection prompt, possibly empty.
struct SelectionInput {
    std::vector<core::EntityId> ids;

    friend bool operator==(const SelectionInput&, const SelectionInput&) = default;
};

/// A number typed for a Distance (drawing units), an Angle (radians,
/// converted from the degrees typed) or a Number (as typed) prompt.
struct ValueInput {
    double value;

    friend constexpr bool operator==(const ValueInput&, const ValueInput&) = default;
};

/// A line typed for a Text prompt, exactly as typed. Never empty.
struct TextInput {
    std::string text;

    friend bool operator==(const TextInput&, const TextInput&) = default;
};

/// The entity chosen at an Object prompt (docs/adr/0032, point 1), with the
/// point where it was picked. Only CommandRunner sends it, and never for an
/// entity on a locked layer.
struct ObjectInput {
    core::EntityId id;
    core::Vec2 point;   ///< where the object was picked, in drawing coordinates
    bool shift{false};  ///< Shift held (PickAction::Remove)

    friend bool operator==(const ObjectInput&, const ObjectInput&) = default;
};

/// One input of the user, from the mouse, the keyboard or the command line
/// (docs/adr/0005).
using Input = std::variant<PointInput, KeywordInput, ConfirmInput, CancelInput, SelectionInput,
                           ValueInput, TextInput, ObjectInput>;

}  // namespace tg::cmd
