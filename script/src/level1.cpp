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
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

/// The state of one run: the drawing, the runner and what the report needs
/// besides them.
class Level1Session {
public:
    Level1Session() : m_editor(m_document), m_runner(registry(), m_editor) {
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    Level1Session(const Level1Session&) = delete;
    Level1Session& operator=(const Level1Session&) = delete;
    Level1Session(Level1Session&&) = delete;
    Level1Session& operator=(Level1Session&&) = delete;
    ~Level1Session() = default;

    void run(const Action& action) {
        std::visit([this](const auto& concrete) { this->perform(concrete); }, action);
    }

    void report(std::string_view title) {
        writeState({title, m_editor, m_runner, m_messages, m_pointer}, m_report);
        m_messages.clear();
    }

    [[nodiscard]] std::string takeReport() { return std::move(m_report); }

private:
    static const cmd::CommandRegistry& registry() {
        static const cmd::CommandRegistry kCommands = [] {
            cmd::CommandRegistry built;
            cmd::registerBuiltinCommands(built);
            return built;
        }();
        return kCommands;
    }

    void perform(const TypedLine& line) {
        // The command line echoes the exchange before handing the text to
        // the runner, as AutoCAD does ("Command: PL").
        const std::string prompt = m_runner.promptText();
        m_messages.push_back(line.text.empty() ? prompt : prompt + " " + line.text);
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

    core::Document m_document;
    edit::Editor m_editor;
    cmd::CommandRunner m_runner;
    std::vector<std::string> m_messages;  ///< since the previous section
    std::optional<core::Vec2> m_pointer;
    std::string m_report;
    int m_reports{0};
};

}  // namespace

std::string runLevel1(const Script& script) {
    assert(script.level == 1 && "a level 2 script does not run at level 1");
    Level1Session session;
    for (const Action& action : script.actions) {
        session.run(action);
    }
    session.report(kFinalSection);
    std::string report{kReportHeader};
    report += '\n';
    report += session.takeReport();
    return report;
}

}  // namespace tg::script
