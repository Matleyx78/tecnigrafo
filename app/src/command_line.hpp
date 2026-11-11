// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "document_session.hpp"
#include "tg/cmd/command_runner.hpp"

#include <QString>
#include <QWidget>

class QKeyEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QTextBlock;
class QToolButton;

namespace tg::app {

/// AutoCAD-style command line: a read-only history (three lines visible,
/// scrollable) above the prompt of the runner and an input field.
///
/// Enter sends input to the runner and Esc cancels. Space submits except
/// when the runner accepts literal text, where spaces and Unicode are kept.
/// Keys typed in a widget registered with forwardKeysFrom() (the
/// drawing view) reach the command line as if typed there.
///
/// Undo and redo keys (Ctrl+Z, Ctrl+Y and the platform's own) always act on
/// the drawing: in the input field they never undo the typed text.
/// Delete reaches the window's Erase action only while the input is empty;
/// otherwise it edits the text normally.
///
/// The command line works with the runner of the session of setSession(),
/// the active one (docs/adr/0041, point 2), and is that runner's message
/// sink. The text typed and not yet sent belongs to the session (decision B2
/// of docs/DECISIONI.md): setSession() leaves it in the session it leaves
/// and shows what the new one kept. Without a runner the input field is
/// disabled and keys are not taken. The history belongs to the window.
///
/// It looks as in the reference of docs/adr/0039 (area 6): a box with the
/// last lines of the history, options in square brackets in the accent
/// colour, and under them the input row with a cross that cancels like Esc.
/// floatOver() lays it over the bottom of the drawing.
///
/// Child widget names, used by tests: "history", "prompt", "input",
/// "cancelButton".
class CommandLine final : public QWidget {
    Q_OBJECT

public:
    /// A command line without a runner, until setSession().
    explicit CommandLine(QWidget* parent = nullptr);

    /// A command line working with `runner` alone, without sessions, as the
    /// widget tests use it. The runner must outlive the command line.
    explicit CommandLine(cmd::CommandRunner& runner, QWidget* parent = nullptr);

    /// Works with the runner of `session` from now on; nullptr for none. The
    /// session is borrowed: the owner calls setSession() again, with
    /// another session or nullptr, before destroying it.
    void setSession(DocumentSession* session);

    [[nodiscard]] DocumentSession* session() const { return m_session; }

    /// Sends to the command line the keys typed in `widget` while it has the
    /// focus: text keys (letters, digits, @ < , . -) are written in the input
    /// field, which takes the focus; Enter, space and Esc act as in the
    /// field. Keys with Ctrl, Alt or Cmd are left alone: they are shortcuts.
    void forwardKeysFrom(QWidget* widget);

    /// Lays the command line over the bottom of `area`, in the middle, as a
    /// child of the parent of `area`, and keeps it there when `area` moves
    /// or changes size (area 6).
    void floatOver(QWidget* area);

    /// Adds a line at the end of the history.
    void appendToHistory(const QString& line);

    /// Shows the runner's current prompt, empty without a runner. Call it
    /// after sending the runner an input from elsewhere, such as a click in
    /// the view.
    void refreshPrompt();

signals:
    /// Emitted after every input sent to the runner: the drawing, the
    /// preview and what can be undone may have changed.
    void inputHandled();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    /// Enter, context-sensitive space and Esc; true when handled.
    bool handleControlKey(const QKeyEvent& event);
    bool forwardTextKey(const QKeyEvent& event);
    void submit();
    void cancel();
    /// Works with `runner` from now on, nullptr for none: the message sink
    /// moves to it, and the input field is enabled with it.
    void attachRunner(cmd::CommandRunner* runner);
    /// Puts the command line in its place over the area of floatOver().
    void place();
    /// The options in square brackets of `block` in the accent colour.
    void colourOptions(const QTextBlock& block);

    DocumentSession* m_session{nullptr};
    cmd::CommandRunner* m_runner{nullptr};
    QPlainTextEdit* m_history{nullptr};
    QLabel* m_prompt{nullptr};
    QLineEdit* m_input{nullptr};
    QToolButton* m_cancel{nullptr};
    QWidget* m_area{nullptr};
};

}  // namespace tg::app
