// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/generated.hpp"

#include "tg/core/effective_properties.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/proximity.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/transform2d.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <numbers>
#include <optional>
#include <type_traits>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();

/// The closed axis-aligned rectangle with corners `a` and `b`.
Polyline rectangle(const Vec2& a, const Vec2& b) {
    return Polyline{{{a}, {{b.x, a.y}}, {b}, {{a.x, b.y}}}, true};
}

/// A drawing with the five entities of before the 0.8 on a layer of their own.
struct Drawing {
    Document document = Document::newDrawing();
    LayerId layer;
    std::vector<EntityId> ids;

    Drawing() {
        Layer own;
        own.name = "Own";
        own.color = Color::fromIndex(3);
        own.linetype = document.continuousLinetype();
        own.lineweight = 50;
        layer = document.addLayer(own);
        CurrentProperties current = document.current();
        current.layer = layer;
        document.setCurrent(current);
        Text text;
        text.position = {0, 20};
        text.height = 2.0;
        text.value = "abc";
        ids = {document.add(Line{{0, 0}, {10, 0}}), document.add(Arc{{0, 0}, 5.0, 0.0, 2.0}),
               document.add(Circle{{15, 15}, 3.0}),
               document.add(Polyline{{{{20, 0}, 0.5}, {{30, 0}}, {{30, 5}}}, false}),
               document.add(text)};
    }

    [[nodiscard]] const EntityRecord& record(std::size_t index) const {
        return *document.find(ids.at(index));
    }

    void setLayer(bool on, bool frozen) {
        Layer changed = *document.layer(layer);
        changed.on = on;
        changed.frozen = frozen;
        document.replaceLayer(changed);
    }
};

void expectSameProperties(const EffectiveProperties& actual, const EffectiveProperties& expected) {
    EXPECT_EQ(actual.color, expected.color);
    EXPECT_EQ(actual.colorIsSeven, expected.colorIsSeven);
    EXPECT_EQ(actual.linetype, expected.linetype);
    EXPECT_EQ(actual.linetypeScale, expected.linetypeScale);
    EXPECT_EQ(actual.lineweight, expected.lineweight);
    EXPECT_EQ(actual.visible, expected.visible);
    EXPECT_EQ(actual.locked, expected.locked);
}

void expectSameBox(const BBox& actual, const BBox& expected) {
    ASSERT_EQ(actual.isEmpty(), expected.isEmpty());
    if (!expected.isEmpty()) {
        EXPECT_EQ(actual.min(), expected.min());
        EXPECT_EQ(actual.max(), expected.max());
    }
}

const std::vector<Vec2>& probePoints() {
    static const std::vector<Vec2> kPoints{{5, 0},   {5, 1},  {0, 5},     {15, 18}, {15, 15},
                                           {25, -1}, {1, 21}, {100, 100}, {-3, 4},  {22, 2}};
    return kPoints;
}

const std::vector<BBox>& probeRectangles() {
    static const std::vector<BBox> kRectangles{
        {{-1, -1}, {11, 1}},  {{4, -1}, {6, 1}},      {{-6, -6}, {6, 6}},
        {{11, 11}, {19, 19}}, {{14, 14}, {16, 16}},   {{19, -5}, {31, 6}},
        {{-1, 19}, {9, 24}},  {{0.5, 20.5}, {1, 21}}, {{50, 50}, {60, 60}}};
    return kRectangles;
}

// --- The five entities of before the 0.8 ------------------------------------

TEST(Generated, TheFiveEntitiesGenerateThemselvesWithTheirProperties) {
    const Drawing drawing;
    for (std::size_t i = 0; i < drawing.ids.size(); ++i) {
        const EntityRecord& record = drawing.record(i);
        const std::vector<GeneratedItem> items = generate(drawing.document, record);
        ASSERT_EQ(items.size(), 1U) << i;
        EXPECT_EQ(shapeEntity(items.front().shape), std::optional<Entity>{record.entity}) << i;
        expectSameProperties(items.front().properties,
                             effectiveProperties(drawing.document, record));
    }
}

