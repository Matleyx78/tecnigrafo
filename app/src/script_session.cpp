// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "script_session.hpp"

#include "cad_view.hpp"
#include "command_line.hpp"
#include "main_window.hpp"
#include "tg/script/case_files.hpp"
#include "tg/script/report.hpp"
#include "theme.hpp"

#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QChar>
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QKeyCombination>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QSize>
#include <QTextBlock>
#include <QTextDocument>

#include <cstddef>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <utility>
#include <variant>

namespace tg::app {
namespace {

// How long the window may take to become active, or the view to take the
// size of !view. The offscreen platform needs a few event loop passes.
constexpr int kWaitMilliseconds = 5000;
// A pass of the event loop while waiting.
constexpr int kWaitStepMilliseconds = 10;

// The view without "!view" (docs/verifica/script.md, "Dettagli").
constexpr int kDefaultViewWidth = 800;
constexpr int kDefaultViewHeight = 600;

constexpr int kExitPassed = 0;
constexpr int kExitError = 1;

/// The Qt key of a character typed on a keyboard: the code of its upper
/// case for printable characters, as Qt reports keys.
int keyOf(char32_t character) {
    if (character == U' ') {
        return Qt::Key_Space;
    }
    if (character == U'\t') {
        return Qt::Key_Tab;
    }
    // Control characters and those outside the Basic Multilingual Plane
    // have no key of their own: they arrive as text.
    constexpr char32_t kFirstPrintable = 0x20;
    constexpr char32_t kLastBmp = 0xFFFF;
    if (character < kFirstPrintable || character > kLastBmp) {
        return Qt::Key_unknown;
    }
    return QChar(static_cast<char16_t>(character)).toUpper().unicode();
}

/// The text a key produces without Ctrl, Alt or Cmd: the character in
/// lower case, or in upper case with Shift; nothing for function keys.
QString textOf(QKeyCombination combination) {
    const Qt::KeyboardModifiers modifiers = combination.keyboardModifiers();
    const int key = combination.key();
    // Qt's codes from Key_Escape up are keys that write nothing.
    const bool printable = key >= Qt::Key_Space && key < Qt::Key_Escape && key <= 0xFFFF;
    if (!printable ||
        modifiers.testAnyFlags(Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
        return {};
    }
    const QChar character(static_cast<char16_t>(key));
    return {modifiers.testFlag(Qt::ShiftModifier) ? character : character.toLower()};
}

/// Runs the event loop until `done` or until the wait is over; returns
/// `done()`.
template <typename Condition>
bool waitFor(Condition done) {
    const QDeadlineTimer deadline(kWaitMilliseconds);
    while (!done() && !deadline.hasExpired()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, kWaitStepMilliseconds);
    }
    return done();
}

}  // namespace

ScriptSession::ScriptSession(MainWindow& window, QString outputFolder)
    : m_window(window),
      m_outputFolder(std::move(outputFolder)),
      m_history(window.commandLine()->findChild<QPlainTextEdit*>(QStringLiteral("history"))) {
    m_window.setScriptMode();
    // The report needs every line written in the history, also beyond the
    // lines the interface keeps.
    if (m_history != nullptr) {
        m_history->setMaximumBlockCount(0);
    }
}

bool ScriptSession::run(const script::Script& script) {
    m_report = std::string{script::kReportHeader} + "\n";
    m_error.clear();
    if (m_history == nullptr) {
        fail(QStringLiteral("the command line has no history"));
        return false;
    }
    m_window.view()->setStyle(sceneStyle(Theme::Light));
    if (!activateWindow()) {
        fail(QStringLiteral("the main window did not become active"));
        return false;
    }
    if (!perform(script::ViewDirective{{0.0, 0.0}, 1.0, kDefaultViewWidth, kDefaultViewHeight})) {
        return false;
    }
    m_screenPointer = toScreen({0.0, 0.0});
    for (const script::Action& action : script.actions) {
        if (!perform(action)) {
            return false;
        }
        // What a real event loop does between two inputs: layouts,
        // repaints, deferred signals.
        QCoreApplication::processEvents();
    }
    collectMessages();
    writeSection(std::string{script::kFinalSection});
    return true;
}

bool ScriptSession::activateWindow() {
    m_window.show();
    m_window.activateWindow();
    // Shortcuts (!key) only fire in the active window.
    return waitFor([this] { return m_window.isActiveWindow(); });
}

bool ScriptSession::perform(const script::Action& action) {
    return std::visit([this](const auto& concrete) { return this->perform(concrete); }, action);
}

bool ScriptSession::perform(const script::TypedLine& line) {
    const QString text = QString::fromStdString(line.text);
    for (const char32_t character : text.toUcs4()) {
        pressKey(keyOf(character), Qt::NoModifier, QString::fromUcs4(&character, 1));
    }
    pressKey(Qt::Key_Return, Qt::NoModifier, QStringLiteral("\r"));
    return true;
}

bool ScriptSession::perform(const script::ViewDirective& view) {
    CadView* cadView = m_window.view();
    const QSize size(view.width, view.height);
    cadView->setFixedSize(size);
    // The window grows if the view does not fit; it never shrinks, which
    // leaves the view its size.
    m_window.resize(m_window.size().expandedTo(m_window.minimumSizeHint()));
    if (!waitFor([cadView, size] { return cadView->size() == size; })) {
        fail(QStringLiteral("the view cannot be %1x%2 pixels").arg(view.width).arg(view.height));
        return false;
    }
    cadView->setView(view.center, view.scale);
    // The mouse has not moved: the pointer stays at its point of the
    // drawing, as at level 1, and later clicks without !move use it.
    if (m_pointer) {
        m_screenPointer = toScreen(*m_pointer);
    }
    return true;
}

bool ScriptSession::perform(const script::MoveDirective& move) {
    moveTo(move.point, Qt::NoModifier);
    return true;
}

bool ScriptSession::perform(const script::ClickDirective& click) {
    const Qt::KeyboardModifiers modifiers = click.remove ? Qt::ShiftModifier : Qt::NoModifier;
    moveTo(click.point, modifiers);
    clickAt(m_screenPointer, Qt::LeftButton, modifiers);
    return true;
}

bool ScriptSession::perform(const script::RightClickDirective& /*directive*/) {
    clickAt(m_screenPointer, Qt::RightButton, Qt::NoModifier);
    return true;
}

bool ScriptSession::perform(const script::EscDirective& /*directive*/) {
    pressKey(Qt::Key_Escape, Qt::NoModifier, QStringLiteral("\x1b"));
    return true;
}

bool ScriptSession::perform(const script::AidDirective& directive) {
    const bool snap = directive.aid == script::AidDirective::Aid::Snap;
    auto* action = m_window.findChild<QAction*>(snap ? QStringLiteral("aids.osnap")
                                                     : QStringLiteral("aids.ortho"));
    if (action == nullptr) {
        fail(QStringLiteral("the Tools menu has no switch for %1")
                 .arg(snap ? QStringLiteral("!snap") : QStringLiteral("!ortho")));
        return false;
    }
    // The switch of the menu keeps the menu, the status bar and the runner
    // in agreement, so that a later F3 or F8 toggles from the right state.
    collectMessages();
    action->setChecked(directive.on);
    collectMessages(false);
    return true;
}

bool ScriptSession::perform(const script::ScriptSequencesDirective& directive) {
    m_window.setScriptSequences(directive.on);
    return true;
}

bool ScriptSession::perform(const script::KeyDirective& key) {
    const QKeySequence sequence =
        QKeySequence::fromString(QString::fromStdString(key.key), QKeySequence::PortableText);
    if (sequence.count() != 1 || sequence[0].key() == Qt::Key_unknown) {
        fail(QStringLiteral("!key %1: not a key").arg(QString::fromStdString(key.key)));
        return false;
    }
    const QKeyCombination combination = sequence[0];
    pressKey(combination.key(), combination.keyboardModifiers(), textOf(combination));
    return true;
}

bool ScriptSession::perform(const script::ReportDirective& /*directive*/) {
    collectMessages();
    ++m_reports;
    writeSection("report " + std::to_string(m_reports));
    return true;
}

bool ScriptSession::perform(const script::ScreenshotDirective& screenshot) {
    CadView* view = m_window.view();
    // One image pixel per pixel of the view, whatever the screen's ratio.
    QImage image(view->size(), QImage::Format_ARGB32);
    image.setDevicePixelRatio(1.0);
    image.fill(Qt::white);
    view->render(&image);
    const QDir folder(m_outputFolder);
    const QString path = folder.filePath(QString::fromStdString(screenshot.name) + ".png");
    if (!folder.mkpath(QStringLiteral(".")) || !image.save(path, "PNG")) {
        fail(QStringLiteral("cannot write %1").arg(path));
        return false;
    }
    return true;
}

// Read by parseScript(): nothing to do while running.
bool ScriptSession::perform(const script::LevelDirective& /*directive*/) {
    return true;
}

bool ScriptSession::perform(const script::ToleranceDirective& /*directive*/) {
    return true;
}

QPointF ScriptSession::toScreen(const core::Vec2& point) const {
    const core::Vec2 screen = m_window.view()->viewport().worldToScreen(point);
    return {screen.x, screen.y};
}

void ScriptSession::moveTo(const core::Vec2& point, Qt::KeyboardModifiers modifiers) {
    CadView* view = m_window.view();
    m_pointer = point;
    m_screenPointer = toScreen(point);
    QMouseEvent event(QEvent::MouseMove, m_screenPointer, view->mapToGlobal(m_screenPointer),
                      Qt::NoButton, Qt::NoButton, modifiers);
    QCoreApplication::sendEvent(view, &event);
}

void ScriptSession::clickAt(const QPointF& position, Qt::MouseButton button,
                            Qt::KeyboardModifiers modifiers) {
    CadView* view = m_window.view();
    // QApplication gives the focus on a press only to events that come
    // from the window system; a real click on the view takes it.
    if ((view->focusPolicy() & Qt::ClickFocus) == Qt::ClickFocus) {
        view->setFocus(Qt::MouseFocusReason);
    }
    const QPointF global = view->mapToGlobal(position);
    QMouseEvent press(QEvent::MouseButtonPress, position, global, button, button, modifiers);
    QCoreApplication::sendEvent(view, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, position, global, button, Qt::NoButton,
                        modifiers);
    QCoreApplication::sendEvent(view, &release);
}

void ScriptSession::pressKey(int key, Qt::KeyboardModifiers modifiers, const QString& text) {
    // Sent through QCoreApplication: event filters run, and a key press
    // that is a shortcut of the active window triggers it, as with a real
    // keyboard. The release goes where the focus is after the press.
    QKeyEvent press(QEvent::KeyPress, key, modifiers, text);
    QCoreApplication::sendEvent(keyboardTarget(), &press);
    QKeyEvent release(QEvent::KeyRelease, key, modifiers, text);
    QCoreApplication::sendEvent(keyboardTarget(), &release);
}

QWidget* ScriptSession::keyboardTarget() const {
    QWidget* focus = QApplication::focusWidget();
    if (focus != nullptr && focus->window() == &m_window) {
        return focus;
    }
    return &m_window;
}

void ScriptSession::collectMessages(bool report) {
    const QTextDocument* document = m_history->document();
    // An empty history is one empty block; appending the first line fills it.
    const int lines = document->isEmpty() ? 0 : document->blockCount();
    for (int line = m_historyLines; report && line < lines; ++line) {
        m_messages.push_back(document->findBlockByNumber(line).text().toStdString());
    }
    m_historyLines = lines;
}

void ScriptSession::writeSection(const std::string& title) {
    script::writeState({title, m_window.editor(), m_window.runner(), m_messages, m_pointer},
                       m_report);
    m_messages.clear();
}

void ScriptSession::fail(const QString& error) {
    m_error = error;
}

int runScriptFile(const QString& scriptPath, const QString& outputFolder) {
    QFile file(scriptPath);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "cannot read " << scriptPath.toStdString() << "\n";
        return kExitError;
    }
    const QByteArray bytes = file.readAll();
    const std::variant<script::Script, script::ParseError> parsed = script::parseScript(
        std::string_view(bytes.constData(), static_cast<std::size_t>(bytes.size())),
        QFileInfo(scriptPath).filesystemFilePath());
    if (const auto* error = std::get_if<script::ParseError>(&parsed)) {
        std::cerr << scriptPath.toStdString() << ", line " << error->line << ": " << error->message
                  << "\n";
        return kExitError;
    }

