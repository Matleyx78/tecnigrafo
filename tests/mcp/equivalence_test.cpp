// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// tecnigrafo-mcp against tecnigrafo-script on every verification case
// (0.6-P26, docs/adr/0027 point 6): run_script gives the same report as
// `tecnigrafo-script run`, check_case the same outcome as `tecnigrafo-script
// check`, with the same --sources. The outcome itself does not matter: a
// case listed in in-attesa.txt is compared all the same.

#include "nlohmann_json/json.hpp"
#include "tg/mcp/session.hpp"
#include "tg/mcp/tool.hpp"
#include "tg/script/case_check.hpp"
#include "tg/script/case_files.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

namespace fs = std::filesystem;
using nlohmann::json;
using tg::mcp::SessionOptions;
using tg::mcp::ToolResult;

const fs::path kCases{TG_VERIFICA_CASES};
const fs::path kSources{TG_VERIFICA_SOURCES_DIR};
const fs::path kOutput{TG_EQUIVALENCE_OUTPUT};

/// Every case of tests/verifica/casi, "<area>/<name>", in order; with
/// `scriptsOnly`, only those with a script.tgs.
std::vector<std::string> cases(bool scriptsOnly) {
    std::vector<std::string> names;
    std::error_code error;
    for (const fs::directory_entry& area : fs::directory_iterator(kCases, error)) {
        if (!area.is_directory()) {
            continue;
        }
        for (const fs::directory_entry& folder : fs::directory_iterator(area.path(), error)) {
            const bool script = fs::exists(folder.path() / "script.tgs");
            if (script || (!scriptsOnly && fs::exists(folder.path() / "domande.tgq"))) {
                names.push_back(area.path().filename().generic_string() + "/" +
                                folder.path().filename().generic_string());
            }
        }
    }
    std::ranges::sort(names);
    return names;
}

std::string readFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{}};
}

/// The tool `name` of tecnigrafo-mcp with these options.
ToolResult callTool(const SessionOptions& options, std::string_view name, const json& arguments) {
    for (const std::unique_ptr<tg::mcp::Tool>& tool : tg::mcp::caseTools(options)) {
        if (tool->name() == name) {
            return tool->call(arguments);
        }
    }
    ADD_FAILURE() << "no tool " << name;
    return ToolResult::error("no tool");
}

/// Restores the current folder at the end of a scope.
class CurrentFolder {
public:
    explicit CurrentFolder(const fs::path& folder) : m_previous(fs::current_path()) {
        fs::current_path(folder);
    }

    CurrentFolder(const CurrentFolder&) = delete;
    CurrentFolder& operator=(const CurrentFolder&) = delete;
    CurrentFolder(CurrentFolder&&) = delete;
    CurrentFolder& operator=(CurrentFolder&&) = delete;

    ~CurrentFolder() { fs::current_path(m_previous); }

private:
    fs::path m_previous;
};

std::string_view outcomeName(int exitCode) {
    switch (exitCode) {
        case tg::script::kExitPassed:
            return "passed";
        case tg::script::kExitFailed:
            return "not passed";
        case tg::script::kExitNeedsLevel2:
            return "needs level 2";
        default:
            return "invalid";
    }
}

class CaseTest : public testing::TestWithParam<std::string> {
protected:
    [[nodiscard]] static fs::path caseFolder() { return kCases / GetParam(); }

    [[nodiscard]] static fs::path output(std::string_view kind) {
        return kOutput / GetParam() / kind;
    }
};

/// The cases with a script.tgs: a file of questions has no report.
class RunScript : public CaseTest {};

/// Every case.
class CheckCase : public CaseTest {};

TEST_P(RunScript, GivesTheReportOfTecnigrafoScriptRun) {
    const fs::path script = caseFolder() / "script.tgs";
    // Each side works on its own fresh copy of the files of the case:
    // tecnigrafo-script in the current folder, run_script in --root.
    const fs::path scriptFiles = output("script-run");
    const fs::path mcpFiles = output("mcp-run");
    ASSERT_EQ(tg::script::prepareCaseFiles(caseFolder(), scriptFiles), std::nullopt);
    ASSERT_EQ(tg::script::prepareCaseFiles(caseFolder(), mcpFiles), std::nullopt);

    std::ostringstream out;
    std::ostringstream err;
    int exitCode = 0;
    {
        const CurrentFolder inside{scriptFiles};
        exitCode = tg::script::runScriptFile(script, std::nullopt, out, err, kSources);
    }
    const ToolResult result = callTool({.root = mcpFiles, .sources = kSources, .questions = false},
                                       "run_script", json{{"script", readFile(script)}});

    switch (exitCode) {
        case tg::script::kExitPassed:
            ASSERT_FALSE(result.isError) << result.text;
            EXPECT_EQ(result.text, out.str());
            break;
        case tg::script::kExitInvalid:
            ASSERT_TRUE(result.isError);
            EXPECT_EQ(err.str(), script.generic_string() + ", " + result.text + "\n");
            break;
        case tg::script::kExitNeedsLevel2:
            ASSERT_TRUE(result.isError);
            EXPECT_EQ(err.str(), script.generic_string() + " " + result.text + "\n");
            break;
        default:
            FAIL() << "tecnigrafo-script run exited with " << exitCode << ": " << err.str();
    }
}

TEST_P(CheckCase, GivesTheOutcomeOfTecnigrafoScriptCheck) {
    std::ostringstream out;
    const int exitCode = tg::script::checkCase(
        caseFolder(), {.pending = false, .output = output("script-check")}, out, kSources);
    const ToolResult result = callTool({.root = kCases, .sources = kSources, .questions = false},
                                       "check_case", json{{"path", GetParam()}});
    ASSERT_FALSE(result.isError) << result.text;
    const json structured = result.structured.value_or(json::object());
    EXPECT_EQ(structured.value("outcome", ""), outcomeName(exitCode));
    // The same output, with the case named as check_case was given it.
    std::string expected = out.str();
    const std::string where = caseFolder().generic_string();
    if (const std::size_t at = expected.find(where); at != std::string::npos) {
        expected.replace(at, where.size(), GetParam());
    }
    EXPECT_EQ(structured.value("output", ""), expected);
}

/// "pline/coordinate" as a name of a test: "pline_coordinate".
std::string testName(const testing::TestParamInfo<std::string>& info) {
    std::string name = info.param;
    std::ranges::replace_if(
        name, [](char c) { return std::isalnum(static_cast<unsigned char>(c)) == 0; }, '_');
    return name;
}

INSTANTIATE_TEST_SUITE_P(Casi, RunScript, testing::ValuesIn(cases(true)), testName);
INSTANTIATE_TEST_SUITE_P(Casi, CheckCase, testing::ValuesIn(cases(false)), testName);

}  // namespace
