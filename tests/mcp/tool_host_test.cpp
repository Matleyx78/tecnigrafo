// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The tools over a lent host and the server over a transport of lines
// (0.8-PT36, docs/adr/0051 points 1 and 4).

#include "tg/mcp/tool_host.hpp"

#include "nlohmann_json/json.hpp"
#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/mcp/server.hpp"
#include "tg/mcp/session.hpp"
#include "tg/mcp/tool.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using nlohmann::json;
using tg::mcp::LineTransport;
using tg::mcp::Server;
using tg::mcp::Tool;
using tg::mcp::ToolHost;
using tg::mcp::ToolResult;

/// A host that records what reaches it, with the built-in commands.
class FakeHost final : public ToolHost {
public:
    FakeHost() { tg::cmd::registerBuiltinCommands(m_commands); }

    [[nodiscard]] ToolResult run(std::span<const std::string> lines) override {
        m_runs.emplace_back(lines.begin(), lines.end());
        return ToolResult::success("prompt: Command:");
    }

    [[nodiscard]] std::string report() override {
        ++m_reports;
        return "tecnigrafo-report 1\n";
    }

    [[nodiscard]] ToolResult open(std::string_view path) override {
        m_opened.emplace_back(path);
        return ToolResult::success("opened");
    }

    [[nodiscard]] ToolResult save(std::string_view path, bool replace) override {
        m_saved.emplace_back(std::string{path}, replace);
        return ToolResult::success("saved");
    }

    [[nodiscard]] const tg::cmd::CommandRegistry& commands() const override { return m_commands; }

    [[nodiscard]] const std::vector<std::vector<std::string>>& runs() const { return m_runs; }

    [[nodiscard]] int reports() const { return m_reports; }

    [[nodiscard]] const std::vector<std::string>& opened() const { return m_opened; }

    [[nodiscard]] const std::vector<std::pair<std::string, bool>>& saved() const { return m_saved; }

private:
    tg::cmd::CommandRegistry m_commands;
    std::vector<std::vector<std::string>> m_runs;
    int m_reports{0};
    std::vector<std::string> m_opened;
    std::vector<std::pair<std::string, bool>> m_saved;
};

/// A transport in memory: the messages to read, and those written.
class MemoryTransport final : public LineTransport {
public:
    explicit MemoryTransport(std::deque<std::string> input) : m_input(std::move(input)) {}

    [[nodiscard]] Read readLine(std::string& line, std::size_t maxBytes) override {
        line.clear();
        if (m_input.empty()) {
            return Read::End;
        }
        std::string next = std::move(m_input.front());
        m_input.pop_front();
        if (next.size() > maxBytes) {
            return Read::TooLong;
        }
        line = std::move(next);
        return Read::Line;
    }

    [[nodiscard]] bool writeLine(std::string_view line) override {
        if (m_failWrites) {
            return false;
        }
        m_written.push_back(json::parse(line));
        return true;
    }

    [[nodiscard]] const std::vector<json>& written() const { return m_written; }

    void failWrites() { m_failWrites = true; }

private:
    std::deque<std::string> m_input;
    std::vector<json> m_written;
    bool m_failWrites{false};
};

