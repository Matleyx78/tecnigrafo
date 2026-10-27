// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// run_script, check_case and ask: what tecnigrafo-script does, as tools of
// the MCP server, apart from the live session (docs/adr/0027, points 5-6).

#include "nlohmann_json/json.hpp"
#include "tg/io/file_access.hpp"
#include "tg/mcp/session.hpp"
#include "tg/mcp/tool.hpp"
#include "tg/script/case_check.hpp"
#include "tg/script/case_files.hpp"
#include "tg/script/level1.hpp"
#include "tg/script/questions.hpp"
#include "tg/script/script.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <iomanip>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace tg::mcp {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

/// An error for the first argument not in `allowed`, or std::nullopt.
std::optional<ToolResult> unknownArgument(const std::string& tool, const json& arguments,
                                          std::initializer_list<std::string_view> allowed) {
    for (const auto& [key, value] : arguments.items()) {
        if (std::ranges::find(allowed, std::string_view{key}) == allowed.end()) {
            std::string message = "Unknown argument \"";
            message += key;
            message += "\" of ";
            message += tool;
            message += '.';
            return ToolResult::error(std::move(message));
        }
    }
    return std::nullopt;
}

/// The string argument `name`, or std::nullopt if it is missing or not a string.
std::optional<std::string> stringArgument(const json& arguments, std::string_view name) {
    const auto value = arguments.find(name);
    if (value == arguments.end() || !value->is_string()) {
        return std::nullopt;
    }
    return value->get<std::string>();
}

json oneStringArgument(std::string_view name, std::string_view description) {
    return json{
        {"type", "object"},
        {"properties", json{{name, json{{"type", "string"}, {"description", description}}}}},
        {"required", json::array({name})},
        {"additionalProperties", false}};
}

/// The lines of a file, counted as parseScript() reads them.
std::size_t lineCount(std::string_view text) {
    const auto breaks = static_cast<std::size_t>(std::ranges::count(text, '\n'));
    return text.empty() || text.back() == '\n' ? breaks : breaks + 1;
}

std::optional<ToolResult> tooLong(std::string_view text) {
    if (lineCount(text) > kMaxScriptLines) {
        return ToolResult::error("Too many lines: at most " + std::to_string(kMaxScriptLines) +
                                 " in one request.");
    }
    return std::nullopt;
}

std::string describe(const script::ParseError& error) {
    return "line " + std::to_string(error.line) + ": " + error.message;
}

/// UTF-8 text as a path, on every system.
fs::path toPath(std::string_view text) {
    return {std::u8string{text.begin(), text.end()}};
}

/// The files of a tool call: inside --root, or none without it.
std::unique_ptr<script::CaseFiles> rootFiles(const SessionOptions& options) {
    if (!options.root) {
        return nullptr;
    }
    return std::make_unique<script::CaseFiles>(*options.root, options.sources);
}

/// A new empty folder in the temporary folder of the system, removed with
/// this object: where check_case lets tecnigrafo-script write what it
/// obtains, never inside --root.
class TemporaryFolder {
public:
    TemporaryFolder() {
        std::random_device random;
        std::error_code error;
        const fs::path base = fs::temp_directory_path(error);
        // A few tries: a name already taken is only possible by chance.
        constexpr int kTries = 8;
        for (int i = 0; i < kTries && !error && m_path.empty(); ++i) {
            const std::uint64_t value = (static_cast<std::uint64_t>(random()) << 32U) | random();
            std::ostringstream name;
            name << "tecnigrafo-mcp-" << std::hex << std::setw(16) << std::setfill('0') << value;
            if (fs::create_directory(base / name.str(), error) && !error) {
                m_path = base / name.str();
            }
        }
    }

    TemporaryFolder(const TemporaryFolder&) = delete;
    TemporaryFolder& operator=(const TemporaryFolder&) = delete;
    TemporaryFolder(TemporaryFolder&&) = delete;
    TemporaryFolder& operator=(TemporaryFolder&&) = delete;

    ~TemporaryFolder() {
        if (!m_path.empty()) {
            std::error_code error;
            fs::remove_all(m_path, error);
        }
    }

    /// Empty if the folder could not be made.
    [[nodiscard]] const fs::path& path() const { return m_path; }

private:
    fs::path m_path;
};

std::string_view outcomeName(int exitCode) {
    switch (exitCode) {
        case script::kExitPassed:
            return "passed";
        case script::kExitFailed:
            return "not passed";
        case script::kExitNeedsLevel2:
            return "needs level 2";
        default:
            return "invalid";
    }
}

class RunScript final : public Tool {
public:
    explicit RunScript(SessionOptions options) : m_options(std::move(options)) {}

    [[nodiscard]] std::string name() const override { return "run_script"; }

    [[nodiscard]] std::string description() const override {
        return "Runs a whole .tgs script at level 1 on a new empty document, apart from the live "
               "session, and returns the complete report: the same text as tecnigrafo-script "
               "run. The script reads and writes its files inside --root, and the public files "
               "as @sources/<name>.";
    }

    [[nodiscard]] json inputSchema() const override {
        return oneStringArgument("script", "The text of the .tgs script.");
    }

