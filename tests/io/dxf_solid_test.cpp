// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "tg/core/solid.hpp"

#include <gtest/gtest.h>

#include <ranges>

namespace tg::io {
namespace {

EntityReadContext readContext() {
    return {"AC1032", "ANSI_1252", {0x20}, {0x51}, 1};
}

core::RawGroups entity(const core::RawGroups& body) {
    core::RawGroups groups{{5, "51"}, {330, "20"}, {100, "AcDbEntity"}, {8, "0"}};
    groups.insert(groups.end(), body.begin(), body.end());
    return groups;
}

std::optional<core::EntityRecord> decode(const core::RawGroups& body) {
    auto context = readContext();
    return decodeEntity(findCodec("SOLID"), entity(body), context);
}

TEST(DxfSolid, ReadsThreeAndFourVertices) {
    const auto triangle = decode(
        {{100, "AcDbTrace"}, {10, "0"}, {20, "1"}, {11, "2"}, {21, "3"}, {12, "4"}, {22, "5"}});
    ASSERT_TRUE(triangle.has_value());
    const core::EntityRecord triangleRecord = triangle.value_or(core::EntityRecord{});
    const auto& three = std::get<core::Solid>(triangleRecord.entity);
    EXPECT_EQ(three.vertices, (std::array<core::Vec2, 4>{{{0, 1}, {2, 3}, {4, 5}, {4, 5}}}));
    EXPECT_FALSE(three.fourthVertexPresent);

    const auto quadrilateral = decode({{100, "AcDbTrace"},
                                       {10, "0"},
                                       {20, "1"},
                                       {11, "2"},
                                       {21, "3"},
                                       {12, "4"},
                                       {22, "5"},
                                       {13, "6"},
                                       {23, "7"}});
    ASSERT_TRUE(quadrilateral.has_value());
    const core::EntityRecord quadrilateralRecord = quadrilateral.value_or(core::EntityRecord{});
    EXPECT_EQ(std::get<core::Solid>(quadrilateralRecord.entity).vertices[3], (core::Vec2{6, 7}));
}

TEST(DxfSolid, WritesThreeVerticesAndPreservesFourthElevation) {
    const auto record = decode({{100, "AcDbTrace"},
                                {10, "0"},
                                {20, "1"},
                                {30, "2"},
                                {11, "3"},
                                {21, "4"},
                                {31, "5"},
                                {12, "6"},
                                {22, "7"},
                                {32, "8"},
                                {33, "9"}});
    ASSERT_TRUE(record.has_value());
    const core::EntityRecord decoded = record.value_or(core::EntityRecord{});

    EntityWriteContext context{"AC1032", "ANSI_1252", {0x20}};
    core::RawGroups written;
    findCodec(decoded.entity)->write(decoded, context, written);

    auto geometry = written | std::views::filter([](const core::RawGroup& group) {
                        return group.code >= 10 && group.code <= 33;
                    });
    EXPECT_EQ(core::RawGroups(geometry.begin(), geometry.end()), (core::RawGroups{{10, "0.0"},
                                                                                  {20, "1.0"},
                                                                                  {30, "2"},
                                                                                  {11, "3.0"},
                                                                                  {21, "4.0"},
                                                                                  {31, "5"},
                                                                                  {12, "6.0"},
                                                                                  {22, "7.0"},
                                                                                  {32, "8"},
                                                                                  {33, "9"}}));
}

TEST(DxfSolid, NegativeExtrusionAndUnknownGroupsSurviveRoundtrip) {
    const core::RawGroups body{{100, "AcDbTrace"}, {39, "2.5"}, {10, "1"},     {20, "2"},
                               {11, "3"},          {21, "4"},   {12, "5"},     {22, "6"},
                               {13, "7"},          {23, "8"},   {440, "42"},   {210, "0"},
                               {220, "0"},         {230, "-1"}, {1001, "APP"}, {1000, "opaque"}};
    const auto record = decode(body);
    ASSERT_TRUE(record.has_value());
    const core::EntityRecord decoded = record.value_or(core::EntityRecord{});
    const auto& solid = std::get<core::Solid>(decoded.entity);
    EXPECT_EQ(solid.vertices[0], (core::Vec2{-1, 2}));
    EXPECT_TRUE(decoded.extras.flippedExtrusion);
    EXPECT_EQ(decoded.extras.specific, (core::RawGroups{{440, "42"}}));

    EntityWriteContext context{"AC1032", "ANSI_1252", {0x20}};
    core::RawGroups written;
    findCodec(decoded.entity)->write(decoded, context, written);
    auto rereadContext = readContext();
    const auto reread = decodeEntity(findCodec("SOLID"), written, rereadContext);
    ASSERT_TRUE(reread.has_value());
    const core::EntityRecord roundtrip = reread.value_or(core::EntityRecord{});
    EXPECT_EQ(roundtrip.entity, decoded.entity);
    EXPECT_EQ(roundtrip.extras, decoded.extras);
}

TEST(DxfSolid, RejectsInvalidAndCollapsedEntities) {
    EXPECT_FALSE(decode({{100, "AcDbTrace"}, {10, "0"}, {20, "0"}, {11, "1"}, {21, "1"}}));
    EXPECT_FALSE(decode(
        {{100, "AcDbTrace"}, {10, "1"}, {20, "1"}, {11, "1"}, {21, "1"}, {12, "1"}, {22, "1"}}));
    EXPECT_FALSE(decode({{100, "AcDbTrace"},
                         {10, "0"},
                         {20, "0"},
                         {11, "1"},
                         {21, "0"},
                         {12, "0"},
                         {22, "1"},
                         {210, "1"}}));
}

}  // namespace
}  // namespace tg::io
