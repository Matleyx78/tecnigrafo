// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/input.hpp"
#include "tg/core/document.hpp"
#include "tg/core/vec2.hpp"
#include "tg/edit/editor.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace tg::cmd {

/// An option of a prompt: {"Close", "C"}. The user can type either string,
/// in any case.
struct Keyword {
    std::string name;
    std::string shortcut;

    friend bool operator==(const Keyword&, const Keyword&) = default;
};

/// What a command asks the user. `text` is the complete sentence shown on
/// the command line, keywords included ("Specify next point or
/// [Close/Undo]:"): it is never assembled from pieces, so that it can be
/// translated as a whole (docs/adr/0005). `keywords` lists the options the
/// runner recognises in typed text.
struct Prompt {
    std::string text;
    std::vector<Keyword> keywords;

    friend bool operator==(const Prompt&, const Prompt&) = default;
};

/// What a command works with. The runner keeps one context for its whole
/// life, so `lastPoint` carries over from one command to the next, as the
/// "@" of AutoCAD does.
struct CommandContext {
    edit::Editor& editor;
    std::optional<core::Vec2> lastPoint;       ///< updated by the command
    std::function<void(std::string)> message;  ///< writes to the history
};

enum class Status : std::uint8_t { Continue, Finished };

/// An interactive command (docs/adr/0005). Commands never read text: they
/// receive inputs already interpreted by CommandRunner.
class Command {
public:
    Command() = default;
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;
    Command(Command&&) = delete;
    Command& operator=(Command&&) = delete;
    virtual ~Command() = default;

    /// Called once, before any input. Opens the command's transaction.
    virtual void start(CommandContext& context) = 0;

    /// Handles one input; never a CancelInput, which the runner handles by
    /// rolling the transaction back. Before returning Finished the command
    /// commits or rolls back its transaction.
    virtual Status handle(CommandContext& context, const Input& input) = 0;

    /// The pointer moved to `point`, in drawing coordinates.
    virtual void pointerMoved(const core::Vec2& point) = 0;

    [[nodiscard]] virtual Prompt prompt() const = 0;

    /// Entities drawn as a preview, not part of the document. Valid until the
    /// next call to a non-const function of the command.
    [[nodiscard]] virtual std::span<const core::Entity> preview() const = 0;
};

}  // namespace tg::cmd
