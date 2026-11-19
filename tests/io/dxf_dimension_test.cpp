// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// DIMENSION and its block *D… (docs/tappe/0.8/PT25-quote-dxf.md, "Test
// richiesti").

#include "dxf_entities.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/line.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <numbers>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::io {
namespace {

using Groups = core::RawGroups;

void append(Groups& out, const Groups& more) {
    out.insert(out.end(), more.begin(), more.end());
}

/// A block *D… of a dimension: a LINE (handled) and a POINT (not handled,
/// as Tecnigrafo does not draw the definition points).
struct DimensionBlock {
    std::string name;
    std::string record;  ///< handle of the BLOCK_RECORD
    std::string begin;   ///< handle of the BLOCK
    std::string end;     ///< handle of the ENDBLK
    std::string line;    ///< handle of its LINE
    std::string point;   ///< handle of its POINT
};

Groups blockRecord(std::string_view handle, std::string_view name) {
    return {{0, "BLOCK_RECORD"},
            {5, std::string{handle}},
            {330, "3"},
            {100, "AcDbSymbolTableRecord"},
            {100, "AcDbBlockTableRecord"},
            {2, std::string{name}},
            {340, "0"},
            {70, "     0"},
            {280, "     1"},
            {281, "     0"}};
}

Groups blockGroups(const DimensionBlock& block) {
    return {{0, "BLOCK"},
            {5, block.begin},
            {330, block.record},
            {100, "AcDbEntity"},
            {8, "0"},
            {100, "AcDbBlockBegin"},
            {2, block.name},
            {70, "     1"},
            {10, "0.0"},
            {20, "0.0"},
            {30, "0.0"},
            {3, block.name},
            {1, ""},
            {0, "LINE"},
            {5, block.line},
            {330, block.record},
            {100, "AcDbEntity"},
            {8, "0"},
            {62, "     0"},
            {440, " 16777216"},
            {370, "    -2"},
            {100, "AcDbLine"},
            {10, "0.0"},
            {20, "0.625"},
            {30, "0.0"},
            {11, "0.0"},
            {21, "11.25"},
            {31, "0.0"},
            {0, "POINT"},
            {5, block.point},
            {330, block.record},
            {100, "AcDbEntity"},
            {8, "0"},
            {100, "AcDbPoint"},
            {10, "0.0"},
            {20, "0.0"},
            {30, "0.0"},
            {0, "ENDBLK"},
            {5, block.end},
            {330, block.record},
            {100, "AcDbEntity"},
            {8, "0"},
            {100, "AcDbBlockEnd"}};
}

/// A drawing with the blocks `blocks` (and *Model_Space, record 20) and the
/// entities `entities`.
std::string drawing(const std::vector<DimensionBlock>& blocks, const Groups& entities,
                    const Groups& extraBlocks = {}, const Groups& extraRecords = {}) {
    Groups out{{0, "SECTION"},      {2, "HEADER"},    {9, "$ACADVER"},         {1, "AC1032"},
               {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}, {9, "$HANDSEED"},        {5, "1000"},
               {0, "ENDSEC"},       {0, "SECTION"},   {2, "TABLES"},           {0, "TABLE"},
               {2, "BLOCK_RECORD"}, {5, "3"},         {100, "AcDbSymbolTable"}};
    out.push_back({70, std::to_string(1 + blocks.size())});
    append(out, blockRecord("20", "*Model_Space"));
    for (const auto& block : blocks) {
        append(out, blockRecord(block.record, block.name));
    }
    append(out, extraRecords);
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
                 {100, "AcDbBlockEnd"}});
    for (const auto& block : blocks) {
        append(out, blockGroups(block));
    }
    append(out, extraBlocks);
    append(out, {{0, "ENDSEC"}, {0, "SECTION"}, {2, "ENTITIES"}});
    append(out, entities);
    append(out, {{0, "ENDSEC"}, {0, "EOF"}});
    return writeGroups(out, TextEncoding::Utf8);
}

