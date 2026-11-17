// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/blocks.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/line.hpp"
#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <array>

namespace tg::edit {
namespace {

TEST(BlocksTest, DefinesAndUndoesWithOriginalRecordsAndAttributeFlag) {
    core::Document document = core::Document::newDrawing();
    Editor editor(document);
    editor.beginTransaction("Entities");
    const core::EntityId line = editor.add(core::Line{{0, 0}, {4, 0}});
    core::AttributeDefinition attribute;
    attribute.text.position = {1, 2};
    attribute.text.value = "value";
    attribute.tag = "TAG";
    const core::EntityId attdef = editor.add(attribute);
    editor.commit();

    editor.beginTransaction("Block");
    const std::array ids{line, attdef};
    ASSERT_TRUE(defineBlock(editor, "Part", {3, 4}, ids));
    editor.commit();

    ASSERT_EQ(document.entityCount(), 0U);
    const core::BlockDefinition* block = document.blocks().find("part");
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(block->base, (core::Vec2{3, 4}));
    EXPECT_TRUE(block->hasAttributes);
    ASSERT_EQ(block->entities.size(), 2U);
    EXPECT_EQ(block->entities[0].id, line);
    EXPECT_EQ(block->entities[1].id, attdef);
    EXPECT_EQ(editor.undoName(), "Block");
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.blocks().find("Part"), nullptr);
    EXPECT_NE(document.find(line), nullptr);
    EXPECT_NE(document.find(attdef), nullptr);
}

TEST(BlocksTest, RedefinitionUpdatesInsertExtentAndUndoRestoresIt) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition original{"Part"};
    original.entities.push_back({core::EntityId{100}, core::Line{{0, 0}, {1, 0}}});
    document.replaceBlock("Part", original);
    Editor editor(document);
    editor.beginTransaction("Insert");
    core::Insert insert;
    insert.block = "Part";
    const core::EntityId insertId = editor.add(insert);
    const core::EntityId replacement = editor.add(core::Line{{0, 0}, {20, 0}});
    editor.commit();
    const core::BBox before = core::extentOf(document, *document.find(insertId), insert);

    editor.beginTransaction("Block");
    ASSERT_TRUE(defineBlock(editor, "part", {0, 0}, std::span{&replacement, 1U}));
    editor.commit();
    const auto* changed = std::get_if<core::Insert>(&document.find(insertId)->entity);
    ASSERT_NE(changed, nullptr);
    EXPECT_GT(core::extentOf(document, *document.find(insertId), *changed).max().x, before.max().x);
    ASSERT_TRUE(editor.undo());
    const auto* restored = std::get_if<core::Insert>(&document.find(insertId)->entity);
    ASSERT_NE(restored, nullptr);
    const core::BBox afterUndo = core::extentOf(document, *document.find(insertId), *restored);
    EXPECT_EQ(afterUndo.min(), before.min());
    EXPECT_EQ(afterUndo.max(), before.max());
}

TEST(BlocksTest, RedefinitionPreservesDefinitionDataAndUndoRestoresOriginal) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition original{"Part"};
    original.base = {1, 2};
    original.description = "Imported part";
    original.units = 4;
    original.explodable = false;
    original.uniformScale = true;
    original.entities.push_back({core::EntityId{100}, core::Line{{0, 0}, {1, 0}}});
    original.recordId = 101;
    original.blockId = 102;
    original.endBlockId = 103;
    original.recordExtras = {{1001, "APP"}, {1000, "record data"}};
    document.replaceBlock("Part", original);
    Editor editor(document);
    editor.beginTransaction("Line");
    const core::EntityId replacement = editor.add(core::Line{{2, 3}, {8, 9}});
    editor.commit();

    editor.beginTransaction("Block");
    ASSERT_TRUE(defineBlock(editor, "part", {3, 4}, std::span{&replacement, 1U}));
    editor.commit();

    const core::BlockDefinition* redefined = document.blocks().find("Part");
    ASSERT_NE(redefined, nullptr);
    EXPECT_EQ(redefined->name, original.name);
    EXPECT_EQ(redefined->base, (core::Vec2{3, 4}));
    ASSERT_EQ(redefined->entities.size(), 1U);
    EXPECT_EQ(redefined->entities.front().id, replacement);
    EXPECT_EQ(redefined->description, original.description);
    EXPECT_EQ(redefined->units, original.units);
    EXPECT_EQ(redefined->explodable, original.explodable);
    EXPECT_EQ(redefined->uniformScale, original.uniformScale);
    EXPECT_EQ(redefined->recordId, original.recordId);
    EXPECT_EQ(redefined->blockId, original.blockId);
    EXPECT_EQ(redefined->endBlockId, original.endBlockId);
    EXPECT_EQ(redefined->recordExtras, original.recordExtras);

    ASSERT_TRUE(editor.undo());
    ASSERT_NE(document.blocks().find("Part"), nullptr);
    EXPECT_EQ(*document.blocks().find("Part"), original);
    EXPECT_NE(document.find(replacement), nullptr);
}

TEST(BlocksTest, RejectsDirectAndIndirectCyclesWithoutChanges) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition child{"Child"};
    core::Insert parentInsert;
    parentInsert.block = "Parent";
    child.entities.push_back({core::EntityId{100}, parentInsert});
    document.replaceBlock("Child", child);
    Editor editor(document);
    editor.beginTransaction("Insert");
    core::Insert childInsert;
    childInsert.block = "Child";
    const core::EntityId id = editor.add(childInsert);
    editor.commit();

    editor.beginTransaction("Block");
    EXPECT_FALSE(defineBlock(editor, "Parent", {0, 0}, std::span{&id, 1U}));
    editor.rollback();
    EXPECT_NE(document.find(id), nullptr);
    EXPECT_EQ(document.blocks().find("Parent"), nullptr);
}

}  // namespace
}  // namespace tg::edit
