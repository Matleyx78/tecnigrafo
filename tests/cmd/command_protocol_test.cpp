// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Includes only the protocol of commands: tests/CMakeLists.txt also compiles
// this file with no include folder of core, edit or io (docs/adr/0041,
// point 5).
#include "tg/cmd/command_protocol.hpp"

#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

Prompt promptWith(std::vector<Keyword> keywords, FilePrompt file = FilePrompt::None) {
    Prompt prompt;
    prompt.text = "Specify next point or [Close/Undo]:";
    prompt.keywords = std::move(keywords);
    prompt.kind = file == FilePrompt::None ? PromptKind::Point : PromptKind::Text;
    prompt.file = file;
    return prompt;
}

TEST(CommandProtocol, ContextCarriesOnlyWhatEveryCommandNeeds) {
    std::vector<std::string> history;
    const ProtocolContext context{[&history](std::string text) {
        history.push_back(std::move(text));
    }};
    EXPECT_FALSE(context.scriptSequences);
    context.message("*Cancel*");
    EXPECT_EQ(history, std::vector<std::string>{"*Cancel*"});
    static_assert(std::is_aggregate_v<ProtocolContext>);
}

TEST(CommandProtocol, KeywordMatchesItsNameOrShortcutInAnyCase) {
    const Keyword close{"Close", "C"};
    EXPECT_TRUE(matchesKeyword(close, "Close"));
    EXPECT_TRUE(matchesKeyword(close, "cLOSE"));
    EXPECT_TRUE(matchesKeyword(close, "C"));
    EXPECT_TRUE(matchesKeyword(close, "c"));
    EXPECT_FALSE(matchesKeyword(close, "Clo"));
    EXPECT_FALSE(matchesKeyword(close, " C"));  // the caller trims
    EXPECT_FALSE(matchesKeyword(close, ""));
}

TEST(CommandProtocol, EmptyShortcutMatchesNothing) {
    const Keyword all{"ALL", ""};
    EXPECT_TRUE(matchesKeyword(all, "all"));
    EXPECT_FALSE(matchesKeyword(all, ""));
}

TEST(CommandProtocol, CaseFoldingIsAsciiOnly) {
    // "è" is two bytes in UTF-8, unchanged by the ASCII folding.
    EXPECT_TRUE(matchesKeyword(Keyword{"Più", "P"}, "PIù"));
    EXPECT_FALSE(matchesKeyword(Keyword{"Più", "P"}, "PIÙ"));
}

TEST(CommandProtocol, FindsTheFirstKeywordThatTextNames) {
    const Prompt prompt = promptWith({{"Close", "C"}, {"Undo", "U"}, {"Copy", "C"}});
    const Keyword* undo = findKeyword(prompt, "u");
    ASSERT_NE(undo, nullptr);
    EXPECT_EQ(undo->name, "Undo");
    EXPECT_EQ(undo, &prompt.keywords[1]);  // points into the prompt
    const Keyword* first = findKeyword(prompt, "C");
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->name, "Close");
    const Keyword* copy = findKeyword(prompt, "copy");
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(copy->name, "Copy");
    EXPECT_EQ(findKeyword(prompt, "Arc"), nullptr);
    EXPECT_EQ(findKeyword(promptWith({}), "Close"), nullptr);
}

TEST(CommandProtocol, TextAnswerIsTheLineAsTyped) {
    const Prompt prompt = promptWith({});
    EXPECT_EQ(textAnswer(prompt, "  two words "), "  two words ");
    EXPECT_EQ(textAnswer(prompt, "\"quoted\""), "\"quoted\"");
    EXPECT_EQ(textAnswer(prompt, ""), "");
}

TEST(CommandProtocol, FileAnswerLosesTheQuotesAroundIt) {
    for (const FilePrompt file : {FilePrompt::Open, FilePrompt::Save}) {
        const Prompt prompt = promptWith({}, file);
        EXPECT_EQ(textAnswer(prompt, "\"My drawings/plan.cadx\""), "My drawings/plan.cadx");
        EXPECT_EQ(textAnswer(prompt, "plan.cadx"), "plan.cadx");
        EXPECT_EQ(textAnswer(prompt, "\"\""), "");
        EXPECT_EQ(textAnswer(prompt, "\""), "\"");
        EXPECT_EQ(textAnswer(prompt, "\"plan.cadx"), "\"plan.cadx");
        EXPECT_EQ(textAnswer(prompt, "plan.cadx\""), "plan.cadx\"");
        EXPECT_EQ(textAnswer(prompt, "\"\"x\"\""), "\"x\"");  // only one pair goes
    }
}

}  // namespace
}  // namespace tg::cmd
