// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The channel of the user's assistant (docs/adr/0051): a local server, the
// common queue with the user, and the host of the tools over the active
// session of the window.

#include "assistant_channel.hpp"

#include "cad_view.hpp"
#include "command_line.hpp"
#include "document_session.hpp"
#include "main_window.hpp"
#include "nlohmann_json/json.hpp"
#include "tg/cmd/command_protocol.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/cmd/input.hpp"
#include "tg/core/vec2.hpp"
#include "tg/mcp/server.hpp"
#include "tg/mcp/session.hpp"
#include "tg/mcp/tool.hpp"
#include "tg/mcp/tool_host.hpp"
#include "tg/script/report.hpp"
#include "tg/script/script.hpp"

#include <QAction>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QScopedValueRollback>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::app {
namespace {

using nlohmann::json;

/// The tools that act on the session, and wait for the user to be at
/// "Command:" (docs/adr/0051, point 6). list_commands and describe_command
/// only read the registry: they never wait.
constexpr std::array<std::string_view, 4> kSessionTools{"run", "report", "open", "save"};

/// How long start() waits to learn whether another program already listens
/// on the name: a local connection either succeeds or is refused at once.
constexpr int kProbeMilliseconds = 1000;

bool isSessionTool(std::string_view tool) {
    return std::ranges::find(kSessionTools, tool) != kSessionTools.end();
}

/// A tool of the program, answered by the channel instead when the gate
/// says so: after the wait, or without a document.
class GatedTool final : public mcp::Tool {
public:
    using Gate = std::function<std::optional<mcp::ToolResult>(std::string_view)>;

    GatedTool(std::unique_ptr<mcp::Tool> tool, Gate gate)
        : m_tool(std::move(tool)), m_name(m_tool->name()), m_gate(std::move(gate)) {}

    [[nodiscard]] std::string name() const override { return m_name; }

    [[nodiscard]] std::string description() const override { return m_tool->description(); }

    [[nodiscard]] json inputSchema() const override { return m_tool->inputSchema(); }

    [[nodiscard]] mcp::ToolResult call(const json& arguments) override {
        if (std::optional<mcp::ToolResult> refusal = m_gate(m_name)) {
            return std::move(*refusal);
        }
        return m_tool->call(arguments);
    }

private:
    std::unique_ptr<mcp::Tool> m_tool;
    std::string m_name;
    Gate m_gate;
};

/// The active session of the window as the host of the tools
/// (docs/adr/0051, points 4 to 8).
class WindowHost final : public mcp::ToolHost {
public:
    explicit WindowHost(MainWindow& window) : m_window(window) {}

    [[nodiscard]] mcp::ToolResult run(std::span<const std::string> lines) override;
    [[nodiscard]] std::string report() override;
    [[nodiscard]] mcp::ToolResult open(std::string_view path) override;
    [[nodiscard]] mcp::ToolResult save(std::string_view path, bool replace) override;

    [[nodiscard]] const cmd::CommandRegistry& commands() const override {
        return m_window.commandRegistry();
    }

private:
    /// While it lives, the lines written in the history are the assistant's:
    /// with "[AI] " in front, and kept for the answer and the report.
    class Lines {
    public:
        explicit Lines(WindowHost& host) : m_commandLine(*host.m_window.commandLine()) {
            m_commandLine.setAssistantLines([&host](const QString& line) {
                std::string text = line.toStdString();
                host.m_exchange.push_back(text);
                host.m_messages.push_back(std::move(text));
            });
        }

        Lines(const Lines&) = delete;
        Lines& operator=(const Lines&) = delete;
        Lines(Lines&&) = delete;
        Lines& operator=(Lines&&) = delete;

        ~Lines() { m_commandLine.setAssistantLines({}); }

    private:
        CommandLine& m_commandLine;
    };

    /// OPEN or SAVEAS with `path`, as Session::fileCommand() does in
    /// tecnigrafo-mcp.
    [[nodiscard]] mcp::ToolResult fileCommand(const QString& command, std::string_view path,
                                              cmd::FilePrompt expected, bool replace);
    /// The lines written during the call, the sections of any !report, and
    /// the prompt now.
    [[nodiscard]] std::string exchange();
    void writeSection(std::string_view title);

