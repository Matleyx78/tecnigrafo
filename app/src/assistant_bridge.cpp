// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The bridge of `tecnigrafo --mcp` (docs/adr/0051, point 1): MCP on stdin
// and stdout for the assistant, passed on to the channel of the program.

#include "assistant_bridge.hpp"

#include "assistant_channel.hpp"
#include "nlohmann_json/json.hpp"
#include "tg/mcp/server.hpp"
#include "tg/mcp/tool.hpp"
#include "tg/mcp/tool_host.hpp"

#include <QByteArray>
#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QLocalSocket>
#include <QString>
#include <QtGlobal>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#endif

namespace tg::app {
namespace {

using nlohmann::json;

/// How long the bridge waits for a connection to the channel: a local
/// connection succeeds, or is refused, at once.
constexpr int kConnectMilliseconds = 2000;

/// How long the bridge waits for the channel to answer its initialize, and
/// for a line to be written. The channel answers both at once, unless the
/// interface is busy with something long.
constexpr int kOpenMilliseconds = 10000;

/// The id of the bridge's own initialize, which the assistant never sees.
constexpr std::string_view kBridgeId = "tecnigrafo-bridge";

/// The `_meta` key of a modern request (mcp/src/server.cpp).
constexpr std::string_view kVersionKey = "io.modelcontextprotocol/protocolVersion";

/// A tool of the program as the bridge offers it: it describes itself, and
/// a call that reaches it, because the channel did not, answers why.
class Unreachable final : public mcp::Tool {
public:
    Unreachable(std::unique_ptr<mcp::Tool> tool, std::shared_ptr<const std::string> reason)
        : m_tool(std::move(tool)), m_reason(std::move(reason)) {}

    [[nodiscard]] std::string name() const override { return m_tool->name(); }

    [[nodiscard]] std::string description() const override { return m_tool->description(); }

    [[nodiscard]] json inputSchema() const override { return m_tool->inputSchema(); }

