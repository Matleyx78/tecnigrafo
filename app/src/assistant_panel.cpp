// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "assistant_panel.hpp"

#include "assistant_channel.hpp"

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace tg::app {

QString assistantCommand(const QString& applicationPath) {
    QString executable = QDir::toNativeSeparators(applicationPath);
    if (executable.contains(QLatin1Char(' '))) {
        executable = QLatin1Char('"') + executable + QLatin1Char('"');
    }
    return executable + QStringLiteral(" --mcp");
}

AssistantPanel::AssistantPanel(AssistantChannel& channel, QWidget* parent)
    : QDockWidget(tr("Assistant"), parent),
      m_channel(channel),
      m_state(new QLabel(this)),
      m_command(new QLabel(this)),
      m_requests(new QListWidget(this)),
      m_clear(new QPushButton(tr("Clear"), this)) {
    setObjectName(QStringLiteral("assistantPanel"));
    m_state->setObjectName(QStringLiteral("assistantState"));
    m_command->setObjectName(QStringLiteral("assistantCommand"));
    m_command->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_command->setText(assistantCommand(QCoreApplication::applicationFilePath()));
    m_requests->setObjectName(QStringLiteral("assistantRequests"));
    m_requests->setWordWrap(true);
    m_clear->setObjectName(QStringLiteral("assistantClear"));

    auto* copy = new QPushButton(tr("Copy"), this);
    copy->setObjectName(QStringLiteral("assistantCopy"));
    auto* commandRow = new QHBoxLayout;
    commandRow->addWidget(m_command, 1);
    commandRow->addWidget(copy);
    auto* content = new QWidget(this);
    auto* layout = new QVBoxLayout(content);
    layout->addWidget(m_state);
    layout->addWidget(new QLabel(tr("Assistant command:"), content));
    layout->addLayout(commandRow);
    layout->addWidget(m_requests, 1);
    layout->addWidget(m_clear, 0, Qt::AlignRight);
    setWidget(content);

    connect(copy, &QPushButton::clicked, this,
            [this] { QApplication::clipboard()->setText(m_command->text()); });
    connect(m_clear, &QPushButton::clicked, this, [this] {
        m_requests->clear();
        m_rows.clear();
    });
    connect(&channel, &AssistantChannel::runningChanged, this, [this] { updateState(); });
    connect(&channel, &AssistantChannel::connectedChanged, this, [this] { updateState(); });
    connect(&channel, &AssistantChannel::requestReceived, this, &AssistantPanel::addRequest);
    connect(&channel, &AssistantChannel::requestFinished, this, &AssistantPanel::finishRequest);
    updateState();
}

void AssistantPanel::updateState() {
    if (m_channel.isConnected()) {
        m_state->setText(tr("Connected"));
    } else if (m_channel.isRunning()) {
        m_state->setText(tr("On"));
    } else {
        m_state->setText(tr("Off"));
    }
}

void AssistantPanel::addRequest(quint64 request, const QString& tool, const QString& text) {
    QString preview = text.section(QLatin1Char('\n'), 0, 1).trimmed();
    if (preview.isEmpty()) {
        preview = tr("(no arguments)");
    }
    const QString requestPreview =
        tr("%1 — %2\n%3")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")), tool, preview);
    m_requests->insertItem(0, requestPreview + QLatin1Char('\n') + tr("Pending"));
    for (auto row = m_rows.begin(); row != m_rows.end(); ++row) {
        ++row.value().index;
    }
    m_rows.insert(request, {0, requestPreview});
}

void AssistantPanel::finishRequest(quint64 request, bool failed, const QString& text) {
    const auto found = m_rows.constFind(request);
    if (found == m_rows.cend()) {
        return;
    }
    QListWidgetItem* item = m_requests->item(found->index);
    const QString outcome = failed ? tr("Failed") : tr("Succeeded");
    const QString detail = text.section(QLatin1Char('\n'), 0, 1).trimmed();
    item->setText(found->preview + QLatin1Char('\n') + outcome +
                  (detail.isEmpty() ? QString{} : QStringLiteral(": ") + detail));
}

void AssistantPanel::showAndFocus() {
    show();
    raise();
    m_requests->setFocus(Qt::OtherFocusReason);
}

}  // namespace tg::app
