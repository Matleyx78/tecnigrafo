// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "text_format_bar.hpp"

#include "text_editor_overlay.hpp"
#include "tg/core/color.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext_edit.hpp"
#include "theme.hpp"

#include <QColor>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QToolButton>

#include <algorithm>
#include <cstdint>

namespace tg::app {
namespace {

using Kind = core::MTextFormatChange::Kind;

core::Color colorFromText(const QString& text, bool& valid) {
    const QString trimmed = text.trimmed();
    if (trimmed.startsWith(QLatin1Char('#')) &&
        QRegularExpression(QStringLiteral("^#[0-9A-Fa-f]{6}$")).match(trimmed).hasMatch()) {
        const QColor color(trimmed);
        valid = color.isValid();
        return core::Color::fromRgb({static_cast<std::uint8_t>(color.red()),
                                     static_cast<std::uint8_t>(color.green()),
                                     static_cast<std::uint8_t>(color.blue())});
    }
    bool numberValid = false;
    const int index = trimmed.toInt(&numberValid);
    valid = numberValid && index >= 0 && index <= 256;
    if (index == 0) {
        return core::Color::byBlock();
    }
    if (index == 256) {
        return core::Color::byLayer();
    }
    return core::Color::fromIndex(std::clamp(index, 1, 255));
}

}  // namespace

QStringList textFormatBarIconNames() {
    return {QStringLiteral("format-bold"),      QStringLiteral("format-italic"),
            QStringLiteral("format-underline"), QStringLiteral("format-overline"),
            QStringLiteral("format-strike"),    QStringLiteral("format-stack"),
            QStringLiteral("format-unstack")};
}

TextFormatBar::TextFormatBar(TextEditorOverlay* editor) : QWidget(editor), m_editor(editor) {
    setObjectName(QStringLiteral("textFormatBar"));
    setAutoFillBackground(true);
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 3, 4, 3);
    layout->setSpacing(2);

    m_bold = addToggle(QStringLiteral("formatBold"), tr("B"), QStringLiteral("format-bold"));
    m_italic = addToggle(QStringLiteral("formatItalic"), tr("I"), QStringLiteral("format-italic"));
    m_underline =
        addToggle(QStringLiteral("formatUnderline"), tr("U"), QStringLiteral("format-underline"));
    m_overline =
        addToggle(QStringLiteral("formatOverline"), tr("O"), QStringLiteral("format-overline"));
    m_strike = addToggle(QStringLiteral("formatStrike"), tr("S"), QStringLiteral("format-strike"));

    m_height = addNumber(QStringLiteral("formatHeight"));
    m_height->setToolTip(tr("Height"));
    m_color = new QLineEdit(QStringLiteral("256"), this);
    m_color->setObjectName(QStringLiteral("formatColor"));
    m_color->setToolTip(tr("Colour: ACI index or #RRGGBB"));
    m_color->setMaximumWidth(76);
    layout->addWidget(m_color);
    m_width = addNumber(QStringLiteral("formatWidth"));
    m_width->setToolTip(tr("Width factor"));
    m_oblique = addNumber(QStringLiteral("formatOblique"), QStringLiteral("°"));
    m_oblique->setToolTip(tr("Oblique angle"));
    m_tracking = addNumber(QStringLiteral("formatTracking"));
    m_tracking->setToolTip(tr("Character spacing"));

    auto* stack =
        addToggle(QStringLiteral("formatStack"), tr("Stack"), QStringLiteral("format-stack"));
    stack->setCheckable(false);
    auto* unstack =
        addToggle(QStringLiteral("formatUnstack"), tr("Unstack"), QStringLiteral("format-unstack"));
    unstack->setCheckable(false);

    applyToggle(m_bold, static_cast<int>(Kind::Bold));
    applyToggle(m_italic, static_cast<int>(Kind::Italic));
    applyToggle(m_underline, static_cast<int>(Kind::Underline));
    applyToggle(m_overline, static_cast<int>(Kind::Overline));
    applyToggle(m_strike, static_cast<int>(Kind::Strike));
    const auto numberChange = [this](QDoubleSpinBox* field, Kind kind) {
        connect(field, &QDoubleSpinBox::valueChanged, this,
                [this, field] { m_changedNumbers.insert(field); });
        connect(field, &QDoubleSpinBox::editingFinished, this, [this, field, kind] {
            if (m_changedNumbers.remove(field)) {
                m_editor->applyFormat({kind, true, field->value()});
            }
        });
    };
    numberChange(m_height, Kind::Height);
    numberChange(m_width, Kind::WidthFactor);
    numberChange(m_oblique, Kind::Oblique);
    numberChange(m_tracking, Kind::Tracking);
    connect(m_color, &QLineEdit::editingFinished, this, [this] {
        bool valid = false;
        const core::Color color = colorFromText(m_color->text(), valid);
        if (valid) {
            core::MTextFormatChange change;
            change.kind = Kind::Color;
            change.color = color;
            m_editor->applyFormat(change);
        }
    });
    connect(stack, &QToolButton::clicked, this,
            [this] { m_editor->applyFormat({Kind::Stack, true}); });
    connect(unstack, &QToolButton::clicked, this,
            [this] { m_editor->applyFormat({Kind::Stack, false}); });
}

void TextFormatBar::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        event->accept();
        m_editor->setFocus(Qt::OtherFocusReason);
        return;
    }
    QWidget::keyPressEvent(event);
}

QToolButton* TextFormatBar::addToggle(const QString& name, const QString& text,
                                      const QString& iconName) {
    auto* button = new QToolButton(this);
    button->setObjectName(name);
    button->setText(text);
    button->setToolTip(text);
    button->setIcon(themedIcon(iconName, themeOf(palette())));
    button->setCheckable(true);
    button->setAutoRaise(true);
    layout()->addWidget(button);
    return button;
}

QDoubleSpinBox* TextFormatBar::addNumber(const QString& name, const QString& suffix) {
    auto* field = new QDoubleSpinBox(this);
    field->setObjectName(name);
    field->setDecimals(3);
    field->setRange(name == QStringLiteral("formatOblique") ? -90.0 : 0.001, 1'000'000.0);
    field->setSuffix(suffix);
    field->setMaximumWidth(82);
    layout()->addWidget(field);
    return field;
}

void TextFormatBar::applyToggle(QToolButton* button, int kind) {
    connect(button, &QToolButton::clicked, this, [this, kind](bool checked) {
        m_editor->applyFormat({static_cast<core::MTextFormatChange::Kind>(kind), checked});
    });
}

void TextFormatBar::refresh() {
    m_changedNumbers.clear();
    const core::MTextEditModel* model = m_editor->model();
    setEnabled(model != nullptr);
    if (model == nullptr) {
        return;
    }
    const core::MTextFormat format = model->formatAtCaret();
    m_bold->setChecked(format.bold);
    m_italic->setChecked(format.italic);
    m_underline->setChecked(format.underline);
    m_overline->setChecked(format.overline);
    m_strike->setChecked(format.strike);
    const QSignalBlocker heightBlocker(m_height);
    const QSignalBlocker widthBlocker(m_width);
    const QSignalBlocker obliqueBlocker(m_oblique);
    const QSignalBlocker trackingBlocker(m_tracking);
    m_height->setValue(format.height);
    m_width->setValue(format.widthFactor);
    m_oblique->setValue(core::radiansToDegrees(format.oblique));
    m_tracking->setValue(format.tracking);
}

}  // namespace tg::app
