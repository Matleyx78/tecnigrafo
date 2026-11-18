// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// A DXF file as a block definition (docs/tappe/0.8/PT19-file-come-blocco.md,
// "Test richiesti").

#include "tg/io/block_import.hpp"

#include "dxf_entities.hpp"
#include "tg/core/document.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/line.hpp"
#include "tg/io/dxf_groups.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::io {
namespace {

using Groups = core::RawGroups;

void append(Groups& out, const Groups& more) {
    out.insert(out.end(), more.begin(), more.end());
}

/// Files kept in memory, by name.
class MemoryFiles final : public FileAccess {
public:
    MemoryFiles() = default;

    void put(std::string name, std::string bytes) {
        m_files.insert_or_assign(std::move(name), std::move(bytes));
    }

    [[nodiscard]] Result<std::string> read(std::string_view name) override {
        const auto found = m_files.find(name);
        if (found == m_files.end()) {
            return Error{Error::Kind::File, 0, "does not exist"};
        }
        return found->second;
    }

    [[nodiscard]] std::optional<Error> write(std::string_view name,
                                             std::string_view bytes) override {
        put(std::string{name}, std::string{bytes});
        return std::nullopt;
    }

    [[nodiscard]] bool exists(std::string_view name) const override {
        return m_files.contains(name);
    }

private:
    std::map<std::string, std::string, std::less<>> m_files;
};

/// What a hand-written AC1032 file holds, section by section.
struct Source {
    std::optional<core::Vec2> base;  ///< $INSBASE, absent if std::nullopt
    int units{0};                    ///< $INSUNITS, absent if 0
    Groups tables;                   ///< whole TABLE ... ENDTAB, before BLOCK_RECORD
    Groups blocks;                   ///< BLOCK ... ENDBLK of the blocks other than *Model_Space
    std::vector<std::pair<std::string, std::string>> records;  ///< BLOCK_RECORD handle, name
    Groups entities;
};

std::string text(const Source& source) {
    Groups out{{0, "SECTION"}, {2, "HEADER"}, {9, "$ACADVER"}, {1, "AC1032"}};
    if (source.base) {
        append(out, {{9, "$INSBASE"},
                     {10, std::to_string(source.base->x)},
                     {20, std::to_string(source.base->y)},
                     {30, "0.0"}});
    }
    if (source.units != 0) {
        append(out, {{9, "$INSUNITS"}, {70, std::to_string(source.units)}});
    }
    append(out, {{0, "ENDSEC"}, {0, "SECTION"}, {2, "TABLES"}});
    append(out, source.tables);
    append(out, {{0, "TABLE"}, {2, "BLOCK_RECORD"}, {5, "3"}, {100, "AcDbSymbolTable"}});
    std::vector<std::pair<std::string, std::string>> records{{"1F", "*Model_Space"}};
    records.insert(records.end(), source.records.begin(), source.records.end());
    for (const auto& [handle, name] : records) {
        append(out, {{0, "BLOCK_RECORD"},
                     {5, handle},
                     {330, "3"},
                     {100, "AcDbSymbolTableRecord"},
                     {100, "AcDbBlockTableRecord"},
                     {2, name},
                     {70, "0"}});
    }
    append(out, {{0, "ENDTAB"},
                 {0, "ENDSEC"},
                 {0, "SECTION"},
                 {2, "BLOCKS"},
                 {0, "BLOCK"},
                 {5, "1D"},
                 {330, "1F"},
                 {100, "AcDbEntity"},
                 {8, "0"},
                 {100, "AcDbBlockBegin"},
                 {2, "*Model_Space"},
                 {70, "0"},
                 {10, "0.0"},
                 {20, "0.0"},
                 {30, "0.0"},
                 {3, "*Model_Space"},
                 {0, "ENDBLK"},
                 {5, "1E"},
                 {330, "1F"},
                 {100, "AcDbEntity"},
                 {8, "0"},
                 {100, "AcDbBlockEnd"}});
    append(out, source.blocks);
    append(out, {{0, "ENDSEC"}, {0, "SECTION"}, {2, "ENTITIES"}});
    append(out, source.entities);
    append(out, {{0, "ENDSEC"}, {0, "EOF"}});
    return writeGroups(out, TextEncoding::Utf8);
}

/// The block `name` (record `record`, BLOCK `begin`, ENDBLK `end`) with
/// `entities`.
Groups block(std::string_view name, std::string_view record, std::string_view begin,
             std::string_view end, const Groups& entities) {
    Groups out{{0, "BLOCK"},
               {5, std::string{begin}},
               {330, std::string{record}},
               {100, "AcDbEntity"},
               {8, "0"},
               {100, "AcDbBlockBegin"},
               {2, std::string{name}},
               {70, "0"},
               {10, "0.0"},
               {20, "0.0"},
               {30, "0.0"},
               {3, std::string{name}}};
    append(out, entities);
    append(out, {{0, "ENDBLK"},
                 {5, std::string{end}},
                 {330, std::string{record}},
                 {100, "AcDbEntity"},
                 {8, "0"},
                 {100, "AcDbBlockEnd"}});
    return out;
}

Groups line(std::string_view handle, std::string_view owner, core::Vec2 start, core::Vec2 end,
            std::string_view layer = "0", const Groups& pointers = {}) {
    Groups out{{0, "LINE"}, {5, std::string{handle}}, {330, std::string{owner}}};
    append(out, pointers);
    append(out, {{100, "AcDbEntity"},
                 {8, std::string{layer}},
                 {100, "AcDbLine"},
                 {10, std::to_string(start.x)},
                 {20, std::to_string(start.y)},
                 {30, "0.0"},
                 {11, std::to_string(end.x)},
                 {21, std::to_string(end.y)},
                 {31, "0.0"}});
    return out;
}

/// A SPLINE, which io keeps unhandled; in paper space if `paper`.
Groups spline(std::string_view handle, std::string_view owner, bool paper = false) {
    Groups out{
        {0, "SPLINE"}, {5, std::string{handle}}, {330, std::string{owner}}, {100, "AcDbEntity"}};
    if (paper) {
        out.push_back({67, "1"});
    }
    append(out,
           {{8, "0"}, {100, "AcDbSpline"}, {70, "0"}, {71, "1"}, {72, "0"}, {73, "0"}, {74, "0"}});
    return out;
}

Groups attdef(std::string_view handle, std::string_view owner) {
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
            {280, "0"},
            {3, "Prompt"},
            {2, "REF"},
            {70, "0"},
            {280, "0"}};
}

