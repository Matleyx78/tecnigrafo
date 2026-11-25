// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// INSERT with ATTRIB and SEQEND, ATTDEF (docs/tappe/0.8,
// PT18-blocchi-dxf.md, "Test richiesti").

#include "dxf_entities.hpp"
#include "tg/core/entity_properties.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/math.hpp"
#include "tg/edit/editor.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace tg::io {
namespace {

using Groups = core::RawGroups;

void append(Groups& out, const Groups& more) {
    out.insert(out.end(), more.begin(), more.end());
}

/// A drawing with the block TAGGED (record 40), which holds `definition`,
/// and the entities `entities`.
std::string drawing(const Groups& definition, const Groups& entities) {
    Groups out{{0, "SECTION"},
               {2, "HEADER"},
               {9, "$ACADVER"},
               {1, "AC1032"},
               {9, "$DWGCODEPAGE"},
               {3, "ANSI_1252"},
               {9, "$HANDSEED"},
               {5, "1000"},
               {0, "ENDSEC"},
               {0, "SECTION"},
               {2, "TABLES"},
               {0, "TABLE"},
               {2, "BLOCK_RECORD"},
               {5, "3"},
               {100, "AcDbSymbolTable"},
               {70, "2"}};
    for (const auto& [handle, name] :
         {std::pair{"20", "*Model_Space"}, std::pair{"40", "TAGGED"}}) {
        append(out, {{0, "BLOCK_RECORD"},
                     {5, handle},
                     {330, "3"},
                     {100, "AcDbSymbolTableRecord"},
                     {100, "AcDbBlockTableRecord"},
                     {2, name},
                     {70, "     0"},
                     {280, "     1"},
                     {281, "     0"}});
    }
    append(out, {{0, "ENDTAB"},
                 {0, "ENDSEC"},
                 {0, "SECTION"},
                 {2, "BLOCKS"},
                 {0, "BLOCK"},
                 {5, "21"},
                 {330, "20"},
                 {100, "AcDbEntity"},
                 {8, "0"},
                 {100, "AcDbBlockBegin"},
                 {2, "*Model_Space"},
                 {70, "0"},
                 {10, "0.0"},
                 {20, "0.0"},
                 {30, "0.0"},
                 {3, "*Model_Space"},
                 {1, ""},
                 {0, "ENDBLK"},
                 {5, "22"},
                 {330, "20"},
                 {100, "AcDbEntity"},
                 {8, "0"},
                 {100, "AcDbBlockEnd"},
                 {0, "BLOCK"},
                 {5, "41"},
                 {330, "40"},
                 {100, "AcDbEntity"},
                 {8, "0"},
                 {100, "AcDbBlockBegin"},
                 {2, "TAGGED"},
                 {70, "     2"},
                 {10, "0.0"},
                 {20, "0.0"},
                 {30, "0.0"},
                 {3, "TAGGED"},
                 {1, ""}});
    append(out, definition);
    append(out, {{0, "ENDBLK"},
                 {5, "42"},
                 {330, "40"},
                 {100, "AcDbEntity"},
                 {8, "0"},
                 {100, "AcDbBlockEnd"},
                 {0, "ENDSEC"},
                 {0, "SECTION"},
                 {2, "ENTITIES"}});
    append(out, entities);
    append(out, {{0, "ENDSEC"}, {0, "EOF"}});
    return writeGroups(out, TextEncoding::Utf8);
}

/// An ATTDEF as AutoCAD writes it: only the groups that differ from their
/// default value.
Groups attdef(std::string_view handle, std::string_view owner, std::string_view tag, int flags) {
    return {{0, "ATTDEF"},
            {5, std::string{handle}},
            {330, std::string{owner}},
            {100, "AcDbEntity"},
            {8, "0"},
            {100, "AcDbText"},
            {10, "2.0"},
            {20, "4.0"},
            {30, "0.0"},
            {40, "2.5"},
            {1, "DEFAULT"},
            {100, "AcDbAttributeDefinition"},
            {280, "     0"},
            {3, "Prompt"},
            {2, std::string{tag}},
            {70, "     " + std::to_string(flags)},
            {280, "     0"}};
}

Groups insert(std::string_view handle, std::string_view name, bool attributes) {
    Groups out{{0, "INSERT"}, {5, std::string{handle}},   {330, "20"}, {100, "AcDbEntity"},
               {8, "0"},      {100, "AcDbBlockReference"}};
    if (attributes) {
        out.push_back({66, "     1"});
    }
    append(out, {{2, std::string{name}}, {10, "10.0"}, {20, "20.0"}, {30, "0.0"}});
    return out;
}

/// An ATTRIB with every group of TEXT, as other programs write it.
Groups attrib(std::string_view handle, std::string_view owner, std::string_view tag,
              std::string_view value, const Groups& common = {}) {
    Groups out{{0, "ATTRIB"},
               {5, std::string{handle}},
               {330, std::string{owner}},
               {100, "AcDbEntity"},
               {8, "0"}};
    append(out, common);
    append(out, {{100, "AcDbText"}, {10, "12"},
                 {20, "24"},        {30, "0.000"},
                 {40, "2.5"},       {1, std::string{value}},
                 {50, "0"},         {41, "1"},
                 {51, "0"},         {7, "Standard"},
                 {71, "0"},         {72, "0"},
                 {11, "0"},         {21, "0"},
                 {31, "0"},         {100, "AcDbAttribute"},
                 {280, "0"},        {2, std::string{tag}},
                 {70, "0"},         {73, "0"},
                 {74, "0"},         {280, "1"}});
    return out;
}

Groups seqend(std::string_view handle, std::string_view owner) {
    return {{0, "SEQEND"},
            {5, std::string{handle}},
            {330, std::string{owner}},
            {100, "AcDbEntity"},
            {8, "0"},
            {1001, "APP"},
            {1000, "kept"}};
}

Groups groupsOf(std::string_view text) {
    auto groups = readGroups(text);
    return groups.ok() ? groups.value().groups : Groups{};
}

Groups rewritten(const core::Document& document) {
    auto written = writeDxf(document, DxfVersion::R2018);
    return written.ok() ? groupsOf(written.value()) : Groups{};
}

TEST(DxfInsert, TwoAttributesAndTheirSeqendAreReadAndWrittenBack) {
    Groups entities = insert("100", "TAGGED", true);
    append(entities, attrib("101", "100", "REF", "-K1", {{62, "     1"}, {48, "2"}, {370, "50"}}));
    append(entities, attrib("102", "100", "CODE", "X9"));
    append(entities, seqend("103", "100"));
    const std::string text = drawing(attdef("43", "40", "REF", 0), entities);

    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    ASSERT_EQ(document.entityCount(), 1U);
    const auto& record = document.entities()[0];
    const auto& insert = std::get<core::Insert>(record.entity);
    EXPECT_EQ(insert.block, "TAGGED");
    EXPECT_EQ(insert.insertion, (core::Vec2{10, 20}));
    ASSERT_EQ(insert.attributes.size(), 2U);
    EXPECT_EQ(insert.attributes[0].id.value, 0x101U);
    EXPECT_EQ(insert.attributes[0].tag, "REF");
    EXPECT_EQ(insert.attributes[0].text.value, "-K1");
    EXPECT_EQ(insert.attributes[0].text.position, (core::Vec2{12, 24}));
    EXPECT_EQ(insert.attributes[0].properties.color, core::Color::fromIndex(1));
    EXPECT_EQ(insert.attributes[0].properties.linetypeScale, 2.0);
    EXPECT_EQ(insert.attributes[0].properties.lineweight, 50);
    EXPECT_EQ(insert.attributes[1].id.value, 0x102U);
    // The SEQEND is no entity, and its handle is never given again.
    EXPECT_EQ(document.find(core::EntityId{0x103}), nullptr);
    EXPECT_TRUE(document.rawEntities().empty());
    EXPECT_GT(document.nextId(), 0x103U);
    EXPECT_TRUE(read.value().report.unhandled.empty());

    EXPECT_EQ(rewritten(document), groupsOf(text));
}

TEST(DxfInsert, ACopyGetsNewHandlesForItsAttributesAndSeqend) {
    Groups entities = insert("100", "TAGGED", true);
    append(entities, attrib("101", "100", "REF", "-K1"));
    append(entities, seqend("103", "100"));
    auto read = readDxf(drawing(attdef("43", "40", "REF", 0), entities));
    ASSERT_TRUE(read.ok());
    auto& document = read.value().document;
    const auto original = document.entities()[0];
    core::EntityRecord copy = original;
    copy.entity = core::insertForCopy(std::get<core::Insert>(original.entity));
    copy.id = core::EntityId{document.nextId()};
    copy.order = original.order + 1;
    document.insert(copy);

    const auto groups = rewritten(document);
    std::vector<std::uint64_t> handles;
    std::vector<std::pair<std::string, std::string>> owned;  // type, owner
    std::string type;
    for (const auto& group : groups) {
        if (group.code == 0) {
            type = group.value;
        } else if (group.code == 5) {
            handles.push_back(parseHandle(group.value).value_or(0));
        } else if (group.code == 330 && (type == "ATTRIB" || type == "SEQEND")) {
            owned.emplace_back(type, group.value);
        }
    }
    std::ranges::sort(handles);
    EXPECT_EQ(std::ranges::adjacent_find(handles), handles.end());
    const std::string copyHandle = handleText(copy.id.value);
    const std::vector<std::pair<std::string, std::string>> expected{
        {"ATTRIB", "100"}, {"SEQEND", "100"}, {"ATTRIB", copyHandle}, {"SEQEND", copyHandle}};
    EXPECT_EQ(owned, expected);

    const auto again = readDxf(writeDxf(document, DxfVersion::R2018).value());
    ASSERT_TRUE(again.ok());
    ASSERT_EQ(again.value().document.entityCount(), 2U);
    EXPECT_EQ(std::get<core::Insert>(again.value().document.entities()[1].entity).attributes.size(),
              1U);
}

TEST(DxfInsert, MinsertAndAMissingBlockStayUnhandledWithTheirAttributes) {
    Groups entities = insert("100", "TAGGED", false);
    append(entities, {{70, "2"}, {71, "1"}, {44, "5"}});
    append(entities, insert("101", "MISSING", true));
    append(entities, attrib("102", "101", "REF", "-K1"));
    append(entities, seqend("103", "101"));
    const std::string text = drawing({}, entities);

    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    EXPECT_EQ(document.entityCount(), 0U);
    ASSERT_EQ(document.rawEntities().size(), 2U);
    const auto& missing = document.rawEntities()[1];
    EXPECT_EQ(missing.id.value, 0x101U);
    // The INSERT stays unhandled as a whole (docs/adr/0044, point 4).
    EXPECT_NE(std::ranges::find(missing.groups, core::RawGroup{0, "ATTRIB"}), missing.groups.end());
    EXPECT_NE(std::ranges::find(missing.groups, core::RawGroup{0, "SEQEND"}), missing.groups.end());
    EXPECT_EQ(read.value().report.unhandled.at("INSERT"), 2);
    EXPECT_EQ(read.value().report.unhandled.size(), 1U);
    EXPECT_GT(document.nextId(), 0x103U);

    EXPECT_EQ(rewritten(document), groupsOf(text));
}

TEST(DxfInsert, AGroup66ThatDoesNotSayWhatFollowsStaysUnhandled) {
    // 66 = 1 with no sequence after it, and attributes with no 66: the writer
    // would give the INSERT the group 66 of what follows it, so both stay
    // as read (docs/adr/0018).
    Groups entities = insert("100", "TAGGED", true);
    append(entities, insert("101", "TAGGED", false));
    append(entities, attrib("102", "101", "REF", "-K1"));
    append(entities, seqend("103", "101"));
    const std::string text = drawing(attdef("43", "40", "REF", 0), entities);

    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    EXPECT_EQ(document.entityCount(), 0U);
    ASSERT_EQ(document.rawEntities().size(), 2U);
    EXPECT_EQ(document.rawEntities()[0].id.value, 0x100U);
    EXPECT_EQ(document.rawEntities()[1].id.value, 0x101U);
    EXPECT_EQ(read.value().report.unhandled.at("INSERT"), 2);
    EXPECT_GT(document.nextId(), 0x103U);

    EXPECT_EQ(rewritten(document), groupsOf(text));
}

TEST(DxfInsert, AHandleAfterASubclassMarkerIsTheHandleOfItsEntity) {
    // Found by the fuzzer: the reader takes the group 5 wherever it is, so an
    // entity whose handle follows a marker has no other, and the item of an
    // INSERT whose ATTRIB has one there spends it too.
    Groups entities{{0, "LWPOLYLINE"}, {100, "AcDbEntity"}, {8, "0"}, {5, "7"}, {90, "0"}};
    append(entities, insert("100", "TAGGED", true));
    Groups late = attrib("102", "100", "REF", "-K1");
    std::rotate(late.begin() + 1, late.begin() + 2, late.begin() + 5);  // 5 after the marker
    append(entities, late);
    append(entities, seqend("103", "100"));
    const std::string text = drawing({}, entities);

    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    EXPECT_EQ(document.entityCount(), 0U);
    ASSERT_EQ(document.rawEntities().size(), 2U);
    EXPECT_EQ(document.rawEntities()[0].id.value, 0x7U);
    EXPECT_EQ(document.rawEntities()[1].id.value, 0x100U);
    EXPECT_GT(document.nextId(), 0x103U);

    EXPECT_EQ(rewritten(document), groupsOf(text));
}

TEST(DxfInsert, AConstantInvisibleAttdefKeepsItsFlagsAndGroups) {
    // A pin of the electrical symbols: constant and invisible (70 = 3).
    const Groups pin = attdef("100", "20", "TG_PIN1", 3);
    const std::string text = drawing({}, pin);
    auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    auto& document = read.value().document;
    ASSERT_EQ(document.entityCount(), 1U);
    const auto& definition = std::get<core::AttributeDefinition>(document.entities()[0].entity);
    EXPECT_EQ(definition.tag, "TG_PIN1");
    EXPECT_EQ(definition.prompt, "Prompt");
    EXPECT_EQ(definition.text.value, "DEFAULT");
    EXPECT_EQ(definition.text.position, (core::Vec2{2, 4}));
    EXPECT_TRUE(definition.flags.constant);
    EXPECT_TRUE(definition.flags.invisible);
    EXPECT_FALSE(definition.flags.verify);
    EXPECT_FALSE(definition.flags.preset);
    EXPECT_EQ(rewritten(document), groupsOf(text));

    // A changed definition keeps the groups it was read with, in their order.
    core::AttributeDefinition moved = definition;
    moved.text.position = {7, 4};
    moved.text.rotation = core::degreesToRadians(90.0);
    moved.flags.invisible = false;
    document.replaceEntity(document.entities()[0].id, moved);
    const auto groups = rewritten(document);
    Groups expected = groupsOf(text);
    const auto replace = [&expected](const core::RawGroup& from, const core::RawGroup& to) {
        *std::ranges::find(expected, from) = to;
    };
    replace({10, "2.0"}, {10, "7.0"});
    replace({70, "     3"}, {70, "2"});
    const auto value = std::ranges::find(expected, core::RawGroup{1, "DEFAULT"});
    expected.insert(value + 1, {50, formatNumber(core::radiansToDegrees(moved.text.rotation))});
    EXPECT_EQ(groups, expected);
}

TEST(DxfInsert, AMultilineAttdefStaysUnhandled) {
    Groups multiline = attdef("100", "20", "A", 0);
    append(multiline, {{101, "Embedded Object"}, {10, "0"}, {20, "0"}, {1, "A"}});
    const auto read = readDxf(drawing({}, multiline));
    ASSERT_TRUE(read.ok());
    EXPECT_EQ(read.value().document.entityCount(), 0U);
    EXPECT_EQ(read.value().report.unhandled.at("ATTDEF"), 1);
}

/// Reads `entities`, changes the scale Z of its insert to 2 with
/// Editor::replace, and checks that the file saved has the groups of
/// `expected` and reads back with the scale Z 2 and every other group intact
/// (docs/tappe/0.8.1/PT24-proprieta-testi-inserimenti.md, "Test richiesti";
/// #927, [bloccante] 6).
void expectScaleZSavedAndReadBack(const Groups& entities, const Groups& expected) {
    auto read = readDxf(drawing({}, entities));
    ASSERT_TRUE(read.ok());
    auto& document = read.value().document;
    ASSERT_EQ(document.entityCount(), 1U);
    const core::EntityRecord record = document.entities()[0];

    const core::PropertyChange change = core::withOwnProperty(document, record, "scale-z", 2.0);
    ASSERT_TRUE(change.entity);
    ASSERT_TRUE(change.extras);
    edit::Editor editor(document);
    editor.beginTransaction("Properties");
    ASSERT_TRUE(editor.replace(record.id, change.entity.value_or(core::Entity{}),
                               change.extras.value_or(core::EntityExtras{})));
    editor.commit();

    const Groups saved = rewritten(document);
    EXPECT_EQ(saved, groupsOf(drawing({}, expected)));

    const auto again = readDxf(writeDxf(document, DxfVersion::R2018).value());
    ASSERT_TRUE(again.ok());
    const auto& reread = again.value().document;
    ASSERT_EQ(reread.entityCount(), 1U);
    const auto properties = core::ownProperties(reread, reread.entities()[0]);
    const auto found =
        std::ranges::find(properties, std::string{"scale-z"}, &core::OwnProperty::name);
    ASSERT_NE(found, properties.end());
    EXPECT_EQ(std::get<double>(found->value), 2.0);
    const auto& insert = std::get<core::Insert>(reread.entities()[0].entity);
    EXPECT_EQ(insert.scaleX, 2.0);
    EXPECT_EQ(insert.scaleY, 1.5);
    EXPECT_EQ(rewritten(reread), saved);
}

// An insert without the group 43 gets a new one, after the 42, and keeps
// every other group, XDATA included.
TEST(DxfInsert, AScaleZChangedWithTheEditorAddsTheGroup43) {
    Groups entities = insert("100", "TAGGED", false);
    append(entities, {{41, "2.0"}, {42, "1.5"}, {50, "30.0"}, {1001, "APP"}, {1000, "kept"}});
    Groups expected = insert("100", "TAGGED", false);
    append(expected,
           {{41, "2.0"}, {42, "1.5"}, {43, "2"}, {50, "30.0"}, {1001, "APP"}, {1000, "kept"}});
    expectScaleZSavedAndReadBack(entities, expected);
}

// An insert with the group 43 changes only its value.
TEST(DxfInsert, AScaleZChangedWithTheEditorReplacesTheGroup43) {
    Groups entities = insert("100", "TAGGED", false);
    append(entities, {{41, "2.0"}, {42, "1.5"}, {43, "3.0"}, {1001, "APP"}, {1000, "kept"}});
    Groups expected = insert("100", "TAGGED", false);
    append(expected, {{41, "2.0"}, {42, "1.5"}, {43, "2"}, {1001, "APP"}, {1000, "kept"}});
    expectScaleZSavedAndReadBack(entities, expected);
}

}  // namespace
}  // namespace tg::io
