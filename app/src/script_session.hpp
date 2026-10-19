// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/vec2.hpp"
#include "tg/render/scene_renderer.hpp"
#include "tg/script/script.hpp"

#include <QPointF>
#include <QString>

#include <optional>
#include <string>
#include <vector>

class QPlainTextEdit;
class QWidget;

namespace tg::app {

class MainWindow;

/// Name of the report that `tecnigrafo --script` writes in its output folder.
inline constexpr const char* kScriptReportFile = "rapporto.txt";

/// The light theme of the control mode (docs/adr/0011, point 5): dark
/// drawing on a white background, so that screenshots do not depend on the
/// theme of the system or of the user. The program has only its dark style
/// until the themes of stage 0.6, which will take over these colours.
[[nodiscard]] render::SceneStyle lightSceneStyle();

/// Level 2 of the control mode (docs/adr/0011, docs/verifica/script.md):
/// runs a script in a main window, with keyboard and mouse events sent to
/// its widgets as the tests of module app do, and writes the same report
/// as level 1 (tg::script::writeState).
///
/// - A normal line is typed character by character into the widget that
///   has the keyboard focus, then Enter; an empty line is Enter alone. The
///   keys reach the program as a keyboard's would: a space is Enter, as in
///   the command line of the interface.
/// - !move and !click are mouse events in the view, at the point of the
///   screen that the view shows for the point of the drawing, unrounded. A
///   click gives the view the focus, as a real click does.
/// - !rightclick is the right button where the pointer is; !esc the Esc key.
/// - !snap and !ortho set the switches of the Tools menu; the line that the
///   menu writes in the history ("<Ortho on>") is left out of the report,
///   because the directives do not pass through the keys. !key sends the
///   key, and its line is reported.
/// - The view uses lightSceneStyle().
/// - !view fixes the size of the view, its center and its scale; without
///   it the view is 800x600, centered on 0,0, one drawing unit per pixel.
/// - !screenshot saves the view, at one image pixel per screen pixel, in
///   `<output folder>/<name>.png`.
class ScriptSession {
public:
    /// `window` must outlive the session and must not have received any
    /// input yet; screenshots go to `outputFolder`, created if needed.
    ScriptSession(MainWindow& window, QString outputFolder);

    /// Shows and activates the window, then performs every action of
    /// `script` and adds the final section of the report. Returns false if
    /// an action could not be performed: error() says why, and report()
    /// holds the sections written until then.
    [[nodiscard]] bool run(const script::Script& script);

    /// The report: its first line and one section per "!report", then the
    /// final section once run() has succeeded.
    [[nodiscard]] const std::string& report() const { return m_report; }

    /// Why run() stopped; empty after a successful run.
    [[nodiscard]] const QString& error() const { return m_error; }

private:
    [[nodiscard]] bool activateWindow();
    [[nodiscard]] bool perform(const script::Action& action);

    [[nodiscard]] bool perform(const script::TypedLine& line);
    [[nodiscard]] bool perform(const script::ViewDirective& view);
    [[nodiscard]] bool perform(const script::MoveDirective& move);
    [[nodiscard]] bool perform(const script::ClickDirective& click);
    [[nodiscard]] bool perform(const script::RightClickDirective& directive);
    [[nodiscard]] bool perform(const script::EscDirective& directive);
    [[nodiscard]] bool perform(const script::AidDirective& directive);
    [[nodiscard]] bool perform(const script::KeyDirective& key);
    [[nodiscard]] bool perform(const script::ReportDirective& directive);
    [[nodiscard]] bool perform(const script::ScreenshotDirective& screenshot);
    [[nodiscard]] static bool perform(const script::LevelDirective& directive);
    [[nodiscard]] static bool perform(const script::ToleranceDirective& directive);

    /// The point of the view's screen that shows `point` of the drawing.
    [[nodiscard]] QPointF toScreen(const core::Vec2& point) const;
    /// The pointer moves to `point` of the drawing.
    void moveTo(const core::Vec2& point, Qt::KeyboardModifiers modifiers);
    /// A press and a release of `button` where the pointer is.
    void clickAt(const QPointF& position, Qt::MouseButton button, Qt::KeyboardModifiers modifiers);
    /// A press and a release of `key` in the widget with the focus.
    void pressKey(int key, Qt::KeyboardModifiers modifiers, const QString& text);
    [[nodiscard]] QWidget* keyboardTarget() const;

    /// Moves the lines added to the history since the last call into
    /// m_messages; with `report` false they are dropped instead.
    void collectMessages(bool report = true);
    void writeSection(const std::string& title);
    void fail(const QString& error);

    MainWindow& m_window;
    QString m_outputFolder;
    QPlainTextEdit* m_history{nullptr};
    /// Lines of the history already collected.
    int m_historyLines{0};
    std::vector<std::string> m_messages;  ///< since the previous section
    /// The last point of the drawing the pointer was moved to, as the
    /// script gave it, and where it is on the screen.
    std::optional<core::Vec2> m_pointer;
    QPointF m_screenPointer;
    std::string m_report;
    int m_reports{0};
    QString m_error;
};

/// `tecnigrafo --script <file> --output <folder>`: runs the script at level
/// 2 on an empty drawing and writes kScriptReportFile and the screenshots
/// in `outputFolder`. Returns 0, or 1 if the script cannot be read, has an
/// error or cannot be performed; the reason goes to the standard error.
/// Needs a QApplication.
[[nodiscard]] int runScriptFile(const QString& scriptPath, const QString& outputFolder);

}  // namespace tg::app