/// An INSERT of `name` at `at` (in model space by default), followed by an
/// ATTRIB at `attribute` and its SEQEND when `handles` has three handles.
Groups insert(const std::vector<std::string>& handles, std::string_view name, core::Vec2 at,
              core::Vec2 attribute = {}, std::string_view owner = "1F") {
    const bool attributes = handles.size() == 3;
    Groups out{{0, "INSERT"},       {5, handles[0]}, {330, std::string{owner}},
               {100, "AcDbEntity"}, {8, "0"},        {100, "AcDbBlockReference"}};
    if (attributes) {
        out.push_back({66, "1"});
    }
    append(out, {{2, std::string{name}},
                 {10, std::to_string(at.x)},
                 {20, std::to_string(at.y)},
                 {30, "0.0"}});
    if (!attributes) {
        return out;
    }
    append(out, {{0, "ATTRIB"},
                 {5, handles[1]},
                 {330, handles[0]},
                 {100, "AcDbEntity"},
                 {8, "0"},
                 {100, "AcDbText"},
                 {10, std::to_string(attribute.x)},
                 {20, std::to_string(attribute.y)},
                 {30, "0.0"},
                 {40, "2.5"},
                 {1, "-K1"},
                 {100, "AcDbAttribute"},
                 {280, "0"},
                 {2, "REF"},
                 {70, "0"},
                 {280, "1"},
                 {0, "SEQEND"},
                 {5, handles[2]},
                 {330, handles[0]},
                 {100, "AcDbEntity"},
                 {8, "0"}});
    return out;
}

