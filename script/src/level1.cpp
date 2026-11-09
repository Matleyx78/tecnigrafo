// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/level1.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/cmd/document_host.hpp"
#include "tg/cmd/drawing_aids.hpp"
#include "tg/cmd/drawing_session.hpp"
#include "tg/cmd/input.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/document_file.hpp"
#include "tg/script/report.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

/// The built-in commands, shared by every run.
const cmd::CommandRegistry& builtinRegistry() {
    static const cmd::CommandRegistry kCommands = [] {
        cmd::CommandRegistry built;
        cmd::registerBuiltinCommands(built);
        return built;
    }();
    return kCommands;
}

// What CommandRunner::submitText() trims from a line typed at "Command:",
// and the names of the runner's own commands (cmd/src/command_runner.cpp):
// the runner does not export them.
constexpr std::string_view kWhiteSpace = " \t\n\v\f\r";
constexpr std::array<std::string_view, 3> kRunnerCommands{"UNDO", "U", "REDO"};

std::string_view trimmed(std::string_view text) {
    const std::size_t first = text.find_first_not_of(kWhiteSpace);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kWhiteSpace);
    return text.substr(first, last - first + 1);
}

bool equalsIgnoringCase(std::string_view lhs, std::string_view rhs) {
    const auto upper = [](char c) {
        return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
    };
    return std::ranges::equal(lhs, rhs, [&upper](char a, char b) { return upper(a) == upper(b); });
}

/// The name a line typed at "Command:" starts, if the runner would answer
/// it with "Unknown command"; nothing otherwise. An empty line repeats the
/// last command.
std::optional<std::string_view> unknownCommand(std::string_view text) {
    const std::string_view name = trimmed(text);
    if (name.empty() ||
        std::ranges::any_of(
            kRunnerCommands,
            [name](std::string_view own) { return equalsIgnoringCase(own, name); }) ||
        builtinRegistry().canonicalName(name)) {
        return std::nullopt;
    }
    return name;
}

}  // namespace

/// The state of one run: the open drawings, one of them active, and what
/// the report needs besides them. The state is the host of the documents
/// that NEW and OPEN open (docs/adr/0041, point 2), as the main window is at
/// level 2: one history for all of them, and the drawing aids, the script
/// sequences and the size of a pixel of the view carried to a new document,
/// as the window keeps them.
class Level1Session::State final : public cmd::DocumentHost {
public:
    explicit State(io::FileAccess* files) : m_files(files) {
        m_active = &add(core::Document::newDrawing(), edit::DocumentFile{});
    }

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&&) = delete;
    State& operator=(State&&) = delete;
    ~State() override = default;

    void run(const Action& action) {
        std::visit([this](const auto& concrete) { this->perform(concrete); }, action);
    }

    void report(std::string_view title) {
        std::vector<ReportDocument> documents;
        documents.reserve(m_sessions.size());
        for (const auto& session : m_sessions) {
            documents.push_back({session->file().name(), session.get() == m_active});
        }
        writeState({title, m_active->editor(), m_active->runner(), m_active->file(), m_messages,
                    m_pointer, documents},
                   m_report);
        m_messages.clear();
    }

    [[nodiscard]] std::string takeReport() { return std::exchange(m_report, std::string{}); }

    [[nodiscard]] std::span<const std::string> history() const { return m_history; }

    [[nodiscard]] const cmd::CommandRunner& runner() const { return m_active->runner(); }

    cmd::DrawingSession& addDocument(core::Document document, edit::DocumentFile file) override {
        cmd::DrawingSession& session = add(std::move(document), std::move(file));
        // What the view and the switches of the window keep at level 2.
        session.runner().setPixelSize(m_active->runner().pixelSize());
        activate(session);
        return session;
    }

    [[nodiscard]] cmd::DrawingSession* findOpen(std::string_view name) override {
        const auto found = std::ranges::find_if(
            m_sessions, [name](const auto& session) { return session->file().name() == name; });
        return found != m_sessions.end() ? found->get() : nullptr;
    }

    void activate(cmd::DrawingSession& session) override {
        assert(std::ranges::any_of(
                   m_sessions, [&session](const auto& open) { return open.get() == &session; }) &&
               "the session is not open here");
        // The drawing aids are the window's at level 2: the runner shown gets
        // them. The runner left keeps its command, halfway or not.
        session.runner().setDrawingAids(m_active->runner().drawingAids());
        m_active = &session;
    }

    [[nodiscard]] std::vector<std::string> openNames() const override {
        std::vector<std::string> names;
        for (const auto& session : m_sessions) {
            if (!session->file().hasFile()) {
                names.push_back(session->file().name());
            }
        }
        return names;
    }

