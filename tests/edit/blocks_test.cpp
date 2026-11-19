// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/blocks.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/line.hpp"
#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <utility>

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

TEST(BlocksTest, AddInsertGivesNewHandlesToTheInsertAndItsAttributes) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition part{"Part"};
    core::AttributeDefinition first;
    first.text.value = "A1";
    first.tag = "PIN";
    core::AttributeDefinition second = first;
    second.tag = "NAME";
    second.text.position = {0, 5};
    part.entities.push_back({core::EntityId{100}, first});
    part.entities.push_back({core::EntityId{101}, second});
    document.replaceBlock("Part", part);
    Editor editor(document);
    const std::uint64_t next = document.nextId();

    editor.beginTransaction("Insert");
    core::Insert insert{"Part", {10, 0}, 1, 1, 0, {}};
    // An attribute already there is replaced by those of the definition.
    core::Attribute old;
    old.id = core::EntityId{7};
    old.tag = "OLD";
    insert.attributes.push_back(old);
    const core::EntityId id = addInsert(editor, insert, {{100, "AcDbBlockReference"}, {43, "3"}});
    editor.commit();

    EXPECT_EQ(id.value, next);
    const core::EntityRecord* record = document.find(id);
    ASSERT_NE(record, nullptr);
    const auto& added = std::get<core::Insert>(record->entity);
    ASSERT_EQ(added.attributes.size(), 2U);
    EXPECT_EQ(added.attributes[0].tag, "PIN");
    EXPECT_EQ(added.attributes[0].id.value, next + 1);
    EXPECT_EQ(added.attributes[1].tag, "NAME");
    EXPECT_EQ(added.attributes[1].id.value, next + 2);
    EXPECT_EQ(added.attributes[1].text.position, (core::Vec2{10, 5}));
    EXPECT_EQ(record->extras.specific.back(), (core::RawGroup{43, "3"}));
    EXPECT_EQ(document.nextId(), next + 3);

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.find(id), nullptr);
    ASSERT_TRUE(editor.redo());
    ASSERT_NE(document.find(id), nullptr);
    EXPECT_EQ(std::get<core::Insert>(document.find(id)->entity).attributes[1].id.value, next + 2);
    // Handles are never given again.
    editor.beginTransaction("Line");
    EXPECT_EQ(editor.add(core::Line{{0, 0}, {1, 0}}).value, next + 3);
    editor.commit();
}

/// What a file brings: a linetype, a layer that uses it, a text style, a
/// nested block and the definition, numbered from `first`.
BlockImport sampleImport(const core::Document& document, std::uint64_t first) {
    BlockImport import;
    core::Linetype dashed;
    dashed.id = core::LinetypeId{first};
    dashed.name = "DASHED";
    import.linetypes.push_back(dashed);
    core::Layer layer;
    layer.id = core::LayerId{first + 1};
    layer.name = "IMPORTED";
    layer.color = core::Color::fromIndex(1);
    layer.linetype = core::LinetypeId{first};
    import.layers.push_back(layer);
    core::TextStyle style;
    style.id = first + 2;
    style.name = "LABELS";
    import.textStyles.push_back(style);
    core::BlockDefinition nested{"Pin"};
    nested.entities.push_back({core::EntityId{first + 3}, core::Circle{{0, 0}, 1}});
    import.nested.push_back(nested);
    import.definition.name = "Symbol";
    core::EntityRecord line{core::EntityId{first + 4}, core::Line{{0, 0}, {5, 0}}};
    line.properties = document.current();
    line.properties.layer = core::LayerId{first + 1};
    import.definition.entities.push_back(line);
    import.definition.entities.push_back(
        {core::EntityId{first + 5}, core::Insert{"Pin", {5, 0}, 1, 1, 0, {}}});
    import.definition.units = 4;
    import.nextId = first + 6;
    return import;
}

