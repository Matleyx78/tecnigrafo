// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document_query.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/block_table.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/proximity.hpp"
#include "tg/core/solid.hpp"
#include "tg/core/text.hpp"
#include "tg/core/text_metrics.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numbers>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

/// A two-point polyline from `from` to `to`, optionally an arc.
Polyline segment(const Vec2& from, const Vec2& to, double bulge = 0.0) {
    Polyline polyline;
    polyline.addVertex(from, bulge);
    polyline.addVertex(to);
    return polyline;
}

/// The closed axis-aligned rectangle with corners `a` and `b`.
Polyline rectangle(const Vec2& a, const Vec2& b) {
    return Polyline{{{a}, {{b.x, a.y}}, {b}, {{a.x, b.y}}}, true};
}

std::vector<std::uint64_t> values(const std::vector<EntityId>& ids) {
    std::vector<std::uint64_t> result;
    result.reserve(ids.size());
    for (const EntityId id : ids) {
        result.push_back(id.value);
    }
    return result;
}

std::optional<std::uint64_t> value(const std::optional<EntityId>& id) {
    return id ? std::optional<std::uint64_t>{id->value} : std::nullopt;
}

TEST(DocumentQuery, LayerStatesFilterAllQueriesAndKeepExtents) {
    Document document = Document::newDrawing();
    const auto id = document.add(Line{{0, 0}, {10, 0}});
    const BBox bounds = document.extents();
    for (const bool on : {false, true}) {
        for (const bool frozen : {false, true}) {
            for (const bool locked : {false, true}) {
                Layer layer = *document.layer(document.layerZero());
                layer.on = on;
                layer.frozen = frozen;
                layer.locked = locked;
                document.replaceLayer(layer);
                const bool visible = on && !frozen;
                EXPECT_EQ(pickEntity(document, {5, 0}, 0.5).has_value(), visible);
                for (const auto mode : {RectangleMode::Window, RectangleMode::Crossing}) {
                    const auto selected = selectInRectangle(document, {{-1, -1}, {11, 1}}, mode);
                    const auto expected =
                        visible ? std::vector<EntityId>{id} : std::vector<EntityId>{};
                    EXPECT_EQ(selected, expected);
                }
                EXPECT_EQ(findSnap(document, {0, 0}, 1, SnapModes{}).has_value(), visible);
                EXPECT_EQ(document.extents().min(), bounds.min());
                EXPECT_EQ(document.extents().max(), bounds.max());
            }
        }
    }
}

// --- pickEntity ---------------------------------------------------------

TEST(DocumentQuery, PickOnAnEntity) {
    Document document;
    const EntityId line = document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(value(pickEntity(document, {5, 0}, 0.5)), line.value);
    EXPECT_EQ(value(pickEntity(document, {5, 0}, 0.0)), line.value);
    EXPECT_EQ(value(pickEntity(document, {5, 0.4}, 0.5)), line.value);
}

TEST(DocumentQuery, PickToleranceIsInclusive) {
    Document document;
    const EntityId line = document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(value(pickEntity(document, {5, 2}, 2.0)), line.value);
    EXPECT_EQ(pickEntity(document, {5, 2.5}, 2.0), std::nullopt);
}

TEST(DocumentQuery, PickBetweenTwoEntitiesTakesTheNearer) {
    Document document;
    const EntityId lower = document.add(segment({0, 0}, {10, 0}));
    const EntityId upper = document.add(segment({0, 2}, {10, 2}));
    EXPECT_EQ(value(pickEntity(document, {5, 0.75}, 3.0)), lower.value);
    EXPECT_EQ(value(pickEntity(document, {5, 1.25}, 3.0)), upper.value);
}

TEST(DocumentQuery, PickAtEqualDistanceTakesTheHighestId) {
    Document document;
    const EntityId upper = document.add(segment({0, 2}, {10, 2}));
    const EntityId lower = document.add(segment({0, 0}, {10, 0}));
    ASSERT_GT(lower.value, upper.value);
    EXPECT_EQ(value(pickEntity(document, {5, 1}, 3.0)), lower.value);
}

TEST(DocumentQuery, PickInTheVoidFindsNothing) {
    Document document;
    EXPECT_EQ(pickEntity(document, {0, 0}, 5.0), std::nullopt);
    document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(pickEntity(document, {5, 30}, 5.0), std::nullopt);
}

TEST(DocumentQuery, PickNeverTakesAnEmptyEntity) {
    Document document;
    document.add(Polyline{});
    EXPECT_EQ(pickEntity(document, {0, 0}, 1e6), std::nullopt);
}

TEST(DocumentQuery, PickInsideAClosedPolylineFindsNothing) {
    Document document;
    document.add(rectangle({0, 0}, {10, 10}));
    EXPECT_EQ(pickEntity(document, {5, 5}, 1.0), std::nullopt);
}

TEST(DocumentQuery, PickIgnoresRemovedEntities) {
    Document document;
    const EntityId first = document.add(segment({0, 0}, {10, 0}));
    const EntityId second = document.add(segment({0, 0.5}, {10, 0.5}));
    ASSERT_TRUE(document.remove(second).has_value());
    EXPECT_EQ(value(pickEntity(document, {5, 0.5}, 1.0)), first.value);
}

// --- selectInRectangle --------------------------------------------------

/// Three entities: one inside [0,10]x[0,10], one across its right border,
/// one outside.
struct RectangleScene {
    Document document;
    EntityId outside;
    EntityId across;
    EntityId inside;

    RectangleScene()
        : outside(document.add(segment({20, 20}, {30, 20}))),
          across(document.add(segment({8, 5}, {15, 5}))),
          inside(document.add(rectangle({2, 2}, {4, 4}))) {}
};

TEST(DocumentQuery, WindowTakesOnlyEntitiesFullyInside) {
    const RectangleScene scene;
    EXPECT_EQ(values(selectInRectangle(scene.document, {{0, 0}, {10, 10}}, RectangleMode::Window)),
              (std::vector<std::uint64_t>{scene.inside.value}));
}

TEST(DocumentQuery, CrossingAlsoTakesEntitiesAcrossTheBorder) {
    const RectangleScene scene;
    EXPECT_EQ(
        values(selectInRectangle(scene.document, {{0, 0}, {10, 10}}, RectangleMode::Crossing)),
        (std::vector<std::uint64_t>{scene.across.value, scene.inside.value}));
}

TEST(DocumentQuery, RectangleResultsAreSortedById) {
    Document document;
    std::vector<std::uint64_t> expected;
    for (int i = 0; i < 20; ++i) {
        const double x = 2.0 * (19 - i);  // added right to left
        expected.push_back(document.add(segment({x, 0}, {x + 1, 1})).value);
    }
    std::ranges::sort(expected);
    EXPECT_EQ(values(selectInRectangle(document, {{-1, -1}, {50, 50}}, RectangleMode::Window)),
              expected);
}

TEST(DocumentQuery, RectangleOutsideOrEmptySelectsNothing) {
    const RectangleScene scene;
    EXPECT_TRUE(
        selectInRectangle(scene.document, {{40, 40}, {50, 50}}, RectangleMode::Crossing).empty());
    EXPECT_TRUE(selectInRectangle(scene.document, BBox{}, RectangleMode::Crossing).empty());
}

TEST(DocumentQuery, CrossingInsideAClosedPolylineWithoutTouchingIt) {
    Document document;
    document.add(rectangle({0, 0}, {10, 10}));
    EXPECT_TRUE(selectInRectangle(document, {{4, 4}, {6, 6}}, RectangleMode::Crossing).empty());
}

// --- findSnap -----------------------------------------------------------

constexpr SnapModes kAllModes{};

TEST(DocumentQuery, SnapToEndpoint) {
    Document document;
    document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(findSnap(document, {9.5, 0.5}, 1.0, kAllModes),
              (SnapPoint{{10, 0}, SnapKind::Endpoint}));
}

TEST(DocumentQuery, SnapToMidpoint) {
    Document document;
    document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(findSnap(document, {5.5, 0.5}, 1.0, kAllModes),
              (SnapPoint{{5, 0}, SnapKind::Midpoint}));
}

TEST(DocumentQuery, SnapTakesTheNearestPoint) {
    Document document;
    document.add(segment({0, 0}, {2, 0}));  // endpoints 0 and 2, midpoint 1
    EXPECT_EQ(findSnap(document, {1.6, 0}, 5.0, kAllModes),
              (SnapPoint{{2, 0}, SnapKind::Endpoint}));
    EXPECT_EQ(findSnap(document, {1.4, 0}, 5.0, kAllModes),
              (SnapPoint{{1, 0}, SnapKind::Midpoint}));
}

TEST(DocumentQuery, SnapPrefersEndpointToMidpointAtEqualDistance) {
    Document document;
    // Entity 1 offers the endpoint (5, 2); entity 2 the midpoint (5, 0).
    // The pointer is 1 away from both: the endpoint wins despite the lower id.
    const EntityId vertical = document.add(segment({5, 2}, {5, 10}));
    const EntityId horizontal = document.add(segment({0, 0}, {10, 0}));
    ASSERT_LT(vertical.value, horizontal.value);
    EXPECT_EQ(findSnap(document, {5, 1}, 2.0, kAllModes), (SnapPoint{{5, 2}, SnapKind::Endpoint}));
}

