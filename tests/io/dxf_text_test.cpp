// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "tg/core/math.hpp"
#include "tg/io/dxf_groups.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <random>

namespace tg::io {
namespace {
// Integer engine output scaled directly keeps fixtures reproducible on every platform.
class FixtureNumbers {
public:
    explicit FixtureNumbers(std::uint32_t seed) : m_engine(seed) {}

    double next() { return static_cast<double>(m_engine()) / 10000.0; }

private:
    std::mt19937 m_engine;
};

TEST(DxfText, RandomPropertiesAndExtrasSurviveRoundtrip) {
    FixtureNumbers random(20261023);
    EntityReadContext read{"AC1032", "ANSI_1252", {0x20}, {0x51}, 1};
    EntityWriteContext write{"AC1032", "ANSI_1252", {0x20}};
    for (int trial = 0; trial < 300; ++trial) {
        core::Text text;
        text.position = {random.next(), -3.5};
        text.alignmentPoint = {2.25, random.next()};
        text.height = 1 + random.next();
        text.rotation = core::degreesToRadians(static_cast<double>(trial));
        text.obliqueAngle = core::degreesToRadians(15.0);
        text.widthFactor = 0.75;
        text.style = "Accenté";
        text.value = "Città %%c10 %%d %%p0.1";
        text.mirroredX = trial % 2 == 0;
        text.mirroredY = trial % 3 == 0;
        text.horizontal = static_cast<core::TextHorizontal>(trial % 6);
        text.vertical = static_cast<core::TextVertical>(trial % 4);
        core::EntityExtras extras;
        extras.beforeEntity = {{102, "{ACAD_XDICTIONARY"}, {360, "F"}, {102, "}"}, {330, "20"}};
        extras.common = {{8, "text"}, {420, "1234"}};
        extras.placement = {{39, "3.00"}, {30, "1.00"}, {31, "2.00"}, {230, "1.0"}};
        const int flags = 8 | (text.mirroredX ? 2 : 0) | (text.mirroredY ? 4 : 0);
        extras.specific = {{71, std::to_string(flags)}, {999, "opaque"}};
        extras.extended = {{1001, "APP"}, {1000, "keep"}, {1070, "2"}};
        core::RawGroups groups;
        kTextCodec.write({read.id, text, extras, 1}, write, groups);
        const auto result = decodeEntity(&kTextCodec, groups, read);
        ASSERT_TRUE(result.has_value());
        const auto decoded = result.value_or(core::EntityRecord{});
        const auto& actual = std::get<core::Text>(decoded.entity);
        auto expected = text;
        EXPECT_NEAR(actual.rotation, text.rotation, 1e-9);
        EXPECT_NEAR(actual.obliqueAngle, text.obliqueAngle, 1e-9);
        expected.rotation = actual.rotation;
        expected.obliqueAngle = actual.obliqueAngle;
        EXPECT_EQ(actual, expected);
        EXPECT_EQ(decoded.extras, extras);
        auto it = std::ranges::find_if(groups, [](const auto& group) { return group.code == 73; });
        ASSERT_NE(it, groups.begin());
        EXPECT_EQ((it - 1)->code, 100);
        EXPECT_EQ((it - 1)->value, "AcDbText");
    }
}

TEST(DxfText, DefaultsAndUnsupportedProperties) {
    EntityReadContext context{"AC1032", "ANSI_1252", {}, {1}, 1};
    const core::RawGroups valid{{5, "1"},  {100, "AcDbEntity"}, {8, "0"},  {100, "AcDbText"},
                                {10, "1"}, {20, "2"},           {40, "3"}, {1, "é %%c"}};
    const auto result = decodeEntity(&kTextCodec, valid, context);
    ASSERT_TRUE(result.has_value());
    const auto decoded = result.value_or(core::EntityRecord{});
    const auto& text = std::get<core::Text>(decoded.entity);
    EXPECT_EQ(text.widthFactor, 1);
    EXPECT_EQ(text.style, "Standard");
    EXPECT_EQ(text.alignmentPoint, (core::Vec2{0, 0}));
    EXPECT_EQ(text.value, "é %%c");
    for (const core::RawGroup& property : {core::RawGroup{40, "0"},
                                           {40, "-1"},
                                           {41, "0"},
                                           {41, "-1"},
                                           {72, "6"},
                                           {72, "-1"},
                                           {73, "4"},
                                           {73, "-1"},
                                           {230, "-1"}}) {
        auto groups = valid;
        groups.push_back(property);
        EXPECT_FALSE(decodeEntity(&kTextCodec, groups, context));
    }
}

TEST(DxfText, NewEntityUsesStandardDefaultsAndTwoTextSubclasses) {
    EntityWriteContext context{"AC1032", "ANSI_1252", {0x20}};
    core::RawGroups groups;
    kTextCodec.write({{1}, core::Text{}}, context, groups);
    EXPECT_EQ(groups[1], (core::RawGroup{330, "20"}));
    EXPECT_EQ(groups[3], (core::RawGroup{8, "0"}));
    EXPECT_EQ(groups[4], (core::RawGroup{100, "AcDbText"}));
    EXPECT_EQ(groups[groups.size() - 2], (core::RawGroup{100, "AcDbText"}));
    EXPECT_EQ(groups.back(), (core::RawGroup{73, "0"}));
}
}  // namespace
}  // namespace tg::io