    void perform(DocumentSession& session, const script::TypedLine& line);
    void perform(DocumentSession& session, const script::ViewDirective& view);
    void perform(DocumentSession& session, const script::MoveDirective& move);
    void perform(DocumentSession& session, const script::ClickDirective& click);
    void perform(DocumentSession& session, const script::HoverDirective& hover);
    void perform(DocumentSession& session, const script::RightClickDirective& directive);
    void perform(DocumentSession& session, const script::EscDirective& directive);
    void perform(DocumentSession& session, const script::AidDirective& directive);
    void perform(DocumentSession& session, const script::OsnapDirective& directive);
    void perform(DocumentSession& session, const script::ScriptSequencesDirective& directive);
    void perform(DocumentSession& session, const script::ReportDirective& directive);

    // Level 2 only (parseScript() refuses them at level 1), or read by
    // parseScript(): nothing to do while running.
    static void perform(DocumentSession& /*session*/, const script::KeyDirective& /*directive*/) {}

    static void perform(DocumentSession& /*session*/,
                        const script::ScreenshotDirective& /*directive*/) {}

    static void perform(DocumentSession& /*session*/, const script::LevelDirective& /*directive*/) {
    }

    static void perform(DocumentSession& /*session*/,
                        const script::ToleranceDirective& /*directive*/) {}

    MainWindow& m_window;
    std::vector<std::string> m_exchange;  ///< the lines of the call running
    std::vector<std::string> m_messages;  ///< since the previous section
    std::string m_report;                 ///< sections of !report not yet shown
    int m_reports{0};
    std::optional<core::Vec2> m_pointer;
};

mcp::ToolResult WindowHost::run(std::span<const std::string> lines) {
    // The same checks, in the same order and with the same messages, as
    // the run of tecnigrafo-mcp (mcp::Session::run).
    if (lines.size() > mcp::kMaxScriptLines) {
        return mcp::ToolResult::error("Too many lines: at most " +
                                      std::to_string(mcp::kMaxScriptLines) + " in one request.");
    }
    std::string text;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].find_first_of("\r\n") != std::string::npos) {
            return mcp::ToolResult::error("line " + std::to_string(i + 1) +
                                          ": a line cannot contain a line break.");
        }
        text += lines[i];
        text += '\n';
    }
    const std::variant<script::Script, script::ParseError> parsed = script::parseScript(text);
    if (const auto* error = std::get_if<script::ParseError>(&parsed)) {
        return mcp::ToolResult::error("line " + std::to_string(error->line) + ": " +
                                      error->message);
    }
    const auto& script = std::get<script::Script>(parsed);
    if (script.level != 1) {
        return mcp::ToolResult::error(
            "The session runs at level 1: \"!level 2\", \"!key\" and \"!screenshot\" are not "
            "available.");
    }
    if (m_window.activeSession() == nullptr) {
        return mcp::ToolResult::error(std::string{kNoDrawingOpen});
    }
    {
        const Lines assistant{*this};
        for (const script::Action& action : script.actions) {
            // NEW and OPEN make another session active: the next line goes
            // there, as the user's would. A CLOSE of the last document ends
            // the call: the lines after it have no drawing.
            DocumentSession* session = m_window.activeSession();
            if (session == nullptr) {
                break;
            }
            std::visit([this, session](const auto& concrete) { this->perform(*session, concrete); },
                       action);
        }
    }
    return mcp::ToolResult::success(exchange());
}

std::string WindowHost::report() {
    if (m_window.activeSession() == nullptr) {
        return std::string{kNoDrawingOpen};
    }
    writeSection(script::kFinalSection);
    std::string text{script::kReportHeader};
    text += '\n';
    text += std::exchange(m_report, std::string{});
    return text;
}

mcp::ToolResult WindowHost::open(std::string_view path) {
    // With the Start page too: OPEN runs in the runner of the window, as
    // the user's (MainWindow::inputRunner()).
    return fileCommand(QStringLiteral("OPEN"), path, cmd::FilePrompt::Open, false);
}

mcp::ToolResult WindowHost::save(std::string_view path, bool replace) {
    if (m_window.activeSession() == nullptr) {
        return mcp::ToolResult::error(std::string{kNoDrawingOpen});
    }
    return fileCommand(QStringLiteral("SAVEAS"), path, cmd::FilePrompt::Save, replace);
}