TEST(DocumentQuery, SnapAtEqualDistanceAndKindTakesTheHighestId) {
    Document document;
    document.add(segment({0, 2}, {-10, 2}));  // endpoint (0, 2)
    document.add(segment({0, 0}, {-10, 0}));  // endpoint (0, 0), higher id
    EXPECT_EQ(findSnap(document, {0, 1}, 2.0, kAllModes), (SnapPoint{{0, 0}, SnapKind::Endpoint}));
}

/// A half circle of radius 10 around the origin, from (-10, 0) to (10, 0)
/// through (0, -10).
Polyline halfCircle() {
    return segment({-10, 0}, {10, 0}, 1.0);
}

/// A point of halfCircle() at -45 degrees, far from its endpoints and midpoint.
constexpr double kOnArc = 10.0 / std::numbers::sqrt2;

/// Arc centres are computed with trigonometry: exact only up to rounding.
constexpr double kCenterTolerance = 1e-12;

void expectCenter(const std::optional<SnapPoint>& actual, const Vec2& expected) {
    if (!actual) {
        ADD_FAILURE() << "no snap";
        return;
    }
    EXPECT_EQ(actual->kind, SnapKind::Center);
    EXPECT_NEAR(actual->point.x, expected.x, kCenterTolerance);
    EXPECT_NEAR(actual->point.y, expected.y, kCenterTolerance);
}

/// The kind of the snap found, nothing if there is none.
std::optional<SnapKind> kindOf(const std::optional<SnapPoint>& snap) {
    return snap ? std::optional<SnapKind>{snap->kind} : std::nullopt;
}

/// A snap of `kind` at `expected`, up to rounding.
void expectSnap(const std::optional<SnapPoint>& actual, SnapKind kind, const Vec2& expected) {
    if (!actual) {
        ADD_FAILURE() << "no snap";
        return;
    }
    EXPECT_EQ(actual->kind, kind);
    EXPECT_NEAR(actual->point.x, expected.x, kCenterTolerance);
    EXPECT_NEAR(actual->point.y, expected.y, kCenterTolerance);
}

TEST(DocumentQuery, SnapToCenterWhenPassingOverTheArc) {
    Document document;
    document.add(halfCircle());
    expectCenter(findSnap(document, {kOnArc, -kOnArc}, 1.0, kAllModes), {0, 0});
}

TEST(DocumentQuery, SnapNeverTakesCenterNearTheCenterItself) {
    Document document;
    document.add(halfCircle());
    EXPECT_EQ(findSnap(document, {0, 0.5}, 1.0, kAllModes), std::nullopt);
}

TEST(DocumentQuery, SnapPrefersEndpointOrMidpointToCenter) {
    Document document;
    document.add(halfCircle());
    EXPECT_EQ(findSnap(document, {0, -9.5}, 1.0, kAllModes),
              (SnapPoint{{0, -10}, SnapKind::Midpoint}));
}

TEST(DocumentQuery, SnapCenterOfTheNearestArc) {
    Document document;
    document.add(segment({-10, 0}, {10, 0}, 1.0));      // centre (0, 0), radius 10
    document.add(segment({-10, 0.5}, {10, 0.5}, 1.0));  // centre (0, 0.5), radius 10
    // About 0.28 from the first arc and 0.07 from the second: the second
    // wins, although the pointer is nearer to the first centre.
    expectCenter(findSnap(document, {kOnArc, -kOnArc + 0.4}, 1.0, kAllModes), {0, 0.5});
}

TEST(DocumentQuery, SnapHonorsEachDisabledMode) {
    Document document;
    document.add(segment({0, 0}, {2, 0}));
    document.add(halfCircle());

    // Endpoint off: the pointer on the endpoint falls back to the midpoint.
    EXPECT_EQ(findSnap(document, {1.9, 0}, 1.0, {false, true, true}),
              (SnapPoint{{1, 0}, SnapKind::Midpoint}));
    // Midpoint off: the pointer on the midpoint falls back to an endpoint.
    EXPECT_EQ(findSnap(document, {1.1, 0}, 1.0, {true, false, true}),
              (SnapPoint{{2, 0}, SnapKind::Endpoint}));
    // Center off: passing over the arc finds nothing.
    EXPECT_EQ(findSnap(document, {kOnArc, -kOnArc}, 1.0, {true, true, false}), std::nullopt);
    // Only center: endpoints and midpoints are ignored, the arc still gives its centre.
    EXPECT_EQ(findSnap(document, {1.9, 0}, 1.0, {false, false, true}), std::nullopt);
    expectCenter(findSnap(document, {-10, 0}, 1.0, {false, false, true}), {0, 0});
    // Every mode off: nothing, even exactly on a vertex.
    EXPECT_EQ(findSnap(document, {2, 0}, 1.0, {false, false, false}), std::nullopt);
}

TEST(DocumentQuery, SnapToASingleVertex) {
    Document document;
    Polyline point;
    point.addVertex({3, 4});
    document.add(point);
    EXPECT_EQ(findSnap(document, {3.5, 4}, 1.0, kAllModes),
              (SnapPoint{{3, 4}, SnapKind::Endpoint}));
    EXPECT_EQ(findSnap(document, {3.5, 4}, 1.0, {false, true, true}), std::nullopt);
}

TEST(DocumentQuery, SnapInTheVoidOrOnAnEmptyDocument) {
    Document document;
    EXPECT_EQ(findSnap(document, {0, 0}, 1.0, kAllModes), std::nullopt);
    document.add(Polyline{});
    document.add(segment({0, 0}, {10, 0}));
    EXPECT_EQ(findSnap(document, {5, 5}, 1.0, kAllModes), std::nullopt);
}

// --- findSnap: intersections (docs/adr/0033, points 3 and 5) -------------

constexpr SnapModes kIntersectionOnly{false, false, false, false, true};

void expectIntersection(const std::optional<SnapPoint>& actual, const Vec2& expected) {
    expectSnap(actual, SnapKind::Intersection, expected);
}

TEST(IntersectionSnap, CrossesTwoLinesAndIsEnabledByDefault) {
    Document document;
    document.add(Line{{-10, -10}, {4, 4}});
    document.add(Line{{-10, 10}, {4, -4}});
    expectIntersection(findSnap(document, {0.1, 0.2}, 0.5, kIntersectionOnly), {0, 0});
    expectIntersection(findSnap(document, {0.1, 0.2}, 0.5, {}), {0, 0});
    constexpr SnapModes kNone{false, false, false, false, false};
    EXPECT_FALSE(findSnap(document, {0.1, 0.2}, 0.5, kNone));
}

TEST(IntersectionSnap, CrossesALineAndAnArcWithoutExtendingEither) {
    Document document;
    document.add(Arc{{0, 0}, 5, 0, kPi});
    document.add(Line{{-10, 4}, {10, 4}});
    expectIntersection(findSnap(document, {3.1, 4}, 0.5, kIntersectionOnly), {3, 4});
    EXPECT_FALSE(findSnap(document, {3, -4}, 0.5, kIntersectionOnly));
    Document shortLine;
    shortLine.add(Line{{-10, 0}, {-1, 0}});
    shortLine.add(Line{{0, -10}, {0, 10}});
    EXPECT_FALSE(findSnap(shortLine, {0, 0}, 2, kIntersectionOnly));
}

TEST(IntersectionSnap, CrossesTwoCirclesAndIncludesTangencies) {
    Document document;
    document.add(Circle{{0, 0}, 5});
    document.add(Circle{{6, 0}, 5});
    expectIntersection(findSnap(document, {3, 4.1}, 0.5, kIntersectionOnly), {3, 4});
    Document tangent;
    tangent.add(Circle{{0, 0}, 5});
    tangent.add(Line{{-10, 5}, {10, 5}});
    expectIntersection(findSnap(tangent, {0.1, 5}, 0.5, kIntersectionOnly), {0, 5});
}

TEST(IntersectionSnap, FindsSelfIntersectionsAndOneAnswerForSeveralPairs) {
    Document document;
    document.add(Polyline{{{{-5, -5}}, {{5, 5}}, {{5, -5}}, {{-5, 5}}}});
    expectIntersection(findSnap(document, {0.1, 0}, 0.5, kIntersectionOnly), {0, 0});
    document.add(Line{{-10, 0}, {10, 0}});
    expectIntersection(findSnap(document, {0.1, 0}, 0.5, kIntersectionOnly), {0, 0});
}

TEST(IntersectionSnap, RejectsCrossingsOutsideTheCircularApertureAndSharedStretches) {
    Document document;
    document.add(Line{{-10, 0}, {10, 0}});
    document.add(Line{{0, -10}, {0, 10}});
    // Both curves pass within 1, but their crossing is sqrt(2) away.
    EXPECT_FALSE(findSnap(document, {1, 1}, 1, kIntersectionOnly));
    Document shared;
    shared.add(Line{{0, 0}, {10, 0}});
    shared.add(Line{{5, 0}, {15, 0}});
    EXPECT_FALSE(findSnap(shared, {5, 0}, 1, kIntersectionOnly));
}

