// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_extrusion.hpp"

#include "dxf_entity_common.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/text.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <string>
#include <string_view>

namespace tg::io {
namespace {

constexpr double kTolerance = 1e-9;

TEST(DxfExtrusion, AxialDirectionsWithinTheTolerance) {
    EXPECT_EQ(classifyExtrusion(0, 0, 1), Extrusion::Up);
    EXPECT_EQ(classifyExtrusion(0, 0, -1), Extrusion::Down);
    EXPECT_EQ(classifyExtrusion(1e-12, -1e-12, 1), Extrusion::Up);
    EXPECT_EQ(classifyExtrusion(1e-12, -1e-12, -1), Extrusion::Down);
    // Only the direction matters: a vector that is not unit is still axial.
    EXPECT_EQ(classifyExtrusion(0, 0, -0.5), Extrusion::Down);
    EXPECT_EQ(classifyExtrusion(1.001e-12, 0, -1), Extrusion::Other);
    EXPECT_EQ(classifyExtrusion(0, -1.001e-12, 1), Extrusion::Other);
    EXPECT_EQ(classifyExtrusion(0, 0.5, 0.866), Extrusion::Other);
    EXPECT_EQ(classifyExtrusion(0, 0, 0), Extrusion::Other);
    EXPECT_EQ(classifyExtrusion(std::numeric_limits<double>::quiet_NaN(), 0, 1), Extrusion::Other);
}

TEST(DxfExtrusion, EntityGroupsGiveTheDirection) {
    EXPECT_EQ(entityExtrusion({}), Extrusion::Up);
    EXPECT_EQ(entityExtrusion({{230, " -1.0"}}), Extrusion::Down);
    EXPECT_EQ(entityExtrusion({{230, "x"}}), Extrusion::Other);
    // Inside 102 groups and in the XDATA the codes mean something else.
    EXPECT_EQ(entityExtrusion({{102, "{APP"}, {230, "-1"}, {102, "}"}}), Extrusion::Up);
    EXPECT_EQ(entityExtrusion({{1001, "APP"}, {230, "-1"}}), Extrusion::Up);
}

TEST(DxfExtrusion, ConversionsAreTheirOwnInverse) {
    EXPECT_EQ(flipX({3, 4}), (core::Vec2{-3, 4}));
    EXPECT_EQ(flipX(flipX({3, 4})), (core::Vec2{3, 4}));
    EXPECT_FALSE(std::signbit(flipX({0, 1}).x));  // no -0 in the file
    EXPECT_FALSE(std::signbit(negate(0.0)));
    EXPECT_EQ(normalizeDegrees(-90), 270);
    EXPECT_EQ(normalizeDegrees(450), 90);
    EXPECT_EQ(normalizeDegrees(360), 0);
    EXPECT_EQ(normalizeDegrees(-1e-15), 0);
    EXPECT_EQ(flipDirectionDegrees(210), 330);
    EXPECT_EQ(flipDirectionDegrees(flipDirectionDegrees(210)), 210);
    EXPECT_EQ(flipDirectionDegrees(0), 180);
}

TEST(DxfExtrusion, AnchorsFollowTheGroupsOfTheSubclasses) {
    const core::RawGroups groups{
        {5, "1"},          {100, "AcDbEntity"}, {39, "9"},  // AcDbEntity: common
        {100, "AcDbText"}, {10, "1"},           {20, "2"},  {30, "3"},     {31, "4"},  {39, "5"},
        {100, "AcDbText"}, {73, "0"},           {210, "0"}, {1001, "APP"}, {230, "-1"}};
    const auto anchors = readPlacementAnchors(groups);
    const std::vector<core::GroupAnchor> expected{{20, 1}, {30, 1}, {31, 1}, {73, 1}};
    EXPECT_EQ(anchors, expected);

    // Back where they were; left alone when an anchor is gone.
    core::EntityExtras extras;
    extras.placement = {{30, "3"}, {31, "4"}, {39, "5"}, {210, "0"}};
    extras.placementAnchors = anchors;
    core::RawGroups out{{5, "1"},          {100, "AcDbText"}, {39, "5"},    {10, "1"},
                        {20, "2"},         {30, "3"},         {31, "4"},    {210, "0"},
                        {100, "AcDbText"}, {73, "0"},         {1001, "APP"}};
    restorePlacement(extras, 1, out);
    const core::RawGroups restored{{5, "1"},  {100, "AcDbText"}, {10, "1"},    {20, "2"},
                                   {30, "3"}, {31, "4"},         {39, "5"},    {100, "AcDbText"},
                                   {73, "0"}, {210, "0"},        {1001, "APP"}};
    EXPECT_EQ(out, restored);
    extras.placementAnchors.back() = {73, 2};
    auto unchanged = restored;
    restorePlacement(extras, 1, unchanged);
    EXPECT_EQ(unchanged, restored);
}

/// A drawing with the given ENTITIES, as the handwritten cases of R11.
std::string drawing(std::string_view entities) {
    return std::string{
               "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n"
               "9\n$HANDSEED\n5\n1000\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"} +
           std::string{entities} + "0\nENDSEC\n0\nEOF\n";
}

core::RawGroups entityGroups(std::string_view dxf) {
    const auto file = readGroups(dxf);
    EXPECT_TRUE(file.ok());
    core::RawGroups groups;
    bool inside = false;
    for (const auto& group : file.value().groups) {
        if (group.code == 2 && group.value == "ENTITIES") {
            inside = true;
        } else if (inside && group.code == 0 && group.value == "ENDSEC") {
            break;
        } else if (inside) {
            groups.push_back(group);
        }
    }
    return groups;
}

void expectSameGroups(const core::RawGroups& actual, const core::RawGroups& expected) {
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i].code, expected[i].code) << "group " << i;
        const auto a = parseNumber(trimDxf(actual[i].value));
        const auto b = parseNumber(trimDxf(expected[i].value));
        if (a && b) {
            EXPECT_NEAR(*a, *b, kTolerance) << "group " << i;
        } else {
            EXPECT_EQ(actual[i].value, expected[i].value) << "group " << i;
        }
    }
}