/// A DIMENSION as AutoCAD writes it: AcDbDimension, then `subclasses`.
Groups dimension(std::string_view handle, std::string_view block, int flags,
                 std::pair<std::string, std::string> linePoint, const Groups& subclasses,
                 const Groups& extended = {}) {
    Groups out{{0, "DIMENSION"},
               {5, std::string{handle}},
               {330, "20"},
               {100, "AcDbEntity"},
               {8, "0"},
               {100, "AcDbDimension"},
               {280, "     0"},
               {2, std::string{block}},
               {10, linePoint.first},
               {20, linePoint.second},
               {30, "0.0"},
               {11, "20.0"},
               {21, "11.875"},
               {31, "0.0"},
               {70, "    " + std::to_string(flags)},
               {71, "     5"},
               {42, "40.0"},
               {73, "     0"},
               {74, "     0"},
               {75, "     0"},
               {3, "ISO-25"}};
    append(out, subclasses);
    append(out, extended);
    return out;
}

Groups point(int code, std::string_view x, std::string_view y) {
    return {{static_cast<std::int16_t>(code), std::string{x}},
            {static_cast<std::int16_t>(code + 10), std::string{y}},
            {static_cast<std::int16_t>(code + 20), "0.0"}};
}

Groups concat(std::initializer_list<Groups> parts) {
    Groups out;
    for (const auto& part : parts) {
        append(out, part);
    }
    return out;
}

DimensionBlock block(int n) {
    // Handles 100 + 10·n…: record, BLOCK, ENDBLK, LINE, POINT.
    const auto hex = [n](int k) {
        constexpr std::uint64_t kFirst = 0x100;
        constexpr std::uint64_t kStep = 0x10;
        return handleText(kFirst + (kStep * static_cast<std::uint64_t>(n)) +
                          static_cast<std::uint64_t>(k));
    };
    return {"*D" + std::to_string(n), hex(0), hex(1), hex(2), hex(3), hex(4)};
}

/// The six dimensions of the round trip: the five kinds of the stage and an
/// ordinate one (other), handles 51 to 56, blocks *D1 to *D6.
Groups sixDimensions() {
    Groups out;
    append(out, dimension("51", "*D1", 32, {"40.0", "10.0"},
                          concat({{{100, "AcDbAlignedDimension"}},
                                  point(13, "0.0", "0.0"),
                                  point(14, "40.0", "0.0"),
                                  {{100, "AcDbRotatedDimension"}, {50, "30.0"}}})));
    append(out, dimension("52", "*D2", 33, {"150.0", "30.0"},
                          concat({{{100, "AcDbAlignedDimension"}},
                                  point(13, "140.0", "0.0"),
                                  point(14, "160.0", "20.0")})));
    append(out, dimension("53", "*D3", 34, {"240.0", "0.0"},
                          concat({{{100, "AcDb2LineAngularDimension"}},
                                  point(13, "200.0", "0.0"),
                                  point(14, "230.0", "20.0"),
                                  point(15, "200.0", "0.0"),
                                  point(16, "235.0", "7.0")})));
    append(out, dimension("54", "*D4", 163, {"321.0", "4.0"},
                          concat({{{100, "AcDbDiametricDimension"}},
                                  point(15, "338.0", "16.0"),
                                  {{40, "0.0"}}})));
    append(
        out,
        dimension(
            "55", "*D5", 164, {"280.0", "10.0"},
            concat({{{100, "AcDbRadialDimension"}}, point(15, "288.9", "14.5"), {{40, "0.0"}}})));
    append(out, dimension("56", "*D6", 102, {"0.0", "0.0"},
                          concat({{{100, "AcDbOrdinateDimension"}},
                                  point(13, "0.0", "-250.0"),
                                  point(14, "0.0", "-235.0")})));
    return out;
}

