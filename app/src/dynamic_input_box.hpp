// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "document_session.hpp"
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
///
/// It follows the runner of the session of setSession(); without a session
/// it stays hidden and takes no keys. What was typed in the fields, with the
/// active field, belongs to the session (decision B2 of docs/DECISIONI.md):
/// setSession() leaves it in the session it leaves and takes back what the
/// new one kept.
class DynamicInputBox final : public QWidget {
    Q_OBJECT

public:
    /// The parent drawing view must be non-null and outlive the overlay.
    explicit DynamicInputBox(QWidget* parent);

    /// Follows `session` from now on; nullptr for none. The session is
    /// borrowed: the owner calls setSession() again before destroying it.
    void setSession(DocumentSession* session);

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

    /// True with a session whose runner runs a command.
    [[nodiscard]] bool commandRunning() const;

    DocumentSession* m_session{nullptr};
    cmd::DynamicInput m_input;
    core::Vec2 m_pointer;
    QPoint m_screenPosition;
    QLabel* m_prompt{nullptr};
    std::vector<QLineEdit*> m_fields;
    QLineEdit* m_commandInput{nullptr};
    bool m_enabled{true};
};

}  // namespace tg::app
