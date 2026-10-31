// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/case_check.hpp"

#include "tg/script/case_files.hpp"
#include "tg/script/case_form.hpp"
#include "tg/script/compare.hpp"
#include "tg/script/level1.hpp"
#include "tg/script/questions.hpp"
#include "tg/script/script.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
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
constexpr std::string_view kPendingList = "tests/verifica/in-attesa/";

// Differences printed for one case; the rest are only counted, so that a
// shifted report does not flood the log. The obtained file has them all.
constexpr std::size_t kMaxPrintedDifferences = 20;

std::optional<std::string> readFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    // Read in blocks so empty files and embedded null bytes are preserved.
    constexpr std::size_t kReadBlockSize = 8192;
    std::array<char, kReadBlockSize> buffer{};
    std::string bytes;
    while (file.read(buffer.data(), static_cast<std::streamsize>(buffer.size())) ||
           file.gcount() != 0) {
        bytes.append(buffer.data(), static_cast<std::size_t>(file.gcount()));
    }
    if (file.bad() || !file.eof()) {
        return std::nullopt;
    }
    return bytes;
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

/// Failed and, for a pending case, compared up to its frontier.
/// FailedBeforeFrontier: a pending case that fails before its frontier, or
/// has none (docs/adr/0037, point 6), an error even when pending.
enum class Verdict : std::uint8_t { Passed, Failed, FailedBeforeFrontier, Invalid, NeedsLevel2 };

struct Outcome {
    Verdict verdict{Verdict::Passed};
    std::string message;  ///< why a case is invalid or not implemented
    std::vector<Difference> differences;
    /// For a pending case the same as expected up to its frontier: where it
    /// is ("script.tgs, line 12: unknown command PLINE").
    std::string frontier;
};

Outcome invalid(std::string message) {
    return {Verdict::Invalid, std::move(message), {}, {}};
}

/// A parse error: a malformed file is invalid, a file that uses what the
/// program does not have yet fails like any case not passed.
Outcome fromParseError(const fs::path& file, const ParseError& error) {
    const std::string where = file.filename().generic_string() + ", " + describe(error);
    if (error.notImplemented) {
        return {Verdict::Failed, "not implemented yet: " + where, {}, {}};
    }
    return invalid(where);
}

/// A violation of the form of a case: never a valid case, not even a
/// pending one (docs/adr/0037, point 2).
Outcome fromFormError(const FormError& error) {
    return invalid(error.file + ", line " + std::to_string(error.line) + ": " + error.message);
}

/// The frontier of a pending script that does not pass, and the comparison
/// of what comes before it (docs/adr/0037, points 4 and 6). `outcome` is the
/// result of the whole run, kept when the case is right up to its frontier
/// and when the script is of level 2.
Outcome checkBeforeFrontier(const fs::path& folder, std::string_view text,
                            std::string_view expected, const CheckOptions& options,
                            const std::optional<fs::path>& sources, Outcome outcome) {
    const fs::path& output = options.output;
    const fs::path scriptPath = folder / kScriptFile;
    std::variant<ScriptPrefix, ParseError> parsed = parseScriptPrefix(text);
    if (const auto* error = std::get_if<ParseError>(&parsed)) {
        return fromParseError(scriptPath, *error);
    }
    const ScriptPrefix& prefix = std::get<ScriptPrefix>(parsed);
    if (prefix.script.level == 2) {
        return outcome;
    }
    const fs::path files = output / fs::path{std::string{kCaseFilesFolder}};
    if (std::optional<std::string> failure = prepareCaseFiles(folder, files)) {
        return invalid(std::move(*failure));
    }
    CaseFiles access{files, sources};
    const PrefixRun run = runLevel1UntilUnknownCommand(prefix.script, {&access});
    if (!writeFile(output / kObtainedFrontierReport, run.report)) {
        return invalid("cannot write " + (output / kObtainedFrontierReport).generic_string());
    }
    // The unknown command is above the frontier of the file, if any: the run
    // only has the actions above it.
    const std::optional<Frontier>& frontier = run.frontier ? run.frontier : prefix.frontier;
    if (!frontier) {
        return {Verdict::FailedBeforeFrontier,
                "fails, but uses nothing the program does not have yet: fix the reference, "
                "or write \"; cambia con #<issue>\" before the line where the behaviour "
                "changes (docs/adr/0037)",
                std::move(outcome.differences),
                {}};
    }
    const std::string where = scriptPath.filename().generic_string() + ", line " +
                              std::to_string(frontier->line) + ": " + frontier->reason;
    std::vector<Difference> differences =
        compareReportPrefix(expected, run.report, prefix.script.tolerances);
    if (!differences.empty()) {
        return {Verdict::FailedBeforeFrontier,
                "differs before the point the program cannot do yet (" + where + ")",
                std::move(differences),
                {}};
    }
    outcome.frontier = where;
    return outcome;
}