TEST(Generated, TheFiveEntitiesAnswerAsTheirOwnFunctions) {
    const Drawing drawing;
    for (std::size_t i = 0; i < drawing.ids.size(); ++i) {
        const EntityRecord& record = drawing.record(i);
        expectSameBox(extent(drawing.document, record), boundingBox(record.entity));
        for (const Vec2& point : probePoints()) {
            EXPECT_EQ(distance(drawing.document, record, point), distance(record.entity, point))
                << i;
        }
        for (const BBox& box : probeRectangles()) {
            EXPECT_EQ(isInside(drawing.document, record, box), isInside(record.entity, box)) << i;
            EXPECT_EQ(intersects(drawing.document, record, box), intersects(record.entity, box))
                << i;
        }
        SnapModes all;
        all.perpendicular = true;
        all.tangent = true;
        all.quadrant = true;
        for (const std::optional<Vec2>& from :
             {std::optional<Vec2>{}, std::optional<Vec2>{{3, 9}}}) {
            std::vector<SnapPoint> expected;
            appendSnapPoints(record.entity, all, expected, from);
            std::vector<SnapPoint> actual;
            appendSnapPoints(drawing.document, record, all, actual, from);
            EXPECT_EQ(actual, expected) << i;
        }
    }
}

TEST(Generated, TheFiveEntitiesTransformAsTransformed) {
    const Drawing drawing;
    const std::optional<Similarity> mirror = Similarity::reflection({0, 0}, {1, 1});
    const std::optional<Similarity> scaling = Similarity::scaling(2.5, {4, 0});
    ASSERT_TRUE(mirror.has_value());
    ASSERT_TRUE(scaling.has_value());
    const std::vector<Similarity> similarities{
        Similarity::translation({3, -2}), Similarity::rotation(0.75, {1, 1}),
        scaling.value_or(Similarity{}), mirror.value_or(Similarity{})};
    for (std::size_t i = 0; i < drawing.ids.size(); ++i) {
        const Entity& entity = drawing.record(i).entity;
        for (const Similarity& similarity : similarities) {
            for (const MirrorText mode : {MirrorText::Readable, MirrorText::Mirrored}) {
                const TransformOutcome outcome =
                    transformedIn(drawing.document, entity, similarity, mode);
                EXPECT_EQ(outcome.entity,
                          std::optional<Entity>{transformed(entity, similarity, mode)})
                    << i;
                EXPECT_TRUE(outcome.refusal.empty());
            }
        }
        const std::vector<Vec2> grips = gripPoints(entity);
        for (std::size_t grip = 0; grip < grips.size(); ++grip) {
            const Vec2 target = grips[grip] + Vec2{0.5, 0.25};
            const GripResult expected = moveGrip(entity, grip, target);
            const GripResult actual = moveGripIn(drawing.document, entity, grip, target);
            EXPECT_EQ(actual.outcome, expected.outcome) << i << ' ' << grip;
            EXPECT_EQ(actual.entity, expected.entity) << i << ' ' << grip;
            EXPECT_TRUE(actual.refusal.empty());
        }
    }
}

TEST(Generated, NothingIsGeneratedOnALayerOffOrFrozenAndTheExtentStays) {
    Drawing drawing;
    for (const bool frozen : {false, true}) {
        drawing.setLayer(/*on=*/frozen, frozen);  // off, then on but frozen
        for (std::size_t i = 0; i < drawing.ids.size(); ++i) {
            const EntityRecord& record = drawing.record(i);
            EXPECT_TRUE(generate(drawing.document, record).empty()) << i;
            // The extent of the five entities does not depend on the layer.
            expectSameBox(extent(drawing.document, record), boundingBox(record.entity));
        }
    }
}

// --- Items: what the types of the 0.8 build on -------------------------------

