// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/effective_properties.hpp"

#include <gtest/gtest.h>

#include <array>

namespace tg::core {
namespace {

TEST(EffectiveProperties, ResolvesIndependentPropertyCombinations) {
    Document document = Document::newDrawing();
    const auto type = document.addLinetype({{}, "Dashed", "", {}, {}});
    Layer layer;
    layer.name = "Custom";
    layer.color = Color::fromRgb({10, 20, 30});
    layer.linetype = type;
    layer.lineweight = 50;
    const auto layerId = document.addLayer(layer);
    document.setLwdefault(70);
    document.setLtscale(3.0);
    const auto id = document.add(Line{{0, 0}, {10, 0}});
    const std::array colors{Color::byLayer(), Color::byBlock(), Color::fromIndex(1)};
    const std::array types{document.byLayerLinetype(), document.byBlockLinetype(), type};
    const std::array weights{lineweight::kByLayer, lineweight::kByBlock, lineweight::kDefault, 0};
    for (const auto& color : colors) {
        for (const auto linetype : types) {
            for (const int weight : weights) {
                document.setProperties(id, {layerId, color, linetype, weight, 2.0});
                const auto result = effectiveProperties(document, *document.find(id));
                Rgb expectedColor{255, 0, 0};
                if (color.kind() == Color::Kind::ByLayer) {
                    expectedColor = {10, 20, 30};
                } else if (color.kind() == Color::Kind::ByBlock) {
                    expectedColor = {255, 255, 255};
                }
                EXPECT_EQ(result.color, expectedColor);
                EXPECT_EQ(result.colorIsSeven, color.kind() == Color::Kind::ByBlock);
                const auto expectedType = linetype == document.byBlockLinetype()
                                              ? document.continuousLinetype()
                                              : type;
                EXPECT_EQ(result.linetype->id, expectedType);
                int expectedWeight = 70;
                if (weight == lineweight::kByLayer) {
                    expectedWeight = 50;
                } else if (weight == 0) {
                    expectedWeight = 0;
                }
                EXPECT_EQ(result.lineweight, expectedWeight);
                EXPECT_DOUBLE_EQ(result.linetypeScale, 6.0);
                EXPECT_TRUE(result.visible);
                EXPECT_FALSE(result.locked);
            }
        }
    }
}

TEST(EffectiveProperties, SevenRetainsItsIdentityButRgbWhiteDoesNot) {
    Document document = Document::newDrawing();
    const auto id = document.add(Line{{0, 0}, {1, 0}});
    auto properties = document.find(id)->properties;
    for (const Color color : {Color::byLayer(), Color::byBlock(), Color::fromIndex(7),
                             Color::fromRgb({255, 255, 255})}) {
        properties.color = color;
        document.setProperties(id, properties);
        const auto result = effectiveProperties(document, *document.find(id));
        EXPECT_EQ(result.color, (Rgb{255, 255, 255}));
        EXPECT_EQ(result.colorIsSeven, color.kind() != Color::Kind::Rgb);
    }
    Layer layer = *document.layer(document.layerZero());
    layer.color = Color::fromRgb({255, 255, 255});
    document.replaceLayer(layer);
    properties.color = Color::byLayer();
    document.setProperties(id, properties);
    EXPECT_FALSE(effectiveProperties(document, *document.find(id)).colorIsSeven);
}

TEST(EffectiveProperties, LayerDefaultUsesDocumentDefault) {
    Document document = Document::newDrawing();
    const auto id = document.add(Line{{0, 0}, {1, 0}});
    document.setLwdefault(90);
    EXPECT_EQ(effectiveProperties(document, *document.find(id)).lineweight, 90);
}

TEST(EffectiveProperties, TableFreeDocumentHasDrawingDefaults) {
    Document document;
    const auto id = document.add(Line{{0, 0}, {1, 0}});
    const auto result = effectiveProperties(document, *document.find(id));
    EXPECT_EQ(result.color, (Rgb{255, 255, 255}));
    EXPECT_TRUE(result.colorIsSeven);
    ASSERT_NE(result.linetype, nullptr);
    EXPECT_EQ(result.linetype->name, "Continuous");
    EXPECT_EQ(result.lineweight, 25);
    EXPECT_TRUE(result.visible);
    EXPECT_FALSE(result.locked);
}

}  // namespace
}  // namespace tg::core
