// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/case_check.hpp"

#include "tg/script/questions.hpp"
#include "tg/script/script.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

namespace fs = std::filesystem;

const fs::path kOutput{TG_SCRIPT_TEST_OUTPUT};

// A PLINE of one segment and its report, written by hand.
constexpr std::string_view kScript = "PLINE\n0,0\n10,0\n\n";
constexpr std::string_view kReport =
    "tecnigrafo-report 1\n"
    "\n"
    "[final]\n"
    "entities:\n"
    "  257 LWPOLYLINE closed=no vertex=0,0 vertex=10,0\n"
    "messages:\n"
    "  Command: PLINE\n"
    "  Specify start point: 0,0\n"
    "  Specify next point or [Arc/Undo]: 10,0\n"
    "  Specify next point or [Arc/Undo]:\n"
    "prompt: Command:\n"
    "preview:\n"
    "selection: none\n"
    "rectangle: none\n"
    "undo:\n"
    "  Polyline\n"
    "redo:\n"
    "snap: on\n"
    "ortho: off\n"
    "pointer: none\n"
    "active snap: none\n"
    "statistics:\n"
    "  LWPOLYLINE 1\n";

std::string readFile(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{}};
}

void writeFile(const fs::path& path, std::string_view text) {
    std::ofstream file(path, std::ios::binary);
    file << text;
}

/// A case folder with these files, empty of anything else.
fs::path makeCase(const std::string& name, const std::map<std::string, std::string>& files) {
    const fs::path folder = kOutput / "cases" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    for (const auto& [file, text] : files) {
        writeFile(folder / file, text);
    }
    return folder;
}

struct Result {
    int code{0};
    std::string output;
};

Result check(const fs::path& folder, bool pending = false) {
    std::ostringstream out;
    const fs::path output = kOutput / "obtained" / folder.filename();
    fs::remove_all(output);
    const int code = checkCase(folder, {pending, output}, out);
    return {code, out.str()};
}

void expectContains(const std::string& text, std::string_view part) {
    EXPECT_NE(text.find(part), std::string::npos) << "missing: " << part << "\nin:\n" << text;
}

TEST(CheckCase, ScriptThatPasses) {
    const fs::path folder = makeCase(
        "passes", {{"script.tgs", std::string{kScript}}, {"atteso.txt", std::string{kReport}}});
    const Result result = check(folder);
    EXPECT_EQ(result.code, kExitPassed);
    expectContains(result.output, "passed: ");
    EXPECT_EQ(readFile(kOutput / "obtained" / "passes" / kObtainedReport), kReport);
}

TEST(CheckCase, ScriptThatDoesNotPass) {
    std::string expected{kReport};
    expected.replace(expected.find("vertex=10,0"), 11, "vertex=11,0");
    const fs::path folder =
        makeCase("fails", {{"script.tgs", std::string{kScript}}, {"atteso.txt", expected}});
    const Result result = check(folder);
    EXPECT_EQ(result.code, kExitFailed);
    expectContains(result.output, "FAILED: ");
    expectContains(result.output, "1 differences");
    expectContains(result.output,
                   "  line 5\n"
                   "    expected:   257 LWPOLYLINE closed=no vertex=0,0 vertex=11,0\n"
                   "    obtained:   257 LWPOLYLINE closed=no vertex=0,0 vertex=10,0\n");
    // The obtained report is written also when the case fails.
    EXPECT_EQ(readFile(kOutput / "obtained" / "fails" / kObtainedReport), kReport);
}

TEST(CheckCase, PendingInvertsTheResult) {
    std::string wrong{kReport};
    wrong.replace(wrong.find("vertex=10,0"), 11, "vertex=11,0");
    const fs::path failing =
        makeCase("pending-fails", {{"script.tgs", std::string{kScript}}, {"atteso.txt", wrong}});
    Result result = check(failing, true);
    EXPECT_EQ(result.code, kExitPassed);
    expectContains(result.output, "pending: ");
    expectContains(result.output, "warning: not passed yet");

    const fs::path passing = makeCase("pending-passes", {{"script.tgs", std::string{kScript}},
                                                         {"atteso.txt", std::string{kReport}}});
    result = check(passing, true);
    EXPECT_EQ(result.code, kExitFailed);
    expectContains(result.output, "remove it from tests/verifica/in-attesa.txt");
}