/// The convention of generateOf() (insert.hpp and the others): a shape that is
/// not drawn (a layer that is off) is in the extent but answers nothing; one
/// on a frozen layer is not there at all. The test of an insert whose line
/// lies on a layer that is off arrives with the package of the blocks
/// (docs/tappe/0.8/PT16-blocchi-core.md).
TEST(Generated, AShapeThatIsNotDrawnCountsOnlyForTheExtent) {
    EffectiveProperties drawn;
    EffectiveProperties hidden;
    hidden.visible = false;
    const std::vector<GeneratedItem> items{
        {Line{{0, 0}, {10, 0}}, hidden},
        {Circle{{5, 5}, 1.0}, drawn},
    };
    expectSameBox(extentOfItems(items), BBox{{0, 0}, {10, 6}});
    EXPECT_EQ(distanceToDrawnItems(items, {5, 0}), 4.0);
    EXPECT_TRUE(drawnItemsInside(items, {{3, 3}, {7, 7}}));
    EXPECT_FALSE(drawnItemsIntersect(items, {{-1, -1}, {11, 1}}));
    EXPECT_TRUE(drawnItemsIntersect(items, {{5, 5.5}, {7, 7}}));

    const std::vector<GeneratedItem> onlyHidden{items.front()};
    expectSameBox(extentOfItems(onlyHidden), BBox{{0, 0}, {10, 0}});
    EXPECT_EQ(distanceToDrawnItems(onlyHidden, {5, 0}), kInfinity);
    EXPECT_FALSE(drawnItemsInside(onlyHidden, {{-1, -1}, {11, 1}}));
    EXPECT_FALSE(drawnItemsIntersect(onlyHidden, {{-1, -1}, {11, 1}}));

    EXPECT_TRUE(extentOfItems({}).isEmpty());
    EXPECT_FALSE(drawnItemsInside({}, {{-1, -1}, {1, 1}}));
}

// --- Shapes ------------------------------------------------------------------

TEST(Generated, ShapesAnswerAsTheEntitiesOfTheSameType) {
    const std::vector<Entity> entities{Line{{0, 0}, {10, 0}}, Arc{{0, 0}, 5.0, 0.0, 2.0},
                                       Circle{{15, 15}, 3.0}, rectangle({20, 0}, {30, 5})};
    for (const Entity& entity : entities) {
        const GeneratedShape shape = std::visit(
            [](const auto& value) -> GeneratedShape {
                if constexpr (std::is_constructible_v<GeneratedShape, decltype(value)>) {
                    return value;
                } else {
                    return Polyline{};
                }
            },
            entity);
        expectSameBox(shapeBoundingBox(shape), boundingBox(entity));
        for (const Vec2& point : probePoints()) {
            EXPECT_EQ(shapeDistance(shape, point), distance(entity, point));
        }
        for (const BBox& box : probeRectangles()) {
            EXPECT_EQ(shapeIsInside(shape, box), isInside(entity, box));
            EXPECT_EQ(shapeIntersects(shape, box), intersects(entity, box));
        }
        std::vector<SnapPoint> expected;
        appendSnapPoints(entity, SnapModes{}, expected);
        std::vector<SnapPoint> actual;
        appendShapeSnapPoints(shape, SnapModes{}, actual, std::nullopt);
        EXPECT_EQ(actual, expected);
    }
}

TEST(Generated, TransformedShapeTakesSimilaritiesThroughTransformed) {
    const Line line{{1, 0}, {3, 0}};
    const Arc arc{{0, 0}, 2.0, 0.0, 1.0};
    // A rotation by a quarter turn.
    const Transform2D quarter{0.0, 1.0, -1.0, 0.0, 0.0, 0.0};
    const std::vector<GeneratedShape> lines = transformedShape(line, quarter);
    ASSERT_EQ(lines.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<Line>(lines.front()));
    EXPECT_NEAR(std::get<Line>(lines.front()).start.x, 0.0, 1e-15);
    EXPECT_NEAR(std::get<Line>(lines.front()).start.y, 1.0, 1e-15);
    EXPECT_NEAR(std::get<Line>(lines.front()).end.x, 0.0, 1e-15);
    EXPECT_NEAR(std::get<Line>(lines.front()).end.y, 3.0, 1e-15);

    // A reflection across X, a scale and a translation: an arc stays an arc.
    const Transform2D reflection{2.0, 0.0, 0.0, -2.0, 5.0, 1.0};
    const std::vector<GeneratedShape> arcs = transformedShape(arc, reflection);
    ASSERT_EQ(arcs.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<Arc>(arcs.front()));
    const Arc& mirrored = std::get<Arc>(arcs.front());
    EXPECT_EQ(mirrored.center, (Vec2{5, 1}));
    EXPECT_EQ(mirrored.radius, 4.0);
    EXPECT_NEAR(mirrored.startAngle, (2.0 * std::numbers::pi) - 1.0, 1e-12);
    EXPECT_NEAR(mirrored.endAngle, 0.0, 1e-12);

    // The identity gives the shape itself.
    const std::vector<GeneratedShape> same = transformedShape(arc, Transform2D{});
    ASSERT_EQ(same.size(), 1U);
    EXPECT_EQ(std::get<Arc>(same.front()), std::get<Arc>(transformed(arc, Similarity{})));
}

