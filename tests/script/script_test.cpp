// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/script.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>
#include <variant>

namespace tg::script {
namespace {

Script parsed(std::string_view text) {
    std::variant<Script, ParseError> result = parseScript(text);
    if (const auto* error = std::get_if<ParseError>(&result)) {
        ADD_FAILURE() << "line " << error->line << ": " << error->message;
        return {};
    }
    return std::get<Script>(std::move(result));
}

ParseError failure(std::string_view text) {
    std::variant<Script, ParseError> result = parseScript(text);
    if (!std::holds_alternative<ParseError>(result)) {
        ADD_FAILURE() << "expected an error for: " << text;
        return {};
    }
    return std::get<ParseError>(std::move(result));
}

/// The only action of a one-line script.
Action single(std::string_view text) {
    const Script script = parsed(text);
    if (script.actions.size() != 1) {
        ADD_FAILURE() << "expected one action for: " << text;
        return TypedLine{};
    }
    return script.actions.front();
}

TEST(ParseScript, NormalLinesAreTypedTextKeptExactly) {
    const Script script = parsed("PLINE\n0,0\n  @10,0  \n\n");
    ASSERT_EQ(script.actions.size(), 4U);
    EXPECT_EQ(script.actions[0], Action(TypedLine{"PLINE"}));
    EXPECT_EQ(script.actions[1], Action(TypedLine{"0,0"}));
    EXPECT_EQ(script.actions[2], Action(TypedLine{"  @10,0  "}));  // spaces kept
    EXPECT_EQ(script.actions[3], Action(TypedLine{""}));           // empty line: Enter
    EXPECT_EQ(script.level, 1);
}

TEST(ParseScript, CommentsAreDropped) {
    const Script script = parsed("; a comment\nPLINE\n;\n; another one\n");
    ASSERT_EQ(script.actions.size(), 1U);
    EXPECT_EQ(script.actions[0], Action(TypedLine{"PLINE"}));
}

TEST(ParseScript, OnlyAFirstSemicolonOrExclamationMarkCounts) {
    const Script script = parsed(" ; typed\n !esc\n");
    ASSERT_EQ(script.actions.size(), 2U);
    EXPECT_EQ(script.actions[0], Action(TypedLine{" ; typed"}));
    EXPECT_EQ(script.actions[1], Action(TypedLine{" !esc"}));
}

TEST(ParseScript, AFinalLineEndingAddsNoLine) {
    EXPECT_EQ(parsed("PLINE").actions.size(), 1U);
    EXPECT_EQ(parsed("PLINE\n").actions.size(), 1U);
    EXPECT_EQ(parsed("PLINE\n\n").actions.size(), 2U);
    EXPECT_TRUE(parsed("").actions.empty());
}

TEST(ParseScript, ByteOrderMarkAndCarriageReturnsGo) {
    const Script script = parsed("\xEF\xBB\xBFPLINE\r\n0,0\r\n\r\n");
    ASSERT_EQ(script.actions.size(), 3U);
    EXPECT_EQ(script.actions[0], Action(TypedLine{"PLINE"}));
    EXPECT_EQ(script.actions[1], Action(TypedLine{"0,0"}));
    EXPECT_EQ(script.actions[2], Action(TypedLine{""}));
}

TEST(ParseScript, NonAsciiTextIsKept) {
    EXPECT_EQ(single("Prova d'uso è 100%%c"), Action(TypedLine{"Prova d'uso è 100%%c"}));
}

TEST(ParseScript, EveryDirective) {
    EXPECT_EQ(single("!view 10,-5 0.25 800x600"),
              Action(ViewDirective{{10.0, -5.0}, 0.25, 800, 600}));
    EXPECT_EQ(single("!move 1.5,2"), Action(MoveDirective{{1.5, 2.0}}));
    EXPECT_EQ(single("!click 3,4"), Action(ClickDirective{{3.0, 4.0}, false}));
    EXPECT_EQ(single("!click 3,4 remove"), Action(ClickDirective{{3.0, 4.0}, true}));
    EXPECT_EQ(single("!rightclick"), Action(RightClickDirective{}));
    EXPECT_EQ(single("!esc"), Action(EscDirective{}));
    EXPECT_EQ(single("!snap on"), Action(AidDirective{AidDirective::Aid::Snap, true}));
    EXPECT_EQ(single("!snap off"), Action(AidDirective{AidDirective::Aid::Snap, false}));
    EXPECT_EQ(single("!ortho on"), Action(AidDirective{AidDirective::Aid::Ortho, true}));
    EXPECT_EQ(single("!ortho off"), Action(AidDirective{AidDirective::Aid::Ortho, false}));
    EXPECT_EQ(single("!report"), Action(ReportDirective{}));
    EXPECT_EQ(single("!level 1"), Action(LevelDirective{1}));
    EXPECT_EQ(single("!tolerance absolute 1e-6"),
              Action(ToleranceDirective{ToleranceDirective::Kind::Absolute, 1e-6}));
}

TEST(ParseScript, OsnapDirective) {
    const core::SnapModes none{false, false, false, false, false, false, false, false};
    EXPECT_EQ(single("!osnap none"), Action(OsnapDirective{none}));

    core::SnapModes ends = none;
    ends.endpoint = true;
    ends.midpoint = true;
    EXPECT_EQ(single("!osnap end,mid"), Action(OsnapDirective{ends}));

    const core::SnapModes all{true, true, true, true, true, true, true, true};
    EXPECT_EQ(single("!osnap end,mid,cen,ins,int,per,tan,qua"), Action(OsnapDirective{all}));

    // The order and repeats do not matter.
    core::SnapModes qp = none;
    qp.quadrant = true;
    qp.perpendicular = true;
    EXPECT_EQ(single("!osnap qua,per,qua"), Action(OsnapDirective{qp}));
}

TEST(ParseScript, OsnapIsParsedAndNeverNotImplemented) {
    for (const char* line : {"!osnap", "!osnap end mid", "!osnap foo", "!osnap end,", "!osnap ,end",
                             "!osnap END", "!osnap none,end", "!osnap on"}) {
        const ParseError error = failure(std::string{"; comment\n"} + line + "\n");
        EXPECT_EQ(error.line, 2) << line;
        EXPECT_FALSE(error.message.empty()) << line;
        EXPECT_FALSE(error.notImplemented) << line;
    }
}

TEST(ParseScript, ArgumentsMayBeSeparatedByTabsAndManySpaces) {
    EXPECT_EQ(single("!click  3,4\tremove  "), Action(ClickDirective{{3.0, 4.0}, true}));
}

TEST(ParseScript, LevelTwoDirectivesNeedLevelTwo) {
    const Script script = parsed("!level 2\n!key F8\n!screenshot after-click\n");
    EXPECT_EQ(script.level, 2);
    ASSERT_EQ(script.actions.size(), 3U);
    EXPECT_EQ(script.actions[0], Action(LevelDirective{2}));
    EXPECT_EQ(script.actions[1], Action(KeyDirective{"F8"}));
    EXPECT_EQ(script.actions[2], Action(ScreenshotDirective{"after-click"}));

    // "!level 2" may come after the directives that need it.
    EXPECT_EQ(parsed("!key Ctrl+Z\n!level 2\n").level, 2);

    const ParseError error = failure("PLINE\n!key F3\n");
    EXPECT_EQ(error.line, 2);
    EXPECT_FALSE(error.notImplemented);
    EXPECT_EQ(failure("!level 1\n!screenshot view\n").line, 2);
    EXPECT_EQ(failure("!level 1\n!level 2\n").line, 1);
}

TEST(ParseScript, TolerancesApplyToTheWholeScript) {
    const Script script = parsed(
        "!tolerance absolute 1e-6\n!tolerance relative 1e-12\n"
        "!tolerance channel 16\n!tolerance pixels 0.5\nPLINE\n");
    EXPECT_EQ(script.tolerances.absolute, 1e-6);
    EXPECT_EQ(script.tolerances.relative, 1e-12);
    EXPECT_EQ(script.tolerances.channel, 16);
    EXPECT_EQ(script.tolerances.pixels, 0.5);
    EXPECT_EQ(parsed("PLINE\n").tolerances, Tolerances{});
}

TEST(ParseScript, ErrorsGiveTheirLine) {
    struct Case {
        const char* text;
        int line;
    };

    for (const Case& test : {
             Case{"!foo", 1},
             Case{"PLINE\n!", 2},
             Case{"PLINE\n0,0\n! esc", 3},
             Case{"!view 0,0 1", 1},
             Case{"!view 0,0 0 800x600", 1},
             Case{"!view 0,0 -1 800x600", 1},
             Case{"!view 0,0 1 800", 1},
             Case{"!view 0,0 1 0x600", 1},
             Case{"!view 0,0 1 800x99999", 1},
             Case{"!view 0;0 1 800x600", 1},
             Case{"!move", 1},
             Case{"!move 1,2 3,4", 1},
             Case{"!move 1, 2", 1},
             Case{"!click 1,2 add", 1},
             Case{"!click remove", 1},
             Case{"!rightclick now", 1},
             Case{"\n\n!esc 1", 3},
             Case{"!report all", 1},
             Case{"!snap", 1},
             Case{"!snap yes", 1},
             Case{"!ortho ON", 1},
             Case{"!level 3", 1},
             Case{"!level 0", 1},
             Case{"!level", 1},
             Case{"!level 2\n!key", 2},
             Case{"!level 2\n!screenshot a b", 2},
             Case{"!level 2\n!screenshot a/b", 2},
             Case{"!tolerance absolute", 1},
             Case{"!tolerance absolute -1", 1},
             Case{"!tolerance exact 1", 1},
             Case{"!tolerance channel 256", 1},
             Case{"!tolerance channel 1.5", 1},
             Case{"!tolerance pixels 101", 1},
             Case{"!View 0,0 1 800x600", 1},
         }) {
        const ParseError error = failure(test.text);
        EXPECT_EQ(error.line, test.line) << test.text;
        EXPECT_FALSE(error.message.empty()) << test.text;
        EXPECT_FALSE(error.notImplemented) << test.text;
    }
}

// The directives that docs/verifica/script.md describes from stage 0.7, before
// the packages that realize them (P3 !scr, P8 !polar, P7 !osnap), each with a
// form that is valid once the program has it.
constexpr std::array<std::string_view, 3> kCatalogueDirectives{"scr", "osnap", "polar"};

// What the catalogue describes is never an invalid script: either the program
// has it and the line is read, or it does not have it yet and the script is
// not implemented yet, so the case is not passed (docs/verifica/casi.md). The
// test does not say which of the two holds today, so a package that realizes
// a directive does not have to touch it. If the program does not know a
// directive of the catalogue and does not announce it either, this fails.
TEST(ParseScript, DirectivesOfTheCatalogueAreNeverInvalid) {
    struct Case {
        std::string_view text;
        int line;
    };

    for (const Case& test : {
             Case{"!scr on", 1},
             Case{"!scr off", 1},
             Case{"PLINE\n0,0\n!osnap end,mid", 3},
             Case{"!osnap none", 1},
             Case{"!polar on", 1},
             Case{"!polar off", 1},
         }) {
        const std::variant<Script, ParseError> result = parseScript(test.text);
        if (const auto* error = std::get_if<ParseError>(&result)) {
            EXPECT_TRUE(error->notImplemented) << test.text << ": " << error->message;
            EXPECT_EQ(error->line, test.line) << test.text;
            EXPECT_NE(error->message.find("not implemented yet"), std::string::npos)
                << error->message;
        }
    }
}

// A typing error of a directive of the catalogue (the wrong case, one letter
// more or less) is still a malformed script, even though the directive is
// announced. Derived from the same names, so it holds whichever the program
// has.
TEST(ParseScript, TypingErrorsOfCatalogueDirectivesStayInvalid) {
    for (const std::string_view name : kCatalogueDirectives) {
        std::string capital{name};
        capital[0] = static_cast<char>(capital[0] - 'a' + 'A');
        for (const std::string& typo :
             {capital, std::string{name} + "x", std::string{name.substr(0, name.size() - 1)}}) {
            const ParseError error = failure("!" + typo + " on");
            EXPECT_EQ(error.line, 1) << typo;
            EXPECT_NE(error.message.find("unknown directive"), std::string::npos) << error.message;
            EXPECT_FALSE(error.notImplemented) << typo;
        }
    }
    EXPECT_FALSE(failure("!foo").notImplemented);
}

}  // namespace
}  // namespace tg::script