Outcome checkScript(const fs::path& folder, const CheckOptions& options,
                    const std::optional<fs::path>& sources) {
    const fs::path& output = options.output;
    const fs::path scriptPath = folder / kScriptFile;
    const std::optional<std::string> text = readFile(scriptPath);
    const std::optional<std::string> expected = readFile(folder / kExpectedFile);
    if (!text) {
        return invalid(std::string{"cannot read "} + kScriptFile);
    }
    // Before running anything; a missing atteso.txt is reported below.
    if (options.earlyChecks && expected) {
        if (const std::optional<FormError> error = checkScriptForm(*text, *expected)) {
            return fromFormError(*error);
        }
    }
    Outcome outcome;
    std::variant<Script, ParseError> parsed = parseScript(*text);
    if (const auto* error = std::get_if<ParseError>(&parsed)) {
        outcome = fromParseError(scriptPath, *error);
    } else {
        const Script& script = std::get<Script>(parsed);
        if (script.level == 2) {
            return {Verdict::NeedsLevel2, {}, {}, {}};
        }
        if (!expected) {
            return invalid(std::string{"cannot read "} + kExpectedFile);
        }
        // The script works on copies of the files of the case, in a folder
        // of its own that each run starts again.
        const fs::path files = output / fs::path{std::string{kCaseFilesFolder}};
        if (std::optional<std::string> failure = prepareCaseFiles(folder, files)) {
            return invalid(std::move(*failure));
        }
        CaseFiles access{files, sources};
        const std::string report = runLevel1(script, {&access});
        if (!writeFile(output / kObtainedReport, report)) {
            return invalid("cannot write " + (output / kObtainedReport).generic_string());
        }
        outcome.differences = compareReports(*expected, report, script.tolerances);
        outcome.verdict = outcome.differences.empty() ? Verdict::Passed : Verdict::Failed;
    }
    if (outcome.verdict == Verdict::Failed && options.pending && options.earlyChecks && expected) {
        return checkBeforeFrontier(folder, *text, *expected, options, sources, std::move(outcome));
    }
    return outcome;
}