private:
    /// Opens a session after the others, not active, with the settings that
    /// every runner of the run shares.
    cmd::DrawingSession& add(core::Document document, edit::DocumentFile file) {
        // A unique_ptr each: the vector may grow, the sessions stay where
        // they are, and none is destroyed before the run ends.
        cmd::DrawingSession& session = *m_sessions.emplace_back(
            std::make_unique<cmd::DrawingSession>(builtinRegistry(), m_files, std::move(document)));
        session.file() = std::move(file);
        cmd::CommandRunner& runner = session.runner();
        runner.setDocumentHost(this);
        runner.setScriptSequences(m_scriptSequences);
        runner.setMessageSink([this](std::string text) { write(std::move(text)); });
        // A prompt missing from its command's description is written in the
        // history: the report differs from any expected one, so the case
        // fails and prints the message among its differences
        // (docs/adr/0026, point 3).
        runner.setDescriptionCheck([this](const std::string& message) { write(message); });
        return session;
    }

    [[nodiscard]] cmd::CommandRunner& active() { return m_active->runner(); }

    /// A line of the history: for the next section of the report, and for
    /// the whole history of a live session.
    void write(std::string line) {
        m_history.push_back(line);
        m_messages.push_back(std::move(line));
    }

    void perform(const TypedLine& line) {
        // The command line echoes the exchange before handing the text to
        // the runner, as AutoCAD does ("Command: PL").
        cmd::CommandRunner& runner = active();
        const std::string prompt = runner.promptText();
        write(line.text.empty() ? prompt : prompt + " " + line.text);
        // NEW and OPEN may make another document active: the runner of this
        // one lives on, and the next action goes to the active one.
        runner.submitText(line.text);
    }

    void perform(const ViewDirective& view) { active().setPixelSize(view.scale); }

    void perform(const MoveDirective& move) {
        m_pointer = move.point;
        active().pointerMoved(move.point);
    }

    void perform(const ClickDirective& click) {
        // A real mouse reaches the point before it clicks there.
        m_pointer = click.point;
        cmd::CommandRunner& runner = active();
        runner.pointerMoved(click.point);
        runner.click(click.point, click.remove ? cmd::PickAction::Remove : cmd::PickAction::Add);
    }

    void perform(const RightClickDirective& /*directive*/) { active().submit(cmd::ConfirmInput{}); }

    void perform(const EscDirective& /*directive*/) { active().submit(cmd::CancelInput{}); }

    void perform(const AidDirective& directive) {
        cmd::DrawingAids aids = active().drawingAids();
        switch (directive.aid) {
            case AidDirective::Aid::Snap:
                aids.objectSnap = directive.on;
                break;
            case AidDirective::Aid::Ortho:
                // Switching ortho on switches polar tracking off (docs/adr/0033,
                // point 9); the runner leaves the exclusion to its callers.
                aids.ortho = directive.on;
                aids.polar = aids.polar && !directive.on;
                break;
            case AidDirective::Aid::Polar:
                aids.polar = directive.on;
                aids.ortho = aids.ortho && !directive.on;
                break;
        }
        active().setDrawingAids(aids);
    }

    void perform(const OsnapDirective& directive) {
        cmd::DrawingAids aids = active().drawingAids();
        aids.snapModes = directive.modes;
        active().setDrawingAids(aids);
    }

    void perform(const ScriptSequencesDirective& directive) {
        // For every runner, as the main window does at level 2.
        m_scriptSequences = directive.on;
        for (const auto& session : m_sessions) {
            session->runner().setScriptSequences(directive.on);
        }
    }

    void perform(const ReportDirective& /*directive*/) {
        ++m_reports;
        report("report " + std::to_string(m_reports));
    }

    static void perform(const KeyDirective& /*directive*/) {
        assert(false && "parseScript() rejects !key at level 1");
    }

    static void perform(const ScreenshotDirective& /*directive*/) {
        assert(false && "parseScript() rejects !screenshot at level 1");
    }

    // Read by parseScript(): nothing to do while running.
    void perform(const LevelDirective& /*directive*/) {}

    void perform(const ToleranceDirective& /*directive*/) {}

    io::FileAccess* m_files;
    bool m_scriptSequences{false};
    /// In the order they were opened; never moved nor closed during a run.
    std::vector<std::unique_ptr<cmd::DrawingSession>> m_sessions;
    cmd::DrawingSession* m_active{nullptr};
    std::vector<std::string> m_messages;  ///< since the previous section
    std::vector<std::string> m_history;   ///< since the start
    std::optional<core::Vec2> m_pointer;
    std::string m_report;
    int m_reports{0};
};

Level1Session::Level1Session(io::FileAccess* files) : m_state(std::make_unique<State>(files)) {}

Level1Session::~Level1Session() = default;

void Level1Session::perform(const Action& action) {
    m_state->run(action);
}

void Level1Session::writeSection(std::string_view title) {
    m_state->report(title);
}

std::string Level1Session::takeReport() {
    return m_state->takeReport();
}

std::span<const std::string> Level1Session::history() const {
    return m_state->history();
}

const cmd::CommandRunner& Level1Session::runner() const {
    return m_state->runner();
}

std::string runLevel1(const Script& script, const RunOptions& options) {
    assert(script.level == 1 && "a level 2 script does not run at level 1");
    Level1Session session{options.files};
    for (const Action& action : script.actions) {
        session.perform(action);
    }
    session.writeSection(kFinalSection);
    std::string report{kReportHeader};
    report += '\n';
    report += session.takeReport();
    return report;
}

PrefixRun runLevel1UntilUnknownCommand(const Script& script, const RunOptions& options) {
    assert(script.level == 1 && "a level 2 script does not run at level 1");
    assert(script.lines.size() == script.actions.size() && "parseScript() fills the lines");
    PrefixRun run;
    Level1Session session{options.files};
    for (std::size_t index = 0; index < script.actions.size(); ++index) {
        const Action& action = script.actions[index];
        if (const auto* line = std::get_if<TypedLine>(&action);
            line != nullptr && !session.runner().isActive()) {
            if (const std::optional<std::string_view> name = unknownCommand(line->text)) {
                run.frontier =
                    Frontier{script.lines[index], "unknown command " + std::string{*name}};
                break;
            }
        }
        session.perform(action);
    }
    session.writeSection(kFinalSection);
    run.report = kReportHeader;
    run.report += '\n';
    run.report += session.takeReport();
    return run;
}

}  // namespace tg::script