std::vector<DimensionBlock> sixBlocks() {
    std::vector<DimensionBlock> blocks;
    for (int n = 1; n <= 6; ++n) {
        blocks.push_back(block(n));
    }
    return blocks;
}

Groups groupsOf(std::string_view text) {
    auto groups = readGroups(text);
    return groups.ok() ? groups.value().groups : Groups{};
}

std::string written(const core::Document& document) {
    auto out = writeDxf(document, DxfVersion::R2018);
    return out.ok() ? out.value() : std::string{};
}

/// The first difference between two lists of groups, or empty: the same
/// codes, and the same text or the same number in each group.
std::string difference(const Groups& before, const Groups& after) {
    for (std::size_t i = 0; i < std::min(before.size(), after.size()); ++i) {
        const auto& a = before[i];
        const auto& b = after[i];
        const auto trim = [](std::string_view text) {
            const auto first = text.find_first_not_of(' ');
            return first == std::string_view::npos ? std::string_view{} : text.substr(first);
        };
        const auto x = parseNumber(trim(a.value));
        const auto y = parseNumber(trim(b.value));
        const bool sameNumber = x && y && *x == *y;
        if (a.code != b.code || (a.value != b.value && !sameNumber)) {
            return "group " + std::to_string(i) + ": " + std::to_string(a.code) + " \"" + a.value +
                   "\" became " + std::to_string(b.code) + " \"" + b.value + "\"";
        }
    }
    return before.size() == after.size() ? std::string{} : std::string{"different sizes"};
}

const core::Dimension& dimensionOf(const core::Document& document, std::uint64_t handle) {
    const auto* record = document.find(core::EntityId{handle});
    EXPECT_NE(record, nullptr);
    return std::get<core::Dimension>(record->entity);
}

/// The groups of the block named `name` (from its group 0 BLOCK to its
/// ENDBLK), or empty.
Groups blockNamed(const Groups& groups, std::string_view name) {
    for (std::size_t i = 0; i + 1 < groups.size(); ++i) {
        if (groups[i].code != 0 || groups[i].value != "BLOCK") {
            continue;
        }
        std::size_t end = i + 1;
        bool named = false;
        while (end < groups.size() && (groups[end].code != 0 || groups[end].value != "ENDBLK")) {
            named = named || (groups[end].code == 2 && groups[end].value == name);
            ++end;
        }
        if (named) {
            return {groups.begin() + static_cast<std::ptrdiff_t>(i),
                    groups.begin() + static_cast<std::ptrdiff_t>(std::min(end + 2, groups.size()))};
        }
    }
    return {};
}

/// The value of the first group `code` after the group 0 of the entity with
/// handle `handle`.
std::optional<std::string> groupOf(const Groups& groups, std::string_view handle, int code) {
    for (std::size_t i = 0; i + 1 < groups.size(); ++i) {
        if (groups[i].code != 5 || groups[i].value != handle) {
            continue;
        }
        for (std::size_t j = i + 1; j < groups.size() && groups[j].code != 0; ++j) {
            if (groups[j].code == code) {
                return groups[j].value;
            }
        }
    }
    return std::nullopt;
}

/// Every handle (group 5) of `groups`.
std::multiset<std::string> handles(const Groups& groups) {
    std::multiset<std::string> out;
    for (const auto& group : groups) {
        if (group.code == 5) {
            out.insert(group.value);
        }
    }
    return out;
}

