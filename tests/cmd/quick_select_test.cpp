// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/quick_select.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace tg::cmd {
namespace {

TEST(QuickSelectTest, FiltersOwnPropertiesAndCombinesTypeWithinAndExclude) {
    auto document = core::Document::newDrawing();
    const auto first = document.add(core::Line{{0, 0}, {10, 0}});
    const auto second = document.add(core::Circle{{0, 0}, 5});
    auto properties = document.find(first)->properties;
    properties.color = core::Color::fromIndex(7);
    properties.lineweight = 50;
    properties.linetype = document.continuousLinetype();
    document.setProperties(first, properties);

    QuickSelectCriteria criteria;
    EXPECT_EQ(quickSelect(document, {}, criteria), (std::vector{first, second}));
    criteria.property = QuickSelectCriteria::Property::Color;
    criteria.value = "BYLAYER";
    EXPECT_EQ(quickSelect(document, {}, criteria), (std::vector{second}));
    criteria.op = QuickSelectCriteria::Operator::NotEqual;
    EXPECT_EQ(quickSelect(document, {}, criteria), (std::vector{first}));
    criteria.type = "circle";
    EXPECT_TRUE(quickSelect(document, {}, criteria).empty());
    criteria.exclude = true;
    EXPECT_EQ(quickSelect(document, {}, criteria), (std::vector{first, second}));
    const std::vector within{second, first, second, core::EntityId{9999}};
    EXPECT_EQ(quickSelect(document, within, criteria), (std::vector{first, second}));
    criteria = {};
    criteria.property = QuickSelectCriteria::Property::Layer;
    criteria.value = "0";
    EXPECT_EQ(quickSelect(document, {}, criteria), (std::vector{first, second}));
    criteria.property = QuickSelectCriteria::Property::Linetype;
    criteria.value = "cOnTiNuOuS";
    EXPECT_EQ(quickSelect(document, {}, criteria), (std::vector{first}));
    criteria.op = QuickSelectCriteria::Operator::NotEqual;
    EXPECT_EQ(quickSelect(document, {}, criteria), (std::vector{second}));
    criteria.property = QuickSelectCriteria::Property::Lineweight;
    criteria.value = "0.50";
    criteria.op = QuickSelectCriteria::Operator::Equal;
    EXPECT_EQ(quickSelect(document, {}, criteria), (std::vector{first}));
    criteria.op = QuickSelectCriteria::Operator::NotEqual;
    EXPECT_EQ(quickSelect(document, {}, criteria), (std::vector{second}));
}

TEST(QuickSelectTest, InvalidValuesNeverMatchEvenWithNotEqual) {
    auto document = core::Document::newDrawing();
    const auto id = document.add(core::Line{{0, 0}, {10, 0}});
    using Property = QuickSelectCriteria::Property;
    using Operator = QuickSelectCriteria::Operator;
    for (const auto property : {Property::Color, Property::Lineweight}) {
        for (const auto op : {Operator::Equal, Operator::NotEqual}) {
            QuickSelectCriteria criteria;
            criteria.property = property;
            criteria.op = op;
            criteria.value = "invalid";
            EXPECT_TRUE(quickSelect(document, {}, criteria).empty());
            criteria.exclude = true;
            EXPECT_EQ(quickSelect(document, {}, criteria), (std::vector{id}));
        }
    }
}

TEST(QuickSelectTest, LayerStatesAreExcludedBeforeInversion) {
    auto document = core::Document::newDrawing();
    const auto id = document.add(core::Line{{0, 0}, {10, 0}});
    auto layer = *document.layer(document.layerZero());
    for (int state = 0; state < 3; ++state) {
        layer.on = state != 0;
        layer.frozen = state == 1;
        layer.locked = state == 2;
        document.replaceLayer(layer);
        QuickSelectCriteria criteria;
        EXPECT_TRUE(quickSelect(document, {}, criteria).empty());
        criteria.type = "CIRCLE";
        criteria.exclude = true;
        EXPECT_TRUE(quickSelect(document, std::vector{id}, criteria).empty());
    }
}

TEST(QuickSelectTest, IdentifierOrderDoesNotDependOnDrawingOrder) {
    core::Document document;
    document.insert({{9}, core::Line{{0, 0}, {1, 0}}, {}, 1});
    document.insert({{2}, core::Line{{0, 0}, {1, 0}}, {}, 2});
    const std::vector expected{core::EntityId{2}, core::EntityId{9}};
    EXPECT_EQ(quickSelect(document, {}, {}), expected);
}

}  // namespace
}  // namespace tg::cmd
