// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/insert.hpp"

#include "tg/core/document.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"

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

/// An entity of a block, with the default properties of a new drawing.
EntityRecord member(const Document& document, Entity entity) {
    EntityRecord record{EntityId{}, std::move(entity)};
    record.properties.layer = document.layerZero();
    record.properties.linetype = document.byLayerLinetype();
    return record;
}

BlockDefinition block(const Document& document, std::string name, const Vec2& base,
                      std::vector<Entity> entities) {
    BlockDefinition definition{std::move(name)};
    definition.base = base;
    for (Entity& entity : entities) {
        definition.entities.push_back(member(document, std::move(entity)));
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

std::vector<Line> linesOf(const std::vector<GeneratedItem>& items) {
    std::vector<Line> lines;
    for (const GeneratedItem& item : items) {
        const auto* line = std::get_if<Line>(&item.shape);
        EXPECT_NE(line, nullptr);
        if (line != nullptr) {
            lines.push_back(*line);
        }
    }
    return lines;
}

void expectLine(const Line& actual, const Vec2& start, const Vec2& end) {
    expectPoint(actual.start, start);
    expectPoint(actual.end, end);
}

/// The block "TG_EL" of the reference cases: base (5, 0), a horizontal and a
/// vertical line.
Document drawingWithTgEl() {
    Document document = Document::newDrawing();
    document.replaceBlock(
        "TG_EL", block(document, "TG_EL", {5, 0}, {Line{{0, 0}, {10, 0}}, Line{{10, 0}, {10, 4}}}));
    return document;
}

std::vector<Line> generatedLines(Document& document, const Insert& insert) {
    const EntityId id = document.add(insert);
    return linesOf(generate(document, *document.find(id)));
}

TEST(Insert, SupportedNeedsScalesThatAreNotZero) {
    EXPECT_TRUE(supported(insertOf("A", {0, 0})));
    EXPECT_FALSE(supported(insertOf("A", {0, 0}, 0.0, 1.0)));
    EXPECT_FALSE(supported(insertOf("A", {0, 0}, 1.0, 0.0)));
    // The attributes arrive with their package (PT17).
    Insert withAttribute = insertOf("A", {0, 0});
    withAttribute.attributes.emplace_back();
    EXPECT_FALSE(supported(withAttribute));
}

TEST(Insert, TransformSubtractsTheBaseScalesRotatesAndMoves) {
    const Document document = drawingWithTgEl();
    const Insert insert = insertOf("TG_EL", {100, 50}, 2.0, 1.0, 30.0);
    const Transform2D map = insertTransform(document, insert);
    expectPoint(map.apply({0, 0}), {91.33974596215561, 45});
    expectPoint(map.apply({5, 0}), {100, 50});
    EXPECT_FALSE(isSimilarity(insert));
    EXPECT_TRUE(isSimilarity(insertOf("TG_EL", {0, 0}, -2.0, 2.0)));
    // Without the definition the base point is the origin.
    expectPoint(insertTransform(document, insertOf("Missing", {1, 2})).apply({0, 0}), {1, 2});
}

TEST(Insert, GeneratedGeometryWithScalesAndRotation) {
    Document document = drawingWithTgEl();
    const auto unit = generatedLines(document, insertOf("TG_EL", {10, 20}));
    ASSERT_EQ(unit.size(), 2U);
    expectLine(unit[0], {5, 20}, {15, 20});
    expectLine(unit[1], {15, 20}, {15, 24});

    const auto doubled = generatedLines(document, insertOf("TG_EL", {10, 20}, 2.0, 2.0));
    ASSERT_EQ(doubled.size(), 2U);
    expectLine(doubled[0], {0, 20}, {20, 20});
    expectLine(doubled[1], {20, 20}, {20, 28});

    const auto negative = generatedLines(document, insertOf("TG_EL", {10, 20}, -1.0, -1.0));
    ASSERT_EQ(negative.size(), 2U);
    expectLine(negative[0], {15, 20}, {5, 20});
    expectLine(negative[1], {5, 20}, {5, 16});

    const auto stretched = generatedLines(document, insertOf("TG_EL", {10, 20}, 2.0, 1.0));
    ASSERT_EQ(stretched.size(), 2U);
    expectLine(stretched[0], {0, 20}, {20, 20});
    expectLine(stretched[1], {20, 20}, {20, 24});

    const auto rotated = generatedLines(document, insertOf("TG_EL", {100, 50}, 2.0, 1.0, 30.0));
    ASSERT_EQ(rotated.size(), 2U);
    expectLine(rotated[0], {91.33974596215561, 45}, {108.66025403784439, 55});
    expectLine(rotated[1], {108.66025403784439, 55}, {106.66025403784439, 58.46410161513776});
}

TEST(Insert, NestedBlocksComposeTheirMaps) {
    Document document = Document::newDrawing();
    document.replaceBlock("Leaf", block(document, "Leaf", {2, 3}, {Line{{2, 3}, {6, 3}}}));
    document.replaceBlock("Parent",
                          block(document, "Parent", {5, 7},
                                {insertOf("Leaf", {5, 7}, 2.0, 1.0, 90.0), Line{{5, 7}, {5, 9}}}));
    const auto lines = generatedLines(document, insertOf("Parent", {100, 200}, -1.0, 2.0, 90.0));
    ASSERT_EQ(lines.size(), 2U);
    expectLine(lines[0], {100, 200}, {84, 200});
    expectLine(lines[1], {100, 200}, {96, 200});
}

TEST(Insert, ACircleUnderTwoScalingsThatCancelStaysACircle) {
    // docs/DECISIONI.md, 2026-11-14, "Revisione delle interfacce PT4", P18.
    Document document = Document::newDrawing();
    document.replaceBlock("B", block(document, "B", {0, 0}, {Circle{{3, 4}, 2}}));
    document.replaceBlock("A", block(document, "A", {0, 0}, {insertOf("B", {0, 0}, 0.5, 1.0)}));
    const EntityId restored = document.add(insertOf("A", {0, 0}, 2.0, 1.0));
    const auto items = generate(document, *document.find(restored));
    ASSERT_EQ(items.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<Circle>(items[0].shape));
    EXPECT_EQ(std::get<Circle>(items[0].shape), (Circle{{3, 4}, 2}));

    // Still affine: an ellipse.
    const EntityId affine = document.add(insertOf("A", {10, 20}, 4.0, 1.0));
    const auto stretched = generate(document, *document.find(affine));
    ASSERT_EQ(stretched.size(), 1U);
    const auto* ellipse = std::get_if<Ellipse>(&stretched[0].shape);
    ASSERT_NE(ellipse, nullptr);
    expectPoint(ellipse->center, {16, 24});
    expectPoint(ellipse->majorAxis, {4, 0});
    EXPECT_NEAR(ellipse->ratio, 0.5, kEps);
}

TEST(Insert, AStretchedPolylineWithAnArcSplitsIntoPiecesWithoutWidth) {
    // docs/DECISIONI.md, 2026-11-14, "Secondo giro delle revisioni PT4 e PT5", P8.
    Document document = Document::newDrawing();
    Polyline bent;
    bent.addVertex({6, 0});
    bent.addVertex({4, 0}, 1.0);
    bent.addVertex({0, 0});
    bent.setConstantWidth(2.0);
    Polyline straight;
    straight.addVertex({0, 5});
    straight.addVertex({2, 5});
    straight.addVertex({2, 7});
    straight.setConstantWidth(1.0);
    document.replaceBlock("Shapes", block(document, "Shapes", {0, 0}, {bent, straight}));
    const EntityId id = document.add(insertOf("Shapes", {10, 20}, 2.0, 1.0));
    const auto items = generate(document, *document.find(id));
    ASSERT_EQ(items.size(), 3U);
    const auto* line = std::get_if<Line>(&items[0].shape);
    ASSERT_NE(line, nullptr);
    expectLine(*line, {22, 20}, {18, 20});
    const auto* arc = std::get_if<Ellipse>(&items[1].shape);
    ASSERT_NE(arc, nullptr);
    expectPoint(arc->center, {14, 20});
    expectPoint(arc->majorAxis, {4, 0});
    EXPECT_NEAR(arc->ratio, 0.5, kEps);
    EXPECT_NEAR(arc->startParameter, 0.0, kEps);
    EXPECT_NEAR(arc->endParameter, kPi, kEps);
    // Without arcs, a polyline through the mapped vertices, without width.
    const auto* polyline = std::get_if<Polyline>(&items[2].shape);
    ASSERT_NE(polyline, nullptr);
    ASSERT_EQ(polyline->vertexCount(), 3U);
    expectPoint(polyline->vertices()[2].position, {14, 27});
    EXPECT_EQ(polyline->constantWidth(), 0.0);
    for (const PolylineVertex& vertex : polyline->vertices()) {
        EXPECT_EQ(vertex.startWidth, 0.0);
        EXPECT_EQ(vertex.endWidth, 0.0);
    }
}

TEST(Insert, SolidsAndFilledRegionsKeepTheAffineMap) {
    const Transform2D stretch = Transform2D::translation({10, 20}) * Transform2D::scaling(2.0, 1.0);
    const Solid solid{{Vec2{0, 0}, Vec2{2, 0}, Vec2{0, 2}, Vec2{2, 2}}};
    const auto solids = transformedShape(solid, stretch);
    ASSERT_EQ(solids.size(), 1U);
    EXPECT_EQ(std::get<Solid>(solids[0]),
              (Solid{{Vec2{10, 20}, Vec2{14, 20}, Vec2{10, 22}, Vec2{14, 22}}}));
    Polyline loop;
    loop.addVertex({0, 0});
    loop.addVertex({1, 0});
    loop.addVertex({1, 1});
    loop.setClosed(true);
    const auto regions = transformedShape(FilledRegion{{loop}, Transform2D{}}, stretch);
    ASSERT_EQ(regions.size(), 1U);
    expectPoint(std::get<FilledRegion>(regions[0]).transform.apply({1, 1}), {12, 21});
}

/// The drawing of the reference case "proprieta-annidate": "Leaf" inside
/// "Middle", with properties on layer 0, ByBlock, ByLayer and other layers.
struct NestedProperties {
    Document document = Document::newDrawing();
    LinetypeId dash;
    LayerId red;
    LayerId blue;
    LayerId off;
    LayerId frozen;

    NestedProperties() {
        Linetype linetype;
        linetype.name = "Dash";
        linetype.pattern.elements = {3.0, -1.0};
        dash = document.addLinetype(linetype);
        red = document.addLayer(layer("Red", 1, dash, 50));
        Layer locked = layer("Blue", 5, document.continuousLinetype(), 35);
        locked.locked = true;
        blue = document.addLayer(locked);
        Layer hidden = layer("Off", 2, document.continuousLinetype(), lineweight::kDefault);
        hidden.on = false;
        off = document.addLayer(hidden);
        Layer cold = layer("Frozen", 4, document.continuousLinetype(), lineweight::kDefault);
        cold.frozen = true;
        frozen = document.addLayer(cold);

        BlockDefinition leaf{"Leaf"};
        EntityRecord byBlock = member(document, Line{{0, 0}, {10, 0}});
        byBlock.properties.color = Color::byBlock();
        byBlock.properties.linetype = document.byBlockLinetype();
        byBlock.properties.lineweight = lineweight::kByBlock;
        leaf.entities.push_back(byBlock);
        leaf.entities.push_back(member(document, Line{{0, 2}, {10, 2}}));
        EntityRecord own = member(document, Line{{0, 4}, {10, 4}});
        own.properties.layer = blue;
        leaf.entities.push_back(own);
        EntityRecord hiddenLine = member(document, Line{{0, 6}, {10, 6}});
        hiddenLine.properties.layer = off;
        leaf.entities.push_back(hiddenLine);
        EntityRecord frozenLine = member(document, Line{{0, 8}, {10, 8}});
        frozenLine.properties.layer = frozen;
        leaf.entities.push_back(frozenLine);
        document.replaceBlock("Leaf", leaf);

        BlockDefinition middle{"Middle"};
        EntityRecord child = member(document, insertOf("Leaf", {0, 0}));
        child.properties.color = Color::byBlock();
        child.properties.linetype = document.byBlockLinetype();
        child.properties.lineweight = lineweight::kByBlock;
        middle.entities.push_back(child);
        document.replaceBlock("Middle", middle);
    }

    [[nodiscard]] static Layer layer(std::string name, int color, LinetypeId linetype, int weight) {
        Layer result;
        result.name = std::move(name);
        result.color = Color::fromIndex(color);
        result.linetype = linetype;
        result.lineweight = weight;
        return result;
    }

    /// An insert of "Middle" on `layer`, coloured 3, Dash, 0.7 mm.
    EntityId insertOn(LayerId layer, const Vec2& at) {
        const EntityId id = document.add(insertOf("Middle", at));
        document.setProperties(id, {layer, Color::fromIndex(3), dash, 70, 1.0});
        return id;
    }
};

TEST(Insert, LayerZeroAndByBlockFollowTheInsertThroughTwoLevels) {
    NestedProperties drawing;
    const EntityId id = drawing.insertOn(drawing.red, {100, 50});
    const Document& document = drawing.document;
    const auto shown = generate(document, *document.find(id));
    ASSERT_EQ(shown.size(), 3U);
    // ByBlock twice: the insert's colour 3, Dash and 0.7.
    EXPECT_EQ(shown[0].properties.color, (Rgb{0, 255, 0}));
    EXPECT_EQ(shown[0].properties.linetype->name, "Dash");
    EXPECT_EQ(shown[0].properties.lineweight, 70);
    // ByLayer on layer 0: the layer Red of the insert.
    EXPECT_EQ(shown[1].properties.color, (Rgb{255, 0, 0}));
    EXPECT_EQ(shown[1].properties.linetype->name, "Dash");
    EXPECT_EQ(shown[1].properties.lineweight, 50);
    EXPECT_FALSE(shown[1].properties.locked);
    // Its own layer Blue, locked.
    EXPECT_EQ(shown[2].properties.color, (Rgb{0, 0, 255}));
    EXPECT_EQ(shown[2].properties.linetype->name, "Continuous");
    EXPECT_EQ(shown[2].properties.lineweight, 35);
    EXPECT_TRUE(shown[2].properties.locked);
    expectLine(std::get<Line>(shown[2].shape), {100, 54}, {110, 54});

    // The line on the layer Off is measured, not drawn; the frozen one is
    // not generated at all.
    const auto all = generateOf(document, *document.find(id), insertOf("Middle", {100, 50}));
    ASSERT_EQ(all.size(), 4U);
    EXPECT_FALSE(all[3].properties.visible);
    const BBox box = extent(document, *document.find(id));
    expectPoint(box.max(), {110, 56});

    // propertiesInBlock() alone.
    const EntityProperties inner =
        propertiesInBlock(document, document.find(id)->properties, EntityProperties{});
    EXPECT_EQ(inner.layer, LayerId{});
    EntityProperties onZero;
    onZero.layer = document.layerZero();
    onZero.color = Color::byBlock();
    const EntityProperties resolved =
        propertiesInBlock(document, document.find(id)->properties, onZero);
    EXPECT_EQ(resolved.layer, drawing.red);
    EXPECT_EQ(resolved.color, Color::fromIndex(3));
}

TEST(Insert, AFrozenOrOffLayerOfTheInsertHidesEverything) {
    NestedProperties drawing;
    const EntityId frozen = drawing.insertOn(drawing.frozen, {300, 50});
    const EntityId off = drawing.insertOn(drawing.off, {200, 50});
    const Document& document = drawing.document;
    EXPECT_TRUE(generate(document, *document.find(frozen)).empty());
    EXPECT_TRUE(
        generateOf(document, *document.find(frozen), insertOf("Middle", {300, 50})).empty());
    EXPECT_TRUE(extent(document, *document.find(frozen)).isEmpty());
    EXPECT_TRUE(generate(document, *document.find(off)).empty());
    // Off: measured, so the extent is there.
    EXPECT_FALSE(extent(document, *document.find(off)).isEmpty());
    EXPECT_FALSE(pickEntity(document, {305, 50}, 1.0));
    EXPECT_FALSE(pickEntity(document, {205, 50}, 1.0));
    EXPECT_TRUE(selectInRectangle(document, {{199, 49}, {311, 59}}, RectangleMode::Window).empty());
    const SnapModes insertion{false, false, false, true, false, false, false, false};
    EXPECT_FALSE(findSnap(document, {200, 50}, 1.0, insertion));
    EXPECT_FALSE(findSnap(document, {300, 50}, 1.0, insertion));
}

TEST(Insert, MirrorFlipsTheXScaleAndTurnsTheRotation) {
    // docs/adr/0045, point 4: rotation = 2φ − r + 180°, X scale opposite.
    Document document = Document::newDrawing();
    document.replaceBlock("Line", block(document, "Line", {0, 0}, {Line{{0, 0}, {10, 0}}}));
    const Insert insert = insertOf("Line", {10, 20}, 2.0, 1.0, 30.0);

    struct Axis {
        Vec2 a;
        Vec2 b;
        Vec2 insertion;
        double degrees{0.0};
    };

    for (const Axis& axis :
         {Axis{{0, 0}, {10, 0}, {10, -20}, 150.0}, Axis{{0, 0}, {0, 10}, {-10, 20}, 330.0},
          Axis{{0, 0}, {10, 10}, {20, 10}, 240.0}}) {
        ASSERT_TRUE(Similarity::reflection(axis.a, axis.b));
        const Similarity mirror = Similarity::reflection(axis.a, axis.b).value_or(Similarity{});
        const TransformOutcome outcome =
            transformedIn(document, Entity{insert}, mirror, MirrorText::Readable);
        ASSERT_TRUE(outcome.entity);
        const auto result = std::get<Insert>(outcome.entity.value_or(Entity{Insert{}}));
        expectPoint(result.insertion, axis.insertion);
        EXPECT_EQ(result.scaleX, -2.0);
        EXPECT_EQ(result.scaleY, 1.0);
        EXPECT_NEAR(radiansToDegrees(result.rotation), axis.degrees, kEps);

        // The geometry of the result is the mirror image of the original's.
        const auto before = generatedLines(document, insert);
        const auto after = generatedLines(document, result);
        ASSERT_EQ(before.size(), 1U);
        ASSERT_EQ(after.size(), 1U);
        expectLine(after[0], mirror.apply(before[0].start), mirror.apply(before[0].end));
    }
    // A negative X scale flips back to positive.
    const Similarity acrossX = Similarity::reflection({0, 0}, {10, 0}).value_or(Similarity{});
    const auto back =
        std::get<Insert>(transformed(Entity{insertOf("Line", {1, 2}, -2.0, 1.0, 270.0)}, acrossX));
    EXPECT_EQ(back.scaleX, 2.0);
    EXPECT_NEAR(radiansToDegrees(back.rotation), 270.0, kEps);
}

TEST(Insert, MoveRotateAndScaleChangeTheInsertData) {
    const Insert insert = insertOf("Line", {10, 20}, 2.0, 1.0, 30.0);
    const auto moved =
        std::get<Insert>(transformed(Entity{insert}, Similarity::translation({5, -7})));
    expectPoint(moved.insertion, {15, 13});
    const auto turned =
        std::get<Insert>(transformed(Entity{insert}, Similarity::rotation(kPi / 2.0, {0, 0})));
    expectPoint(turned.insertion, {-20, 10});
    EXPECT_NEAR(radiansToDegrees(turned.rotation), 120.0, kEps);
    const Similarity doubled = Similarity::scaling(2.0, {5, 10}).value_or(Similarity{});
    const auto scaled = std::get<Insert>(transformed(Entity{insert}, doubled));
    expectPoint(scaled.insertion, {15, 30});
    EXPECT_EQ(scaled.scaleX, 4.0);
    EXPECT_EQ(scaled.scaleY, 2.0);
}

TEST(Insert, OneGripOnTheInsertionPoint) {
    const Insert insert = insertOf("Line", {10, 20}, 2.0, 1.0, 30.0);
    EXPECT_EQ(gripPoints(Entity{insert}), (std::vector<Vec2>{{10, 20}}));
    const GripResult moved = moveGrip(Entity{insert}, 0, {0, 0});
    ASSERT_EQ(moved.outcome, GripOutcome::Moved);
    EXPECT_EQ(std::get<Insert>(moved.entity).insertion, (Vec2{0, 0}));
    EXPECT_EQ(std::get<Insert>(moved.entity).rotation, insert.rotation);
}

TEST(Insert, TheExtentChangesWithTheDefinition) {
    Document document = Document::newDrawing();
    document.replaceBlock("B", block(document, "B", {0, 0}, {Line{{0, 0}, {10, 0}}}));
    const EntityId id = document.add(insertOf("B", {0, 0}));
    expectPoint(document.extents().max(), {10, 0});
    std::vector<EntityId> found;
    document.query({{15, 2}, {16, 4}}, found);
    EXPECT_TRUE(found.empty());

    document.replaceBlock("b", block(document, "b", {0, 0}, {Line{{0, 0}, {20, 5}}}));
    EXPECT_EQ(document.blocks().definitions()[0].name, "B");  // the spelling stays
    expectPoint(document.extents().max(), {20, 5});
    document.query({{15, 2}, {16, 4}}, found);
    EXPECT_EQ(found, (std::vector<EntityId>{id}));
    EXPECT_TRUE(dependsOnBlock(document, *document.find(id), "b"));
}

TEST(Insert, ABlockInUseIsNotRemoved) {
    Document document = Document::newDrawing();
    document.replaceBlock("B", block(document, "B", {0, 0}, {Line{{0, 0}, {1, 0}}}));
    document.replaceBlock("A", block(document, "A", {0, 0}, {insertOf("B", {0, 0})}));
    document.replaceBlock("Free", BlockDefinition{"Free"});
    // B is used by A's definition, even before A is inserted (PURGE).
    EXPECT_TRUE(isBlockUsed(document, "B"));
    EXPECT_FALSE(isBlockUsed(document, "A"));
    const EntityId id = document.add(insertOf("A", {0, 0}));
    EXPECT_TRUE(isBlockUsed(document, "a"));
    EXPECT_TRUE(dependsOnBlock(document, *document.find(id), "B"));
    EXPECT_FALSE(dependsOnBlock(document, *document.find(id), "Free"));

    const BlockTable before = document.blocks();
    document.replaceBlock("A", std::nullopt);
    document.replaceBlock("B", std::nullopt);
    EXPECT_EQ(document.blocks(), before);
    document.replaceBlock("Free", std::nullopt);
    EXPECT_EQ(document.blocks().find("Free"), nullptr);
    EXPECT_EQ(document.blocks().definitions().size(), 2U);
}

TEST(Insert, TheHandlesOfABlockAreNeverGivenAgain) {
    Document document = Document::newDrawing();
    BlockDefinition definition = block(document, "B", {0, 0}, {Line{{0, 0}, {1, 0}}});
    const std::uint64_t handle = document.nextId() + 10;
    definition.entities[0].id = EntityId{handle};
    definition.recordId = handle + 1;
    document.replaceBlock("B", definition);
    EXPECT_GT(document.nextId(), handle + 1);
    EXPECT_GT(document.add(Line{{0, 0}, {1, 1}}).value, handle + 1);
}

TEST(Insert, AChainOf65LevelsStopsAtThe64th) {
    // B1 holds a line; each Bn inserts B(n-1) moved by (1, 0).
    Document document = Document::newDrawing();
    document.replaceBlock("B1", block(document, "B1", {0, 0}, {Line{{1, 1}, {2, 1}}}));
    for (int level = 2; level <= kMaxInsertDepth + 1; ++level) {
        const std::string name = "B" + std::to_string(level);
        document.replaceBlock(name, block(document, name, {0, 0},
                                          {insertOf("B" + std::to_string(level - 1), {1, 0})}));
    }
    const auto at64 = generatedLines(document, insertOf("B64", {0, 0}));
    ASSERT_EQ(at64.size(), 1U);
    expectLine(at64[0], {64, 1}, {65, 1});
    EXPECT_TRUE(generatedLines(document, insertOf("B65", {0, 0})).empty());
}

/// The snap found, or a point far from every test, so that a missing snap
/// fails the comparisons that follow.
SnapPoint found(const std::optional<SnapPoint>& snap) {
    EXPECT_TRUE(snap.has_value());
    return snap.value_or(SnapPoint{{-1e9, -1e9}, SnapKind::Center});
}

SnapModes only(bool SnapModes::*mode) {
    SnapModes modes{false, false, false, false, false, false, false, false};
    modes.*mode = true;
    return modes;
}

TEST(Insert, TheInsertionPointSnapsFarFromTheGeometry) {
    Document document = Document::newDrawing();
    document.replaceBlock("Far", block(document, "Far", {1000, 1000}, {Line{{0, 0}, {10, 0}}}));
    const EntityId id = document.add(insertOf("Far", {0, 0}));
    const SnapPoint snap = found(findSnap(document, {0.1, 0.1}, 0.5, only(&SnapModes::insertion)));
    EXPECT_EQ(snap.kind, SnapKind::Insertion);
    EXPECT_EQ(snap.point, (Vec2{0, 0}));
    EXPECT_FALSE(findSnap(document, {0.1, 0.1}, 0.5, only(&SnapModes::endpoint)));
    const SnapPoint end =
        found(findSnap(document, {-999.9, -1000}, 0.5, only(&SnapModes::endpoint)));
    expectPoint(end.point, {-1000, -1000});
    // The point is not a shape: a click there takes nothing.
    EXPECT_FALSE(pickEntity(document, {0, 0}, 1.0));
    EXPECT_EQ(selectInRectangle(document, {{-1001, -1001}, {-989, -999}}, RectangleMode::Window),
              (std::vector<EntityId>{id}));
}

TEST(Insert, IntersectionsInsideOneInsertAndWithAnotherEntity) {
    Document document = Document::newDrawing();
    document.replaceBlock("Cross", block(document, "Cross", {0, 0},
                                         {Line{{0, 0}, {10, 10}}, Line{{0, 10}, {10, 0}}}));
    document.add(insertOf("Cross", {0, 0}));
    const SnapPoint inside =
        found(findSnap(document, {5, 5.1}, 0.5, only(&SnapModes::intersection)));
    EXPECT_EQ(inside.kind, SnapKind::Intersection);
    expectPoint(inside.point, {5, 5});

    document.replaceBlock("Single", block(document, "Single", {0, 0}, {Line{{0, 0}, {10, 10}}}));
    document.add(insertOf("Single", {1000, 1000}));
    document.add(Line{{1000, 1010}, {1010, 1000}});
    const SnapPoint outside =
        found(findSnap(document, {1005, 1005.1}, 0.5, only(&SnapModes::intersection)));
    EXPECT_EQ(outside.kind, SnapKind::Intersection);
    expectPoint(outside.point, {1005, 1005});
}

/// "Pair": two horizontal lines and a hidden one, inserted at (100, 50).
struct Pair {
    Document document = Document::newDrawing();
    EntityId id;

    Pair() {
        Layer off;
        off.name = "Off";
        off.linetype = document.continuousLinetype();
        off.on = false;
        const LayerId hidden = document.addLayer(off);
        BlockDefinition pair =
            block(document, "Pair", {0, 0}, {Line{{0, 0}, {10, 0}}, Line{{0, 10}, {10, 10}}});
        EntityRecord far = member(document, Line{{0, 100}, {10, 100}});
        far.properties.layer = hidden;
        pair.entities.push_back(far);
        document.replaceBlock("Pair", pair);
        id = document.add(insertOf("Pair", {100, 50}));
    }
};

TEST(Insert, AClickTakesTheNearestVisibleShape) {
    const Pair drawing;
    const Document& document = drawing.document;
    EXPECT_EQ(pickEntity(document, {105, 51}, 1.0), drawing.id);
    EXPECT_FALSE(pickEntity(document, {105, 51.000001}, 1.0));
    EXPECT_FALSE(pickEntity(document, {100, 55}, 1.0));   // inside, between the lines
    EXPECT_FALSE(pickEntity(document, {105, 150}, 1.0));  // the hidden line
    EXPECT_EQ(distance(document, *document.find(drawing.id), {105, 51}), 1.0);
}

TEST(Insert, AWindowTakesTheInsertWhenEveryVisibleShapeIsInside) {
    const Pair drawing;
    const Document& document = drawing.document;
    const std::vector<EntityId> pair{drawing.id};
    EXPECT_EQ(selectInRectangle(document, {{100, 50}, {110, 60}}, RectangleMode::Window), pair);
    EXPECT_TRUE(selectInRectangle(document, {{99, 49}, {111, 51}}, RectangleMode::Window).empty());
    EXPECT_EQ(selectInRectangle(document, {{104, 49}, {106, 51}}, RectangleMode::Crossing), pair);
    EXPECT_TRUE(
        selectInRectangle(document, {{104, 54}, {106, 56}}, RectangleMode::Crossing).empty());
}

TEST(Insert, AnEmptyBlockIsTakenOnlyByAWindowOnTheInsertionPoint) {
    Document document = Document::newDrawing();
    document.replaceBlock("Empty", BlockDefinition{"Empty"});
    const EntityId id = document.add(insertOf("Empty", {200, 50}));
    const std::vector<EntityId> empty{id};
    EXPECT_TRUE(generate(document, *document.find(id)).empty());
    EXPECT_FALSE(pickEntity(document, {200, 50}, 1.0));
    EXPECT_EQ(selectInRectangle(document, {{199, 49}, {201, 51}}, RectangleMode::Window), empty);
    EXPECT_EQ(selectInRectangle(document, {{199, 49}, {201, 51}}, RectangleMode::Crossing), empty);
    EXPECT_TRUE(selectInRectangle(document, {{201, 49}, {202, 51}}, RectangleMode::Window).empty());
    const SnapPoint snap = found(findSnap(document, {200.1, 50}, 0.5, only(&SnapModes::insertion)));
    EXPECT_EQ(snap.point, (Vec2{200, 50}));
}

}  // namespace
}  // namespace tg::core
