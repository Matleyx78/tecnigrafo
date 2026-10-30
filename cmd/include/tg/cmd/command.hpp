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

namespace tg::io {
class FileAccess;
}

namespace tg::cmd {

/// An option of a prompt: {"Close", "C"}. The user can type either string,
/// in any case.
struct Keyword {
    std::string name;
    std::string shortcut;

    friend bool operator==(const Keyword&, const Keyword&) = default;
};

/// File dialogs may replace these text questions in the application (ADR 0021).
enum class FilePrompt : std::uint8_t { None, Open, Save };

/// What a prompt asks for, which decides how CommandRunner reads the answer
/// (docs/adr/0014 and docs/adr/0032):
/// - Point, Distance, Angle: a point, or a bare number for Distance and Angle;
/// - Text: a line, as typed;
/// - Object: one entity, picked with a click or a typed point (ObjectInput);
/// - Number: a pure number, with no unit or angle (ValueInput), or a point;
/// - Selection: a selection built with the runner's gestures and written
///   options, handed over with Enter (SelectionInput).
enum class PromptKind : std::uint8_t { Point, Distance, Angle, Text, Object, Number, Selection };

/// What a command asks the user. `text` is the complete sentence shown on
/// the command line, keywords included ("Specify next point or
/// [Close/Undo]:"): it is never assembled from pieces, so that it can be
/// translated as a whole (docs/adr/0005). `keywords` lists the options the
/// runner recognises in typed text.
struct Prompt {
    std::string text;
    std::vector<Keyword> keywords;
    PromptKind kind{PromptKind::Point};
    FilePrompt file{FilePrompt::None};

    friend bool operator==(const Prompt&, const Prompt&) = default;
};

/// Values commands remember for the whole session (docs/adr/0014).
struct CommandDefaults {
    double textHeight{2.5};
    double textRotation{0.0};            ///< radians
    std::optional<double> circleRadius;  ///< none before the first circle
};

/// What a command works with. The runner keeps one context for its whole
/// life, so `lastPoint` carries over from one command to the next, as the
/// "@" of AutoCAD does. Ortho does not use it: it belongs to the running
/// command (Command::basePoint()).
struct CommandContext {
    edit::Editor& editor;
    std::optional<core::Vec2> lastPoint;       ///< updated by the command
    std::function<void(std::string)> message;  ///< writes to the history
    CommandDefaults defaults{};
    io::FileAccess* files{nullptr};
    bool documentReplaced{false};
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
    /// calling cancel(). Before returning Finished the command
    /// commits or rolls back its transaction.
    virtual Status handle(CommandContext& context, const Input& input) = 0;

    /// Esc: the command keeps what it has completed and closes its
    /// transaction (docs/adr/0015). The default rolls everything back.
    virtual void cancel(CommandContext& context) {
        if (context.editor.inTransaction()) {
            context.editor.rollback();
        }
    }

    /// The pointer moved to `point`, in drawing coordinates.
    virtual void pointerMoved(const core::Vec2& point) = 0;

    [[nodiscard]] virtual Prompt prompt() const = 0;

    /// Entities drawn as a preview, not part of the document. Valid until the
    /// next call to a non-const function of the command.
    [[nodiscard]] virtual std::span<const core::Entity> preview() const = 0;

    /// The point of this command that the next point is drawn from, which
    /// ortho aligns the mouse to (docs/adr/0008); std::nullopt while the
    /// command asks for its first point, so that a first click lands where
    /// the user clicks, as in AutoCAD. Unlike CommandContext::lastPoint it
    /// never comes from a previous command.
    [[nodiscard]] virtual std::optional<core::Vec2> basePoint() const { return std::nullopt; }

    /// True for commands that work on existing entities. The runner asks
    /// "Select objects:" itself and calls start() only once it has a
    /// selection, then hands it over with one SelectionInput
    /// (docs/adr/0008).
    [[nodiscard]] virtual bool needsSelection() const { return false; }
};

}  // namespace tg::cmd