TEST(IntersectionSnap, IgnoresOffAndFrozenLayersButIncludesLockedLayers) {
    Document document = Document::newDrawing();
    document.add(Line{{-10, 0}, {10, 0}});
    Layer layer = *document.layer(document.layerZero());
    layer.id = {};
    layer.name = "Crossing";
    const auto layerId = document.addLayer(layer);
    const auto crossing = document.add(Line{{0, -10}, {0, 10}});
    auto properties = document.find(crossing)->properties;
    properties.layer = layerId;
    document.setProperties(crossing, properties);
    for (const bool on : {false, true}) {
        for (const bool frozen : {false, true}) {
            layer = *document.layer(layerId);
            layer.on = on;
            layer.frozen = frozen;
            layer.locked = true;
            document.replaceLayer(layer);
            EXPECT_EQ(findSnap(document, {0.1, 0}, 0.5, kIntersectionOnly).has_value(),
                      on && !frozen);
        }
    }
}

TEST(IntersectionSnap, UsesDistanceBeforeKindThenEndpointBeforeIntersection) {
    Document document;
    document.add(Line{{-10, -10}, {4, 4}});
    document.add(Line{{-10, 10}, {4, -4}});
    document.add(Line{{2, 0}, {10, 0}});
    constexpr SnapModes kEndAndIntersection{true, false, false, false, true};
    expectSnap(findSnap(document, {1, 0}, 1.5, kEndAndIntersection), SnapKind::Endpoint, {2, 0});
    expectIntersection(findSnap(document, {0.8, 0}, 1.5, kEndAndIntersection), {0, 0});
}

TEST(IntersectionSnap, PrefersTheHighestEntityIdOnAnEqualDistance) {
    Document document;
    document.add(Line{{-10, 0}, {10, 0}});
    document.add(Line{{-1, -10}, {-1, 10}});
    document.add(Line{{1, -10}, {1, 10}});
    expectIntersection(findSnap(document, {0, 0}, 1, kIntersectionOnly), {1, 0});
}

// --- findSnap: quadrant and perpendicular (docs/adr/0033, points 4 and 5) ---

constexpr SnapModes kQuadrantOnly{false, false, false, false, false, false, false, true};
constexpr SnapModes kPerpendicularOnly{false, false, false, false, false, true, false, false};

TEST(DocumentQuery, SnapToAQuadrantOfACircle) {
    Document document;
    document.add(Circle{{0, 0}, 5});
    EXPECT_EQ(findSnap(document, {5.2, 0.3}, 1.0, kQuadrantOnly),
              (SnapPoint{{5, 0}, SnapKind::Quadrant}));
    EXPECT_EQ(findSnap(document, {-0.2, 5.3}, 1.0, kQuadrantOnly),
              (SnapPoint{{0, 5}, SnapKind::Quadrant}));
    // Away from the four points, even on the circle: nothing.
    EXPECT_EQ(findSnap(document, {3.5355, 3.5355}, 0.5, kQuadrantOnly), std::nullopt);
    // With the mode off (the default) there is none: the centre of the circle
    // crossed is all the default modes give.
    EXPECT_EQ(kindOf(findSnap(document, {5.2, 0.3}, 1.0, kAllModes)), SnapKind::Center);
}

TEST(DocumentQuery, QuadrantsOfAnArcAreOnlyThoseInsideIt) {
    Document document;
    document.add(Arc{{0, 0}, 5, 0, kPi / 2.0});
    EXPECT_EQ(findSnap(document, {0.2, 5.2}, 1.0, kQuadrantOnly),
              (SnapPoint{{0, 5}, SnapKind::Quadrant}));
    EXPECT_EQ(findSnap(document, {-5.2, 0.2}, 1.0, kQuadrantOnly), std::nullopt);
}

TEST(DocumentQuery, PerpendicularNeedsTheBasePointAndTheFootInTheAperture) {
    Document document;
    document.add(segment({0, 0}, {10, 0}));
    const SnapPoint foot{{4, 0}, SnapKind::Perpendicular};
    EXPECT_EQ(findSnap(document, {4.3, 0.4}, 1.0, kPerpendicularOnly, Vec2{4, 8}), foot);
    // Without a base point the mode gives nothing.
    EXPECT_EQ(findSnap(document, {4.3, 0.4}, 1.0, kPerpendicularOnly), std::nullopt);
    // The pointer is near the line but not near the foot.
    EXPECT_EQ(findSnap(document, {7.0, 0.4}, 1.0, kPerpendicularOnly, Vec2{4, 8}), std::nullopt);
    // The foot would be on the extension of the line.
    EXPECT_EQ(findSnap(document, {10.2, 0.4}, 1.0, kPerpendicularOnly, Vec2{14, 8}), std::nullopt);
}

TEST(DocumentQuery, PerpendicularOfACircleIsTheNearerFootInTheAperture) {
    Document document;
    document.add(Circle{{0, 0}, 10});
    expectSnap(findSnap(document, {0.3, 9.7}, 1.0, kPerpendicularOnly, Vec2{0, 20}),
               SnapKind::Perpendicular, {0, 10});
    expectSnap(findSnap(document, {0.3, -9.7}, 1.0, kPerpendicularOnly, Vec2{0, 20}),
               SnapKind::Perpendicular, {0, -10});
}

// --- findSnap: tangent (docs/adr/0033, points 4 and 5; package P19) ---

constexpr SnapModes kTangentOnly{false, false, false, false, false, false, true, false};

TEST(DocumentQuery, TangentNeedsTheBasePointAndThePointInTheAperture) {
    Document document;
    document.add(Circle{{0, 0}, 5});
    // From (10,0) the two lines touch the circle at x = 2.5,
    // y = +-5 * sqrt(75) / 10.
    const double y = 5.0 * std::sqrt(75.0) / 10.0;
    expectSnap(findSnap(document, {2.6, 4.2}, 1.0, kTangentOnly, Vec2{10, 0}), SnapKind::Tangent,
               {2.5, y});
    expectSnap(findSnap(document, {2.6, -4.2}, 1.0, kTangentOnly, Vec2{10, 0}), SnapKind::Tangent,
               {2.5, -y});
    // Without a base point the mode gives nothing.
    EXPECT_EQ(findSnap(document, {2.6, 4.2}, 1.0, kTangentOnly), std::nullopt);
    // On the circle but far from both points of tangency.
    EXPECT_EQ(findSnap(document, {-5.0, 0.2}, 1.0, kTangentOnly, Vec2{10, 0}), std::nullopt);
    // From inside the circle there is no tangent.
    EXPECT_EQ(findSnap(document, {2.6, 4.2}, 1.0, kTangentOnly, Vec2{1, 0}), std::nullopt);
    // With the mode off (the default) there is none.
    EXPECT_NE(kindOf(findSnap(document, {2.6, 4.2}, 1.0, kAllModes, Vec2{10, 0})),
              SnapKind::Tangent);
}

TEST(DocumentQuery, TangentOfAnArcIsOnlyOnTheArcAndALineHasNone) {
    Document document;
    document.add(Arc{{0, 0}, 5, 0, kPi / 2.0});
    document.add(segment({20, 0}, {30, 0}));
    const double y = 5.0 * std::sqrt(75.0) / 10.0;
    expectSnap(findSnap(document, {2.6, 4.2}, 1.0, kTangentOnly, Vec2{10, 0}), SnapKind::Tangent,
               {2.5, y});
    EXPECT_EQ(findSnap(document, {2.6, -4.2}, 1.0, kTangentOnly, Vec2{10, 0}), std::nullopt);
    EXPECT_EQ(findSnap(document, {25.0, 0.2}, 1.0, kTangentOnly, Vec2{25, 10}), std::nullopt);
}

TEST(DocumentQuery, TangentOfAPolylineArcAndTheEndpointFirstOnATie) {
    // Two half circles below (0,0)-(2,0)-(4,0): from (2,1) the shared vertex
    // (2,0) is the point of tangency of both (B7 of #464).
    Document document;
    Polyline polyline;
    polyline.addVertex({0, 0}, 1.0);
    polyline.addVertex({2, 0}, 1.0);
    polyline.addVertex({4, 0});
    document.add(polyline);
    expectSnap(findSnap(document, {2.1, -0.1}, 0.5, kTangentOnly, Vec2{2, 1}), SnapKind::Tangent,
               {2, 0});
    constexpr SnapModes kEndpointAndTangent{true, false, false, false, false, false, true, false};
    EXPECT_EQ(kindOf(findSnap(document, {2.1, -0.1}, 0.5, kEndpointAndTangent, Vec2{2, 1})),
              SnapKind::Endpoint);
}

TEST(DocumentQuery, TiesFollowEndpointMidpointQuadrantPerpendicular) {
    constexpr SnapModes kEverything{true, true, true, true, true, true, true, true};
    // The arc from (10,0) to (-10,0) over the top: its midpoint, the quadrant
    // at 90° and the foot from (0,20) are the same point (0,10).
    Document document;
    document.add(Arc{{0, 0}, 10, 0, kPi});
    EXPECT_EQ(kindOf(findSnap(document, {0, 10.5}, 1.0, kEverything, Vec2{0, 20})),
              SnapKind::Midpoint);

    // Quadrant before Perpendicular.
    EXPECT_EQ(kindOf(findSnap(document, {0, 10.5}, 1.0,
                              SnapModes{false, false, false, false, false, true, false, true},
                              Vec2{0, 20})),
              SnapKind::Quadrant);

    // Endpoint before Quadrant: the arc ends on a quadrant.
    Document quarter;
    quarter.add(Arc{{0, 0}, 10, 0, kPi / 2.0});
    EXPECT_EQ(kindOf(findSnap(quarter, {10.2, 0}, 1.0, kEverything)), SnapKind::Endpoint);
}

