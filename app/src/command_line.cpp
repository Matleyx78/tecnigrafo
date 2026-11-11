// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "command_line.hpp"

#include "theme.hpp"

#include <QEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QRect>
#include <QRegularExpression>
#include <QSize>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <initializer_list>
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

// The box over the drawing (docs/adr/0039, area 6): as wide as this part
// of the drawing (876 px of 1276 in the reference), never narrower than
// kMinimumWidth unless the drawing is, this far from the edges of the
// drawing, with this space between its border and what it holds.
constexpr double kWidthRatio = 0.68;
constexpr int kMinimumWidth = 560;
constexpr int kOuterMargin = 12;
constexpr int kInnerMargin = 4;
// The input row, as high as in the reference, and the side of its icons.
constexpr int kInputHeight = 24;
constexpr int kIconSide = 16;

constexpr const char* kCancelIcon = "ui-cancel";

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

CommandLine::CommandLine(cmd::CommandRunner& runner, QWidget* parent) : CommandLine(parent) {
    attachRunner(&runner);
}

CommandLine::CommandLine(QWidget* parent)
    : QWidget(parent),
      m_history(new QPlainTextEdit(this)),
      m_prompt(new QLabel(this)),
      m_input(new QLineEdit(this)),
      m_cancel(new QToolButton(this)) {
    setObjectName(QStringLiteral("commandLine"));
    // The box is drawn by the style sheet (docs/adr/0039, area 6); a plain
    // QWidget draws no background of its own.
    setAttribute(Qt::WA_StyledBackground);
    m_history->setObjectName(QStringLiteral("history"));
    m_prompt->setObjectName(QStringLiteral("prompt"));
    m_input->setObjectName(QStringLiteral("input"));
    m_cancel->setObjectName(QStringLiteral("cancelButton"));

    // The input row on the surface of a field, with the cross that cancels
    // and the mark of the prompt before the prompt (area 6).
    auto* field = new QFrame(this);
    field->setObjectName(QStringLiteral("commandInput"));
    field->setMinimumHeight(kInputHeight);
    auto* mark = new QLabel(QStringLiteral(">"), field);
    mark->setObjectName(QStringLiteral("promptMark"));

    // Fixed pitch, as in the reference of docs/adr/0039 (area 6).
    for (QWidget* widget : std::initializer_list<QWidget*>{m_history, m_prompt, m_input, mark}) {
        widget->setFont(fixedFont());
    }
    m_history->setReadOnly(true);
    m_history->setFocusPolicy(Qt::NoFocus);
    m_history->setUndoRedoEnabled(false);
    m_history->setMaximumBlockCount(kMaxHistoryLines);
    m_history->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    const QFontMetrics metrics(m_history->font());
    const int margins = static_cast<int>(std::ceil(2.0 * m_history->document()->documentMargin())) +
                        (2 * m_history->frameWidth());
    m_history->setFixedHeight((metrics.lineSpacing() * kVisibleHistoryLines) + margins);

    m_cancel->setAutoRaise(true);
    m_cancel->setIconSize(QSize(kIconSide, kIconSide));
    m_cancel->setToolTip(tr("Cancel (Esc)"));
    // A click must not take the keyboard away from the drawing and the
    // command line.
    m_cancel->setFocusPolicy(Qt::NoFocus);
    m_cancel->setEnabled(false);
    connect(m_cancel, &QToolButton::clicked, this, [this] {
        if (m_runner != nullptr) {
            cancel();
        }
    });

    auto* inputRow = new QHBoxLayout(field);
    inputRow->setContentsMargins(2, 0, kInnerMargin, 0);
    inputRow->setSpacing(kInnerMargin);
    inputRow->addWidget(m_cancel);
    inputRow->addWidget(mark);
    inputRow->addWidget(m_prompt);
    inputRow->addWidget(m_input, 1);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(kInnerMargin, kInnerMargin, kInnerMargin, kInnerMargin);
    layout->setSpacing(0);
    layout->addWidget(m_history);
    layout->addWidget(field);

    setFocusProxy(m_input);
    m_input->installEventFilter(this);
    m_input->setEnabled(false);
    m_cancel->setIcon(themedIcon(QString::fromLatin1(kCancelIcon), themeOf(palette())));
    refreshPrompt();
}

void CommandLine::floatOver(QWidget* area) {
    const bool visible = isVisible();
    // A sibling of the area laid over it, not a child: an image of the area
    // (QWidget::grab(), the screenshot of a script) shows the drawing alone.
    setParent(area->parentWidget());
    m_area = area;
    area->installEventFilter(this);
    place();
    if (visible) {
        show();
    }
}

void CommandLine::place() {
    if (m_area == nullptr) {
        return;
    }
    const QRect area = m_area->geometry();
    const int available = std::max(area.width() - (2 * kOuterMargin), 0);
    const int wanted = static_cast<int>(std::lround(area.width() * kWidthRatio));
    const int width = std::clamp(wanted, std::min(kMinimumWidth, available), available);
    const int height = sizeHint().height();
    setGeometry(area.x() + ((area.width() - width) / 2),
                area.y() + area.height() - height - kOuterMargin, width, height);
    raise();
}

