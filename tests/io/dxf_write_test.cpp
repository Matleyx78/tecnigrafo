// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entities.hpp"
#include "tg/core/line.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <string>
#include <utility>

namespace tg::io {
namespace {

std::string fixture(std::string_view name) {
    std::ifstream input{std::filesystem::path{TG_IO_TEST_DATA} / "dxf" / name, std::ios::binary};
    return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

TEST(DxfWrite, PreservesUnknownSectionsEntitiesAndSpelling) {
    const auto bytes = fixture("conservative.dxf");
    ASSERT_FALSE(bytes.empty());
    const auto before = readGroups(bytes);
    const auto read = readDxf(bytes);
    ASSERT_TRUE(read.ok()) << read.error().message;
    const auto written = writeDxf(read.value().document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok()) << written.error().message;
    const auto after = readGroups(written.value());
    ASSERT_TRUE(after.ok());
    ASSERT_TRUE(before.ok());
    EXPECT_EQ(before.value().groups, after.value().groups);
    const auto reread = readDxf(written.value());
    ASSERT_TRUE(reread.ok());
    EXPECT_EQ(read.value().document.raw(), reread.value().document.raw());
    EXPECT_EQ(read.value().report.unhandled, reread.value().report.unhandled);
}

TEST(DxfWrite, UpdatesHandseedAndUnitsWithoutMovingHeaderVariables) {
    auto read = readDxf(fixture("conservative.dxf"));
    ASSERT_TRUE(read.ok());
    read.value().document.setNextId(0x200);
    read.value().document.setUnits(24);
    const auto written = writeDxf(read.value().document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    const auto after = readDxf(written.value());
    ASSERT_TRUE(after.ok());
    EXPECT_EQ(after.value().document.units(), 24);
    EXPECT_EQ(after.value().document.nextId(), 0x200U);
    const auto& oldHeader = read.value().document.raw().sections.front().groups;
    const auto& newHeader = after.value().document.raw().sections.front().groups;
    ASSERT_EQ(oldHeader.size(), newHeader.size());
    for (std::size_t i = 0; i < oldHeader.size(); ++i) {
        EXPECT_EQ(oldHeader[i].code, newHeader[i].code);
        if (i == 0 ||
            (oldHeader[i - 1].value != "$HANDSEED" && oldHeader[i - 1].value != "$INSUNITS")) {
            EXPECT_EQ(oldHeader[i].value, newHeader[i].value);
        }
    }
}

TEST(DxfWrite, NewDrawingHasACompleteR2018SkeletonWithoutWarnings) {
    const core::Document doc;
    const auto written = writeDxf(doc, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    const auto read = readDxf(written.value());
    ASSERT_TRUE(read.ok()) << read.error().message;
    EXPECT_EQ(read.value().document.raw().version, "AC1032");
    EXPECT_EQ(read.value().document.units(), core::Document::kDefaultUnits);
    EXPECT_TRUE(read.value().report.warnings.empty());
    EXPECT_TRUE(read.value().report.unhandled.empty());
    EXPECT_NE(modelSpaceHandle(read.value().document.raw()).value, 0U);
    for (const auto* const name :
         {"VPORT", "LTYPE", "LAYER", "STYLE", "VIEW", "UCS", "APPID", "DIMSTYLE", "BLOCK_RECORD",
          "*Model_Space", "*Paper_Space", "ACAD_GROUP", "ACAD_LAYOUT", "Model", "Layout1",
          "ByBlock", "ByLayer", "Continuous", "Standard"}) {
        EXPECT_NE(written.value().find(std::string{"\r\n"} + name + "\r\n"), std::string::npos)
            << name;
    }
    const auto again = writeDxf(read.value().document, DxfVersion::R2018);
    ASSERT_TRUE(again.ok());
    EXPECT_EQ(written.value(), again.value());
}

TEST(DxfWrite, SkeletonHandlesNeverCollideWithExistingEntities) {
    core::Document doc;
    doc.insertRaw({{1}, 2, "POINT", {{5, "1"}, {10, "0"}}});
    doc.insertRaw({{0x100}, 1, "POINT", {{5, "100"}, {10, "1"}}});
    const auto written = writeDxf(doc, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    const auto read = readDxf(written.value());
    ASSERT_TRUE(read.ok()) << read.error().message;
    ASSERT_EQ(read.value().document.rawEntities().size(), 2U);
    EXPECT_EQ(read.value().document.rawEntities()[0].id.value, 0x100U);
    EXPECT_EQ(read.value().document.rawEntities()[1].id.value, 1U);
    EXPECT_GT(read.value().document.nextId(), doc.nextId());
}

TEST(DxfWrite, ConvertsAnsiTextAndKeepsOriginalVersionOnSave) {
    const auto original = fixture("r2000.dxf");
    auto read = readDxf(original);
    ASSERT_TRUE(read.ok()) << read.error().message;
    EXPECT_EQ(read.value().document.rawEntities()[0].groups.back().value, "caf\xC3\xA9");
    const auto saved = writeDxf(read.value().document, DxfVersion::R2000);
    ASSERT_TRUE(saved.ok());
    EXPECT_EQ(saved.value().find("caf\xC3\xA9"), std::string::npos);
    EXPECT_NE(saved.value().find("caf\xE9"), std::string::npos);
    const auto converted = writeDxf(read.value().document, DxfVersion::R2018);
    ASSERT_TRUE(converted.ok());
    EXPECT_NE(converted.value().find("caf\xC3\xA9"), std::string::npos);
    const auto reread = readDxf(converted.value());
    ASSERT_TRUE(reread.ok());
    EXPECT_EQ(reread.value().document.raw().version, "AC1032");
    EXPECT_EQ(reread.value().document.rawEntities()[0], read.value().document.rawEntities()[0]);
}

TEST(DxfWrite, RejectsDowngradesAndUnsupportedManagedEntitiesWithoutDroppingThem) {
    auto read = readDxf(fixture("conservative.dxf"));
    ASSERT_TRUE(read.ok());
    const auto older = writeDxf(read.value().document, DxfVersion::R2000);
    ASSERT_FALSE(older.ok());
    EXPECT_EQ(older.error().kind, Error::Kind::Unsupported);
    core::Document fresh;
    EXPECT_FALSE(writeDxf(fresh, DxfVersion::R2000).ok());
    fresh.add(core::Line{});
    const auto unsupported = writeDxf(fresh, DxfVersion::R2018);
    ASSERT_FALSE(unsupported.ok());
    EXPECT_EQ(unsupported.error().kind, Error::Kind::Unsupported);
    EXPECT_FALSE(makeSkeleton(std::numeric_limits<std::uint64_t>::max()).ok());
}

TEST(DxfWrite, AddsAbsentHeaderValuesAndRejectsInvalidRawText) {
    const auto read = readDxf("0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");
    ASSERT_TRUE(read.ok());
    const auto written = writeDxf(read.value().document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    const auto reread = readDxf(written.value());
    ASSERT_TRUE(reread.ok());
    EXPECT_TRUE(reread.value().report.warnings.empty());
    core::Document doc;
    doc.insertRaw({{1}, 1, "POINT", {{5, "1"}, {1, "invalid\nvalue"}}});
    EXPECT_FALSE(writeDxf(doc, DxfVersion::R2018).ok());
}

TEST(DxfWrite, LeavesMissingHeaderVariablesAbsentUntilTheDocumentChanges) {
    const core::RawGroups original{
        {0, "SECTION"},    {2, "HEADER"},  {9, "$ACADVER"}, {1, "AC1032"},
        {0, "ENDSEC"},     {0, "SECTION"}, {2, "ENTITIES"}, {0, "POINT"},
        {5, "20"},         {0, "ENDSEC"},  {0, "SECTION"},  {2, "OBJECTS"},
        {0, "DICTIONARY"}, {5, "30"},      {0, "ENDSEC"},   {0, "EOF"}};
    auto read = readDxf(writeGroups(original, TextEncoding::Utf8));
    ASSERT_TRUE(read.ok());
    auto written = writeDxf(read.value().document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    auto groups = readGroups(written.value());
    ASSERT_TRUE(groups.ok());
    EXPECT_EQ(groups.value().groups, original);

    read.value().document.setUnits(4);
    written = writeDxf(read.value().document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    EXPECT_NE(written.value().find("$INSUNITS"), std::string::npos);
    EXPECT_EQ(written.value().find("$HANDSEED"), std::string::npos);
    EXPECT_EQ(written.value().find("$DWGCODEPAGE"), std::string::npos);

    // Even an entity allocated and then deleted must never reuse its handle.
    const auto id = read.value().document.add(core::Line{});
    ASSERT_TRUE(read.value().document.remove(id).has_value());
    written = writeDxf(read.value().document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    EXPECT_NE(written.value().find("$HANDSEED"), std::string::npos);
    const auto reread = readDxf(written.value());
    ASSERT_TRUE(reread.ok());
    EXPECT_EQ(reread.value().document.nextId(), id.value + 1);
}

TEST(DxfWrite, AddsMissingCodePageWhenTheOutputEncodingChanges) {
    const std::string bytes =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n"
        "0\nENDSEC\n0\nEOF\n";
    auto read = readDxf(bytes);
    ASSERT_TRUE(read.ok());
    auto raw = read.value().document.raw();
    raw.codePage = "ANSI_1250";
    read.value().document.setRaw(std::move(raw));
    const auto written = writeDxf(read.value().document, DxfVersion::R2000);
    ASSERT_TRUE(written.ok());
    EXPECT_NE(written.value().find("$DWGCODEPAGE\r\n  3\r\nANSI_1250"), std::string::npos);
    EXPECT_EQ(written.value().find("$HANDSEED"), std::string::npos);
    EXPECT_EQ(written.value().find("$INSUNITS"), std::string::npos);
}

}  // namespace
}  // namespace tg::io