TEST(BlocksTest, ApplyImportAddsEverythingInOneStepThatUndoTakesBack) {
    core::Document document = core::Document::newDrawing();
    Editor editor(document);
    const std::uint64_t first = document.nextId();
    const core::Document before = document;

    editor.beginTransaction("Insert");
    ASSERT_TRUE(applyImport(editor, sampleImport(document, first), false));
    editor.commit();

    ASSERT_NE(document.findLinetype("DASHED"), nullptr);
    EXPECT_EQ(document.findLinetype("DASHED")->id.value, first);
    ASSERT_NE(document.findLayer("IMPORTED"), nullptr);
    EXPECT_EQ(document.findLayer("IMPORTED")->id.value, first + 1);
    ASSERT_NE(document.textStyles().find("LABELS"), nullptr);
    EXPECT_EQ(document.textStyles().find("LABELS")->id, first + 2);
    ASSERT_NE(document.blocks().find("Pin"), nullptr);
    const core::BlockDefinition* symbol = document.blocks().find("Symbol");
    ASSERT_NE(symbol, nullptr);
    EXPECT_EQ(symbol->entities[0].properties.layer.value, first + 1);
    EXPECT_EQ(document.nextId(), first + 6);
    EXPECT_EQ(editor.undoNames(), (std::vector<std::string>{"Insert"}));

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.blocks(), before.blocks());
    EXPECT_EQ(document.findLayer("IMPORTED"), nullptr);
    EXPECT_EQ(document.findLinetype("DASHED"), nullptr);
    EXPECT_EQ(document.textStyles(), before.textStyles());
    // The counter stays: the handles of the undone import are not reused.
    EXPECT_EQ(document.nextId(), first + 6);
    ASSERT_TRUE(editor.redo());
    EXPECT_NE(document.blocks().find("Symbol"), nullptr);
    EXPECT_NE(document.findLayer("IMPORTED"), nullptr);
}

TEST(BlocksTest, ApplyImportRedefinesKeepingTheRecordsOfTheBlock) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition symbol{"symbol"};
    symbol.recordId = 200;
    symbol.blockId = 201;
    symbol.endBlockId = 202;
    symbol.recordExtras = {{1001, "APP"}};
    symbol.entities.push_back({core::EntityId{203}, core::Line{{0, 0}, {1, 0}}});
    document.replaceBlock("symbol", symbol);
    Editor editor(document);
    const std::uint64_t first = document.nextId();

    editor.beginTransaction("Insert");
    EXPECT_FALSE(applyImport(editor, sampleImport(document, first), false));
    EXPECT_EQ(document.findLayer("IMPORTED"), nullptr);
    ASSERT_TRUE(applyImport(editor, sampleImport(document, first), true));
    editor.commit();

    const core::BlockDefinition* redefined = document.blocks().find("Symbol");
    ASSERT_NE(redefined, nullptr);
    EXPECT_EQ(redefined->name, "symbol");
    EXPECT_EQ(redefined->recordId, 200U);
    EXPECT_EQ(redefined->blockId, 201U);
    EXPECT_EQ(redefined->endBlockId, 202U);
    EXPECT_EQ(redefined->recordExtras, symbol.recordExtras);
    ASSERT_EQ(redefined->entities.size(), 2U);
    EXPECT_EQ(redefined->entities[0].id.value, first + 4);
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(*document.blocks().find("symbol"), symbol);
}

TEST(BlocksTest, ApplyImportRefusesADefinitionThatContainsItself) {
    core::Document document = core::Document::newDrawing();
    Editor editor(document);
    BlockImport import = sampleImport(document, document.nextId());
    import.definition.name = "Pin";
    import.nested.clear();
    import.nested.push_back(core::BlockDefinition{"Other"});
    import.nested.back().entities.push_back(
        {core::EntityId{import.nextId}, core::Insert{"Pin", {}, 1, 1, 0, {}}});
    import.definition.entities.back() = {core::EntityId{import.nextId + 1},
                                         core::Insert{"Other", {}, 1, 1, 0, {}}};
    import.nextId += 2;

    editor.beginTransaction("Insert");
    EXPECT_FALSE(applyImport(editor, import, false));
    EXPECT_EQ(document.findLayer("IMPORTED"), nullptr);
    EXPECT_EQ(document.blocks().find("Other"), nullptr);
    editor.commit();
    EXPECT_TRUE(editor.undoNames().empty());
}

}  // namespace
}  // namespace tg::edit
