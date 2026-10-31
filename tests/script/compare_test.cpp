// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/compare.hpp"

#include "tg/script/script.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

const Tolerances kDefault;

TEST(CompareReports, EqualReportsHaveNoDifferences) {
    const std::string report = "tecnigrafo-report 1\n\n[final]\nentities:\n";
    EXPECT_TRUE(compareReports(report, report, kDefault).empty());
}

TEST(CompareReports, NumbersWithinTheAbsoluteToleranceAreEqual) {
    EXPECT_TRUE(
        compareReports("vertex=0,50\n", "vertex=3.061616997868383e-15,50\n", kDefault).empty());
    EXPECT_TRUE(compareReports("x=1\n", "x=1.0000000009\n", kDefault).empty());
    EXPECT_TRUE(compareReports("x=100\n", "x=100.0\n", kDefault).empty());
    EXPECT_TRUE(compareReports("x=0.00001\n", "x=1e-05\n", kDefault).empty());

    const std::vector<Difference> differences =
        compareReports("x=1\n", "x=1.000000002\n", kDefault);
    ASSERT_EQ(differences.size(), 1U);
    EXPECT_EQ(differences[0], (Difference{1, "x=1", "x=1.000000002"}));
}

TEST(CompareReports, RelativeToleranceScalesWithTheNumbers) {
    Tolerances tolerances;
    tolerances.relative = 1e-12;
    EXPECT_TRUE(compareReports("r=1000000\n", "r=1000000.0000005\n", tolerances).empty());
    EXPECT_FALSE(compareReports("r=1000000\n", "r=1000000.00001\n", tolerances).empty());
}

TEST(CompareReports, ToleranceDirectiveOfTheScriptIsUsed) {
    const std::variant<Script, ParseError> script = parseScript("!tolerance absolute 0.01\n");
    ASSERT_TRUE(std::holds_alternative<Script>(script));
    const Tolerances& tolerances = std::get<Script>(script).tolerances;
    EXPECT_TRUE(compareReports("x=1\n", "x=1.005\n", tolerances).empty());
    EXPECT_FALSE(compareReports("x=1\n", "x=1.02\n", tolerances).empty());
}

TEST(CompareReports, TextMustMatchExactly) {
    const std::vector<Difference> differences =
        compareReports("prompt: Command:\nsnap: on\n", "prompt: Command:\nsnap: off\n", kDefault);
    ASSERT_EQ(differences.size(), 1U);
    EXPECT_EQ(differences[0], (Difference{2, "snap: on", "snap: off"}));

    EXPECT_FALSE(compareReports("a  b\n", "a b\n", kDefault).empty());
    EXPECT_FALSE(compareReports("x=1,2\n", "x=1;2\n", kDefault).empty());
    EXPECT_FALSE(compareReports("x=1,2\n", "x=1,2,3\n", kDefault).empty());
}

TEST(CompareReports, DigitsInsideWordsAreText) {
    // "R10" is not the number 10: a different word is a different text.
    EXPECT_FALSE(compareReports("issue R10\n", "issue R10.0000000001\n", kDefault).empty());
    EXPECT_TRUE(compareReports("3 found, 5 total\n", "3 found, 5 total\n", kDefault).empty());
    EXPECT_FALSE(compareReports("3 found\n", "4 found\n", kDefault).empty());
}

TEST(CompareReports, MissingAndExtraLines) {
    std::vector<Difference> differences = compareReports("a\nb\nc\n", "a\nb\n", kDefault);
    ASSERT_EQ(differences.size(), 1U);
    EXPECT_EQ(differences[0], (Difference{3, "c", std::string{kNoLine}}));

    differences = compareReports("a\n", "a\nb\nc\n", kDefault);
    ASSERT_EQ(differences.size(), 2U);
    EXPECT_EQ(differences[0], (Difference{2, std::string{kNoLine}, "b"}));
    EXPECT_EQ(differences[1], (Difference{3, std::string{kNoLine}, "c"}));
}

TEST(CompareReports, EmptyLinesCount) {
    EXPECT_FALSE(compareReports("a\n\nb\n", "a\nb\n", kDefault).empty());
}

