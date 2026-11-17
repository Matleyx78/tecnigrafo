// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The channel of the user's assistant, its common queue with the user and
// the bridge of `tecnigrafo --mcp` (docs/adr/0051; docs/tappe/0.8/PT37),
// with a QLocalSocket as the client.

#include "assistant_channel.hpp"

#include "assistant_bridge.hpp"
#include "command_line.hpp"
#include "document_session.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "nlohmann_json/json.hpp"
#include "tg/mcp/server.hpp"
#include "tg/mcp/tool_host.hpp"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QLocalSocket>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSignalSpy>
#include <QString>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::app {
namespace {

using nlohmann::json;

/// How long a test waits for an answer that must come.
constexpr int kAnswerMilliseconds = 10000;
/// How long a test waits to see that an answer does not come.
constexpr int kSilenceMilliseconds = 300;

/// A name of its own for every channel of the tests, so that they never
/// meet the program of the user, nor each other.
QString uniqueName() {
    static int count = 0;
    return QStringLiteral("tecnigrafo-test-%1-%2")
        .arg(QCoreApplication::applicationPid())
        .arg(++count);
}

json request(int id, std::string_view method, json params) {
    return json{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", std::move(params)}};
}

json initializeRequest(int id) {
    return request(id, "initialize",
                   json{{"protocolVersion", mcp::kLegacyProtocolVersion},
                        {"capabilities", json::object()},
                        {"clientInfo", json{{"name", "test"}, {"version", "1"}}}});
}

json callRequest(int id, std::string_view tool, json arguments = json::object()) {
    return request(id, "tools/call", json{{"name", tool}, {"arguments", std::move(arguments)}});
}

json runRequest(int id, const std::vector<std::string>& lines) {
    return callRequest(id, "run", json{{"lines", lines}});
}

/// The text of the result of a tool call; empty for anything else.
std::string toolText(const json& reply) {
    const auto result = reply.find("result");
    if (result == reply.end() || !result->contains("content")) {
        return {};
    }
    return result->at("content").at(0).value("text", "");
}

bool toolFailed(const json& reply) {
    const auto result = reply.find("result");
    return result != reply.end() && result->value("isError", false);
}

std::vector<std::string> toolNames(const json& reply) {
    std::vector<std::string> names;
    for (const json& tool : reply.at("result").at("tools")) {
        names.push_back(tool.at("name").get<std::string>());
    }
    return names;
}

const std::vector<std::string> kProgramTools{
    "list_commands", "describe_command", "run", "report", "open", "save"};

/// A client of the channel in the thread of the test: what it writes and
/// reads goes through the event loop that QTest::qWaitFor runs.
class Client {
public:
    explicit Client(const QString& name) { m_socket.connectToServer(name); }

    [[nodiscard]] bool connected() {
        return QTest::qWaitFor([this] { return m_socket.state() == QLocalSocket::ConnectedState; },
                               kAnswerMilliseconds);
    }

    [[nodiscard]] bool disconnected() {
        return QTest::qWaitFor(
            [this] { return m_socket.state() == QLocalSocket::UnconnectedState; },
            kAnswerMilliseconds);
    }

    void send(const json& message) {
        const QByteArray line = QByteArray::fromStdString(message.dump() + "\n");
        m_socket.write(line);
        m_socket.flush();
    }

    /// The next message of the channel; null if none comes in time.
    [[nodiscard]] json receive(int milliseconds = kAnswerMilliseconds) {
        if (!QTest::qWaitFor([this] { return m_socket.canReadLine(); }, milliseconds)) {
            return json{};
        }
        return json::parse(m_socket.readLine().toStdString());
    }

    /// `message` sent, and the answer.
    [[nodiscard]] json ask(const json& message) {
        send(message);
        return receive();
    }

    /// Whether no message comes for a while.
    [[nodiscard]] bool silent() {
        QTest::qWait(kSilenceMilliseconds);
        return !m_socket.canReadLine();
    }

private:
    QLocalSocket m_socket;
};

/// The two ends of a transport of lines between the test and a bridge that
/// runs in a thread of its own.
class PipeTransport final : public mcp::LineTransport {
public:
    [[nodiscard]] Read readLine(std::string& line, std::size_t maxBytes) override {
        std::unique_lock lock(m_mutex);
        m_ready.wait(lock, [this] { return m_closed || !m_in.empty(); });
        if (m_in.empty()) {
            return Read::End;
        }
        line = std::move(m_in.front());
        m_in.pop_front();
        if (line.size() > maxBytes) {
            line.clear();
            return Read::TooLong;
        }
        return Read::Line;
    }

    [[nodiscard]] bool writeLine(std::string_view line) override {
        const std::scoped_lock lock(m_mutex);
        m_out.emplace_back(line);
        return true;
    }

    void send(std::string line) {
        {
            const std::scoped_lock lock(m_mutex);
            m_in.push_back(std::move(line));
        }
        m_ready.notify_one();
    }

    void close() {
        {
            const std::scoped_lock lock(m_mutex);
            m_closed = true;
        }
        m_ready.notify_one();
    }

    [[nodiscard]] std::optional<std::string> take() {
        const std::scoped_lock lock(m_mutex);
        if (m_out.empty()) {
            return std::nullopt;
        }
        std::string line = std::move(m_out.front());
        m_out.pop_front();
        return line;
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_ready;
    std::deque<std::string> m_in;
    std::deque<std::string> m_out;
    bool m_closed{false};
};

/// `tecnigrafo --mcp` in a thread of its own, as the assistant would start
/// it: its blocking waits leave the thread of the test free to run the
/// channel.
class BridgeThread {
public:
    explicit BridgeThread(QString name)
        : m_thread(QThread::create([this, name = std::move(name)] {
              // Made and destroyed in its thread, as its socket must be.
              AssistantBridge bridge(name);
              std::ostringstream log;
              static_cast<void>(bridge.run(m_transport, log));
          })) {
        m_thread->start();
    }

    BridgeThread(const BridgeThread&) = delete;
    BridgeThread& operator=(const BridgeThread&) = delete;
    BridgeThread(BridgeThread&&) = delete;
    BridgeThread& operator=(BridgeThread&&) = delete;

    ~BridgeThread() {
        m_transport.close();
        m_thread->wait();
    }

    /// `message` sent to the bridge, and its answer, while the event loop
    /// of the test runs.
    [[nodiscard]] json ask(const json& message) {
        m_transport.send(message.dump());
        std::optional<std::string> reply;
        static_cast<void>(QTest::qWaitFor(
            [this, &reply] {
                reply = m_transport.take();
                return reply.has_value();
            },
            kAnswerMilliseconds));
        return reply ? json::parse(*reply) : json{};
    }

private:
    PipeTransport m_transport;
    std::unique_ptr<QThread> m_thread;
};

class AssistantChannelTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_directory.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
        m_window = std::make_unique<MainWindow>(MainWindow::InitialDrawing::Empty);
        channel().setServerName(uniqueName());
    }

    void TearDown() override {
        m_window.reset();
        QSettings::setDefaultFormat(m_format);
    }

    [[nodiscard]] MainWindow& window() { return *m_window; }

    [[nodiscard]] AssistantChannel& channel() { return m_window->assistantChannel(); }

    [[nodiscard]] std::size_t entities() const { return m_window->document().entityCount(); }

    /// A client of the channel, on, after initialize.
    [[nodiscard]] std::unique_ptr<Client> connectClient() {
        auto client = std::make_unique<Client>(channel().serverName());
        EXPECT_TRUE(client->connected());
        const json reply = client->ask(initializeRequest(1));
        EXPECT_TRUE(reply.contains("result")) << reply.dump();
        client->send(json{{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}});
        return client;
    }

    /// The user types `text` on the command line and presses Enter.
    void userTypes(const QString& text) {
        auto* input = m_window->commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
        ASSERT_NE(input, nullptr);
        input->setText(text);
        events::press(input, Qt::Key_Return);
    }

    /// The user presses Esc on the command line.
    void userCancels() {
        auto* input = m_window->commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
        ASSERT_NE(input, nullptr);
        events::press(input, Qt::Key_Escape);
    }

    [[nodiscard]] QString history() const {
        const auto* history =
            m_window->commandLine()->findChild<QPlainTextEdit*>(QStringLiteral("history"));
        return history != nullptr ? history->toPlainText() : QString{};
    }

    /// A folder of the test, for settings and files.
    [[nodiscard]] QString directory() const { return m_directory.path(); }

private:
    QTemporaryDir m_directory;
    QSettings::Format m_format{QSettings::defaultFormat()};
    std::unique_ptr<MainWindow> m_window;
};

TEST_F(AssistantChannelTest, IsOffAtStartAndTakesNoConnection) {
    EXPECT_FALSE(channel().isRunning());
    EXPECT_FALSE(channel().isConnected());
    EXPECT_TRUE(
        AssistantChannel::defaultServerName().startsWith(QStringLiteral("tecnigrafo-assistant-")));
    QLocalSocket socket;
    socket.connectToServer(channel().serverName());
    EXPECT_FALSE(socket.waitForConnected(kSilenceMilliseconds));
}

TEST_F(AssistantChannelTest, OnItAnswersInitializeAndListsTheSixTools) {
    const QSignalSpy running(&channel(), &AssistantChannel::runningChanged);
    const QSignalSpy connected(&channel(), &AssistantChannel::connectedChanged);
    ASSERT_TRUE(channel().start()) << channel().errorString().toStdString();
    EXPECT_TRUE(channel().isRunning());
    EXPECT_EQ(running.count(), 1);
    const std::unique_ptr<Client> client = connectClient();
    EXPECT_TRUE(channel().isConnected());
    EXPECT_EQ(connected.count(), 1);
    const json tools = client->ask(request(2, "tools/list", json::object()));
    EXPECT_EQ(toolNames(tools), kProgramTools);
    // Not a tool of the program: reset would throw the user's drawing away.
    const json reset = client->ask(callRequest(3, "reset"));
    ASSERT_TRUE(reset.contains("error")) << reset.dump();
}

TEST_F(AssistantChannelTest, RunDrawsInTheActiveSessionAndUUndoesIt) {
    ASSERT_TRUE(channel().start());
    const std::unique_ptr<Client> client = connectClient();
    const QSignalSpy received(&channel(), &AssistantChannel::requestReceived);
    const QSignalSpy finished(&channel(), &AssistantChannel::requestFinished);

    const json line = client->ask(runRequest(2, {"LINE", "0,0", "10,0", ""}));
    EXPECT_FALSE(toolFailed(line)) << line.dump();
    EXPECT_NE(toolText(line).find("Command: LINE\n"), std::string::npos) << toolText(line);
    EXPECT_TRUE(toolText(line).ends_with("prompt: Command:")) << toolText(line);
    EXPECT_EQ(entities(), 1U);
    // The lines of the assistant are marked in the window only.
    EXPECT_TRUE(history().contains(QStringLiteral("[AI] Command: LINE")))
        << history().toStdString();
    ASSERT_EQ(received.count(), 1);
    EXPECT_EQ(received.at(0).at(1).toString(), QStringLiteral("run"));
    ASSERT_EQ(finished.count(), 1);
    EXPECT_FALSE(finished.at(0).at(1).toBool());

    const json undo = client->ask(runRequest(3, {"U"}));
    EXPECT_FALSE(toolFailed(undo)) << undo.dump();
    EXPECT_EQ(entities(), 0U);

    // The user's lines keep no mark.
    userTypes(QStringLiteral("REDO"));
    EXPECT_EQ(entities(), 1U);
    EXPECT_TRUE(history().contains(QStringLiteral("Command: REDO"))) << history().toStdString();
    EXPECT_FALSE(history().contains(QStringLiteral("[AI] Command: REDO")));
}

TEST_F(AssistantChannelTest, ReportDescribesTheActiveSession) {
    ASSERT_TRUE(channel().start());
    const std::unique_ptr<Client> client = connectClient();
    static_cast<void>(client->ask(runRequest(2, {"LINE", "0,0", "10,0", ""})));
    const json report = client->ask(callRequest(3, "report"));
    EXPECT_FALSE(toolFailed(report)) << report.dump();
    EXPECT_TRUE(toolText(report).starts_with("tecnigrafo-report 1\n")) << toolText(report);
    EXPECT_NE(toolText(report).find("LINE"), std::string::npos) << toolText(report);
}

TEST_F(AssistantChannelTest, RefusesASecondClient) {
    ASSERT_TRUE(channel().start());
    const std::unique_ptr<Client> first = connectClient();
    Client second(channel().serverName());
    const json refusal = second.receive();
    ASSERT_TRUE(refusal.contains("error")) << refusal.dump();
    EXPECT_EQ(refusal.at("error").at("message").get<std::string>(), kAnotherAssistant);
    EXPECT_TRUE(second.disconnected());
    // The first goes on.
    const json ping = first->ask(request(2, "ping", json::object()));
    EXPECT_TRUE(ping.contains("result")) << ping.dump();
}

TEST_F(AssistantChannelTest, StopClosesTheClient) {
    ASSERT_TRUE(channel().start());
    const std::unique_ptr<Client> client = connectClient();
    channel().stop();
    EXPECT_FALSE(channel().isRunning());
    EXPECT_FALSE(channel().isConnected());
    EXPECT_TRUE(client->disconnected());
}

TEST_F(AssistantChannelTest, ARequestWaitsForTheUserToEndACommand) {
    ASSERT_TRUE(channel().start());
    const std::unique_ptr<Client> client = connectClient();
    userTypes(QStringLiteral("LINE"));
    ASSERT_FALSE(window().isAtCommandPrompt());

    client->send(runRequest(2, {"LINE", "20,0", "30,0", ""}));
    EXPECT_TRUE(client->silent());
    EXPECT_EQ(entities(), 0U);
    // list_commands only reads the registry, but it waits for its turn.
    client->send(callRequest(3, "list_commands"));
    EXPECT_TRUE(client->silent());

    userCancels();
    const json run = client->receive();
    ASSERT_TRUE(run.contains("result")) << run.dump();
    EXPECT_EQ(run.at("id"), 2);
    EXPECT_FALSE(toolFailed(run)) << run.dump();
    EXPECT_EQ(entities(), 1U);
    const json list = client->receive();
    ASSERT_TRUE(list.contains("result")) << list.dump();
    EXPECT_EQ(list.at("id"), 3);
    EXPECT_FALSE(toolFailed(list)) << list.dump();
}

TEST_F(AssistantChannelTest, ARequestGivesUpAfterThirtySeconds) {
    // Simulated time: the clock moves only when the test moves it.
    auto now =
        std::make_shared<std::chrono::steady_clock::time_point>(std::chrono::steady_clock::now());
    channel().setClock([now] { return *now; });
    ASSERT_TRUE(channel().start());
    const std::unique_ptr<Client> client = connectClient();
    userTypes(QStringLiteral("LINE"));

    client->send(runRequest(2, {"LINE", "20,0", "30,0", ""}));
    // The request has come, and waits from the time it came.
    EXPECT_TRUE(client->silent());
    *now += kAssistantWait - std::chrono::seconds{1};
    EXPECT_TRUE(client->silent());
    *now += std::chrono::seconds{1};
    const json reply = client->receive();
    EXPECT_TRUE(toolFailed(reply)) << reply.dump();
    EXPECT_EQ(toolText(reply), kUserBusy);
    EXPECT_EQ(entities(), 0U);
    // The user's command is left as it was.
    EXPECT_FALSE(window().isAtCommandPrompt());
    userCancels();
}

TEST_F(AssistantChannelTest, WithoutDocumentsOnlyOpenWorks) {
    ASSERT_TRUE(channel().start());
    const std::unique_ptr<Client> client = connectClient();
    static_cast<void>(client->ask(runRequest(2, {"LINE", "0,0", "10,0", ""})));
    const QString path = QDir(directory()).filePath(QStringLiteral("assistant.cadx"));
    const json save = client->ask(callRequest(3, "save", json{{"path", path.toStdString()}}));
    EXPECT_FALSE(toolFailed(save)) << save.dump();
    ASSERT_TRUE(QFileInfo::exists(path)) << toolText(save);

    window().closeSession(*window().activeSession());
    ASSERT_EQ(window().activeSession(), nullptr);
    for (const auto& [id, tool] : {std::pair{4, "run"}, std::pair{5, "report"}}) {
        json arguments = json::object();
        if (std::string_view{tool} == "run") {
            arguments["lines"] = json::array({"LINE"});
        }
        const json reply = client->ask(callRequest(id, tool, arguments));
        EXPECT_TRUE(toolFailed(reply)) << reply.dump();
        EXPECT_EQ(toolText(reply), kNoDrawingOpen) << tool;
    }
    const json noSave = client->ask(callRequest(6, "save", json{{"path", path.toStdString()}}));
    EXPECT_EQ(toolText(noSave), kNoDrawingOpen);
    const json list = client->ask(callRequest(7, "list_commands"));
    EXPECT_FALSE(toolFailed(list)) << list.dump();

    const json open = client->ask(callRequest(8, "open", json{{"path", path.toStdString()}}));
    EXPECT_FALSE(toolFailed(open)) << open.dump();
    ASSERT_NE(window().activeSession(), nullptr) << toolText(open);
    EXPECT_EQ(entities(), 1U);
}

TEST_F(AssistantChannelTest, TheBridgeAnswersAloneWithoutTheProgram) {
    BridgeThread bridge(channel().serverName());
    const json initialize = bridge.ask(initializeRequest(1));
    EXPECT_TRUE(initialize.contains("result")) << initialize.dump();
    const json tools = bridge.ask(request(2, "tools/list", json::object()));
    EXPECT_EQ(toolNames(tools), kProgramTools);
}

TEST_F(AssistantChannelTest, TheBridgeTriesAgainAtEveryCall) {
    BridgeThread bridge(channel().serverName());
    static_cast<void>(bridge.ask(initializeRequest(1)));
    const json off = bridge.ask(runRequest(2, {"LINE", "0,0", "10,0", ""}));
    EXPECT_TRUE(toolFailed(off)) << off.dump();
    EXPECT_EQ(toolText(off), mcp::kNotRunning);

    ASSERT_TRUE(channel().start());
    const json on = bridge.ask(runRequest(3, {"LINE", "0,0", "10,0", ""}));
    EXPECT_FALSE(toolFailed(on)) << on.dump();
    EXPECT_EQ(on.at("id"), 3);
    EXPECT_EQ(entities(), 1U);
    EXPECT_TRUE(channel().isConnected());
}

TEST_F(AssistantChannelTest, TheBridgeFollowsTheChannelOffAndOnAgain) {
    ASSERT_TRUE(channel().start());
    BridgeThread bridge(channel().serverName());
    static_cast<void>(bridge.ask(initializeRequest(1)));
    const json first = bridge.ask(runRequest(2, {"LINE", "0,0", "10,0", ""}));
    EXPECT_FALSE(toolFailed(first)) << first.dump();

    channel().stop();
    ASSERT_TRUE(channel().start());
    const json second = bridge.ask(runRequest(3, {"LINE", "0,10", "10,10", ""}));
    EXPECT_FALSE(toolFailed(second)) << second.dump();
    EXPECT_EQ(entities(), 2U);

    channel().stop();
    const json third = bridge.ask(runRequest(4, {"U"}));
    EXPECT_TRUE(toolFailed(third)) << third.dump();
    EXPECT_EQ(toolText(third), mcp::kNotRunning);
    EXPECT_EQ(entities(), 2U);
}

}  // namespace
}  // namespace tg::app