/// The file "Pezzo.dxf": $INSBASE `base`, a LINE from base to base + (10,0),
/// an INSERT of the block TAGGED (an ATTDEF) at base + (10,20) with an
/// ATTRIB at base + (12,24), and `more` at the end of model space.
Source pezzo(std::optional<core::Vec2> base, const Groups& more = {}) {
    const core::Vec2 at = base.value_or(core::Vec2{});
    Source source;
    source.base = base;
    source.records = {{"40", "TAGGED"}};
    source.blocks = block("TAGGED", "40", "41", "42", attdef("43", "40"));
    source.entities = line("30", "1F", at, at + core::Vec2{10, 0});
    append(source.entities, insert({"100", "101", "103"}, "TAGGED", at + core::Vec2{10, 20},
                                   at + core::Vec2{12, 24}));
    append(source.entities, more);
    return source;
}

Result<BlockImport> import(const Source& source, const core::Document& target,
                           std::string_view name = "Pezzo.dxf") {
    MemoryFiles files;
    files.put(std::string{name}, text(source));
    return importBlock(std::filesystem::path{std::string{name}}, target, files);
}

const core::Line& lineOf(const core::EntityRecord& record) {
    return std::get<core::Line>(record.entity);
}

const core::Insert& insertOf(const core::EntityRecord& record) {
    return std::get<core::Insert>(record.entity);
}

/// The groups of `raw` other than its handle (5) and its owner (330).
Groups withoutHandles(const Groups& groups) {
    Groups out;
    for (const core::RawGroup& group : groups) {
        if (group.code != 5 && group.code != 330) {
            out.push_back(group);
        }
    }
    return out;
}

/// The value of the first group `code` of `groups`, or "".
std::string valueOf(const Groups& groups, std::int16_t code) {
    for (const core::RawGroup& group : groups) {
        if (group.code == code) {
            return group.value;
        }
    }
    return {};
}

/// Every group an entity kept, in the order the writer puts them back.
Groups allOf(const core::EntityExtras& extras) {
    Groups out;
    for (const Groups* groups : {&extras.beforeEntity, &extras.common, &extras.specific,
                                 &extras.placement, &extras.extended}) {
        append(out, *groups);
    }
    return out;
}

TEST(BlockImport, MovesTheModelSpaceByMinusInsbaseAttributesIncluded) {
    const auto result = import(pezzo(core::Vec2{100, 120}), core::Document::newDrawing());
    ASSERT_TRUE(result.ok()) << result.error().message;
    const core::BlockDefinition& definition = result.value().definition;
    EXPECT_EQ(definition.name, "Pezzo");
    EXPECT_EQ(definition.base, (core::Vec2{0, 0}));
    EXPECT_EQ(definition.units, 0);
    ASSERT_EQ(definition.entities.size(), 2U);
    EXPECT_TRUE(definition.rawEntities.empty());
    EXPECT_EQ(lineOf(definition.entities[0]).start, (core::Vec2{0, 0}));
    EXPECT_EQ(lineOf(definition.entities[0]).end, (core::Vec2{10, 0}));
    const core::Insert& tagged = insertOf(definition.entities[1]);
    EXPECT_EQ(tagged.block, "TAGGED");
    EXPECT_EQ(tagged.insertion, (core::Vec2{10, 20}));
    ASSERT_EQ(tagged.attributes.size(), 1U);
    EXPECT_EQ(tagged.attributes[0].text.position, (core::Vec2{12, 24}));
    EXPECT_EQ(tagged.attributes[0].text.value, "-K1");
    // The nested block keeps its own coordinates.
    ASSERT_EQ(result.value().nested.size(), 1U);
    EXPECT_EQ(result.value().nested[0].name, "TAGGED");
}

