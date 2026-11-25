// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/entity_properties.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/block_table.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/raw_drawing.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

/// The tolerance of the values of the session of 2026-11-22, which have up
/// to 16 significant digits.
constexpr double kTolerance = 1e-9;

/// The DXF names in the order of the alternatives of Entity.
constexpr std::array<std::string_view, 12> kTypeNames{"LWPOLYLINE", "LINE",   "CIRCLE",    "ARC",
                                                      "TEXT",       "SOLID",  "ELLIPSE",   "MTEXT",
                                                      "ATTDEF",     "INSERT", "DIMENSION", "HATCH"};
static_assert(kTypeNames.size() == std::variant_size_v<Entity>,
              "a name for every alternative of core::Entity");

template <std::size_t... Index>
std::vector<std::string_view> namesOfEveryAlternative(std::index_sequence<Index...> /*indices*/) {
    return {entityTypeName(Entity{std::in_place_index<Index>})...};
}

TEST(EntityTypeName, GivesTheDxfNameOfEveryAlternative) {
    const std::vector<std::string_view> names =
        namesOfEveryAlternative(std::make_index_sequence<std::variant_size_v<Entity>>{});
    ASSERT_EQ(names.size(), kTypeNames.size());
    for (std::size_t i = 0; i < names.size(); ++i) {
        EXPECT_EQ(names.at(i), kTypeNames.at(i)) << "alternative " << i;
    }
}

/// The ellipse with centre `center`, half axes 20 and 10 along X and Y.
Ellipse ellipse20By10(const Vec2& center = {10, 10}) {
    return Ellipse{center, {20, 0}, 0.5, 0.0, kTwoPi};
}

/// The arc of ellipse20By10() between the parameters of the true angles 45°
/// and 135° (the A7 of autocad-proprieta, session of 2026-11-22).
Ellipse arc45To135() {
    return Ellipse{{210, -40},
                   {20, 0},
                   0.5,
                   degreesToRadians(63.43494882292198),
                   degreesToRadians(116.56505117707795)};
}

EntityRecord recordOf(Entity entity) {
    return EntityRecord{EntityId{300}, std::move(entity)};
}

/// The value of property `name`: a number in degrees for an angle.
std::optional<PropertyValue> valueOf(const std::vector<OwnProperty>& properties,
                                     std::string_view name) {
    for (const OwnProperty& property : properties) {
        if (property.name == name) {
            if (property.kind == PropertyKind::Angle) {
                return PropertyValue{radiansToDegrees(std::get<double>(property.value))};
            }
            return property.value;
        }
    }
    return std::nullopt;
}

double numberOf(const std::vector<OwnProperty>& properties, std::string_view name) {
    const auto value = valueOf(properties, name);
    if (!value || !std::holds_alternative<double>(*value)) {
        ADD_FAILURE() << "no number named " << name;
        return std::numeric_limits<double>::quiet_NaN();
    }
    return std::get<double>(*value);
}

/// The ellipse after setting `name` to `value`; a refusal fails the test.
Ellipse changedEllipse(const Ellipse& ellipse, std::string_view name, const PropertyValue& value) {
    const Document document = Document::newDrawing();
    const PropertyChange change = withOwnProperty(document, recordOf(ellipse), name, value);
    EXPECT_TRUE(change.refusal.empty()) << change.refusal;
    EXPECT_FALSE(change.extras);
    if (!change.entity || !std::holds_alternative<Ellipse>(*change.entity)) {
        ADD_FAILURE() << "no ellipse after " << name;
        return ellipse;
    }
    return std::get<Ellipse>(*change.entity);
}

/// The solid of a change; an empty or different entity fails the test.
Solid solidOf(const PropertyChange& change) {
    if (!change.entity || !std::holds_alternative<Solid>(*change.entity)) {
        ADD_FAILURE() << "no solid in the change";
        return Solid{};
    }
    return std::get<Solid>(*change.entity);
}

std::vector<OwnProperty> propertiesOf(const Entity& entity) {
    const Document document = Document::newDrawing();
    return ownProperties(document, recordOf(entity));
}

TEST(EllipseProperties, ListsTheNamesOfTheCatalogueInOrder) {
    const std::vector<OwnProperty> properties = propertiesOf(ellipse20By10());
    std::vector<std::string> names;
    for (const OwnProperty& property : properties) {
        names.push_back(property.name);
        EXPECT_EQ(property.readOnly, property.name == "~area") << property.name;
    }
    EXPECT_EQ(names, (std::vector<std::string>{"center", "major-radius", "minor-radius", "ratio",
                                               "start-angle", "end-angle", "~area"}));
    EXPECT_EQ(std::get<Vec2>(properties.front().value), (Vec2{10, 10}));
    EXPECT_EQ(properties.at(4).kind, PropertyKind::Angle);
    EXPECT_DOUBLE_EQ(numberOf(properties, "major-radius"), 20);
    EXPECT_DOUBLE_EQ(numberOf(properties, "minor-radius"), 10);
    EXPECT_DOUBLE_EQ(numberOf(properties, "ratio"), 0.5);
    EXPECT_DOUBLE_EQ(numberOf(properties, "start-angle"), 0);
    EXPECT_DOUBLE_EQ(numberOf(properties, "end-angle"), 360);
    EXPECT_NEAR(numberOf(properties, "~area"), kPi * 200, kTolerance);
}

// The five ellipses of autocad-proprieta (session of 2026-11-22).

