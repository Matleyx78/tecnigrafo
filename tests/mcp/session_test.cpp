// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The live session and the tools of tecnigrafo-mcp (0.6-P26, docs/adr/0027
// points 4-5).

#include "tg/mcp/session.hpp"

#include "nlohmann_json/json.hpp"
#include "tg/mcp/server.hpp"
#include "tg/mcp/tool.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;
using nlohmann::json;
using tg::mcp::Session;
using tg::mcp::SessionOptions;
using tg::mcp::Tool;
using tg::mcp::ToolResult;

/// An empty folder of this test, for --root.
fs::path emptyRoot(std::string_view name) {
    const fs::path root = fs::path{TG_SCRIPT_TEST_OUTPUT} / "mcp-session" / name;
    fs::remove_all(root);
    fs::create_directories(root);
    return root;
}

/// The tools of the server, as tecnigrafo-mcp builds them, by name.
class Tools {
public:
    explicit Tools(const SessionOptions& options) {
        m_tools = tg::mcp::sessionTools(std::make_shared<Session>(options));
        for (std::unique_ptr<Tool>& tool : tg::mcp::caseTools(options)) {
            m_tools.push_back(std::move(tool));
        }
    }

    [[nodiscard]] ToolResult call(std::string_view name, const json& arguments = json::object()) {
        for (const std::unique_ptr<Tool>& tool : m_tools) {
            if (tool->name() == name) {
                return tool->call(arguments);
            }
        }
        ADD_FAILURE() << "no tool " << name;
        return ToolResult::error("no tool");
    }

    [[nodiscard]] ToolResult run(const std::vector<std::string>& lines) {
        return call("run", json{{"lines", lines}});
    }

    [[nodiscard]] std::string report() { return call("report").text; }

    [[nodiscard]] const std::vector<std::unique_ptr<Tool>>& all() const { return m_tools; }

private:
    std::vector<std::unique_ptr<Tool>> m_tools;
};

/// The structured content of a result, or an empty object after a failure.
const json& structured(const ToolResult& result) {
    static const json kNone = json::object();
    if (!result.structured) {
        ADD_FAILURE() << "no structured content: " << result.text;
        return kNone;
    }
    return *result.structured;
}

/// The lines of the field `name` of the last section of a report.
std::string field(const std::string& report, std::string_view name) {
    const std::string start = "\n" + std::string{name} + ":";
    const std::size_t at = report.rfind(start);
    if (at == std::string::npos) {
        return "(missing)";
    }
    std::string value;
    std::istringstream lines(report.substr(at + 1));
    std::string line;
    std::getline(lines, line);
    value = line.substr(name.size() + 1);
    while (std::getline(lines, line) && line.starts_with("  ")) {
        value += "\n" + line;
    }
    return value;
}

TEST(McpSession, RunEchoesTheLinesAndShowsThePrompt) {
    Tools tools{{}};
    const ToolResult first = tools.run({"LINE", "0,0", "10,0", ""});
    EXPECT_FALSE(first.isError) << first.text;
    EXPECT_EQ(first.text,
              "Command: LINE\n"
              "Specify first point: 0,0\n"
              "Specify next point or [Undo]: 10,0\n"
              "Specify next point or [Undo]:\n"
              "prompt: Command:");
    // Only what is new since the previous call, and the question now.
    const ToolResult second = tools.run({"LINE", "0,0"});
    EXPECT_EQ(second.text,
              "Command: LINE\n"
              "Specify first point: 0,0\n"
              "prompt: Specify next point or [Undo]:");
    const ToolResult nothing = tools.run({});
    EXPECT_EQ(nothing.text, "prompt: Specify next point or [Undo]:");
}

TEST(McpSession, RunTakesTheDirectivesOfLevel1) {
    Tools tools{{}};
    const ToolResult result = tools.run({"; a comment", "!view 0,0 1 800x600", "LINE", "0,0",
                                         "!move 5,5", "!report", "!esc", "!snap off"});
    ASSERT_FALSE(result.isError) << result.text;
    EXPECT_NE(result.text.find("\n[report 1]\n"), std::string::npos) << result.text;
    // The history first, then the section of !report, then the prompt.
    EXPECT_NE(result.text.find("Specify first point: 0,0\n*Cancel*\n\n[report 1]\n"),
              std::string::npos)
        << result.text;
    EXPECT_TRUE(result.text.ends_with("\nprompt: Command:")) << result.text;
    const std::string report = tools.report();
    EXPECT_EQ(field(report, "pointer"), " 5,5");
    EXPECT_EQ(field(report, "snap"), " off");
    EXPECT_EQ(field(report, "entities"), "");
    // The messages of the final section start after the !report.
    EXPECT_EQ(field(report, "messages"), "\n  *Cancel*");
}