mcp::ToolResult WindowHost::fileCommand(const QString& command, std::string_view path,
                                        cmd::FilePrompt expected, bool replace) {
    if (path.empty()) {
        return mcp::ToolResult::error(command.toLower().toStdString() +
                                      " needs \"path\", the path of a file.");
    }
    if (path.find_first_of("\r\n") != std::string_view::npos) {
        return mcp::ToolResult::error("A path cannot contain a line break.");
    }
    {
        const Lines assistant{*this};
        const cmd::CommandRunner* runner = &m_window.inputRunner();
        m_window.submitAssistantText(command);
        // OPEN asks first whether to save a modified drawing: the path is
        // not the answer to that question, so the exchange stops there.
        if (runner->isActive() && runner->prompt().file == expected) {
            m_window.submitAssistantText(
                QString::fromUtf8(path.data(), static_cast<qsizetype>(path.size())));
            // OPEN may have made another session active; SAVEAS stays in this one.
            const cmd::Prompt question = runner->prompt();
            const bool asksToReplace =
                runner->isActive() &&
                std::ranges::any_of(question.keywords, [](const cmd::Keyword& keyword) {
                    return keyword.name == "Yes";
                });
            if (replace && asksToReplace) {
                m_window.submitAssistantText(QStringLiteral("Y"));
            }
        }
    }
    return mcp::ToolResult::success(exchange());
}

std::string WindowHost::exchange() {
    std::string text;
    for (const std::string& line : m_exchange) {
        text += line;
        text += '\n';
    }
    m_exchange.clear();
    text += std::exchange(m_report, std::string{});
    text += "prompt: " + m_window.inputRunner().promptText();
    return text;
}

void WindowHost::writeSection(std::string_view title) {
    const DocumentSession* active = m_window.activeSession();
    assert(active != nullptr && "a report needs a document");
    std::vector<script::ReportDocument> documents;
    documents.reserve(m_window.sessions().size());
    for (const auto& session : m_window.sessions()) {
        documents.push_back({session->file().name(), session.get() == active});
    }
    script::writeState({title, active->editor(), active->runner(), active->file(), m_messages,
                        m_pointer, documents},
                       m_report);
    m_messages.clear();
}

void WindowHost::perform(DocumentSession& /*session*/, const script::TypedLine& line) {
    m_window.submitAssistantText(QString::fromStdString(line.text));
}

void WindowHost::perform(DocumentSession& /*session*/, const script::ViewDirective& view) {
    // The view of the window moves; its size stays the user's.
    m_window.view()->setView(view.center, view.scale);
    m_window.assistantInputHandled();
}

void WindowHost::perform(DocumentSession& session, const script::MoveDirective& move) {
    m_pointer = move.point;
    session.runner().pointerMoved(move.point);
    m_window.assistantInputHandled();
}

void WindowHost::perform(DocumentSession& session, const script::ClickDirective& click) {
    // A real mouse reaches the point before it clicks there.
    m_pointer = click.point;
    cmd::CommandRunner& runner = session.runner();
    runner.pointerMoved(click.point);
    runner.click(click.point, click.remove ? cmd::PickAction::Remove : cmd::PickAction::Add);
    m_window.assistantInputHandled();
}

void WindowHost::perform(DocumentSession& session, const script::HoverDirective& hover) {
    m_pointer = hover.point;
    session.runner().pointerHeld(hover.point);
    m_window.assistantInputHandled();
}

void WindowHost::perform(DocumentSession& session,
                         const script::RightClickDirective& /*directive*/) {
    session.runner().submit(cmd::ConfirmInput{});
    m_window.assistantInputHandled();
}

void WindowHost::perform(DocumentSession& session, const script::EscDirective& /*directive*/) {
    session.runner().submit(cmd::CancelInput{});
    m_window.assistantInputHandled();
}

