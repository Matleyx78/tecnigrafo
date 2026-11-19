// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/explode.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/solid.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kEps = 1e-9;

void expectPoint(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kEps);
    EXPECT_NEAR(actual.y, expected.y, kEps);
}

/// An entity of a block, on layer 0 with linetype ByLayer, with order key
/// `order`.
EntityRecord member(const Document& document, Entity entity, std::uint64_t order = 0) {
    EntityRecord record{EntityId{}, std::move(entity)};
    record.properties.layer = document.layerZero();
    record.properties.linetype = document.byLayerLinetype();
    record.order = order;
    return record;
}

BlockDefinition block(const Document& document, std::string name, std::vector<Entity> entities) {
    BlockDefinition definition{std::move(name)};
    std::uint64_t order = 1;
    for (Entity& entity : entities) {
        definition.entities.push_back(member(document, std::move(entity), order++));
    }
    return definition;
}

Insert insertOf(std::string name, const Vec2& insertion, double scaleX = 1.0, double scaleY = 1.0,
                double degrees = 0.0) {
    Insert insert;
    insert.block = std::move(name);
    insert.insertion = insertion;
    insert.scaleX = scaleX;
    insert.scaleY = scaleY;
    insert.rotation = degreesToRadians(degrees);
    return insert;
}

AttributeDefinition attributeDefinition(std::string tag, const Vec2& position, bool constant) {
    AttributeDefinition definition;
    definition.text.position = position;
    definition.text.value = "value";
    definition.tag = std::move(tag);
    definition.prompt = "Prompt";
    definition.flags.constant = constant;
    return definition;
}

MText mtextAt(const Vec2& insertion) {
    MText mtext;
    mtext.insertion = insertion;
    mtext.value = "Note";
    return mtext;
}

/// A raw entity of the block with the groups `groups` and order key `order`.
RawEntity rawEntity(std::uint64_t id, std::uint64_t order, std::string type, RawGroups groups) {
    return RawEntity{EntityId{id}, order, std::move(type), std::move(groups)};
}

template <typename T>
const T& entityAt(const Exploded& exploded, std::size_t index) {
    return std::get<T>(exploded.entities.at(index).entity);
}

TEST(ExplodeInsert, RotatedAndScaledByTwoMapsTheEntitiesWithTheirProperties) {
    Document document = Document::newDrawing();
    BlockDefinition definition = block(document, "Piece", {Line{{0, 0}, {10, 0}}});
    definition.entities[0].properties.color = Color::fromIndex(1);
    definition.entities[0].extras.beforeEntity = {
        {102, "{ACAD_REACTORS"}, {330, "1F"}, {102, "}"}, {330, "2A"}};
    definition.entities[0].extras.extended = {{1001, "APP"}, {1000, "kept"}};
    document.replaceBlock("Piece", definition);
    const Exploded exploded = explodeInsert(document, insertOf("Piece", {20, 10}, 2.0, 2.0, 90.0));
    ASSERT_TRUE(exploded.refusal.empty()) << exploded.refusal;
    ASSERT_EQ(exploded.entities.size(), 1U);
    EXPECT_TRUE(exploded.raw.empty());
    const auto& line = entityAt<Line>(exploded, 0);
    expectPoint(line.start, {20, 10});
    expectPoint(line.end, {20, 30});
    const EntityRecord& record = exploded.entities[0];
    EXPECT_EQ(record.id, EntityId{});
    EXPECT_EQ(record.properties, definition.entities[0].properties);
    EXPECT_EQ(record.extras.beforeEntity, (RawGroups{{330, "2A"}}));
    EXPECT_EQ(record.extras.extended, definition.entities[0].extras.extended);
}