TEST(EllipseProperties, MajorRadiusFrom20To30KeepsTheMinorOne) {
    const Ellipse changed = changedEllipse(ellipse20By10(), "major-radius", 30.0);
    EXPECT_EQ(changed.majorAxis, (Vec2{30, 0}));
    EXPECT_NEAR(changed.ratio, 0.3333333333333333, kTolerance);
    EXPECT_EQ(changed.center, (Vec2{10, 10}));
    EXPECT_NEAR(numberOf(propertiesOf(changed), "minor-radius"), 10, kTolerance);
    // pi * 30 * 10 (PT22, point 2).
    EXPECT_NEAR(numberOf(propertiesOf(changed), "~area"), 942.4777960769379, kTolerance);
}

TEST(EllipseProperties, MinorRadiusFrom10To5KeepsTheMajorOne) {
    const Ellipse changed = changedEllipse(ellipse20By10(), "minor-radius", 5.0);
    EXPECT_EQ(changed.majorAxis, (Vec2{20, 0}));
    EXPECT_DOUBLE_EQ(changed.ratio, 0.25);
    EXPECT_NEAR(numberOf(propertiesOf(changed), "~area"), 314.1592653589793, kTolerance);
}

TEST(EllipseProperties, UnchangedEllipseOfTheSession) {
    const std::vector<OwnProperty> properties = propertiesOf(ellipse20By10({160, -40}));
    EXPECT_EQ(std::get<Vec2>(properties.front().value), (Vec2{160, -40}));
    EXPECT_NEAR(numberOf(properties, "~area"), 628.3185307179587, kTolerance);
}

TEST(EllipseProperties, TrueAnglesAndSegmentAreaOfAnArc) {
    const std::vector<OwnProperty> properties = propertiesOf(arc45To135());
    EXPECT_NEAR(numberOf(properties, "start-angle"), 45, kTolerance);
    EXPECT_NEAR(numberOf(properties, "end-angle"), 135, kTolerance);
    EXPECT_NEAR(numberOf(properties, "~area"), 12.7295218001612, kTolerance);
}

TEST(EllipseProperties, StartAngleChangesOnlyTheStartParameter) {
    const Ellipse arc = arc45To135();
    const Ellipse changed = changedEllipse(arc, "start-angle", degreesToRadians(30));
    EXPECT_NEAR(radiansToDegrees(changed.startParameter), 49.1066053508691, kTolerance);
    EXPECT_EQ(changed.endParameter, arc.endParameter);
    const std::vector<OwnProperty> properties = propertiesOf(changed);
    EXPECT_NEAR(numberOf(properties, "start-angle"), 30, kTolerance);
    EXPECT_NEAR(numberOf(properties, "end-angle"), 135, kTolerance);
    EXPECT_NEAR(numberOf(properties, "~area"), 25.3770242285045, kTolerance);
}

TEST(EllipseProperties, EndAngleChangesOnlyTheEndParameter) {
    const Ellipse arc = arc45To135();
    const Ellipse changed = changedEllipse(arc, "end-angle", degreesToRadians(150));
    EXPECT_EQ(changed.startParameter, arc.startParameter);
    EXPECT_NEAR(radiansToDegrees(changed.endParameter), 130.89339464913093, kTolerance);
    // The angle of the end of a whole ellipse leaves it whole.
    const Ellipse whole = changedEllipse(ellipse20By10(), "end-angle", 0.0);
    EXPECT_EQ(whole, ellipse20By10());
}

TEST(EllipseProperties, MajorRadiusBelowTheMinorOneSwapsTheAxes) {
    const Ellipse changed = changedEllipse(ellipse20By10(), "major-radius", 5.0);
    EXPECT_EQ(changed.majorAxis, (Vec2{0, 10}));
    EXPECT_DOUBLE_EQ(changed.ratio, 0.5);
    EXPECT_EQ(changed.startParameter, 0.0);
    EXPECT_EQ(changed.endParameter, kTwoPi);
    const Ellipse longer = changedEllipse(ellipse20By10(), "minor-radius", 25.0);
    EXPECT_EQ(longer.majorAxis, (Vec2{0, 25}));
    EXPECT_DOUBLE_EQ(longer.ratio, 0.8);
}

TEST(EllipseProperties, SwappedAxesKeepThePointsOfAnArc) {
    const Ellipse arc{{0, 0}, {20, 0}, 0.5, 0.0, kPi / 2.0};
    const Ellipse changed = changedEllipse(arc, "major-radius", 5.0);
    EXPECT_EQ(changed.majorAxis, (Vec2{0, 10}));
    EXPECT_NEAR(changed.startParameter, 3.0 * kPi / 2.0, kTolerance);
    EXPECT_NEAR(changed.endParameter, 0.0, kTolerance);
    const Vec2 start = pointAt(changed, changed.startParameter);
    const Vec2 end = pointAt(changed, changed.endParameter);
    EXPECT_NEAR(start.x, 5, kTolerance);
    EXPECT_NEAR(start.y, 0, kTolerance);
    EXPECT_NEAR(end.x, 0, kTolerance);
    EXPECT_NEAR(end.y, 10, kTolerance);
}

TEST(EllipseProperties, RatioKeepsTheMajorAxisAndRatioOneIsACircleAlongX) {
    EXPECT_DOUBLE_EQ(changedEllipse(ellipse20By10(), "ratio", 0.75).ratio, 0.75);
    const Ellipse tilted{{0, 0}, {0, 20}, 0.5, 0.0, kTwoPi};
    const Ellipse circle = changedEllipse(tilted, "ratio", 1.0);
    EXPECT_EQ(circle.majorAxis, (Vec2{20, 0}));
    EXPECT_EQ(circle.ratio, 1.0);
}

TEST(EllipseProperties, CenterMovesTheEllipse) {
    const Ellipse changed = changedEllipse(ellipse20By10(), "center", Vec2{15, -5});
    Ellipse expected = ellipse20By10();
    expected.center = {15, -5};
    EXPECT_EQ(changed, expected);
}

