// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Layers, linetypes, header variables and entity properties in the DXF
// (docs/adr/0023, 0024 and 0025; package 0.6-P10).

#include "dxf_tables.hpp"

#include "tg/core/line.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <utility>
#include <vector>

namespace tg::io {
namespace {

using core::RawGroups;

/// A linetype record, from group 0 on.
RawGroups linetype(std::string_view handle, std::string_view name, const RawGroups& elements = {},
                   std::string_view description = "") {
    RawGroups groups{{0, "LTYPE"},
                     {5, std::string{handle}},
                     {330, "5"},
                     {100, "AcDbSymbolTableRecord"},
                     {100, "AcDbLinetypeTableRecord"},
                     {2, std::string{name}},
                     {70, "     0"},
                     {3, std::string{description}},
                     {72, "    65"}};
    groups.insert(groups.end(), elements.begin(), elements.end());
    return groups;
}

/// A layer record, from group 0 on; `body` follows the name.
RawGroups layer(std::string_view handle, std::string_view name, const RawGroups& body) {
    RawGroups groups{{0, "LAYER"},
                     {5, std::string{handle}},
                     {330, "2"},
                     {100, "AcDbSymbolTableRecord"},
                     {100, "AcDbLayerTableRecord"},
                     {2, std::string{name}}};
    groups.insert(groups.end(), body.begin(), body.end());
    return groups;
}

/// The three linetypes every drawing has, and a dashed one with handle 17.
RawGroups standardLinetypes() {
    RawGroups groups;
    for (const auto& record :
         {linetype("14", "ByBlock", {{73, "     0"}, {40, "0.0"}}),
          linetype("15", "ByLayer", {{73, "     0"}, {40, "0.0"}}),
          linetype("16", "Continuous", {{73, "     0"}, {40, "0.0"}}, "Solid line"),
          linetype("17", "DASHED",
                   {{73, "     2"},
                    {40, "15.0"},
                    {49, "12.0"},
                    {74, "     0"},
                    {49, "-3.0"},
                    {74, "     0"}},
                   "Dashed __ __")}) {
        groups.insert(groups.end(), record.begin(), record.end());
    }
    return groups;
}

/// A DXF R2018 file with these tables, header variables and entities.
std::string drawing(const RawGroups& linetypes, const RawGroups& layers, const RawGroups& entities,
                    const RawGroups& header = {}) {
    RawGroups groups{{0, "SECTION"},      {2, "HEADER"},    {9, "$ACADVER"},  {1, "AC1032"},
                     {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}, {9, "$HANDSEED"}, {5, "100"}};
    groups.insert(groups.end(), header.begin(), header.end());
    groups.insert(groups.end(), {{0, "ENDSEC"}, {0, "SECTION"}, {2, "TABLES"}});
    if (!linetypes.empty()) {
        groups.insert(groups.end(), {{0, "TABLE"},
                                     {2, "LTYPE"},
                                     {5, "5"},
                                     {330, "0"},
                                     {100, "AcDbSymbolTable"},
                                     {70, "     2"}});
        groups.insert(groups.end(), linetypes.begin(), linetypes.end());
        groups.push_back({0, "ENDTAB"});
    }
    if (!layers.empty()) {
        groups.insert(groups.end(), {{0, "TABLE"},
                                     {2, "LAYER"},
                                     {5, "2"},
                                     {330, "0"},
                                     {100, "AcDbSymbolTable"},
                                     {70, "     9"}});
        groups.insert(groups.end(), layers.begin(), layers.end());
        groups.push_back({0, "ENDTAB"});
    }
    groups.insert(groups.end(), {{0, "ENDSEC"}, {0, "SECTION"}, {2, "ENTITIES"}});
    groups.insert(groups.end(), entities.begin(), entities.end());
    groups.insert(groups.end(), {{0, "ENDSEC"}, {0, "EOF"}});
    return writeGroups(groups, TextEncoding::Utf8);
}

/// A LINE with these AcDbEntity groups.
RawGroups line(std::string_view handle, const RawGroups& common) {
    RawGroups groups{{0, "LINE"}, {5, std::string{handle}}, {100, "AcDbEntity"}};
    groups.insert(groups.end(), common.begin(), common.end());
    groups.insert(groups.end(), {{100, "AcDbLine"},
                                 {10, "0.0"},
                                 {20, "0.0"},
                                 {30, "0.0"},
                                 {11, "1.0"},
                                 {21, "0.0"},
                                 {31, "0.0"}});
    return groups;
}

ReadResult readOk(const std::string& bytes) {
    auto result = readDxf(bytes);
    EXPECT_TRUE(result.ok()) << (result.ok() ? "" : result.error().message);
    if (!result.ok()) {
        ADD_FAILURE() << "line " << result.error().line << ": " << result.error().message;
    }
    return result.ok() ? std::move(result).value() : ReadResult{};
}

std::string writeOk(const core::Document& document, DxfVersion version = DxfVersion::R2018) {
    auto written = writeDxf(document, version);
    EXPECT_TRUE(written.ok()) << (written.ok() ? "" : written.error().message);
    return written.ok() ? std::move(written).value() : std::string{};
}

RawGroups groupsOf(const std::string& bytes) {
    auto groups = readGroups(bytes);
    EXPECT_TRUE(groups.ok());
    return groups.ok() ? std::move(groups).value().groups : RawGroups{};
}

/// The groups of the first object of type `type` named `name` (group 2,
/// or 8 for an entity named by its handle with `byHandle`), from group 0.
RawGroups objectGroups(const std::string& bytes, std::string_view type, std::string_view name,
                       bool byHandle = false) {
    const auto all = groupsOf(bytes);
    for (std::size_t i = 0; i < all.size(); ++i) {
        if (all[i].code != 0 || all[i].value != type) {
            continue;
        }
        std::size_t end = i + 1;
        while (end < all.size() && all[end].code != 0) {
            ++end;
        }
        const RawGroups object(all.begin() + static_cast<std::ptrdiff_t>(i),
                               all.begin() + static_cast<std::ptrdiff_t>(end));
        const int code = byHandle ? 5 : 2;
        if (std::ranges::any_of(object, [&](const core::RawGroup& group) {
                return group.code == code && group.value == name;
            })) {
            return object;
        }
    }
    return {};
}

const core::Layer& layerNamed(const core::Document& document, std::string_view name) {
    const core::Layer* found = document.findLayer(name);
    EXPECT_NE(found, nullptr) << name;
    static const core::Layer kNone;
    return found != nullptr ? *found : kNone;
}

// --- Tables ----------------------------------------------------------------

TEST(DxfTables, LayerGroupsEnterTheModelAndAreWrittenAgainAsRead) {
    const RawGroups layers = [] {
        RawGroups groups;
        for (const auto& record :
             {layer("10", "0",
                    {{70, "     0"},
                     {62, "     7"},
                     {6, "Continuous"},
                     {370, "    -3"},
                     {390, "F"},
                     {347, "98"}}),
              layer("11", "Off", {{70, "     0"}, {62, "    -3"}, {6, "DASHED"}, {370, "    25"}}),
              layer("12", "Frozen", {{70, "    17"}, {62, "     1"}, {6, "Continuous"}}),
              layer("13", "Locked",
                    {{70, "     4"},
                     {62, "     5"},
                     {6, "Continuous"},
                     {290, "     0"},
                     {370, "   211"}}),
              layer("18", "Rgb",
                    {{70, "     0"},
                     {62, "    30"},
                     {420, "  4745408"},
                     {6, "Continuous"},
                     {370, "     0"}}),
              layer("1A", "Xdata",
                    {{70, "0"},
                     {62, "2"},
                     {6, "Continuous"},
                     {1001, "APP"},
                     {1000, "Xdata"},
                     {1070, "62"}})}) {
            groups.insert(groups.end(), record.begin(), record.end());
        }
        return groups;
    }();
    const std::string bytes = drawing(standardLinetypes(), layers, line("30", {{8, "Off"}}));
    const auto read = readOk(bytes);
    EXPECT_TRUE(read.report.warnings.empty());
    const auto& document = read.document;
    ASSERT_EQ(document.layers().size(), 6U);
    ASSERT_EQ(document.linetypes().size(), 4U);
    EXPECT_EQ(document.layers()[0].name, "0");
    EXPECT_EQ(document.layers()[0].id.value, 0x10U);
    EXPECT_EQ(document.layers()[0].lineweight, core::lineweight::kDefault);

    const auto& off = layerNamed(document, "Off");
    EXPECT_FALSE(off.on);
    EXPECT_EQ(off.color, core::Color::fromIndex(3));
    EXPECT_EQ(document.linetype(off.linetype)->name, "DASHED");
    EXPECT_EQ(off.lineweight, 25);

    const auto& frozen = layerNamed(document, "Frozen");
    EXPECT_TRUE(frozen.frozen);
    EXPECT_FALSE(frozen.locked);
    const auto& locked = layerNamed(document, "Locked");
    EXPECT_TRUE(locked.locked);
    EXPECT_FALSE(locked.plot);
    EXPECT_EQ(locked.lineweight, 211);

    // With a 420 the colour is RGB and the 62 is ignored (docs/adr/0024).
    EXPECT_EQ(layerNamed(document, "Rgb").color, core::Color::fromRgb({72, 104, 192}));

    const auto& dashed = *document.findLinetype("dashed");
    EXPECT_EQ(dashed.id.value, 0x17U);
    EXPECT_EQ(dashed.description, "Dashed __ __");
    EXPECT_EQ(dashed.pattern.elements, (std::vector<double>{12.0, -3.0}));

    // Written again group for group, spelling included; only $HANDSEED may
    // change.
    const auto written = writeOk(document);
    EXPECT_EQ(objectGroups(written, "LAYER", "Rgb"), objectGroups(bytes, "LAYER", "Rgb"));
    auto before = groupsOf(bytes);
    auto after = groupsOf(written);
    ASSERT_EQ(before.size(), after.size());
    for (std::size_t i = 0; i < before.size(); ++i) {
        if (i == 0 || before[i - 1].value != "$HANDSEED") {
            EXPECT_EQ(before[i], after[i]) << i << " " << before[i].value;
        }
    }
}

TEST(DxfTables, AnRgbWithoutIndexGetsTheNearestOne) {
    // A layer with a 420 and no 62: RGB, on; written with the nearest index
    // as every RGB (docs/adr/0024, point 2).
    RawGroups layers = layer("10", "0", {{70, "0"}, {62, "7"}, {6, "Continuous"}});
    const auto blue = layer("19", "Blue", {{70, "0"}, {420, "255"}, {6, "Continuous"}});
    layers.insert(layers.end(), blue.begin(), blue.end());
    const auto read = readOk(drawing(standardLinetypes(), layers, {}));
    EXPECT_EQ(layerNamed(read.document, "Blue").color, core::Color::fromRgb({0, 0, 255}));
    EXPECT_TRUE(layerNamed(read.document, "Blue").on);
    const auto groups = objectGroups(writeOk(read.document), "LAYER", "Blue");
    EXPECT_NE(std::ranges::search(groups, RawGroups{{62, "5"}, {420, "255"}}).begin(),
              groups.end());
}

TEST(DxfTables, ChangedLayersAreWrittenFromTheModel) {
    RawGroups layers = layer("10", "0", {{70, "     0"}, {62, "     7"}, {6, "Continuous"}});
    const RawGroups rgb = layer(
        "18", "Rgb",
        {{70, "    16"}, {62, "    30"}, {420, "  4745408"}, {6, "Continuous"}, {370, "    -3"}});
    layers.insert(layers.end(), rgb.begin(), rgb.end());
    auto read = readOk(drawing(standardLinetypes(), layers, {}));
    auto& document = read.document;
    core::Layer changed = layerNamed(document, "Rgb");
    changed.on = false;
    changed.frozen = true;
    changed.locked = true;
    changed.plot = false;
    changed.lineweight = 50;
    changed.linetype = document.findLinetype("DASHED")->id;
    document.replaceLayer(changed);
    // An RGB still the one read keeps the 62 read with it, negative for off.
    EXPECT_EQ(objectGroups(writeOk(document), "LAYER", "Rgb"),
              (RawGroups{{0, "LAYER"},
                         {5, "18"},
                         {330, "2"},
                         {100, "AcDbSymbolTableRecord"},
                         {100, "AcDbLayerTableRecord"},
                         {2, "Rgb"},
                         {70, "21"},  // bit 16 kept as read, 1 frozen, 4 locked
                         {62, "-30"},
                         {420, "  4745408"},
                         {6, "DASHED"},
                         {290, "0"},
                         {370, "50"}}));
    // A new RGB takes the nearest index (docs/adr/0024, point 4); an index
    // drops the 420.
    changed.on = true;
    changed.color = core::Color::fromRgb({255, 0, 0});
    document.replaceLayer(changed);
    auto groups = objectGroups(writeOk(document), "LAYER", "Rgb");
    EXPECT_NE(std::ranges::find(groups, core::RawGroup{62, "1"}), groups.end());
    EXPECT_NE(std::ranges::find(groups, core::RawGroup{420, "16711680"}), groups.end());
    changed.color = core::Color::fromIndex(4);
    document.replaceLayer(changed);
    groups = objectGroups(writeOk(document), "LAYER", "Rgb");
    EXPECT_NE(std::ranges::find(groups, core::RawGroup{62, "4"}), groups.end());
    EXPECT_EQ(std::ranges::find(groups, 420, &core::RawGroup::code), groups.end());
}

TEST(DxfTables, NewRecordsGoAtTheEndOfTheirTableAndTheCountFollows) {
    auto read = readOk(drawing(standardLinetypes(),
                               layer("10", "0", {{70, "0"}, {62, "7"}, {6, "Continuous"}}), {}));
    auto& document = read.document;
    core::Layer walls;
    walls.name = "Walls";
    walls.color = core::Color::fromRgb({1, 2, 3});
    walls.linetype = document.continuousLinetype();
    walls.lineweight = 35;
    const core::LayerId id = document.addLayer(walls);
    const core::LinetypeId hidden = document.addLinetype(
        core::Linetype{{}, "HIDDEN", "Hidden __ __", core::LinetypePattern{{6.35, -3.175}}, {}});
    const auto written = writeOk(document);
    const auto all = groupsOf(written);
    // The counts change by the records added: AutoCAD's 2 for four linetypes
    // becomes 3, 9 layers become 10.
    EXPECT_NE(
        std::ranges::search(
            all, RawGroups{{2, "LTYPE"}, {5, "5"}, {330, "0"}, {100, "AcDbSymbolTable"}, {70, "3"}})
            .begin(),
        all.end());
    EXPECT_NE(std::ranges::search(all, RawGroups{{100, "AcDbSymbolTable"}, {70, "10"}}).begin(),
              all.end());
    EXPECT_EQ(objectGroups(written, "LAYER", "Walls"),
              (RawGroups{{0, "LAYER"},
                         {5, handleText(id.value)},
                         {330, "2"},
                         {100, "AcDbSymbolTableRecord"},
                         {100, "AcDbLayerTableRecord"},
                         {2, "Walls"},
                         {70, "0"},
                         {62, std::to_string(core::nearestAci({1, 2, 3}))},
                         {420, "66051"},
                         {6, "Continuous"},
                         {370, "35"}}));
    EXPECT_EQ(objectGroups(written, "LTYPE", "HIDDEN"),
              (RawGroups{{0, "LTYPE"},
                         {5, handleText(hidden.value)},
                         {330, "5"},
                         {100, "AcDbSymbolTableRecord"},
                         {100, "AcDbLinetypeTableRecord"},
                         {2, "HIDDEN"},
                         {70, "0"},
                         {3, "Hidden __ __"},
                         {72, "65"},
                         {73, "2"},
                         {40, formatNumber(core::LinetypePattern{{6.35, -3.175}}.length())},
                         {49, "6.35"},
                         {74, "0"},
                         {49, "-3.175"},
                         {74, "0"}}));
    // The new records come after those read, which stay at their place.
    const auto reread = readOk(written);
    EXPECT_EQ(reread.document.layers().back().name, "Walls");
    EXPECT_EQ(reread.document.linetypes().back().name, "HIDDEN");
    EXPECT_EQ(writeOk(reread.document), written);
    // Removing a record lowers the count and leaves the others in order.
    ASSERT_TRUE(document.removeLayer(id).has_value());
    const auto removed = groupsOf(writeOk(document));
    EXPECT_NE(
        std::ranges::search(removed, RawGroups{{100, "AcDbSymbolTable"}, {70, "     9"}}).begin(),
        removed.end());
}

TEST(DxfTables, ComplexLinetypesKeepWhatTheModelDoesNotRepresent) {
    // A linetype with a text element (74 = 2): the model has dashes, gaps
    // and dots, the rest stays to write it again (docs/adr/0025, point 1).
    const RawGroups gas = linetype("20", "GAS_LINE",
                                   {{73, "     3"},
                                    {40, "24.13"},
                                    {49, "12.7"},
                                    {74, "     0"},
                                    {49, "-5.08"},
                                    {74, "     2"},
                                    {75, "     0"},
                                    {340, "11"},
                                    {46, "2.54"},
                                    {50, "0.0"},
                                    {44, "-2.54"},
                                    {45, "-1.27"},
                                    {9, "GAS"},
                                    {49, "-6.35"},
                                    {74, "     0"}},
                                   "Gas line ----GAS----");
    RawGroups linetypes = standardLinetypes();
    linetypes.insert(linetypes.end(), gas.begin(), gas.end());
    const std::string bytes = drawing(linetypes, {}, {});
    const auto read = readOk(bytes);
    const core::Linetype* found = read.document.findLinetype("gas_line");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->pattern.elements, (std::vector<double>{12.7, -5.08, -6.35}));
    EXPECT_EQ(found->description, "Gas line ----GAS----");
    EXPECT_EQ(objectGroups(writeOk(read.document), "LTYPE", "GAS_LINE"), gas);
}

TEST(DxfTables, FileWithoutTablesStaysWithoutRecords) {
    // Layer 0 and ByLayer only: no record, and the handles of the file are
    // those of a file read before the tables (docs/verifica/domande.md).
    RawGroups groups{{0, "SECTION"}, {2, "HEADER"},    {9, "$ACADVER"},
                     {1, "AC1032"},  {9, "$HANDSEED"}, {5, "100"},
                     {0, "ENDSEC"},  {0, "SECTION"},   {2, "ENTITIES"}};
    const RawGroups entity = line("30", {{8, "0"}, {62, "1"}, {370, "13"}});
    groups.insert(groups.end(), entity.begin(), entity.end());
    groups.insert(groups.end(), {{0, "ENDSEC"}, {0, "EOF"}});
    const std::string bytes = writeGroups(groups, TextEncoding::Utf8);
    auto read = readOk(bytes);
    auto& document = read.document;
    EXPECT_TRUE(document.layers().empty());
    EXPECT_TRUE(document.linetypes().empty());
    ASSERT_EQ(document.entityCount(), 1U);
    EXPECT_EQ(document.entities()[0].properties.color, core::Color::fromIndex(1));
    EXPECT_EQ(document.entities()[0].properties.lineweight, 13);
    EXPECT_EQ(writeOk(document), bytes);
    EXPECT_EQ(document.add(core::Line{}).value, 0x100U);
}

TEST(DxfTables, ImpliedLinetypesKeepTheSpellingOfTheFile) {
    // FreeCAD writes layers with CONTINUOUS and no LTYPE table: the implied
    // record is Continuous, the layer writes CONTINUOUS again.
    const std::string bytes =
        drawing({}, layer("10", "0", {{70, "     0"}, {62, "     7"}, {6, "CONTINUOUS"}}),
                line("30", {{8, "0"}}));
    const auto read = readOk(bytes);
    EXPECT_TRUE(read.report.warnings.empty());
    ASSERT_NE(read.document.findLinetype("Continuous"), nullptr);
    EXPECT_EQ(read.document.findLinetype("Continuous")->name, "Continuous");
    EXPECT_EQ(objectGroups(writeOk(read.document), "LAYER", "0"),
              objectGroups(bytes, "LAYER", "0"));
}

TEST(DxfTables, LinetypesTheFileLacksDoNotSpendTheHandleSeed) {
    // The standard linetypes exist only in the model: the entity drawn after
    // OPEN still takes the first free handle of the file.
    const std::string bytes =
        drawing({}, layer("10", "0", {{70, "     0"}, {62, "     7"}, {6, "CONTINUOUS"}}),
                line("30", {{8, "0"}}));
    auto read = readOk(bytes);
    const std::uint64_t seed = read.document.nextId();
    std::vector<std::uint64_t> ids;
    for (const char* name : {"ByBlock", "ByLayer", "Continuous"}) {
        const core::Linetype* found = read.document.findLinetype(name);
        ASSERT_NE(found, nullptr);
        EXPECT_LT(found->id.value, seed);
        EXPECT_NE(found->id.value, 0x10U);
        EXPECT_NE(found->id.value, 0x30U);
        ids.push_back(found->id.value);
    }
    std::sort(ids.begin(), ids.end());
    EXPECT_EQ(std::adjacent_find(ids.begin(), ids.end()), ids.end());
    EXPECT_EQ(read.document.add(core::Line{}).value, seed);
    EXPECT_EQ(read.document.nextId(), seed + 1);
}

TEST(DxfTables, EntitiesWithoutHandleDoNotClashWithTheImpliedLinetypes) {
    // An entity without a handle takes one from the counter, above the seed:
    // the implied linetypes must stay below it.
    RawGroups entity = line("30", {{8, "0"}});
    entity.erase(std::remove_if(entity.begin(), entity.end(),
                                [](const core::RawGroup& group) { return group.code == 5; }),
                 entity.end());
    auto read = readOk(drawing({}, layer("10", "0", {{70, "     0"}, {6, "CONTINUOUS"}}), entity));
    const std::uint64_t seed = read.document.nextId() - 1;
    for (const char* name : {"ByBlock", "ByLayer", "Continuous"}) {
        ASSERT_NE(read.document.findLinetype(name), nullptr);
        EXPECT_LT(read.document.findLinetype(name)->id.value, seed);
    }
}

// --- Entities --------------------------------------------------------------

TEST(DxfTables, EntityPropertiesEnterTheModel) {
    const RawGroups layers = [] {
        RawGroups groups = layer("10", "0", {{70, "0"}, {62, "7"}, {6, "Continuous"}});
        const auto walls = layer("11", "Walls", {{70, "0"}, {62, "1"}, {6, "Continuous"}});
        groups.insert(groups.end(), walls.begin(), walls.end());
        return groups;
    }();
    RawGroups entities;
    for (const auto& entity :
         {line("30", {{8, "Walls"}, {6, "DASHED"}, {62, "     5"}, {370, "    50"}, {48, "2.5"}}),
          line("31", {{8, "0"}, {62, "     0"}, {370, "    -2"}}),
          line("32", {{8, "0"}, {62, "    30"}, {420, "  4745408"}, {60, "1"}}),
          line("33", {{8, "0"}, {420, "255"}}),
          line("34", {{8, "0"},
                      {6, "ByLayer"},
                      {62, "   256"},
                      {370, "    -1"},
                      {48, "1.0"},
                      {430, "BOOK$RED"}})}) {
        entities.insert(entities.end(), entity.begin(), entity.end());
    }
    const std::string bytes = drawing(standardLinetypes(), layers, entities);
    const auto read = readOk(bytes);
    const auto& document = read.document;
    const auto properties = [&](std::uint64_t handle) {
        return document.find(core::EntityId{handle})->properties;
    };
    const auto first = properties(0x30);
    EXPECT_EQ(document.layer(first.layer)->name, "Walls");
    EXPECT_EQ(document.linetype(first.linetype)->name, "DASHED");
    EXPECT_EQ(first.color, core::Color::fromIndex(5));
    EXPECT_EQ(first.lineweight, 50);
    EXPECT_EQ(first.linetypeScale, 2.5);
    EXPECT_TRUE(document.find(core::EntityId{0x30})->extras.common.empty());

    const auto second = properties(0x31);
    EXPECT_EQ(second.color, core::Color::byBlock());
    EXPECT_EQ(second.lineweight, core::lineweight::kByBlock);
    EXPECT_EQ(second.layer, document.layerZero());
    EXPECT_EQ(second.linetype, document.byLayerLinetype());

    EXPECT_EQ(properties(0x32).color, core::Color::fromRgb({72, 104, 192}));
    EXPECT_EQ(properties(0x33).color, core::Color::fromRgb({0, 0, 255}));
    // The defaults written anyway and the other groups stay where they are.
    const auto defaults = properties(0x34);
    EXPECT_EQ(defaults.color, core::Color::byLayer());
    EXPECT_EQ(defaults.linetype, document.byLayerLinetype());
    EXPECT_EQ(defaults.lineweight, core::lineweight::kByLayer);
    EXPECT_EQ(defaults.linetypeScale, 1.0);
    EXPECT_EQ(
        document.find(core::EntityId{0x34})->extras.common,
        (RawGroups{
            {6, "ByLayer"}, {62, "   256"}, {370, "    -1"}, {48, "1.0"}, {430, "BOOK$RED"}}));
    EXPECT_EQ(document.find(core::EntityId{0x32})->extras.common,
              (RawGroups{{62, "    30"}, {60, "1"}}));

    // Written in the order of the DXF reference, the 62 of an RGB as read.
    const auto written = writeOk(document);
    const auto common = [&](std::string_view handle) {
        const auto groups = objectGroups(written, "LINE", handle, true);
        const auto begin = std::ranges::find(groups, core::RawGroup{100, "AcDbEntity"});
        const auto end = std::ranges::find(groups, core::RawGroup{100, "AcDbLine"});
        return RawGroups(begin + 1, end);
    };
    EXPECT_EQ(common("30"),
              (RawGroups{{8, "Walls"}, {6, "DASHED"}, {62, "5"}, {370, "50"}, {48, "2.5"}}));
    EXPECT_EQ(common("31"), (RawGroups{{8, "0"}, {62, "0"}, {370, "-2"}}));
    EXPECT_EQ(common("32"), (RawGroups{{8, "0"}, {62, "    30"}, {60, "1"}, {420, "4745408"}}));
    EXPECT_EQ(common("33"), (RawGroups{{8, "0"}, {62, "5"}, {420, "255"}}));
    EXPECT_EQ(common("34"), (RawGroups{{8, "0"},
                                       {6, "ByLayer"},
                                       {62, "   256"},
                                       {370, "    -1"},
                                       {48, "1.0"},
                                       {430, "BOOK$RED"}}));
}

TEST(DxfTables, ChangedEntityPropertiesReplaceTheGroupsRead) {
    auto read = readOk(drawing(
        standardLinetypes(), layer("10", "0", {{70, "0"}, {62, "7"}, {6, "Continuous"}}),
        line("30",
             {{8, "0"}, {6, "BYLAYER"}, {62, "    30"}, {420, "  4745408"}, {370, "    -1"}})));
    auto& document = read.document;
    core::EntityProperties properties = document.find(core::EntityId{0x30})->properties;
    properties.color = core::Color::byLayer();
    properties.linetype = document.findLinetype("DASHED")->id;
    properties.lineweight = 9;
    document.setProperties(core::EntityId{0x30}, properties);
    const auto groups = objectGroups(writeOk(document), "LINE", "30", true);
    const auto begin = std::ranges::find(groups, core::RawGroup{100, "AcDbEntity"});
    const auto end = std::ranges::find(groups, core::RawGroup{100, "AcDbLine"});
    EXPECT_EQ(RawGroups(begin + 1, end), (RawGroups{{8, "0"}, {6, "DASHED"}, {370, "9"}}));
}

TEST(DxfTables, NamesSpeltOtherwiseAreWrittenAsRead) {
    const std::string bytes =
        drawing(standardLinetypes(), layer("10", "Walls", {{70, "0"}, {62, "1"}, {6, "dashed"}}),
                line("30", {{8, "WALLS"}, {6, "Dashed"}}));
    const auto read = readOk(bytes);
    EXPECT_TRUE(read.report.warnings.empty());
    const auto& entity = *read.document.find(core::EntityId{0x30});
    EXPECT_EQ(read.document.layer(entity.properties.layer)->name, "Walls");
    EXPECT_EQ(read.document.linetype(entity.properties.linetype)->name, "DASHED");
    const auto written = writeOk(read.document);
    EXPECT_EQ(objectGroups(written, "LINE", "30", true), objectGroups(bytes, "LINE", "30", true));
    EXPECT_EQ(objectGroups(written, "LAYER", "Walls"), objectGroups(bytes, "LAYER", "Walls"));
}

TEST(DxfTables, EntitiesWithInvalidPropertiesStayUnhandled) {
    for (const RawGroups& common :
         {RawGroups{{8, "0"}, {62, "257"}}, RawGroups{{8, "0"}, {62, "-1"}},
          RawGroups{{8, "0"}, {420, "16777216"}}, RawGroups{{8, "0"}, {420, "-1"}},
          RawGroups{{8, "0"}, {48, "0.0"}}, RawGroups{{8, "0"}, {48, "-2"}}, RawGroups{{8, "a*b"}},
          RawGroups{{8, "0"}, {6, "a|b"}}}) {
        const auto read = readOk(drawing(standardLinetypes(), {}, line("30", common)));
        EXPECT_EQ(read.document.entityCount(), 0U);
        EXPECT_EQ(read.report.unhandled.at("LINE"), 1);
        ASSERT_EQ(read.document.rawEntities().size(), 1U);
    }
}

// --- Inconsistent files (docs/adr/0023, point 6; docs/adr/0025, point 6) --

TEST(DxfTables, InconsistentFilesAreCompletedAndCounted) {
    RawGroups layers = layer("10", "0", {{70, "0"}, {62, "7"}, {6, "Continuous"}});
    for (const auto& record :
         {layer("11", "Walls", {{70, "0"}, {62, "1"}, {6, "Continuous"}, {370, "45"}}),
          layer("12", "WALLS", {{70, "0"}, {62, "2"}, {6, "Continuous"}}),
          layer("13", "Bad", {{70, "0"}, {62, "300"}, {6, "Continuous"}}),
          layer("1C", "Bylayer", {{70, "0"}, {62, "3"}, {6, "ByLayer"}}),
          layer("1B", "Unknown", {{70, "0"}, {62, "3"}, {6, "MissingType"}})}) {
        layers.insert(layers.end(), record.begin(), record.end());
    }
    RawGroups linetypes = standardLinetypes();
    const auto twice = linetype("21", "dashed", {{73, "0"}, {40, "0.0"}});
    linetypes.insert(linetypes.end(), twice.begin(), twice.end());
    RawGroups entities;
    for (const auto& entity :
         {line("30", {{8, "Missing"}, {370, "7"}}), line("31", {{8, "missing"}, {6, "Other"}}),
          line("32", {{8, "Second"}, {370, "1000"}})}) {
        entities.insert(entities.end(), entity.begin(), entity.end());
    }
    const auto read =
        readOk(drawing(linetypes, layers, entities,
                       {{9, "$CLAYER"}, {8, "Current"}, {9, "$LWDEFAULT"}, {370, "26"}}));
    std::vector<std::string> warnings = read.report.warnings;
    std::ranges::sort(warnings);
    EXPECT_EQ(
        warnings,
        (std::vector<std::string>{
            "1 duplicate layer kept as unhandled data.",
            "1 duplicate linetype kept as unhandled data.",
            "1 invalid layer kept as unhandled data.", "1 layer linetype replaced by Continuous.",
            "2 missing linetypes added.", "3 missing layers added.", "4 lineweights rounded."}));
    const auto& document = read.document;
    // Created with the values of a new layer 0, at the end of the table.
    const auto& missing = layerNamed(document, "MISSING");
    EXPECT_EQ(missing.name, "Missing");
    EXPECT_EQ(missing.color, core::Color::fromIndex(7));
    EXPECT_EQ(missing.linetype, document.continuousLinetype());
    EXPECT_EQ(document.layers().back().name, "Current");
    EXPECT_EQ(document.layer(document.current().layer)->name, "Current");
    EXPECT_TRUE(document.findLinetype("Other")->pattern.continuous());
    EXPECT_EQ(layerNamed(document, "Walls").color, core::Color::fromIndex(1));
    EXPECT_EQ(layerNamed(document, "Walls").lineweight, 40);
    EXPECT_EQ(document.linetype(layerNamed(document, "Bylayer").linetype)->name, "Continuous");
    EXPECT_EQ(document.lwdefault(), 25);
    EXPECT_EQ(document.find(core::EntityId{0x30})->properties.lineweight, 5);
    EXPECT_EQ(document.find(core::EntityId{0x32})->properties.lineweight, 211);
    EXPECT_EQ(document.findLayer("Bad"), nullptr);
    // The records kept raw are written where they were, with their handles.
    const auto written = writeOk(document);
    EXPECT_FALSE(objectGroups(written, "LAYER", "WALLS").empty());
    EXPECT_FALSE(objectGroups(written, "LAYER", "Bad").empty());
    EXPECT_FALSE(objectGroups(written, "LTYPE", "dashed").empty());
    const auto reread = readOk(written);
    EXPECT_EQ(reread.document.layers().size(), document.layers().size());
}

TEST(DxfTables, RecordsWithoutHandleGetOne) {
    const RawGroups layers{{0, "LAYER"},
                           {100, "AcDbSymbolTableRecord"},
                           {100, "AcDbLayerTableRecord"},
                           {2, "0"},
                           {70, "0"},
                           {62, "7"},
                           {6, "Continuous"}};
    const auto read = readOk(drawing(standardLinetypes(), layers, {}));
    ASSERT_EQ(read.document.layers().size(), 1U);
    EXPECT_EQ(read.document.layers()[0].id.value, 0x100U);
    ASSERT_EQ(read.report.warnings.size(), 1U);
    EXPECT_NE(read.report.warnings[0].find("LAYER record had no handle; assigned 100"),
              std::string::npos);
}

// --- Header variables (docs/adr/0023, point 3; docs/adr/0025) -------------

TEST(DxfTables, HeaderVariablesEnterTheDocumentAndKeepTheirSpelling) {
    const RawGroups header{{9, "$LTSCALE"},   {40, "2.5"},     {9, "$CLAYER"},    {8, "Walls"},
                           {9, "$CELTYPE"},   {6, "DASHED"},   {9, "$CECOLOR"},   {62, "     3"},
                           {9, "$CELTSCALE"}, {40, "0.5"},     {9, "$CELWEIGHT"}, {370, "    30"},
                           {9, "$LWDISPLAY"}, {290, "     1"}, {9, "$LWDEFAULT"}, {370, "    18"}};
    RawGroups layers = layer("10", "0", {{70, "0"}, {62, "7"}, {6, "Continuous"}});
    const auto walls = layer("11", "Walls", {{70, "0"}, {62, "1"}, {6, "Continuous"}});
    layers.insert(layers.end(), walls.begin(), walls.end());
    const std::string bytes = drawing(standardLinetypes(), layers, {}, header);
    auto read = readOk(bytes);
    auto& document = read.document;
    EXPECT_EQ(document.ltscale(), 2.5);
    EXPECT_TRUE(document.lwdisplay());
    EXPECT_EQ(document.lwdefault(), 18);
    const auto& current = document.current();
    EXPECT_EQ(document.layer(current.layer)->name, "Walls");
    EXPECT_EQ(document.linetype(current.linetype)->name, "DASHED");
    EXPECT_EQ(current.color, core::Color::fromIndex(3));
    EXPECT_EQ(current.linetypeScale, 0.5);
    EXPECT_EQ(current.lineweight, 30);
    // A renamed current layer leaves no old name in the raw header.
    for (const auto& section : document.raw().sections) {
        if (section.name == "HEADER") {
            EXPECT_EQ(std::ranges::find(section.groups, core::RawGroup{8, "Walls"}),
                      section.groups.end());
        }
    }
    auto before = groupsOf(bytes);
    auto after = groupsOf(writeOk(document));
    ASSERT_EQ(before.size(), after.size());
    for (std::size_t i = 0; i < before.size(); ++i) {
        if (i == 0 || before[i - 1].value != "$HANDSEED") {
            EXPECT_EQ(before[i], after[i]) << i;
        }
    }
    // Changed values replace those read; RGB adds the 420.
    core::CurrentProperties changed = current;
    changed.color = core::Color::fromRgb({255, 0, 0});
    changed.layer = document.layerZero();
    document.setCurrent(changed);
    document.setLwdisplay(false);
    const auto written = groupsOf(writeOk(document));
    for (const RawGroups& expected : {RawGroups{{9, "$CLAYER"}, {8, "0"}},
                                      RawGroups{{9, "$CECOLOR"}, {62, "1"}, {420, "16711680"}},
                                      RawGroups{{9, "$LWDISPLAY"}, {290, "0"}},
                                      RawGroups{{9, "$LWDEFAULT"}, {370, "    18"}}}) {
        EXPECT_NE(std::ranges::search(written, expected).begin(), written.end())
            << expected[0].value;
    }
}

TEST(DxfTables, MissingHeaderVariablesAreAddedOnlyWhenTheyDiffer) {
    auto read = readOk(drawing(standardLinetypes(),
                               layer("10", "0", {{70, "0"}, {62, "7"}, {6, "Continuous"}}), {}));
    auto& document = read.document;
    EXPECT_EQ(writeOk(document).find("$LTSCALE"), std::string::npos);
    document.setLtscale(3.0);
    const auto written = groupsOf(writeOk(document));
    EXPECT_NE(std::ranges::search(written, RawGroups{{9, "$LTSCALE"}, {40, "3.0"}}).begin(),
              written.end());
    EXPECT_EQ(std::ranges::find(written, core::RawGroup{9, "$CLAYER"}), written.end());
}

// --- Random drawings ----------------------------------------------------------

/// Numbers from 0 to count - 1, the same at every run.
class FixtureNumbers {
public:
    explicit FixtureNumbers(std::uint32_t seed) : m_engine(seed) {}