TEST(Generated, OtherAffineMapsWaitForTheirPackages) {
    // docs/tappe/0.8: PT48 (circles, arcs, ellipses), PT17 (text), PT16 (the rest).
    const Transform2D stretch = Transform2D::scaling(2.0, 1.0);
    EXPECT_TRUE(transformedShape(Line{{0, 0}, {1, 1}}, stretch).empty());
    EXPECT_TRUE(transformedShape(Circle{{0, 0}, 1.0}, stretch).empty());
    // A degenerate map is not a similarity either.
    EXPECT_TRUE(
        transformedShape(Line{{0, 0}, {1, 1}}, Transform2D{0.0, 0.0, 0.0, 0.0, 1.0, 1.0}).empty());
}

TEST(Generated, AFilledRegionKeepsTheMapComposedWithItsOwn) {
    const FilledRegion region{{rectangle({0, 0}, {1, 1})}, Transform2D::scaling(2.0, 1.0)};
    const std::vector<GeneratedShape> moved =
        transformedShape(region, Transform2D::translation({10, 0}));
    ASSERT_EQ(moved.size(), 1U);
    const auto& result = std::get<FilledRegion>(moved.front());
    EXPECT_EQ(result.loops, region.loops);
    EXPECT_EQ(result.transform.apply({1, 1}), (Vec2{12, 1}));
}

/// A square from 0 to 10 with a square hole from 3 to 7.
FilledRegion squareWithHole() {
    return FilledRegion{{rectangle({0, 0}, {10, 10}), rectangle({3, 3}, {7, 7})}, Transform2D{}};
}

TEST(Generated, AFilledRegionIsFilledWithTheEvenOddRule) {
    const FilledRegion region = squareWithHole();
    EXPECT_EQ(shapeDistance(region, {1, 1}), 0.0);
    EXPECT_EQ(shapeDistance(region, {8.5, 5}), 0.0);
    EXPECT_EQ(shapeDistance(region, {5, 5}), 2.0);  // in the hole: to its border
    EXPECT_EQ(shapeDistance(region, {12, 5}), 2.0);
    expectSameBox(shapeBoundingBox(region), BBox{{0, 0}, {10, 10}});

    EXPECT_TRUE(shapeIsInside(region, {{-1, -1}, {11, 11}}));
    EXPECT_FALSE(shapeIsInside(region, {{-1, -1}, {9, 11}}));
    EXPECT_TRUE(shapeIntersects(region, {{1, 1}, {2, 2}}));   // in the fill
    EXPECT_FALSE(shapeIntersects(region, {{4, 4}, {6, 6}}));  // in the hole
    EXPECT_TRUE(shapeIntersects(region, {{6, 4}, {8, 6}}));   // across the hole's border
    EXPECT_FALSE(shapeIntersects(region, {{11, 11}, {12, 12}}));

    std::vector<SnapPoint> points;
    appendShapeSnapPoints(region, SnapModes{}, points, std::nullopt);
    EXPECT_TRUE(points.empty());
    EXPECT_EQ(shapeEntity(region), std::nullopt);
}

TEST(Generated, AFilledRegionWithArcsIsFilledInsideTheArcs) {
    // A half disc of radius 2 below the X axis: from (-2,0) to (2,0) with a
    // counter-clockwise half circle, then back along the diameter.
    const FilledRegion region{{Polyline{{{{-2, 0}, 1.0}, {{2, 0}}}, true}}, Transform2D{}};
    EXPECT_EQ(shapeDistance(region, {0, -1}), 0.0);
    EXPECT_EQ(shapeDistance(region, {0, 1}), 1.0);
    EXPECT_NEAR(shapeDistance(region, {0, -3}), 1.0, 1e-12);
    EXPECT_GT(shapeDistance(region, {1.9, -1.9}), 0.0);
}