TEST(CompareReports, LineEndingsAndByteOrderMarkDoNotCount) {
    EXPECT_TRUE(compareReports("a\r\nb\r\n", "a\nb", kDefault).empty());
    EXPECT_TRUE(compareReports("\xEF\xBB\xBF"
                               "a\nb\n",
                               "a\nb\n", kDefault)
                    .empty());
}

TEST(SplitLines, AFinalLineEndingStartsNoLine) {
    EXPECT_TRUE(splitLines("").empty());
    EXPECT_EQ(splitLines("a"), (std::vector<std::string_view>{"a"}));
    EXPECT_EQ(splitLines("a\n"), (std::vector<std::string_view>{"a"}));
    EXPECT_EQ(splitLines("a\n\n"), (std::vector<std::string_view>{"a", ""}));
    EXPECT_EQ(splitLines("a\r\nb"), (std::vector<std::string_view>{"a", "b"}));
}

TEST(NumbersMatch, AbsoluteOrRelative) {
    Tolerances tolerances;
    tolerances.absolute = 0.1;
    tolerances.relative = 0.01;
    EXPECT_TRUE(numbersMatch(1.0, 1.09, tolerances));
    EXPECT_FALSE(numbersMatch(1.0, 1.2, tolerances));
    EXPECT_TRUE(numbersMatch(100.0, 101.0, tolerances));
    EXPECT_FALSE(numbersMatch(100.0, 102.1, tolerances));
}

// docs/adr/0037, point 6. The expected report of a case whose script
// draws a line, writes [report 1], draws a circle, then types a command the
// program does not have yet.
constexpr std::string_view kExpected =
    "tecnigrafo-report 1\n"            // 1
    "\n"                               // 2
    "[report 1]\n"                     // 3
    "entities:\n"                      // 4
    "  257 LINE start=0,0 end=10,0\n"  // 5
    "messages:\n"                      // 6
    "  Command: LINE\n"                // 7
    "  Specify first point: 0,0\n"     // 8
    "prompt: Command:\n"               // 9
    "\n"                               // 10
    "[final]\n"                        // 11
    "entities:\n"                      // 12
    "  257 LINE start=0,0 end=10,0\n"  // 13
    "  258 FUTURE center=5,5\n"        // 14
    "messages:\n"                      // 15
    "  Command: CIRCLE\n"              // 16
    "  Specify center point: 5,5\n"    // 17
    "  Command: FUTURE\n"              // 18
    "prompt: Command:\n";              // 19

// What the program writes up to the frontier, before FUTURE.
constexpr std::string_view kReportOne =
    "tecnigrafo-report 1\n"
    "\n"
    "[report 1]\n"
    "entities:\n"
    "  257 LINE start=0,0 end=10,0\n"
    "messages:\n"
    "  Command: LINE\n"
    "  Specify first point: 0,0\n"
    "prompt: Command:\n";
constexpr std::string_view kFinalAtFrontier =
    "\n"
    "[final]\n"
    "entities:\n"
    "  257 LINE start=0,0 end=10,0\n"
    "messages:\n"
    "  Command: CIRCLE\n"
    "  Specify center point: 5,5\n"
    "prompt: Specify radius:\n";

std::string obtained(std::string_view reportOne = kReportOne,
                     std::string_view final = kFinalAtFrontier) {
    return std::string{reportOne} + std::string{final};
}

std::string replaced(std::string_view text, std::string_view from, std::string_view to) {
    std::string result{text};
    const std::size_t at = result.find(from);
    EXPECT_NE(at, std::string::npos) << from;
    if (at != std::string::npos) {
        result.replace(at, from.size(), to);
    }
    return result;
}

TEST(CompareReportPrefix, SameBeforeTheFrontier) {
    EXPECT_TRUE(compareReportPrefix(kExpected, obtained(), kDefault).empty());
    // Line endings and a byte order mark do not count, as in compareReports().
    EXPECT_TRUE(compareReportPrefix(kExpected, "\xEF\xBB\xBF" + obtained(), kDefault).empty());
    // Numbers within the tolerances.
    Tolerances loose;
    loose.absolute = 0.01;
    EXPECT_TRUE(compareReportPrefix(
                    kExpected, obtained(replaced(kReportOne, "end=10,0", "end=10.001,0")), loose)
                    .empty());
    // The frontier before the first !report: the first expected section.
    EXPECT_TRUE(compareReportPrefix(kExpected,
                                    "tecnigrafo-report 1\n\n[final]\nentities:\nmessages:\n"
                                    "  Command: LINE\nprompt: Specify first point:\n",
                                    kDefault)
                    .empty());
}