    int operator()(int count) {
        return static_cast<int>(m_engine() % static_cast<unsigned>(count));
    }

private:
    std::mt19937 m_engine;
};

core::Color randomColor(FixtureNumbers& pick) {
    switch (pick(4)) {
        case 0:
            return core::Color::byLayer();
        case 1:
            return core::Color::byBlock();
        case 2:
            return core::Color::fromIndex(1 + pick(255));
        default:
            return core::Color::fromRgb({static_cast<std::uint8_t>(pick(256)), 7, 9});
    }
}

TEST(DxfTables, RandomDrawingsSurviveReadingAndWriting) {
    FixtureNumbers pick(20261026);
    for (int trial = 0; trial < 40; ++trial) {
        core::Document document = core::Document::newDrawing();
        std::vector<core::LinetypeId> linetypes{
            document.byLayerLinetype(), document.byBlockLinetype(), document.continuousLinetype()};
        for (int i = 0; i < pick(4); ++i) {
            std::vector<double> elements;
            const int count = 1 + pick(5);
            elements.reserve(static_cast<std::size_t>(count));
            for (int e = 0; e < count; ++e) {
                elements.push_back(static_cast<double>(pick(41) - 20) / 4.0);
            }
            linetypes.push_back(document.addLinetype(core::Linetype{
                {}, "T" + std::to_string(i), "type " + std::to_string(i), {elements}, {}}));
        }
        std::vector<core::LayerId> layers{document.layerZero()};
        for (int i = 0; i < pick(6); ++i) {
            core::Layer layer;
            layer.name = "Layer " + std::to_string(i);
            layer.color = pick(2) == 0
                              ? core::Color::fromIndex(1 + pick(255))
                              : core::Color::fromRgb({static_cast<std::uint8_t>(pick(256)),
                                                      static_cast<std::uint8_t>(pick(256)),
                                                      static_cast<std::uint8_t>(pick(256))});
            const auto drawn = linetypes.at(
                2 + static_cast<std::size_t>(pick(static_cast<int>(linetypes.size()) - 2)));
            layer.linetype = drawn;
            layer.lineweight = core::lineweight::kAllowed.at(static_cast<std::size_t>(pick(24)));
            layer.on = pick(3) != 0;
            layer.frozen = pick(4) == 0;
            layer.locked = pick(4) == 0;
            layer.plot = pick(5) != 0;
            layers.push_back(document.addLayer(layer));
        }
        for (int i = 0; i < 1 + pick(10); ++i) {
            const auto id = document.add(core::Line{{static_cast<double>(i), 0.0}, {0.0, 1.0}});
            core::EntityProperties properties;
            properties.layer =
                layers.at(static_cast<std::size_t>(pick(static_cast<int>(layers.size()))));
            properties.linetype =
                linetypes.at(static_cast<std::size_t>(pick(static_cast<int>(linetypes.size()))));
            properties.color = randomColor(pick);
            properties.lineweight =
                pick(2) == 0 ? -1 - pick(3)
                             : core::lineweight::kAllowed.at(static_cast<std::size_t>(pick(24)));
            properties.linetypeScale = pick(2) == 0 ? 1.0 : 0.25 * (1 + pick(8));
            document.setProperties(id, properties);
        }
        core::CurrentProperties current = document.current();
        current.layer = layers.back();
        current.color = core::Color::fromIndex(1 + pick(255));
        document.setCurrent(current);
        document.setLtscale(0.5 * (1 + pick(4)));
        document.setLwdisplay(pick(2) == 0);

        const auto written = writeOk(document);
        const auto read = readOk(written);
        EXPECT_TRUE(read.report.warnings.empty()) << trial;
        const auto& copy = read.document;
        ASSERT_EQ(copy.layers().size(), document.layers().size());
        for (std::size_t i = 0; i < copy.layers().size(); ++i) {
            core::Layer layer = copy.layers()[i];
            layer.extras = document.layers()[i].extras;
            EXPECT_EQ(layer, document.layers()[i]) << trial << " layer " << i;
        }
        ASSERT_EQ(copy.linetypes().size(), document.linetypes().size());
        for (std::size_t i = 0; i < copy.linetypes().size(); ++i) {
            core::Linetype linetype = copy.linetypes()[i];
            linetype.extras = document.linetypes()[i].extras;
            EXPECT_EQ(linetype, document.linetypes()[i]) << trial << " linetype " << i;
        }
        ASSERT_EQ(copy.entityCount(), document.entityCount());
        for (std::size_t i = 0; i < copy.entityCount(); ++i) {
            EXPECT_EQ(copy.entities()[i].properties, document.entities()[i].properties)
                << trial << " entity " << i;
        }
        EXPECT_EQ(copy.current(), document.current());
        EXPECT_EQ(copy.ltscale(), document.ltscale());
        EXPECT_EQ(copy.lwdisplay(), document.lwdisplay());
        // Read again and written, the file is the same.
        EXPECT_EQ(writeOk(copy), written) << trial;
    }
}

}  // namespace
}  // namespace tg::io