void WindowHost::perform(DocumentSession& /*session*/, const script::AidDirective& directive) {
    // The switches of the window, which keep the menus, the status bar and
    // the runner in agreement, as a script of level 2 sets them.
    switch (directive.aid) {
        case script::AidDirective::Aid::Polar:
            m_window.setPolar(directive.on);
            break;
        case script::AidDirective::Aid::Tracking:
            m_window.setTracking(directive.on);
            break;
        case script::AidDirective::Aid::Snap:
        case script::AidDirective::Aid::Ortho: {
            const bool snap = directive.aid == script::AidDirective::Aid::Snap;
            auto* action = m_window.findChild<QAction*>(snap ? QStringLiteral("aids.osnap")
                                                             : QStringLiteral("aids.ortho"));
            if (action != nullptr) {
                action->setChecked(directive.on);
            }
            break;
        }
    }
    m_window.assistantInputHandled();
}

void WindowHost::perform(DocumentSession& /*session*/, const script::OsnapDirective& directive) {
    m_window.setSnapModes(directive.modes);
    m_window.assistantInputHandled();
}

void WindowHost::perform(DocumentSession& /*session*/,
                         const script::ScriptSequencesDirective& directive) {
    m_window.setScriptSequences(directive.on);
}

void WindowHost::perform(DocumentSession& /*session*/,
                         const script::ReportDirective& /*directive*/) {
    ++m_reports;
    writeSection("report " + std::to_string(m_reports));
}

/// A call of a tool, for the signals of the interface.
struct ToolCall {
    std::string tool;  ///< its name
    QString text;      ///< its arguments: for run, the lines
};

/// The tool called by `message`, if it is a request tools/call with a name;
/// std::nullopt for any other message.
std::optional<ToolCall> toolCall(std::string_view message) {
    // What the client sends is not checked yet: no accessor here may throw.
    const json value = json::parse(message, nullptr, /*allow_exceptions=*/false);
    if (!value.is_object() || !value.contains("id")) {
        return std::nullopt;
    }
    const auto method = value.find("method");
    if (method == value.end() || !method->is_string() || *method != "tools/call") {
        return std::nullopt;
    }
    const auto params = value.find("params");
    if (params == value.end() || !params->is_object()) {
        return std::nullopt;
    }
    const auto name = params->find("name");
    if (name == params->end() || !name->is_string()) {
        return std::nullopt;
    }
    ToolCall call{name->get<std::string>(), {}};
    const auto arguments = params->find("arguments");
    if (arguments == params->end()) {
        return call;
    }
    const auto lines = arguments->is_object() ? arguments->find("lines") : arguments->end();
    if (lines != arguments->end() && lines->is_array()) {
        QStringList text;
        for (const json& line : *lines) {
            text.append(line.is_string() ? QString::fromStdString(line.get<std::string>())
                                         : QString::fromStdString(line.dump()));
        }
        call.text = text.join(QLatin1Char('\n'));
    } else {
        call.text =
            QString::fromStdString(arguments->dump(-1, ' ', false, json::error_handler_t::replace));
    }
    return call;
}

/// Whether `reply` is an error, and its text: the text of a tool's result,
/// or the message of a JSON-RPC error.
std::pair<bool, QString> outcome(std::string_view reply) {
    const json value = json::parse(reply, nullptr, /*allow_exceptions=*/false);
    if (!value.is_object()) {
        return {true, {}};
    }
    if (const auto error = value.find("error"); error != value.end() && error->is_object()) {
        return {true, QString::fromStdString(error->value("message", ""))};
    }
    const auto result = value.find("result");
    if (result == value.end() || !result->is_object()) {
        return {true, {}};
    }
    QString text;
    const auto content = result->find("content");
    if (content != result->end() && content->is_array() && !content->empty() &&
        content->front().is_object()) {
        text = QString::fromStdString(content->front().value("text", ""));
    }
    return {result->value("isError", false), text};
}

}  // namespace

AssistantChannel::AssistantChannel(MainWindow& window, QObject* parent)
    : QObject(parent),
      m_window(window),
      m_name(defaultServerName()),
      m_clock([] { return std::chrono::steady_clock::now(); }),
      m_server(new QLocalServer(this)) {
    m_server->setSocketOptions(QLocalServer::UserAccessOption);
    connect(m_server, &QLocalServer::newConnection, this, &AssistantChannel::acceptClients);
    m_poll.setInterval(kPollInterval);
    connect(&m_poll, &QTimer::timeout, this, &AssistantChannel::processQueue);
    // The end of an input of the user may let a request start: queued, so
    // that it starts from the event loop, once what the user started (a
    // tab that closes, a file saved) is over, never in the middle of it.
    connect(&m_window, &MainWindow::inputHandled, this, &AssistantChannel::processQueue,
            Qt::QueuedConnection);
}