TEST(BlockImport, AnUnhandledEntityKeepsInsbaseAndEveryEntityIntact) {
    const Groups raw = spline("50", "1F");
    const auto result = import(pezzo(core::Vec2{100, 120}, raw), core::Document::newDrawing());
    ASSERT_TRUE(result.ok()) << result.error().message;
    const core::BlockDefinition& definition = result.value().definition;
    EXPECT_EQ(definition.base, (core::Vec2{100, 120}));
    ASSERT_EQ(definition.entities.size(), 2U);
    EXPECT_EQ(lineOf(definition.entities[0]).start, (core::Vec2{100, 120}));
    EXPECT_EQ(lineOf(definition.entities[0]).end, (core::Vec2{110, 120}));
    EXPECT_EQ(insertOf(definition.entities[1]).insertion, (core::Vec2{110, 140}));
    EXPECT_EQ(insertOf(definition.entities[1]).attributes[0].text.position, (core::Vec2{112, 144}));
    ASSERT_EQ(definition.rawEntities.size(), 1U);
    EXPECT_EQ(definition.rawEntities[0].type, "SPLINE");
    const Groups expected(raw.begin() + 1, raw.end());
    EXPECT_EQ(withoutHandles(definition.rawEntities[0].groups), withoutHandles(expected));
}

TEST(BlockImport, AnUnhandledEntityWithoutInsbaseKeepsTheOrigin) {
    for (const std::optional<core::Vec2> base :
         {std::optional<core::Vec2>{core::Vec2{0, 0}}, std::optional<core::Vec2>{}}) {
        const Groups raw = spline("50", "1F");
        const auto result = import(pezzo(base, raw), core::Document::newDrawing());
        ASSERT_TRUE(result.ok()) << result.error().message;
        const core::BlockDefinition& definition = result.value().definition;
        EXPECT_EQ(definition.base, (core::Vec2{0, 0}));
        EXPECT_EQ(lineOf(definition.entities[0]).end, (core::Vec2{10, 0}));
        ASSERT_EQ(definition.rawEntities.size(), 1U);
        const Groups expected(raw.begin() + 1, raw.end());
        EXPECT_EQ(withoutHandles(definition.rawEntities[0].groups), withoutHandles(expected));
    }
}

TEST(BlockImport, OnlyTheModelSpaceCountsForTheMove) {
    // The SPLINE in paper space: it does not enter, and the rest moves.
    const auto paper =
        import(pezzo(core::Vec2{100, 120}, spline("50", "1F", true)), core::Document::newDrawing());
    ASSERT_TRUE(paper.ok()) << paper.error().message;
    EXPECT_EQ(paper.value().definition.base, (core::Vec2{0, 0}));
    EXPECT_TRUE(paper.value().definition.rawEntities.empty());
    EXPECT_EQ(lineOf(paper.value().definition.entities[0]).start, (core::Vec2{0, 0}));

    // The SPLINE in a nested block: the block keeps it and its coordinates,
    // the model space moves (its INSERT of the block too).
    Source nested = pezzo(core::Vec2{100, 120});
    nested.records.emplace_back("60", "CURVY");
    Groups curvy = line("64", "60", {1, 2}, {3, 4});
    append(curvy, spline("63", "60"));
    append(nested.blocks, block("CURVY", "60", "61", "62", curvy));
    append(nested.entities, insert({"70"}, "CURVY", {105, 125}));
    const auto inner = import(nested, core::Document::newDrawing());
    ASSERT_TRUE(inner.ok()) << inner.error().message;
    const core::BlockDefinition& definition = inner.value().definition;
    EXPECT_EQ(definition.base, (core::Vec2{0, 0}));
    ASSERT_EQ(definition.entities.size(), 3U);
    EXPECT_EQ(lineOf(definition.entities[0]).start, (core::Vec2{0, 0}));
    EXPECT_EQ(insertOf(definition.entities[2]).insertion, (core::Vec2{5, 5}));
    ASSERT_EQ(inner.value().nested.size(), 2U);
    const core::BlockDefinition& kept = inner.value().nested[1];
    EXPECT_EQ(kept.name, "CURVY");
    ASSERT_EQ(kept.entities.size(), 1U);
    EXPECT_EQ(lineOf(kept.entities[0]).start, (core::Vec2{1, 2}));
    EXPECT_EQ(lineOf(kept.entities[0]).end, (core::Vec2{3, 4}));
    ASSERT_EQ(kept.rawEntities.size(), 1U);
    EXPECT_EQ(kept.rawEntities[0].type, "SPLINE");
}

