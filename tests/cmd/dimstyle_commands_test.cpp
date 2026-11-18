// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace tg::cmd {
namespace {

TEST(DimstyleCommandsTest, ChangesSavesRestoresListsAndUndoes) {
    core::Document document = core::Document::newDrawing();
    edit::Editor editor(document);
    CommandRegistry registry;
    registerBuiltinCommands(registry);
    CommandRunner runner(registry, editor);
    std::vector<std::string> messages;
    runner.setMessageSink([&](std::string message) { messages.push_back(std::move(message)); });

    runner.submitText("DIMTXT");
    runner.submitText("3.5");
    EXPECT_DOUBLE_EQ(document.dimStyles().currentVariables().textHeight, 3.5);
    for (const char* input : {"-DIMSTYLE", "S", "Large"}) {
        runner.submitText(input);
    }
    ASSERT_NE(document.dimStyles().find("Large"), nullptr);
    EXPECT_EQ(document.dimStyles().current(), "Large");

    for (const char* input : {"-DIMSTYLE", "R", "ISO-25"}) {
        runner.submitText(input);
    }
    EXPECT_EQ(document.dimStyles().current(), "ISO-25");
    EXPECT_DOUBLE_EQ(document.dimStyles().currentVariables().textHeight, 2.5);
    for (const char* input : {"-DIMSTYLE", "ST"}) {
        runner.submitText(input);
    }
    for (const char* input : {"-DIMSTYLE", "?", ""}) {
        runner.submitText(input);
    }
    EXPECT_NE(std::find(messages.begin(), messages.end(), "  Large"), messages.end());

    runner.submitText("U");
    EXPECT_EQ(document.dimStyles().current(), "Large");
}

TEST(DimstyleCommandsTest, RejectsOutOfRangeVariable) {
    core::Document document = core::Document::newDrawing();
    edit::Editor editor(document);
    CommandRegistry registry;
    registerBuiltinCommands(registry);
    CommandRunner runner(registry, editor);
    runner.submitText("DIMDEC");
    runner.submitText("9");
    EXPECT_TRUE(runner.isActive());
    runner.submitText("3");
    EXPECT_EQ(document.dimStyles().currentVariables().decimals, 3);
}

TEST(DimstyleCommandsTest, EmptyRestoreKeepsCurrentStyleWithoutUndoStep) {
    core::Document document = core::Document::newDrawing();
    const std::string current = document.dimStyles().current();
    const core::DimVariables variables = document.dimStyles().currentVariables();
    edit::Editor editor(document);
    CommandRegistry registry;
    registerBuiltinCommands(registry);
    CommandRunner runner(registry, editor);
    std::vector<std::string> messages;
    runner.setMessageSink([&](std::string message) { messages.push_back(std::move(message)); });

    for (const char* input : {"-DIMSTYLE", "R", ""}) {
        runner.submitText(input);
    }

    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(document.dimStyles().current(), current);
    EXPECT_EQ(document.dimStyles().currentVariables(), variables);
    EXPECT_FALSE(editor.canUndo());
    EXPECT_NE(std::find(messages.begin(), messages.end(), "Current dimension style: ISO-25"),
              messages.end());
    EXPECT_EQ(std::find(messages.begin(), messages.end(), "Unknown dimension style."),
              messages.end());
}

}  // namespace
}  // namespace tg::cmd
