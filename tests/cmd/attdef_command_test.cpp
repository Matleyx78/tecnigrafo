// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

class AttdefCommandTest : public ::testing::Test {
protected:
    AttdefCommandTest() : editor(document), runner(registry, editor) {
        registerBuiltinCommands(registry);
        runner.setScriptSequences(true);
        runner.setMessageSink(
            [this](std::string message) { messages.push_back(std::move(message)); });
    }

    void type(std::initializer_list<const char*> inputs) {
        for (const char* input : inputs) {
            runner.submitText(input);
        }
    }

    core::Document document = core::Document::newDrawing();
    edit::Editor editor;
    CommandRegistry registry;
    CommandRunner runner;
    std::vector<std::string> messages;
};

TEST_F(AttdefCommandTest, AflagsValidatesAndSuppliesModes) {
    type({"AFLAGS", "32"});
    EXPECT_EQ(messages.back(), "Requires an integer between 0 and 31.");
    type({"5", "AFLAGS"});
    EXPECT_EQ(runner.promptText(), "Enter new value for AFLAGS <5>:");
    type({"", "-ATTDEF"});
    EXPECT_EQ(messages.back(),
              "Current attribute modes: Invisible=Y Constant=N Verify=Y Preset=N Lock position=N");
    runner.submit(CancelInput{});
}

TEST_F(AttdefCommandTest, CreatesDefinitionAndOneUndoStep) {
    type({"AFLAGS", "0", "-ATTDEF", "", "TAG", "Prompt with spaces", "Default value", "1,2", "3",
          "30"});
    ASSERT_EQ(document.entityCount(), 1U);
    const auto definition = std::get<core::AttributeDefinition>(document.entities().front().entity);
    EXPECT_EQ(definition.tag, "TAG");
    EXPECT_EQ(definition.prompt, "Prompt with spaces");
    EXPECT_EQ(definition.text.value, "Default value");
    EXPECT_EQ(definition.text.position, (core::Vec2{1, 2}));
    EXPECT_DOUBLE_EQ(definition.text.height, 3.0);
    EXPECT_DOUBLE_EQ(definition.text.rotation, core::degreesToRadians(30.0));
    EXPECT_EQ(editor.undoName(), "Attdef");
}

TEST_F(AttdefCommandTest, EmptyAndInvalidTagsRepeatPrompt) {
    type({"-ATTDEF", "", ""});
    EXPECT_EQ(messages.back(), "Tag cannot be null.");
    runner.submitText("BAD TAG");
    EXPECT_EQ(messages.back(), "Invalid tag name.");
    EXPECT_EQ(runner.promptText(), "Enter attribute tag name:");
}

TEST_F(AttdefCommandTest, ConstantSkipsPromptAndMcSetsAlignment) {
    type({"AFLAGS", "0", "-ATTDEF", "C", "", "CONST", "value", "J", "MC", "4,5", "2.5", "10"});
    const auto definition = std::get<core::AttributeDefinition>(document.entities().front().entity);
    EXPECT_TRUE(definition.flags.constant);
    EXPECT_TRUE(definition.prompt.empty());
    EXPECT_EQ(definition.text.horizontal, core::TextHorizontal::Center);
    EXPECT_EQ(definition.text.vertical, core::TextVertical::Middle);
}

TEST_F(AttdefCommandTest, EscapeRollsBackModesAndStyle) {
    auto styles = document.textStyles();
    core::TextStyle arial;
    arial.name = "ARIAL";
    arial.font = "arial.ttf";
    styles.put(arial, document);
    document.replaceTextStyles(std::move(styles));
    type({"AFLAGS", "0", "-ATTDEF", "V", "", "TAG", "Prompt", "Value", "S", "ARIAL"});
    runner.submit(CancelInput{});
    EXPECT_TRUE(document.isEmpty());
    EXPECT_EQ(document.textStyles().current(), "Standard");
    type({"AFLAGS"});
    EXPECT_EQ(runner.promptText(), "Enter new value for AFLAGS <0>:");
}

}  // namespace
}  // namespace tg::cmd
