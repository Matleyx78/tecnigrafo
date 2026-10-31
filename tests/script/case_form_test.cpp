// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/case_form.hpp"

#include <gtest/gtest.h>

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

namespace tg::script {
namespace {

constexpr std::string_view kCarriageReturn = "carriage return: lines end with LF only";
constexpr std::string_view kTab = "tab character";
constexpr std::string_view kEmptyLineAtEnd = "empty line at the end of the file";
constexpr std::string_view kEmptyLineNotBeforeSection = "empty line not followed by a section";
constexpr std::string_view kTrailingSpaces = "trailing spaces";
constexpr std::string_view kNoEcho =
    "no echo of this line in the messages of atteso.txt (an empty line is Enter: at Command: "
    "it repeats the last command)";

/// A report with one section [final] and these lines in its messages: the
/// form checks read only the structure, not the rest of the fields.
std::string report(std::initializer_list<std::string_view> messages) {
    std::string text = "tecnigrafo-report 1\n\n[final]\nentities:\nmessages:\n";
    for (const std::string_view message : messages) {
        text += "  ";
        text += message;
        text += "\n";
    }
    text += "prompt: Command:\nstatistics:\n";
    return text;
}

FormError expected(std::string_view file, int line, std::string_view message) {
    return {std::string{file}, line, std::string{message}};
}

// LINE, two points and an empty line that ends the command, with its report.
constexpr std::string_view kLineScript = "LINE\n0,0\n10,0\n\n";
const std::string kLineReport =
    report({"Command: LINE", "Specify first point: 0,0", "Specify next point or [Undo]: 10,0",
            "Specify next point or [Undo]:"});

TEST(ScriptForm, ValidCase) {
    EXPECT_EQ(checkScriptForm(kLineScript, kLineReport), std::nullopt);
}

TEST(ScriptForm, CarriageReturnInEitherFile) {
    EXPECT_EQ(checkScriptForm("LINE\r\n0,0\r\n10,0\r\n\r\n", kLineReport),
              expected("script.tgs", 1, kCarriageReturn));
    std::string withCr = kLineReport;
    withCr.replace(withCr.find("[final]\n"), 8, "[final]\r\n");
    EXPECT_EQ(checkScriptForm(kLineScript, withCr), expected("atteso.txt", 3, kCarriageReturn));
}

TEST(ScriptForm, TabInEitherFile) {
    EXPECT_EQ(checkScriptForm("LINE\n0,0\n10,0\n\n; a\tcomment\n", kLineReport),
              expected("script.tgs", 5, kTab));
    std::string withTab = kLineReport;
    withTab.replace(withTab.find("prompt: "), 8, "prompt:\t");
    EXPECT_EQ(checkScriptForm(kLineScript, withTab), expected("atteso.txt", 10, kTab));
}

TEST(ScriptForm, ExpectedReportEndsWithoutEmptyLines) {
    EXPECT_EQ(checkScriptForm(kLineScript, kLineReport + "\n\n"),
              expected("atteso.txt", 12, kEmptyLineAtEnd));
    // Empty lines at the end of a script are Enter, and allowed.
    EXPECT_EQ(checkScriptForm("LINE\n0,0\n10,0\n\n\n",
                              report({"Command: LINE", "Specify first point: 0,0",
                                      "Specify next point or [Undo]: 10,0",
                                      "Specify next point or [Undo]:", "Command:"})),
              std::nullopt);
}

TEST(ScriptForm, EmptyLineOnlyBeforeASection) {
    std::string misplaced = kLineReport;
    misplaced.replace(misplaced.find("messages:"), 9, "\nmessages:");
    EXPECT_EQ(checkScriptForm(kLineScript, misplaced),
              expected("atteso.txt", 5, kEmptyLineNotBeforeSection));

    // An empty line before every section, [report 1] and [final].
    const std::string twoSections =
        "tecnigrafo-report 1\n\n[report 1]\nmessages:\n  Command: LINE\n"
        "  Specify first point: 0,0\nprompt: Specify next point or [Undo]:\n\n"
        "[final]\nmessages:\n  Specify next point or [Undo]: 10,0\n"
        "  Specify next point or [Undo]:\nprompt: Command:\n";
    EXPECT_EQ(checkScriptForm("LINE\n0,0\n!report\n10,0\n\n", twoSections), std::nullopt);
}

TEST(ScriptForm, TrailingSpacesOnlyOnTypedLinesAndTheirEcho) {
    EXPECT_EQ(checkScriptForm("; a comment \nLINE\n0,0\n10,0\n\n", kLineReport),
              expected("script.tgs", 1, kTrailingSpaces));
    EXPECT_EQ(checkScriptForm("!snap off \nLINE\n0,0\n10,0\n\n", kLineReport),
              expected("script.tgs", 1, kTrailingSpaces));

    // A text that ends with two spaces, written on purpose, and its echo.
    const std::string_view script = "TEXT\n0,0\n2.5\n0\nab  \n";
    const std::string withEcho =
        report({"Command: TEXT", "Specify start point of text: 0,0", "Specify height: 2.5",
                "Specify rotation angle of text: 0", "Enter text: ab  "});
    EXPECT_EQ(checkScriptForm(script, withEcho), std::nullopt);

    // The same spaces on another line of the report.
    std::string elsewhere = withEcho;
    elsewhere.replace(elsewhere.find("prompt: Command:"), 16, "prompt: Command:  ");
    EXPECT_EQ(checkScriptForm(script, elsewhere), expected("atteso.txt", 11, kTrailingSpaces));
    // Without the typed line, the echo is no longer allowed either.
    EXPECT_EQ(checkScriptForm("TEXT\n0,0\n2.5\n0\nab\n", withEcho),
              expected("atteso.txt", 10, kTrailingSpaces));
}

TEST(ScriptForm, EchoOfAnEmptyLineThatEndsTheCommand) {
    EXPECT_EQ(checkScriptForm(kLineScript, kLineReport), std::nullopt);
}

// The empty line after a finished CIRCLE is Enter at Command:, which repeats
// the command: without its echo the reference is wrong (#417).
TEST(ScriptForm, EmptyLineAfterAFinishedCommandWithoutEcho) {
    const std::string circle =
        report({"Command: CIRCLE", "Specify center point for circle or [3P/2P/Ttr]: 0,0",
                "Specify radius of circle or [Diameter]: 10"});
    EXPECT_EQ(checkScriptForm("CIRCLE\n0,0\n10\n\n", circle), expected("script.tgs", 4, kNoEcho));
    // Also when a later empty line has its echo: an echo comes in order,
    // before the echo of the next non-empty line.
    const std::string thenLine = report(
        {"Command: CIRCLE", "Specify center point for circle or [3P/2P/Ttr]: 0,0",
         "Specify radius of circle or [Diameter]: 10", "Command: LINE", "Specify first point: 0,0",
         "Specify next point or [Undo]: 10,0", "Specify next point or [Undo]:"});
    EXPECT_EQ(checkScriptForm("CIRCLE\n0,0\n10\n\nLINE\n0,0\n10,0\n\n", thenLine),
              expected("script.tgs", 4, kNoEcho));
}

TEST(ScriptForm, NonEmptyLineWithoutEcho) {
    EXPECT_EQ(checkScriptForm("LINE\n0,0\n20,0\n\n", kLineReport),
              expected("script.tgs", 3, kNoEcho));
    // An echo is looked for after the echo of the line before.
    EXPECT_EQ(checkScriptForm("LINE\n10,0\n0,0\n\n", kLineReport),
              expected("script.tgs", 3, kNoEcho));
}

TEST(ScriptForm, EchoesSplitBetweenTwoSections) {
    const std::string twoSections =
        "tecnigrafo-report 1\n\n[report 1]\nmessages:\n  Command: LINE\n"
        "  Specify first point: 0,0\nprompt: Specify next point or [Undo]:\n\n"
        "[final]\nmessages:\n  Specify next point or [Undo]: 10,0\n"
        "  Specify next point or [Undo]:\nprompt: Command:\n";
    EXPECT_EQ(
        checkScriptForm("; first point, then a report\nLINE\n0,0\n!report\n10,0\n\n", twoSections),
        std::nullopt);
    // Only the messages count: the prompt of a section is no echo.
    EXPECT_EQ(checkScriptForm("LINE\n0,0\n!report\n10,0\n\n\n", twoSections),
              expected("script.tgs", 6, kNoEcho));
}

TEST(ScriptForm, NoEchoCheckAtLevel2) {
    EXPECT_EQ(checkScriptForm("!level 2\nPL 0,0 50,0\n", report({})), std::nullopt);
    // The rules of the form still hold at level 2.
    EXPECT_EQ(checkScriptForm("!level 2\nPL 0,0 50,0\n", report({}) + "\n"),
              expected("atteso.txt", 8, kEmptyLineAtEnd));
}

TEST(ScriptForm, ByteOrderMarkDoesNotCount) {
    const std::string bom = "\xEF\xBB\xBF";
    EXPECT_EQ(checkScriptForm(bom + std::string{kLineScript}, bom + kLineReport), std::nullopt);
    EXPECT_EQ(checkQuestionsForm(bom + "? length A -> 10\n"), std::nullopt);
}

TEST(QuestionsForm, ValidFile) {
    EXPECT_EQ(checkQuestionsForm("; a comment\nA = LINE start=0,0 end=10,0\n\n? length A -> 10\n"),
              std::nullopt);
}

TEST(QuestionsForm, Violations) {
    EXPECT_EQ(checkQuestionsForm("? length A -> 10\r\n"),
              expected("domande.tgq", 1, kCarriageReturn));
    EXPECT_EQ(checkQuestionsForm("A = LINE start=0,0 end=10,0\n? length A\t-> 10\n"),
              expected("domande.tgq", 2, kTab));
    EXPECT_EQ(checkQuestionsForm("? length A -> 10\n\n"),
              expected("domande.tgq", 2, kEmptyLineAtEnd));
    EXPECT_EQ(checkQuestionsForm("? length A -> 10 \n"),
              expected("domande.tgq", 1, kTrailingSpaces));
    EXPECT_EQ(checkQuestionsForm("; a comment \n? length A -> 10\n"),
              expected("domande.tgq", 1, kTrailingSpaces));
}

}  // namespace
}  // namespace tg::script
