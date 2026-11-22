// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/entity_properties.hpp"

#include "tg/core/document.hpp"
#include "tg/core/math.hpp"

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
        EXPECT_EQ(std::get<Solid>(*change.entity), expected) << names.at(i);
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
    EXPECT_EQ(std::get<Solid>(*change.entity), expected);
    // Moved apart from the third, the fourth vertex is written again.
    const PropertyChange apart =
        withOwnProperty(document, recordOf(triangle), "vertex4", Vec2{5, 5});
    ASSERT_TRUE(apart.entity);
    EXPECT_TRUE(std::get<Solid>(*apart.entity).fourthVertexPresent);
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

}  // namespace
}  // namespace tg::core