TEST(EllipseProperties, InvalidValuesAreRefusedWithoutAnEntity) {
    const Document document = Document::newDrawing();
    const EntityRecord record = recordOf(ellipse20By10());
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double infinity = std::numeric_limits<double>::infinity();
    const std::vector<std::pair<std::string_view, PropertyValue>> refused{
        {"ratio", 0.0},
        {"ratio", -0.25},
        {"ratio", 1.01},
        {"ratio", nan},
        {"major-radius", 0.0},
        {"minor-radius", -1.0},
        {"major-radius", infinity},
        {"start-angle", nan},
        {"center", Vec2{infinity, 0}},
        {"center", 3.0},
        {"major-radius", Vec2{1, 1}},
    };
    for (const auto& [name, value] : refused) {
        const PropertyChange change = withOwnProperty(document, record, name, value);
        EXPECT_FALSE(change.entity) << name;
        EXPECT_EQ(change.refusal, kInvalidPropertyValue) << name;
    }
}

TEST(EllipseProperties, UnknownAndReadOnlyNamesAreRefused) {
    const Document document = Document::newDrawing();
    const EntityRecord record = recordOf(ellipse20By10());
    const PropertyChange unknown = withOwnProperty(document, record, "width", 1.0);
    EXPECT_FALSE(unknown.entity);
    EXPECT_EQ(unknown.refusal, kUnknownProperty);
    const PropertyChange area = withOwnProperty(document, record, "~area", 1.0);
    EXPECT_FALSE(area.entity);
    EXPECT_EQ(area.refusal, kReadOnlyProperty);
}

TEST(SolidProperties, EachOfTheFourVerticesMovesAlone) {
    const Solid solid{{Vec2{0, 0}, Vec2{10, 0}, Vec2{0, 10}, Vec2{10, 10}}};
    const Document document = Document::newDrawing();
    const EntityRecord record = recordOf(solid);
    const std::vector<OwnProperty> properties = ownProperties(document, record);
    ASSERT_EQ(properties.size(), 4U);
    const std::array<std::string_view, 4> names{"vertex1", "vertex2", "vertex3", "vertex4"};
    for (std::size_t i = 0; i < names.size(); ++i) {
        EXPECT_EQ(properties.at(i).name, names.at(i));
        EXPECT_EQ(properties.at(i).kind, PropertyKind::Point);
        EXPECT_EQ(std::get<Vec2>(properties.at(i).value), solid.vertices.at(i));
        const Vec2 target{20.0 + static_cast<double>(i), 30};
        const PropertyChange change = withOwnProperty(document, record, names.at(i), target);
        ASSERT_TRUE(change.entity) << names.at(i);
        Solid expected = solid;
        expected.vertices.at(i) = target;
        EXPECT_EQ(solidOf(change), expected) << names.at(i);
    }
    const PropertyChange refused = withOwnProperty(
        document, record, "vertex2", Vec2{std::numeric_limits<double>::quiet_NaN(), 0});
    EXPECT_FALSE(refused.entity);
    EXPECT_EQ(refused.refusal, kInvalidPropertyValue);
}

TEST(SolidProperties, AFourthVertexEqualToTheThirdStaysATriangle) {
    const Solid triangle{{Vec2{0, 0}, Vec2{10, 0}, Vec2{0, 10}, Vec2{0, 10}}, false};
    const Document document = Document::newDrawing();
    const PropertyChange change =
        withOwnProperty(document, recordOf(triangle), "vertex1", Vec2{-2, 1});
    ASSERT_TRUE(change.entity);
    const Solid expected{{Vec2{-2, 1}, Vec2{10, 0}, Vec2{0, 10}, Vec2{0, 10}}, false};
    EXPECT_EQ(solidOf(change), expected);
    // Moved apart from the third, the fourth vertex is written again.
    const PropertyChange apart =
        withOwnProperty(document, recordOf(triangle), "vertex4", Vec2{5, 5});
    ASSERT_TRUE(apart.entity);
    EXPECT_TRUE(solidOf(apart).fourthVertexPresent);
}

TEST(OwnProperties, TypesOfBeforeThe08HaveNone) {
    EXPECT_TRUE(propertiesOf(Line{{0, 0}, {10, 0}}).empty());
    EXPECT_TRUE(propertiesOf(Circle{{0, 0}, 5}).empty());
    const Document document = Document::newDrawing();
    const PropertyChange change =
        withOwnProperty(document, recordOf(Line{{0, 0}, {10, 0}}), "start", Vec2{1, 1});
    EXPECT_FALSE(change.entity);
    EXPECT_EQ(change.refusal, kUnknownProperty);
}

// MTEXT, ATTDEF and INSERT (docs/tappe/0.8.1/PT24-proprieta-testi-inserimenti.md).

std::vector<std::string> namesOf(const std::vector<OwnProperty>& properties) {
    std::vector<std::string> names;
    names.reserve(properties.size());
    for (const OwnProperty& property : properties) {
        names.push_back(property.name);
        EXPECT_FALSE(property.readOnly) << property.name;
    }
    return names;
}

/// The entity of type T after setting `name` to `value` in `document`; a
/// refusal fails the test.
template <typename T>
T changedOf(const Document& document, const EntityRecord& record, std::string_view name,
            const PropertyValue& value) {
    const PropertyChange change = withOwnProperty(document, record, name, value);
    EXPECT_TRUE(change.refusal.empty()) << name << ": " << change.refusal;
    if (!change.entity || !std::holds_alternative<T>(*change.entity)) {
        ADD_FAILURE() << "no entity of the same type after " << name;
        return std::get<T>(record.entity);
    }
    return std::get<T>(*change.entity);
}

