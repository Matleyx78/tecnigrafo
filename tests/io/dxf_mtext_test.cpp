// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace tg::io {
namespace {

EntityReadContext readContext() {
    return {"AC1032", "UTF-8", {0x20}, {0x51}, 1};
}

core::RawGroups entity(const core::RawGroups& body) {
    core::RawGroups groups{{5, "51"}, {330, "20"}, {100, "AcDbEntity"}, {8, "0"}};
    groups.insert(groups.end(), body.begin(), body.end());
    return groups;
}

std::optional<core::EntityRecord> decode(const core::RawGroups& body) {
    auto context = readContext();
    return decodeEntity(findCodec("MTEXT"), entity(body), context);
}

core::RawGroups basicBody(std::string value = "Short") {
    return {{100, "AcDbMText"}, {10, "10"}, {20, "20"}, {40, "3.5"},
            {41, "200"},        {71, "5"},  {72, "1"},  {1, std::move(value)},
            {7, "Standard"},    {50, "30"}, {73, "2"},  {44, "1.5"}};
}

core::RawGroups written(const core::EntityRecord& record) {
    EntityWriteContext context{"AC1032", "UTF-8", {0x20}};
    core::RawGroups groups;
    findCodec(record.entity)->write(record, context, groups);
    return groups;
}

TEST(DxfMText, ShortTextGoesRoundWithAllProperties) {
    const auto record = decode(basicBody());
    ASSERT_TRUE(record.has_value());
    const core::EntityRecord decoded = record.value_or(core::EntityRecord{});
    const auto& mtext = std::get<core::MText>(decoded.entity);
    EXPECT_EQ(mtext.insertion, (core::Vec2{10, 20}));
    EXPECT_EQ(mtext.height, 3.5);
    EXPECT_EQ(mtext.width, 200);
    EXPECT_EQ(mtext.attachment, core::MTextAttachment::MiddleCenter);
    EXPECT_NEAR(mtext.rotation, core::degreesToRadians(30), 1e-12);
    EXPECT_EQ(mtext.style, "Standard");
    EXPECT_EQ(mtext.lineSpacing, 1.5);
    EXPECT_TRUE(mtext.exactSpacing);
    EXPECT_EQ(mtext.value, "Short");

    auto context = readContext();
    const auto again = decodeEntity(findCodec(decoded.entity), written(decoded), context);
    ASSERT_TRUE(again.has_value());
    const core::EntityRecord roundtrip = again.value_or(core::EntityRecord{});
    EXPECT_EQ(roundtrip.entity, decoded.entity);
    EXPECT_EQ(roundtrip.extras, decoded.extras);
}

TEST(DxfMText, LongUtf8TextUsesGroupsOfAtMost250Characters) {
    std::string value;
    for (int i = 0; i < 260; ++i) {
        value += "è";
    }
    const auto record = decode(basicBody(value));
    ASSERT_TRUE(record.has_value());
    const core::RawGroups groups = written(record.value_or(core::EntityRecord{}));
    const auto part = std::find_if(groups.begin(), groups.end(),
                                   [](const core::RawGroup& group) { return group.code == 3; });
    ASSERT_NE(part, groups.end());
    EXPECT_EQ(part->value.size(), 500U);
    EXPECT_TRUE(part->value.ends_with("è"));
    const auto final = std::find_if(part + 1, groups.end(),
                                    [](const core::RawGroup& group) { return group.code == 1; });
    ASSERT_NE(final, groups.end());
    EXPECT_EQ(final->value.size(), 20U);
}

TEST(DxfMText, DirectionVectorIsKeptUntilRotationChanges) {
    core::RawGroups body = basicBody("Vector");
    body.erase(std::remove_if(body.begin(), body.end(),
                              [](const core::RawGroup& group) { return group.code == 50; }),
               body.end());
    body.insert(body.end() - 2, {{11, "0.6"}, {21, "0.8"}, {31, "0.0"}});
    const auto record = decode(body);
    ASSERT_TRUE(record.has_value());
    core::EntityRecord decoded = record.value_or(core::EntityRecord{});
    EXPECT_NEAR(std::get<core::MText>(decoded.entity).rotation, core::atan2(0.8, 0.6), 1e-12);
    core::RawGroups groups = written(decoded);
    const auto hasGroup = [&groups](int code, std::string_view value) {
        return std::ranges::any_of(groups, [code, value](const core::RawGroup& group) {
            return group.code == code && group.value == value;
        });
    };
    EXPECT_TRUE(hasGroup(11, "0.6"));
    EXPECT_TRUE(hasGroup(21, "0.8"));
    EXPECT_TRUE(hasGroup(31, "0.0"));
    EXPECT_FALSE(
        std::ranges::any_of(groups, [](const core::RawGroup& group) { return group.code == 50; }));

    std::get<core::MText>(decoded.entity).rotation = core::kPi / 2;
    groups = written(decoded);
    EXPECT_FALSE(hasGroup(11, "0.6"));
    EXPECT_FALSE(
        std::ranges::any_of(groups, [](const core::RawGroup& group) { return group.code == 50; }));
}

TEST(DxfMText, ColumnsAndInvalidGeometryStayUnhandled) {
    for (const core::RawGroup& invalid :
         {core::RawGroup{75, "2"}, core::RawGroup{101, "Embedded"}}) {
        auto body = basicBody();
        body.push_back(invalid);
        EXPECT_FALSE(decode(body).has_value());
    }
    auto zeroHeight = basicBody();
    zeroHeight.at(3).value = "0";
    EXPECT_FALSE(decode(zeroHeight).has_value());
    auto negativeWidth = basicBody();
    negativeWidth.at(4).value = "-1";
    EXPECT_FALSE(decode(negativeWidth).has_value());
    auto vertical = basicBody();
    vertical.at(6).value = "3";
    EXPECT_FALSE(decode(vertical).has_value());
}

}  // namespace
}  // namespace tg::io
