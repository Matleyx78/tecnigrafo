// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QSet>
#include <QStringList>
#include <QWidget>

class QDoubleSpinBox;
class QKeyEvent;
class QLineEdit;
class QToolButton;

namespace tg::app {

class TextEditorOverlay;

/// The icons used directly by the text format bar: no action names them.
[[nodiscard]] QStringList textFormatBarIconNames();

/// The formatting controls shown above an open multiline text editor.
class TextFormatBar final : public QWidget {
    Q_OBJECT

public:
    explicit TextFormatBar(TextEditorOverlay* editor);

    /// Refreshes the checked controls from the format at the editor caret.
    void refresh();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    QToolButton* addToggle(const QString& name, const QString& text, const QString& iconName);
    QDoubleSpinBox* addNumber(const QString& name, const QString& suffix = {});
    void applyToggle(QToolButton* button, int kind);

    TextEditorOverlay* m_editor;
    QToolButton* m_bold;
    QToolButton* m_italic;
    QToolButton* m_underline;
    QToolButton* m_overline;
    QToolButton* m_strike;
    QDoubleSpinBox* m_height;
    QLineEdit* m_color;
    QDoubleSpinBox* m_width;
    QDoubleSpinBox* m_oblique;
    QDoubleSpinBox* m_tracking;
    QSet<QDoubleSpinBox*> m_changedNumbers;
};

}  // namespace tg::app