TEST(DocumentQuery, TheNearestSnapBeatsTheKindRank) {
    Document document;
    document.add(Circle{{0, 0}, 10});
    // The pointer is nearer to the perpendicular foot than to the quadrant.
    EXPECT_EQ(kindOf(findSnap(document, {5.0, 8.7}, 3.0,
                              SnapModes{false, false, false, false, false, true, false, true},
                              Vec2{10, 17.32})),
              SnapKind::Perpendicular);
}

TEST(DocumentQuery, TheCentreIsOnlyTheLastResort) {
    Document document;
    document.add(Circle{{0, 0}, 5});
    constexpr SnapModes kCenterAndQuadrant{false, false, true, false, false, false, false, true};
    // A quadrant within the aperture wins over the centre.
    EXPECT_EQ(findSnap(document, {5.1, 0.1}, 1.0, kCenterAndQuadrant),
              (SnapPoint{{5, 0}, SnapKind::Quadrant}));
    // With none within the aperture, the centre of the circle crossed.
    EXPECT_EQ(kindOf(findSnap(document, {3.5355, 3.5355}, 0.5, kCenterAndQuadrant)),
              SnapKind::Center);
}

TEST(DocumentQuery, EveryModeOffGivesNothing) {
    Document document;
    document.add(Circle{{0, 0}, 5});
    document.add(segment({0, 0}, {10, 0}));
    constexpr SnapModes kNone{false, false, false, false, false, false, false, false};
    EXPECT_EQ(findSnap(document, {5, 0}, 1.0, kNone, Vec2{5, 5}), std::nullopt);
    EXPECT_EQ(findSnap(document, {0, 0}, 1.0, kNone), std::nullopt);
}

// --- Nearest (docs/adr/0057, point 2) -------------------------------------

/// Only Nearest, with the modes of `others` that are on.
SnapModes nearestAnd(SnapModes others = {false, false, false, false, false}) {
    others.nearest = true;
    return others;
}

void expectNearest(const std::optional<SnapPoint>& actual, const Vec2& expected,
                   double tolerance = 1e-12) {
    if (!actual) {
        ADD_FAILURE() << "no snap";
        return;
    }
    EXPECT_EQ(actual->kind, SnapKind::Nearest);
    EXPECT_NEAR(actual->point.x, expected.x, tolerance);
    EXPECT_NEAR(actual->point.y, expected.y, tolerance);
}

TEST(NearestSnap, IsOffByDefaultAndAloneEnablesTheSnap) {
    EXPECT_FALSE(SnapModes{}.nearest);
    EXPECT_FALSE(SnapModes{}.extension);
    EXPECT_FALSE(SnapModes{}.parallel);
    Document document;
    document.add(Line{{0, 20}, {10, 20}});
    EXPECT_FALSE(findSnap(document, {3, 20.25}, 0.5, SnapModes{}));
    expectNearest(findSnap(document, {3, 20.25}, 0.5, nearestAnd()), {3, 20});
    // A point exactly on the line within a zero aperture.
    expectNearest(findSnap(document, {3, 20}, 0.0, nearestAnd()), {3, 20});
    EXPECT_FALSE(findSnap(document, {3, 20.25}, 0.0, nearestAnd()));
    // Extension and parallel alone give no snap point.
    SnapModes guides{false, false, false, false, false};
    guides.extension = true;
    guides.parallel = true;
    EXPECT_FALSE(findSnap(document, {3, 20}, 1.0, guides));
}

// The six objects of autocad-vicino.dxf, as AutoCAD 2021 placed its points.
TEST(NearestSnap, TheObjectsOfAutocadVicino) {
    Document document = Document::newDrawing();
    document.add(Line{{0, 0}, {40, 10}});
    document.add(Arc{{70, 0}, 10, 0, std::numbers::pi});
    document.add(Circle{{110, 0}, 10});
    document.add(Polyline{{{{140, 0}}, {{170, 0}, 1.0}, {{170, 20}}, {{140, 20}}}, false});
    document.add(Ellipse{{210, 0}, {15, 0}, 1.0 / 3.0, 0.0, 2.0 * std::numbers::pi});
    MText text;
    text.insertion = {240, 10};
    text.height = 2.5;
    text.width = 30;
    text.value = "Testo multiriga per lo snap Vicino";
    document.add(text);
    const SnapModes modes = nearestAnd();
    expectNearest(findSnap(document, {20, 5.3}, 0.5, modes),
                  {20.07058823529412, 5.017647058823529});
    expectNearest(findSnap(document, {77.28, 7.28}, 0.5, modes),
                  {77.07106781186548, 7.071067811865473});
    expectNearest(findSnap(document, {118.92, 5.15}, 0.5, modes),
                  {118.6602390719562, 5.000025921589077});
    expectNearest(findSnap(document, {155, 0.3}, 0.5, modes), {155, 0});
    expectNearest(findSnap(document, {180.3, 10}, 0.5, modes), {180, 10});
    expectNearest(findSnap(document, {217.5, 4.6}, 0.5, modes),
                  {217.4503351974475, 4.339642912161662}, 1e-9);
    // The rows of an MTEXT give no Nearest, and here nothing else either.
    EXPECT_FALSE(findSnap(document, {250, 7}, 0.5, modes));
}

TEST(NearestSnap, TheEllipseUsesTheDistanceOfNearest) {
    Document document;
    document.add(Ellipse{{0, 0}, {10, 0}, 0.5, 0.0, 2.0 * std::numbers::pi});
    // solid-space certifies 2.8816126631665626 from (12, 3).
    EXPECT_FALSE(findSnap(document, {12, 3}, 2.0, nearestAnd()));
    expectNearest(findSnap(document, {12, 3}, 3.0, nearestAnd()),
                  {9.551207013215885, 1.481084449879183}, 1e-9);
}

TEST(NearestSnap, BeatsTheCentreOnTheRimButNotAPointInTheAperture) {
    Document document;
    document.add(Circle{{0, 0}, 10});
    document.add(Line{{0, 20}, {10, 20}});
    const SnapModes centre{false, false, true, false, false};
    expectNearest(findSnap(document, {10.25, 0}, 1.0, nearestAnd(centre)), {10, 0});
    EXPECT_EQ(findSnap(document, {10.25, 0}, 1.0, centre), (SnapPoint{{0, 0}, SnapKind::Center}));
    // The end point within the aperture wins, though the line is nearer.
    const SnapModes endpoint{true, false, false, false, false};
    EXPECT_EQ(findSnap(document, {9.75, 20.25}, 1.0, nearestAnd(endpoint)),
              (SnapPoint{{10, 20}, SnapKind::Endpoint}));
    expectNearest(findSnap(document, {9.75, 20.25}, 1.0, nearestAnd()), {9.75, 20});
    // Every mode of 0.7 on: away from the points of the line, Nearest.
    SnapModes every;
    every.perpendicular = true;
    every.tangent = true;
    every.quadrant = true;
    expectNearest(findSnap(document, {2, 20.25}, 1.0, nearestAnd(every)), {2, 20});
}

TEST(NearestSnap, TiesTakeTheHighestIdThenTheFirstSegment) {
    Document document;
    document.add(Line{{100, -1}, {110, -1}});
    document.add(Line{{100, 1}, {110, 1}});
    expectNearest(findSnap(document, {105, 0}, 2.0, nearestAnd()), {105, 1});
    expectNearest(findSnap(document, {105, -0.25}, 2.0, nearestAnd()), {105, -1});
    // Inside a polyline bend, the first of two segments at the same distance.
    document.add(Polyline{{{{20, 0}}, {{30, 0}}, {{30, 10}}}});
    expectNearest(findSnap(document, {29, 1}, 2.0, nearestAnd()), {29, 0});
}

/// A block entity with the default properties of a new drawing.
EntityRecord member(const Document& document, Entity entity) {
    EntityRecord record{EntityId{}, std::move(entity)};
    record.properties.layer = document.layerZero();
    record.properties.linetype = document.byLayerLinetype();
    return record;
}

/// Puts the block `name` with base (0, 0) and `entities` in the document.
void defineBlock(Document& document, const std::string& name, std::vector<Entity> entities) {
    BlockDefinition definition{name};
    for (Entity& entity : entities) {
        definition.entities.push_back(member(document, std::move(entity)));
    }
    document.replaceBlock(name, std::move(definition));
}

Insert insertOf(std::string name, const Vec2& insertion) {
    Insert insert;
    insert.block = std::move(name);
    insert.insertion = insertion;
    return insert;
}

