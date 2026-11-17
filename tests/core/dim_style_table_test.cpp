// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The dimension styles and the dimension variables (docs/adr/0047, point 2;
// docs/tappe/0.8/PT22-stili-di-quota.md).

#include "tg/core/dim_style_table.hpp"

#include "tg/core/document.hpp"
#include "tg/core/start_handles.hpp"
#include "tg/core/text_style_table.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

using Overrides = std::vector<std::pair<std::string, std::string>>;

TEST(DimStyleTable, ANewDrawingHasStandardAndIso25WithIso25Current) {
    const Document document = Document::newDrawing();
    const DimStyleTable& table = document.dimStyles();
    ASSERT_EQ(table.styles().size(), 2U);
    EXPECT_EQ(table.styles()[0].name, "Standard");
    EXPECT_EQ(table.styles()[0].id, start_handles::kDimstyleStandard);
    EXPECT_EQ(table.styles()[0].vars, dxfDefaultDimVariables());
    EXPECT_EQ(table.styles()[1].name, "ISO-25");
    EXPECT_EQ(table.styles()[1].id, start_handles::kDimstyleIso25);
    EXPECT_EQ(table.styles()[1].vars, DimVariables{});
    EXPECT_EQ(table.current(), "ISO-25");
    EXPECT_EQ(table.currentVariables(), DimVariables{});
    EXPECT_TRUE(table.currentExtras().empty());
    // The entities of a new drawing still start at 257.
    EXPECT_EQ(document.nextId(), start_handles::kFirstEntity);
    EXPECT_TRUE(DimStyleTable::regenerable(table.styles()[1].extras));
}

TEST(DimStyleTable, FindIgnoresTheCaseAndPutReplacesByName) {
    Document document = Document::newDrawing();
    DimStyleTable table = document.dimStyles();
    ASSERT_NE(table.find("iso-25"), nullptr);
    EXPECT_EQ(table.find("iso-25")->name, "ISO-25");
    EXPECT_EQ(table.find("PROVA"), nullptr);

    DimStyle prova;
    prova.name = "Prova";
    prova.vars.decimals = 3;
    const std::uint64_t id = table.put(prova, document);
    EXPECT_EQ(id, start_handles::kFirstEntity);
    EXPECT_EQ(document.nextId(), start_handles::kFirstEntity + 1);
    ASSERT_NE(table.find("PROVA"), nullptr);

    // The same name: replaced in place, with its id and its spelling.
    DimStyle again;
    again.name = "PROVA";
    again.vars.decimals = 4;
    EXPECT_EQ(table.put(again, document), id);
    ASSERT_EQ(table.styles().size(), 3U);
    EXPECT_EQ(table.styles()[2].name, "Prova");
    EXPECT_EQ(table.styles()[2].vars.decimals, 4);

    table.setCurrent("Prova");
    EXPECT_EQ(table.current(), "Prova");
    DimVariables current = table.find("Prova")->vars;
    current.textHeight = 5.0;
    table.setCurrentVariables(current);
    EXPECT_EQ(table.currentVariables().textHeight, 5.0);
}

TEST(DimStyleTable, RegenerableWhenTheBlockingVariablesHaveTheIso25Value) {
    EXPECT_TRUE(DimStyleTable::regenerable({}));
    EXPECT_TRUE(DimStyleTable::regenerable({{71, "0"}, {172, "1"}, {289, "3"}, {342, "0"}}));
    // DIMTOL 1.
    EXPECT_FALSE(DimStyleTable::regenerable({{71, "1"}}));
    // DIMBLK other than the closed filled arrow, by handle and by name.
    EXPECT_FALSE(DimStyleTable::regenerable({{342, "1F"}}));
    EXPECT_FALSE(DimStyleTable::regenerable({{5, "_ARCHTICK"}}));
    EXPECT_TRUE(DimStyleTable::regenerable({{5, ""}}));
    // DIMTOFL off, DIMATFIT 0, DIMLUNIT architectural.
    EXPECT_FALSE(DimStyleTable::regenerable({{172, "0"}}));
    EXPECT_FALSE(DimStyleTable::regenerable({{289, "0"}}));
    EXPECT_FALSE(DimStyleTable::regenerable({{277, "4"}}));
    // The essential variables do not block it.
    EXPECT_TRUE(DimStyleTable::regenerable({{140, "7"}, {271, "5"}}));
}

TEST(DimStyleTable, InDomainChecksTheRangesAndTheTextStyle) {
    const Document document = Document::newDrawing();
    const TextStyleTable& textStyles = document.textStyles();
    EXPECT_TRUE(DimStyleTable::inDomain(DimVariables{}, textStyles));
    EXPECT_TRUE(DimStyleTable::inDomain(dxfDefaultDimVariables(), textStyles));

    DimVariables decimals;
    decimals.decimals = -1;
    EXPECT_FALSE(DimStyleTable::inDomain(decimals, textStyles));
    DimVariables height;
    height.textHeight = 0.0;
    EXPECT_FALSE(DimStyleTable::inDomain(height, textStyles));
    DimVariables above;
    above.textAbove = 5;
    EXPECT_FALSE(DimStyleTable::inDomain(above, textStyles));
    DimVariables factor;
    factor.linearFactor = 0.0;
    EXPECT_FALSE(DimStyleTable::inDomain(factor, textStyles));
    // DIMTXSTY that names no text style.
    DimVariables missing;
    missing.textStyle = "NONC";
    EXPECT_FALSE(DimStyleTable::inDomain(missing, textStyles));
    DimVariables unresolved;
    unresolved.textStyle.clear();
    EXPECT_FALSE(DimStyleTable::inDomain(unresolved, textStyles));
}

