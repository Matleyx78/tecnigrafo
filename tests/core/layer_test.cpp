// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/layer.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>

namespace tg::core {
namespace {

TEST(TableName, OrdinaryNamesAreValid) {
    EXPECT_TRUE(isValidTableName("0"));
    EXPECT_TRUE(isValidTableName("Walls"));
    EXPECT_TRUE(isValidTableName("Walls - outer"));
    EXPECT_TRUE(isValidTableName("A_B.c-d$e"));
    EXPECT_TRUE(isValidTableName("Pareti esterne"));
    EXPECT_TRUE(isValidTableName("Quote è più"));
}

TEST(TableName, EveryForbiddenCharacterMakesTheNameInvalid) {
    for (const char forbidden : std::string_view{"<>/\\\":;?*|,=`"}) {
        const std::string name = std::string{"A"} + forbidden + "B";
        EXPECT_FALSE(isValidTableName(name)) << name;
        EXPECT_FALSE(isValidTableName(std::string(1, forbidden))) << forbidden;
    }
}

TEST(TableName, EmptyAndBlankEndsAreInvalid) {
    EXPECT_FALSE(isValidTableName(""));
    EXPECT_FALSE(isValidTableName(" "));
    EXPECT_FALSE(isValidTableName(" Walls"));
    EXPECT_FALSE(isValidTableName("Walls "));
    EXPECT_FALSE(isValidTableName(" Walls "));
    EXPECT_TRUE(isValidTableName("Outer walls"));
}

TEST(TableName, AtMost255Characters) {
    constexpr std::size_t kLongest = 255;
    EXPECT_TRUE(isValidTableName(std::string(kLongest, 'a')));
    EXPECT_FALSE(isValidTableName(std::string(kLongest + 1, 'a')));

    // Characters, not bytes: "è" takes two bytes in UTF-8.
    std::string accented;
    for (std::size_t i = 0; i < kLongest; ++i) {
        accented += "\xC3\xA8";
    }
    EXPECT_TRUE(isValidTableName(accented));
    EXPECT_FALSE(isValidTableName(accented + "a"));
}

TEST(TableName, ComparisonIgnoresTheCaseOfAsciiLetters) {
    EXPECT_TRUE(sameTableName("Walls", "WALLS"));
    EXPECT_TRUE(sameTableName("bylayer", "ByLayer"));
    EXPECT_TRUE(sameTableName("Continuous", "Continuous"));
    EXPECT_FALSE(sameTableName("Walls", "Wall"));
    EXPECT_FALSE(sameTableName("Walls", "Walls "));
    EXPECT_FALSE(sameTableName("A1", "A2"));
    // Only ASCII letters fold: "È" and "è" stay different.
    EXPECT_FALSE(sameTableName("\xC3\x88", "\xC3\xA8"));
}

TEST(EntityProperties, DefaultsAreThoseOfAnEntityWithoutGroups) {
    const EntityProperties properties;
    EXPECT_EQ(properties.layer, LayerId{});
    EXPECT_EQ(properties.color, Color::byLayer());
    EXPECT_EQ(properties.linetype, LinetypeId{});
    EXPECT_EQ(properties.lineweight, lineweight::kByLayer);
    EXPECT_DOUBLE_EQ(properties.linetypeScale, 1.0);
}

TEST(Layer, DefaultsAreThoseOfANewLayer) {
    const Layer layer;
    EXPECT_EQ(layer.color, Color::fromIndex(7));
    EXPECT_EQ(layer.lineweight, lineweight::kDefault);
    EXPECT_TRUE(layer.on);
    EXPECT_FALSE(layer.frozen);
    EXPECT_FALSE(layer.locked);
    EXPECT_TRUE(layer.plot);
    EXPECT_TRUE(layer.extras.empty());
}

}  // namespace
}  // namespace tg::core