void expectRefused(const Document& document, const EntityRecord& record, std::string_view name,
                   const PropertyValue& value) {
    const PropertyChange change = withOwnProperty(document, record, name, value);
    EXPECT_FALSE(change.entity) << name;
    EXPECT_FALSE(change.extras) << name;
    EXPECT_EQ(change.refusal, kInvalidPropertyValue) << name;
}

MText mtextOf() {
    MText mtext;
    mtext.insertion = {0, 0};
    mtext.height = 2.5;
    mtext.width = 60;
    mtext.value = "uno\\Pdue";
    return mtext;
}

TEST(MTextProperties, ListsAndChangesEachProperty) {
    const Document document = Document::newDrawing();
    const EntityRecord record = recordOf(mtextOf());
    const std::vector<OwnProperty> properties = ownProperties(document, record);
    EXPECT_EQ(namesOf(properties),
              (std::vector<std::string>{"value", "style", "attachment", "width", "height",
                                        "rotation", "line-spacing", "line-spacing-style"}));
    EXPECT_EQ(std::get<std::string>(valueOf(properties, "attachment").value_or(PropertyValue{})),
              "top-left");
    EXPECT_EQ(properties.at(2).choices.size(), 9U);
    EXPECT_EQ(
        std::get<std::string>(valueOf(properties, "line-spacing-style").value_or(PropertyValue{})),
        "at-least");

    EXPECT_EQ(changedOf<MText>(document, record, "value", std::string{"tre"}).value, "tre");
    EXPECT_EQ(changedOf<MText>(document, record, "style", std::string{"standard"}).style,
              "Standard");
    EXPECT_EQ(
        changedOf<MText>(document, record, "attachment", std::string{"bottom-right"}).attachment,
        MTextAttachment::BottomRight);
    EXPECT_EQ(changedOf<MText>(document, record, "width", 0.0).width, 0.0);
    EXPECT_EQ(changedOf<MText>(document, record, "height", 5.0).height, 5.0);
    EXPECT_EQ(changedOf<MText>(document, record, "rotation", kPi / 2).rotation, kPi / 2);
    EXPECT_EQ(changedOf<MText>(document, record, "line-spacing", 1.5).lineSpacing, 1.5);
    EXPECT_TRUE(changedOf<MText>(document, record, "line-spacing-style", std::string{"exactly"})
                    .exactSpacing);
}

TEST(MTextProperties, InvalidValuesAreRefused) {
    const Document document = Document::newDrawing();
    const EntityRecord record = recordOf(mtextOf());
    expectRefused(document, record, "value", 1.0);
    expectRefused(document, record, "style", std::string{"NONESISTE"});
    expectRefused(document, record, "attachment", std::string{"centre"});
    expectRefused(document, record, "width", -1.0);
    expectRefused(document, record, "height", 0.0);
    expectRefused(document, record, "rotation", std::numeric_limits<double>::infinity());
    expectRefused(document, record, "line-spacing", 0.0);
    expectRefused(document, record, "line-spacing", -1.0);
    expectRefused(document, record, "line-spacing-style", std::string{"double"});
}

AttributeDefinition attdefOf() {
    AttributeDefinition definition;
    definition.text.value = "K1";
    definition.tag = "CODICE";
    definition.prompt = "Codice";
    return definition;
}

TEST(AttdefProperties, ListsAndChangesEachProperty) {
    const Document document = Document::newDrawing();
    const EntityRecord record = recordOf(attdefOf());
    EXPECT_EQ(
        namesOf(ownProperties(document, record)),
        (std::vector<std::string>{"tag", "prompt", "default", "invisible", "constant", "verify",
                                  "preset", "lock-position", "height", "rotation", "style"}));
    using Definition = AttributeDefinition;
    EXPECT_EQ(changedOf<Definition>(document, record, "tag", std::string{"NEW"}).tag, "NEW");
    EXPECT_EQ(changedOf<Definition>(document, record, "prompt", std::string{"P"}).prompt, "P");
    EXPECT_EQ(changedOf<Definition>(document, record, "default", std::string{"X"}).text.value, "X");
    EXPECT_TRUE(changedOf<Definition>(document, record, "invisible", true).flags.invisible);
    EXPECT_TRUE(changedOf<Definition>(document, record, "constant", true).flags.constant);
    EXPECT_TRUE(changedOf<Definition>(document, record, "verify", true).flags.verify);
    EXPECT_TRUE(changedOf<Definition>(document, record, "preset", true).flags.preset);
    EXPECT_TRUE(changedOf<Definition>(document, record, "lock-position", true).lockPosition);
    EXPECT_EQ(changedOf<Definition>(document, record, "height", 4.0).text.height, 4.0);
    EXPECT_EQ(changedOf<Definition>(document, record, "rotation", kPi).text.rotation, kPi);
    EXPECT_EQ(changedOf<Definition>(document, record, "style", std::string{"STANDARD"}).text.style,
              "Standard");
}

TEST(AttdefProperties, InvalidValuesAreRefused) {
    const Document document = Document::newDrawing();
    const EntityRecord record = recordOf(attdefOf());
    expectRefused(document, record, "tag", std::string{"CODICE 2"});
    expectRefused(document, record, "tag", std::string{});
    expectRefused(document, record, "prompt", true);
    expectRefused(document, record, "default", 2.0);
    expectRefused(document, record, "invisible", std::string{"yes"});
    expectRefused(document, record, "constant", std::string{"yes"});
    expectRefused(document, record, "verify", 1.0);
    expectRefused(document, record, "preset", std::string{});
    expectRefused(document, record, "lock-position", 1.0);
    expectRefused(document, record, "height", -2.5);
    expectRefused(document, record, "rotation", std::numeric_limits<double>::quiet_NaN());
    expectRefused(document, record, "style", std::string{"NONESISTE"});
}

