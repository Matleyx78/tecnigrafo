// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_runner.hpp"

#include <QString>
#include <QWidget>

class QKeyEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;

namespace tg::app {

/// AutoCAD-style command line: a read-only history (three lines visible,
/// scrollable) above the prompt of the runner and an input field.
///
/// In the input field, Enter and space send the text to the runner and Esc
/// cancels. Keys typed in a widget registered with forwardKeysFrom() (the
/// drawing view) reach the command line as if typed there.
///
/// Undo and redo keys (Ctrl+Z, Ctrl+Y and the platform's own) always act on
/// the drawing: in the input field they never undo the typed text.
///
/// Child widget names, used by tests: "history", "prompt", "input".
class CommandLine final : public QWidget {
    Q_OBJECT

public:
    /// The runner must outlive the command line; the command line becomes
    /// the runner's message sink.
    explicit CommandLine(cmd::CommandRunner& runner, QWidget* parent = nullptr);

    /// Sends to the command line the keys typed in `widget` while it has the
    /// focus: text keys (letters, digits, @ < , . -) are written in the input
    /// field, which takes the focus; Enter, space and Esc act as in the
    /// field. Keys with Ctrl, Alt or Cmd are left alone: they are shortcuts.
    void forwardKeysFrom(QWidget* widget);

    /// Adds a line at the end of the history.
    void appendToHistory(const QString& line);

    /// Shows the runner's current prompt. Call it after sending the runner
    /// an input from elsewhere, such as a click in the view.
    void refreshPrompt();

signals:
    /// Emitted after every input sent to the runner: the drawing, the
    /// preview and what can be undone may have changed.
    void inputHandled();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    /// Enter, space and Esc. Returns true if `event` was one of them.
    bool handleControlKey(const QKeyEvent& event);
    bool forwardTextKey(const QKeyEvent& event);
    void submit();
    void cancel();

    cmd::CommandRunner& m_runner;
    QPlainTextEdit* m_history{nullptr};
    QLabel* m_prompt{nullptr};
    QLineEdit* m_input{nullptr};
};

}  // namespace tg::app