TEST(ExplodeInsert, ANestedBlockStaysAnInsertWithTheComposedTransformation) {
    Document document = Document::newDrawing();
    document.replaceBlock("Leaf", block(document, "Leaf", {Circle{{1, 0}, 1}}));
    Insert nested = insertOf("Leaf", {5, 0}, 1.0, 1.0, 0.0);
    Attribute attribute;
    attribute.id = EntityId{0x50};
    attribute.tag = "TAG";
    attribute.text.position = {5, 1};
    attribute.text.value = "A";
    attribute.extras.beforeEntity = {{102, "{ACAD_XDICTIONARY"}, {360, "9"}, {102, "}"}};
    nested.attributes.push_back(attribute);
    document.replaceBlock("Parent", block(document, "Parent", {nested}));
    const Exploded exploded = explodeInsert(document, insertOf("Parent", {10, 10}, 2.0, 2.0, 90.0));
    ASSERT_TRUE(exploded.refusal.empty()) << exploded.refusal;
    ASSERT_EQ(exploded.entities.size(), 1U);
    const auto& insert = entityAt<Insert>(exploded, 0);
    EXPECT_EQ(insert.block, "Leaf");
    expectPoint(insert.insertion, {10, 20});
    EXPECT_NEAR(insert.scaleX, 2.0, kEps);
    EXPECT_NEAR(insert.scaleY, 2.0, kEps);
    EXPECT_NEAR(insert.rotation, kPi / 2.0, kEps);
    // The ATTRIB turns with its insert and takes a new handle.
    ASSERT_EQ(insert.attributes.size(), 1U);
    EXPECT_EQ(insert.attributes[0].id, EntityId{});
    expectPoint(insert.attributes[0].text.position, {8, 20});
    EXPECT_TRUE(insert.attributes[0].extras.beforeEntity.empty());
}

TEST(ExplodeInsert, TheAttributeDefinitionsTakeThePlaceOfTheAttributes) {
    Document document = Document::newDrawing();
    document.replaceBlock("Tagged",
                          block(document, "Tagged",
                                {Line{{0, 0}, {4, 0}}, attributeDefinition("NAME", {0, 1}, false),
                                 attributeDefinition("FIXED", {0, 2}, true)}));
    Insert insert = insertOf("Tagged", {10, 0}, 1.0, 1.0, 0.0);
    insert.attributes = defaultAttributes(document, insert);
    ASSERT_EQ(insert.attributes.size(), 1U);
    const Exploded exploded = explodeInsert(document, insert);
    ASSERT_TRUE(exploded.refusal.empty()) << exploded.refusal;
    ASSERT_EQ(exploded.entities.size(), 3U);
    EXPECT_TRUE(std::holds_alternative<Line>(exploded.entities[0].entity));
    const auto& name = entityAt<AttributeDefinition>(exploded, 1);
    EXPECT_EQ(name.tag, "NAME");
    expectPoint(name.text.position, {10, 1});
    const auto& fixed = entityAt<AttributeDefinition>(exploded, 2);
    EXPECT_EQ(fixed.tag, "FIXED");
    EXPECT_TRUE(fixed.flags.constant);
    expectPoint(fixed.text.position, {10, 2});
}

TEST(ExplodeInsert, ANonUniformScaleTurnsCirclesAndArcsIntoEllipses) {
    Document document = Document::newDrawing();
    document.replaceBlock(
        "Round", block(document, "Round", {Circle{{0, 0}, 1}, Arc{{5, 0}, 1, 0.0, kPi / 2.0}}));
    const Exploded exploded = explodeInsert(document, insertOf("Round", {0, 0}, 2.0, 1.0));
    ASSERT_TRUE(exploded.refusal.empty()) << exploded.refusal;
    ASSERT_EQ(exploded.entities.size(), 2U);
    const auto& full = entityAt<Ellipse>(exploded, 0);
    expectPoint(full.center, {0, 0});
    expectPoint(full.majorAxis, {2, 0});
    EXPECT_NEAR(full.ratio, 0.5, kEps);
    const auto& part = entityAt<Ellipse>(exploded, 1);
    expectPoint(part.center, {10, 0});
    expectPoint(pointAt(part, part.startParameter), {12, 0});
    expectPoint(pointAt(part, part.endParameter), {10, 1});
}

TEST(ExplodeInsert, ANonUniformScaleRefusesANestedBlockRotatedByThirtyDegrees) {
    Document document = Document::newDrawing();
    document.replaceBlock("Leaf", block(document, "Leaf", {Line{{0, 0}, {1, 0}}}));
    document.replaceBlock(
        "Parent",
        block(document, "Parent", {Circle{{0, 0}, 1}, insertOf("Leaf", {0, 0}, 1.0, 1.0, 30.0)}));
    const Exploded exploded = explodeInsert(document, insertOf("Parent", {0, 0}, 2.0, 1.0));
    EXPECT_EQ(exploded.refusal,
              "Block \"Parent\" contains a nested block that cannot stay a block reference.");
    EXPECT_TRUE(exploded.entities.empty());
    EXPECT_TRUE(exploded.raw.empty());
}

