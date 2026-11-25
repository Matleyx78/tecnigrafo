// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/blocks.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/line.hpp"
#include "tg/edit/editor.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <set>
#include <string>
#include <utility>
#include <vector>

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

core::AttributeDefinition attdef(const std::string& tag, const std::string& value) {
    core::AttributeDefinition definition;
    definition.tag = tag;
    definition.prompt = tag + "?";
    definition.text.value = value;
    return definition;
}

const core::AttributeDefinition& attdefAt(const core::BlockDefinition& block, std::size_t i) {
    return std::get<core::AttributeDefinition>(block.entities[i].entity);
}

/// PT16, [bloccante] 6 of the PT5 review: ATTDEF A at order 10, an
/// unhandled entity at 20, ATTDEF B at 30.
core::Document partDocument() {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition part{"Part"};
    part.recordId = 0x2A0;
    part.hasAttributes = true;
    core::EntityRecord first{core::EntityId{0x2A1}, attdef("A", "one"), {}, 10};
    first.properties = document.current();
    core::EntityRecord second{core::EntityId{0x2A3}, attdef("B", "two"), {}, 30};
    second.properties = document.current();
    second.extras.extended = {{1001, "APP"}, {1000, "kept"}};
    part.entities = {first, second};
    part.rawEntities.push_back(
        core::RawEntity{core::EntityId{0x2A2},
                        20,
                        "ACAD_PROXY_ENTITY",
                        {{5, "2A2"}, {330, "2A0"}, {100, "AcDbEntity"}, {8, "0"}, {90, "498"}}});
    document.replaceBlock("Part", part);
    return document;
}

const core::BlockDefinition& partOf(const core::Document& document) {
    return *document.blocks().find("Part");
}