TEST(DxfDimension, TheFiveKindsAndAnOrdinateOneGoRoundUnchanged) {
    const std::string text = drawing(sixBlocks(), sixDimensions());
    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    EXPECT_TRUE(read.value().report.unhandled.empty());
    ASSERT_EQ(document.entityCount(), 6U);
    // The blocks *D… are not definitions: they belong to their dimension.
    EXPECT_TRUE(document.blocks().definitions().empty());

    const auto& linear = dimensionOf(document, 0x51);
    EXPECT_EQ(linear.definition.kind, core::DimensionKind::Linear);
    EXPECT_EQ(linear.definition.point14, (core::Vec2{40.0, 0.0}));
    EXPECT_EQ(linear.definition.linePoint, (core::Vec2{40.0, 10.0}));
    EXPECT_NEAR(linear.definition.rotation, std::numbers::pi / 6.0, 1e-15);
    EXPECT_EQ(linear.presentation.textPoint, (core::Vec2{20.0, 11.875}));
    EXPECT_EQ(linear.presentation.style, "ISO-25");
    EXPECT_EQ(linear.value.measurement, 40.0);
    EXPECT_FALSE(linear.presentation.userTextPosition);
    // The graphics: the LINE handled, with its handle; the POINT kept as read.
    ASSERT_EQ(linear.graphics.size(), 1U);
    EXPECT_EQ(linear.graphics[0].id.value, 0x113U);
    EXPECT_TRUE(std::holds_alternative<core::Line>(linear.graphics[0].entity));
    ASSERT_EQ(linear.rawGraphics.size(), 1U);
    EXPECT_EQ(linear.rawGraphics[0].id.value, 0x114U);

    EXPECT_EQ(dimensionOf(document, 0x52).definition.kind, core::DimensionKind::Aligned);
    const auto& angular = dimensionOf(document, 0x53);
    EXPECT_EQ(angular.definition.kind, core::DimensionKind::Angular);
    EXPECT_EQ(angular.definition.point16, (core::Vec2{235.0, 7.0}));
    const auto& diameter = dimensionOf(document, 0x54);
    EXPECT_EQ(diameter.definition.kind, core::DimensionKind::Diameter);
    EXPECT_EQ(diameter.definition.point15, (core::Vec2{338.0, 16.0}));
    EXPECT_TRUE(diameter.presentation.userTextPosition);
    const auto& radius = dimensionOf(document, 0x55);
    EXPECT_EQ(radius.definition.kind, core::DimensionKind::Radius);
    EXPECT_EQ(radius.definition.linePoint, (core::Vec2{280.0, 10.0}));
    const auto& ordinate = dimensionOf(document, 0x56);
    EXPECT_EQ(ordinate.definition.kind, core::DimensionKind::Other);
    EXPECT_EQ(ordinate.definition.dxfType, 6);
    EXPECT_EQ(ordinate.definition.point13, (core::Vec2{0.0, -250.0}));

    // Written back as read, blocks and records included (docs/adr/0018, point 6).
    EXPECT_EQ(difference(groupsOf(text), groupsOf(written(document))), "");
}

TEST(DxfDimension, AMissingOrSharedBlockKeepsTheDimensionUnhandled) {
    // *D9 is not in the file; *D1 is named by two dimensions.
    Groups entities = dimension(
        "61", "*D9", 32, {"40.0", "10.0"},
        concat(
            {{{100, "AcDbAlignedDimension"}}, point(13, "0.0", "0.0"), point(14, "40.0", "0.0")}));
    for (const char* handle : {"62", "63"}) {
        append(entities, dimension(handle, "*D1", 33, {"40.0", "10.0"},
                                   concat({{{100, "AcDbAlignedDimension"}},
                                           point(13, "0.0", "0.0"),
                                           point(14, "40.0", "0.0")})));
    }
    const std::string text = drawing({block(1)}, entities);
    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    EXPECT_EQ(document.entityCount(), 0U);
    EXPECT_EQ(read.value().report.unhandled.at("DIMENSION"), 3U);
    EXPECT_EQ(difference(groupsOf(text), groupsOf(written(document))), "");
}