TEST(Generated, AFilledRegionTakesATinyBulgeAsStraight) {
    // docs/adr/0036, point 7: below the threshold of PolylineSegment::isArc().
    const FilledRegion region{{Polyline{{{{0, 0}, 1e-13}, {{10, 0}}, {{10, 10}}, {{0, 10}}}, true}},
                              Transform2D{}};
    EXPECT_EQ(shapeDistance(region, {5, 5}), 0.0);
    EXPECT_EQ(shapeDistance(region, {5, 12}), 2.0);
}

TEST(Generated, APointOnTheChordOfAnArcIsFilled) {
    // A rectangle below the X axis closed above by a half circle on its top
    // side, in both directions: (0,0) lies on the chord, inside the region.
    const std::vector<Polyline> loops{
        Polyline{{{{2, 0}, 1.0}, {{-2, 0}}, {{-2, -2}}, {{2, -2}}}, true},
        Polyline{{{{2, -2}}, {{-2, -2}}, {{-2, 0}, -1.0}, {{2, 0}}}, true}};
    for (const Polyline& loop : loops) {
        const FilledRegion region{{loop}, Transform2D{}};
        EXPECT_EQ(shapeDistance(region, {0, 0}), 0.0);
        EXPECT_EQ(shapeDistance(region, {1, 0}), 0.0);
        EXPECT_EQ(shapeDistance(region, {0, 1}), 0.0);
        EXPECT_EQ(shapeDistance(region, {0, -1}), 0.0);
        EXPECT_TRUE(shapeIntersects(region, {{0, 0}, {0.25, 0.25}}));
        EXPECT_TRUE(shapeIntersects(region, {{-0.25, -0.25}, {0, 0}}));
        EXPECT_NEAR(shapeDistance(region, {0, 3}), 1.0, 1e-12);
        EXPECT_EQ(shapeDistance(region, {3, -1}), 1.0);
    }
}

TEST(Generated, APointOnASlantedChordIsFilledInBothDirections) {
    // The chord from (0,0) to (3,11), closed by a half circle outside the
    // quadrilateral: p lies on it, inside the region, in both directions.
    const std::vector<Polyline> loops{
        Polyline{{{{0, 0}, -1.0}, {{3, 11}}, {{14, 8}}, {{11, -3}}}, true},
        Polyline{{{{11, -3}}, {{14, 8}}, {{3, 11}, 1.0}, {{0, 0}}}, true}};
    const Vec2 p{15.0 / 11.0, 5.0};
    for (const Polyline& loop : loops) {
        const FilledRegion region{{loop}, Transform2D{}};
        EXPECT_EQ(shapeDistance(region, p), 0.0);
        EXPECT_TRUE(shapeIntersects(region, {p, p + Vec2{0.01, 0.01}}));
        EXPECT_EQ(shapeDistance(region, {7, 4}), 0.0);
        EXPECT_EQ(shapeDistance(region, {-1, 6}), 0.0);  // under the arc
        EXPECT_GT(shapeDistance(region, {20, 20}), 0.0);
    }
}

TEST(Generated, AFilledRegionUnderAMapAnswersInDrawingCoordinates) {
    FilledRegion region = squareWithHole();
    region.transform = Transform2D::translation({100, 0});
    EXPECT_EQ(shapeDistance(region, {101, 1}), 0.0);
    EXPECT_EQ(shapeDistance(region, {105, 5}), 2.0);
    expectSameBox(shapeBoundingBox(region), BBox{{100, 0}, {110, 10}});

    // A stretch: the fill follows it at once; its border waits for PT16.
    region.transform = Transform2D::scaling(2.0, 1.0);
    EXPECT_EQ(shapeDistance(region, {19, 1}), 0.0);
    expectSameBox(shapeBoundingBox(region), BBox{{0, 0}, {20, 10}});
}

}  // namespace
}  // namespace tg::core