Outcome checkQuestionFile(const fs::path& folder, const CheckOptions& options,
                          const std::optional<fs::path>& sources) {
    const fs::path& output = options.output;
    const fs::path path = folder / kQuestionsFile;
    const std::optional<std::string> text = readFile(path);
    if (!text) {
        return invalid(std::string{"cannot read "} + kQuestionsFile);
    }
    if (options.earlyChecks) {
        if (const std::optional<FormError> error = checkQuestionsForm(*text)) {
            return fromFormError(*error);
        }
    }
    // The questions only read: the hand-written files of the case directly.
    CaseFiles access{folder, sources};
    Outcome outcome;
    std::variant<std::vector<Answer>, ParseError> answers = answerQuestions(*text, &access);
    if (const auto* error = std::get_if<ParseError>(&answers)) {
        outcome = fromParseError(path, *error);
    } else {
        const std::vector<Answer>& list = std::get<std::vector<Answer>>(answers);
        if (!writeFile(output / kObtainedAnswers, withActualAnswers(*text, list))) {
            return invalid("cannot write " + (output / kObtainedAnswers).generic_string());
        }
        for (const Answer& answer : list) {
            if (!answer.matches) {
                outcome.differences.push_back({answer.line, answer.expected, answer.actual});
            }
        }
        outcome.verdict = outcome.differences.empty() ? Verdict::Passed : Verdict::Failed;
    }
    if (outcome.verdict == Verdict::Passed || !options.pending || !options.earlyChecks) {
        return outcome;
    }
    // A full read may fail below the frontier; the prefix deliberately
    // does not read those lines, including malformed ones (ADR 0037.4).
    auto parsed = answerQuestionsUntilFrontier(*text, &access);
    if (const auto* error = std::get_if<ParseError>(&parsed)) {
        return fromParseError(path, *error);
    }
    const QuestionPrefix& prefix = std::get<QuestionPrefix>(parsed);
    std::vector<Difference> differences;
    bool skipped = false;
    for (const Answer& answer : prefix.answers) {
        skipped = skipped || answer.skipped;
        if (!answer.skipped && !answer.matches) {
            differences.push_back({answer.line, answer.expected, answer.actual});
        }
    }
    if (!prefix.frontier && !skipped) {
        return {Verdict::FailedBeforeFrontier,
                "fails, but uses nothing the program does not have yet: fix the reference, "
                "or write \"; cambia con #<issue>\" before the line where the behaviour "
                "changes (docs/adr/0037)",
                std::move(differences),
                {}};
    }
    const std::string where = prefix.frontier
                                  ? "domande.tgq, line " + std::to_string(prefix.frontier->line) +
                                        ": " + prefix.frontier->reason
                                  : "domande.tgq: questions not implemented yet are skipped";
    if (!differences.empty()) {
        return {Verdict::FailedBeforeFrontier,
                "differs before the point the program cannot do yet (" + where + ")",
                std::move(differences),
                {}};
    }
    if (outcome.verdict == Verdict::Invalid) {
        outcome.message = "not implemented yet: " + where;
    }
    outcome.verdict = Verdict::Failed;
    outcome.frontier = where;
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
                out << "FAILED: " << name
                    << " passes now: remove its line from the file of its block in " << kPendingList
                    << " (delete the file when it lists no more case)\n";
                return kExitFailed;
            }
            out << "passed: " << name << "\n";
            return kExitPassed;
        case Verdict::FailedBeforeFrontier:
            out << "FAILED: " << name << ": " << outcome.message << "\n";
            printDifferences(outcome.differences, out);
            return kExitFailed;
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
        out << "warning: not passed yet, as expected while it is listed in " << kPendingList;
        if (!outcome.frontier.empty()) {
            out << "; as expected up to " << outcome.frontier;
        }
        out << "\n";
        return kExitPassed;
    }
    return kExitFailed;
}

}  // namespace

int runScriptFile(const fs::path& script, const std::optional<fs::path>& report, std::ostream& out,
                  std::ostream& err, const std::optional<fs::path>& sources) {
    const std::optional<std::string> text = readFile(script);
    if (!text) {
        err << "cannot read " << script.generic_string() << "\n";
        return kExitInvalid;
    }
    std::variant<Script, ParseError> parsed = parseScript(*text, script.filename().string());
    if (const auto* error = std::get_if<ParseError>(&parsed)) {
        err << script.generic_string() << ", " << describe(*error) << "\n";
        return kExitInvalid;
    }
    const Script& parsedScript = std::get<Script>(parsed);
    if (parsedScript.level == 2) {
        err << script.generic_string() << " needs level 2: run it with tecnigrafo --script\n";
        return kExitNeedsLevel2;
    }
    CaseFiles access{fs::path{"."}, sources};
    const std::string reportText = runLevel1(parsedScript, {&access});
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

int checkCase(const fs::path& folder, const CheckOptions& options, std::ostream& out,
              const std::optional<fs::path>& sources) {
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
        outcome = checkScript(folder, options, sources);
    } else {
        outcome = checkQuestionFile(folder, options, sources);
    }
    return conclude(outcome, folder, options.pending, out);
}

}  // namespace tg::script