TEST(ExplodeInsert, ANonUniformScaleKeepsANestedBlockAtAQuarterTurn) {
    Document document = Document::newDrawing();
    document.replaceBlock("Leaf", block(document, "Leaf", {Line{{0, 0}, {1, 0}}}));
    document.replaceBlock("Parent",
                          block(document, "Parent", {insertOf("Leaf", {1, 1}, 1.0, 1.0, 90.0)}));
    const Insert outer = insertOf("Parent", {0, 0}, 2.0, 1.0);
    const Exploded exploded = explodeInsert(document, outer);
    ASSERT_TRUE(exploded.refusal.empty()) << exploded.refusal;
    const auto& insert = entityAt<Insert>(exploded, 0);
    expectPoint(insert.insertion, {2, 1});
    // The map of the new insert is that of the block through the outer one.
    const Transform2D expected =
        insertTransform(document, outer) *
        insertTransform(document, insertOf("Leaf", {1, 1}, 1.0, 1.0, 90.0));
    for (const Vec2 point : {Vec2{0, 0}, Vec2{1, 0}, Vec2{0, 1}}) {
        expectPoint(insertTransform(document, insert).apply(point), expected.apply(point));
    }
}

TEST(ExplodeInsert, ABlockThatCannotBeExplodedIsRefused) {
    Document document = Document::newDrawing();
    BlockDefinition definition = block(document, "Fixed", {Line{{0, 0}, {1, 0}}});
    definition.explodable = false;
    document.replaceBlock("Fixed", definition);
    const Exploded exploded = explodeInsert(document, insertOf("Fixed", {0, 0}));
    EXPECT_EQ(exploded.refusal, "Block \"Fixed\" cannot be exploded.");
    EXPECT_TRUE(exploded.entities.empty());
}

TEST(ExplodeInsert, AnMTextUnderANonUniformScaleIsRefusedAndAnAttributeDefinitionStays) {
    Document document = Document::newDrawing();
    document.replaceBlock("Note", block(document, "Note", {mtextAt({0, 0})}));
    document.replaceBlock("Tag",
                          block(document, "Tag", {attributeDefinition("NAME", {1, 1}, false)}));
    const Exploded note = explodeInsert(document, insertOf("Note", {0, 0}, 2.0, 1.0));
    EXPECT_EQ(note.refusal, "Block \"Note\" contains objects Tecnigrafo cannot explode.");
    EXPECT_TRUE(note.entities.empty());

    const Exploded tag = explodeInsert(document, insertOf("Tag", {0, 0}, 2.0, 1.0));
    ASSERT_TRUE(tag.refusal.empty()) << tag.refusal;
    ASSERT_EQ(tag.entities.size(), 1U);
    const auto& definition = entityAt<AttributeDefinition>(tag, 0);
    EXPECT_EQ(definition.tag, "NAME");
    expectPoint(definition.text.position, {2, 1});
    EXPECT_NEAR(definition.text.widthFactor, 2.0, kEps);
}

TEST(ExplodeInsert, EntitiesAndUnhandledEntitiesKeepTheOrderOfTheDefinition) {
    Document document = Document::newDrawing();
    BlockDefinition definition{"Mixed"};
    definition.entities.push_back(member(document, Line{{0, 0}, {1, 0}}, 1));
    definition.entities.push_back(member(document, Line{{0, 1}, {1, 1}}, 3));
    definition.rawEntities.push_back(
        rawEntity(0x40, 2, "ACAD_PROXY_ENTITY", {{5, "40"}, {330, "1F"}, {100, "AcDbEntity"}}));
    document.replaceBlock("Mixed", definition);
    const Exploded exploded = explodeInsert(document, insertOf("Mixed", {5, 0}));
    ASSERT_TRUE(exploded.refusal.empty()) << exploded.refusal;
    ASSERT_EQ(exploded.entities.size(), 2U);
    ASSERT_EQ(exploded.raw.size(), 1U);
    // The order keys say Line A, the raw entity, Line B.
    EXPECT_LT(exploded.entities[0].order, exploded.raw[0].order);
    EXPECT_LT(exploded.raw[0].order, exploded.entities[1].order);
    expectPoint(entityAt<Line>(exploded, 0).start, {5, 0});
    expectPoint(entityAt<Line>(exploded, 1).start, {5, 1});
    EXPECT_EQ(exploded.raw[0].id, EntityId{});
    EXPECT_EQ(exploded.raw[0].groups, definition.rawEntities[0].groups);
}