TEST(DimStyleTable, InDomainIsCheckedOnTheVariablesWithTheOverrides) {
    const Document document = Document::newDrawing();
    const TextStyleTable& textStyles = document.textStyles();
    DimStyleTable table = document.dimStyles();

    // A valid style with the override DIMDEC -1: out of the domain.
    EXPECT_FALSE(DimStyleTable::inDomain(
        dimensionVariables(table, "ISO-25", Overrides{{"DIMDEC", "-1"}}), textStyles));

    // A style with DIMDEC -1 and the override 2: in the domain.
    Document owner = Document::newDrawing();
    DimStyle broken;
    broken.name = "Broken";
    broken.vars.decimals = -1;
    table.put(broken, owner);
    EXPECT_FALSE(DimStyleTable::inDomain(dimensionVariables(table, "Broken", {}), textStyles));
    EXPECT_TRUE(DimStyleTable::inDomain(
        dimensionVariables(table, "Broken", Overrides{{"DIMDEC", "2"}}), textStyles));

    // An override of DIMTXSTY that names no text style.
    EXPECT_FALSE(DimStyleTable::inDomain(
        dimensionVariables(table, "ISO-25", Overrides{{"DIMTXSTY", "NONC"}}), textStyles));
}

TEST(DimStyleTable, ADimensionOfAMissingStyleUsesStandard) {
    const Document document = Document::newDrawing();
    const DimStyleTable& table = document.dimStyles();
    EXPECT_EQ(dimensionVariables(table, "PROVA", {}), table.find("Standard")->vars);
    EXPECT_EQ(dimensionVariables(table, "ISO-25", {}), DimVariables{});
    // Without Standard either: the values of AutoCAD.
    EXPECT_EQ(dimensionVariables(DimStyleTable{}, "PROVA", {}), dxfDefaultDimVariables());
}

TEST(DimVariables, ValuesByNameAndOverrides) {
    EXPECT_EQ(dimVariableInfos().size(), 28U);
    ASSERT_NE(findDimVariable("DIMTXT"), nullptr);
    EXPECT_EQ(findDimVariable("DIMTXT")->styleCode, 140);
    EXPECT_EQ(findDimVariable("DIMTXSTY")->headerCode, 7);
    EXPECT_EQ(findDimVariable("DIMTOL"), nullptr);

    DimVariables variables;
    EXPECT_EQ(dimValue(variables, "DIMTXT"), DimValue{2.5});
    EXPECT_EQ(dimValue(variables, "DIMDSEP"), DimValue{std::string{","}});
    EXPECT_EQ(dimValue(variables, "DIMCLRD"), DimValue{0.0});
    EXPECT_EQ(dimValue(variables, "DIMTXSTY"), DimValue{std::string{"Standard"}});

    EXPECT_TRUE(setDimValue(variables, "DIMCLRD", DimValue{256.0}));
    EXPECT_EQ(variables.dimColor, Color::byLayer());
    EXPECT_TRUE(setDimValue(variables, "DIMCLRE", DimValue{3.0}));
    EXPECT_EQ(variables.extColor, Color::fromIndex(3));
    EXPECT_TRUE(setDimValue(variables, "DIMDSEP", DimValue{46.0}));
    EXPECT_EQ(variables.decimalSeparator, '.');
    EXPECT_TRUE(setDimValue(variables, "DIMSE1", DimValue{1.0}));
    EXPECT_TRUE(variables.suppressExt1);

    const DimVariables before = variables;
    EXPECT_FALSE(setDimValue(variables, "DIMDEC", DimValue{2.5}));
    EXPECT_FALSE(setDimValue(variables, "DIMSE2", DimValue{2.0}));
    EXPECT_FALSE(setDimValue(variables, "DIMCLRT", DimValue{257.0}));
    EXPECT_FALSE(setDimValue(variables, "DIMDSEP", DimValue{std::string{"ab"}}));
    EXPECT_FALSE(setDimValue(variables, "DIMTXT", DimValue{std::string{"3"}}));
    EXPECT_EQ(variables, before);

    const DimVariables overridden = withOverrides(DimVariables{}, Overrides{{"DIMTXT", "3.5"},
                                                                            {"DIMDSEP", "."},
                                                                            {"DIMPOST", "<> mm"},
                                                                            {"DIMTOL", "1"},
                                                                            {"DIMDEC", "x"}});
    EXPECT_EQ(overridden.textHeight, 3.5);
    EXPECT_EQ(overridden.decimalSeparator, '.');
    EXPECT_EQ(overridden.post, "<> mm");
    EXPECT_EQ(overridden.decimals, 2);
}

}  // namespace
}  // namespace tg::core