TEST(AttributeDefinitionsTest, ReorderSwapsTheOrderKeysAndWritesThemSoInDxf) {
    core::Document document = partDocument();
    const core::BlockDefinition original = partOf(document);
    Editor editor(document);
    editor.beginTransaction("Reorder");
    const std::vector<std::string> tags{"b", "A"};
    ASSERT_TRUE(reorderAttributeDefinitions(editor, "PART", tags));
    editor.commit();

    ASSERT_EQ(partOf(document).entities.size(), 2U);
    EXPECT_EQ(partOf(document).entities[0].id.value, 0x2A3U);
    EXPECT_EQ(partOf(document).entities[0].order, 10U);
    EXPECT_EQ(partOf(document).entities[0].extras.extended, original.entities[1].extras.extended);
    EXPECT_EQ(attdefAt(partOf(document), 0).tag, "B");
    EXPECT_EQ(partOf(document).entities[1].id.value, 0x2A1U);
    EXPECT_EQ(partOf(document).entities[1].order, 30U);
    EXPECT_EQ(partOf(document).rawEntities, original.rawEntities);

    const auto written = io::writeDxf(document, io::DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    const std::string& dxf = written.value();
    // The writer ends its lines with CR LF.
    const std::size_t b = dxf.find("\n2A3\r\n");
    const std::size_t raw = dxf.find("\n2A2\r\n");
    const std::size_t a = dxf.find("\n2A1\r\n");
    ASSERT_NE(b, std::string::npos);
    ASSERT_NE(raw, std::string::npos);
    ASSERT_NE(a, std::string::npos);
    EXPECT_LT(b, raw);
    EXPECT_LT(raw, a);

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(partOf(document), original);
}

TEST(AttributeDefinitionsTest, ReorderRefusesWhatIsNotAPermutation) {
    core::Document document = partDocument();
    const core::BlockDefinition original = partOf(document);
    Editor editor(document);
    editor.beginTransaction("Reorder");
    const std::vector<std::string> missing{"A"};
    const std::vector<std::string> twice{"A", "a"};
    const std::vector<std::string> unknown{"A", "C"};
    const std::vector<std::string> tags{"B", "A"};
    EXPECT_FALSE(reorderAttributeDefinitions(editor, "Part", missing));
    EXPECT_FALSE(reorderAttributeDefinitions(editor, "Part", twice));
    EXPECT_FALSE(reorderAttributeDefinitions(editor, "Part", unknown));
    EXPECT_FALSE(reorderAttributeDefinitions(editor, "Other", tags));
    editor.commit();
    EXPECT_EQ(partOf(document), original);
    EXPECT_TRUE(editor.undoNames().empty());
}

TEST(AttributeDefinitionsTest, ReplaceKeepsTheRecordAndRefusesATagOfAnother) {
    core::Document document = partDocument();
    const core::BlockDefinition original = partOf(document);
    Editor editor(document);
    editor.beginTransaction("Edit");
    EXPECT_FALSE(replaceAttributeDefinition(editor, "Part", "C", attdef("C", "x")));
    EXPECT_FALSE(replaceAttributeDefinition(editor, "Other", "A", attdef("A", "x")));
    // The new tag equals that of B, without case.
    EXPECT_FALSE(replaceAttributeDefinition(editor, "Part", "A", attdef("b", "x")));
    EXPECT_EQ(partOf(document), original);
    ASSERT_TRUE(replaceAttributeDefinition(editor, "Part", "b", attdef("NAME", "three")));
    editor.commit();

    EXPECT_EQ(attdefAt(partOf(document), 1), attdef("NAME", "three"));
    EXPECT_EQ(partOf(document).entities[1].id, original.entities[1].id);
    EXPECT_EQ(partOf(document).entities[1].order, original.entities[1].order);
    EXPECT_EQ(partOf(document).entities[1].extras, original.entities[1].extras);
    EXPECT_EQ(partOf(document).entities[1].properties, original.entities[1].properties);
    EXPECT_EQ(partOf(document).entities[0], original.entities[0]);

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(partOf(document), original);
}

TEST(AttributeDefinitionsTest, RemovingTheLastAttdefTurnsHasAttributesOff) {
    core::Document document = partDocument();
    const core::BlockDefinition original = partOf(document);
    Editor editor(document);
    editor.beginTransaction("Remove");
    EXPECT_FALSE(removeAttributeDefinition(editor, "Part", "C"));
    ASSERT_TRUE(removeAttributeDefinition(editor, "Part", "a"));
    editor.commit();
    ASSERT_EQ(partOf(document).entities.size(), 1U);
    EXPECT_EQ(attdefAt(partOf(document), 0).tag, "B");
    EXPECT_TRUE(partOf(document).hasAttributes);

    editor.beginTransaction("Remove");
    ASSERT_TRUE(removeAttributeDefinition(editor, "Part", "B"));
    editor.commit();
    EXPECT_TRUE(partOf(document).entities.empty());
    EXPECT_FALSE(partOf(document).hasAttributes);
    EXPECT_EQ(partOf(document).rawEntities, original.rawEntities);

    ASSERT_TRUE(editor.undo());
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(partOf(document), original);
}

core::Attribute attrib(std::uint64_t id, const std::string& tag, const std::string& value) {
    core::Attribute attribute;
    attribute.id = core::EntityId{id};
    attribute.tag = tag;
    attribute.text.value = value;
    return attribute;
}

/// [bloccante] 12 of the PT5 review: an INSERT A in the drawing and one in
/// the definition of B, which the drawing inserts twice.
TEST(SynchronizeInsertsTest, UpdatesEachRecordOnceWithNewHandlesAndOneUndo) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition a{"A"};
    a.hasAttributes = true;
    a.entities.push_back({core::EntityId{0x300}, attdef("P", "p0"), {}, 1});
    a.entities.push_back({core::EntityId{0x301}, attdef("Q", "q0"), {}, 2});
    document.replaceBlock("A", a);
    core::BlockDefinition b{"B"};
    core::Insert nested{"A", {5, 0}, 1, 1, 0, {attrib(0x311, "q", "nested")}};
    b.entities.push_back({core::EntityId{0x310}, nested, {}, 1});
    b.entities.push_back({core::EntityId{0x312}, core::Line{{0, 0}, {1, 0}}, {}, 2});
    document.replaceBlock("B", b);
    document.reserveHandle(core::EntityId{0x320});
    Editor editor(document);
    editor.beginTransaction("Inserts");
    const core::EntityId direct =
        editor.add(core::Insert{"A", {0, 0}, 1, 1, 0, {attrib(0x320, "Q", "direct")}});
    const core::EntityId outer1 = editor.add(core::Insert{"B", {10, 0}, 1, 1, 0, {}});
    const core::EntityId outer2 = editor.add(core::Insert{"B", {20, 0}, 1, 1, 0, {}});
    editor.commit();
    const core::Document before = document;
    const std::uint64_t next = document.nextId();

    editor.beginTransaction("Synchronize");
    EXPECT_EQ(synchronizeInserts(editor, "a"), 2U);
    editor.commit();

    const auto& synced = std::get<core::Insert>(document.find(direct)->entity);
    ASSERT_EQ(synced.attributes.size(), 2U);
    EXPECT_EQ(synced.attributes[0].tag, "P");
    EXPECT_EQ(synced.attributes[0].text.value, "p0");
    EXPECT_EQ(synced.attributes[1].tag, "Q");
    EXPECT_EQ(synced.attributes[1].text.value, "direct");
    EXPECT_EQ(synced.attributes[1].id.value, 0x320U);
    const auto& inner = std::get<core::Insert>(document.blocks().find("B")->entities[0].entity);
    ASSERT_EQ(inner.attributes.size(), 2U);
    EXPECT_EQ(inner.attributes[0].tag, "P");
    EXPECT_EQ(inner.attributes[1].tag, "Q");
    EXPECT_EQ(inner.attributes[1].text.value, "nested");
    EXPECT_EQ(inner.attributes[1].id.value, 0x311U);
    // The new ATTRIBs take new handles, one each, in handle order of the
    // records: the one in B (0x310) before the one in the drawing.
    EXPECT_EQ(inner.attributes[0].id.value, next);
    EXPECT_EQ(synced.attributes[0].id.value, next + 1);
    EXPECT_GE(document.nextId(), next + 2);
    EXPECT_EQ(document.blocks().find("B")->entities[1], before.blocks().find("B")->entities[1]);
    EXPECT_EQ(*document.find(outer1), *before.find(outer1));
    EXPECT_EQ(*document.find(outer2), *before.find(outer2));

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(*document.find(direct), *before.find(direct));
    EXPECT_EQ(*document.blocks().find("B"), *before.blocks().find("B"));
    EXPECT_EQ(editor.undoName(), "Inserts");
    // Handles are never given again, even after undo.
    editor.beginTransaction("Line");
    EXPECT_GE(editor.add(core::Line{{0, 0}, {1, 0}}).value, next + 2);
    editor.commit();
}