TEST(ExplodeInsert, AnUnhandledEntityWithCoordinatesIsRefused) {
    Document document = Document::newDrawing();
    BlockDefinition definition = block(document, "Placed", {Line{{0, 0}, {1, 0}}});
    definition.rawEntities.push_back(
        rawEntity(0x41, 2, "POINT", {{5, "41"}, {100, "AcDbEntity"}, {10, "1.0"}, {20, "2.0"}}));
    document.replaceBlock("Placed", definition);
    const Exploded exploded = explodeInsert(document, insertOf("Placed", {0, 0}));
    EXPECT_EQ(exploded.refusal, "Block \"Placed\" contains objects Tecnigrafo cannot explode.");
    EXPECT_TRUE(exploded.entities.empty());
}

TEST(ExplodeInsert, AnMTextWinsOverANestedBlockInBothOrders) {
    Document document = Document::newDrawing();
    document.replaceBlock("B", block(document, "B", {Line{{0, 0}, {1, 0}}}));
    const Entity nested = insertOf("B", {0, 0}, 1.0, 1.0, 30.0);
    const Entity note = mtextAt({0, 0});
    for (const auto& entities :
         {std::vector<Entity>{note, nested}, std::vector<Entity>{nested, note}}) {
        document.replaceBlock("A", block(document, "A", entities));
        const Exploded exploded = explodeInsert(document, insertOf("A", {0, 0}, 2.0, 1.0));
        EXPECT_EQ(exploded.refusal, "Block \"A\" contains objects Tecnigrafo cannot explode.");
        EXPECT_TRUE(exploded.entities.empty());
    }
}

TEST(ExplodeDimension, ALinearDimensionGivesItsGraphicsAsTheyAre) {
    const Document document = Document::newDrawing();
    Dimension dimension;
    dimension.definition.kind = DimensionKind::Linear;
    dimension.definition.point13 = {0, 0};
    dimension.definition.point14 = {40, 0};
    dimension.definition.linePoint = {20, 10};
    std::string reason;
    const std::optional<Dimension> made = regenerated(document, dimension, &reason);
    ASSERT_TRUE(made.has_value()) << reason;
    Dimension shown = made.value_or(Dimension{});
    for (std::size_t index = 0; index < shown.graphics.size(); ++index) {
        shown.graphics[index].id = EntityId{0x100 + index};
    }
    const Exploded exploded = explodeDimension(document, shown);
    ASSERT_TRUE(exploded.refusal.empty()) << exploded.refusal;
    EXPECT_TRUE(exploded.raw.empty());
    // Three lines, two arrows and the text, as AutoCAD (dxf-di-prova.md).
    ASSERT_EQ(exploded.entities.size(), 6U);
    for (std::size_t index = 0; index < exploded.entities.size(); ++index) {
        EXPECT_EQ(exploded.entities[index].id, EntityId{});
        EXPECT_EQ(exploded.entities[index].entity, shown.graphics[index].entity);
        EXPECT_EQ(exploded.entities[index].properties, shown.graphics[index].properties);
    }
    EXPECT_TRUE(std::holds_alternative<Line>(exploded.entities[0].entity));
    EXPECT_TRUE(std::holds_alternative<Solid>(exploded.entities[3].entity));
    const auto& text = entityAt<MText>(exploded, 5);
    EXPECT_EQ(text.value, "40");
    expectPoint(text.insertion, {20, 10.625});
}

TEST(ExplodeDimension, AnUnhandledPointInTheGraphicsIsRefused) {
    const Document document = Document::newDrawing();
    Dimension dimension;
    dimension.graphics.push_back(member(document, Line{{0, 0}, {1, 0}}));
    dimension.rawGraphics.push_back(
        rawEntity(0x42, 2, "POINT", {{5, "42"}, {100, "AcDbEntity"}, {10, "0.0"}, {20, "0.0"}}));
    const Exploded exploded = explodeDimension(document, dimension);
    EXPECT_EQ(exploded.refusal, "Dimension contains objects Tecnigrafo cannot explode.");
    EXPECT_TRUE(exploded.entities.empty());
}

}  // namespace
}  // namespace tg::core
