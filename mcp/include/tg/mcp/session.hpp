// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/mcp/tool.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tg::mcp {

/// Most lines of script one request may carry (docs/adr/0027, point 3): a
/// longer one is refused without running anything.
inline constexpr std::size_t kMaxScriptLines = 10000;

/// The message of every file tool without --root (as the file commands
/// say when they have no file access).
inline constexpr std::string_view kNoFileAccess = "File access is not available.";

/// The options of tecnigrafo-mcp that the tools use (docs/adr/0027, point 4).
struct SessionOptions {
    /// The only folder where the file commands read and write: relative
    /// names, no "..", no absolute paths. std::nullopt: no file access.
    std::optional<std::filesystem::path> root;
    /// The pinned public files of tests/verifica/sorgenti.json, read only
    /// as "@sources/<name>" (only together with `root`).
    std::optional<std::filesystem::path> sources;
    /// Offer the tool ask, for questions to the model (.tgq).
    bool questions{false};
};

/// The live session of tecnigrafo-mcp (docs/adr/0027, point 4): one
/// document, the editor and a cmd::CommandRunner with the built-in commands
/// and the description check on, driven exactly as a level 1 script
/// (script::Level1Session): same echo, same directives, same determinism.
/// Files go through script::CaseFiles inside --root.
class Session {
public:
    explicit Session(SessionOptions options);
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&) = delete;
    Session& operator=(Session&&) = delete;
    ~Session();

    [[nodiscard]] const SessionOptions& options() const { return m_options; }

    /// Runs lines of a .tgs script, level 1 directives only. All the lines
    /// are read first: a line that is not valid, a directive of level 2 or
    /// more than kMaxScriptLines lines is an error and nothing runs.
    /// The text is the lines written in the history since the previous call,
    /// then the sections of any "!report" among the lines, then
    /// "prompt: <the prompt now>".
    [[nodiscard]] ToolResult run(std::span<const std::string> lines);

    /// The report of the state now (docs/verifica/rapporto.md): its first
    /// line and a section [final], whose messages are those written since
    /// the previous section (the previous report or "!report").
    [[nodiscard]] std::string report();

    /// Starts again from an empty document: "prompt: Command:".
    [[nodiscard]] std::string reset();

    /// OPEN with `path` inside --root, as run() would with the two lines.
    /// If OPEN asks first whether to save the changes, it stops at that
    /// question. An invalid name, a missing file, no --root or a command
    /// still running is an error, and nothing runs.
    [[nodiscard]] ToolResult open(std::string_view path);

    /// SAVEAS with `path` inside --root, as run() would with the two lines.
    /// If the file exists, `replace` answers Y to the question to replace
    /// it; otherwise the question stays. An invalid name, no --root or a
    /// command still running is an error, and nothing runs.
    [[nodiscard]] ToolResult save(std::string_view path, bool replace);

private:
    class State;

    /// OPEN or SAVEAS with `path`; see open() and save().
    [[nodiscard]] ToolResult fileCommand(std::string_view command, std::string_view path,
                                         bool mustExist, bool replace);

    SessionOptions m_options;
    std::unique_ptr<State> m_state;
};

/// The error of a name that --root does not allow, or of no --root;
/// std::nullopt if `name` is a relative name inside `options.root`. With
/// `mustExist`, a missing file is an error too. The messages are those of
/// script::CaseFiles (io::FileAccess).
[[nodiscard]] std::optional<std::string> checkRootName(const SessionOptions& options,
                                                       std::string_view name, bool mustExist);

/// run, report, reset, open and save over the live `session`
/// (docs/adr/0027, point 5). Precondition: `session` is not null.
[[nodiscard]] std::vector<std::unique_ptr<Tool>> sessionTools(std::shared_ptr<Session> session);

/// run_script, check_case and, only with `options.questions`, ask
/// (docs/adr/0027, points 5 and 6): each call works apart from the live
/// session. run_script gives the same report as `tecnigrafo-script run`,
/// check_case the same outcome as `tecnigrafo-script check`.
[[nodiscard]] std::vector<std::unique_ptr<Tool>> caseTools(const SessionOptions& options);

}  // namespace tg::mcp