Attribute attributeOf(std::uint64_t id, std::string tag, std::string value) {
    Attribute attribute;
    attribute.id = EntityId{id};
    attribute.tag = std::move(tag);
    attribute.text.value = std::move(value);
    return attribute;
}

/// A drawing with the blocks B1 (a line) and B2 (a line).
Document twoBlocks() {
    Document document = Document::newDrawing();
    for (const auto* name : {"B1", "B2"}) {
        BlockDefinition definition{name};
        definition.entities.push_back({EntityId{0}, Line{{0, 0}, {10, 0}}});
        document.replaceBlock(name, definition);
    }
    return document;
}

Insert insertOf() {
    Insert insert;
    insert.block = "B1";
    insert.insertion = {10, 10};
    insert.scaleX = 2;
    insert.scaleY = 3;
    insert.rotation = kPi / 6;
    insert.attributes = {attributeOf(401, "UNO", "1"), attributeOf(402, "DUE", "2")};
    return insert;
}

TEST(InsertProperties, ListsTheAttributesInTheOrderOfTheInsert) {
    const Document document = twoBlocks();
    EntityRecord record = recordOf(insertOf());
    const std::vector<OwnProperty> properties = ownProperties(document, record);
    EXPECT_EQ(namesOf(properties),
              (std::vector<std::string>{"block", "insertion", "scale", "scale-z", "rotation",
                                        "attribute", "attribute"}));
    EXPECT_EQ(std::get<Vec2>(valueOf(properties, "scale").value_or(PropertyValue{})), (Vec2{2, 3}));
    EXPECT_EQ(numberOf(properties, "scale-z"), 1.0);
    EXPECT_EQ(std::get<std::string>(properties.at(5).value), R"("UNO":"1")");
    EXPECT_EQ(std::get<std::string>(properties.at(6).value), R"("DUE":"2")");
    record.extras.specific = {{100, "AcDbBlockReference"}, {43, "3"}};
    EXPECT_EQ(numberOf(ownProperties(document, record), "scale-z"), 3.0);
}

