// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/case_check.hpp"

#include "tg/script/compare.hpp"
#include "tg/script/level1.hpp"
#include "tg/script/questions.hpp"
#include "tg/script/script.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

namespace fs = std::filesystem;

constexpr const char* kScriptFile = "script.tgs";
constexpr const char* kExpectedFile = "atteso.txt";
constexpr const char* kQuestionsFile = "domande.tgq";
constexpr std::string_view kPendingList = "tests/verifica/in-attesa.txt";

// Differences printed for one case; the rest are only counted, so that a
// shifted report does not flood the log. The obtained file has them all.
constexpr std::size_t kMaxPrintedDifferences = 20;

std::optional<std::string> readFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{});
}

bool writeFile(const fs::path& path, std::string_view text) {
    std::error_code error;
    fs::create_directories(path.parent_path(), error);
    std::ofstream file(path, std::ios::binary);
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    return static_cast<bool>(file);
}

std::string describe(const ParseError& error) {
    return "line " + std::to_string(error.line) + ": " + error.message;
}

enum class Verdict : std::uint8_t { Passed, Failed, Invalid, NeedsLevel2 };

struct Outcome {
    Verdict verdict{Verdict::Passed};
    std::string message;  ///< why a case is invalid or not implemented
    std::vector<Difference> differences;
};

Outcome invalid(std::string message) {
    return {Verdict::Invalid, std::move(message), {}};
}

/// A parse error: a malformed file is invalid, a file that uses what the
/// program does not have yet fails like any case not passed.
Outcome fromParseError(const fs::path& file, const ParseError& error) {
    const std::string where = file.filename().generic_string() + ", " + describe(error);
    if (error.notImplemented) {
        return {Verdict::Failed, "not implemented yet: " + where, {}};
    }
    return invalid(where);
}

Outcome checkScript(const fs::path& folder, const fs::path& output) {
    const fs::path scriptPath = folder / kScriptFile;
    const std::optional<std::string> text = readFile(scriptPath);
    const std::optional<std::string> expected = readFile(folder / kExpectedFile);
    if (!text) {
        return invalid(std::string{"cannot read "} + kScriptFile);
    }
    std::variant<Script, ParseError> parsed = parseScript(*text);
    if (const auto* error = std::get_if<ParseError>(&parsed)) {
        return fromParseError(scriptPath, *error);
    }
    const Script& script = std::get<Script>(parsed);
    if (script.level == 2) {
        return {Verdict::NeedsLevel2, {}, {}};
    }
    if (!expected) {
        return invalid(std::string{"cannot read "} + kExpectedFile);
    }
    const std::string report = runLevel1(script);
    Outcome outcome;
    if (!writeFile(output / kObtainedReport, report)) {
        return invalid("cannot write " + (output / kObtainedReport).generic_string());
    }
    outcome.differences = compareReports(*expected, report, script.tolerances);
    outcome.verdict = outcome.differences.empty() ? Verdict::Passed : Verdict::Failed;
    return outcome;
}

Outcome checkQuestionFile(const fs::path& folder, const fs::path& output) {
    const fs::path path = folder / kQuestionsFile;
    const std::optional<std::string> text = readFile(path);
    if (!text) {
        return invalid(std::string{"cannot read "} + kQuestionsFile);
    }
    std::variant<std::vector<Answer>, ParseError> answers = answerQuestions(*text);
    if (const auto* error = std::get_if<ParseError>(&answers)) {
        return fromParseError(path, *error);
    }
    const std::vector<Answer>& list = std::get<std::vector<Answer>>(answers);
    if (!writeFile(output / kObtainedAnswers, withActualAnswers(*text, list))) {
        return invalid("cannot write " + (output / kObtainedAnswers).generic_string());
    }
    Outcome outcome;
    for (const Answer& answer : list) {
        if (!answer.matches) {
            outcome.differences.push_back({answer.line, answer.expected, answer.actual});
        }
    }
    outcome.verdict = outcome.differences.empty() ? Verdict::Passed : Verdict::Failed;
    return outcome;
}

void printDifferences(const std::vector<Difference>& differences, std::ostream& out) {
    std::size_t printed = 0;
    for (const Difference& difference : differences) {
        if (printed == kMaxPrintedDifferences) {
            out << "  ... and " << std::to_string(differences.size() - printed)
                << " more differences\n";
            break;
        }
        out << "  line " << std::to_string(difference.line) << "\n"
            << "    expected: " << difference.expected << "\n"
            << "    obtained: " << difference.actual << "\n";
        ++printed;
    }
}

int conclude(const Outcome& outcome, const fs::path& folder, bool pending, std::ostream& out) {
    const std::string name = folder.generic_string();
    switch (outcome.verdict) {
        case Verdict::Invalid:
            out << "INVALID: " << name << ": " << outcome.message << "\n";
            return kExitInvalid;
        case Verdict::NeedsLevel2:
            out << "skipped: " << name << " needs level 2\n";
            return kExitNeedsLevel2;
        case Verdict::Passed:
            if (pending) {
                out << "FAILED: " << name << " passes now: remove it from " << kPendingList << "\n";
                return kExitFailed;
            }
            out << "passed: " << name << "\n";
            return kExitPassed;
        case Verdict::Failed:
            break;
    }
    out << (pending ? "pending: " : "FAILED: ") << name;
    if (outcome.message.empty()) {
        out << ": " << std::to_string(outcome.differences.size()) << " differences\n";
    } else {
        out << ": " << outcome.message << "\n";
    }
    printDifferences(outcome.differences, out);
    if (pending) {
        out << "warning: not passed yet, as expected while it is listed in " << kPendingList
            << "\n";
        return kExitPassed;
    }
    return kExitFailed;
}

}  // namespace

int runScriptFile(const fs::path& script, const std::optional<fs::path>& report, std::ostream& out,
                  std::ostream& err) {
    const std::optional<std::string> text = readFile(script);
    if (!text) {
        err << "cannot read " << script.generic_string() << "\n";
        return kExitInvalid;
    }
    std::variant<Script, ParseError> parsed = parseScript(*text);
    if (const auto* error = std::get_if<ParseError>(&parsed)) {
        err << script.generic_string() << ", " << describe(*error) << "\n";
        return kExitInvalid;
    }
    const Script& parsedScript = std::get<Script>(parsed);
    if (parsedScript.level == 2) {
        err << script.generic_string() << " needs level 2: run it with tecnigrafo --script\n";
        return kExitNeedsLevel2;
    }
    const std::string reportText = runLevel1(parsedScript);
    if (!report) {
        out << reportText;
        return kExitPassed;
    }
    if (!writeFile(*report, reportText)) {
        err << "cannot write " << report->generic_string() << "\n";
        return kExitInvalid;
    }
    return kExitPassed;
}

int checkCase(const fs::path& folder, const CheckOptions& options, std::ostream& out) {
    std::error_code error;
    const bool hasScript = fs::exists(folder / kScriptFile, error);
    const bool hasQuestions = fs::exists(folder / kQuestionsFile, error);
    Outcome outcome;
    if (!fs::is_directory(folder, error)) {
        outcome = invalid("not a folder");
    } else if (hasScript == hasQuestions) {
        outcome = invalid(std::string{"a case has either "} + kScriptFile + " and " +
                          kExpectedFile + ", or " + kQuestionsFile);
    } else if (hasScript) {
        outcome = checkScript(folder, options.output);
    } else {
        outcome = checkQuestionFile(folder, options.output);
    }
    return conclude(outcome, folder, options.pending, out);
}

}  // namespace tg::script
