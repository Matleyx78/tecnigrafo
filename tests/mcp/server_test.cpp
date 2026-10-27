// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The MCP server with streams in memory (0.6-P25, docs/adr/0027).

#include "tg/mcp/server.hpp"

#include "nlohmann_json/json.hpp"
#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_description.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/mcp/tool.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using nlohmann::json;
using tg::mcp::Server;
using tg::mcp::ServerOptions;

constexpr std::string_view kVersionKey = "io.modelcontextprotocol/protocolVersion";
constexpr std::string_view kCapabilitiesKey = "io.modelcontextprotocol/clientCapabilities";

std::shared_ptr<tg::cmd::CommandRegistry> builtinRegistry() {
    auto registry = std::make_shared<tg::cmd::CommandRegistry>();
    tg::cmd::registerBuiltinCommands(*registry);
    return registry;
}

/// A tool that throws, to check that the server survives a bug of a tool.
class ThrowingTool final : public tg::mcp::Tool {
public:
    [[nodiscard]] std::string name() const override { return "throws"; }

    [[nodiscard]] std::string description() const override { return "Always throws."; }

    [[nodiscard]] json inputSchema() const override { return json{{"type", "object"}}; }

    [[nodiscard]] tg::mcp::ToolResult call(const json& /*arguments*/) override {
        throw std::runtime_error("broken tool");
    }
};

struct Exchange {
    std::vector<json> answers;
    std::string log;
    int exitCode{0};
};

/// Runs a new server on `input` and parses every line it writes.
Exchange run(const std::string& input, ServerOptions options = {}, bool throwingTool = false) {
    std::vector<std::unique_ptr<tg::mcp::Tool>> tools = tg::mcp::commandTools(builtinRegistry());
    if (throwingTool) {
        tools.push_back(std::make_unique<ThrowingTool>());
    }
    Server server(std::move(tools), options);
    std::istringstream in(input);
    std::ostringstream out;
    std::ostringstream log;
    Exchange exchange;
    exchange.exitCode = server.run(in, out, log);
    exchange.log = log.str();
    std::istringstream lines(out.str());
    std::string line;
    while (std::getline(lines, line)) {
        exchange.answers.push_back(json::parse(line));
    }
    return exchange;
}

std::string lines(const std::vector<json>& messages) {
    std::string input;
    for (const json& message : messages) {
        input += message.dump() + "\n";
    }
    return input;
}