TEST(CheckCase, LevelTwoCaseIsSkipped) {
    const fs::path folder = makeCase(
        "level2", {{"script.tgs", "!level 2\nPLINE\n!key F8\n"}, {"atteso.txt", "anything"}});
    EXPECT_EQ(check(folder).code, kExitNeedsLevel2);
    EXPECT_EQ(check(folder, true).code, kExitNeedsLevel2);
    EXPECT_FALSE(fs::exists(kOutput / "obtained" / "level2" / kObtainedReport));
}

TEST(CheckCase, InvalidCasesFailEvenWhenPending) {
    const fs::path malformed =
        makeCase("malformed", {{"script.tgs", "PLINE\n!foo\n"}, {"atteso.txt", ""}});
    const fs::path empty = makeCase("empty", {});
    const fs::path both =
        makeCase("both", {{"script.tgs", ""}, {"atteso.txt", ""}, {"domande.tgq", ""}});
    const fs::path noExpected = makeCase("no-expected", {{"script.tgs", "PLINE\n"}});
    const fs::path badQuestions = makeCase("bad-questions", {{"domande.tgq", "? volume A -> 1\n"}});
    for (const fs::path& folder :
         {malformed, empty, both, noExpected, badQuestions, kOutput / "cases" / "does-not-exist"}) {
        for (const bool pending : {false, true}) {
            const Result result = check(folder, pending);
            EXPECT_EQ(result.code, kExitInvalid) << folder;
            expectContains(result.output, "INVALID: ");
        }
    }
    expectContains(check(malformed).output, "script.tgs, line 2: unknown directive '!foo'");
}

// What the specifications describe and the program does not have yet is a
// case not passed, not an invalid one: pending, it waits for its package
// without stopping the CI; not pending, it fails (docs/verifica/casi.md).
//
// A form that the program already has is left out, the way file_test.cpp
// leaves out a DXF drawing that io does not read yet: so the package that
// realizes it does not have to touch this test. It counts as realized only if
// the file is read; any other error is checked here, as an invalid case would
// fail the exit codes. If the program has all of them, the test is skipped.
TEST(CheckCase, SyntaxOfTheCatalogueNotRealizedYetIsNotPassed) {
    struct Form {
        std::string name;
        std::string file;
        std::string text;
    };

    const std::string drawing = "A = LWPOLYLINE vertex=0,0 vertex=10,0\n";
    const std::vector<Form> forms{
        {"announced-directive", "script.tgs", "PLINE\n!scr on\n"},
        {"announced-mode", "domande.tgq", drawing + "? snaps A modes=quadrant -> none\n"},
        {"announced-argument", "domande.tgq", drawing + "? snap 5,0 aperture=1 from=1,1 -> none\n"},
    };
    int checked = 0;
    for (const Form& form : forms) {
        const bool isScript = form.file == "script.tgs";
        const bool realized =
            isScript ? std::holds_alternative<Script>(parseScript(form.text))
                     : std::holds_alternative<std::vector<Answer>>(answerQuestions(form.text));
        if (realized) {
            continue;
        }
        ++checked;
        std::map<std::string, std::string> files{{form.file, form.text}};
        if (isScript) {
            files.emplace("atteso.txt", "");
        }
        const fs::path folder = makeCase(form.name, files);

        const Result failing = check(folder);
        EXPECT_EQ(failing.code, kExitFailed) << form.name;
        expectContains(failing.output, "FAILED: ");
        expectContains(failing.output, ": not implemented yet: ");
        expectContains(failing.output, "is not implemented yet");

        const Result pending = check(folder, true);
        EXPECT_EQ(pending.code, kExitPassed) << form.name;
        expectContains(pending.output, "pending: ");
        expectContains(pending.output, ": not implemented yet: ");
        expectContains(pending.output, "warning: not passed yet");
    }
    if (checked == 0) {
        GTEST_SKIP() << "the program already has all the syntax that the catalogue announced";
    }
}

