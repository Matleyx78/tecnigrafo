// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/input.hpp"
#include "tg/core/document.hpp"
#include "tg/core/vec2.hpp"
#include "tg/edit/editor.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace tg::cmd {

/// Runs one command at a time and turns what the user types into inputs,
/// so that the syntax is the same in every command (docs/adr/0005).
///
/// With no command running, typed text is a command name or alias; empty
/// text (Enter) or a ConfirmInput repeats the last command. U, UNDO and REDO
/// are handled here, and only work with no command running.
///
/// With a command running, typed text becomes, in this order: a
/// ConfirmInput if empty, a KeywordInput if it matches an option of the
/// prompt, a PointInput if it is a coordinate that can be resolved;
/// otherwise the runner writes "Invalid point or option." and the prompt
/// stays. A CancelInput rolls the command's transaction back and ends it.
///
/// The registry and the editor must outlive the runner.
class CommandRunner {
public:
    CommandRunner(const CommandRegistry& registry, edit::Editor& editor);

    // The context hands commands a callback bound to this object.
    CommandRunner(const CommandRunner&) = delete;
    CommandRunner& operator=(const CommandRunner&) = delete;
    CommandRunner(CommandRunner&&) = delete;
    CommandRunner& operator=(CommandRunner&&) = delete;
    ~CommandRunner() = default;

    /// Text from the command line; empty text means Enter. Leading and
    /// trailing white space is ignored.
    void submitText(std::string_view text);

    /// Input from the mouse and the keyboard.
    void submit(const Input& input);

    /// The pointer moved to `point`, in drawing coordinates. Gives the
    /// direction of distances typed alone ("50").
    void pointerMoved(const core::Vec2& point);

    /// True while a command is running.
    [[nodiscard]] bool isActive() const { return m_command != nullptr; }

    /// The prompt of the running command, or "Command:" with none.
    [[nodiscard]] std::string promptText() const;

    /// The preview of the running command, empty with none.
    [[nodiscard]] std::span<const core::Entity> preview() const;

    /// Where messages go (the history of the command line). An empty
    /// function discards them.
    void setMessageSink(std::function<void(std::string)> sink);

private:
    void startCommand(std::string_view text);
    void repeatLastCommand();
    void handleTypedWhileActive(std::string_view text);
    void forward(const Input& input);
    void cancel();
    void finish();
    void undo();
    void redo();
    void message(std::string text) const;

    const CommandRegistry& m_registry;
    edit::Editor& m_editor;
    CommandContext m_context;
    std::unique_ptr<Command> m_command;
    std::string m_lastCommand;  ///< canonical name, empty before the first one
    std::optional<core::Vec2> m_pointer;
    std::function<void(std::string)> m_sink;
};

}  // namespace tg::cmd
