// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/attribute_definition.hpp"
#include "tg/core/insert.hpp"
#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

class AttributeCommandsTest : public ::testing::Test {
public:
    AttributeCommandsTest()
        : document(core::Document::newDrawing()), editor(document), runner(registry, editor) {
        registerBuiltinCommands(registry);
        runner.setMessageSink(
            [this](std::string message) { messages.push_back(std::move(message)); });
    }

    void addAttributeBlock(std::string name = "Labels") {
        core::BlockDefinition block{std::move(name)};
        core::AttributeDefinition definition;
        definition.tag = "CODE";
        definition.text.value = "new";
        definition.text.height = 2.0;
        block.entities.push_back({core::EntityId{100}, definition});
        block.hasAttributes = true;
        document.replaceBlock(block.name, block);
    }

    core::EntityId addInsert(std::string block = "Labels") {
        core::Insert insert;
        insert.block = std::move(block);
        core::Attribute attribute;
        attribute.id = core::EntityId{200};
        attribute.tag = "CODE";
        attribute.text.value = "kept";
        attribute.text.height = 1.0;
        insert.attributes.push_back(attribute);
        editor.beginTransaction("Insert");
        const core::EntityId id = editor.add(insert);
        editor.commit();
        return id;
    }

    bool heard(std::string_view message) const {
        return std::ranges::find(messages, message) != messages.end();
    }

    core::Document document;
    edit::Editor editor;
    CommandRegistry registry;
    CommandRunner runner;
    std::vector<std::string> messages;
};

TEST_F(AttributeCommandsTest, AttsyncWithoutAttributeBlocksFinishesImmediately) {
    runner.submitText("ATTSYNC");
    EXPECT_EQ(runner.promptText(), "Command:");
    EXPECT_TRUE(heard("This drawing contains no blocks with attributes."));
}

TEST_F(AttributeCommandsTest, AttsyncNameSynchronizesAndUndoRestoresInsert) {
    addAttributeBlock();
    const core::EntityId id = addInsert();

    runner.submitText("ATTSYNC");
    runner.submitText("Name");
    runner.submitText("labels");

    const auto& changed = std::get<core::Insert>(document.find(id)->entity);
    ASSERT_EQ(changed.attributes.size(), 1U);
    EXPECT_EQ(changed.attributes[0].text.value, "kept");
    EXPECT_DOUBLE_EQ(changed.attributes[0].text.height, 2.0);
    EXPECT_TRUE(heard("ATTSYNC complete."));
    EXPECT_EQ(editor.undoName(), "Attsync");

    runner.submitText("U");
    const auto& restored = std::get<core::Insert>(document.find(id)->entity);
    EXPECT_DOUBLE_EQ(restored.attributes[0].text.height, 1.0);
}

TEST_F(AttributeCommandsTest, AttsyncRejectsMissingAndAttributeLessBlocks) {
    addAttributeBlock();
    document.replaceBlock("Plain", core::BlockDefinition{"Plain"});

    runner.submitText("ATTSYNC");
    runner.submitText("Name");
    runner.submitText("Plain");
    EXPECT_TRUE(heard("The block table contains no block with attributes named Plain."));

    runner.submitText("ATTSYNC");
    runner.submitText("Name");
    runner.submitText("Missing");
    EXPECT_TRUE(heard("The block table contains no block with attributes named Missing."));
}

TEST_F(AttributeCommandsTest, AttsyncSelectUsesThePickedInsert) {
    addAttributeBlock();
    addInsert();
    runner.submitText("ATTSYNC");
    runner.submitText("");
    runner.submit(PointInput{{0, 0}});
    EXPECT_TRUE(heard("ATTSYNC complete."));
}

TEST_F(AttributeCommandsTest, AttdispSetsEveryValueWithoutUndoAndMarksModified) {
    editor.markSaved();
    const std::vector<std::string> undoBefore = editor.undoNames();

    runner.submitText("ATTDISP");
    runner.submitText("ON");
    EXPECT_EQ(document.attributeDisplay(), core::AttributeDisplay::On);
    EXPECT_TRUE(editor.isModified());
    EXPECT_EQ(editor.undoNames(), undoBefore);
    EXPECT_TRUE(heard("Regenerating model."));

    runner.submitText("ATTDISP");
    runner.submitText("OFF");
    EXPECT_EQ(document.attributeDisplay(), core::AttributeDisplay::Off);
    runner.submitText("ATTDISP");
    runner.submitText("Normal");
    EXPECT_EQ(document.attributeDisplay(), core::AttributeDisplay::Normal);
}

}  // namespace
}  // namespace tg::cmd
