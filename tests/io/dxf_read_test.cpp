// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entities.hpp"
#include "tg/core/line.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>

namespace tg::io {
namespace {

std::string drawing(const core::RawGroups& entities, const core::RawGroups& extra = {}) {
    core::RawGroups groups{{0, "SECTION"}, {2, "HEADER"},       {9, "$ACADVER"},
                           {1, "AC1032"},  {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}};
    groups.insert(groups.end(), extra.begin(), extra.end());
    groups.insert(groups.end(), {{0, "ENDSEC"}, {0, "SECTION"}, {2, "ENTITIES"}});
    groups.insert(groups.end(), entities.begin(), entities.end());
    groups.insert(groups.end(), {{0, "ENDSEC"}, {0, "EOF"}});
    return writeGroups(groups, TextEncoding::Utf8);
}

TEST(DxfRead, UnknownEntitiesKeepAllGroupsAndOrder) {
    const core::RawGroups groups{{5, "2F"},     {8, "layer"},          {10, "1.0000"},
                                 {1001, "APP"}, {1000, "caf\xC3\xA9"}, {1004, "aB01"}};
    core::RawGroups entities{{0, "HATCH"}};
    entities.insert(entities.end(), groups.begin(), groups.end());
    entities.insert(entities.end(), {{0, "INSERT"}, {5, "30"}, {2, "block"}});
    const auto result = readDxf(drawing(entities));
    ASSERT_TRUE(result.ok());
    const auto& doc = result.value().document;
    EXPECT_EQ(doc.entityCount(), 0U);
    ASSERT_EQ(doc.rawEntities().size(), 2U);
    EXPECT_EQ(doc.rawEntities()[0].groups, groups);
    EXPECT_EQ(doc.rawEntities()[0].id.value, 0x2FU);
    EXPECT_LT(doc.rawEntities()[0].order, doc.rawEntities()[1].order);
    EXPECT_EQ(result.value().report.unhandled.at("HATCH"), 1);
    EXPECT_EQ(result.value().report.unhandled.at("INSERT"), 1);
    EXPECT_TRUE(result.value().report.warnings.empty());
    EXPECT_TRUE(doc.raw().sections.back().groups.empty());
}

TEST(DxfRead, MissingHandlesAreAllocatedAfterAllExistingOnesAndHandseed) {
    auto result = readDxf(drawing({{0, "POINT"}, {10, "0"}, {0, "POINT"}, {5, "FF"}, {0, "POINT"}},
                                  {{9, "$HANDSEED"}, {5, "100"}}));
    ASSERT_TRUE(result.ok());
    auto& doc = result.value().document;
    ASSERT_EQ(doc.rawEntities().size(), 3U);
    EXPECT_EQ(doc.rawEntities()[0].id.value, 0x100U);
    EXPECT_EQ(doc.rawEntities()[1].id.value, 0xFFU);
    EXPECT_EQ(doc.rawEntities()[2].id.value, 0x101U);
    EXPECT_EQ(result.value().report.warnings.size(), 2U);
    EXPECT_EQ(doc.add(core::Line{}).value, 0x102U);
}

TEST(DxfRead, RejectsDuplicateZeroAndExhaustedHandlesWithALine) {
    for (const auto& text :
         {drawing({{0, "POINT"}, {5, "a"}, {0, "POINT"}, {5, "A"}}),
          drawing({{0, "POINT"}, {5, "0"}}), drawing({{0, "POINT"}, {5, "FFFFFFFFFFFFFFFF"}}),
          drawing({{0, "POINT"}, {5, "1"}, {5, "2"}}),
          drawing({}, {{9, "$HANDSEED"}, {5, "FFFFFFFFFFFFFFFF"}})}) {
        const auto result = readDxf(text);
        ASSERT_FALSE(result.ok());
        EXPECT_EQ(result.error().kind, Error::Kind::Malformed);
        EXPECT_GT(result.error().line, 0U);
    }
}

TEST(DxfRead, HandseedIsNotAnAllocatedHandle) {
    auto result = readDxf(drawing({{0, "POINT"}, {5, "20"}}, {{9, "$HANDSEED"}, {5, "20"}}));
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.value().document.nextId(), 0x21U);
}

TEST(DxfRead, ReservesTableObjectAndDimstyleHandlesRegardlessOfSectionOrder) {
    const core::RawGroups groups{
        {0, "SECTION"}, {2, "HEADER"},     {9, "$ACADVER"}, {1, "AC1032"}, {0, "ENDSEC"},
        {0, "SECTION"}, {2, "ENTITIES"},   {0, "POINT"},    {0, "ENDSEC"}, {0, "SECTION"},
        {2, "TABLES"},  {0, "DIMSTYLE"},   {105, "40"},     {0, "ENDSEC"}, {0, "SECTION"},
        {2, "OBJECTS"}, {0, "DICTIONARY"}, {5, "50"},       {0, "ENDSEC"}, {0, "EOF"}};
    auto result = readDxf(writeGroups(groups, TextEncoding::Utf8));
    ASSERT_TRUE(result.ok());
    auto& doc = result.value().document;
    EXPECT_EQ(doc.rawEntities()[0].id.value, 0x51U);
    EXPECT_EQ(doc.add(core::Line{}).value, 0x52U);
    auto duplicate = groups;
    duplicate[12].value = "50";
    const auto bad = readDxf(writeGroups(duplicate, TextEncoding::Utf8));
    ASSERT_FALSE(bad.ok());
    EXPECT_EQ(bad.error().message, "duplicate handle 50");
}

TEST(DxfRead, SortHandlesAreReferencesButRecordHandlesMustStayUnique) {
    const core::RawGroups groups{{0, "SECTION"},
                                 {2, "HEADER"},
                                 {9, "$ACADVER"},
                                 {1, "AC1032"},
                                 {0, "ENDSEC"},
                                 {0, "SECTION"},
                                 {2, "ENTITIES"},
                                 {0, "POINT"},
                                 {5, "A"},
                                 {0, "ENDSEC"},
                                 {0, "SECTION"},
                                 {2, "OBJECTS"},
                                 {0, "SORTENTSTABLE"},
                                 {5, "B"},
                                 {100, "AcDbSortentsTable"},
                                 {331, "A"},
                                 {5, "A"},
                                 {331, "A"},
                                 {5, "0"},
                                 {0, "DICTIONARY"},
                                 {5, "C"},
                                 {0, "ENDSEC"},
                                 {0, "EOF"}};
    auto result = readDxf(writeGroups(groups, TextEncoding::Utf8));
    ASSERT_TRUE(result.ok()) << result.error().message;
    EXPECT_EQ(result.value().document.nextId(), 0xDU);
    EXPECT_EQ(result.value().document.add(core::Line{}).value, 0xDU);
    auto duplicate = groups;
    duplicate[20].value = "B";
    const auto bad = readDxf(writeGroups(duplicate, TextEncoding::Utf8));
    ASSERT_FALSE(bad.ok());
    EXPECT_EQ(bad.error().message, "duplicate handle B");
}

TEST(DxfRead, HeaderHandleValuesOtherThanHandseedAreNotDefinitions) {
    const auto result = readDxf(drawing({{0, "POINT"}, {5, "A"}}, {{9, "$CUSTOM"}, {5, "A"}}));
    ASSERT_TRUE(result.ok()) << result.error().message;
    EXPECT_EQ(result.value().document.nextId(), 0xBU);
}

TEST(DxfRead, UnitsDefaultToZeroAndStayInRange) {
    const auto absent = readDxf(drawing({}));
    ASSERT_TRUE(absent.ok());
    EXPECT_EQ(absent.value().document.units(), 0);
    for (const int value : {0, 4, 24, -1, 25}) {
        const auto result = readDxf(drawing({}, {{9, "$INSUNITS"}, {70, std::to_string(value)}}));
        if (value >= 0 && value <= 24) {
            ASSERT_TRUE(result.ok());
            EXPECT_EQ(result.value().document.units(), value);
        } else {
            EXPECT_FALSE(result.ok());
        }
    }
    EXPECT_FALSE(readDxf(drawing({}, {{9, "$INSUNITS"}})).ok());
}

TEST(DxfRead, RejectsMalformedSectionsAndTrailingData) {
    for (const auto* const text :
         {"", "0\nSECTION\n", "0\nSECTION\n2\nENTITIES\n0\nEOF\n",
          "0\nSECTION\n2\nENTITIES\n0\nSECTION\n2\nHEADER\n0\nENDSEC\n0\nEOF\n",
          "0\nENDSEC\n0\nEOF\n", "0\nEOF\n0\nPOINT\n", "0\nEOF\ngarbage",
          "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n",
          "0\nSECTION\n2\nENTITIES\n5\n1\n0\nENDSEC\n0\nEOF\n"}) {
        const auto result = readDxf(text);
        ASSERT_FALSE(result.ok()) << text;
        EXPECT_EQ(result.error().kind, Error::Kind::Malformed);
    }
    EXPECT_TRUE(readDxf("0\nEOF").ok());
    EXPECT_TRUE(readDxf("0\r\nEOF\r\n \r\n").ok());
}

std::optional<core::EntityRecord> probeRead(const core::RawGroups& /*groups*/,
                                            EntityReadContext& context) {
    return core::EntityRecord{context.id, core::Line{}, {}, context.order};
}

void probeWrite(const core::EntityRecord& /*record*/, EntityWriteContext& /*context*/,
                core::RawGroups& /*out*/) {}

TEST(DxfRead, KnownCodecIsGatedByOwnerPaperSpaceAndExtrusion) {
    const EntityCodec codec{"LINE", probeRead, probeWrite};
    EntityReadContext context{"AC1032", "ANSI_1252", {0x20}, {1}, 1};
    for (const core::RawGroups& groups :
         {core::RawGroups{},
          {{67, "0"}, {330, "20"}},
          {{230, "-1"}},
          {{210, "1e-12"}},
          {{102, "{ACAD_REACTORS"}, {330, "FF"}, {102, "}"}, {330, "20"}}}) {
        EXPECT_TRUE(decodeEntity(&codec, groups, context).has_value());
    }
    for (const core::RawGroups& groups : {core::RawGroups{{67, "1"}},
                                          {{330, "21"}},
                                          {{210, "1.1e-12"}},
                                          {{220, "1"}},
                                          {{230, "0"}}}) {
        EXPECT_FALSE(decodeEntity(&codec, groups, context).has_value());
    }
    EXPECT_FALSE(decodeEntity(nullptr, {}, context).has_value());
    EXPECT_EQ(findCodec(std::string_view{"LINE"}), nullptr);
    EXPECT_EQ(findCodec(core::Entity{core::Line{}}), nullptr);
}

}  // namespace
}  // namespace tg::io
