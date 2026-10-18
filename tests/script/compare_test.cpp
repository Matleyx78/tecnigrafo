// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/compare.hpp"

#include "tg/script/script.hpp"

#include <gtest/gtest.h>

#include <string>
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

}  // namespace
}  // namespace tg::script