TEST(DxfDimension, AnExtrusionDownOrADimensionInABlockStaysUnhandled) {
    const Groups down = dimension("61", "*D1", 32, {"40.0", "10.0"},
                                  concat({{{100, "AcDbAlignedDimension"}},
                                          point(13, "0.0", "0.0"),
                                          point(14, "40.0", "0.0"),
                                          {{210, "0.0"}, {220, "0.0"}, {230, "-1.0"}}}));
    // The block PLAIN holds a DIMENSION of *D2.
    Groups inner = dimension(
        "71", "*D2", 33, {"40.0", "10.0"},
        concat(
            {{{100, "AcDbAlignedDimension"}}, point(13, "0.0", "0.0"), point(14, "40.0", "0.0")}));
    inner[2].value = "70";  // owner: the record of PLAIN
    const Groups plain = concat({{{0, "BLOCK"},
                                  {5, "72"},
                                  {330, "70"},
                                  {100, "AcDbEntity"},
                                  {8, "0"},
                                  {100, "AcDbBlockBegin"},
                                  {2, "PLAIN"},
                                  {70, "     0"},
                                  {10, "0.0"},
                                  {20, "0.0"},
                                  {30, "0.0"},
                                  {3, "PLAIN"},
                                  {1, ""}},
                                 inner,
                                 {{0, "ENDBLK"},
                                  {5, "73"},
                                  {330, "70"},
                                  {100, "AcDbEntity"},
                                  {8, "0"},
                                  {100, "AcDbBlockEnd"}}});
    const std::string text = drawing({block(1), block(2)}, down, plain, blockRecord("70", "PLAIN"));
    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    EXPECT_EQ(document.entityCount(), 0U);
    EXPECT_EQ(read.value().report.unhandled.at("DIMENSION"), 1U);
    const auto* definition = document.blocks().find("PLAIN");
    ASSERT_NE(definition, nullptr);
    EXPECT_TRUE(definition->entities.empty());
    ASSERT_EQ(definition->rawEntities.size(), 1U);
    EXPECT_EQ(difference(groupsOf(text), groupsOf(written(document))), "");
}

