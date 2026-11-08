// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/dynamic_input.hpp"

#include <QPoint>
#include <QWidget>

#include <vector>

class QLabel;
class QLineEdit;

namespace tg::app {

/// Non-focusable overlay: only keys from the drawing view edit its fields.
/// Explicit command-line input keeps its usual interpretation. Unhandled
/// keys continue through CommandLine's event filter.
class DynamicInputBox final : public QWidget {
    Q_OBJECT

public:
    /// The parent drawing view must be non-null and outlive the overlay.
    DynamicInputBox(const cmd::CommandRunner& runner, QWidget* parent);

    /// Install after CommandLine::forwardKeysFrom(), so numbers reach us first.
    void routeKeysFrom(QWidget* view, QLineEdit* commandInput);
    void setInputEnabled(bool enabled);
    /// Called after a runner submission: no typed values cross prompt boundaries.
    void refresh(bool clearTyped = true);
    /// Raw world position; clickPoint() applies the runner's current drawing aids.
    void updatePointer(core::Vec2 pointer, QPoint screenPosition);

signals:
    void textSubmitted(const QString& text);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void updateFields();

    const cmd::CommandRunner& m_runner;
    cmd::DynamicInput m_input;
    core::Vec2 m_pointer;
    QPoint m_screenPosition;
    QLabel* m_prompt{nullptr};
    std::vector<QLineEdit*> m_fields;
    QLineEdit* m_commandInput{nullptr};
    bool m_enabled{true};
};

}  // namespace tg::app