json request(int id, const std::string& method, json params = json::object()) {
    return json{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", std::move(params)}};
}

json initializeRequest(int id) {
    return request(id, "initialize",
                   json{{"protocolVersion", "2025-11-25"},
                        {"capabilities", json::object()},
                        {"clientInfo", json{{"name", "test"}, {"version", "1"}}}});
}

json callRequest(int id, const std::string& tool, json arguments) {
    return request(id, "tools/call", json{{"name", tool}, {"arguments", std::move(arguments)}});
}

/// Runs a server with `tools` on the messages, over a MemoryTransport.
std::vector<json> exchange(std::vector<std::unique_ptr<Tool>> tools,
                           const std::vector<json>& messages) {
    std::deque<std::string> input;
    for (const json& message : messages) {
        input.push_back(message.dump());
    }
    MemoryTransport transport{std::move(input)};
    Server server(std::move(tools));
    std::ostringstream log;
    EXPECT_EQ(server.run(transport, log), 0);
    return transport.written();
}

/// The answer of Server::answer(), or null for none.
json parsed(const std::optional<std::string>& answer) {
    return answer ? json::parse(*answer) : json();
}

std::vector<std::string> listedNames(const json& answer) {
    std::vector<std::string> names;
    for (const json& tool : answer.at("result").at("tools")) {
        names.push_back(tool.at("name").get<std::string>());
    }
    return names;
}

const std::vector<std::string> kProgramToolNames{
    "list_commands", "describe_command", "run", "report", "open", "save"};

// --- the tools of the program over a host --------------------------------

TEST(McpToolHost, ProgramToolsAreTheSixAndRefuseTheOthers) {
    auto host = std::make_shared<FakeHost>();
    const std::vector<json> answers = exchange(
        tg::mcp::programTools(host),
        {initializeRequest(1), request(2, "tools/list"), callRequest(3, "reset", json::object()),
         callRequest(4, "run_script", json{{"script", "LINE\n"}}),
         callRequest(5, "check_case", json{{"case", "x"}}),
         callRequest(6, "ask", json{{"questions", "; nothing\n"}})});
    ASSERT_EQ(answers.size(), 6U);
    EXPECT_EQ(listedNames(answers[1]), kProgramToolNames);
    for (std::size_t i = 2; i < answers.size(); ++i) {
        // As an unknown tool: the same answer as for a name that does not exist.
        EXPECT_EQ(answers[i].at("error").at("code"), tg::mcp::kInvalidParams) << answers[i].dump();
        EXPECT_EQ(
            answers[i].at("error").at("message").get<std::string>().rfind("Unknown tool: ", 0), 0U);
    }
    EXPECT_TRUE(host->runs().empty());
}

TEST(McpToolHost, EveryToolReachesTheHost) {
    auto host = std::make_shared<FakeHost>();
    const std::vector<json> answers = exchange(
        tg::mcp::programTools(host),
        {initializeRequest(1), callRequest(2, "run", json{{"lines", {"LINE", "0,0", "10,0", ""}}}),
         callRequest(3, "report", json::object()),
         callRequest(4, "open", json{{"path", "/home/user/a.dxf"}}),
         callRequest(5, "save", json{{"path", "C:\\drawings\\b.cadx"}, {"replace", true}}),
         callRequest(6, "save", json{{"path", "c.dxf"}})});
    ASSERT_EQ(answers.size(), 6U);
    for (std::size_t i = 1; i < answers.size(); ++i) {
        EXPECT_FALSE(answers[i].at("result").at("isError").get<bool>()) << answers[i].dump();
    }
    ASSERT_EQ(host->runs().size(), 1U);
    EXPECT_EQ(host->runs()[0], (std::vector<std::string>{"LINE", "0,0", "10,0", ""}));
    EXPECT_EQ(answers[1].at("result").at("content")[0].at("text"), "prompt: Command:");
    EXPECT_EQ(host->reports(), 1);
    EXPECT_EQ(answers[2].at("result").at("content")[0].at("text"), "tecnigrafo-report 1\n");
    EXPECT_EQ(host->opened(), (std::vector<std::string>{"/home/user/a.dxf"}));
    EXPECT_EQ(host->saved(), (std::vector<std::pair<std::string, bool>>{
                                 {"C:\\drawings\\b.cadx", true}, {"c.dxf", false}}));
}

TEST(McpToolHost, ArgumentsAreCheckedBeforeTheHost) {
    auto host = std::make_shared<FakeHost>();
    const std::vector<json> answers =
        exchange(tg::mcp::programTools(host),
                 {initializeRequest(1), callRequest(2, "run", json{{"lines", "LINE"}}),
                  callRequest(3, "open", json::object()),
                  callRequest(4, "save", json{{"path", "a.dxf"}, {"replace", "yes"}}),
                  callRequest(5, "report", json{{"extra", 1}})});
    ASSERT_EQ(answers.size(), 5U);
    for (std::size_t i = 1; i < answers.size(); ++i) {
        EXPECT_TRUE(answers[i].at("result").at("isError").get<bool>()) << answers[i].dump();
    }
    EXPECT_EQ(answers[2].at("result").at("content")[0].at("text"),
              "open needs \"path\", the path of a file.");
    EXPECT_TRUE(host->runs().empty());
    EXPECT_EQ(host->reports(), 0);
    EXPECT_TRUE(host->opened().empty());
    EXPECT_TRUE(host->saved().empty());
}

TEST(McpToolHost, CommandToolsUseTheCommandsOfTheHost) {
    auto host = std::make_shared<FakeHost>();
    auto registry = std::make_shared<tg::cmd::CommandRegistry>();
    tg::cmd::registerBuiltinCommands(*registry);
    std::vector<std::unique_ptr<Tool>> hosted = tg::mcp::hostCommandTools(host);
    std::vector<std::unique_ptr<Tool>> own = tg::mcp::commandTools(registry);
    ASSERT_EQ(hosted.size(), 2U);
    ASSERT_EQ(own.size(), 2U);
    for (std::size_t i = 0; i < hosted.size(); ++i) {
        EXPECT_EQ(hosted[i]->name(), own[i]->name());
        EXPECT_EQ(hosted[i]->description(), own[i]->description());
        EXPECT_EQ(hosted[i]->inputSchema(), own[i]->inputSchema());
    }
    EXPECT_EQ(hosted[0]->call(json::object()).text, own[0]->call(json::object()).text);
    const json line{{"name", "l"}};
    EXPECT_EQ(hosted[1]->call(line).text, own[1]->call(line).text);
    EXPECT_TRUE(hosted[1]->call(json{{"name", "NOPE"}}).isError);
}

TEST(McpToolHost, WithoutAHostTheToolsDescribeThemselvesAndCallsFail) {
    // The bridge of the program answers tools/list before it reaches the
    // program (docs/adr/0051, point 1).
    const std::vector<json> answers = exchange(
        tg::mcp::programTools(nullptr), {initializeRequest(1), request(2, "tools/list"),
                                         callRequest(3, "run", json{{"lines", {"LINE"}}}),
                                         callRequest(4, "list_commands", json::object()),
                                         callRequest(5, "describe_command", json{{"name", "LINE"}}),
                                         callRequest(6, "open", json{{"path", "a.dxf"}})});
    ASSERT_EQ(answers.size(), 6U);
    EXPECT_EQ(listedNames(answers[1]), kProgramToolNames);
    for (const json& tool : answers[1].at("result").at("tools")) {
        EXPECT_FALSE(tool.at("description").get<std::string>().empty());
        EXPECT_EQ(tool.at("inputSchema").at("type"), "object");
        // The program has no --root: its tools never speak of it.
        EXPECT_EQ(tool.dump().find("--root"), std::string::npos) << tool.dump();
    }
    for (std::size_t i = 2; i < answers.size(); ++i) {
        const json& result = answers[i].at("result");
        EXPECT_TRUE(result.at("isError").get<bool>()) << answers[i].dump();
        EXPECT_EQ(result.at("content")[0].at("text").get<std::string>(),
                  std::string{tg::mcp::kNotRunning});
    }
}

TEST(McpToolHost, TheSessionOfTecnigrafoMcpIsAHost) {
    auto session = std::make_shared<tg::mcp::Session>(tg::mcp::SessionOptions{});
    std::vector<std::unique_ptr<Tool>> tools = tg::mcp::programTools(session);
    ASSERT_EQ(tools.size(), kProgramToolNames.size());
    const ToolResult drawn = tools[2]->call(json{{"lines", {"LINE", "0,0", "10,0", ""}}});
    ASSERT_FALSE(drawn.isError) << drawn.text;
    EXPECT_NE(drawn.text.find("prompt: Command:"), std::string::npos) << drawn.text;
    // The commands do not change with reset.
    const tg::cmd::CommandRegistry* before = &session->commands();
    EXPECT_EQ(session->reset(), "prompt: Command:");
    EXPECT_EQ(&session->commands(), before);
    EXPECT_FALSE(tools[0]->call(json::object()).isError);
}

TEST(McpToolHost, SessionToolsKeepTheirOrderAndTexts) {
    auto session = std::make_shared<tg::mcp::Session>(tg::mcp::SessionOptions{});
    std::vector<std::string> names;
    for (const std::unique_ptr<Tool>& tool : tg::mcp::sessionTools(session)) {
        names.push_back(tool->name());
        if (tool->name() == "open" || tool->name() == "save") {
            EXPECT_NE(tool->description().find("--root"), std::string::npos);
        }
    }
    EXPECT_EQ(names, (std::vector<std::string>{"run", "report", "reset", "open", "save"}));
}

// --- the transport of lines ----------------------------------------------

TEST(McpTransport, InitializeListAndCallOverLinesInMemory) {
    auto host = std::make_shared<FakeHost>();
    std::deque<std::string> input{
        initializeRequest(1).dump(),
        json{{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}}.dump(),
        "",
        request(2, "tools/list").dump(),
        callRequest(3, "run", json{{"lines", {"LINE"}}}).dump(),
    };
    MemoryTransport transport{std::move(input)};
    Server server(tg::mcp::programTools(host));
    std::ostringstream log;
    EXPECT_EQ(server.run(transport, log), 0);
    ASSERT_EQ(transport.written().size(), 3U);  // no answer to the notification nor to ""
    EXPECT_EQ(transport.written()[0].at("id"), 1);
    EXPECT_EQ(transport.written()[0].at("result").at("protocolVersion"), "2025-11-25");
    EXPECT_EQ(listedNames(transport.written()[1]), kProgramToolNames);
    EXPECT_EQ(transport.written()[2].at("id"), 3);
    EXPECT_FALSE(transport.written()[2].at("result").at("isError").get<bool>());
    EXPECT_EQ(host->runs().size(), 1U);
}

TEST(McpTransport, TooLongMessageIsRefusedAndTheServerGoesOn) {
    std::deque<std::string> input{std::string(129, 'x'), request(2, "ping").dump()};
    MemoryTransport transport{std::move(input)};
    Server server(tg::mcp::programTools(nullptr), tg::mcp::ServerOptions{.maxMessageBytes = 128});
    std::ostringstream log;
    EXPECT_EQ(server.run(transport, log), 0);
    ASSERT_EQ(transport.written().size(), 2U);
    EXPECT_TRUE(transport.written()[0].at("id").is_null());
    EXPECT_EQ(transport.written()[0].at("error").at("code"), tg::mcp::kInvalidRequest);
    EXPECT_EQ(transport.written()[1].at("id"), 2);
    EXPECT_NE(log.str().find("128 bytes was refused"), std::string::npos) << log.str();
}

TEST(McpTransport, AFailedWriteEndsTheServer) {
    MemoryTransport transport{{request(1, "ping").dump(), request(2, "ping").dump()}};
    transport.failWrites();
    Server server(tg::mcp::programTools(nullptr));
    std::ostringstream log;
    EXPECT_EQ(server.run(transport, log), 1);
    EXPECT_NE(log.str().find("cannot write the answer"), std::string::npos);
}

TEST(McpTransport, StreamTransportReadsLinesAsBefore) {
    std::istringstream in("one\r\n\ntwo-and-more\nlast");
    std::ostringstream out;
    tg::mcp::StreamTransport transport{in, out};
    std::string line;
    EXPECT_EQ(transport.readLine(line, 8), LineTransport::Read::Line);
    EXPECT_EQ(line, "one");
    EXPECT_EQ(transport.readLine(line, 8), LineTransport::Read::Line);
    EXPECT_EQ(line, "");
    EXPECT_EQ(transport.readLine(line, 8), LineTransport::Read::TooLong);
    EXPECT_EQ(line, "");
    EXPECT_EQ(transport.readLine(line, 8), LineTransport::Read::Line);
    EXPECT_EQ(line, "last");
    EXPECT_EQ(transport.readLine(line, 8), LineTransport::Read::End);
    EXPECT_TRUE(transport.writeLine("{}"));
    EXPECT_EQ(out.str(), "{}\n");
}

TEST(McpTransport, AnswerTakesOneMessageAtATime) {
    auto host = std::make_shared<FakeHost>();
    Server server(tg::mcp::programTools(host), tg::mcp::ServerOptions{.maxMessageBytes = 512});
    std::ostringstream log;
    const std::optional<std::string> init = server.answer(initializeRequest(1).dump(), log);
    ASSERT_TRUE(init);
    EXPECT_EQ(parsed(init).at("id"), 1);
    EXPECT_FALSE(server.answer(
        json{{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}}.dump(), log));
    EXPECT_FALSE(server.answer("  ", log));
    const std::optional<std::string> call =
        server.answer(callRequest(2, "run", json{{"lines", {"LINE"}}}).dump(), log);
    ASSERT_TRUE(call);
    EXPECT_EQ(parsed(call).at("result").at("content")[0].at("text"), "prompt: Command:");
    const std::optional<std::string> large = server.answer(std::string(513, ' '), log);
    ASSERT_TRUE(large);
    EXPECT_EQ(parsed(large).at("error").at("code"), tg::mcp::kInvalidRequest);
    EXPECT_EQ(host->runs().size(), 1U);
}

}  // namespace