TEST(InsertProperties, ChangesEachPropertyAndOnlyTheValueOfAnAttribute) {
    const Document document = twoBlocks();
    const EntityRecord record = recordOf(insertOf());
    Insert expected = insertOf();
    expected.insertion = {1, 2};
    EXPECT_EQ(changedOf<Insert>(document, record, "insertion", Vec2{1, 2}), expected);
    expected = insertOf();
    expected.scaleX = -1;
    expected.scaleY = 4;
    EXPECT_EQ(changedOf<Insert>(document, record, "scale", Vec2{-1, 4}), expected);
    expected = insertOf();
    expected.rotation = kPi;
    EXPECT_EQ(changedOf<Insert>(document, record, "rotation", kPi), expected);
    expected = insertOf();
    expected.attributes[1].text.value = "nuovo \"due\"";
    EXPECT_EQ(
        changedOf<Insert>(document, record, "attribute", std::string{R"("due":"nuovo \"due\"")"}),
        expected);
}

TEST(InsertProperties, AnotherBlockKeepsPointScalesRotationAndAttributes) {
    const Document document = twoBlocks();
    const EntityRecord record = recordOf(insertOf());
    Insert expected = insertOf();
    expected.block = "B2";
    EXPECT_EQ(changedOf<Insert>(document, record, "block", std::string{"b2"}), expected);
    expectRefused(document, record, "block", std::string{"NONESISTE"});
}

TEST(InsertProperties, ABlockThatWouldContainItselfIsRefused) {
    Document document = twoBlocks();
    BlockDefinition outer{"OUTER"};
    Insert inner;
    inner.block = "B1";
    outer.entities.push_back({EntityId{500}, inner});
    document.replaceBlock("OUTER", outer);
    const EntityRecord record{EntityId{500}, inner};
    expectRefused(document, record, "block", std::string{"OUTER"});
    EXPECT_EQ(changedOf<Insert>(document, record, "block", std::string{"B2"}).block, "B2");
}

TEST(InsertProperties, InvalidValuesAreRefused) {
    const Document document = twoBlocks();
    const EntityRecord record = recordOf(insertOf());
    expectRefused(document, record, "insertion", Vec2{std::numeric_limits<double>::infinity(), 0});
    expectRefused(document, record, "scale", Vec2{0, 1});
    expectRefused(document, record, "scale", Vec2{1, 0});
    expectRefused(document, record, "scale-z", 0.0);
    expectRefused(document, record, "scale-z", std::numeric_limits<double>::quiet_NaN());
    expectRefused(document, record, "rotation", std::string{"30"});
    expectRefused(document, record, "attribute", std::string{R"("TRE":"3")"});
    expectRefused(document, record, "attribute", std::string{"UNO=1"});
    expectRefused(document, record, "block", 1.0);
}

TEST(InsertProperties, ScaleZChangesOnlyTheGroup43OfTheExtras) {
    const Document document = twoBlocks();
    EntityRecord record = recordOf(insertOf());
    const RawGroups before{{100, "AcDbBlockReference"},
                           {2, "B1"},
                           {41, "2"},
                           {42, "3"},
                           {43, "0"},
                           {50, "30"},
                           {1001, "APP"},
                           {1000, "kept"}};
    record.extras.specific = before;
    const PropertyChange change = withOwnProperty(document, record, "scale-z", 2.0);
    ASSERT_TRUE(change.entity);
    EXPECT_EQ(std::get<Insert>(change.entity.value_or(Entity{})), insertOf());
    ASSERT_TRUE(change.extras);
    RawGroups expected = before;
    expected[4].value = "2";
    EXPECT_EQ(change.extras.value_or(EntityExtras{}).specific, expected);

    // Read with the 43 at 0, the insert changes its rotation and keeps it.
    const PropertyChange rotated = withOwnProperty(document, record, "rotation", kPi);
    ASSERT_TRUE(rotated.entity);
    EXPECT_FALSE(rotated.extras);
    EXPECT_EQ(numberOf(ownProperties(document, record), "scale-z"), 0.0);

    // A missing 43 goes after the 42.
    record.extras.specific = {{100, "AcDbBlockReference"}, {41, "2"}, {42, "3"}, {50, "30"}};
    const PropertyChange added = withOwnProperty(document, record, "scale-z", -1.5);
    ASSERT_TRUE(added.extras);
    EXPECT_EQ(
        added.extras.value_or(EntityExtras{}).specific,
        (RawGroups{{100, "AcDbBlockReference"}, {41, "2"}, {42, "3"}, {43, "-1.5"}, {50, "30"}}));
}

TEST(InsertProperties, OfTwoTagsEqualButForCaseTheFirstIsShownAndChanged) {
    const Document document = twoBlocks();
    Insert insert = insertOf();
    insert.attributes = {attributeOf(401, "ID", "uno"), attributeOf(402, "id", "due")};
    const EntityRecord record = recordOf(insert);
    const std::vector<OwnProperty> properties = ownProperties(document, record);
    ASSERT_EQ(properties.size(), 6U);
    EXPECT_EQ(std::get<std::string>(properties.back().value), R"("ID":"uno")");
    const auto changed =
        changedOf<Insert>(document, record, "attribute", std::string{R"("id":"tre")"});
    EXPECT_EQ(changed.attributes[0].text.value, "tre");
    EXPECT_EQ(changed.attributes[1], insert.attributes[1]);
}

// --- HATCH and DIMENSION (docs/tappe/0.8.1/PT23-proprieta-retini-quote.md) --

/// The hatch CC of autocad-hatchedit: ANSI31 on a 60 x 40 rectangle with an
/// island of radius 8 flagged outermost, its pattern at the origin 245,5.
EntityRecord hatchCc() {
    Hatch hatch;
    hatch.pattern = "ANSI31";
    HatchLoop outer;
    outer.flags = 7;
    outer.polyline = Polyline{{{{300, 40}}, {{240, 40}}, {{240, 0}}, {{300, 0}}}, true};
    HatchLoop island;
    island.flags = 22;
    island.polyline = Polyline{{{{262, 20}, -1.0}, {{278, 20}, -1.0}}, true};
    hatch.loops = {outer, island};
    hatch.seeds = {{245, 5}, {245, 5}};
    constexpr double kOffset = 2.245064030267288;
    hatch.patternLines = {PatternLine{degreesToRadians(45), {245, 5}, {-kOffset, kOffset}, {}}};
    EntityRecord record = recordOf(std::move(hatch));
    record.extras.extended = {{1001, "GradientColor1ACI"},
                              {1070, "     5"},
                              {1001, "GradientColor2ACI"},
                              {1070, "     2"},
                              {1001, "ACAD"},
                              {1010, "245.0"},
                              {1020, "5.0"},
                              {1030, "0.0"}};
    return record;
}

/// The hatch of a change; an empty or different entity fails the test.
Hatch hatchOf(const PropertyChange& change) {
    EXPECT_TRUE(change.refusal.empty()) << change.refusal;
    if (!change.entity || !std::holds_alternative<Hatch>(*change.entity)) {
        ADD_FAILURE() << "no hatch in the change";
        return Hatch{};
    }
    return std::get<Hatch>(*change.entity);
}

/// A document whose header gives $MEASUREMENT `measurement`.
Document drawingMeasuring(int measurement) {
    Document document = Document::newDrawing();
    RawDrawing raw = document.raw();
    raw.sections.push_back(
        RawSection{"HEADER", {{9, "$MEASUREMENT"}, {70, "     " + std::to_string(measurement)}}});
    document.setRaw(std::move(raw));
    return document;
}

TEST(HatchProperties, ListsTheNamesOfTheCatalogueWithTheOriginAndTheArea) {
    const Document document = Document::newDrawing();
    const std::vector<OwnProperty> properties = ownProperties(document, hatchCc());
    std::vector<std::string> names;
    for (const OwnProperty& property : properties) {
        names.push_back(property.name);
        EXPECT_EQ(property.readOnly, property.name == "~area") << property.name;
    }
    EXPECT_EQ(names, (std::vector<std::string>{"pattern", "scale", "angle", "origin", "islands",
                                               "~area"}));
    EXPECT_EQ(std::get<std::string>(properties.at(0).value), "ANSI31");
    EXPECT_EQ(properties.at(0).kind, PropertyKind::Text);
    EXPECT_DOUBLE_EQ(numberOf(properties, "scale"), 1);
    EXPECT_DOUBLE_EQ(numberOf(properties, "angle"), 0);
    EXPECT_EQ(std::get<Vec2>(properties.at(3).value), (Vec2{245, 5}));
    EXPECT_EQ(std::get<std::string>(properties.at(4).value), "normal");
    EXPECT_EQ(properties.at(4).choices, (std::vector<std::string>{"normal", "outer", "ignore"}));
    EXPECT_NEAR(numberOf(properties, "~area"), 2198.938070170253, kTolerance);
}

TEST(HatchProperties, AHatchWithoutXdataHasTheOriginZero) {
    EntityRecord record = hatchCc();
    record.extras = {};
    const Document document = Document::newDrawing();
    EXPECT_EQ(valueOf(ownProperties(document, record), "origin"), (PropertyValue{Vec2{0, 0}}));
}

TEST(HatchProperties, OriginMovesTheFamiliesAndTheXdataTogether) {
    const Document document = Document::newDrawing();
    const EntityRecord record = hatchCc();
    const PropertyChange change = withOwnProperty(document, record, "origin", Vec2{0, 0});
    const Hatch hatch = hatchOf(change);
    EXPECT_EQ(hatch.patternLines.at(0).origin, (Vec2{0, 0}));
    EXPECT_TRUE(change.extras);
    const EntityExtras extras = change.extras.value_or(EntityExtras{});
    EXPECT_EQ(patternOrigin(extras), (Vec2{0, 0}));
    ASSERT_EQ(extras.extended.size(), record.extras.extended.size());
    EXPECT_EQ(extras.extended.at(5), (RawGroup{1010, "0.0"}));
    EXPECT_EQ(extras.extended.at(6), (RawGroup{1020, "0.0"}));

    const PropertyChange same = withOwnProperty(document, record, "origin", Vec2{245, 5});
    EXPECT_EQ(hatchOf(same), std::get<Hatch>(record.entity));
    EXPECT_FALSE(same.extras);
}

TEST(HatchProperties, ScaleAngleAndPatternKeepTheOrigin) {
    const Document document = Document::newDrawing();
    const EntityRecord record = hatchCc();
    const Hatch scaled = hatchOf(withOwnProperty(document, record, "scale", 2.0));
    EXPECT_EQ(scaled.scale, 2);
    EXPECT_EQ(scaled.patternLines.at(0).origin, (Vec2{245, 5}));
    EXPECT_NEAR(scaled.patternLines.at(0).offset.y, 4.490128060534576, kTolerance);

    const Hatch turned =
        hatchOf(withOwnProperty(document, record, "angle", reducedDegreesToRadians(30)));
    EXPECT_NEAR(radiansToDegrees(turned.patternLines.at(0).angle), 75, kTolerance);
    EXPECT_NEAR(turned.patternLines.at(0).offset.x, -3.066814498467791, kTolerance);
    EXPECT_NEAR(turned.patternLines.at(0).offset.y, 0.8217504682005037, kTolerance);

    const Hatch net = hatchOf(withOwnProperty(document, record, "pattern", std::string{"NET"}));
    EXPECT_EQ(net.pattern, "NET");
    ASSERT_EQ(net.patternLines.size(), 2U);
    EXPECT_EQ(net.patternLines.at(1).origin, (Vec2{245, 5}));
    EXPECT_NEAR(net.patternLines.at(0).offset.y, 3.175, kTolerance);

    const Hatch solid = hatchOf(withOwnProperty(document, record, "pattern", std::string{"SOLID"}));
    EXPECT_TRUE(solid.solid);
    EXPECT_TRUE(solid.patternLines.empty());
}

TEST(HatchProperties, NetTakesTheStepOfTheMeasurementOfTheDrawing) {
    const EntityRecord record = hatchCc();
    const Hatch metric =
        hatchOf(withOwnProperty(drawingMeasuring(1), record, "pattern", std::string{"NET"}));
    const Hatch inches =
        hatchOf(withOwnProperty(drawingMeasuring(0), record, "pattern", std::string{"NET"}));
    ASSERT_EQ(metric.patternLines.size(), 2U);
    ASSERT_EQ(inches.patternLines.size(), 2U);
    EXPECT_NEAR(metric.patternLines.at(0).offset.y, 3.175, kTolerance);
    EXPECT_NEAR(inches.patternLines.at(0).offset.y, 0.125, kTolerance);
}

TEST(HatchProperties, IslandsChangeTheStyleAndTheArea) {
    const Document document = Document::newDrawing();
    const PropertyChange change =
        withOwnProperty(document, hatchCc(), "islands", std::string{"ignore"});
    const Hatch hatch = hatchOf(change);
    EXPECT_EQ(hatch.islands, IslandStyle::Ignore);
    EntityRecord changed = hatchCc();
    changed.entity = hatch;
    EXPECT_NEAR(numberOf(ownProperties(document, changed), "~area"), 2400, kTolerance);
}

TEST(HatchProperties, InvalidValuesAreRefusedWithoutAnEntity) {
    const Document document = Document::newDrawing();
    const EntityRecord record = hatchCc();
    const std::vector<std::pair<std::string_view, PropertyValue>> refused{
        {"pattern", std::string{"ANSI37"}},
        {"pattern", std::string{"NONESISTE"}},
        {"pattern", 1.0},
        {"scale", 0.0},
        {"scale", -1.0},
        {"scale", std::numeric_limits<double>::infinity()},
        {"angle", std::numeric_limits<double>::quiet_NaN()},
        {"origin", Vec2{std::numeric_limits<double>::infinity(), 0}},
        {"islands", std::string{"outermost"}},
    };
    for (const auto& [name, value] : refused) {
        SCOPED_TRACE(name);
        const PropertyChange change = withOwnProperty(document, record, name, value);
        EXPECT_FALSE(change.entity);
        EXPECT_EQ(change.refusal, kInvalidPropertyValue);
    }
    const PropertyChange area = withOwnProperty(document, record, "~area", 1.0);
    EXPECT_EQ(area.refusal, kReadOnlyProperty);
}

TEST(HatchProperties, ScaleAndAngleOfAPatternThatIsNotGeneratedTurnItsFamilies) {
    const Document document = Document::newDrawing();
    EntityRecord record = hatchCc();
    auto& hatch = std::get<Hatch>(record.entity);
    hatch.pattern = "ANSI37";
    const Hatch turned =
        hatchOf(withOwnProperty(document, record, "angle", reducedDegreesToRadians(90)));
    EXPECT_EQ(turned.pattern, "ANSI37");
    EXPECT_EQ(turned.patternLines.at(0).origin, (Vec2{245, 5}));
    EXPECT_EQ(turned.patternLines.at(0).offset, (Vec2{-2.245064030267288, -2.245064030267288}));
}

/// A linear dimension of 40 as the case retini-quote defines it, made with
/// the tables of a new drawing.
Dimension linear40() {
    Dimension dimension;
    dimension.definition.point13 = {0, 0};
    dimension.definition.point14 = {40, 0};
    dimension.definition.linePoint = {40, 10};
    const std::optional<Dimension> made = regenerated(Document::newDrawing(), dimension);
    EXPECT_TRUE(made);
    return made.value_or(dimension);
}

/// The dimension of a change; an empty or different entity fails the test.
Dimension dimensionOf(const PropertyChange& change) {
    EXPECT_TRUE(change.refusal.empty()) << change.refusal;
    if (!change.entity || !std::holds_alternative<Dimension>(*change.entity)) {
        ADD_FAILURE() << "no dimension in the change";
        return Dimension{};
    }
    return std::get<Dimension>(*change.entity);
}

/// The values of the MTEXT of the graphics of `dimension`.
std::vector<std::string> graphicTexts(const Dimension& dimension) {
    std::vector<std::string> texts;
    for (const EntityRecord& graphic : dimension.graphics) {
        if (const auto* text = std::get_if<MText>(&graphic.entity)) {
            texts.push_back(text->value);
        }
    }
    return texts;
}

TEST(DimensionProperties, ListsTheNamesOfTheCatalogue) {
    const std::vector<OwnProperty> properties = propertiesOf(linear40());
    std::vector<std::string> names;
    for (const OwnProperty& property : properties) {
        names.push_back(property.name);
        EXPECT_EQ(property.readOnly, property.name == "~measurement") << property.name;
    }
    EXPECT_EQ(names, (std::vector<std::string>{"style", "text", "text-rotation", "~measurement"}));
    EXPECT_EQ(valueOf(properties, "style"), PropertyValue{std::string{"ISO-25"}});
    EXPECT_EQ(valueOf(properties, "text"), PropertyValue{std::string{}});
    EXPECT_DOUBLE_EQ(numberOf(properties, "text-rotation"), 0);
    EXPECT_DOUBLE_EQ(numberOf(properties, "~measurement"), 40);
}

TEST(DimensionProperties, TextWithTheMeasureRegeneratesTheGraphics) {
    const Document document = Document::newDrawing();
    const Dimension dimension =
        dimensionOf(withOwnProperty(document, recordOf(linear40()), "text", std::string{"<> mm"}));
    EXPECT_EQ(dimension.presentation.textOverride, "<> mm");
    EXPECT_EQ(dimension.presentation.textPoint, linear40().presentation.textPoint);
    EXPECT_EQ(graphicTexts(dimension), (std::vector<std::string>{"40 mm"}));
    const Dimension back =
        dimensionOf(withOwnProperty(document, recordOf(dimension), "text", std::string{}));
    EXPECT_EQ(graphicTexts(back), graphicTexts(linear40()));
}

TEST(DimensionProperties, StyleMustExistAndRegenerates) {
    const Document document = Document::newDrawing();
    const Dimension standard = dimensionOf(
        withOwnProperty(document, recordOf(linear40()), "style", std::string{"standard"}));
    EXPECT_EQ(standard.presentation.style, "Standard");
    EXPECT_NE(standard.graphics, linear40().graphics);
    const PropertyChange missing =
        withOwnProperty(document, recordOf(linear40()), "style", std::string{"NONE"});
    EXPECT_FALSE(missing.entity);
    EXPECT_EQ(missing.refusal, kInvalidPropertyValue);
}

TEST(DimensionProperties, AnExplicitTextAngleGoesInTheMiddleOfTheLine) {
    const Document document = Document::newDrawing();
    const Dimension turned =
        dimensionOf(withOwnProperty(document, recordOf(linear40()), "text-rotation", kPi / 2));
    EXPECT_EQ(turned.presentation.textRotation, std::optional<double>{kPi / 2});
    EXPECT_NEAR(turned.presentation.textPoint.x, 20, kTolerance);
    EXPECT_NEAR(turned.presentation.textPoint.y, 10, kTolerance);
    // 0 shown for the automatic angle: the same value leaves it automatic.
    const Dimension same =
        dimensionOf(withOwnProperty(document, recordOf(linear40()), "text-rotation", 0.0));
    EXPECT_EQ(same, linear40());
}

TEST(DimensionProperties, ADimensionThatCannotBeRegeneratedRefusesTheChange) {
    Dimension other = linear40();
    other.definition.kind = DimensionKind::Other;
    other.definition.dxfType = 6;
    other.presentation.textOverride = "A";
    const Document document = Document::newDrawing();
    const PropertyChange change =
        withOwnProperty(document, recordOf(other), "text", std::string{"B"});
    EXPECT_FALSE(change.entity);
    EXPECT_EQ(change.refusal, kDimensionNotRegenerable);
    for (const auto& [name, value] : std::vector<std::pair<std::string_view, PropertyValue>>{
             {"style", std::string{"Standard"}}, {"text-rotation", 1.0}}) {
        SCOPED_TRACE(name);
        EXPECT_EQ(withOwnProperty(document, recordOf(other), name, value).refusal,
                  kDimensionNotRegenerable);
    }
}

}  // namespace
}  // namespace tg::core
