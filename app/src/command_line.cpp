// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "command_line.hpp"

#include <QEvent>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QTextDocument>
#include <QVBoxLayout>

#include <cmath>
#include <string>
#include <utility>

namespace tg::app {

namespace {

// Lines of history visible without scrolling, as in AutoCAD's docked
// command line.
constexpr int kVisibleHistoryLines = 3;

// Older lines are dropped beyond this, so a long session does not grow the
// history without bound.
constexpr int kMaxHistoryLines = 2000;

// Characters of coordinates and command names typed in the view and sent to
// the command line, besides ASCII letters and digits (docs/tappe/0.2).
constexpr QStringView kForwardedSymbols = u"@<,.-";

bool isForwardedCharacter(QChar c) {
    const char16_t code = c.unicode();
    const bool asciiLetter = (code >= u'a' && code <= u'z') || (code >= u'A' && code <= u'Z');
    const bool digit = code >= u'0' && code <= u'9';
    return asciiLetter || digit || kForwardedSymbols.contains(c);
}

/// Undo and redo of the drawing. Ctrl+Y is added to the platform's redo keys
/// because AutoCAD users expect it everywhere (MainWindow does the same).
bool isUndoOrRedo(const QKeyEvent& event) {
    return event.matches(QKeySequence::Undo) || event.matches(QKeySequence::Redo) ||
           event.keyCombination() == QKeyCombination(Qt::ControlModifier, Qt::Key_Y);
}

bool hasShortcutModifier(const QKeyEvent& event) {
    // On macOS Qt maps Cmd to ControlModifier and Ctrl to MetaModifier.
    return event.modifiers().testAnyFlags(Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
}

}  // namespace

CommandLine::CommandLine(cmd::CommandRunner& runner, QWidget* parent)
    : QWidget(parent),
      m_runner(runner),
      m_history(new QPlainTextEdit(this)),
      m_prompt(new QLabel(this)),
      m_input(new QLineEdit(this)) {
    m_history->setObjectName(QStringLiteral("history"));
    m_prompt->setObjectName(QStringLiteral("prompt"));
    m_input->setObjectName(QStringLiteral("input"));

    m_history->setReadOnly(true);
    m_history->setFocusPolicy(Qt::NoFocus);
    m_history->setMaximumBlockCount(kMaxHistoryLines);
    m_history->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    const QFontMetrics metrics(m_history->font());
    const int margins = static_cast<int>(std::ceil(2.0 * m_history->document()->documentMargin())) +
                        (2 * m_history->frameWidth());
    m_history->setFixedHeight((metrics.lineSpacing() * kVisibleHistoryLines) + margins);

    auto* inputRow = new QHBoxLayout;
    inputRow->setContentsMargins(0, 0, 0, 0);
    inputRow->addWidget(m_prompt);
    inputRow->addWidget(m_input, 1);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_history);
    layout->addLayout(inputRow);

    setFocusProxy(m_input);
    m_input->installEventFilter(this);

    // The guard keeps the sink harmless if the runner outlives this widget.
    m_runner.setMessageSink([self = QPointer<CommandLine>(this)](const std::string& text) {
        if (!self.isNull()) {
            self->appendToHistory(QString::fromStdString(text));
        }
    });
    refreshPrompt();
}

void CommandLine::forwardKeysFrom(QWidget* widget) {
    widget->installEventFilter(this);
}

void CommandLine::appendToHistory(const QString& line) {
    m_history->appendPlainText(line);
}

bool CommandLine::eventFilter(QObject* watched, QEvent* event) {
    const bool keyEvent =
        event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride;
    const auto* key = keyEvent ? dynamic_cast<const QKeyEvent*>(event) : nullptr;
    if (key == nullptr) {
        return QWidget::eventFilter(watched, event);
    }
    if (watched == m_input && isUndoOrRedo(*key)) {
        // QLineEdit claims these keys (ShortcutOverride) for its own text
        // undo. Filtering the event without accepting it lets the window's
        // Undo and Redo shortcuts fire; a key press that still arrives, with
        // Undo disabled during a command, is dropped.
        return true;
    }
    if (event->type() != QEvent::KeyPress) {
        return QWidget::eventFilter(watched, event);
    }
    if (watched == m_input) {
        return handleControlKey(*key);
    }
    if (!hasShortcutModifier(*key)) {
        return handleControlKey(*key) || forwardTextKey(*key);
    }
    return QWidget::eventFilter(watched, event);
}

bool CommandLine::handleControlKey(const QKeyEvent& event) {
    switch (event.key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            submit();
            return true;
        case Qt::Key_Escape:
            cancel();
            return true;
        default:
            return false;
    }
}

bool CommandLine::forwardTextKey(const QKeyEvent& event) {
    const QString text = event.text();
    if (text.size() != 1 || !isForwardedCharacter(text.front())) {
        return false;
    }
    m_input->setFocus(Qt::OtherFocusReason);
    m_input->insert(text);
    return true;
}

void CommandLine::submit() {
    const QString text = m_input->text();
    // Echo the exchange in the history, as AutoCAD does ("Command: PL").
    appendToHistory(text.isEmpty() ? m_prompt->text()
                                   : QStringLiteral("%1 %2").arg(m_prompt->text(), text));
    m_input->clear();
    m_runner.submitText(text.toStdString());
    refreshPrompt();
    emit inputHandled();
}

void CommandLine::cancel() {
    m_input->clear();
    m_runner.submit(cmd::CancelInput{});
    refreshPrompt();
    emit inputHandled();
}

void CommandLine::refreshPrompt() {
    m_prompt->setText(QString::fromStdString(m_runner.promptText()));
}

}  // namespace tg::app
