// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/line.hpp"
#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

class BlockCommandTest : public ::testing::Test {
public:
    BlockCommandTest()
        : document(core::Document::newDrawing()), editor(document), runner(registry, editor) {
        registerBuiltinCommands(registry);
        runner.setMessageSink(
            [this](std::string message) { messages.push_back(std::move(message)); });
    }

    core::EntityId addLine(core::Vec2 start = {0, 0}, core::Vec2 end = {10, 0}) {
        editor.beginTransaction("Line");
        const core::EntityId id = editor.add(core::Line{start, end});
        editor.commit();
        return id;
    }

    bool heard(const std::string& message) const {
        return std::ranges::find(messages, message) != messages.end();
    }

    core::Document document;
    edit::Editor editor;
    CommandRegistry registry;
    CommandRunner runner;
    std::vector<std::string> messages;
};

TEST_F(BlockCommandTest, CompletesSequenceAndUUndoesDefinition) {
    const core::EntityId id = addLine();
    runner.submitText("BLOCK");
    runner.submitText("Part");
    runner.submitText("2,3");
    runner.submitText("5,0");
    runner.submitText("");
    ASSERT_NE(document.blocks().find("Part"), nullptr);
    EXPECT_EQ(document.find(id), nullptr);
    EXPECT_EQ(editor.undoName(), "Block");
    runner.submitText("U");
    EXPECT_EQ(document.blocks().find("Part"), nullptr);
    EXPECT_NE(document.find(id), nullptr);
}

TEST_F(BlockCommandTest, RedefineNoAndEmptySelectionDoNotChangeHistory) {
    core::BlockDefinition block{"Part"};
    document.replaceBlock("Part", block);
    const core::EntityId id = addLine();
    const std::vector<std::string> undo = editor.undoNames();
    runner.submitText("-BLOCK");
    runner.submitText("Part");
    runner.submitText("No");
    EXPECT_EQ(editor.undoNames(), undo);
    EXPECT_NE(document.find(id), nullptr);

    runner.submitText("-BLOCK");
    runner.submitText("Empty");
    runner.submitText("0,0");
    runner.submitText("");
    EXPECT_TRUE(heard("No objects selected."));
    EXPECT_EQ(document.blocks().find("Empty"), nullptr);
    EXPECT_EQ(editor.undoNames(), undo);
}

TEST_F(BlockCommandTest, QuestionListsBlocksAndInvalidNameRepeatsPrompt) {
    document.replaceBlock("Door", core::BlockDefinition{"Door"});
    runner.submitText("-BLOCK");
    runner.submitText("?");
    EXPECT_TRUE(heard("Door"));
    EXPECT_EQ(runner.promptText(), "Enter block name or [?]:");
    runner.submitText("Bad<Name");
    EXPECT_TRUE(heard("Invalid block name."));
    EXPECT_EQ(runner.promptText(), "Enter block name or [?]:");
}

}  // namespace
}  // namespace tg::cmd
