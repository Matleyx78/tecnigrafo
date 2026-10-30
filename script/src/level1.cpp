// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/level1.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/cmd/drawing_aids.hpp"
#include "tg/cmd/input.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"
#include "tg/script/report.hpp"

#include <cassert>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::script {

/// The state of one run: the drawing, the runner and what the report needs
/// besides them.
class Level1Session::State {
public:
    explicit State(io::FileAccess* files)
        : m_editor(m_document), m_runner(registry(), m_editor), m_files(files) {
        m_runner.setFileAccess(files);
        m_runner.setMessageSink([this](std::string text) { write(std::move(text)); });
        // A prompt missing from its command's description is written in the
        // history: the report differs from any expected one, so the case
        // fails and prints the message among its differences
        // (docs/adr/0026, point 3).
        m_runner.setDescriptionCheck([this](const std::string& message) { write(message); });
    }

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&&) = delete;
    State& operator=(State&&) = delete;
    ~State() = default;

    void run(const Action& action) {
        std::visit([this](const auto& concrete) { this->perform(concrete); }, action);
    }

    void report(std::string_view title) {
        writeState({title, m_editor, m_runner, m_messages, m_pointer}, m_report);
        m_messages.clear();
    }

    [[nodiscard]] std::string takeReport() { return std::exchange(m_report, std::string{}); }

    [[nodiscard]] std::span<const std::string> history() const { return m_history; }

    [[nodiscard]] const cmd::CommandRunner& runner() const { return m_runner; }

private:
    static const cmd::CommandRegistry& registry() {
        static const cmd::CommandRegistry kCommands = [] {
            cmd::CommandRegistry built;
            cmd::registerBuiltinCommands(built);
            return built;
        }();
        return kCommands;
    }

    /// A line of the history: for the next section of the report, and for
    /// the whole history of a live session.
    void write(std::string line) {
        m_history.push_back(line);
        m_messages.push_back(std::move(line));
    }

    void perform(const TypedLine& line) {
        // The command line echoes the exchange before handing the text to
        // the runner, as AutoCAD does ("Command: PL").
        const std::string prompt = m_runner.promptText();
        write(line.text.empty() ? prompt : prompt + " " + line.text);
        m_runner.submitText(line.text);
    }

    void perform(const ViewDirective& view) { m_runner.setPixelSize(view.scale); }

    void perform(const MoveDirective& move) {
        m_pointer = move.point;
        m_runner.pointerMoved(move.point);
    }

    void perform(const ClickDirective& click) {
        // A real mouse reaches the point before it clicks there.
        m_pointer = click.point;
        m_runner.pointerMoved(click.point);
        m_runner.click(click.point, click.remove ? cmd::PickAction::Remove : cmd::PickAction::Add);
    }

    void perform(const RightClickDirective& /*directive*/) { m_runner.submit(cmd::ConfirmInput{}); }

    void perform(const EscDirective& /*directive*/) { m_runner.submit(cmd::CancelInput{}); }

    void perform(const AidDirective& directive) {
        cmd::DrawingAids aids = m_runner.drawingAids();
        if (directive.aid == AidDirective::Aid::Snap) {
            aids.objectSnap = directive.on;
        } else {
            aids.ortho = directive.on;
        }
        m_runner.setDrawingAids(aids);
    }

    void perform(const ScriptSequencesDirective& directive) {
        m_runner.setScriptSequences(directive.on);
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

    core::Document m_document{core::Document::newDrawing()};
    edit::Editor m_editor;
    cmd::CommandRunner m_runner;
    // Kept for the file commands: package P15 of milestone 0.5 gives it to
    // the runner. Unused until then, hence [[maybe_unused]].
    [[maybe_unused]] io::FileAccess* m_files;
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

}  // namespace tg::script