    [[nodiscard]] mcp::ToolResult call(const json& /*arguments*/) override {
        return mcp::ToolResult::error(*m_reason);
    }

private:
    std::unique_ptr<mcp::Tool> m_tool;
    std::shared_ptr<const std::string> m_reason;
};

std::vector<std::unique_ptr<mcp::Tool>> bridgeTools(const std::shared_ptr<std::string>& reason) {
    std::vector<std::unique_ptr<mcp::Tool>> tools;
    for (std::unique_ptr<mcp::Tool>& tool : mcp::programTools(nullptr)) {
        tools.push_back(std::make_unique<Unreachable>(std::move(tool), reason));
    }
    return tools;
}

/// What the bridge needs to know of a message of the assistant.
struct Kind {
    std::string method;   ///< empty if there is none, or it is not a string
    bool request{false};  ///< it has an id: it waits for an answer
    bool modern{false};   ///< it carries the protocol version in `_meta`
};

Kind kindOf(std::string_view message) {
    // Not checked yet: no accessor here may throw.
    const json value = json::parse(message, nullptr, /*allow_exceptions=*/false);
    Kind kind;
    if (!value.is_object()) {
        return kind;
    }
    if (const auto method = value.find("method"); method != value.end() && method->is_string()) {
        kind.method = method->get<std::string>();
    }
    kind.request = value.contains("id");
    const auto params = value.find("params");
    if (params != value.end() && params->is_object()) {
        const auto meta = params->find("_meta");
        kind.modern = meta != params->end() && meta->is_object() && meta->contains(kVersionKey);
    }
    return kind;
}

bool isResult(std::string_view reply) {
    const json value = json::parse(reply, nullptr, /*allow_exceptions=*/false);
    return value.is_object() && value.contains("result");
}

}  // namespace

AssistantBridge::AssistantBridge(QString serverName)
    : m_name(std::move(serverName)),
      m_reason(std::make_shared<std::string>(mcp::kNotRunning)),
      m_local(bridgeTools(m_reason)) {}

AssistantBridge::~AssistantBridge() {
    disconnectChannel();
}

int AssistantBridge::run(mcp::LineTransport& client, std::ostream& log) {
    const std::size_t limit = mcp::ServerOptions{}.maxMessageBytes;
    std::string line;
    for (;;) {
        const mcp::LineTransport::Read status = client.readLine(line, limit);
        if (status == mcp::LineTransport::Read::End) {
            disconnectChannel();
            return 0;
        }
        // A message longer than the limit is refused by the bridge's own
        // server, which tells it by its size: the transport has dropped it,
        // and one byte more than the limit stands for it.
        const std::optional<std::string> reply =
            status == mcp::LineTransport::Read::TooLong
                ? m_local.answer(std::string(limit + 1, ' '), log)
                : answer(line, log);
        if (reply && !client.writeLine(*reply)) {
            log << "tecnigrafo --mcp: cannot write the answer\n";
            disconnectChannel();
            return 1;
        }
    }
}

std::optional<std::string> AssistantBridge::answer(std::string_view message, std::ostream& log) {
    Kind kind;
    if (message.size() <= mcp::ServerOptions{}.maxMessageBytes) {
        std::string_view body = message;
        if (body.ends_with('\r')) {
            body.remove_suffix(1);
        }
        kind = kindOf(body);
        // A legacy client calls a tool after its initialize; before it, the
        // bridge's own server says so, as tecnigrafo-mcp would.
        const bool open = m_initialized || kind.modern;
        if (kind.method == "tools/call" && kind.request && open) {
            if (std::optional<std::string> reply = forward(body, log)) {
                return reply;
            }
            // The bridge's own tool answers with the reason.
        } else if (!kind.request && !kind.method.empty() && isConnected() && !writeLine(body)) {
            disconnectChannel();
        }
    }
    std::optional<std::string> reply = m_local.answer(message, log);
    if (kind.method == "initialize" && reply && isResult(*reply)) {
        m_initialized = true;
    }
    return reply;
}

bool AssistantBridge::isConnected() const {
    return m_socket != nullptr && m_socket->state() == QLocalSocket::ConnectedState;
}

std::optional<std::string> AssistantBridge::forward(std::string_view message, std::ostream& log) {
    if (m_socket != nullptr) {
        // A connection that the channel closed meanwhile (turned off, or the
        // program closed) is noticed now, before a request is written on it.
        // The channel writes nothing between two answers.
        if (m_socket->waitForReadyRead(0)) {
            const QByteArray stray = m_socket->readAll();
            log << "tecnigrafo --mcp: " << stray.size() << " bytes of the channel ignored\n";
        }
        if (m_socket->state() != QLocalSocket::ConnectedState) {
            disconnectChannel();
        }
    }
    bool fresh = false;
    if (m_socket == nullptr) {
        if (!connectChannel(log)) {
            return std::nullopt;
        }
        fresh = true;
    }
    bool written = writeLine(message);
    if (!written && !fresh) {
        // An old connection that the check above did not see closed: the
        // request never left, so it goes once more on a new one.
        disconnectChannel();
        written = connectChannel(log) && writeLine(message);
    }
    if (!written) {
        if (m_socket != nullptr) {
            log << "tecnigrafo --mcp: the connection to the program dropped\n";
            *m_reason = mcp::kNotRunning;
            disconnectChannel();
        }
        return std::nullopt;
    }
    // The program answers when the user is at "Command:", or after the wait
    // of the queue: no limit here, only the end of the connection.
    std::optional<std::string> reply = readLine(-1);
    if (!reply) {
        log << "tecnigrafo --mcp: the connection to the program dropped\n";
        *m_reason = mcp::kNotRunning;
        disconnectChannel();
    }
    return reply;
}

bool AssistantBridge::connectChannel(std::ostream& log) {
    *m_reason = mcp::kNotRunning;
    m_socket = std::make_unique<QLocalSocket>();
    m_socket->connectToServer(m_name);
    if (!m_socket->waitForConnected(kConnectMilliseconds)) {
        m_socket.reset();
        return false;
    }
    // The conversation with the channel is the bridge's own: the assistant
    // has opened its own with the bridge.
    const json initialize{
        {"jsonrpc", "2.0"},
        {"id", kBridgeId},
        {"method", "initialize"},
        {"params",
         json{{"protocolVersion", mcp::kLegacyProtocolVersion},
              {"capabilities", json::object()},
              {"clientInfo",
               json{{"name", "tecnigrafo-bridge"},
                    {"version", QCoreApplication::applicationVersion().toStdString()}}}}}};
    const std::optional<std::string> reply =
        writeLine(initialize.dump()) ? readLine(kOpenMilliseconds) : std::nullopt;
    const json answer =
        reply ? json::parse(*reply, nullptr, /*allow_exceptions=*/false) : json(nullptr);
    if (!answer.is_object() || !answer.contains("result")) {
        // A channel that refuses says why: kAnotherAssistant.
        if (answer.is_object()) {
            const auto error = answer.find("error");
            if (error != answer.end() && error->is_object()) {
                const auto text = error->find("message");
                if (text != error->end() && text->is_string()) {
                    *m_reason = text->get<std::string>();
                }
            }
        }
        log << "tecnigrafo --mcp: the program refused the connection: " << *m_reason << "\n";
        disconnectChannel();
        return false;
    }
    const json initialized{{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}};
    if (!writeLine(initialized.dump())) {
        disconnectChannel();
        return false;
    }
    return true;
}

void AssistantBridge::disconnectChannel() {
    if (m_socket != nullptr) {
        m_socket->abort();
        m_socket.reset();
    }
}

bool AssistantBridge::writeLine(std::string_view line) {
    if (m_socket == nullptr) {
        return false;
    }
    std::string text{line};
    text += '\n';
    const auto size = static_cast<qint64>(text.size());
    if (m_socket->write(text.data(), size) != size) {
        return false;
    }
    // Without an event loop, the bytes leave only while the bridge waits.
    while (m_socket->bytesToWrite() > 0) {
        if (!m_socket->waitForBytesWritten(kOpenMilliseconds)) {
            return false;
        }
    }
    return true;
}

std::optional<std::string> AssistantBridge::readLine(int milliseconds) {
    if (m_socket == nullptr) {
        return std::nullopt;
    }
    const QDeadlineTimer deadline =
        milliseconds < 0 ? QDeadlineTimer(QDeadlineTimer::Forever) : QDeadlineTimer(milliseconds);
    while (!m_socket->canReadLine()) {
        if (m_socket->state() != QLocalSocket::ConnectedState) {
            return std::nullopt;
        }
        // -1 waits without limit; a finite wait never exceeds an int.
        const qint64 remaining = deadline.remainingTime();
        const int wait =
            static_cast<int>(std::min<qint64>(remaining, std::numeric_limits<int>::max()));
        if (!m_socket->waitForReadyRead(wait) && !m_socket->canReadLine()) {
            return std::nullopt;
        }
    }
    QByteArray line = m_socket->readLine();
    if (line.endsWith('\n')) {
        line.chop(1);
    }
    return std::string(line.constData(), static_cast<std::size_t>(line.size()));
}

int runAssistantBridge() {
#ifdef _WIN32
    // One message per line ends with '\n' alone, never "\r\n".
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    AssistantBridge bridge(AssistantChannel::defaultServerName());
    mcp::StreamTransport transport{std::cin, std::cout};
    return bridge.run(transport, std::cerr);
}

}  // namespace tg::app