AssistantChannel::~AssistantChannel() {
    // No signal reaches a half-destroyed channel.
    if (m_client != nullptr) {
        disconnect(m_client, nullptr, this, nullptr);
    }
    m_server->close();
}

QString AssistantChannel::defaultServerName() {
    // The user of the system on every platform, and the home folder, which
    // tells apart two users of the same name; hashed, so that the name is
    // short and has no characters a socket name cannot hold.
    QString user = qEnvironmentVariable("USER");
    if (user.isEmpty()) {
        user = qEnvironmentVariable("USERNAME");
    }
    const QByteArray key = (user + QLatin1Char('\n') + QDir::homePath()).toUtf8();
    // 16 hexadecimal digits: 64 bits, enough to tell the users of one
    // computer apart, and short within the 104 bytes of a Unix socket path.
    constexpr qsizetype kHashDigits = 16;
    const QByteArray hash =
        QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex().left(kHashDigits);
    return QStringLiteral("tecnigrafo-assistant-") + QString::fromLatin1(hash);
}

void AssistantChannel::setServerName(QString name) {
    assert(!isRunning() && "the name changes only while the channel is off");
    m_name = std::move(name);
}

void AssistantChannel::setClock(Clock clock) {
    m_clock = std::move(clock);
}

bool AssistantChannel::start() {
    if (isRunning()) {
        return true;
    }
    m_error.clear();
    if (!m_server->listen(m_name)) {
        // A socket left by a program that ended badly (Unix) blocks the
        // name; one that answers belongs to a program still running, whose
        // channel must not be taken away.
        QLocalSocket probe;
        probe.connectToServer(m_name);
        if (probe.waitForConnected(kProbeMilliseconds)) {
            probe.abort();
            m_error = tr("Another Tecnigrafo of this user has its assistant channel on.");
            return false;
        }
        static_cast<void>(QLocalServer::removeServer(m_name));
        if (!m_server->listen(m_name)) {
            m_error = m_server->errorString();
            return false;
        }
    }
    emit runningChanged(true);
    return true;
}

void AssistantChannel::stop() {
    if (!isRunning()) {
        return;
    }
    if (m_client != nullptr) {
        disconnect(m_client, nullptr, this, nullptr);
        m_client->abort();
        if (m_running) {
            m_dropPending = true;
        } else {
            dropClient();
        }
    }
    m_server->close();
    emit runningChanged(false);
}

bool AssistantChannel::isRunning() const {
    return m_server->isListening();
}

void AssistantChannel::acceptClients() {
    while (QLocalSocket* socket = m_server->nextPendingConnection()) {
        if (m_client != nullptr) {
            // A second assistant learns why, and goes (docs/adr/0051, point 2).
            const json refusal{
                {"jsonrpc", "2.0"},
                {"id", nullptr},
                {"error", json{{"code", mcp::kInvalidRequest}, {"message", kAnotherAssistant}}}};
            const std::string line = refusal.dump() + "\n";
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            socket->write(line.data(), static_cast<qint64>(line.size()));
            // Waits for the line to be written, then closes.
            socket->disconnectFromServer();
            if (socket->state() == QLocalSocket::UnconnectedState) {
                socket->deleteLater();
            }
            continue;
        }
        m_client = socket;
        // A new client starts a new conversation: initialize again.
        auto host = std::make_shared<WindowHost>(m_window);
        std::vector<std::unique_ptr<mcp::Tool>> tools;
        for (std::unique_ptr<mcp::Tool>& tool : mcp::programTools(std::move(host))) {
            tools.push_back(std::make_unique<GatedTool>(
                std::move(tool), [this](std::string_view name) { return gate(name); }));
        }
        m_mcp = std::make_unique<mcp::Server>(std::move(tools));
        connect(socket, &QLocalSocket::readyRead, this, &AssistantChannel::readClient);
        connect(socket, &QLocalSocket::disconnected, this, &AssistantChannel::clientGone);
        emit connectedChanged(true);
        readClient();
    }
}