TEST(CompareReportPrefix, DifferentSectionBeforeTheFrontier) {
    const std::vector<Difference> differences = compareReportPrefix(
        kExpected, obtained(replaced(kReportOne, "end=10,0", "end=11,0")), kDefault);
    ASSERT_EQ(differences.size(), 1U);
    EXPECT_EQ(differences[0],
              (Difference{5, "  257 LINE start=0,0 end=10,0", "  257 LINE start=0,0 end=11,0"}));
    // A line more in the obtained section.
    const std::vector<Difference> longer = compareReportPrefix(
        kExpected, obtained(replaced(kReportOne, "prompt:", "  *Cancel*\nprompt:")), kDefault);
    ASSERT_FALSE(longer.empty());
    EXPECT_EQ(longer[0], (Difference{9, "prompt: Command:", "  *Cancel*"}));
}

// #296: the expected handle of the first entity is 258 instead of 257, in a
// section written before a command the program does not have yet.
TEST(CompareReportPrefix, WrongHandleBeforeAnUnknownCommand) {
    const std::string expected = replaced(kExpected, "  257 LINE", "  258 LINE");
    const std::vector<Difference> differences = compareReportPrefix(expected, obtained(), kDefault);
    ASSERT_EQ(differences.size(), 1U);
    EXPECT_EQ(differences[0],
              (Difference{5, "  258 LINE start=0,0 end=10,0", "  257 LINE start=0,0 end=10,0"}));
}

TEST(CompareReportPrefix, MessagesOfTheFinalSectionAreTheStartOfTheExpectedOnes) {
    // #301: a question of a command that exists, written wrong.
    std::vector<Difference> differences = compareReportPrefix(
        replaced(kExpected, "Specify center point: 5,5", "Specify centre point: 5,5"), obtained(),
        kDefault);
    ASSERT_EQ(differences.size(), 1U);
    EXPECT_EQ(differences[0],
              (Difference{17, "  Specify centre point: 5,5", "  Specify center point: 5,5"}));

    // More messages than the expected section has.
    differences = compareReportPrefix(
        kExpected,
        obtained(kReportOne, replaced(kFinalAtFrontier, "prompt:",
                                      "  Command: FUTURE\n  Unknown command: FUTURE.\nprompt:")),
        kDefault);
    ASSERT_EQ(differences.size(), 1U);
    EXPECT_EQ(differences[0], (Difference{19, std::string{kNoLine}, "  Unknown command: FUTURE."}));

    // The rest of the final section is not compared: entities, prompt.
    EXPECT_TRUE(compareReportPrefix(
                    kExpected,
                    obtained(kReportOne, replaced(kFinalAtFrontier, "  257 LINE", "  999 LINE")),
                    kDefault)
                    .empty());
}

TEST(CompareReportPrefix, HeaderAndMissingSections) {
    std::vector<Difference> differences = compareReportPrefix(
        kExpected, replaced(obtained(), "tecnigrafo-report 1", "tecnigrafo-report 2"), kDefault);
    ASSERT_EQ(differences.size(), 1U);
    EXPECT_EQ(differences[0], (Difference{1, "tecnigrafo-report 1", "tecnigrafo-report 2"}));

    // [report 2] is not in the expected report: one difference.
    const std::string twoReports = std::string{kReportOne} +
                                   "\n[report 2]\nentities:\nmessages:\n  Command: X\n"
                                   "prompt: Command:\n" +
                                   std::string{kFinalAtFrontier};
    differences = compareReportPrefix(kExpected, twoReports, kDefault);
    ASSERT_EQ(differences.size(), 1U);
    EXPECT_EQ(differences[0], (Difference{20, std::string{kNoLine}, "[report 2]"}));
}

}  // namespace
}  // namespace tg::script