TEST(McpSession, InvalidLinesRunNothing) {
    Tools tools{{}};
    for (const std::vector<std::string>& lines : {std::vector<std::string>{"LINE", "!bogus"},
                                                  {"LINE", "!key F3"},
                                                  {"!level 2", "LINE"},
                                                  {"!screenshot view"},
                                                  {"LINE\n0,0"},
                                                  {"LINE\r"}}) {
        const ToolResult result = tools.run(lines);
        EXPECT_TRUE(result.isError) << lines.front() << " -> " << result.text;
    }
    EXPECT_TRUE(tools.call("run", json{{"lines", "LINE"}}).isError);
    EXPECT_TRUE(tools.call("run", json{{"lines", json::array({1, 2})}}).isError);
    EXPECT_TRUE(tools.call("run", json::object()).isError);
    EXPECT_TRUE(tools.call("run", json{{"lines", json::array()}, {"extra", 1}}).isError);
    EXPECT_TRUE(tools.run(std::vector<std::string>(tg::mcp::kMaxScriptLines + 1, "")).isError);
    // Nothing ran: no echo, no command.
    const std::string report = tools.report();
    EXPECT_EQ(field(report, "messages"), "");
    EXPECT_EQ(field(report, "prompt"), " Command:");
}

TEST(McpSession, ReportAfterEveryStep) {
    Tools tools{{}};
    std::string report = tools.report();
    EXPECT_TRUE(report.starts_with("tecnigrafo-report 1\n\n[final]\nentities:\nmessages:\n"))
        << report;
    EXPECT_EQ(report.find("[report"), std::string::npos);

    ASSERT_FALSE(tools.run({"LINE", "0,0", "10,0"}).isError);
    report = tools.report();
    EXPECT_EQ(field(report, "entities"), "\n  257 LINE start=0,0 end=10,0");
    EXPECT_EQ(field(report, "prompt"), " Specify next point or [Undo]:");
    EXPECT_EQ(field(report, "messages"),
              "\n  Command: LINE\n  Specify first point: 0,0\n  Specify next point or [Undo]: "
              "10,0");

    ASSERT_FALSE(tools.run({""}).isError);
    report = tools.report();
    EXPECT_EQ(field(report, "messages"), "\n  Specify next point or [Undo]:");
    EXPECT_EQ(field(report, "undo"), "\n  Line");

    ASSERT_FALSE(tools.run({"U"}).isError);
    report = tools.report();
    EXPECT_EQ(field(report, "entities"), "");
    EXPECT_EQ(field(report, "redo"), "\n  Line");
    // A report with nothing new in between has no messages.
    EXPECT_EQ(field(tools.report(), "messages"), "");
}

TEST(McpSession, ResetStartsFromAnEmptyDocument) {
    Tools tools{{}};
    ASSERT_FALSE(tools.run({"LINE", "0,0", "10,0", "", "!snap off", "!report"}).isError);
    const ToolResult reset = tools.call("reset");
    EXPECT_FALSE(reset.isError);
    EXPECT_EQ(reset.text, "prompt: Command:");
    const std::string report = tools.report();
    EXPECT_EQ(field(report, "entities"), "");
    EXPECT_EQ(field(report, "messages"), "");
    EXPECT_EQ(field(report, "undo"), "");
    EXPECT_EQ(field(report, "snap"), " on");
    // The sections of !report count again from 1, and the handles too.
    const ToolResult again = tools.run({"LINE", "0,0", "5,0", "", "!report"});
    EXPECT_NE(again.text.find("[report 1]"), std::string::npos) << again.text;
    EXPECT_NE(again.text.find("257 LINE start=0,0 end=5,0"), std::string::npos) << again.text;
}