TEST(DxfDimension, ANewDimensionTakesTheFirstFreeNameAndNewHandles) {
    const auto read = readDxf(drawing(sixBlocks(), sixDimensions()));
    ASSERT_TRUE(read.ok());
    core::Document document = read.value().document;
    const std::uint64_t first = document.nextId();
    core::Dimension made;
    made.definition.kind = core::DimensionKind::Aligned;
    made.definition.dxfType = 1;
    made.definition.point13 = {0.0, 100.0};
    made.definition.point14 = {30.0, 100.0};
    made.definition.linePoint = {30.0, 110.0};
    made.presentation.userTextPosition = true;
    const auto regenerated = core::regenerated(document, made);
    if (!regenerated) {
        FAIL() << "not regenerated";
    }
    core::Dimension dimension = *regenerated;
    ASSERT_FALSE(dimension.graphics.empty());
    // Two graphics kept as groups, with handle 0: one without a group 5 or
    // an owner, one with a group 5 "0" and an owner "0".
    const std::uint64_t after = dimension.graphics.back().order + 1;
    const core::RawEntity bare{core::EntityId{0}, after, "POINT",
                               Groups{{100, "AcDbEntity"},
                                      {8, "0"},
                                      {100, "AcDbPoint"},
                                      {10, "0.0"},
                                      {20, "100.0"},
                                      {30, "0.0"}}};
    const core::RawEntity zero{core::EntityId{0}, after + 1, "POINT",
                               Groups{{5, "0"},
                                      {330, "0"},
                                      {100, "AcDbEntity"},
                                      {8, "0"},
                                      {100, "AcDbPoint"},
                                      {10, "30.0"},
                                      {20, "100.0"},
                                      {30, "0.0"}}};
    dimension.rawGraphics = {bare, zero};
    const core::EntityId id = document.add(dimension);

    const std::string text = written(document);
    const Groups groups = groupsOf(text);
    const std::string handle = handleText(id.value);
    EXPECT_EQ(groupOf(groups, handle, 2), std::optional<std::string>{"*D7"});
    // 70: the type, 32 (the block is this dimension's only) and 128 (the
    // text where the user put it).
    EXPECT_EQ(groupOf(groups, handle, 70), std::optional<std::string>{"161"});
    const Groups made7 = blockNamed(groups, "*D7");
    ASSERT_FALSE(made7.empty());
    // Every handle of the file is used once; those of the new block and of
    // its graphics are new.
    const auto all = handles(groups);
    for (const auto& value : handles(made7)) {
        EXPECT_EQ(all.count(value), 1U) << value;
        EXPECT_GE(parseHandle(value).value_or(0), first) << value;
    }
    // The two POINT have a new handle right after their group 0 and the
    // record of the block as owner (the 330 of its BLOCK).
    ASSERT_GE(made7.size(), 3U);
    ASSERT_EQ(made7[2].code, 330);
    const std::string owner = made7[2].value;
    std::size_t points = 0;
    for (std::size_t i = 0; i + 2 < made7.size(); ++i) {
        if (made7[i].code != 0 || made7[i].value != "POINT") {
            continue;
        }
        ++points;
        EXPECT_EQ(made7[i + 1].code, 5);
        EXPECT_GE(parseHandle(made7[i + 1].value).value_or(0), first) << made7[i + 1].value;
        EXPECT_EQ(made7[i + 2].code, 330);
        EXPECT_EQ(made7[i + 2].value, owner);
    }
    EXPECT_EQ(points, 2U);
    EXPECT_EQ(std::ranges::count_if(made7,
                                    [](const core::RawGroup& group) {
                                        return (group.code == 5 || group.code == 330) &&
                                               group.value == "0";
                                    }),
              0);

    // Read again, it is handled, with its graphics.
    const auto again = readDxf(text);
    ASSERT_TRUE(again.ok());
    EXPECT_TRUE(again.value().report.unhandled.empty());
    const auto& reread = dimensionOf(again.value().document, id.value);
    EXPECT_EQ(reread.graphics.size(), dimension.graphics.size());
    ASSERT_EQ(reread.rawGraphics.size(), 2U);
    for (const auto& raw : reread.rawGraphics) {
        EXPECT_GE(raw.id.value, first);
        EXPECT_EQ(all.count(handleText(raw.id.value)), 1U);
    }
    EXPECT_NE(reread.rawGraphics[0].id, reread.rawGraphics[1].id);
}

TEST(DxfDimension, ANewDrawingWritesTheBlockOfItsDimension) {
    core::Document document = core::Document::newDrawing();
    core::Dimension made;
    made.definition.point14 = {40.0, 0.0};
    made.definition.linePoint = {40.0, 10.0};
    const auto regenerated = core::regenerated(document, made);
    if (!regenerated) {
        FAIL() << "not regenerated";
    }
    const core::EntityId id = document.add(*regenerated);
    const std::string text = written(document);
    const Groups groups = groupsOf(text);
    EXPECT_EQ(groupOf(groups, handleText(id.value), 2), std::optional<std::string>{"*D1"});
    EXPECT_FALSE(blockNamed(groups, "*D1").empty());
    const auto again = readDxf(text);
    ASSERT_TRUE(again.ok());
    EXPECT_EQ(again.value().document.entityCount(), 1U);
    EXPECT_EQ(difference(groups, groupsOf(written(again.value().document))), "");
}

