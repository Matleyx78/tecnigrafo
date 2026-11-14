// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dynamic_input_box.hpp"

#include "theme.hpp"

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace tg::app {
namespace {

// Keep the overlay clear of the crosshair, including on high-DPI displays.
constexpr int kCursorOffset = 16;
constexpr int kFieldWidth = 90;
// The space inside the box and between its rows (docs/adr/0039, area 7).
constexpr int kBoxMargin = 8;
constexpr int kBoxSpacing = 6;

QString fieldName(cmd::DynamicField::Kind kind) {
    switch (kind) {
        case cmd::DynamicField::Kind::X:
            return QStringLiteral("X");
        case cmd::DynamicField::Kind::Y:
            return QStringLiteral("Y");
        case cmd::DynamicField::Kind::Length:
            return QStringLiteral("Length");
        case cmd::DynamicField::Kind::Angle:
            return QStringLiteral("Angle");
        case cmd::DynamicField::Kind::Value:
            return QStringLiteral("Value");
    }
    return {};
}

}  // namespace

DynamicInputBox::DynamicInputBox(QWidget* parent) : QWidget(parent), m_prompt(new QLabel(this)) {
    assert(parent != nullptr && "dynamic input needs a parent drawing view");
    setObjectName(QStringLiteral("dynamicInputBox"));
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    // The style sheet draws the box: surface.panel, a border, round corners.
    setAttribute(Qt::WA_StyledBackground);
    m_prompt->setObjectName(QStringLiteral("dynamicPrompt"));
    m_prompt->setTextFormat(Qt::PlainText);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(kBoxMargin, kBoxMargin, kBoxMargin, kBoxMargin);
    layout->setSpacing(kBoxSpacing);
    layout->addWidget(m_prompt);
    auto* fields = new QHBoxLayout;
    layout->addLayout(fields);
    // The model has at most two fields (ADR 0042, point 2).
    constexpr std::size_t kMaxFields = 2;
    for (std::size_t index = 0; index < kMaxFields; ++index) {
        auto* field = new QLineEdit(this);
        field->setObjectName(QStringLiteral("dynamicField%1").arg(index));
        field->setReadOnly(true);
        field->setFocusPolicy(Qt::NoFocus);
        field->setMinimumWidth(kFieldWidth);
        // Numbers at fixed pitch, as in the reference.
        field->setFont(fixedFont());
        // The fields fill the row, equally wide (the responsible's choice,
        // docs/tappe/fuori-tappa/F6-stato.md, area 7).
        fields->addWidget(field, 1);
        m_fields.push_back(field);
    }
    hide();
}

void DynamicInputBox::setSession(DocumentSession* session) {
    if (session == m_session) {
        return;
    }
    if (m_session != nullptr) {
        m_session->pendingInput().dynamicInput = m_input;
    }
    m_session = session;
    m_input = {};
    if (m_session != nullptr) {
        // The runner of a session that is not shown receives nothing: what
        // its fields held still answers its prompt.
        auto& saved = m_session->pendingInput().dynamicInput;
        m_input = saved.has_value() ? *saved : cmd::dynamicInputFor(m_session->runner());
        saved.reset();
    }
    updateFields();
}

bool DynamicInputBox::commandRunning() const {
    return m_session != nullptr && m_session->runner().isActive();
}

void DynamicInputBox::routeKeysFrom(QWidget* view, QLineEdit* commandInput) {
    m_commandInput = commandInput;
    view->installEventFilter(this);
}

void DynamicInputBox::setInputEnabled(bool enabled) {
    m_enabled = enabled;
    refresh();
}

void DynamicInputBox::refresh(bool clearTyped) {
    if (clearTyped) {
        m_input =
            m_session != nullptr ? cmd::dynamicInputFor(m_session->runner()) : cmd::DynamicInput{};
    }
    updateFields();
}

void DynamicInputBox::updatePointer(core::Vec2 pointer, QPoint screenPosition) {
    m_pointer = pointer;
    m_screenPosition = screenPosition;
    updateFields();
}

void DynamicInputBox::updateFields() {
    if (!m_enabled || !commandRunning()) {
        hide();
        return;
    }
    const QString prompt = QString::fromStdString(m_input.prompt);
    m_prompt->setText(prompt);
    // On one line when the drawing view has room for it; long prompts wrap
    // and stay within even a narrow view.
    m_prompt->setWordWrap(true);
    const int room = std::max(1, parentWidget()->width() - (2 * kBoxMargin) - 8);
    m_prompt->setMaximumWidth(room);
    m_prompt->setMinimumWidth(
        std::min(m_prompt->fontMetrics().horizontalAdvance(prompt) + 1, room));
    const auto shown = cmd::dynamicInputShown(m_input, m_session->runner(), m_pointer);
    const auto& tokens = themeTokens(themeOf(palette()));
    for (std::size_t index = 0; index < m_fields.size(); ++index) {
        QLineEdit* field = m_fields[index];
        const bool exists = index < shown.size();
        field->setVisible(exists);
        if (exists) {
            field->setText(QString::fromStdString(shown[index]));
            field->setAccessibleName(fieldName(m_input.fields[index].kind));
            // The active field, border and text, in the colour of the points
            // of the icons (iconPt, docs/adr/0040), the responsible's choice.
            const bool active = index == m_input.active;
            field->setStyleSheet(
                QStringLiteral("QLineEdit { border: 1px solid %1; border-radius: 2px; "
                               "padding: 1px 4px; background: %2; color: %3; }")
                    .arg((active ? tokens.iconPt : tokens.border).name(), tokens.field.name(),
                         (active ? tokens.iconPt : tokens.text).name()));
        }
    }
    adjustSize();
    // Clip to the parent even if a long prompt exceeds its height.
    resize(std::min(width(), parentWidget()->width()),
           std::min(height(), parentWidget()->height()));
    move(std::clamp(m_screenPosition.x() + kCursorOffset, 0, parentWidget()->width() - width()),
         std::clamp(m_screenPosition.y() + kCursorOffset, 0, parentWidget()->height() - height()));
    show();
    raise();
}

bool DynamicInputBox::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() != QEvent::KeyPress || !isVisible() || m_session == nullptr ||
        m_commandInput == nullptr || !m_commandInput->text().isEmpty()) {
        return QWidget::eventFilter(watched, event);
    }
    const auto* key = dynamic_cast<QKeyEvent*>(event);
    if (key == nullptr ||
        key->modifiers().testAnyFlags(Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) {
        return false;
    }
    if (key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
        const QString text =
            QString::fromStdString(cmd::dynamicInputText(m_input, m_session->runner(), m_pointer));
        emit textSubmitted(text);
        refresh();
        return true;
    }
    if (m_input.fields.empty()) {
        return false;
    }
    if (key->key() == Qt::Key_Tab) {
        m_input.active = (m_input.active + 1) % m_input.fields.size();
        updateFields();
        return true;
    }
    if (key->key() == Qt::Key_Backspace) {
        auto& typed = m_input.fields[m_input.active].typed;
        if (!typed.empty()) {
            typed.pop_back();
        }
        updateFields();
        return true;
    }
    const QString text = key->text();
    if (text.size() == 1 && QStringView(u"0123456789.-,").contains(text.front())) {
        m_input.fields[m_input.active].typed += text.toStdString();
        updateFields();
        return true;
    }
    return false;
}

}  // namespace tg::app