TEST(DxfExtrusion, MirroredTextAndPolylineThroughAFile) {
    // A backward text (71 = 2) with extrusion (0,0,-1) reads normally in
    // world coordinates; rotation 90 becomes -90 -> 270.
    const std::string_view entities =
        "0\nTEXT\n5\nD1\n100\nAcDbEntity\n8\n0\n100\nAcDbText\n10\n3\n20\n4\n30\n2.5000\n"
        "40\n2\n1\nA\n50\n90\n41\n1\n51\n0\n7\nStandard\n71\n2\n72\n0\n11\n9\n21\n7\n"
        "31\n-3.2500\n39\n0.5000\n100\nAcDbText\n73\n0\n210\n0\n220\n0\n230\n-1\n"
        "0\nLWPOLYLINE\n5\nB5\n100\nAcDbEntity\n8\n0\n100\nAcDbPolyline\n90\n3\n70\n1\n"
        "10\n1\n20\n2\n42\n0.5\n10\n7\n20\n2\n42\n-1\n10\n7\n20\n8\n210\n0\n220\n0\n230\n-1\n";
    const std::string file = drawing(entities);
    const auto read = readDxf(file);
    ASSERT_TRUE(read.ok());
    const auto& document = read.value().document;
    ASSERT_EQ(document.entityCount(), 2U);

    const auto* textRecord = document.find(core::EntityId{0xD1});
    ASSERT_NE(textRecord, nullptr);
    const auto& text = std::get<core::Text>(textRecord->entity);
    EXPECT_EQ(text.position, (core::Vec2{-3, 4}));
    EXPECT_EQ(text.alignmentPoint, (core::Vec2{-9, 7}));
    EXPECT_NEAR(text.rotation, core::degreesToRadians(270), kTolerance);
    EXPECT_FALSE(text.mirroredX);  // mirrored back: it reads normally
    EXPECT_FALSE(text.mirroredY);
    EXPECT_TRUE(textRecord->extras.flippedExtrusion);

    const auto* polylineRecord = document.find(core::EntityId{0xB5});
    ASSERT_NE(polylineRecord, nullptr);
    const auto& polyline = std::get<core::Polyline>(polylineRecord->entity);
    ASSERT_EQ(polyline.vertexCount(), 3U);
    EXPECT_EQ(polyline.vertices()[0].position, (core::Vec2{-1, 2}));
    EXPECT_EQ(polyline.vertices()[0].bulge, -0.5);
    EXPECT_EQ(polyline.vertices()[1].position, (core::Vec2{-7, 2}));
    EXPECT_EQ(polyline.vertices()[1].bulge, 1);
    EXPECT_EQ(polyline.vertices()[2].position, (core::Vec2{-7, 8}));
    EXPECT_TRUE(polyline.isClosed());

    const auto written = writeDxf(document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    expectSameGroups(entityGroups(written.value()), entityGroups(file));
}

TEST(DxfExtrusion, ObliqueExtrusionStaysUnhandledAndIsWrittenIdentical) {
    const std::string_view entities =
        "0\nCIRCLE\n5\nF2\n100\nAcDbEntity\n8\n0\n100\nAcDbCircle\n10\n2\n20\n3\n30\n0\n"
        "40\n4\n210\n0\n220\n0.5\n230\n0.866\n"
        "0\nTEXT\n5\nF5\n100\nAcDbEntity\n8\n0\n100\nAcDbText\n10\n1\n20\n2\n40\n1\n1\nA\n"
        "100\nAcDbText\n210\n0\n220\n0.5\n230\n0.866\n";
    const std::string file = drawing(entities);
    const auto read = readDxf(file);
    ASSERT_TRUE(read.ok());
    EXPECT_EQ(read.value().document.entityCount(), 0U);
    EXPECT_EQ(read.value().report.unhandled.at("CIRCLE"), 1);
    EXPECT_EQ(read.value().report.unhandled.at("TEXT"), 1);
    const auto written = writeDxf(read.value().document, DxfVersion::R2018);
    ASSERT_TRUE(written.ok());
    EXPECT_EQ(entityGroups(written.value()), entityGroups(file));
}

}  // namespace
}  // namespace tg::io