TEST(NearestSnap, TheShapesOfAnInsertInGenerationOrder) {
    Document document = Document::newDrawing();
    defineBlock(document, "Pair", {Line{{0, -1}, {10, -1}}, Line{{0, 1}, {10, 1}}});
    document.add(insertOf("Pair", {0, 0}));
    expectNearest(findSnap(document, {5, 0}, 2.0, nearestAnd()), {5, -1});
    // The insertion point is a point snap: within the aperture it wins.
    document.add(insertOf("Pair", {40, 0}));
    const SnapModes insertion{false, false, false, true, false};
    EXPECT_EQ(findSnap(document, {40, 0.75}, 1.0, nearestAnd(insertion)),
              (SnapPoint{{40, 0}, SnapKind::Insertion}));
    expectNearest(findSnap(document, {40, 0.75}, 1.0, nearestAnd()), {40, 1});
    // So it does with a block without shapes, which gives no Nearest.
    defineBlock(document, "Empty", {});
    document.add(insertOf("Empty", {60, 0}));
    EXPECT_EQ(findSnap(document, {60, 0.5}, 1.0, nearestAnd(insertion)),
              (SnapPoint{{60, 0}, SnapKind::Insertion}));
    EXPECT_FALSE(findSnap(document, {60, 0.5}, 1.0, nearestAnd()));
}

TEST(NearestSnap, TheSidesOfASolidNotItsRegion) {
    Document document = Document::newDrawing();
    document.add(Solid{{Vec2{80, 0}, Vec2{90, 0}, Vec2{80, 10}, Vec2{90, 10}}});
    // Inside, farther than the aperture from every side.
    EXPECT_FALSE(findSnap(document, {85, 5}, 2.0, nearestAnd()));
    // Equally far from the bottom and the right side: the first, v0 to v1.
    expectNearest(findSnap(document, {89, 1}, 2.0, nearestAnd()), {89, 0});
    expectNearest(findSnap(document, {91, 5}, 2.0, nearestAnd()), {90, 5});
    // The sides of a solid inside an insert are shapes too.
    defineBlock(document, "Filled", {Solid{{Vec2{0, 0}, Vec2{20, 0}, Vec2{0, 10}, Vec2{20, 10}}}});
    document.add(insertOf("Filled", {130, 0}));
    expectNearest(findSnap(document, {140, -0.3}, 0.5, nearestAnd()), {140, 0});
}

TEST(NearestSnap, TheLineOfADimension) {
    Document document = Document::newDrawing();
    Dimension dimension;
    dimension.definition.kind = DimensionKind::Linear;
    dimension.definition.point13 = {80, 0};
    dimension.definition.point14 = {110, 0};
    dimension.definition.linePoint = {110, 10};
    std::string reason;
    const std::optional<Dimension> made = regenerated(document, std::move(dimension), &reason);
    EXPECT_TRUE(made.has_value()) << reason;
    document.add(made.value_or(Dimension{}));
    expectNearest(findSnap(document, {88, 9.7}, 0.5, nearestAnd()), {88, 10});
    // Over the text of the measurement, nothing.
    EXPECT_FALSE(findSnap(document, {95, 11.875}, 0.5, nearestAnd()));
}

TEST(NearestSnap, NoneOnTextsAttributesAndHatches) {
    Document document = Document::newDrawing();
    Text text;
    text.position = {0, 40};
    text.height = 2.5;
    text.value = "AB";
    document.add(text);
    EXPECT_FALSE(findSnap(document, {1, 41}, 1.0, nearestAnd()));
    // On the text the insertion point still answers, as before.
    const SnapModes insertion{false, false, false, true, false};
    EXPECT_EQ(findSnap(document, {1, 41}, 1.0, nearestAnd(insertion)),
              (SnapPoint{{0, 40}, SnapKind::Insertion}));

    AttributeDefinition definition;
    definition.text.position = {0, 50};
    definition.text.height = 2.5;
    definition.text.value = "Default";
    definition.tag = "CODE";
    document.add(definition);
    EXPECT_FALSE(findSnap(document, {1, 51}, 1.0, nearestAnd()));

    Text label = text;
    label.position = {0, 0};
    defineBlock(document, "Label", {label});
    document.add(insertOf("Label", {0, 60}));
    EXPECT_FALSE(findSnap(document, {1, 61}, 1.0, nearestAnd()));

    Hatch hatch;
    hatch.pattern = "SOLID";
    hatch.solid = true;
    HatchLoop loop;
    loop.flags = 3;
    loop.polyline = Polyline{{{{170, 0}}, {{200, 0}}, {{200, 20}}, {{170, 20}}}, true};
    hatch.loops.push_back(loop);
    document.add(hatch);
    EXPECT_FALSE(findSnap(document, {185, -0.3}, 0.5, nearestAnd()));
    EXPECT_FALSE(findSnap(document, {185, 10}, 0.5, nearestAnd()));
}

// --- Comparison with an exhaustive search -------------------------------

/// Random geometry reproducible on every platform: std::mt19937 is fully
/// specified by the standard, its distributions are not.
class Generator {
public:
    explicit Generator(std::uint32_t seed) : m_engine(seed) {}

    double unit() { return static_cast<double>(m_engine()) / kRange; }

    double between(double low, double high) { return low + ((high - low) * unit()); }

    std::size_t index(std::size_t count) {
        return static_cast<std::size_t>(unit() * static_cast<double>(count));
    }

    /// Zero to four vertices near a random point, some segments arcs.
    Polyline polyline() {
        Polyline result;
        const Vec2 origin{between(0.0, 500.0), between(0.0, 500.0)};
        const auto vertices = index(5);
        for (std::size_t i = 0; i < vertices; ++i) {
            const double bulge = unit() < 0.3 ? between(-1.5, 1.5) : 0.0;
            result.addVertex(origin + Vec2{between(0.0, 40.0), between(0.0, 40.0)}, bulge);
        }
        result.setClosed(unit() < 0.2);
        return result;
    }

    BBox area() {
        const Vec2 corner{between(-20.0, 520.0), between(-20.0, 520.0)};
        return {corner, corner + Vec2{between(0.0, 150.0), between(0.0, 150.0)}};
    }

    /// Half of the points near a vertex of the document, so that queries hit.
    Vec2 point(const Document& document) {
        if (unit() < 0.5) {
            const auto entities = document.entities();
            const auto& entity = entities[index(document.entityCount())].entity;
            const auto vertices = std::get<Polyline>(entity).vertices();
            if (!vertices.empty()) {
                return vertices[index(vertices.size())].position +
                       Vec2{between(-3.0, 3.0), between(-3.0, 3.0)};
            }
        }
        return {between(-20.0, 560.0), between(-20.0, 560.0)};
    }

    // This exhaustive oracle covers the original per-entity snap modes.
    // Pair intersections have their own tests below.
    SnapModes modes() { return {unit() < 0.7, unit() < 0.7, unit() < 0.7, true, false}; }

private:
    static constexpr double kRange = 4294967296.0;  // 2^32, mt19937's range

    std::mt19937 m_engine;
};

std::optional<std::uint64_t> exhaustivePick(const Document& document, const Vec2& point,
                                            double tolerance) {
    std::optional<std::tuple<double, std::uint64_t>> best;  // (distance, -id)
    std::optional<std::uint64_t> picked;
    for (const EntityRecord& record : document.entities()) {
        const double d = distance(record.entity, point);
        const std::tuple<double, std::uint64_t> key{
            d, std::numeric_limits<std::uint64_t>::max() - record.id.value};
        if (d <= tolerance && (!best || key < *best)) {
            best = key;
            picked = record.id.value;
        }
    }
    return picked;
}

std::vector<std::uint64_t> exhaustiveSelect(const Document& document, const BBox& rectangle,
                                            RectangleMode mode) {
    std::vector<std::uint64_t> selected;
    for (const EntityRecord& record : document.entities()) {
        const bool inside = isInside(record.entity, rectangle);
        const bool crossing = inside || intersects(record.entity, rectangle);
        if (mode == RectangleMode::Window ? inside : crossing) {
            selected.push_back(record.id.value);
        }
    }
    std::ranges::sort(selected);
    return selected;
}

std::optional<SnapPoint> exhaustiveSnap(const Document& document, const Vec2& pointer,
                                        double aperture, const SnapModes& modes) {
    // (distance, kind rank, -id, order of discovery inside the entity)
    using Key = std::tuple<double, int, std::uint64_t, std::size_t>;
    std::optional<std::pair<Key, SnapPoint>> point;
    std::optional<std::pair<Key, SnapPoint>> center;
    for (const EntityRecord& record : document.entities()) {
        const auto& polyline = std::get<Polyline>(record.entity);
        std::vector<PolylineSegment> segments;
        if (polyline.vertexCount() == 1) {
            const Vec2 only = polyline.vertices().front().position;
            segments.push_back({only, only});
        }
        for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
            segments.push_back(polyline.segment(i));
        }
        const std::uint64_t reversedId =
            std::numeric_limits<std::uint64_t>::max() - record.id.value;
        std::size_t order = 0;
        for (const PolylineSegment& seg : segments) {
            const double segmentDistance = distance(seg, pointer);
            if (segmentDistance > aperture) {
                continue;
            }
            std::vector<SnapPoint> snaps;
            appendSnapPoints(seg, modes, snaps);
            for (const SnapPoint& snap : snaps) {
                ++order;
                if (snap.kind == SnapKind::Center) {
                    const Key key{segmentDistance, 0, reversedId, order};
                    if (!center || key < center->first) {
                        center = {key, snap};
                    }
                    continue;
                }
                const double d = distance(snap.point, pointer);
                const Key key{d, snap.kind == SnapKind::Endpoint ? 0 : 1, reversedId, order};
                if (d <= aperture && (!point || key < point->first)) {
                    point = {key, snap};
                }
            }
        }
    }
    if (point) {
        return point->second;
    }
    if (center) {
        return center->second;
    }
    return std::nullopt;
}

