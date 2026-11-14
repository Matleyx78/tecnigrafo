// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"

#include <gtest/gtest.h>

#include <numbers>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

Solid sampleSolid() {
    return Solid{{Vec2{0, 0}, Vec2{2, 0}, Vec2{0, 2}, Vec2{2, 2}}};
}

Ellipse sampleEllipse() {
    return Ellipse{{1, 2}, {4, 0}, 0.5, 0.0, 2.0 * std::numbers::pi};
}

MText sampleMText() {
    MText mtext;
    mtext.insertion = {3, 4};
    mtext.width = 20.0;
    mtext.attachment = MTextAttachment::MiddleCenter;
    mtext.value = "first\\Psecond";
    return mtext;
}

AttributeDefinition sampleDefinition() {
    AttributeDefinition definition;
    definition.text.position = {1, 1};
    definition.text.value = "default";
    definition.tag = "TAG";
    definition.prompt = "Value?";
    definition.flags.invisible = true;
    return definition;
}

Insert sampleInsert() {
    Insert insert;
    insert.block = "A";
    insert.insertion = {5, 5};
    insert.scaleX = 2.0;
    Attribute attribute;
    attribute.id = EntityId{0x40};
    attribute.tag = "TAG";
    attribute.text.value = "value";
    insert.attributes.push_back(attribute);
    return insert;
}

Dimension sampleDimension() {
    Dimension dimension;
    dimension.definition.kind = DimensionKind::Aligned;
    dimension.definition.dxfType = 1;
    dimension.definition.point13 = {0, 0};
    dimension.definition.point14 = {10, 0};
    dimension.presentation.textRotation = 0.5;
    dimension.presentation.overrides.emplace_back("DIMSCALE", "2");
    dimension.value.measurement = 10.0;
    dimension.graphics.push_back(EntityRecord{EntityId{0x50}, Line{{0, 0}, {10, 0}}});
    dimension.rawGraphics.push_back(RawEntity{EntityId{0x51}, 2, "POINT", {}});
    return dimension;
}

Hatch sampleHatch() {
    Hatch hatch;
    hatch.pattern = "ANSI31";
    hatch.islands = IslandStyle::Outer;
    hatch.patternLines.push_back(PatternLine{0.25, {0, 0}, {1, 1}, {1.0, -0.5}});
    hatch.seeds.emplace_back(1, 1);
    HatchLoop loop;
    loop.flags = 1;
    loop.edges.push_back(HatchEdge{Line{{0, 0}, {4, 0}}});
    loop.edges.push_back(HatchEdge{Arc{{2, 0}, 2.0, 0.0, std::numbers::pi}});
    hatch.loops.push_back(loop);
    return hatch;
}

/// Each new type, built with values other than the defaults.
std::vector<Entity> newEntities() {
    return {sampleSolid(),  sampleEllipse(),   sampleMText(), sampleDefinition(),
            sampleInsert(), sampleDimension(), sampleHatch()};
}

TEST(NewEntities, AreBuiltCopiedAndCompared) {
    for (const Entity& entity : newEntities()) {
        const Entity copy = entity;
        EXPECT_EQ(copy, entity);
        EXPECT_NE(copy, Entity{std::in_place_index<0>}) << copy.index();
    }
    EXPECT_NE(Entity{sampleSolid()}, Entity{Solid{}});
    EXPECT_NE(Entity{sampleEllipse()}, Entity{Ellipse{}});
    EXPECT_NE(Entity{sampleMText()}, Entity{MText{}});
    EXPECT_NE(Entity{sampleDefinition()}, Entity{AttributeDefinition{}});
    EXPECT_NE(Entity{sampleInsert()}, Entity{Insert{}});
    EXPECT_NE(Entity{sampleDimension()}, Entity{Dimension{}});
    EXPECT_NE(Entity{sampleHatch()}, Entity{Hatch{}});
}

TEST(NewEntities, EachFieldTakesPartInTheComparison) {
    Insert insert = sampleInsert();
    insert.attributes.front().properties.linetypeScale = 2.0;
    EXPECT_NE(insert, sampleInsert());

    Dimension dimension = sampleDimension();
    dimension.graphics.front().entity = Line{{0, 0}, {10, 1}};
    EXPECT_NE(dimension, sampleDimension());
    dimension = sampleDimension();
    dimension.presentation.textRotation.reset();
    EXPECT_NE(dimension, sampleDimension());

    Hatch hatch = sampleHatch();
    hatch.loops.front().edges.back().curve = Line{{4, 0}, {0, 0}};
    EXPECT_NE(hatch, sampleHatch());

    AttributeDefinition definition = sampleDefinition();
    definition.flags.preset = true;
    EXPECT_NE(definition, sampleDefinition());
}

TEST(NewEntities, DefaultsFollowTheDxfDefaults) {
    EXPECT_EQ(MText{}.attachment, MTextAttachment::TopLeft);
    EXPECT_EQ(static_cast<int>(MTextAttachment::BottomRight), 9);
    EXPECT_EQ(MText{}.style, "Standard");
    EXPECT_EQ(Ellipse{}.ratio, 1.0);
    EXPECT_EQ(Insert{}.scaleX, 1.0);
    EXPECT_EQ(Insert{}.scaleY, 1.0);
    EXPECT_EQ(Dimension{}.presentation.style, "ISO-25");
    EXPECT_EQ(Dimension{}.value.source, ValueSource::Measured);
    EXPECT_EQ(Hatch{}.patternType, 1);
    EXPECT_EQ(Hatch{}.islands, IslandStyle::Normal);
}

TEST(NewEntities, AreNotSupportedYet) {
    for (const Entity& entity : newEntities()) {
        EXPECT_FALSE(isSupported(entity)) << entity.index();
    }
}

TEST(NewEntities, TheFiveTypesOfBeforeAreSupported) {
    const std::vector<Entity> entities{Polyline{}, Line{{0, 0}, {1, 0}}, Circle{{0, 0}, 1.0},
                                       Arc{{0, 0}, 1.0, 0.0, 1.0}, Text{}};
    for (const Entity& entity : entities) {
        EXPECT_TRUE(isSupported(entity)) << entity.index();
    }
}

TEST(NewEntities, TheVariantKeepsTheOrderOfTheAdr) {
    // docs/adr/0044, point 1: the new types follow the five of before.
    EXPECT_EQ(Entity{sampleSolid()}.index(), 5U);
    EXPECT_EQ(Entity{sampleEllipse()}.index(), 6U);
    EXPECT_EQ(Entity{sampleMText()}.index(), 7U);
    EXPECT_EQ(Entity{sampleDefinition()}.index(), 8U);
    EXPECT_EQ(Entity{sampleInsert()}.index(), 9U);
    EXPECT_EQ(Entity{sampleDimension()}.index(), 10U);
    EXPECT_EQ(Entity{sampleHatch()}.index(), 11U);
    EXPECT_EQ(std::variant_size_v<Entity>, 12U);
}

}  // namespace
}  // namespace tg::core