TEST(McpSession, SaveAndOpenInsideRoot) {
    const fs::path root = emptyRoot("save-open");
    Tools tools{{.root = root, .sources = std::nullopt, .questions = false}};
    ASSERT_FALSE(tools.run({"LINE", "0,0", "10,0", ""}).isError);

    const ToolResult saved = tools.call("save", json{{"path", "disegno.dxf"}});
    ASSERT_FALSE(saved.isError) << saved.text;
    EXPECT_TRUE(saved.text.starts_with("Command: SAVEAS\nSave drawing as <"));
    EXPECT_TRUE(saved.text.ends_with("disegno.dxf\nprompt: Command:")) << saved.text;
    EXPECT_TRUE(fs::is_regular_file(root / "disegno.dxf"));

    // The file exists: without replace the question stays, with it Y.
    const ToolResult asked = tools.call("save", json{{"path", "disegno.dxf"}});
    EXPECT_FALSE(asked.isError);
    EXPECT_TRUE(asked.text.ends_with("Do you want to replace it? [Yes/No] <No>:")) << asked.text;
    EXPECT_TRUE(tools.call("save", json{{"path", "copia.cadx"}}).isError)  // still asking
        << "a command is running";
    ASSERT_FALSE(tools.run({"!esc"}).isError);
    const ToolResult replaced =
        tools.call("save", json{{"path", "disegno.dxf"}, {"replace", true}});
    EXPECT_TRUE(replaced.text.ends_with(" Y\nprompt: Command:")) << replaced.text;

    // A name without extension is a .cadx, as SAVEAS does.
    fs::create_directories(root / "sotto");
    const ToolResult copy = tools.call("save", json{{"path", "sotto/copia"}});
    ASSERT_FALSE(copy.isError) << copy.text;
    EXPECT_TRUE(copy.text.ends_with("sotto/copia\nprompt: Command:")) << copy.text;
    EXPECT_TRUE(fs::is_regular_file(root / "sotto" / "copia.cadx"));

    ASSERT_EQ(tools.call("reset").text, "prompt: Command:");
    const ToolResult opened = tools.call("open", json{{"path", "disegno.dxf"}});
    ASSERT_FALSE(opened.isError) << opened.text;
    EXPECT_EQ(opened.text,
              "Command: OPEN\nEnter name of drawing to open: disegno.dxf\nprompt: Command:");
    EXPECT_EQ(field(tools.report(), "entities"), "\n  257 LINE start=0,0 end=10,0");

    ASSERT_FALSE(tools.call("open", json{{"path", "sotto/copia.cadx"}}).isError);
    EXPECT_EQ(field(tools.report(), "entities"), "\n  257 LINE start=0,0 end=10,0");

    // A modified drawing: OPEN asks first whether to save the changes.
    ASSERT_FALSE(tools.run({"LINE", "0,0", "0,10", ""}).isError);
    const ToolResult changes = tools.call("open", json{{"path", "disegno.dxf"}});
    EXPECT_FALSE(changes.isError);
    EXPECT_TRUE(changes.text.starts_with("Command: OPEN\nprompt: Save changes to "))
        << changes.text;
}

TEST(McpSession, FileToolsStayInsideRoot) {
    const fs::path root = emptyRoot("outside");
    { const std::ofstream file(root / "vuoto.dxf"); }
    Tools tools{{.root = root, .sources = std::nullopt, .questions = false}};
    for (const std::string path :
         {"../fuori.dxf", "sotto/../../fuori.dxf", "/assoluto.dxf", "C:/assoluto.dxf",
          "C:assoluto.dxf", R"(\\server\f.dxf)", "", "@altro/f.dxf"}) {
        const ToolResult open = tools.call("open", json{{"path", path}});
        EXPECT_TRUE(open.isError) << path;
        EXPECT_EQ(open.text, "Invalid file name.") << path;
        const ToolResult save = tools.call("save", json{{"path", path}});
        EXPECT_TRUE(save.isError) << path;
        EXPECT_EQ(save.text, "Invalid file name.") << path;
        const ToolResult check = tools.call("check_case", json{{"path", path}});
        EXPECT_TRUE(check.isError) << path;
        EXPECT_EQ(check.text, "Invalid file name.") << path;
    }
    const ToolResult missing = tools.call("open", json{{"path", "manca.dxf"}});
    EXPECT_TRUE(missing.isError);
    EXPECT_TRUE(missing.text.starts_with("\"manca.dxf\"")) << missing.text;
    EXPECT_TRUE(tools.call("open", json{{"path", 3}}).isError);
    EXPECT_TRUE(tools.call("save", json{{"path", "a.dxf"}, {"replace", "yes"}}).isError);
    EXPECT_FALSE(fs::exists(root.parent_path() / "fuori.dxf"));
    // Nothing ran.
    EXPECT_EQ(field(tools.report(), "messages"), "");
}