TEST(CheckCase, AnnouncedQuestionsCanWaitButUnknownQuestionsStayInvalid) {
    const std::string drawing = "A = LINE start=0,0 end=10,0\n";
    const std::vector<std::string> forms{"transform A move 1,2", "grips A", "grip A 0,0 1,2"};
    int checked = 0;
    for (const std::string& form : forms) {
        std::string text = drawing;
        text += "? ";
        text += form;
        text += " -> none\n";
        const auto answers = answerQuestions(text);
        ASSERT_TRUE(std::holds_alternative<std::vector<Answer>>(answers)) << form;
        const auto& list = std::get<std::vector<Answer>>(answers);
        ASSERT_EQ(list.size(), 1U);
        // A realized question is covered by its package; do not freeze the
        // catalogue in its current, unimplemented state.
        if (list[0].actual != "error: question not implemented yet") {
            continue;
        }
        ++checked;
        const fs::path folder = makeCase("announced-question", {{"domande.tgq", text}});
        const Result failing = check(folder);
        EXPECT_EQ(failing.code, kExitFailed) << form;
        expectContains(failing.output, "question not implemented yet");
        const Result pending = check(folder, true);
        EXPECT_EQ(pending.code, kExitPassed) << form;
        expectContains(pending.output, "pending: ");
    }
    const std::string unknownText = drawing + "? transformx A move 1,2 -> none\n";
    const fs::path unknown = makeCase("unknown-question", {{"domande.tgq", unknownText}});
    for (const bool pending : {false, true}) {
        const Result result = check(unknown, pending);
        EXPECT_EQ(result.code, kExitInvalid);
        expectContains(result.output, "unknown question 'transformx'");
    }
    if (checked == 0) {
        GTEST_SKIP() << "the program already answers all three announced questions";
    }
}

TEST(CheckCase, Questions) {
    const fs::path passing =
        makeCase("questions-pass",
                 {{"domande.tgq", "A = LWPOLYLINE vertex=0,0 vertex=10,0\n? length A -> 10\n"}});
    EXPECT_EQ(check(passing).code, kExitPassed);
    EXPECT_EQ(readFile(kOutput / "obtained" / "questions-pass" / kObtainedAnswers),
              "A = LWPOLYLINE vertex=0,0 vertex=10,0\n? length A -> 10\n");

    const fs::path failing =
        makeCase("questions-fail",
                 {{"domande.tgq", "A = LWPOLYLINE vertex=0,0 vertex=10,0\n? length A -> 12\n"}});
    const Result result = check(failing);
    EXPECT_EQ(result.code, kExitFailed);
    expectContains(result.output, "  line 2\n    expected: 12\n    obtained: 10\n");
    EXPECT_EQ(readFile(kOutput / "obtained" / "questions-fail" / kObtainedAnswers),
              "A = LWPOLYLINE vertex=0,0 vertex=10,0\n? length A -> 10\n");
    EXPECT_EQ(check(failing, true).code, kExitPassed);
}

TEST(CheckCase, WrongFontMetricsFailAndWriteMeasuredValues) {
    const fs::path question =
        makeCase("wrong-font-metrics", {{"domande.tgq", "? font-metrics -> ascent=1 descent=0\n"}});
    const Result result = check(question);
    EXPECT_EQ(result.code, kExitFailed);
    expectContains(result.output, "expected: ascent=1 descent=0");
    expectContains(result.output,
                   "obtained: ascent=1.3158268275372604 descent=-0.3080198722498226");
    EXPECT_EQ(readFile(kOutput / "obtained" / "wrong-font-metrics" / kObtainedAnswers),
              "? font-metrics -> ascent=1.3158268275372604 descent=-0.3080198722498226\n");
    EXPECT_EQ(check(question, true).code, kExitPassed);
}

TEST(RunScriptFile, WritesTheReport) {
    const fs::path folder = makeCase("run", {{"script.tgs", std::string{kScript}}});
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runScriptFile(folder / "script.tgs", std::nullopt, out, err), kExitPassed);
    EXPECT_EQ(out.str(), kReport);
    EXPECT_TRUE(err.str().empty());

    const fs::path report = kOutput / "run-report" / "report.txt";
    fs::remove_all(report.parent_path());
    std::ostringstream silent;
    EXPECT_EQ(runScriptFile(folder / "script.tgs", report, silent, err), kExitPassed);
    EXPECT_TRUE(silent.str().empty());
    EXPECT_EQ(readFile(report), kReport);
}

TEST(RunScriptFile, ErrorsAndLevelTwo) {
    const fs::path folder =
        makeCase("run-errors", {{"bad.tgs", "\n!snap maybe\n"}, {"level2.tgs", "!level 2\n"}});
    std::ostringstream out;
    std::ostringstream err;
    EXPECT_EQ(runScriptFile(folder / "bad.tgs", std::nullopt, out, err), kExitInvalid);
    expectContains(err.str(), "line 2:");
    EXPECT_EQ(runScriptFile(folder / "level2.tgs", std::nullopt, out, err), kExitNeedsLevel2);
    EXPECT_EQ(runScriptFile(folder / "missing.tgs", std::nullopt, out, err), kExitInvalid);
    EXPECT_TRUE(out.str().empty());
}

}  // namespace
}  // namespace tg::script