void AssistantChannel::readClient() {
    if (m_client == nullptr) {
        return;
    }
    // The limit of the protocol (docs/adr/0027, point 3): a longer message
    // keeps one byte more than the limit, which mcp::Server::answer()
    // refuses as too large, and the rest of it is dropped.
    const std::size_t limit = mcp::ServerOptions{}.maxMessageBytes;
    const QByteArray bytes = m_client->readAll();
    qsizetype start = 0;
    while (start < bytes.size()) {
        const qsizetype end = bytes.indexOf('\n', start);
        const qsizetype stop = end < 0 ? bytes.size() : end;
        const std::size_t room = limit + 1 - std::min(m_buffer.size(), limit + 1);
        const std::size_t length = std::min(static_cast<std::size_t>(stop - start), room);
        m_buffer.append(bytes.constData() + start, length);
        if (end < 0) {
            break;
        }
        Pending pending{std::exchange(m_buffer, std::string{}), m_clock(), std::nullopt, {}};
        if (pending.message.size() <= limit) {
            if (std::optional<ToolCall> call = toolCall(pending.message)) {
                pending.request = ++m_requests;
                pending.tool = call->tool;
                emit requestReceived(*pending.request, QString::fromStdString(call->tool),
                                     call->text);
            }
        }
        m_queue.push_back(std::move(pending));
        start = end + 1;
    }
    processQueue();
}

void AssistantChannel::clientGone() {
    if (m_running) {
        m_dropPending = true;
        return;
    }
    dropClient();
}

void AssistantChannel::dropClient() {
    m_dropPending = false;
    if (m_client == nullptr) {
        return;
    }
    disconnect(m_client, nullptr, this, nullptr);
    m_client->deleteLater();
    m_client = nullptr;
    m_mcp.reset();
    m_buffer.clear();
    m_queue.clear();
    m_poll.stop();
    emit connectedChanged(false);
}

void AssistantChannel::processQueue() {
    // One message at a time: an event loop that a request might run (a
    // modal dialog) brings no other one in the middle of it.
    if (m_running) {
        return;
    }
    {
        const QScopedValueRollback<bool> guard(m_running, true);
        serveQueue();
    }
    if (m_dropPending) {
        dropClient();
    }
}

void AssistantChannel::serveQueue() {
    while (m_client != nullptr && !m_dropPending && !m_queue.empty()) {
        const Pending& next = m_queue.front();
        // Without a document, the tools that need one answer at once.
        const bool answersAtOnce = next.tool != "open" && m_window.activeSession() == nullptr;
        const bool waits =
            isSessionTool(next.tool) && !answersAtOnce && !m_window.isAtCommandPrompt();
        if (waits) {
            if (m_clock() - next.arrived < kAssistantWait) {
                if (!m_poll.isActive()) {
                    m_poll.start();
                }
                return;
            }
            m_refusal = std::string{kUserBusy};
        }
        const Pending pending = std::move(m_queue.front());
        m_queue.pop_front();
        answer(pending);
        m_refusal.reset();
    }
    m_poll.stop();
}

void AssistantChannel::answer(const Pending& pending) {
    // Logs of the library go nowhere: what the user sees is in the history
    // and in the signals.
    std::ostringstream log;
    const std::optional<std::string> reply = m_mcp->answer(pending.message, log);
    if (pending.request) {
        const auto [failed, text] = reply ? outcome(*reply) : std::pair{true, QString{}};
        emit requestFinished(*pending.request, failed, text);
    }
    if (!reply || m_client == nullptr) {
        return;
    }
    const std::string line = *reply + "\n";
    m_client->write(line.data(), static_cast<qint64>(line.size()));
    m_client->flush();
}

std::optional<mcp::ToolResult> AssistantChannel::gate(std::string_view tool) const {
    if (m_refusal) {
        return mcp::ToolResult::error(*m_refusal);
    }
    if (isSessionTool(tool) && tool != "open" && m_window.activeSession() == nullptr) {
        return mcp::ToolResult::error(std::string{kNoDrawingOpen});
    }
    return std::nullopt;
}

}  // namespace tg::app
