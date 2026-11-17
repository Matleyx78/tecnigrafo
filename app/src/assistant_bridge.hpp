// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/mcp/server.hpp"

#include <QString>

#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class QLocalSocket;

namespace tg::app {

/// The bridge that stays alive (docs/adr/0051, point 1): what
/// `tecnigrafo --mcp` runs, without windows, between the assistant, on a
/// transport of lines (stdin and stdout), and the channel of the program
/// (AssistantChannel), until the transport ends.
///
/// It answers initialize, ping, server/discover and tools/list by itself,
/// with the tools of the program (mcp::programTools(nullptr)), which it
/// knows because it is the same program, before it reaches the program. A
/// request tools/call goes to the channel as it is, and its answer comes
/// back as it is: the bridge connects to the channel when it is not
/// connected, opening the conversation there with an initialize of its own.
/// If the channel is not there, or the connection drops, that call answers
/// the error of the tool mcp::kNotRunning (or the error of the channel, as
/// kAnotherAssistant), and the next call tries again. Notifications go to
/// the channel only while connected.
///
/// The bridge waits for the channel with the blocking calls of QLocalSocket:
/// it needs a QCoreApplication, never its event loop, and may run in a
/// thread of its own; it must be destroyed in the thread that used it.
class AssistantBridge {
public:
    /// A bridge to the channel called `serverName`
    /// (AssistantChannel::defaultServerName() for the program).
    explicit AssistantBridge(QString serverName);
    ~AssistantBridge();

    AssistantBridge(const AssistantBridge&) = delete;
    AssistantBridge& operator=(const AssistantBridge&) = delete;
    AssistantBridge(AssistantBridge&&) = delete;
    AssistantBridge& operator=(AssistantBridge&&) = delete;

    /// Serves `client` until its end. Returns 0, or 1 if a write failed.
    int run(mcp::LineTransport& client, std::ostream& log);

    /// The answer to one message of the assistant, as run() writes it, or
    /// std::nullopt for none (a notification). `message` is what comes
    /// before the "\n", as for mcp::Server::answer().
    [[nodiscard]] std::optional<std::string> answer(std::string_view message, std::ostream& log);

    /// Whether the bridge is connected to the channel now.
    [[nodiscard]] bool isConnected() const;

private:
    /// The answer of the channel to the request `message`; std::nullopt if
    /// the channel is not there or the connection dropped (m_reason says
    /// why).
    [[nodiscard]] std::optional<std::string> forward(std::string_view message, std::ostream& log);
    /// Connects and opens the conversation with the channel.
    [[nodiscard]] bool connectChannel(std::ostream& log);
    void disconnectChannel();
    [[nodiscard]] bool writeLine(std::string_view line);
    /// The next line of the channel, without its "\n"; std::nullopt if the
    /// connection drops first, or after `milliseconds` (-1: no limit).
    [[nodiscard]] std::optional<std::string> readLine(int milliseconds);

    QString m_name;
    /// Why a call does not reach the program: mcp::kNotRunning, or the error
    /// of the channel that refused the bridge.
    std::shared_ptr<std::string> m_reason;
    /// The bridge's own server: initialize, tools/list, and the error of a
    /// call that cannot be passed on.
    mcp::Server m_local;
    std::unique_ptr<QLocalSocket> m_socket;
    /// The assistant opened its conversation with initialize: a legacy
    /// tools/call goes to the program only after it.
    bool m_initialized{false};
};

/// `tecnigrafo --mcp`: the bridge on stdin and stdout, with logs on stderr,
/// to the channel of this user. Returns when stdin ends: 0, or 1 if stdout
/// failed. Needs a QCoreApplication.
[[nodiscard]] int runAssistantBridge();

}  // namespace tg::app