TEST(SynchronizeInsertsTest, ReordersAndDropsAndCountsOnlyTheRecordsChanged) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition a{"A"};
    a.hasAttributes = true;
    a.entities.push_back({core::EntityId{0x300}, attdef("P", "p0"), {}, 1});
    a.entities.push_back({core::EntityId{0x301}, attdef("Q", "q0"), {}, 2});
    document.replaceBlock("A", a);
    document.reserveHandle(core::EntityId{0x331});
    Editor editor(document);
    editor.beginTransaction("Inserts");
    const core::EntityId fresh = addInsert(editor, core::Insert{"A", {}, 1, 1, 0, {}});
    const core::EntityId old = editor.add(
        core::Insert{"A", {}, 1, 1, 0, {attrib(0x330, "GONE", "x"), attrib(0x331, "q", "v")}});
    editor.commit();

    // Swap P and Q in the definition: both records follow its order.
    editor.beginTransaction("Reorder");
    const std::vector<std::string> tags{"Q", "P"};
    ASSERT_TRUE(reorderAttributeDefinitions(editor, "A", tags));
    EXPECT_EQ(synchronizeInserts(editor, "A"), 2U);
    editor.commit();
    const auto& first = std::get<core::Insert>(document.find(fresh)->entity);
    ASSERT_EQ(first.attributes.size(), 2U);
    EXPECT_EQ(first.attributes[0].tag, "Q");
    EXPECT_EQ(first.attributes[1].tag, "P");
    const auto& second = std::get<core::Insert>(document.find(old)->entity);
    ASSERT_EQ(second.attributes.size(), 2U);
    EXPECT_EQ(second.attributes[0].id.value, 0x331U);
    EXPECT_EQ(second.attributes[0].tag, "Q");
    EXPECT_EQ(second.attributes[0].text.value, "v");
    EXPECT_EQ(second.attributes[1].tag, "P");

    // In step with the definition: nothing to change.
    editor.beginTransaction("Again");
    EXPECT_EQ(synchronizeInserts(editor, "A"), 0U);
    EXPECT_EQ(synchronizeInserts(editor, "Missing"), 0U);
    editor.commit();
    EXPECT_EQ(editor.undoName(), "Reorder");
}

/// [decisione] 1 of the PT5 review: ATTRIBs A and a with one ATTDEF A.
TEST(SynchronizeInsertsTest, EqualTagsKeepTheFirstAttribOnly) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition block{"T"};
    block.hasAttributes = true;
    block.entities.push_back({core::EntityId{0x300}, attdef("A", "default"), {}, 1});
    document.replaceBlock("T", block);
    Editor editor(document);
    editor.beginTransaction("Insert");
    core::Attribute first = attrib(31, "A", "first");
    first.extras.extended = {{1001, "APP"}, {1000, "kept"}};
    const core::EntityId id =
        editor.add(core::Insert{"T", {}, 1, 1, 0, {first, attrib(32, "a", "second")}});
    editor.commit();

    editor.beginTransaction("Synchronize");
    EXPECT_EQ(synchronizeInserts(editor, "T"), 1U);
    editor.commit();
    const auto& insert = std::get<core::Insert>(document.find(id)->entity);
    ASSERT_EQ(insert.attributes.size(), 1U);
    EXPECT_EQ(insert.attributes[0].id.value, 31U);
    EXPECT_EQ(insert.attributes[0].text.value, "first");
    EXPECT_EQ(insert.attributes[0].extras.extended, first.extras.extended);
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(std::get<core::Insert>(document.find(id)->entity).attributes.size(), 2U);
}

}  // namespace
}  // namespace tg::edit
