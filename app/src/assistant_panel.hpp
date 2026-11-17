// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QDockWidget>
#include <QHash>

class QLabel;
class QListWidget;
class QPushButton;

namespace tg::app {

class AssistantChannel;

/// The visible history and setup information of the user's assistant channel.
class AssistantPanel final : public QDockWidget {
    Q_OBJECT

public:
    explicit AssistantPanel(AssistantChannel& channel, QWidget* parent = nullptr);
    ~AssistantPanel() override = default;

    Q_DISABLE_COPY_MOVE(AssistantPanel)

    void showAndFocus();

private:
    void updateState();
    void addRequest(quint64 request, const QString& tool, const QString& text);
    void finishRequest(quint64 request, bool failed, const QString& text);

    AssistantChannel& m_channel;
    QLabel* m_state;
    QLabel* m_command;
    QListWidget* m_requests;
    QPushButton* m_clear;
    QHash<quint64, int> m_rows;
};

}  // namespace tg::app