void CommandLine::colourOptions(const QTextBlock& block) {
    // Options in square brackets, as AutoCAD writes them ("[Close/Undo]").
    static const QRegularExpression kOption(QStringLiteral(R"(\[[^\]]*\])"));
    QTextCursor cursor(block);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.setCharFormat(QTextCharFormat{});
    QTextCharFormat option;
    option.setForeground(themeTokens(themeOf(palette())).accent);
    for (auto match = kOption.globalMatch(block.text()); match.hasNext();) {
        const QRegularExpressionMatch found = match.next();
        cursor.setPosition(block.position() + static_cast<int>(found.capturedStart()));
        cursor.setPosition(block.position() + static_cast<int>(found.capturedEnd()),
                           QTextCursor::KeepAnchor);
        cursor.mergeCharFormat(option);
    }
}

void CommandLine::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        m_cancel->setIcon(themedIcon(QString::fromLatin1(kCancelIcon), themeOf(palette())));
        for (QTextBlock block = m_history->document()->firstBlock(); block.isValid();
             block = block.next()) {
            colourOptions(block);
        }
    } else if (event->type() == QEvent::FontChange || event->type() == QEvent::StyleChange) {
        place();
    }
}

void CommandLine::attachRunner(cmd::CommandRunner* runner) {
    if (m_runner != nullptr) {
        // A runner that is not shown has nobody to tell anything to.
        m_runner->setMessageSink({});
    }
    m_runner = runner;
    if (m_runner != nullptr) {
        // The guard keeps the sink harmless if the runner outlives this widget.
        m_runner->setMessageSink([self = QPointer<CommandLine>(this)](const std::string& text) {
            if (!self.isNull()) {
                self->appendToHistory(QString::fromStdString(text));
            }
        });
    }
    m_input->setEnabled(m_runner != nullptr);
    m_cancel->setEnabled(m_runner != nullptr);
    refreshPrompt();
}

void CommandLine::setSession(DocumentSession* session) {
    if (session == m_session) {
        return;
    }
    if (m_session != nullptr) {
        m_session->pendingInput().commandLine = m_input->text();
    }
    m_session = session;
    m_input->setText(session != nullptr ? std::exchange(session->pendingInput().commandLine, {})
                                        : QString{});
    attachRunner(session != nullptr ? &session->runner() : nullptr);
}

void CommandLine::forwardKeysFrom(QWidget* widget) {
    widget->installEventFilter(this);
}

void CommandLine::appendToHistory(const QString& line) {
    m_history->appendPlainText(line);
    colourOptions(m_history->document()->lastBlock());
}

bool CommandLine::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_area && (event->type() == QEvent::Resize || event->type() == QEvent::Move)) {
        place();
    }
    const bool keyEvent =
        event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride;
    const auto* key = keyEvent ? dynamic_cast<const QKeyEvent*>(event) : nullptr;
    if (key == nullptr) {
        return QWidget::eventFilter(watched, event);
    }
    if (watched == m_input && key->matches(QKeySequence::Delete)) {
        if (m_input->text().isEmpty()) {
            // As for Undo/Redo, leave ShortcutOverride unaccepted so the
            // window's Erase action can fire, when enabled.
            return true;
        }
        if (event->type() == QEvent::ShortcutOverride) {
            // Keep Delete in QLineEdit while there is text, even when the
            // drawing has a selection and Erase is enabled.
            event->accept();
            return true;
        }
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
    if (m_runner == nullptr) {
        return false;
    }
    switch (event.key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:
            submit();
            return true;
        case Qt::Key_Space:
            if (m_runner->acceptsText()) {
                m_input->setFocus(Qt::OtherFocusReason);
                m_input->insert(QStringLiteral(" "));
            } else {
                submit();
            }
            return true;
        case Qt::Key_Escape:
            cancel();
            return true;
        default:
            return false;
    }
}

bool CommandLine::forwardTextKey(const QKeyEvent& event) {
    if (m_runner == nullptr) {
        return false;
    }
    const QString text = event.text();
    const bool textInput = m_runner->acceptsText() && !text.isEmpty();
    if (!textInput && (text.size() != 1 || !isForwardedCharacter(text.front()))) {
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
    m_runner->submitText(text.toStdString());
    refreshPrompt();
    emit inputHandled();
}

void CommandLine::cancel() {
    m_input->clear();
    m_runner->submit(cmd::CancelInput{});
    refreshPrompt();
    emit inputHandled();
}

void CommandLine::refreshPrompt() {
    m_prompt->setText(m_runner != nullptr ? QString::fromStdString(m_runner->promptText())
                                          : QString{});
}

}  // namespace tg::app
