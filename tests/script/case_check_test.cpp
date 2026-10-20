// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/case_check.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>

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
    "  1 LWPOLYLINE closed=no vertex=0,0 vertex=10,0\n"
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
                   "    expected:   1 LWPOLYLINE closed=no vertex=0,0 vertex=11,0\n"
                   "    obtained:   1 LWPOLYLINE closed=no vertex=0,0 vertex=10,0\n");
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

TEST(CheckCase, WhatIsNotImplementedYetFailsLikeAnyCase) {
    const fs::path text = makeCase("not-implemented-type", {{"domande.tgq",
                                                             "T = TEXT position=0,0 value=\"A\"\n"
                                                             "? bbox T -> 0,0 1,1\n"}});
    Result result = check(text);
    EXPECT_EQ(result.code, kExitFailed);
    expectContains(result.output, "not implemented yet: domande.tgq, line 1:");
    EXPECT_EQ(check(text, true).code, kExitPassed);

    const fs::path question = makeCase("not-implemented-question",
                                       {{"domande.tgq", "? font-metrics -> ascent=1 descent=0\n"}});
    result = check(question);
    EXPECT_EQ(result.code, kExitFailed);
    expectContains(result.output, "obtained: error: question not implemented yet");
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
