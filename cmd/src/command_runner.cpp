// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/command_runner.hpp"

#include "text.hpp"
#include "tg/cmd/coordinate_input.hpp"

#include <cassert>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

constexpr std::string_view kIdlePrompt = "Command:";
constexpr std::string_view kInvalidInput = "Invalid point or option.";
constexpr std::string_view kCancelled = "*Cancel*";
constexpr std::string_view kNothingToUndo = "Nothing to undo.";
constexpr std::string_view kNothingToRedo = "Nothing to redo.";

// Commands of the runner itself. "U" is the AutoCAD shortcut of a single
// undo step; here UNDO does the same, since undo groups and marks are not
// part of stage 0.2.
constexpr std::string_view kUndo = "UNDO";
constexpr std::string_view kUndoAlias = "U";
constexpr std::string_view kRedo = "REDO";

bool isUndo(std::string_view name) {
    return detail::equalsIgnoringCase(name, kUndo) || detail::equalsIgnoringCase(name, kUndoAlias);
}

bool isRedo(std::string_view name) {
    return detail::equalsIgnoringCase(name, kRedo);
}

bool matches(const Keyword& keyword, std::string_view text) {
    return detail::equalsIgnoringCase(text, keyword.name) ||
           (!keyword.shortcut.empty() && detail::equalsIgnoringCase(text, keyword.shortcut));
}

}  // namespace

CommandRunner::CommandRunner(const CommandRegistry& registry, edit::Editor& editor)
    : m_registry(registry),
      m_editor(editor),
      m_context{editor, std::nullopt, [this](std::string text) {
                    message(std::move(text));
                }} {}

void CommandRunner::submitText(std::string_view text) {
    text = detail::trim(text);
    if (isActive()) {
        handleTypedWhileActive(text);
    } else if (text.empty()) {
        repeatLastCommand();
    } else {
        startCommand(text);
    }
}

void CommandRunner::submit(const Input& input) {
    if (!isActive()) {
        // Points and options mean nothing without a command; Esc has
        // nothing to cancel.
        if (std::holds_alternative<ConfirmInput>(input)) {
            repeatLastCommand();
        }
        return;
    }
    if (std::holds_alternative<CancelInput>(input)) {
        cancel();
        return;
    }
    forward(input);
}

void CommandRunner::pointerMoved(const core::Vec2& point) {
    m_pointer = point;
    if (isActive()) {
        m_command->pointerMoved(point);
    }
}

std::string CommandRunner::promptText() const {
    return isActive() ? m_command->prompt().text : std::string{kIdlePrompt};
}

std::span<const core::Entity> CommandRunner::preview() const {
    return isActive() ? m_command->preview() : std::span<const core::Entity>{};
}

void CommandRunner::setMessageSink(std::function<void(std::string)> sink) {
    m_sink = std::move(sink);
}

void CommandRunner::startCommand(std::string_view text) {
    if (isUndo(text)) {
        m_lastCommand = kUndo;
        undo();
        return;
    }
    if (isRedo(text)) {
        m_lastCommand = kRedo;
        redo();
        return;
    }
    const std::optional<std::string> name = m_registry.canonicalName(text);
    if (!name) {
        // Composed from a template with one argument, as the plan of stage
        // 0.2 prescribes; translation is decided in stage 0.9.
        message("Unknown command: " + std::string{text} + ".");
        return;
    }
    m_command = m_registry.create(*name);
    assert(m_command && "a registered factory returned no command");
    if (!m_command) {
        return;
    }
    m_lastCommand = *name;
    m_command->start(m_context);
}

void CommandRunner::repeatLastCommand() {
    if (!m_lastCommand.empty()) {
        const std::string name = m_lastCommand;  // startCommand() overwrites it
        startCommand(name);
    }
}

void CommandRunner::handleTypedWhileActive(std::string_view text) {
    if (text.empty()) {
        forward(ConfirmInput{});
        return;
    }
    for (const Keyword& keyword : m_command->prompt().keywords) {
        if (matches(keyword, text)) {
            forward(KeywordInput{keyword.name});
            return;
        }
    }
    if (const std::optional<PointSpec> spec = parseCoordinate(text)) {
        // Before the first pointer movement a typed distance has no
        // direction: falling back to the last point makes it unresolvable.
        const core::Vec2 pointer = m_pointer.value_or(m_context.lastPoint.value_or(core::Vec2{}));
        if (const std::optional<core::Vec2> point = resolve(*spec, m_context.lastPoint, pointer)) {
            forward(PointInput{*point});
            return;
        }
    }
    message(std::string{kInvalidInput});
}

void CommandRunner::forward(const Input& input) {
    if (m_command->handle(m_context, input) == Status::Finished) {
        finish();
    }
}

void CommandRunner::cancel() {
    // End the command first, so that the editor's listener, called by
    // rollback(), already sees the runner idle.
    m_command.reset();
    if (m_editor.inTransaction()) {
        m_editor.rollback();
    }
    message(std::string{kCancelled});
}

void CommandRunner::finish() {
    m_command.reset();
    const bool closed = !m_editor.inTransaction();
    assert(closed && "a command finished without closing its transaction");
    if (!closed) {
        m_editor.rollback();
    }
}

void CommandRunner::undo() {
    const std::string name = m_editor.undoName();
    if (!m_editor.undo()) {
        message(std::string{kNothingToUndo});
        return;
    }
    message("Undone: " + name + ".");
}

void CommandRunner::redo() {
    const std::string name = m_editor.redoName();
    if (!m_editor.redo()) {
        message(std::string{kNothingToRedo});
        return;
    }
    message("Redone: " + name + ".");
}

void CommandRunner::message(std::string text) const {
    if (m_sink) {
        m_sink(std::move(text));
    }
}

}  // namespace tg::cmd
