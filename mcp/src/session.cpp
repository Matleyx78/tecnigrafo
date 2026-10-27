// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The live session of tecnigrafo-mcp (docs/adr/0027, point 4): a level 1
// run of the control mode that goes on one request at a time.

#include "tg/mcp/session.hpp"

#include "tg/cmd/command.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/io/result.hpp"
#include "tg/script/case_files.hpp"
#include "tg/script/level1.hpp"
#include "tg/script/report.hpp"
#include "tg/script/script.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace tg::mcp {

/// The files, then the level 1 session that uses them: a new State is a
/// new empty document with the same --root.
class Session::State {
public:
    explicit State(const SessionOptions& options)
        : m_files(options.root ? std::make_unique<script::CaseFiles>(*options.root, options.sources)
                               : nullptr),
          m_session(m_files.get()) {}

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&&) = delete;
    State& operator=(State&&) = delete;
    ~State() = default;

    void perform(const script::Action& action) { m_session.perform(action); }

    [[nodiscard]] const cmd::CommandRunner& runner() const { return m_session.runner(); }

    /// The history lines written since the previous call, the sections of
    /// any "!report", and the prompt: what run() and the file tools show.
    [[nodiscard]] std::string exchange() {
        std::string text;
        const std::span<const std::string> history = m_session.history();
        for (std::size_t i = m_seen; i < history.size(); ++i) {
            text += history[i];
            text += '\n';
        }
        m_seen = history.size();
        text += m_session.takeReport();
        text += prompt();
        return text;
    }

    [[nodiscard]] std::string report() {
        m_session.writeSection(script::kFinalSection);
        std::string text{script::kReportHeader};
        text += '\n';
        text += m_session.takeReport();
        return text;
    }

    [[nodiscard]] std::string prompt() const { return "prompt: " + runner().promptText(); }

private:
    std::unique_ptr<script::CaseFiles> m_files;  ///< nullptr without --root
    script::Level1Session m_session;
    std::size_t m_seen{0};  ///< history lines already shown
};

namespace {

std::string describe(const script::ParseError& error) {
    return "line " + std::to_string(error.line) + ": " + error.message;
}

}  // namespace

Session::Session(SessionOptions options)
    : m_options(std::move(options)), m_state(std::make_unique<State>(m_options)) {}

Session::~Session() = default;

ToolResult Session::run(std::span<const std::string> lines) {
    if (lines.size() > kMaxScriptLines) {
        return ToolResult::error("Too many lines: at most " + std::to_string(kMaxScriptLines) +
                                 " in one request.");
    }
    // One string per line: the script text is the lines, each with its end,
    // so that "" is Enter alone as in a .tgs file.
    std::string text;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find_first_of("\r\n") != std::string::npos) {
            return ToolResult::error("line " + std::to_string(i + 1) +
                                     ": a line cannot contain a line break.");
        }
        text += lines[i];
        text += '\n';
    }
    std::variant<script::Script, script::ParseError> parsed = script::parseScript(text);
    if (const auto* error = std::get_if<script::ParseError>(&parsed)) {
        return ToolResult::error(describe(*error));
    }
    const script::Script& script = std::get<script::Script>(parsed);
    if (script.level != 1) {
        return ToolResult::error(
            "The session runs at level 1: \"!level 2\", \"!key\" and \"!screenshot\" are not "
            "available.");
    }
    for (const script::Action& action : script.actions) {
        m_state->perform(action);
    }
    return ToolResult::success(m_state->exchange());
}

std::string Session::report() {
    return m_state->report();
}

std::string Session::reset() {
    m_state = std::make_unique<State>(m_options);
    return m_state->prompt();
}

ToolResult Session::open(std::string_view path) {
    return fileCommand("OPEN", path, true, false);
}

ToolResult Session::save(std::string_view path, bool replace) {
    return fileCommand("SAVEAS", path, false, replace);
}

ToolResult Session::fileCommand(std::string_view command, std::string_view path, bool mustExist,
                                bool replace) {
    if (std::optional<std::string> error = checkRootName(m_options, path, mustExist)) {
        return ToolResult::error(std::move(*error));
    }
    const cmd::CommandRunner& runner = m_state->runner();
    if (runner.isActive() || runner.selectionRectangle()) {
        return ToolResult::error(
            "A command is still running: finish it, or cancel it with "
            "\"!esc\", before " +
            std::string{command} + ".");
    }
    m_state->perform(script::TypedLine{std::string{command}});
    // OPEN asks first whether to save a modified drawing: the name is not
    // the answer to that question, so the exchange stops there.
    const cmd::FilePrompt expected = mustExist ? cmd::FilePrompt::Open : cmd::FilePrompt::Save;
    if (runner.isActive() && runner.prompt().file == expected) {
        m_state->perform(script::TypedLine{std::string{path}});
        const cmd::Prompt question = runner.prompt();
        const bool asksToReplace =
            runner.isActive() &&
            std::ranges::any_of(question.keywords,
                                [](const cmd::Keyword& keyword) { return keyword.name == "Yes"; });
        if (replace && asksToReplace) {
            m_state->perform(script::TypedLine{"Y"});
        }
    }
    return ToolResult::success(m_state->exchange());
}

std::optional<std::string> checkRootName(const SessionOptions& options, std::string_view name,
                                         bool mustExist) {
    if (!options.root) {
        return std::string{kNoFileAccess};
    }
    script::CaseFiles files{*options.root, options.sources};
    if (files.exists(name)) {
        return std::nullopt;
    }
    // exists() says no both for a missing file and for a name CaseFiles
    // rejects: reading tells them apart, with the message of CaseFiles.
    const io::Result<std::string> bytes = files.read(name);
    if (bytes.ok()) {
        return std::nullopt;
    }
    if (mustExist || bytes.error().message == script::kInvalidFileName) {
        return bytes.error().message;
    }
    return std::nullopt;
}

}  // namespace tg::mcp