/// A LTYPE table with `records` (handle, name, description), continuous.
Groups linetypes(const std::vector<std::vector<std::string>>& records) {
    Groups out{{0, "TABLE"}, {2, "LTYPE"}, {5, "5"}, {100, "AcDbSymbolTable"}};
    for (const std::vector<std::string>& record : records) {
        append(out, {{0, "LTYPE"},
                     {5, record[0]},
                     {330, "5"},
                     {100, "AcDbSymbolTableRecord"},
                     {100, "AcDbLinetypeTableRecord"},
                     {2, record[1]},
                     {70, "0"},
                     {3, record[2]},
                     {72, "65"},
                     {73, "0"},
                     {40, "0.0"}});
    }
    append(out, {{0, "ENDTAB"}});
    return out;
}

/// A LAYER table with `records` (handle, name, colour, linetype).
Groups layers(const std::vector<std::vector<std::string>>& records) {
    Groups out{{0, "TABLE"}, {2, "LAYER"}, {5, "2"}, {100, "AcDbSymbolTable"}};
    for (const std::vector<std::string>& record : records) {
        append(out, {{0, "LAYER"},
                     {5, record[0]},
                     {330, "2"},
                     {100, "AcDbSymbolTableRecord"},
                     {100, "AcDbLayerTableRecord"},
                     {2, record[1]},
                     {70, "0"},
                     {62, record[2]},
                     {6, record[3]}});
    }
    append(out, {{0, "ENDTAB"}});
    return out;
}

TEST(BlockImport, TheRecordsOfTheDrawingWinAndTheOthersEnter) {
    Source source;
    source.tables = linetypes({{"14", "Continuous", "Other solid line"}, {"15", "Mine", "My own"}});
    append(source.tables, layers({{"10", "0", "1", "Continuous"}, {"11", "Extra", "3", "Mine"}}));
    source.entities = line("30", "1F", {0, 0}, {1, 0}, "0");
    append(source.entities, line("31", "1F", {0, 0}, {0, 1}, "Extra"));
    const core::Document target = core::Document::newDrawing();
    const auto result = import(source, target, "tables.dxf");
    ASSERT_TRUE(result.ok()) << result.error().message;
    const BlockImport& imported = result.value();

    ASSERT_EQ(imported.linetypes.size(), 1U);
    EXPECT_EQ(imported.linetypes[0].name, "Mine");
    EXPECT_EQ(imported.linetypes[0].description, "My own");
    ASSERT_EQ(imported.layers.size(), 1U);
    const core::Layer& extra = imported.layers[0];
    EXPECT_EQ(extra.name, "Extra");
    EXPECT_EQ(extra.color, core::Color::fromIndex(3));
    // Renumbered in the order of the file: LTYPE Mine, then LAYER Extra.
    EXPECT_EQ(imported.linetypes[0].id.value, target.nextId());
    EXPECT_EQ(extra.id.value, target.nextId() + 1);
    EXPECT_EQ(extra.linetype, imported.linetypes[0].id);
    // The entities use the layer 0 of the drawing and the Extra that enters.
    const core::BlockDefinition& definition = imported.definition;
    ASSERT_EQ(definition.entities.size(), 2U);
    EXPECT_EQ(definition.entities[0].properties.layer, target.layerZero());
    EXPECT_EQ(definition.entities[1].properties.layer, extra.id);
}