TEST(DxfDimension, ACopyTakesItsOwnBlockAndTheErasedOneLeavesWithItsBlock) {
    const auto read = readDxf(drawing(sixBlocks(), sixDimensions()));
    ASSERT_TRUE(read.ok());
    core::Document document = read.value().document;
    // A copy of the linear dimension 51: the same entity and extras.
    core::EntityRecord copy = *document.find(core::EntityId{0x51});
    copy.id = core::EntityId{document.nextId()};
    copy.order = document.entities().back().order + 1;
    document.setNextId(copy.id.value + 1);
    document.insert(copy);
    // The radial one is erased.
    ASSERT_TRUE(document.remove(core::EntityId{0x55}).has_value());

    const std::string text = written(document);
    const Groups groups = groupsOf(text);
    EXPECT_EQ(groupOf(groups, "51", 2), std::optional<std::string>{"*D1"});
    // *D5 is free again: the copy takes it.
    EXPECT_EQ(groupOf(groups, handleText(copy.id.value), 2), std::optional<std::string>{"*D5"});
    const Groups original = blockNamed(groups, "*D1");
    const Groups copied = blockNamed(groups, "*D5");
    ASSERT_FALSE(original.empty());
    ASSERT_FALSE(copied.empty());
    EXPECT_EQ(handles(original), (std::multiset<std::string>{"111", "112", "113", "114"}));
    for (const auto& value : handles(groups)) {
        EXPECT_EQ(handles(groups).count(value), 1U) << value;
    }
    // Its entities, the POINT kept as read too, have new handles and the
    // record of the new block as owner (the 330 of its BLOCK).
    ASSERT_GE(copied.size(), 3U);
    ASSERT_EQ(copied[2].code, 330);
    const std::string owner = copied[2].value;
    EXPECT_NE(owner, "110");
    std::size_t entities = 0;
    for (std::size_t i = 0; i + 2 < copied.size(); ++i) {
        if (copied[i].code == 0 && (copied[i].value == "POINT" || copied[i].value == "LINE")) {
            ++entities;
            EXPECT_EQ(copied[i + 2].code, 330);
            EXPECT_EQ(copied[i + 2].value, owner);
        }
    }
    EXPECT_EQ(entities, 2U);

    const auto again = readDxf(text);
    ASSERT_TRUE(again.ok());
    EXPECT_TRUE(again.value().report.unhandled.empty());
    EXPECT_EQ(again.value().document.entityCount(), 6U);
    EXPECT_EQ(dimensionOf(again.value().document, copy.id.value).rawGraphics.size(), 1U);
}

TEST(DxfDimension, OverridesAreReadKeptAndWritten) {
    // DSTYLE: DIMTXT (140) 3.5, essential; DIMATFIT (289) 1, not essential.
    const Groups extended{{1001, "ACAD"}, {1000, "DSTYLE"}, {1002, "{"},      {1070, "   140"},
                          {1040, "3.5"},  {1070, "   289"}, {1070, "     1"}, {1002, "}"}};
    const Groups entities = dimension("51", "*D1", 32, {"40.0", "10.0"},
                                      concat({{{100, "AcDbAlignedDimension"}},
                                              point(13, "0.0", "0.0"),
                                              point(14, "40.0", "0.0"),
                                              {{100, "AcDbRotatedDimension"}}}),
                                      extended);
    const std::string text = drawing({block(1)}, entities);
    const auto read = readDxf(text);
    ASSERT_TRUE(read.ok());
    core::Document document = read.value().document;
    const auto& dimension = dimensionOf(document, 0x51);
    EXPECT_EQ(dimension.presentation.overrides,
              (std::vector<std::pair<std::string, std::string>>{{"DIMTXT", "3.5"}}));
    EXPECT_EQ(difference(groupsOf(text), groupsOf(written(document))), "");

    // A changed override is written; the other stays.
    core::Dimension changed = dimension;
    changed.presentation.overrides = {{"DIMTXT", "5"}};
    document.replaceEntity(core::EntityId{0x51}, changed);
    const Groups groups = groupsOf(written(document));
    const auto dstyle = std::ranges::find(groups, core::RawGroup{1000, "DSTYLE"});
    ASSERT_NE(dstyle, groups.end());
    const Groups list(dstyle, std::min(dstyle + 7, groups.end()));
    EXPECT_EQ(list, (Groups{{1000, "DSTYLE"},
                            {1002, "{"},
                            {1070, "140"},
                            {1040, "5"},
                            {1070, "289"},
                            {1070, "     1"},
                            {1002, "}"}}));
}

}  // namespace
}  // namespace tg::io
