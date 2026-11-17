// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "assistant_panel.hpp"

#include "action_registry.hpp"
#include "assistant_channel.hpp"
#include "main_window.hpp"
#include "nlohmann_json/json.hpp"
#include "tg/mcp/server.hpp"

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QLabel>
#include <QListWidget>
#include <QLocalSocket>
#include <QPushButton>
#include <QTest>
#include <QToolButton>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

using nlohmann::json;

QString uniqueName() {
    static int count = 0;
    return QStringLiteral("tecnigrafo-panel-test-%1-%2")
        .arg(QCoreApplication::applicationPid())
        .arg(++count);
}

TEST(AssistantPanelTest, SwitchPanelRequestAndConnectionState) {
    const MainWindow window;
    AssistantChannel& channel = window.assistantChannel();
    channel.setServerName(uniqueName());
    QAction* toggle = window.actionRegistry().action(QStringLiteral("assistant.channel"));
    ASSERT_NE(toggle, nullptr);
    EXPECT_FALSE(channel.isRunning());

    toggle->setChecked(true);
    EXPECT_TRUE(channel.isRunning());
    EXPECT_TRUE(toggle->isChecked());

    auto* panel = window.findChild<AssistantPanel*>(QStringLiteral("assistantPanel"));
    ASSERT_NE(panel, nullptr);
    auto* state = panel->findChild<QLabel*>(QStringLiteral("assistantState"));
    auto* requests = panel->findChild<QListWidget*>(QStringLiteral("assistantRequests"));
    ASSERT_NE(state, nullptr);
    ASSERT_NE(requests, nullptr);
    EXPECT_EQ(state->text(), QStringLiteral("On"));

    QLocalSocket client;
    client.connectToServer(channel.serverName());
    ASSERT_TRUE(client.waitForConnected(1000));
    ASSERT_TRUE(QTest::qWaitFor([&channel] { return channel.isConnected(); }));
    EXPECT_EQ(state->text(), QStringLiteral("Connected"));
    auto* button = window.findChild<QToolButton*>(QStringLiteral("assistantButton"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->property("assistantState").toString(), QStringLiteral("connected"));
    toggle->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F12));
    EXPECT_EQ(button->property("assistantState").toString(), QStringLiteral("connected"));
    EXPECT_TRUE(button->toolTip().contains(QStringLiteral("connected")));

    const json initialize{{"jsonrpc", "2.0"},
                          {"id", 0},
                          {"method", "initialize"},
                          {"params",
                           {{"protocolVersion", mcp::kLegacyProtocolVersion},
                            {"capabilities", json::object()},
                            {"clientInfo", {{"name", "test"}, {"version", "1"}}}}}};
    client.write(QByteArray::fromStdString(initialize.dump() + "\n"));
    client.flush();
    ASSERT_TRUE(QTest::qWaitFor([&client] { return client.canReadLine(); }));
    client.readLine();

    const json call{{"jsonrpc", "2.0"},
                    {"id", 1},
                    {"method", "tools/call"},
                    {"params", {{"name", "report"}, {"arguments", json::object()}}}};
    client.write(QByteArray::fromStdString(call.dump() + "\n"));
    client.flush();
    ASSERT_TRUE(QTest::qWaitFor([requests] { return requests->count() == 1; }));
    EXPECT_TRUE(requests->item(0)->text().contains(QStringLiteral("report")));
    EXPECT_TRUE(requests->item(0)->text().contains(QStringLiteral("Succeeded")));

    channel.requestReceived(2, QStringLiteral("run"), QStringLiteral("first\nsecond\nthird"));
    channel.requestFinished(2, false, QStringLiteral("done"));
    ASSERT_EQ(requests->count(), 2);
    EXPECT_TRUE(requests->item(0)->text().contains(QStringLiteral("first\nsecond")));

    panel->findChild<QPushButton*>(QStringLiteral("assistantClear"))->click();
    EXPECT_EQ(requests->count(), 0);
    toggle->setChecked(false);
    EXPECT_FALSE(channel.isRunning());
    EXPECT_EQ(button->property("assistantState").toString(), QStringLiteral("off"));
}

TEST(AssistantPanelTest, CommandQuotesPathsWithSpaces) {
    const QString path = QStringLiteral("folder with spaces/tecnigrafo");
    EXPECT_EQ(assistantCommand(path),
              QLatin1Char('"') + QDir::toNativeSeparators(path) + QStringLiteral("\" --mcp"));
}

TEST(AssistantPanelTest, FailedStartResetsActionAndButton) {
    const QString serverName = uniqueName();
    MainWindow owner;
    owner.assistantChannel().setServerName(serverName);
    ASSERT_TRUE(owner.assistantChannel().start());

    MainWindow window;
    AssistantChannel& channel = window.assistantChannel();
    channel.setServerName(serverName);
    QAction* toggle = window.actionRegistry().action(QStringLiteral("assistant.channel"));
    auto* button = window.findChild<QToolButton*>(QStringLiteral("assistantButton"));
    ASSERT_NE(toggle, nullptr);
    ASSERT_NE(button, nullptr);

    toggle->setChecked(true);
    QCoreApplication::processEvents();
    EXPECT_FALSE(channel.isRunning());
    EXPECT_FALSE(toggle->isChecked());
    EXPECT_FALSE(button->isChecked());
    EXPECT_EQ(button->property("assistantState").toString(), QStringLiteral("off"));
}

}  // namespace
}  // namespace tg::app