TEST(BlockImport, HandlesAreRenumberedAndTheirOwnersFollow) {
    // A pointer (340) to the TAGGED record that enters and one (360) to a
    // handle the file does not have, which stays as read.
    Source source = pezzo(std::nullopt);
    source.entities = line("30", "1F", {0, 0}, {10, 0}, "0", {{340, "40"}, {360, "ABC"}});
    append(source.entities, insert({"100", "101", "103"}, "TAGGED", {10, 20}, {12, 24}));
    const core::Document target = core::Document::newDrawing();
    const std::uint64_t first = target.nextId();
    const auto result = import(source, target);
    ASSERT_TRUE(result.ok()) << result.error().message;
    const BlockImport& imported = result.value();

    // File order: BLOCK_RECORD 40, BLOCK 41, ATTDEF 43, ENDBLK 42, LINE 30,
    // INSERT 100, ATTRIB 101, SEQEND 103.
    ASSERT_EQ(imported.nested.size(), 1U);
    const core::BlockDefinition& tagged = imported.nested[0];
    EXPECT_EQ(tagged.recordId, first);
    EXPECT_EQ(tagged.blockId, first + 1);
    ASSERT_EQ(tagged.entities.size(), 1U);
    EXPECT_EQ(tagged.entities[0].id.value, first + 2);
    EXPECT_EQ(tagged.endBlockId, first + 3);
    EXPECT_EQ(valueOf(tagged.blockExtras, 330), handleText(first));
    EXPECT_EQ(valueOf(tagged.endBlockExtras, 330), handleText(first));
    EXPECT_EQ(valueOf(allOf(tagged.entities[0].extras), 330), handleText(first));

    const core::BlockDefinition& definition = imported.definition;
    ASSERT_EQ(definition.entities.size(), 2U);
    const core::EntityRecord& lineRecord = definition.entities[0];
    EXPECT_EQ(lineRecord.id.value, first + 4);
    EXPECT_EQ(valueOf(allOf(lineRecord.extras), 340), handleText(first));
    EXPECT_EQ(valueOf(allOf(lineRecord.extras), 360), "ABC");
    // The owner *Model_Space does not enter: it stays as read.
    EXPECT_EQ(valueOf(allOf(lineRecord.extras), 330), "1F");

    const core::EntityRecord& insertRecord = definition.entities[1];
    EXPECT_EQ(insertRecord.id.value, first + 5);
    const core::Attribute& attribute = insertOf(insertRecord).attributes.at(0);
    EXPECT_EQ(attribute.id.value, first + 6);
    EXPECT_EQ(valueOf(allOf(attribute.extras), 330), handleText(first + 5));
    // The SEQEND is kept with the INSERT, renumbered and owned by it.
    const Groups& seqend = insertRecord.extras.specific;
    EXPECT_EQ(valueOf(seqend, 5), handleText(first + 7));
    EXPECT_EQ(valueOf(seqend, 330), handleText(first + 5));
    EXPECT_EQ(imported.nextId, first + 8);
}

TEST(BlockImport, UnitFactorIsTheRatioOfTwoDifferentUnits) {
    Source source = pezzo(std::nullopt);
    source.units = 4;
    core::Document metres = core::Document::newDrawing();
    metres.setUnits(6);
    const auto result = import(source, metres);
    ASSERT_TRUE(result.ok()) << result.error().message;
    EXPECT_EQ(result.value().definition.units, 4);
    EXPECT_DOUBLE_EQ(result.value().unitFactor, 0.001);

    // Same units, or one of them 0: no conversion.
    core::Document millimetres = core::Document::newDrawing();
    millimetres.setUnits(4);
    EXPECT_EQ(import(source, millimetres).value().unitFactor, 1.0);
    core::Document unitless = core::Document::newDrawing();
    unitless.setUnits(0);
    EXPECT_EQ(import(source, unitless).value().unitFactor, 1.0);
    source.units = 0;
    EXPECT_EQ(import(source, metres).value().unitFactor, 1.0);
}

TEST(BlockImport, AFileThatInsertsItselfIsRefused) {
    Source direct;
    direct.entities = insert({"30"}, "self", {0, 0});
    const auto refused = import(direct, core::Document::newDrawing(), "self.dxf");
    ASSERT_FALSE(refused.ok());
    EXPECT_EQ(refused.error().message, "Block \"self\" references itself.");

    // Through a block of the file.
    Source through;
    through.records = {{"40", "Outer"}};
    through.blocks = block("Outer", "40", "41", "42", insert({"43"}, "SELF", {1, 1}, {}, "40"));
    through.entities = insert({"30"}, "Outer", {0, 0});
    const auto again = import(through, core::Document::newDrawing(), "self.dxf");
    ASSERT_FALSE(again.ok());
    EXPECT_EQ(again.error().message, "Block \"self\" references itself.");
}

TEST(BlockImport, AMissingFileIsAnError) {
    MemoryFiles files;
    const auto result =
        importBlock(std::filesystem::path{"none.dxf"}, core::Document::newDrawing(), files);
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.error().kind, Error::Kind::File);
}

}  // namespace
}  // namespace tg::io
