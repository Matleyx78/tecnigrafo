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
    MainWindow window;
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

    panel->findChild<QPushButton*>(QStringLiteral("assistantClear"))->click();
    EXPECT_EQ(requests->count(), 0);
    toggle->setChecked(false);
    EXPECT_FALSE(channel.isRunning());
    EXPECT_EQ(button->property("assistantState").toString(), QStringLiteral("off"));
}

}  // namespace
}  // namespace tg::app