TEST(McpSession, WithoutRootNoFileAccess) {
    Tools tools{{}};
    for (const std::string_view tool : {"open", "save", "check_case"}) {
        const ToolResult result = tools.call(tool, json{{"path", "disegno.dxf"}});
        EXPECT_TRUE(result.isError) << tool;
        EXPECT_EQ(result.text, "File access is not available.") << tool;
    }
    // The commands themselves say the same in the history.
    const ToolResult typed = tools.run({"SAVEAS"});
    EXPECT_EQ(typed.text, "Command: SAVEAS\nFile access is not available.\nprompt: Command:");
}

TEST(McpSession, RunScriptIsApartFromTheLiveSession) {
    Tools tools{{}};
    ASSERT_FALSE(tools.run({"LINE", "0,0"}).isError);
    const ToolResult result = tools.call("run_script", json{{"script", "LINE\n0,0\n10,0\n\n"}});
    ASSERT_FALSE(result.isError) << result.text;
    EXPECT_TRUE(
        result.text.starts_with("tecnigrafo-report 1\n\n[final]\nentities:\n  257 LINE "
                                "start=0,0 end=10,0\n"))
        << result.text;
    // The live session did not move.
    EXPECT_EQ(tools.run({}).text, "prompt: Specify next point or [Undo]:");

    EXPECT_EQ(tools.call("run_script", json{{"script", "!bogus\n"}}).isError, true);
    const ToolResult level2 = tools.call("run_script", json{{"script", "!level 2\nLINE\n"}});
    EXPECT_TRUE(level2.isError);
    EXPECT_EQ(level2.text, "needs level 2: run it with tecnigrafo --script");
    std::string tooLong;
    for (std::size_t i = 0; i <= tg::mcp::kMaxScriptLines; ++i) {
        tooLong += "\n";
    }
    EXPECT_TRUE(tools.call("run_script", json{{"script", tooLong}}).isError);
    EXPECT_TRUE(tools.call("run_script", json::object()).isError);
}

TEST(McpSession, CheckCaseGivesTheOutcome) {
    const fs::path root = emptyRoot("check");
    fs::create_directories(root / "area" / "buono");
    fs::create_directories(root / "area" / "vuoto");
    {
        std::ofstream(root / "area" / "buono" / "script.tgs") << "LINE\n0,0\n10,0\n\n";
        std::ofstream(root / "area" / "buono" / "atteso.txt") << "tecnigrafo-report 1\n";
    }
    Tools tools{{.root = root, .sources = std::nullopt, .questions = false}};
    const ToolResult notPassed = tools.call("check_case", json{{"path", "area/buono"}});
    ASSERT_FALSE(notPassed.isError) << notPassed.text;
    ASSERT_TRUE(notPassed.structured);
    EXPECT_EQ(structured(notPassed).at("outcome"), "not passed");
    const std::string output = structured(notPassed).at("output").get<std::string>();
    EXPECT_TRUE(output.starts_with("FAILED: area/buono: ")) << output;

    const ToolResult invalid = tools.call("check_case", json{{"path", "area/vuoto"}});
    ASSERT_TRUE(invalid.structured);
    EXPECT_EQ(structured(invalid).at("outcome"), "invalid");
    EXPECT_EQ(structured(tools.call("check_case", json{{"path", "area/manca"}})).at("outcome"),
              "invalid");
    // Nothing is written inside --root.
    EXPECT_FALSE(fs::exists(root / "area" / "buono" / "ottenuto.txt"));
    EXPECT_FALSE(fs::exists(root / "ottenuto.txt"));
}

