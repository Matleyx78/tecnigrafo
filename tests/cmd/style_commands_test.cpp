// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace tg::cmd {
namespace {

TEST(StyleCommandsTest, CreatesChangesListsAndSelectsStylesUndoably) {
    core::Document document = core::Document::newDrawing();
    edit::Editor editor(document);
    CommandRegistry registry;
    registerBuiltinCommands(registry);
    CommandRunner runner(registry, editor);
    std::vector<std::string> messages;
    runner.setMessageSink([&](std::string message) { messages.push_back(std::move(message)); });
    for (const char* input : {"-STYLE", "Note", "times.ttf", "2.5", "0.8", "15", "No", "Yes"}) {
        runner.submitText(input);
    }
    const auto* note = document.textStyles().find("Note");
    ASSERT_NE(note, nullptr);
    EXPECT_EQ(note->id, 257U);
    EXPECT_EQ(note->font, "times.ttf");
    EXPECT_DOUBLE_EQ(note->fixedHeight, 2.5);
    EXPECT_DOUBLE_EQ(note->widthFactor, 0.8);
    EXPECT_DOUBLE_EQ(note->oblique, core::degreesToRadians(15.0));
    EXPECT_TRUE(note->upsideDown);
    EXPECT_EQ(document.textStyles().current(), "Note");
    for (const char* input : {"STYLE", "Note", "", "", "1", "", "Yes", "No"}) {
        runner.submitText(input);
    }
    EXPECT_DOUBLE_EQ(document.textStyles().find("Note")->widthFactor, 1.0);
    runner.submitText("-STYLE");
    runner.submitText("?");
    EXPECT_NE(std::find(messages.begin(), messages.end(), "Standard"), messages.end());
    runner.submit(CancelInput{});
    runner.submitText("TEXTSTYLE");
    runner.submitText("Missing");
    EXPECT_TRUE(runner.isActive());
    runner.submitText("Standard");
    EXPECT_EQ(document.textStyles().current(), "Standard");
    runner.submitText("U");
    EXPECT_EQ(document.textStyles().current(), "Note");
}

}  // namespace
}  // namespace tg::cmd
