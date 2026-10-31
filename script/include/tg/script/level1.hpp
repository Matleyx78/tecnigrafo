// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/io/file_access.hpp"
#include "tg/script/script.hpp"

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace tg::cmd {
class CommandRunner;
}  // namespace tg::cmd

namespace tg::script {

/// How a level 1 script runs.
struct RunOptions {
    /// Where the file commands read and write (docs/adr/0021, point 4):
    /// a CaseFiles in tecnigrafo-script. nullptr: the script has no files.
    io::FileAccess* files{nullptr};
};

/// Runs a level 1 script on an empty document and returns the report.
/// A directive that needs level 2 is an error at level 1: parseScript()
/// rejects it unless the script says "!level 2", and such a script does not
/// run here.
///
/// The script drives a cmd::CommandRunner with the built-in commands, as
/// the main window does (docs/verifica/script.md): a normal line is written
/// in the history after the prompt, as the command line echoes it, and
/// then submitted; !move, !click, !rightclick and !esc are the pointer, the
/// clicks and the Esc key of the view; !view sets the size of a pixel.
/// Precondition: script.level == 1.
[[nodiscard]] std::string runLevel1(const Script& script, const RunOptions& options = {});

/// A level 1 run up to the frontier.
struct PrefixRun {
    std::string report;                ///< as runLevel1() writes it, [final] at the frontier
    std::optional<Frontier> frontier;  ///< the line typed with an unknown command name
};

/// Runs `script` as runLevel1() does, but stops before the first normal
/// line typed while no command is running (CommandRunner::isActive() is
/// false) whose text the runner would answer with "Unknown command":
/// not U, not REDO, and not a command or alias of the registry
/// (CommandRegistry::canonicalName), read as startCommand() reads it
/// (docs/adr/0037, point 4.2). The report ends with the section [final]
/// at that point. Precondition: script.level == 1, script.lines filled.
[[nodiscard]] PrefixRun runLevel1UntilUnknownCommand(const Script& script,
                                                     const RunOptions& options = {});

/// The state of a level 1 run, one action at a time: what runLevel1() does
/// for a whole script, open for a live session (tecnigrafo-mcp,
/// docs/adr/0027 point 4). An empty document, the editor and a
/// cmd::CommandRunner with the built-in commands and the description check
/// on (docs/adr/0026). runLevel1() is exactly: perform() for every action,
/// writeSection(kFinalSection), then kReportHeader, '\n' and takeReport().
class Level1Session {
public:
    /// `files` is not owned and must outlive the session; nullptr: no files.
    explicit Level1Session(io::FileAccess* files);
    Level1Session(const Level1Session&) = delete;
    Level1Session& operator=(const Level1Session&) = delete;
    Level1Session(Level1Session&&) = delete;
    Level1Session& operator=(Level1Session&&) = delete;
    ~Level1Session();

    /// Performs one action as a level 1 script does; "!report" writes the
    /// section "report <n>" (see takeReport()).
    /// Precondition: not a directive of level 2 (!key, !screenshot).
    void perform(const Action& action);

    /// Writes a section of the report with the state now and the messages
    /// since the previous section, then forgets those messages.
    void writeSection(std::string_view title);

    /// The sections written so far, without the first line of the report;
    /// the session forgets them.
    [[nodiscard]] std::string takeReport();

    /// Every line written in the history of the command line since the
    /// start, in order: the echoes and the messages.
    [[nodiscard]] std::span<const std::string> history() const;

    /// The runner, to read its prompt and state.
    [[nodiscard]] const cmd::CommandRunner& runner() const;

private:
    class State;
    std::unique_ptr<State> m_state;
};

}  // namespace tg::script
