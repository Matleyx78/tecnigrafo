// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The BLOCKS section and the BLOCK_RECORD table (docs/tappe/0.8,
// PT18-blocchi-dxf.md, "Test richiesti").

#include "dxf_entities.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/line.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::io {
namespace {

using Groups = core::RawGroups;

void append(Groups& out, const Groups& more) {
    out.insert(out.end(), more.begin(), more.end());
}

Groups record(std::string_view handle, std::string_view name) {
    return {{0, "BLOCK_RECORD"},
            {5, std::string{handle}},
            {330, "3"},
            {100, "AcDbSymbolTableRecord"},
            {100, "AcDbBlockTableRecord"},
            {2, std::string{name}},
            {70, "0"},
            {280, "1"},
            {281, "0"}};
}

/// A BLOCK whose record is `owner`, with `entities`, and its ENDBLK.
Groups block(std::string_view handle, std::string_view owner, std::string_view name, int flags,
             const Groups& entities, std::string_view end) {
    Groups out{{0, "BLOCK"},
               {5, std::string{handle}},
               {330, std::string{owner}},
               {100, "AcDbEntity"},
               {8, "0"},
               {100, "AcDbBlockBegin"},
               {2, std::string{name}},
               {70, std::to_string(flags)},
               {10, "0"},
               {20, "0"},
               {30, "0"},
               {3, std::string{name}},
               {1, ""}};
    append(out, entities);
    append(out, {{0, "ENDBLK"},
                 {5, std::string{end}},
                 {330, std::string{owner}},
                 {100, "AcDbEntity"},
                 {8, "0"},
                 {100, "AcDbBlockEnd"}});
    return out;
}

Groups line(std::string_view handle, std::string_view owner, std::string_view x) {
    return {{0, "LINE"},
            {5, std::string{handle}},
            {330, std::string{owner}},
            {100, "AcDbEntity"},
            {8, "0"},
            {100, "AcDbLine"},
            {10, "0"},
            {20, "0"},
            {30, "0"},
            {11, std::string{x}},
            {21, "0"},
            {31, "0"}};
}

Groups insert(std::string_view handle, std::string_view owner, std::string_view name) {
    return {{0, "INSERT"},
            {5, std::string{handle}},
            {330, std::string{owner}},
            {100, "AcDbEntity"},
            {8, "0"},
            {100, "AcDbBlockReference"},
            {2, std::string{name}},
            {10, "0"},
            {20, "0"},
            {30, "0"}};
}

/// A drawing with the records `records` after those of the two layouts
/// (20 and 30), the blocks `blocks` after theirs (the paper space holds
/// `paper`), and the entities `entities`.
std::string drawing(const Groups& records, const Groups& blocks, const Groups& entities,
                    const Groups& paper = {}, std::string_view handseed = "1000") {
    Groups out{{0, "SECTION"}, {2, "HEADER"},       {9, "$ACADVER"},
               {1, "AC1032"},  {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}};
    if (!handseed.empty()) {
        append(out, {{9, "$HANDSEED"}, {5, std::string{handseed}}});
    }
    append(out, {{0, "ENDSEC"},
                 {0, "SECTION"},
                 {2, "TABLES"},
                 {0, "TABLE"},
                 {2, "BLOCK_RECORD"},
                 {5, "3"},
                 {100, "AcDbSymbolTable"},
                 {70, "2"}});
    append(out, record("20", "*Model_Space"));
    append(out, record("30", "*Paper_Space"));
    append(out, records);
    append(out, {{0, "ENDTAB"}, {0, "ENDSEC"}, {0, "SECTION"}, {2, "BLOCKS"}});
    append(out, block("21", "20", "*Model_Space", 0, {}, "22"));
    append(out, block("31", "30", "*Paper_Space", 0, paper, "32"));
    append(out, blocks);
    append(out, {{0, "ENDSEC"}, {0, "SECTION"}, {2, "ENTITIES"}});
    append(out, entities);
    append(out, {{0, "ENDSEC"}, {0, "EOF"}});
    return writeGroups(out, TextEncoding::Utf8);
}

Groups groupsOf(std::string_view text) {
    auto groups = readGroups(text);
    return groups.ok() ? groups.value().groups : Groups{};
}

/// The groups of `text` written again after reading it, without changes.
Groups rewritten(std::string_view text) {
    auto read = readDxf(text);
    if (!read.ok()) {
        return {};
    }
    auto written = writeDxf(read.value().document, DxfVersion::R2018);
    return written.ok() ? groupsOf(written.value()) : Groups{};
}

/// The value of the first group `code` after the group 0 `type` whose group
/// 2 is `name`, if any.
std::optional<std::string> valueOf(const Groups& groups, std::string_view type,
                                   std::string_view name, int code) {
    for (std::size_t i = 0; i < groups.size(); ++i) {
        if (groups[i].code != 0 || groups[i].value != type) {
            continue;
        }
        std::optional<std::string> value;
        bool named = false;
        for (std::size_t j = i + 1; j < groups.size() && groups[j].code != 0; ++j) {
            named = named || (groups[j].code == 2 && groups[j].value == name);
            if (groups[j].code == code && !value) {
                value = groups[j].value;
            }
        }
        if (named) {
            return value;
        }
    }
    return std::nullopt;
}

TEST(DxfBlocks, SimpleNestedAnonymousAndPaperSpaceAreWrittenBackAsRead) {
    Groups records = record("40", "SIMPLE");
    append(records, record("50", "OUTER"));
    append(records, record("60", "*U1"));
    Groups blocks = block("41", "40", "SIMPLE", 0, line("43", "40", "10"), "42");
    append(blocks, block("51", "50", "OUTER", 0, insert("53", "50", "SIMPLE"), "52"));
    append(blocks, block("61", "60", "*U1", 1, line("63", "60", "5"), "62"));
    Groups entities = insert("100", "20", "OUTER");
    append(entities, insert("101", "20", "*U1"));
    const std::string text = drawing(records, blocks, entities, line("33", "30", "7"));

    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    const auto definitions = document.blocks().definitions();
    ASSERT_EQ(definitions.size(), 3U);
    EXPECT_EQ(definitions[0].name, "SIMPLE");
    EXPECT_EQ(definitions[0].recordId, 0x40U);
    EXPECT_EQ(definitions[0].blockId, 0x41U);
    EXPECT_EQ(definitions[0].endBlockId, 0x42U);
    ASSERT_EQ(definitions[0].entities.size(), 1U);
    EXPECT_EQ(definitions[0].entities[0].id.value, 0x43U);
    EXPECT_TRUE(std::holds_alternative<core::Line>(definitions[0].entities[0].entity));
    ASSERT_EQ(definitions[1].entities.size(), 1U);
    EXPECT_EQ(std::get<core::Insert>(definitions[1].entities[0].entity).block, "SIMPLE");
    EXPECT_EQ(definitions[2].name, "*U1");
    EXPECT_TRUE(definitions[2].anonymous);
    // The layouts stay out of the table, the paper space with its line.
    EXPECT_EQ(document.blocks().find("*Paper_Space"), nullptr);
    EXPECT_EQ(document.find(core::EntityId{0x33}), nullptr);
    EXPECT_EQ(document.entityCount(), 2U);
    EXPECT_TRUE(document.rawEntities().empty());
    EXPECT_TRUE(read.value().report.unhandled.empty());

    EXPECT_EQ(rewritten(text), groupsOf(text));
}

TEST(DxfBlocks, ANewBlockGetsItsRecordAndHandlesOwnedByIt) {
    const std::string text = drawing({}, {}, {});
    auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    auto& document = read.value().document;
    core::BlockDefinition definition{"NEW"};
    definition.base = {1, 2};
    definition.units = 4;
    definition.explodable = false;
    definition.entities.push_back({{}, core::Line{{0, 0}, {10, 0}}, {}, 1});
    document.replaceBlock("NEW", definition);
    document.add(core::Insert{"NEW", {5, 5}, 1, 1, 0, {}});

    const auto written = writeDxf(document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    const auto groups = groupsOf(written.value());
    const auto recordHandle = valueOf(groups, "BLOCK_RECORD", "NEW", 5);
    ASSERT_TRUE(recordHandle.has_value());
    EXPECT_EQ(valueOf(groups, "BLOCK_RECORD", "NEW", 280), "0");
    EXPECT_EQ(valueOf(groups, "BLOCK_RECORD", "NEW", 70), "4");
    EXPECT_EQ(valueOf(groups, "BLOCK", "NEW", 330), recordHandle);
    // The table counts the new record.
    EXPECT_EQ(valueOf(groups, "TABLE", "BLOCK_RECORD", 70), "3");
    // The line and the ENDBLK after the BLOCK are owned by the record.
    std::vector<std::uint64_t> handles;
    std::uint64_t handseed = 0;
    for (std::size_t i = 0; i + 1 < groups.size(); ++i) {
        if (groups[i].code == 9 && groups[i].value == "$HANDSEED") {
            handseed = parseHandle(groups[++i].value).value_or(0);
        } else if (groups[i].code == 5) {
            handles.push_back(parseHandle(groups[i].value).value_or(0));
        }
    }
    // From the name of the BLOCK (after its own owner) to the end of BLOCKS.
    const auto marker = std::ranges::find(groups, core::RawGroup{100, "AcDbBlockBegin"});
    const auto begin = std::find(marker, groups.end(), core::RawGroup{2, "NEW"});
    std::vector<std::string> owners;
    for (auto it = begin; it != groups.end() && (it->code != 0 || it->value != "ENDSEC"); ++it) {
        if (it->code == 330) {
            owners.push_back(it->value);
        }
    }
    ASSERT_EQ(owners.size(), 2U);  // the LINE and the ENDBLK
    EXPECT_EQ(owners[0], recordHandle.value_or(""));
    EXPECT_EQ(owners[1], recordHandle.value_or(""));
    std::ranges::sort(handles);
    EXPECT_EQ(std::ranges::adjacent_find(handles), handles.end());
    EXPECT_GT(handseed, handles.back());

    const auto again = readDxf(written.value());
    ASSERT_TRUE(again.ok());
    const auto* read2 = again.value().document.blocks().find("NEW");
    ASSERT_NE(read2, nullptr);
    EXPECT_EQ(read2->base, (core::Vec2{1, 2}));
    EXPECT_EQ(read2->units, 4);
    EXPECT_FALSE(read2->explodable);
    ASSERT_EQ(read2->entities.size(), 1U);
    EXPECT_EQ(std::get<core::Line>(read2->entities[0].entity).end, (core::Vec2{10, 0}));
}

TEST(DxfBlocks, RecordsInAnotherOrderThanTheirBlocksGoBackWhereTheyWere) {
    Groups records = record("50", "SECOND");
    append(records, record("40", "FIRST"));
    Groups blocks = block("41", "40", "FIRST", 0, line("43", "40", "1"), "42");
    append(blocks, block("51", "50", "SECOND", 0, line("53", "50", "2"), "52"));
    const std::string text = drawing(records, blocks, insert("100", "20", "SECOND"));
    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto definitions = read.value().document.blocks().definitions();
    ASSERT_EQ(definitions.size(), 2U);
    EXPECT_EQ(definitions[0].name, "FIRST");
    EXPECT_EQ(definitions[1].name, "SECOND");
    EXPECT_EQ(rewritten(text), groupsOf(text));
}

TEST(DxfBlocks, AnInsertThatMakesACycleStaysUnhandled) {
    Groups records = record("40", "SELF");
    append(records, record("50", "A"));
    append(records, record("60", "B"));
    Groups blocks = block("41", "40", "SELF", 0, insert("43", "40", "SELF"), "42");
    append(blocks, block("51", "50", "A", 0, insert("53", "50", "B"), "52"));
    append(blocks, block("61", "60", "B", 0, insert("63", "60", "A"), "62"));
    Groups entities = insert("100", "20", "SELF");
    append(entities, insert("101", "20", "A"));
    const std::string text = drawing(records, blocks, entities);

    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    const auto* self = document.blocks().find("SELF");
    ASSERT_NE(self, nullptr);
    EXPECT_TRUE(self->entities.empty());
    ASSERT_EQ(self->rawEntities.size(), 1U);
    EXPECT_EQ(self->rawEntities[0].id.value, 0x43U);
    const auto* a = document.blocks().find("A");
    const auto* b = document.blocks().find("B");
    ASSERT_NE(a, nullptr);
    ASSERT_NE(b, nullptr);
    EXPECT_EQ(a->entities.size(), 1U);
    EXPECT_EQ(b->rawEntities.size(), 1U);
    EXPECT_EQ(document.entityCount(), 2U);
    const auto& warnings = read.value().report.warnings;
    EXPECT_NE(std::ranges::find(warnings, std::string{"2 inserts that would make a block contain "
                                                      "itself kept as unhandled data."}),
              warnings.end());

    EXPECT_EQ(rewritten(text), groupsOf(text));
}

TEST(DxfBlocks, ExternalDimensionAndUnrecordedBlocksStayAsTheyAre) {
    Groups records = record("40", "XREF");
    append(records, record("50", "*D1"));
    Groups blocks = block("41", "40", "XREF", 4, line("43", "40", "1"), "42");
    append(blocks, block("51", "50", "*D1", 1, line("53", "50", "1"), "52"));
    append(blocks, block("71", "70", "NORECORD", 0, line("73", "70", "1"), "72"));
    Groups entities = insert("100", "20", "XREF");
    append(entities, insert("101", "20", "NORECORD"));
    const std::string text = drawing(records, blocks, entities);

    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    EXPECT_TRUE(document.blocks().definitions().empty());
    EXPECT_EQ(document.entityCount(), 0U);
    EXPECT_EQ(document.rawEntities().size(), 2U);
    EXPECT_EQ(rewritten(text), groupsOf(text));
}

TEST(DxfBlocks, HandseedPassesTheHandlesOfTheBlocks) {
    // $HANDSEED below the handle of an entity of a block: the first entity
    // drawn takes the next one, and the file saved says so.
    const std::string text =
        drawing(record("40", "HIGH"), block("41", "40", "HIGH", 0, line("FFFE", "40", "1"), "42"),
                {}, {}, "100");
    auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    auto& document = read.value().document;
    EXPECT_EQ(document.add(core::Line{{0, 0}, {1, 0}}).value, 0xFFFFU);
    const auto written = writeDxf(document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    const auto groups = groupsOf(written.value());
    const auto seed = std::ranges::find(groups, core::RawGroup{9, "$HANDSEED"});
    ASSERT_NE(seed, groups.end());
    EXPECT_GT(parseHandle((seed + 1)->value).value_or(0), 0xFFFFU);
}

/// A chain of `levels` blocks, each inserting the next, and an INSERT of
/// the first in the drawing.
std::string chain(int levels) {
    Groups records;
    Groups blocks;
    for (int i = 0; i < levels; ++i) {
        const std::string name = "L" + std::to_string(i);
        const std::string recordHandle = handleText(0x1000 + (static_cast<std::uint64_t>(i) * 4));
        const std::string blockHandle = handleText(0x1001 + (static_cast<std::uint64_t>(i) * 4));
        const std::string endHandle = handleText(0x1002 + (static_cast<std::uint64_t>(i) * 4));
        const std::string entity = handleText(0x1003 + (static_cast<std::uint64_t>(i) * 4));
        append(records, record(recordHandle, name));
        append(blocks,
               block(blockHandle, recordHandle, name, 0,
                     i + 1 < levels ? insert(entity, recordHandle, "L" + std::to_string(i + 1))
                                    : line(entity, recordHandle, "1"),
                     endHandle));
    }
    return drawing(records, blocks, insert("100", "20", "L0"), {}, "");
}

TEST(DxfBlocks, InsertsDeeperThanTheLimitAreReported) {
    const auto deep = readDxf(chain(core::kMaxInsertDepth + 1));
    ASSERT_TRUE(deep.ok());
    EXPECT_EQ(deep.value().document.blocks().definitions().size(),
              static_cast<std::size_t>(core::kMaxInsertDepth + 1));
    const auto& warnings = deep.value().report.warnings;
    EXPECT_NE(std::ranges::find(warnings, std::string{"1 insert nest blocks deeper than 64 levels: "
                                                      "the deeper ones are not drawn."}),
              warnings.end());
    const auto limit = readDxf(chain(core::kMaxInsertDepth));
    ASSERT_TRUE(limit.ok());
    EXPECT_TRUE(limit.value().report.warnings.empty());
}

}  // namespace
}  // namespace tg::io