json request(int id, const std::string& method, json params = json::object()) {
    return json{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", std::move(params)}};
}

json notification(const std::string& method) {
    return json{{"jsonrpc", "2.0"}, {"method", method}};
}

json initializeRequest(int id, const std::string& version = "2025-11-25") {
    return request(id, "initialize",
                   json{{"protocolVersion", version},
                        {"capabilities", json::object()},
                        {"clientInfo", json{{"name", "test"}, {"version", "1"}}}});
}

json modernMeta(const std::string& version = "2026-07-28") {
    return json{{kVersionKey, version}, {kCapabilitiesKey, json::object()}};
}

json modernRequest(int id, const std::string& method, json params = json::object()) {
    params["_meta"] = modernMeta();
    return request(id, method, std::move(params));
}

json callRequest(int id, const std::string& tool, json arguments) {
    return request(id, "tools/call", json{{"name", tool}, {"arguments", std::move(arguments)}});
}

int errorCode(const json& answer) {
    return answer.at("error").at("code").get<int>();
}

/// The structured content of a successful tool call.
const json& structured(const json& answer) {
    const json& result = answer.at("result");
    EXPECT_FALSE(result.at("isError").get<bool>()) << answer.dump();
    return result.at("structuredContent");
}

// --- the sequences of the two revisions ---------------------------------

TEST(McpServer, LegacySequenceInitializeListAndCall) {
    const Exchange exchange = run(lines(
        {initializeRequest(1), notification("notifications/initialized"), request(2, "tools/list"),
         callRequest(3, "describe_command", json{{"name", "LINE"}})}));
    EXPECT_EQ(exchange.exitCode, 0);
    ASSERT_EQ(exchange.answers.size(), 3U);  // the notification has no answer

    const json& init = exchange.answers[0];
    EXPECT_EQ(init.at("jsonrpc"), "2.0");
    EXPECT_EQ(init.at("id"), 1);
    const json& initResult = init.at("result");
    EXPECT_EQ(initResult.at("protocolVersion"), "2025-11-25");
    EXPECT_EQ(initResult.at("serverInfo").at("name"), "tecnigrafo-mcp");
    EXPECT_FALSE(initResult.at("serverInfo").at("version").get<std::string>().empty());
    EXPECT_TRUE(initResult.at("capabilities").at("tools").is_object());

    const json& tools = exchange.answers[1].at("result").at("tools");
    ASSERT_EQ(tools.size(), 2U);
    EXPECT_EQ(tools[0].at("name"), "list_commands");
    EXPECT_EQ(tools[1].at("name"), "describe_command");
    for (const json& tool : tools) {
        EXPECT_FALSE(tool.at("description").get<std::string>().empty());
        EXPECT_EQ(tool.at("inputSchema").at("type"), "object");
    }
    // A legacy result has none of the fields of the modern revision.
    EXPECT_FALSE(exchange.answers[1].at("result").contains("resultType"));

    const json& call = exchange.answers[2];
    EXPECT_EQ(call.at("id"), 3);
    EXPECT_EQ(structured(call).at("name"), "LINE");
    const json& content = call.at("result").at("content");
    ASSERT_EQ(content.size(), 1U);
    EXPECT_EQ(content[0].at("type"), "text");
    EXPECT_EQ(json::parse(content[0].at("text").get<std::string>()), structured(call));
}

TEST(McpServer, InitializeAnswersTheLegacyRevisionForAnyRequestedVersion) {
    for (const std::string version : {"2025-11-25", "2025-06-18", "2026-07-28", "1900-01-01"}) {
        const Exchange exchange = run(lines({initializeRequest(1, version)}));
        ASSERT_EQ(exchange.answers.size(), 1U) << version;
        EXPECT_EQ(exchange.answers[0].at("result").at("protocolVersion"), "2025-11-25") << version;
    }
}

TEST(McpServer, ModernSequenceNeedsNoInitialize) {
    const Exchange exchange = run(lines(
        {modernRequest(1, "server/discover"), modernRequest(2, "tools/list"),
         modernRequest(3, "tools/call",
                       json{{"name", "describe_command"}, {"arguments", json{{"name", "pl"}}}})}));
    ASSERT_EQ(exchange.answers.size(), 3U);
    for (const json& answer : exchange.answers) {
        const json& result = answer.at("result");
        EXPECT_EQ(result.at("resultType"), "complete");
        EXPECT_EQ(result.at("_meta").at("io.modelcontextprotocol/serverInfo").at("name"),
                  "tecnigrafo-mcp");
    }

    const json& discover = exchange.answers[0].at("result");
    EXPECT_EQ(discover.at("supportedVersions"), json::array({"2026-07-28"}));
    EXPECT_TRUE(discover.at("capabilities").at("tools").is_object());
    EXPECT_GE(discover.at("ttlMs").get<int>(), 0);
    EXPECT_EQ(discover.at("cacheScope"), "private");

    const json& list = exchange.answers[1].at("result");
    EXPECT_EQ(list.at("tools").size(), 2U);
    EXPECT_GE(list.at("ttlMs").get<int>(), 0);
    EXPECT_EQ(list.at("cacheScope"), "private");

    EXPECT_EQ(structured(exchange.answers[2]).at("name"), "PLINE");
}

TEST(McpServer, ModernRequestWithAnotherVersionIsRefused) {
    json params = json::object();
    params["_meta"] = modernMeta("2025-11-25");
    const Exchange exchange = run(lines({request(7, "tools/list", params)}));
    ASSERT_EQ(exchange.answers.size(), 1U);
    const json& answer = exchange.answers[0];
    EXPECT_EQ(answer.at("id"), 7);
    EXPECT_EQ(errorCode(answer), -32022);
    EXPECT_EQ(answer.at("error").at("data").at("requested"), "2025-11-25");
    EXPECT_EQ(answer.at("error").at("data").at("supported"), json::array({"2026-07-28"}));
}

TEST(McpServer, ModernRequestWithIncompleteMetaIsInvalidParams) {
    json noCapabilities = json::object();
    noCapabilities["_meta"] = json{{kVersionKey, "2026-07-28"}};
    json numericVersion = json::object();
    numericVersion["_meta"] = json{{kVersionKey, 20260728}, {kCapabilitiesKey, json::object()}};
    const Exchange exchange =
        run(lines({request(1, "tools/list", noCapabilities),
                   request(2, "server/discover", numericVersion), request(3, "server/discover")}));
    ASSERT_EQ(exchange.answers.size(), 3U);
    for (const json& answer : exchange.answers) {
        EXPECT_EQ(errorCode(answer), -32602) << answer.dump();
    }
}

TEST(McpServer, ModernRevisionHasNoPingNorInitializeState) {
    const Exchange exchange =
        run(lines({modernRequest(1, "ping"), modernRequest(2, "resources/list")}));
    ASSERT_EQ(exchange.answers.size(), 2U);
    EXPECT_EQ(errorCode(exchange.answers[0]), -32601);
    EXPECT_EQ(errorCode(exchange.answers[1]), -32601);
}

// --- before initialize ----------------------------------------------------

TEST(McpServer, BeforeInitializeOnlyPingIsAnswered) {
    const Exchange exchange =
        run(lines({request(1, "tools/list"), callRequest(2, "list_commands", json::object()),
                   request(3, "ping"), request(4, "unknown/method")}));
    ASSERT_EQ(exchange.answers.size(), 4U);
    EXPECT_EQ(errorCode(exchange.answers[0]), -32600);
    EXPECT_EQ(errorCode(exchange.answers[1]), -32600);
    EXPECT_EQ(exchange.answers[2].at("result"), json::object());
    EXPECT_EQ(errorCode(exchange.answers[3]), -32601);
}

TEST(McpServer, InitializeOnlyOnce) {
    const Exchange exchange = run(lines({initializeRequest(1), initializeRequest(2)}));
    ASSERT_EQ(exchange.answers.size(), 2U);
    EXPECT_TRUE(exchange.answers[0].contains("result"));
    EXPECT_EQ(errorCode(exchange.answers[1]), -32600);
}

// --- JSON-RPC errors ------------------------------------------------------

TEST(McpServer, InvalidJsonIsAParseErrorAndTheServerGoesOn) {
    const Exchange exchange =
        run("{\"jsonrpc\": \"2.0\", \"id\": 1,\n" + lines({request(2, "ping")}));
    ASSERT_EQ(exchange.answers.size(), 2U);
    EXPECT_EQ(errorCode(exchange.answers[0]), -32700);
    EXPECT_TRUE(exchange.answers[0].at("id").is_null());
    EXPECT_EQ(exchange.answers[1].at("id"), 2);
    EXPECT_FALSE(exchange.log.empty());
}

TEST(McpServer, InvalidRequests) {
    const std::vector<std::string> invalid = {
        R"([{"jsonrpc": "2.0", "id": 1, "method": "ping"}])",  // a batch
        R"(42)",
        R"("ping")",
        R"({"id": 1, "method": "ping"})",                    // no jsonrpc
        R"({"jsonrpc": "1.0", "id": 1, "method": "ping"})",  // another version
        R"({"jsonrpc": "2.0", "id": 1})",                    // no method
        R"({"jsonrpc": "2.0", "id": 1, "method": 5})",
        R"({"jsonrpc": "2.0", "id": null, "method": "ping"})",
        R"({"jsonrpc": "2.0", "id": 1.5, "method": "ping"})",
        R"({"jsonrpc": "2.0", "id": true, "method": "ping"})",
        R"({"jsonrpc": "2.0", "id": {}, "method": "ping"})",
        R"({"jsonrpc": "2.0", "id": 1, "result": {}})",  // a response
    };
    std::string input;
    for (const std::string& line : invalid) {
        input += line + "\n";
    }
    const Exchange exchange = run(input);
    ASSERT_EQ(exchange.answers.size(), invalid.size());
    for (std::size_t i = 0; i < invalid.size(); ++i) {
        EXPECT_EQ(errorCode(exchange.answers[i]), -32600) << invalid[i];
    }
    // The id comes back only when it is a valid one.
    EXPECT_EQ(exchange.answers[3].at("id"), 1);
    EXPECT_TRUE(exchange.answers[7].at("id").is_null());
    EXPECT_TRUE(exchange.answers[8].at("id").is_null());
}

TEST(McpServer, MethodNotFound) {
    const Exchange exchange = run(
        lines({initializeRequest(1), request(2, "prompts/list"), request(3, "resources/read")}));
    ASSERT_EQ(exchange.answers.size(), 3U);
    EXPECT_EQ(errorCode(exchange.answers[1]), -32601);
    EXPECT_EQ(errorCode(exchange.answers[2]), -32601);
}

TEST(McpServer, InvalidParams) {
    const Exchange exchange = run(lines({
        json{{"jsonrpc", "2.0"}, {"id", 1}, {"method", "ping"}, {"params", json::array()}},
        request(2, "initialize", json{{"capabilities", json::object()}}),
        request(3, "initialize", json{{"protocolVersion", "2025-11-25"}}),
        initializeRequest(4),
        request(5, "tools/call", json::object()),
        request(6, "tools/call", json{{"name", 5}}),
        request(7, "tools/call", json{{"name", "list_commands"}, {"arguments", json::array()}}),
        callRequest(8, "no_such_tool", json::object()),
    }));
    ASSERT_EQ(exchange.answers.size(), 8U);
    for (const std::size_t i : {0U, 1U, 2U, 4U, 5U, 6U, 7U}) {
        EXPECT_EQ(errorCode(exchange.answers[i]), -32602) << exchange.answers[i].dump();
        EXPECT_EQ(exchange.answers[i].at("id"), static_cast<int>(i) + 1);
    }
    EXPECT_TRUE(exchange.answers[3].contains("result"));
}

TEST(McpServer, MessageTooLargeIsRefusedWithoutParsing) {
    ServerOptions options;
    options.maxMessageBytes = 64;
    const std::string atLimit =
        R"({"jsonrpc":"2.0","id":1,"method":"ping","params":{"x":"123456"}})";
    ASSERT_EQ(atLimit.size(), 64U);
    // Valid JSON, but one byte too many: refused with an unknown id.
    const std::string overLimit =
        R"({"jsonrpc":"2.0","id":2,"method":"ping","params":{"x":"1234567"}})";
    const Exchange exchange =
        run(atLimit + "\n" + overLimit + "\n" + lines({request(3, "ping")}), options);
    ASSERT_EQ(exchange.answers.size(), 3U);
    EXPECT_EQ(exchange.answers[0].at("id"), 1);
    EXPECT_TRUE(exchange.answers[0].contains("result"));
    EXPECT_EQ(errorCode(exchange.answers[1]), -32600);
    EXPECT_TRUE(exchange.answers[1].at("id").is_null());
    EXPECT_EQ(exchange.answers[2].at("id"), 3);
    EXPECT_FALSE(exchange.log.empty());
}

TEST(McpServer, DefaultLimitIsFourMebibytes) {
    EXPECT_EQ(ServerOptions{}.maxMessageBytes, 4U * 1024U * 1024U);
}

TEST(McpServer, NotificationsAreNeverAnswered) {
    const Exchange exchange = run(lines({
        notification("notifications/initialized"),
        notification("notifications/cancelled"),
        notification("tools/list"),
        notification("no/such/method"),
        json{{"jsonrpc", "2.0"}, {"method", "ping"}, {"params", json::array()}},
    }));
    EXPECT_TRUE(exchange.answers.empty());
    EXPECT_EQ(exchange.exitCode, 0);
}

TEST(McpServer, StringIdsCrLfBlankLinesAndNoFinalNewline) {
    const json ping = json{{"jsonrpc", "2.0"}, {"id", "a-1"}, {"method", "ping"}};
    const json last = json{{"jsonrpc", "2.0"}, {"id", "a-2"}, {"method", "ping"}};
    const Exchange exchange = run("\n  \r\n" + ping.dump() + "\r\n" + last.dump());
    ASSERT_EQ(exchange.answers.size(), 2U);
    EXPECT_EQ(exchange.answers[0].at("id"), "a-1");
    EXPECT_EQ(exchange.answers[1].at("id"), "a-2");
}

TEST(McpServer, EmptyInputEndsAtOnce) {
    const Exchange exchange = run("");
    EXPECT_EQ(exchange.exitCode, 0);
    EXPECT_TRUE(exchange.answers.empty());
}

TEST(McpServer, ToolThatThrowsIsAnInternalError) {
    const Exchange exchange = run(
        lines({initializeRequest(1), callRequest(2, "throws", json::object()), request(3, "ping")}),
        ServerOptions{}, /*throwingTool=*/true);
    ASSERT_EQ(exchange.answers.size(), 3U);
    EXPECT_EQ(errorCode(exchange.answers[1]), -32603);
    EXPECT_EQ(exchange.answers[2].at("id"), 3);
}

// --- list_commands and describe_command -----------------------------------

TEST(McpCommandTools, ListCommandsGivesEveryCommand) {
    const auto registry = builtinRegistry();
    const Exchange exchange =
        run(lines({initializeRequest(1), callRequest(2, "list_commands", json::object())}));
    ASSERT_EQ(exchange.answers.size(), 2U);
    const json& commands = structured(exchange.answers[1]).at("commands");
    const std::vector<const tg::cmd::CommandDescription*> descriptions = registry->descriptions();
    ASSERT_EQ(commands.size(), descriptions.size());
    ASSERT_FALSE(descriptions.empty());
    for (std::size_t i = 0; i < descriptions.size(); ++i) {
        EXPECT_EQ(commands[i].at("name"), descriptions[i]->name);
        EXPECT_EQ(commands[i].at("aliases"), json(descriptions[i]->aliases));
        EXPECT_EQ(commands[i].at("summary"), descriptions[i]->summary);
    }
}

TEST(McpCommandTools, DescribeCommandByNameAndAlias) {
    const auto registry = builtinRegistry();
    const tg::cmd::CommandDescription* pline = registry->describe("PLINE");
    ASSERT_NE(pline, nullptr);
    const Exchange exchange = run(
        lines({initializeRequest(1), callRequest(2, "describe_command", json{{"name", "PLINE"}}),
               callRequest(3, "describe_command", json{{"name", "pl"}})}));
    ASSERT_EQ(exchange.answers.size(), 3U);
    const json& byName = structured(exchange.answers[1]);
    EXPECT_EQ(byName, structured(exchange.answers[2]));

    EXPECT_EQ(byName.at("name"), "PLINE");
    EXPECT_EQ(byName.at("aliases"), json(pline->aliases));
    EXPECT_EQ(byName.at("summary"), pline->summary);
    EXPECT_EQ(byName.at("selectsFirst"), pline->selectsFirst);
    const json& prompts = byName.at("prompts");
    ASSERT_EQ(prompts.size(), pline->prompts.size());
    ASSERT_FALSE(prompts.empty());
    EXPECT_EQ(prompts[0].at("id"), "start-point");
    EXPECT_EQ(prompts[0].at("text"), "Specify start point:");
    EXPECT_EQ(prompts[0].at("value"), "point");
    EXPECT_EQ(prompts[0].at("keywords"), json::array());
    EXPECT_EQ(prompts[0].at("acceptsEmpty"), false);
    for (std::size_t i = 0; i < prompts.size(); ++i) {
        const tg::cmd::PromptDescription& prompt = pline->prompts[i];
        EXPECT_EQ(prompts[i].at("id"), prompt.id);
        EXPECT_EQ(prompts[i].at("text"), prompt.text);
        EXPECT_EQ(prompts[i].at("acceptsEmpty"), prompt.acceptsEmpty);
        ASSERT_EQ(prompts[i].at("keywords").size(), prompt.keywords.size());
        for (std::size_t k = 0; k < prompt.keywords.size(); ++k) {
            EXPECT_EQ(prompts[i].at("keywords")[k].at("name"), prompt.keywords[k].name);
            EXPECT_EQ(prompts[i].at("keywords")[k].at("shortcut"), prompt.keywords[k].shortcut);
        }
    }
}

TEST(McpCommandTools, EveryCommandCanBeDescribed) {
    const auto registry = builtinRegistry();
    std::vector<json> messages{initializeRequest(1)};
    int id = 2;
    for (const tg::cmd::CommandDescription* description : registry->descriptions()) {
        messages.push_back(
            callRequest(id++, "describe_command", json{{"name", description->name}}));
    }
    const Exchange exchange = run(lines(messages));
    ASSERT_EQ(exchange.answers.size(), messages.size());
    for (std::size_t i = 1; i < exchange.answers.size(); ++i) {
        const json& described = structured(exchange.answers[i]);
        EXPECT_EQ(described.at("name"), registry->descriptions()[i - 1]->name);
        for (const json& prompt : described.at("prompts")) {
            EXPECT_NE(prompt.at("value"), "unknown") << described.at("name");
        }
    }
}

TEST(McpCommandTools, UnknownCommandAndWrongArgumentsAreToolErrors) {
    const Exchange exchange = run(lines({
        initializeRequest(1), callRequest(2, "describe_command", json{{"name", "NOSUCHCOMMAND"}}),
        callRequest(3, "describe_command", json::object()),
        callRequest(4, "describe_command", json{{"name", 5}}),
        callRequest(5, "describe_command", json{{"name", "LINE"}, {"extra", true}}),
        callRequest(6, "list_commands", json{{"name", "LINE"}}),
        request(7, "tools/call", json{{"name", "describe_command"}}),  // no arguments at all
    }));
    ASSERT_EQ(exchange.answers.size(), 7U);
    for (std::size_t i = 1; i < exchange.answers.size(); ++i) {
        const json& result = exchange.answers[i].at("result");
        EXPECT_TRUE(result.at("isError").get<bool>()) << exchange.answers[i].dump();
        EXPECT_FALSE(result.contains("structuredContent"));
        EXPECT_FALSE(result.at("content")[0].at("text").get<std::string>().empty());
    }
    EXPECT_EQ(exchange.answers[1].at("result").at("content")[0].at("text"),
              "Unknown command \"NOSUCHCOMMAND\".");
}

}  // namespace