    [[nodiscard]] ToolResult call(const json& arguments) override {
        if (auto error = unknownArgument(name(), arguments, {"script"})) {
            return std::move(*error);
        }
        const std::optional<std::string> text = stringArgument(arguments, "script");
        if (!text) {
            return ToolResult::error("run_script needs \"script\", the text of a .tgs script.");
        }
        if (auto error = tooLong(*text)) {
            return std::move(*error);
        }
        // As script::runScriptFile(), with --root in place of the current
        // folder, and the same messages without the name of a file.
        std::variant<script::Script, script::ParseError> parsed = script::parseScript(*text);
        if (const auto* error = std::get_if<script::ParseError>(&parsed)) {
            return ToolResult::error(describe(*error));
        }
        const script::Script& script = std::get<script::Script>(parsed);
        if (script.level == 2) {
            return ToolResult::error("needs level 2: run it with tecnigrafo --script");
        }
        const std::unique_ptr<script::CaseFiles> files = rootFiles(m_options);
        return ToolResult::success(script::runLevel1(script, {files.get()}));
    }

private:
    SessionOptions m_options;
};

class CheckCase final : public Tool {
public:
    explicit CheckCase(SessionOptions options) : m_options(std::move(options)) {}

    [[nodiscard]] std::string name() const override { return "check_case"; }

    [[nodiscard]] std::string description() const override {
        return "Checks a verification case (a folder with script.tgs and atteso.txt, or "
               "domande.tgq) inside --root, as tecnigrafo-script check does, apart from the live "
               "session. Returns the outcome (passed, not passed, invalid, needs level 2) and "
               "the output of tecnigrafo-script check with the differences.";
    }

    [[nodiscard]] json inputSchema() const override {
        return oneStringArgument(
            "path",
            "The case folder, relative to --root (\"pline/coordinate\" with --root "
            "tests/verifica/casi): no \"..\", no absolute path.");
    }

    [[nodiscard]] ToolResult call(const json& arguments) override {
        if (auto error = unknownArgument(name(), arguments, {"path"})) {
            return std::move(*error);
        }
        const std::optional<std::string> path = stringArgument(arguments, "path");
        if (!path) {
            return ToolResult::error(
                "check_case needs \"path\", a case folder relative to --root.");
        }
        if (!m_options.root) {
            return ToolResult::error(std::string{kNoFileAccess});
        }
        if (std::optional<std::string> error = checkRootName(m_options, *path, false)) {
            return ToolResult::error(std::move(*error));
        }
        const fs::path root = *m_options.root;
        const TemporaryFolder output;
        if (output.path().empty()) {
            return ToolResult::error("Cannot make a temporary folder for the obtained report.");
        }
        const fs::path folder = root / toPath(*path);
        std::ostringstream out;
        const int exitCode = script::checkCase(folder, {.pending = false, .output = output.path()},
                                               out, m_options.sources);
        // The output names the case as the tool was given it, not where
        // --root is on this computer.
        std::string text = out.str();
        const std::string where = folder.generic_string();
        if (const std::size_t at = text.find(where); at != std::string::npos) {
            text.replace(at, where.size(), *path);
        }
        return ToolResult::object(json{{"outcome", outcomeName(exitCode)}, {"output", text}});
    }

private:
    SessionOptions m_options;
};

class Ask final : public Tool {
public:
    explicit Ask(SessionOptions options) : m_options(std::move(options)) {}

    [[nodiscard]] std::string name() const override { return "ask"; }

    [[nodiscard]] std::string description() const override {
        return "Answers a file of questions to the model (.tgq, docs/verifica/domande.md) as "
               "tecnigrafo-script check does for domande.tgq. Returns the outcome (passed or "
               "not passed), the file with the obtained answers in place of the expected ones, "
               "and every answer that differs. Files are read inside --root.";
    }

    [[nodiscard]] json inputSchema() const override {
        return oneStringArgument("questions", "The text of the .tgq file.");
    }

    [[nodiscard]] ToolResult call(const json& arguments) override {
        if (auto error = unknownArgument(name(), arguments, {"questions"})) {
            return std::move(*error);
        }
        const std::optional<std::string> text = stringArgument(arguments, "questions");
        if (!text) {
            return ToolResult::error("ask needs \"questions\", the text of a .tgq file.");
        }
        if (auto error = tooLong(*text)) {
            return std::move(*error);
        }
        const std::unique_ptr<script::CaseFiles> files = rootFiles(m_options);
        std::variant<std::vector<script::Answer>, script::ParseError> answers =
            script::answerQuestions(*text, files.get());
        if (const auto* error = std::get_if<script::ParseError>(&answers)) {
            // As tecnigrafo-script check: what the program does not have yet
            // is a case not passed, a malformed file is an error.
            if (!error->notImplemented) {
                return ToolResult::error(describe(*error));
            }
            return ToolResult::object(json{{"outcome", "not passed"},
                                           {"message", "not implemented yet: " + describe(*error)},
                                           {"answers", *text},
                                           {"differences", json::array()}});
        }
        const std::vector<script::Answer>& list = std::get<std::vector<script::Answer>>(answers);
        json differences = json::array();
        for (const script::Answer& answer : list) {
            if (!answer.matches) {
                differences.push_back(json{{"line", answer.line},
                                           {"expected", answer.expected},
                                           {"obtained", answer.actual}});
            }
        }
        const bool passed = differences.empty();
        return ToolResult::object(json{{"outcome", passed ? "passed" : "not passed"},
                                       {"answers", script::withActualAnswers(*text, list)},
                                       {"differences", std::move(differences)}});
    }

private:
    SessionOptions m_options;
};

}  // namespace

std::vector<std::unique_ptr<Tool>> caseTools(const SessionOptions& options) {
    std::vector<std::unique_ptr<Tool>> tools;
    tools.push_back(std::make_unique<RunScript>(options));
    tools.push_back(std::make_unique<CheckCase>(options));
    // Without --questions, ask is not listed and a call is an unknown tool
    // (docs/adr/0027, point 5).
    if (options.questions) {
        tools.push_back(std::make_unique<Ask>(options));
    }
    return tools;
}

}  // namespace tg::mcp