TEST(McpSession, AskOnlyWithQuestions) {
    const auto names = [](const SessionOptions& options) {
        std::vector<std::string> list;
        for (const std::unique_ptr<Tool>& tool : tg::mcp::caseTools(options)) {
            list.push_back(tool->name());
        }
        return list;
    };
    EXPECT_EQ(names({}), (std::vector<std::string>{"run_script", "check_case"}));
    EXPECT_EQ(names({.root = std::nullopt, .sources = std::nullopt, .questions = true}),
              (std::vector<std::string>{"run_script", "check_case", "ask"}));

    // Through the server: without --questions, ask is not listed and a call
    // is answered as an unknown tool.
    const auto exchange = [](const SessionOptions& options) {
        std::vector<std::unique_ptr<Tool>> tools =
            tg::mcp::sessionTools(std::make_shared<Session>(options));
        for (std::unique_ptr<Tool>& tool : tg::mcp::caseTools(options)) {
            tools.push_back(std::move(tool));
        }
        tg::mcp::Server server(std::move(tools));
        const json init{{"jsonrpc", "2.0"},
                        {"id", 1},
                        {"method", "initialize"},
                        {"params",
                         {{"protocolVersion", "2025-11-25"},
                          {"capabilities", json::object()},
                          {"clientInfo", {{"name", "test"}, {"version", "1"}}}}}};
        const json list{{"jsonrpc", "2.0"}, {"id", 2}, {"method", "tools/list"}};
        const json call{
            {"jsonrpc", "2.0"},
            {"id", 3},
            {"method", "tools/call"},
            {"params", {{"name", "ask"}, {"arguments", {{"questions", "; nothing\n"}}}}}};
        std::istringstream in(init.dump() + "\n" + list.dump() + "\n" + call.dump() + "\n");
        std::ostringstream out;
        std::ostringstream log;
        EXPECT_EQ(server.run(in, out, log), 0);
        std::vector<json> answers;
        std::istringstream lines(out.str());
        for (std::string line; std::getline(lines, line);) {
            answers.push_back(json::parse(line));
        }
        return answers;
    };

    const std::vector<json> without = exchange({});
    ASSERT_EQ(without.size(), 3U);
    std::vector<std::string> listed;
    for (const json& tool : without[1].at("result").at("tools")) {
        listed.push_back(tool.at("name").get<std::string>());
        // Every tool says its types and its required fields.
        const json& schema = tool.at("inputSchema");
        EXPECT_EQ(schema.at("type"), "object");
        EXPECT_TRUE(schema.at("properties").is_object());
    }
    EXPECT_EQ(listed, (std::vector<std::string>{"run", "report", "reset", "open", "save",
                                                "run_script", "check_case"}));
    EXPECT_EQ(without[2].at("error").at("code"), tg::mcp::kInvalidParams);
    EXPECT_EQ(without[2].at("error").at("message"), "Unknown tool: ask");

    const std::vector<json> with =
        exchange({.root = std::nullopt, .sources = std::nullopt, .questions = true});
    ASSERT_EQ(with.size(), 3U);
    EXPECT_EQ(with[1].at("result").at("tools").size(), 8U);
    const json& result = with[2].at("result");
    EXPECT_FALSE(result.at("isError").get<bool>()) << result.dump();
    EXPECT_EQ(result.at("structuredContent").at("outcome"), "passed");
}

TEST(McpSession, AskAnswersTheQuestions) {
    Tools tools{{.root = std::nullopt, .sources = std::nullopt, .questions = true}};
    const std::string questions =
        "C1 = CIRCLE center=0,0 radius=10\n"
        "? bbox C1 -> -10,-10 10,10\n"
        "? bbox C1 -> 0,0 1,1\n";
    const ToolResult answered = tools.call("ask", json{{"questions", questions}});
    ASSERT_FALSE(answered.isError) << answered.text;
    ASSERT_TRUE(answered.structured);
    EXPECT_EQ(structured(answered).at("outcome"), "not passed");
    EXPECT_EQ(structured(answered).at("answers"),
              "C1 = CIRCLE center=0,0 radius=10\n"
              "? bbox C1 -> -10,-10 10,10\n"
              "? bbox C1 -> -10,-10 10,10\n");
    EXPECT_EQ(
        structured(answered).at("differences"),
        json::array({json{{"line", 3}, {"expected", "0,0 1,1"}, {"obtained", "-10,-10 10,10"}}}));

    const ToolResult wrong = tools.call("ask", json{{"questions", "!bogus\n"}});
    EXPECT_TRUE(wrong.isError) << wrong.text;
    EXPECT_TRUE(tools.call("ask", json::object()).isError);
}

}  // namespace
