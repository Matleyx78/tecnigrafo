// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The DIMSTYLE table, the $DIM… variables of the header and the overrides of
// a dimension in the DXF (docs/adr/0047, point 2; package 0.8-PT22).

#include "dxf_tables.hpp"
#include "tg/core/dim_style_table.hpp"
#include "tg/core/start_handles.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace tg::io {
namespace {

using core::RawGroups;

ReadResult readOk(const std::string& bytes) {
    auto result = readDxf(bytes);
    EXPECT_TRUE(result.ok()) << (result.ok() ? "" : result.error().message);
    return result.ok() ? std::move(result).value() : ReadResult{};
}

std::string writeOk(const core::Document& document) {
    auto written = writeDxf(document, DxfVersion::R2018);
    EXPECT_TRUE(written.ok()) << (written.ok() ? "" : written.error().message);
    return written.ok() ? std::move(written).value() : std::string{};
}

RawGroups groupsOf(const std::string& bytes) {
    auto groups = readGroups(bytes);
    EXPECT_TRUE(groups.ok());
    return groups.ok() ? std::move(groups).value().groups : RawGroups{};
}

/// The groups from the group (code, value) to the next group 0 or 9 (a
/// header variable or an object), the first included.
RawGroups partOf(const RawGroups& all, int code, std::string_view value) {
    for (std::size_t i = 0; i < all.size(); ++i) {
        if (all[i].code == code && all[i].value == value) {
            std::size_t end = i + 1;
            while (end < all.size() && all[end].code != 0 && all[end].code != 9) {
                ++end;
            }
            return {all.begin() + static_cast<std::ptrdiff_t>(i),
                    all.begin() + static_cast<std::ptrdiff_t>(end)};
        }
    }
    return {};
}

/// The DIMSTYLE table, from 0 TABLE to 0 ENDTAB.
RawGroups dimStyleTable(const RawGroups& all) {
    for (std::size_t i = 0; i + 1 < all.size(); ++i) {
        if (all[i].code == 0 && all[i].value == "TABLE" && all[i + 1].value == "DIMSTYLE") {
            std::size_t end = i;
            while (end < all.size() && (all[end].code != 0 || all[end].value != "ENDTAB")) {
                ++end;
            }
            return {all.begin() + static_cast<std::ptrdiff_t>(i),
                    all.begin() + static_cast<std::ptrdiff_t>(end + 1)};
        }
    }
    return {};
}

/// A file of AutoCAD 2018: the header with the $DIM… variables (essential and
/// not), the STYLE table with Standard, the DIMSTYLE table with Standard
/// (the values of the DXF, no group), ISO-25 (with reactors and the groups
/// of AutoCAD) and Elettrico, and `entities`.
std::string autocadDrawing(const RawGroups& header, const RawGroups& entities = {}) {
    RawGroups groups{{0, "SECTION"}, {2, "HEADER"},    {9, "$ACADVER"},
                     {1, "AC1032"},  {9, "$HANDSEED"}, {5, "100"}};
    groups.insert(groups.end(), header.begin(), header.end());
    groups.insert(groups.end(), {{0, "ENDSEC"},
                                 {0, "SECTION"},
                                 {2, "TABLES"},
                                 {0, "TABLE"},
                                 {2, "STYLE"},
                                 {5, "3"},
                                 {330, "0"},
                                 {100, "AcDbSymbolTable"},
                                 {70, "1"},
                                 {0, "STYLE"},
                                 {5, "11"},
                                 {330, "3"},
                                 {100, "AcDbSymbolTableRecord"},
                                 {100, "AcDbTextStyleTableRecord"},
                                 {2, "Standard"},
                                 {70, "0"},
                                 {40, "0.0"},
                                 {41, "1.0"},
                                 {50, "0.0"},
                                 {71, "0"},
                                 {42, "2.5"},
                                 {3, "arial.ttf"},
                                 {4, ""},
                                 {0, "ENDTAB"},
                                 {0, "TABLE"},
                                 {2, "DIMSTYLE"},
                                 {5, "A"},
                                 {330, "0"},
                                 {100, "AcDbSymbolTable"},
                                 {70, "3"},
                                 {100, "AcDbDimStyleTable"},
                                 {71, "3"},
                                 {340, "27"},
                                 {340, "4E"},
                                 {340, "4F"},
                                 {0, "DIMSTYLE"},
                                 {105, "27"},
                                 {330, "A"},
                                 {100, "AcDbSymbolTableRecord"},
                                 {100, "AcDbDimStyleTableRecord"},
                                 {2, "Standard"},
                                 {70, "0"},
                                 {340, "11"},
                                 {0, "DIMSTYLE"},
                                 {105, "4E"},
                                 {102, "{ACAD_REACTORS"},
                                 {330, "50"},
                                 {102, "}"},
                                 {330, "A"},
                                 {100, "AcDbSymbolTableRecord"},
                                 {100, "AcDbDimStyleTableRecord"},
                                 {2, "ISO-25"},
                                 {70, "0"},
                                 {41, "2.5"},
                                 {42, "0.625"},
                                 {43, "3.75"},
                                 {44, "1.25"},
                                 {73, "0"},
                                 {74, "0"},
                                 {77, "1"},
                                 {78, "8"},
                                 {140, "2.5"},
                                 {141, "2.5"},
                                 {143, "0.03937007874016"},
                                 {147, "0.625"},
                                 {171, "3"},
                                 {172, "1"},
                                 {271, "2"},
                                 {272, "2"},
                                 {274, "3"},
                                 {278, "44"},
                                 {283, "0"},
                                 {284, "8"},
                                 {340, "11"},
                                 {0, "DIMSTYLE"},
                                 {105, "4F"},
                                 {330, "A"},
                                 {100, "AcDbSymbolTableRecord"},
                                 {100, "AcDbDimStyleTableRecord"},
                                 {2, "Elettrico"},
                                 {70, "0"},
                                 {41, "2.0"},
                                 {140, "3.5"},
                                 {271, "3"},
                                 {340, "11"},
                                 {0, "ENDTAB"},
                                 {0, "ENDSEC"},
                                 {0, "SECTION"},
                                 {2, "ENTITIES"}});
    groups.insert(groups.end(), entities.begin(), entities.end());
    groups.insert(groups.end(), {{0, "ENDSEC"}, {0, "EOF"}});
    return writeGroups(groups, TextEncoding::Utf8);
}

/// The $DIM… variables of the header of autocadDrawing(): $DIMSTYLE,
/// essential ones ($DIMTXT differs from the style) and others.
RawGroups autocadHeader() {
    return {{9, "$DIMSCALE"}, {40, "1.0"},       {9, "$DIMASZ"},
            {40, "2.0"},      {9, "$DIMTOL"},    {70, "0"},
            {9, "$DIMTXT"},   {40, "5.0"},       {9, "$DIMBLK"},
            {1, ""},          {9, "$DIMSTYLE"},  {2, "Elettrico"},
            {9, "$DIMDSEP"},  {70, "44"},        {9, "$DIMTXSTY"},
            {7, "Standard"},  {9, "$DIMJOGANG"}, {40, "0.785398163397"}};
}

TEST(DxfDimStyle, AMissingCurrentStyleGivesStandard) {
    // $DIMSTYLE=PROVA, which the table lacks (PT5, B06).
    const auto read = readOk(autocadDrawing({{9, "$DIMSTYLE"}, {2, "PROVA"}}));
    EXPECT_TRUE(read.report.warnings.empty());
    const core::DimStyleTable& table = read.document.dimStyles();
    ASSERT_EQ(table.styles().size(), 3U);
    EXPECT_EQ(table.current(), "Standard");
    EXPECT_EQ(table.currentVariables(), table.find("Standard")->vars);
    EXPECT_EQ(table.find("Standard")->vars, core::dxfDefaultDimVariables());

    // Written again as it was: the name read stays while Standard is the
    // current style that a reader gives for it.
    const std::string written = writeOk(read.document);
    EXPECT_EQ(partOf(groupsOf(written), 9, "$DIMSTYLE"),
              (RawGroups{{9, "$DIMSTYLE"}, {2, "PROVA"}}));
    EXPECT_EQ(readOk(written).document.dimStyles(), table);
}

TEST(DxfDimStyle, WithoutStandardAndCurrentStyleStandardIsCreated) {
    // A file without DIMSTYLE table nor $DIMSTYLE.
    const std::string bytes = writeGroups(RawGroups{{0, "SECTION"},
                                                    {2, "HEADER"},
                                                    {9, "$ACADVER"},
                                                    {1, "AC1032"},
                                                    {0, "ENDSEC"},
                                                    {0, "SECTION"},
                                                    {2, "ENTITIES"},
                                                    {0, "ENDSEC"},
                                                    {0, "EOF"}},
                                          TextEncoding::Utf8);
    const auto read = readOk(bytes);
    const core::DimStyleTable& table = read.document.dimStyles();
    ASSERT_EQ(table.styles().size(), 1U);
    EXPECT_EQ(table.styles()[0].name, "Standard");
    EXPECT_EQ(table.styles()[0].vars, core::dxfDefaultDimVariables());
    EXPECT_EQ(table.current(), "Standard");
    // Written without a DIMSTYLE table, as it was read.
    const std::string written = writeOk(read.document);
    EXPECT_TRUE(dimStyleTable(groupsOf(written)).empty());
    EXPECT_TRUE(partOf(groupsOf(written), 9, "$DIMSTYLE").empty());

    // A table without Standard but with the current style: nothing is created.
    const std::string iso = autocadDrawing({{9, "$DIMSTYLE"}, {2, "ISO-25"}});
    const auto withIso = readOk(iso);
    EXPECT_EQ(withIso.document.dimStyles().styles().size(), 3U);
    EXPECT_EQ(withIso.document.dimStyles().current(), "ISO-25");
}

TEST(DxfDimStyle, ADimensionOfAMissingStyleKeepsTheNameAndUsesStandard) {
    const RawGroups dimension{{0, "DIMENSION"},    {5, "60"},    {330, "1F"},
                              {100, "AcDbEntity"}, {8, "0"},     {100, "AcDbDimension"},
                              {2, "*D1"},          {10, "40.0"}, {20, "10.0"},
                              {30, "0.0"},         {70, "32"},   {3, "PROVA"}};
    const auto read = readOk(autocadDrawing({{9, "$DIMSTYLE"}, {2, "ISO-25"}}, dimension));
    const core::DimStyleTable& table = read.document.dimStyles();
    ASSERT_EQ(read.document.rawEntities().size(), 1U);
    const auto& groups = read.document.rawEntities()[0].groups;
    EXPECT_NE(std::ranges::find(groups, core::RawGroup{3, "PROVA"}), groups.end());
    EXPECT_EQ(core::dimensionVariables(table, "PROVA", {}), table.find("Standard")->vars);
    const std::string written = writeOk(read.document);
    EXPECT_EQ(partOf(groupsOf(written), 0, "DIMENSION"), dimension);
}

TEST(DxfDimStyle, TheTableAndTheHeaderOfAutocadAreReadAndWrittenAgainAsTheyWere) {
    const std::string bytes = autocadDrawing(autocadHeader());
    const auto read = readOk(bytes);
    EXPECT_TRUE(read.report.warnings.empty());
    const core::DimStyleTable& table = read.document.dimStyles();
    ASSERT_EQ(table.styles().size(), 3U);
    EXPECT_EQ(table.styles()[0].id, 0x27U);
    EXPECT_EQ(table.styles()[0].vars, core::dxfDefaultDimVariables());
    EXPECT_EQ(table.styles()[1].vars, core::DimVariables{});
    EXPECT_TRUE(core::DimStyleTable::regenerable(table.styles()[1].extras));
    const core::DimStyle& elettrico = table.styles()[2];
    EXPECT_EQ(elettrico.vars.arrowSize, 2.0);
    EXPECT_EQ(elettrico.vars.textHeight, 3.5);
    EXPECT_EQ(elettrico.vars.decimals, 3);
    EXPECT_EQ(elettrico.vars.decimalSeparator, '.');
    EXPECT_EQ(elettrico.vars.textStyle, "Standard");
    EXPECT_EQ(table.current(), "Elettrico");
    // The variables the header lacks are those of the current style.
    core::DimVariables current = elettrico.vars;
    current.textHeight = 5.0;
    current.decimalSeparator = ',';
    EXPECT_EQ(table.currentVariables(), current);
    EXPECT_EQ(table.currentExtras(), (RawGroups{{9, "$DIMTOL"},
                                                {70, "0"},
                                                {9, "$DIMBLK"},
                                                {1, ""},
                                                {9, "$DIMJOGANG"},
                                                {40, "0.785398163397"}}));

    const auto input = groupsOf(bytes);
    const auto output = groupsOf(writeOk(read.document));
    EXPECT_EQ(dimStyleTable(output), dimStyleTable(input));
    for (const auto& group : autocadHeader()) {
        if (group.code == 9) {
            EXPECT_EQ(partOf(output, 9, group.value), partOf(input, 9, group.value)) << group.value;
        }
    }
}

TEST(DxfDimStyle, ChangedStylesAndVariablesAreWrittenAndReadBack) {
    auto read = readOk(autocadDrawing(autocadHeader()));
    core::Document& document = read.document;
    core::DimStyleTable table = document.dimStyles();
    core::DimStyle iso25 = *table.find("ISO-25");
    iso25.vars.textHeight = 3.5;
    iso25.vars.decimalSeparator = '.';
    table.put(iso25, document);
    core::DimStyle nuovo;
    nuovo.name = "Nuovo";
    nuovo.vars.dimColor = core::Color::fromIndex(1);
    nuovo.vars.post = "<> mm";
    const std::uint64_t id = table.put(nuovo, document);
    table.setCurrent("Nuovo");
    core::DimVariables current = nuovo.vars;
    current.decimals = 4;
    table.setCurrentVariables(current);
    table.setCurrentExtras({{9, "$DIMTOL"}, {70, "1"}});
    document.replaceDimStyles(table);

    const std::string written = writeOk(document);
    const auto groups = groupsOf(written);
    // ISO-25: the groups changed in place.
    const auto record = partOf(groups, 105, "4E");
    EXPECT_NE(std::ranges::find(record, core::RawGroup{140, "3.5"}), record.end());
    EXPECT_NE(std::ranges::find(record, core::RawGroup{278, "46"}), record.end());
    // Nuovo: a new record with its handle and the handle of its text style.
    const auto added = partOf(groups, 105, handleText(id));
    EXPECT_NE(std::ranges::find(added, core::RawGroup{2, "Nuovo"}), added.end());
    EXPECT_NE(std::ranges::find(added, core::RawGroup{176, "1"}), added.end());
    EXPECT_NE(std::ranges::find(added, core::RawGroup{3, "<> mm"}), added.end());
    EXPECT_NE(std::ranges::find(added, core::RawGroup{340, "11"}), added.end());
    EXPECT_EQ(partOf(groups, 9, "$DIMSTYLE"), (RawGroups{{9, "$DIMSTYLE"}, {2, "Nuovo"}}));
    EXPECT_EQ(partOf(groups, 9, "$DIMTOL"), (RawGroups{{9, "$DIMTOL"}, {70, "1"}}));
    EXPECT_TRUE(partOf(groups, 9, "$DIMBLK").empty());
    EXPECT_EQ(partOf(groups, 9, "$DIMDEC"), (RawGroups{{9, "$DIMDEC"}, {70, "4"}}));

    const auto again = readOk(written);
    EXPECT_EQ(again.document.dimStyles().styles().size(), 4U);
    EXPECT_EQ(again.document.dimStyles().find("ISO-25")->vars, iso25.vars);
    EXPECT_EQ(again.document.dimStyles().find("Nuovo")->vars, nuovo.vars);
    EXPECT_EQ(again.document.dimStyles().current(), "Nuovo");
    EXPECT_EQ(again.document.dimStyles().currentVariables(), current);
}

TEST(DxfDimStyle, ANewDrawingIsWrittenWithStandardAndIso25) {
    const core::Document document = core::Document::newDrawing();
    const std::string written = writeOk(document);
    const auto groups = groupsOf(written);
    EXPECT_EQ(partOf(groups, 9, "$DIMSTYLE"), (RawGroups{{9, "$DIMSTYLE"}, {2, "ISO-25"}}));
    EXPECT_FALSE(partOf(groups, 105, handleText(core::start_handles::kDimstyleStandard)).empty());
    EXPECT_FALSE(partOf(groups, 105, handleText(core::start_handles::kDimstyleIso25)).empty());
    const auto read = readOk(written);
    EXPECT_TRUE(read.report.warnings.empty());
    const core::DimStyleTable& table = read.document.dimStyles();
    ASSERT_EQ(table.styles().size(), 2U);
    EXPECT_EQ(table.styles()[0].vars, document.dimStyles().styles()[0].vars);
    EXPECT_EQ(table.styles()[1].vars, document.dimStyles().styles()[1].vars);
    EXPECT_EQ(table.current(), "ISO-25");
    EXPECT_EQ(table.currentVariables(), core::DimVariables{});
    EXPECT_EQ(writeOk(read.document), written);
}

TEST(DxfDimStyle, ARecordWithoutHandleGetsOneWithAWarning) {
    std::string bytes = autocadDrawing({});
    const std::string handle = "105\r\n4F\r\n";
    bytes.erase(bytes.find(handle), handle.size());
    const auto read = readOk(bytes);
    ASSERT_EQ(read.report.warnings.size(), 1U);
    EXPECT_NE(read.report.warnings[0].find("DIMSTYLE record had no handle; assigned"),
              std::string::npos);
    EXPECT_NE(read.document.dimStyles().find("Elettrico"), nullptr);
}

TEST(DxfDimStyle, TheOverridesOfADimensionAreReadAndWritten) {
    const core::Document document = core::Document::newDrawing();
    const core::TextStyleTable& styles = document.textStyles();
    const std::string standard = handleText(core::start_handles::kStyleStandard);
    RawGroups extras{
        {100, "AcDbDimension"}, {3, "ISO-25"}, {1001, "ACAD"},   {1000, "DSTYLE"}, {1002, "{"},
        {1070, "71"},           {1070, "1"},   {1070, "140"},    {1040, "3.5"},    {1070, "278"},
        {1070, "46"},           {1070, "340"}, {1005, standard}, {1002, "}"}};
    const DimOverrides read = readDimOverrides(extras, styles);
    using Pairs = std::vector<std::pair<std::string, std::string>>;
    EXPECT_EQ(read.essential,
              (Pairs{{"DIMTXT", "3.5"}, {"DIMDSEP", "."}, {"DIMTXSTY", "Standard"}}));
    EXPECT_EQ(read.others, (RawGroups{{71, "1"}}));
    EXPECT_FALSE(core::DimStyleTable::regenerable(read.others));
    const core::DimVariables effective = core::withOverrides(core::DimVariables{}, read.essential);
    EXPECT_EQ(effective.textHeight, 3.5);
    EXPECT_EQ(effective.decimalSeparator, '.');

    // The same overrides: the extras do not change.
    const RawGroups before = extras;
    writeDimOverrides(extras, read, styles);
    EXPECT_EQ(extras, before);

    // Others: the list is written again, in the order of the codes.
    DimOverrides changed = read;
    changed.essential = {{"DIMDEC", "3"}, {"DIMTXT", "5"}};
    changed.others.clear();
    writeDimOverrides(extras, changed, styles);
    EXPECT_EQ(extras, (RawGroups{{100, "AcDbDimension"},
                                 {3, "ISO-25"},
                                 {1001, "ACAD"},
                                 {1000, "DSTYLE"},
                                 {1002, "{"},
                                 {1070, "140"},
                                 {1040, "5"},
                                 {1070, "271"},
                                 {1070, "3"},
                                 {1002, "}"}}));
    EXPECT_EQ(readDimOverrides(extras, styles).essential,
              (Pairs{{"DIMTXT", "5"}, {"DIMDEC", "3"}}));

    // None: the list goes, the application stays.
    writeDimOverrides(extras, DimOverrides{}, styles);
    EXPECT_EQ(extras, (RawGroups{{100, "AcDbDimension"}, {3, "ISO-25"}, {1001, "ACAD"}}));

    // A dimension without XDATA gets the application.
    RawGroups plain{{100, "AcDbDimension"}};
    writeDimOverrides(plain, DimOverrides{{{"DIMTXSTY", "Standard"}}, {}}, styles);
    EXPECT_EQ(plain, (RawGroups{{100, "AcDbDimension"},
                                {1001, "ACAD"},
                                {1000, "DSTYLE"},
                                {1002, "{"},
                                {1070, "340"},
                                {1005, standard},
                                {1002, "}"}}));

    // A DIMTXSTY handle that names no text style stays among the others.
    const RawGroups unknown{{1001, "ACAD"}, {1000, "DSTYLE"}, {1002, "{"},
                            {1070, "340"},  {1005, "ABC"},    {1002, "}"}};
    EXPECT_TRUE(readDimOverrides(unknown, styles).essential.empty());
    EXPECT_EQ(readDimOverrides(unknown, styles).others, (RawGroups{{340, "ABC"}}));
}

}  // namespace
}  // namespace tg::io
