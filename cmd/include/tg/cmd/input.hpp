// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/vec2.hpp"

#include <string>
#include <variant>

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

/// One input of the user, from the mouse, the keyboard or the command line
/// (docs/adr/0005).
using Input = std::variant<PointInput, KeywordInput, ConfirmInput, CancelInput>;

}  // namespace tg::cmd