class RandomDocument : public testing::Test {
protected:
    static constexpr int kEntityCount = 2000;
    static constexpr int kQueryCount = 500;

    void SetUp() override {
        for (int i = 0; i < kEntityCount; ++i) {
            m_document.add(m_generator.polyline());
        }
        // Removals exercise the index after updates, not only after insertions;
        // the document is then refilled to kEntityCount polylines.
        for (std::uint64_t id = 7; id <= kEntityCount; id += 13) {
            ASSERT_TRUE(m_document.remove(EntityId{id}).has_value());
        }
        while (m_document.entityCount() < kEntityCount) {
            m_document.add(m_generator.polyline());
        }
    }

    Generator& generator() { return m_generator; }

    [[nodiscard]] const Document& document() const { return m_document; }

private:
    Generator m_generator{20261017U};
    Document m_document;
};

/// Entities of every type, as in the reference case modello/tipi-misti.
struct MixedDocument {
    Document document;
    EntityId polyline{document.add(Polyline{{{{0.0, 0.0}}, {{10.0, 0.0}}, {{10.0, 10.0}}}})};
    EntityId line{document.add(Line{{20.0, 0.0}, {30.0, 0.0}})};
    EntityId circle{document.add(Circle{{50.0, 0.0}, 10.0})};
    EntityId arc{document.add(Arc{{80.0, 0.0}, 10.0, 0.0, std::numbers::pi})};
};

TEST(MixedTypes, PickTakesEachTypeOnItsCurve) {
    const MixedDocument mixed;
    EXPECT_EQ(pickEntity(mixed.document, {5.0, 0.2}, 0.5), mixed.polyline);
    EXPECT_EQ(pickEntity(mixed.document, {25.0, 0.2}, 0.5), mixed.line);
    EXPECT_EQ(pickEntity(mixed.document, {50.0, 10.2}, 0.5), mixed.circle);
    EXPECT_EQ(pickEntity(mixed.document, {80.0, 10.2}, 0.5), mixed.arc);
    // The centre of a circle or of an arc is not on the curve.
    EXPECT_FALSE(pickEntity(mixed.document, {50.0, 0.0}, 2.0));
    EXPECT_FALSE(pickEntity(mixed.document, {80.0, 0.0}, 2.0));
}

TEST(MixedTypes, WindowAndCrossing) {
    const MixedDocument mixed;
    EXPECT_EQ(values(selectInRectangle(mixed.document, BBox{{-5.0, -15.0}, {95.0, 25.0}},
                                       RectangleMode::Window)),
              (std::vector<std::uint64_t>{mixed.polyline.value, mixed.line.value,
                                          mixed.circle.value, mixed.arc.value}));
    // Inside the circle, touching nothing.
    EXPECT_TRUE(
        selectInRectangle(mixed.document, BBox{{45.0, -5.0}, {55.0, 5.0}}, RectangleMode::Crossing)
            .empty());
    EXPECT_EQ(values(selectInRectangle(mixed.document, BBox{{38.0, -12.0}, {62.0, 12.0}},
                                       RectangleMode::Window)),
              (std::vector<std::uint64_t>{mixed.circle.value}));
    // Across the top of the arc, with no end point inside.
    EXPECT_EQ(values(selectInRectangle(mixed.document, BBox{{78.0, 8.0}, {82.0, 12.0}},
                                       RectangleMode::Crossing)),
              (std::vector<std::uint64_t>{mixed.arc.value}));
    EXPECT_EQ(values(selectInRectangle(mixed.document, BBox{{28.0, -2.0}, {82.0, 12.0}},
                                       RectangleMode::Crossing)),
              (std::vector<std::uint64_t>{mixed.line.value, mixed.circle.value, mixed.arc.value}));
}

TEST(MixedTypes, SnapFollowsThePrioritiesOfAdr0008) {
    const MixedDocument mixed;
    const SnapModes all;
    // The centre of the circle, passing over the circle.
    EXPECT_EQ(findSnap(mixed.document, {60.0, 0.5}, 1.0, all),
              (SnapPoint{{50.0, 0.0}, SnapKind::Center}));
    EXPECT_FALSE(findSnap(mixed.document, {50.0, 0.0}, 1.0, all));
    // On the arc the midpoint beats the centre; away from it, the centre.
    const SnapPoint midpoint =
        findSnap(mixed.document, {80.0, 9.8}, 1.0, all).value_or(SnapPoint{});
    EXPECT_EQ(midpoint.kind, SnapKind::Midpoint);
    EXPECT_NEAR(midpoint.point.x, 80.0, 1e-12);
    EXPECT_NEAR(midpoint.point.y, 10.0, 1e-12);
    EXPECT_EQ(findSnap(mixed.document, {87.071, 7.071}, 1.0, all),
              (SnapPoint{{80.0, 0.0}, SnapKind::Center}));
    // On the line the nearer of end point and midpoint.
    EXPECT_EQ(findSnap(mixed.document, {20.5, 0.5}, 2.0, all),
              (SnapPoint{{20.0, 0.0}, SnapKind::Endpoint}));
    EXPECT_EQ(findSnap(mixed.document, {25.0, 0.3}, 1.0, all),
              (SnapPoint{{25.0, 0.0}, SnapKind::Midpoint}));
    // Without the centre mode, passing over the circle snaps to nothing.
    EXPECT_FALSE(findSnap(mixed.document, {60.0, 0.5}, 1.0, SnapModes{true, true, false}));
}

TEST(DocumentQuery, SnapAnEndPointBeatsTheCentreOfAnotherType) {
    Document document;
    document.add(segment({0.0, 0.0}, {10.0, 0.0}));
    document.add(Circle{{30.0, 0.0}, 20.0});  // passes through (10,0)
    // The circle's centre is reached over the circle, but the polyline's end
    // point within the aperture wins (docs/adr/0008, point 4).
    EXPECT_EQ(findSnap(document, {10.2, 0.0}, 1.0, SnapModes{}),
              (SnapPoint{{10.0, 0.0}, SnapKind::Endpoint}));
    EXPECT_EQ(findSnap(document, {10.2, 0.0}, 1.0, SnapModes{false, true, true, true, false}),
              (SnapPoint{{30.0, 0.0}, SnapKind::Center}));
}

// --- Texts: regions with an insertion point (docs/adr/0013, point 6) ------

/// "Hello" 2.5 high at `position`: about 8.28 wide, from y - 0.77 to y + 3.29.
Text hello(const Vec2& position) {
    Text text;
    text.position = position;
    text.value = "Hello";
    return text;
}

TEST(TextQueries, PickInsideTheBoxTakesTheText) {
    Document document;
    const EntityId text = document.add(hello({0.0, 0.0}));
    EXPECT_EQ(pickEntity(document, {4.0, 1.0}, 0.0), text);
    EXPECT_EQ(pickEntity(document, {0.0, 0.0}, 0.0), text);
    // Outside, within or beyond the tolerance.
    EXPECT_EQ(pickEntity(document, {-0.4, 1.0}, 0.5), text);
    EXPECT_FALSE(pickEntity(document, {-0.6, 1.0}, 0.5));
}

TEST(TextQueries, PickOnOverlappingTextsTakesTheHighestId) {
    Document document;
    document.add(hello({0.0, 0.0}));
    const EntityId over = document.add(hello({1.0, 0.5}));
    EXPECT_EQ(pickEntity(document, {2.0, 1.0}, 0.0), over);
}

TEST(TextQueries, WindowAndCrossing) {
    Document document;
    const EntityId text = document.add(hello({0.0, 0.0}));
    const EntityId line = document.add(Line{{20.0, 0.0}, {30.0, 0.0}});
    EXPECT_EQ(
        values(selectInRectangle(document, BBox{{-1.0, -1.0}, {9.0, 4.0}}, RectangleMode::Window)),
        (std::vector<std::uint64_t>{text.value}));
    // Inside the text: the window takes nothing, the crossing takes the text.
    EXPECT_TRUE(
        selectInRectangle(document, BBox{{2.0, 0.0}, {6.0, 2.0}}, RectangleMode::Window).empty());
    EXPECT_EQ(
        values(selectInRectangle(document, BBox{{2.0, 0.0}, {6.0, 2.0}}, RectangleMode::Crossing)),
        (std::vector<std::uint64_t>{text.value}));
    EXPECT_EQ(values(selectInRectangle(document, BBox{{5.0, -1.0}, {25.0, 1.0}},
                                       RectangleMode::Crossing)),
              (std::vector<std::uint64_t>{text.value, line.value}));
}

