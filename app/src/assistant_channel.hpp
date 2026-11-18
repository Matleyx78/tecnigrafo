// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QtGlobal>

#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class QLocalServer;
class QLocalSocket;

namespace tg::mcp {
class Server;
class Tool;
struct ToolResult;
}  // namespace tg::mcp

namespace tg::app {

class MainWindow;

/// The error of run, report and save with no document open, the Start page
/// (docs/adr/0051, point 5).
inline constexpr std::string_view kNoDrawingOpen = "No drawing is open.";

/// The error of a request that waited kAssistantWait for the user to end a
/// command (docs/adr/0051, point 6).
inline constexpr std::string_view kUserBusy = "The user is running a command.";

/// The error a second client gets, before it is closed (docs/adr/0051,
/// point 2).
inline constexpr std::string_view kAnotherAssistant = "Another assistant is connected.";

/// How long a request of the assistant waits for the user to end a command
/// (docs/adr/0051, point 6).
inline constexpr std::chrono::seconds kAssistantWait{30};

/// The channel of the user's assistant (docs/adr/0051): a QLocalServer,
/// accessible only to the user of the system (QLocalServer::UserAccessOption,
/// a Unix domain socket on Linux and macOS, a local named pipe on Windows),
/// never a network port. Off when it is made, as at every start of the
/// program; start() turns it on.
///
/// One client at a time, which speaks MCP one JSON-RPC message per line,
/// with the library of module mcp and the tools of the program
/// (mcp::programTools()) over the active session of the window: list_commands,
/// describe_command, run, report, open and save, with the paths of the
/// computer. A second client gets a JSON-RPC error kAnotherAssistant, and
/// is closed. stop() closes the client too.
///
/// The common queue (docs/adr/0051, points 6 and 7): the messages run one at
/// a time, in the order they came, in the thread of the interface. A call
/// of run, report, open or save starts only when the user is at "Command:"
/// (MainWindow::isAtCommandPrompt()); otherwise it waits, checking at the
/// end of every input of the user, and also every kPollInterval, until
/// kAssistantWait has passed since it came, then answers the error
/// kUserBusy. The same holds when the assistant itself left a command
/// halfway in a previous call: the runner is not at "Command:". Without a
/// document, run, report and save answer kNoDrawingOpen at once; open opens
/// one. A request runs to its end without returning to the event loop, so
/// the input of the user, on the command line and in the view, waits for it.
/// What the assistant types is a step of the history of the session, as
/// what the user types: U undoes it. Its lines are written in the history of
/// the command line with kAssistantPrefix in front (CommandLine).
///
/// The tools answer as those of tecnigrafo-mcp (docs/adr/0027, point 5),
/// over the window instead of a session of their own:
/// - run: the lines of a .tgs script, level 1, as the user would give them:
///   a normal line is typed on the command line, !move, !click, !hover,
///   !rightclick and !esc are the pointer, the buttons and Esc on the
///   runner, !snap, !ortho, !polar, !otrack, !osnap and !scr are the
///   switches of the window, !view moves the view. The text is the lines
///   the assistant wrote in the history during the call, then the sections
///   of any !report, then "prompt: <the prompt now>".
/// - report: the report of the active document now, whose messages are the
///   lines the assistant wrote since the previous section: the user's lines
///   are not the assistant's exchange.
/// - open and save: OPEN and SAVEAS with the path, as run would with the
///   two lines; save answers Y to the question to replace a file with
///   replace true. No file dialog opens: the assistant types the path.
class AssistantChannel final : public QObject {
    Q_OBJECT

public:
    /// How often a request that waits checks again for the user, besides
    /// the end of every input: a change of tab, or a dialog closed, ends
    /// the wait too.
    static constexpr std::chrono::milliseconds kPollInterval{100};

    /// The clock of the wait, which the tests replace to simulate time.
    using Clock = std::function<std::chrono::steady_clock::time_point()>;

    /// A channel, off, for the active session of `window`, which must
    /// outlive it.
    explicit AssistantChannel(MainWindow& window, QObject* parent = nullptr);
    ~AssistantChannel() override;

    Q_DISABLE_COPY_MOVE(AssistantChannel)

    /// The name of the channel of this user of the system:
    /// "tecnigrafo-assistant-" and a hash of the user and of the home folder,
    /// the same for the program and for the bridge (`tecnigrafo --mcp`).
    [[nodiscard]] static QString defaultServerName();

    /// The name the channel listens on: defaultServerName() unless
    /// setServerName() changed it (the tests do, so that they never meet
    /// the program of the user).
    [[nodiscard]] const QString& serverName() const { return m_name; }

    /// Changes the name; while off only.
    void setServerName(QString name);

    /// Replaces the clock of the wait: the tests simulate the time.
    void setClock(Clock clock);

    /// Turns the channel on. False, with the reason in errorString(), if it
    /// cannot listen: another program of the same user has its channel on.
    /// True if it was already on.
    bool start();

    /// Turns the channel off: the client, if any, is closed, and its
    /// requests are dropped.
    void stop();

    [[nodiscard]] bool isRunning() const;

    [[nodiscard]] bool isConnected() const { return m_client != nullptr; }

    /// Why start() failed.
    [[nodiscard]] const QString& errorString() const { return m_error; }

signals:
    void runningChanged(bool running);
    void connectedChanged(bool connected);
    /// A call of a tool came: `request` numbers the calls from 1, `tool` is
    /// its name, `text` its arguments (for run, the lines).
    void requestReceived(quint64 request, const QString& tool, const QString& text);
    /// The call `request` was answered: `failed` for an error, `text` the
    /// text of the result or the message of the error.
    void requestFinished(quint64 request, bool failed, const QString& text);

private:
    /// A message of the client, waiting for its turn.
    struct Pending {
        std::string message;
        std::chrono::steady_clock::time_point arrived;
        std::optional<quint64> request;  ///< the number of a tool call
        std::string tool;                ///< its name, empty for another message
    };

    void acceptClients();
    void readClient();
    void clientGone();
    /// Answers the messages at the front of the queue that can run now.
    void processQueue();
    void serveQueue();
    /// Answers `pending` and writes the answer, if any.
    void answer(const Pending& pending);
    /// Forgets the client, its server and its queue.
    void dropClient();
    /// The answer of a tool before it runs: kUserBusy after the wait,
    /// kNoDrawingOpen without a document; std::nullopt to run it.
    [[nodiscard]] std::optional<mcp::ToolResult> gate(std::string_view tool) const;

    MainWindow& m_window;
    QString m_name;
    QString m_error;
    Clock m_clock;
    QLocalServer* m_server{nullptr};
    QLocalSocket* m_client{nullptr};
    std::unique_ptr<mcp::Server> m_mcp;
    std::string m_buffer;  ///< the bytes of a message not yet ended
    std::deque<Pending> m_queue;
    QTimer m_poll;
    quint64 m_requests{0};
    bool m_running{false};      ///< a message is being answered
    bool m_dropPending{false};  ///< the client went while one was
    std::optional<std::string> m_refusal;
};

}  // namespace tg::app