    // Keep the public script-session API unchanged; --sources is specific to
    // this command-line entry point, not to sessions driven by widget tests.
    const QStringList arguments = QCoreApplication::arguments();
    std::optional<std::filesystem::path> sources;
    const QString sourcesPrefix = QStringLiteral("--sources=");
    for (qsizetype i = 1; i < arguments.size(); ++i) {
        if (arguments[i] == QStringLiteral("--sources") && i + 1 < arguments.size()) {
            sources = QDir(arguments[++i]).filesystemPath();
        } else if (arguments[i].startsWith(sourcesPrefix)) {
            sources = QDir(arguments[i].mid(sourcesPrefix.size())).filesystemPath();
        }
    }
    const auto output = QDir(outputFolder).filesystemPath();
    const auto filesFolder = output / script::kCaseFilesFolder;
    const auto caseFolder = QFileInfo(scriptPath).absoluteDir().filesystemPath();
    if (const auto error = script::prepareCaseFiles(caseFolder, filesFolder)) {
        std::cerr << *error << "\n";
        return kExitError;
    }
    script::CaseFiles files(filesFolder, sources);
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.setFileAccess(&files);
    ScriptSession session(window, outputFolder);
    if (!session.run(std::get<script::Script>(parsed))) {
        std::cerr << scriptPath.toStdString() << ": " << session.error().toStdString() << "\n";
        return kExitError;
    }

    const QDir folder(outputFolder);
    QFile report(folder.filePath(QString::fromLatin1(kScriptReportFile)));
    const std::string& text = session.report();
    if (!folder.mkpath(QStringLiteral(".")) || !report.open(QIODevice::WriteOnly) ||
        report.write(text.data(), static_cast<qint64>(text.size())) !=
            static_cast<qint64>(text.size())) {
        std::cerr << "cannot write " << report.fileName().toStdString() << "\n";
        return kExitError;
    }
    return kExitPassed;
}

}  // namespace tg::app