TEST(TextQueries, SnapToTheInsertionPassingOverTheText) {
    Document document;
    document.add(hello({0.0, 0.0}));
    const SnapPoint insertion{{0.0, 0.0}, SnapKind::Insertion};
    EXPECT_EQ(findSnap(document, {4.0, 1.0}, 1.0, kAllModes), insertion);
    // Within the aperture of the box, from outside.
    EXPECT_EQ(findSnap(document, {4.0, 4.0}, 1.0, kAllModes), insertion);
    EXPECT_FALSE(findSnap(document, {4.0, 5.0}, 1.0, kAllModes));
}

TEST(TextQueries, AnEndpointOrMidpointWithinTheApertureBeatsTheInsertion) {
    Document document;
    document.add(hello({0.0, 0.0}));
    document.add(Line{{8.0, 0.0}, {15.0, 0.0}});
    EXPECT_EQ(findSnap(document, {7.9, 0.1}, 1.0, kAllModes),
              (SnapPoint{{8.0, 0.0}, SnapKind::Endpoint}));
    // With the endpoint mode off, the insertion point comes back.
    EXPECT_EQ(findSnap(document, {7.9, 0.1}, 1.0, SnapModes{false, true, true, true}),
              (SnapPoint{{0.0, 0.0}, SnapKind::Insertion}));
}

TEST(TextQueries, InsertionModeOff) {
    Document document;
    document.add(hello({0.0, 0.0}));
    EXPECT_FALSE(findSnap(document, {4.0, 1.0}, 1.0, SnapModes{true, true, true, false}));
    EXPECT_EQ(findSnap(document, {4.0, 1.0}, 1.0, SnapModes{false, false, false, true}),
              (SnapPoint{{0.0, 0.0}, SnapKind::Insertion}));
}

TEST(TextQueries, InsertionAndCentreHaveTheSamePriority) {
    Document document;
    document.add(Circle{{20.0, 1.0}, 10.0});  // passes through (10,1)
    document.add(hello({0.0, 0.0}));          // its box ends at about x = 8.28
    // 0.5 from the circle, 1.22 from the text box: the circle's centre.
    EXPECT_EQ(findSnap(document, {9.5, 1.0}, 2.0, kAllModes),
              (SnapPoint{{20.0, 1.0}, SnapKind::Center}));
    // Inside the text box, 2 from the circle: the insertion point.
    EXPECT_EQ(findSnap(document, {8.0, 1.0}, 2.0, kAllModes),
              (SnapPoint{{0.0, 0.0}, SnapKind::Insertion}));
}

// docs/tappe/0.8/PT13-mtext-core.md, point 6: the rows of an MTEXT give no
// snap point of their own, they lead to the insertion point of the MTEXT.
MText twoRows(const Vec2& insertion) {
    MText mtext;
    mtext.insertion = insertion;
    mtext.height = 3.0;
    mtext.value = R"(A\PB)";
    return mtext;
}

constexpr SnapModes kInsertionOnly{false, false, false, true};

TEST(MTextQueries, ARowLeadsToTheInsertionPointOfTheMText) {
    Document document;
    document.add(twoRows({30.0, 20.0}));
    const SnapPoint insertion{{30.0, 20.0}, SnapKind::Insertion};
    // On the box of the first row, whose baseline is at y = 17, never the
    // start of the row.
    EXPECT_EQ(findSnap(document, {31.0, 17.0}, 0.1, kInsertionOnly), insertion);
    EXPECT_EQ(findSnap(document, {31.0, 17.0}, 0.1, kAllModes), insertion);
    // On the second row, five below.
    EXPECT_EQ(findSnap(document, {31.0, 12.0}, 0.1, kInsertionOnly), insertion);
    // Farther than the aperture from every row and from the point: nothing.
    EXPECT_FALSE(findSnap(document, {40.0, 30.0}, 0.1, kInsertionOnly));
    EXPECT_FALSE(findSnap(document, {31.0, 17.0}, 0.1, SnapModes{true, true, true, false}));
}

TEST(MTextQueries, TheNearestRowDecidesAgainstAText) {
    Document document;
    document.add(twoRows({30.0, 20.0}));
    document.add(hello({33.0, 16.0}));  // its box starts at x = 33
    // Inside the first row, 0.5 from the text box: the MTEXT.
    EXPECT_EQ(findSnap(document, {32.5, 17.0}, 1.0, kInsertionOnly),
              (SnapPoint{{30.0, 20.0}, SnapKind::Insertion}));
    // Inside the text box, beyond the first row: the text.
    EXPECT_EQ(findSnap(document, {34.0, 17.0}, 1.0, kInsertionOnly),
              (SnapPoint{{33.0, 16.0}, SnapKind::Insertion}));
}

TEST(MTextQueries, AnEmptyMTextIsItsInsertionPoint) {
    Document document;
    MText empty;
    empty.insertion = {5.0, 5.0};
    empty.height = 2.5;
    document.add(empty);
    EXPECT_EQ(findSnap(document, {5.0, 5.0}, 0.1, kInsertionOnly),
              (SnapPoint{{5.0, 5.0}, SnapKind::Insertion}));
    EXPECT_FALSE(findSnap(document, {5.5, 5.0}, 0.1, kInsertionOnly));
}

// docs/tappe/0.8.1/PT8-impaginazione-formati.md, points 1 and 2: the pieces
// of a formatted MTEXT, a Bold one and an underlined one, give no snap point
// of their own, directly, in a block and in a dimension.
constexpr std::string_view kBoldAndUnderlined = R"({\fArial|b1;A} {\LA})";

SnapModes endMidIntNea() {
    return nearestAnd(SnapModes{true, true, false, false, true});
}

/// The generated pieces of `mtext` alone, as in the drawing.
std::vector<GeneratedItem> piecesOf(const MText& mtext) {
    Document document = Document::newDrawing();
    return generate(document, *document.find(document.add(mtext)));
}

Vec2 middleOf(const Line& line) {
    return (line.start + line.end) * 0.5;
}

template <typename T>
T pieceOf(const std::vector<GeneratedItem>& items) {
    for (const GeneratedItem& item : items) {
        if (const auto* shape = std::get_if<T>(&item.shape)) {
            return *shape;
        }
    }
    ADD_FAILURE() << "no piece of the expected shape";
    return T{};
}

MText boldAndUnderlined(const Vec2& insertion) {
    MText mtext;
    mtext.insertion = insertion;
    mtext.height = 2.5;
    mtext.value = std::string{kBoldAndUnderlined};
    return mtext;
}

TEST(FormattedMTextSnap, ADirectMTextLeadsToItsInsertionPointOnly) {
    Document document = Document::newDrawing();
    const MText mtext = boldAndUnderlined({0, 30});
    document.add(mtext);
    const std::vector<GeneratedItem> pieces = piecesOf(mtext);
    const auto bold = pieceOf<FormattedText>(pieces);
    const auto plain = pieceOf<Text>(pieces);
    const Line underline = pieceOf<TextLine>(pieces).line;
    const SnapPoint insertion{{0, 30}, SnapKind::Insertion};
    // On the Bold piece and on the underlined one, farther than the aperture
    // from the insertion point: the insertion point of the MTEXT.
    const Vec2 onBold = bold.text.position + Vec2{0.2, 1.0};
    ASSERT_GT(distance(onBold, insertion.point), 0.5);
    EXPECT_EQ(findSnap(document, onBold, 0.5, kInsertionOnly), insertion);
    EXPECT_EQ(findSnap(document, plain.position + Vec2{0.2, 1.0}, 0.5, kInsertionOnly), insertion);
    // No end, middle or Nearest of the underline.
    for (const Vec2& point : {underline.start, underline.end, middleOf(underline)}) {
        EXPECT_FALSE(findSnap(document, point, 0.5, endMidIntNea())) << point.x;
    }
}

TEST(FormattedMTextSnap, InABlockNoPointFromTheLinesNorFromAFormattedText) {
    Document document = Document::newDrawing();
    const MText mtext = boldAndUnderlined({0, 30});
    const std::vector<GeneratedItem> pieces = piecesOf(mtext);
    const Line underline = pieceOf<TextLine>(pieces).line;
    const Vec2 middle = middleOf(underline);
    // A line of the block crosses the underline in its middle.
    defineBlock(document, "Tag", {mtext, Line{middle - Vec2{0, 2}, middle + Vec2{0, 2}}});
    const Vec2 at{100, 0};
    document.add(insertOf("Tag", at));
    const auto bold = pieceOf<FormattedText>(pieces);
    const auto plain = pieceOf<Text>(pieces);
    // The Bold piece gives not even its insertion point; a Text gives its own.
    EXPECT_FALSE(findSnap(document, at + bold.text.position + Vec2{0.2, 1.0}, 0.5, kInsertionOnly));
    EXPECT_EQ(findSnap(document, at + plain.position + Vec2{0.2, 1.0}, 0.5, kInsertionOnly),
              (SnapPoint{at + plain.position, SnapKind::Insertion}));
    // No end or middle of the underline, no crossing with the line.
    for (const Vec2& point : {underline.start, underline.end}) {
        EXPECT_FALSE(findSnap(document, at + point, 0.5, endMidIntNea())) << point.x;
    }
    const SnapModes intersection{false, false, false, false, true};
    EXPECT_FALSE(findSnap(document, at + middle, 0.5, intersection));
    // Near the underline, away from the line: no Nearest.
    EXPECT_FALSE(findSnap(document, at + middle + Vec2{0.3, 0.05}, 0.2, endMidIntNea()));
}

TEST(FormattedMTextSnap, InADimensionTheInsertionPointOfItsText) {
    Document document = Document::newDrawing();
    Dimension dimension;
    dimension.definition.kind = DimensionKind::Linear;
    dimension.definition.point13 = {0, 0};
    dimension.definition.point14 = {40, 0};
    dimension.definition.linePoint = {40, 10};
    dimension.presentation.textOverride = std::string{kBoldAndUnderlined};
    std::string reason;
    const std::optional<Dimension> regenerable =
        regenerated(document, std::move(dimension), &reason);
    ASSERT_TRUE(regenerable.has_value()) << reason;
    const Dimension made = regenerable.value_or(Dimension{});
    document.add(made);
    MText text;
    for (const EntityRecord& record : made.graphics) {
        if (const auto* shown = std::get_if<MText>(&record.entity)) {
            text = *shown;
        }
    }
    const std::vector<GeneratedItem> pieces =
        generate(document, EntityRecord{EntityId{9}, Entity{text}});
    const auto bold = pieceOf<FormattedText>(pieces);
    const Line underline = pieceOf<TextLine>(pieces).line;
    const SnapPoint insertion{text.insertion, SnapKind::Insertion};
    const Vec2 onBold = bold.text.position + Vec2{0.2, 1.0};
    ASSERT_GT(distance(onBold, text.insertion), 0.5);
    EXPECT_EQ(findSnap(document, onBold, 0.5, kInsertionOnly), insertion);
    // The underline is 0.125 above the dimension line: within 0.1 nothing.
    for (const Vec2& point : {underline.start, underline.end, middleOf(underline)}) {
        EXPECT_FALSE(findSnap(document, point, 0.1, endMidIntNea())) << point.x;
    }
}

TEST(FormattedMTextSnap, AVisibleTextLineCountsForDistanceAndSelection) {
    // Underlined spaces: a TextLine without a text over it.
    Document document = Document::newDrawing();
    MText mtext;
    mtext.height = 1.0;
    mtext.value = R"(A{\L   })";
    const EntityId id = document.add(mtext);
    const EntityRecord& record = *document.find(id);
    const double a = textWidth(U"A");
    const double end = textWidth(U"A   ");
    const Vec2 onTheLine{(a + end) / 2.0, -1.2};
    EXPECT_EQ(distance(document, record, onTheLine), 0.0);
    EXPECT_EQ(selectInRectangle(document,
                                BBox{onTheLine - Vec2{0.05, 0.05}, onTheLine + Vec2{0.05, 0.05}},
                                RectangleMode::Crossing),
              std::vector<EntityId>{id});
    // A window around the A alone leaves the line out, and the MTEXT.
    EXPECT_TRUE(
        selectInRectangle(document, BBox{{-0.1, -2}, {a + 0.1, 1}}, RectangleMode::Window).empty());
    EXPECT_EQ(selectInRectangle(document, BBox{{-0.1, -2}, {end + 0.1, 1}}, RectangleMode::Window),
              std::vector<EntityId>{id});
}

TEST_F(RandomDocument, PickMatchesExhaustiveSearch) {
    int hits = 0;
    for (int i = 0; i < kQueryCount; ++i) {
        SCOPED_TRACE(i);
        const Vec2 point = generator().point(document());
        const double tolerance = generator().between(0.0, 4.0);
        const auto expected = exhaustivePick(document(), point, tolerance);
        EXPECT_EQ(value(pickEntity(document(), point, tolerance)), expected);
        hits += expected.has_value() ? 1 : 0;
    }
    EXPECT_GT(hits, kQueryCount / 10);  // the comparison must not be trivially empty
}

TEST_F(RandomDocument, RectanglesMatchExhaustiveSearch) {
    for (int i = 0; i < kQueryCount; ++i) {
        SCOPED_TRACE(i);
        const BBox area = generator().area();
        for (const RectangleMode mode : {RectangleMode::Window, RectangleMode::Crossing}) {
            EXPECT_EQ(values(selectInRectangle(document(), area, mode)),
                      exhaustiveSelect(document(), area, mode));
        }
    }
}

TEST_F(RandomDocument, SnapMatchesExhaustiveSearch) {
    int hits = 0;
    int centers = 0;
    for (int i = 0; i < kQueryCount; ++i) {
        SCOPED_TRACE(i);
        const Vec2 pointer = generator().point(document());
        const double aperture = generator().between(0.0, 4.0);
        const SnapModes modes = generator().modes();
        const auto expected = exhaustiveSnap(document(), pointer, aperture, modes);
        EXPECT_EQ(findSnap(document(), pointer, aperture, modes), expected);
        hits += expected.has_value() ? 1 : 0;
        centers += expected && expected->kind == SnapKind::Center ? 1 : 0;
    }
    EXPECT_GT(hits, kQueryCount / 10);
    EXPECT_GT(centers, 0);
}

// findSnapHit() (docs/tappe/0.8.1/PT31-prolungamento-parallelo.md, point 1;
// #992, [bloccante] 5).
TEST(FindSnapHit, TwoLinesWithACommonEndGiveTheHighestIdAndWhichEnd) {
    Document document;
    document.add(Line{{0, 0}, {10, 0}});
    const EntityId upward = document.add(Line{{0, 0}, {0, 10}});
    const SnapModes modes;
    const std::optional<SnapHit> maybeHit = findSnapHit(document, {0.5, 0.5}, 1.0, modes);
    ASSERT_TRUE(maybeHit);
    const SnapHit hit = maybeHit.value_or(SnapHit{});
    EXPECT_EQ(hit.snap, (SnapPoint{{0, 0}, SnapKind::Endpoint}));
    EXPECT_EQ(hit.entity, upward);
    EXPECT_EQ(hit.segment, std::nullopt);
    EXPECT_TRUE(hit.atStart);
    // findSnap() gives the same point and kind as before.
    EXPECT_EQ(findSnap(document, {0.5, 0.5}, 1.0, modes), hit.snap);

    const std::optional<SnapHit> maybeEnd = findSnapHit(document, {0.2, 9.5}, 1.0, modes);
    ASSERT_TRUE(maybeEnd);
    const SnapHit end = maybeEnd.value_or(SnapHit{});
    EXPECT_EQ(end.snap.point, (Vec2{0, 10}));
    EXPECT_FALSE(end.atStart);
}

TEST(FindSnapHit, TheEndsOfAnArcAndOfThePolylineSegments) {
    Document document;
    const EntityId arc = document.add(Arc{{0, 0}, 10, 0, kPi / 2});
    const EntityId polyline = document.add(Polyline{{{{100, 0}}, {{110, 0}}, {{110, 10}}}});
    const SnapModes modes;
    const std::optional<SnapHit> maybeStart = findSnapHit(document, {9.8, 0.3}, 1.0, modes);
    ASSERT_TRUE(maybeStart);
    const SnapHit start = maybeStart.value_or(SnapHit{});
    EXPECT_EQ(start.entity, arc);
    EXPECT_TRUE(start.atStart);
    const std::optional<SnapHit> maybeEnd = findSnapHit(document, {0.3, 9.8}, 1.0, modes);
    ASSERT_TRUE(maybeEnd);
    const SnapHit end = maybeEnd.value_or(SnapHit{});
    EXPECT_FALSE(end.atStart);

    // The inner vertex is the end of the first segment, the earliest.
    const std::optional<SnapHit> maybeInner = findSnapHit(document, {109.8, 0.3}, 1.0, modes);
    ASSERT_TRUE(maybeInner);
    const SnapHit inner = maybeInner.value_or(SnapHit{});
    EXPECT_EQ(inner.entity, polyline);
    EXPECT_EQ(inner.segment, 0U);
    EXPECT_FALSE(inner.atStart);
    const std::optional<SnapHit> maybeLast = findSnapHit(document, {110.3, 9.8}, 1.0, modes);
    ASSERT_TRUE(maybeLast);
    const SnapHit last = maybeLast.value_or(SnapHit{});
    EXPECT_EQ(last.segment, 1U);
    EXPECT_FALSE(last.atStart);
    const std::optional<SnapHit> maybeFirst = findSnapHit(document, {100.3, 0.2}, 1.0, modes);
    ASSERT_TRUE(maybeFirst);
    const SnapHit first = maybeFirst.value_or(SnapHit{});
    EXPECT_EQ(first.segment, 0U);
    EXPECT_TRUE(first.atStart);
}

TEST(FindSnapHit, OtherPointsGiveOnlyTheEntity) {
    Document document;
    const EntityId line = document.add(Line{{0, 0}, {10, 0}});
    const std::optional<SnapHit> maybeMiddle = findSnapHit(document, {5, 0.2}, 1.0, SnapModes{});
    ASSERT_TRUE(maybeMiddle);
    const SnapHit middle = maybeMiddle.value_or(SnapHit{});
    EXPECT_EQ(middle.snap.kind, SnapKind::Midpoint);
    EXPECT_EQ(middle.entity, line);
    EXPECT_FALSE(middle.atStart);
    EXPECT_EQ(findSnapHit(document, {50, 50}, 1.0, SnapModes{}), std::nullopt);
}

}  // namespace
}  // namespace tg::core
