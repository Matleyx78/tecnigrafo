// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/hatch_boundary.hpp"

#include "tg/core/block_table.hpp"
#include "tg/core/document.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

// docs/tappe/0.8/PT43-contorni-dei-retini.md, "Test richiesti". The cases
// of the request are those of solid-space's ADR 0014, "I casi della
// richiesta", 1, 1 bis, 2, 4 and 5.

// The flags of DXF group 92 (docs/verifica/rapporto.md, row HATCH).
constexpr std::uint32_t kExternal = 1U;
constexpr std::uint32_t kPolyline = 2U;
constexpr std::uint32_t kDerived = 4U;
constexpr std::uint32_t kOutermost = 16U;

constexpr double kEps = 1e-9;

/// The four lines of the rectangle from (x0, y0) to (x1, y1), counterclockwise.
std::array<EntityId, 4> addRectangle(Document& document, double x0, double y0, double x1,
                                     double y1) {
    return {document.add(Line{{x0, y0}, {x1, y0}}), document.add(Line{{x1, y0}, {x1, y1}}),
            document.add(Line{{x1, y1}, {x0, y1}}), document.add(Line{{x0, y1}, {x0, y0}})};
}

Polyline closedPolyline(std::vector<PolylineVertex> vertices) {
    return Polyline{std::move(vertices), true};
}

/// A new layer with the given state, named `name`.
LayerId addLayer(Document& document, std::string name, bool on, bool frozen, bool locked) {
    Layer layer = *document.layer(document.layerZero());
    layer.id = {};
    layer.name = std::move(name);
    layer.on = on;
    layer.frozen = frozen;
    layer.locked = locked;
    return document.addLayer(layer);
}

void moveToLayer(Document& document, EntityId id, LayerId layer) {
    EntityProperties properties = document.find(id)->properties;
    properties.layer = layer;
    document.setProperties(id, properties);
}

void expectStraight(const HatchLoop& loop) {
    if (!loop.polyline.has_value()) {
        FAIL() << "Expected a polyline loop";
    }
    for (const PolylineVertex& vertex : loop.polyline->vertices()) {
        EXPECT_EQ(vertex.bulge, 0.0);
    }
}

void expectPolylineLoop(const HatchLoop& loop, std::uint32_t flags,
                        const std::vector<PolylineVertex>& vertices) {
    EXPECT_EQ(loop.flags, flags | kPolyline);
    EXPECT_TRUE(loop.edges.empty());
    if (!loop.polyline.has_value()) {
        FAIL() << "Expected a polyline loop";
    }
    const Polyline& polyline = *loop.polyline;
    EXPECT_TRUE(polyline.isClosed());
    EXPECT_EQ(polyline.constantWidth(), 0.0);
    ASSERT_EQ(polyline.vertexCount(), vertices.size());
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const PolylineVertex& vertex = polyline.vertices()[i];
        EXPECT_NEAR(vertex.position.x, vertices[i].position.x, kEps) << "vertex " << i;
        EXPECT_NEAR(vertex.position.y, vertices[i].position.y, kEps) << "vertex " << i;
        EXPECT_NEAR(vertex.bulge, vertices[i].bulge, kEps) << "vertex " << i;
        EXPECT_EQ(vertex.startWidth, 0.0) << "vertex " << i;
        EXPECT_EQ(vertex.endWidth, 0.0) << "vertex " << i;
    }
}

/// A whole circle: one Arc edge from 0 to 2*pi, with the same doubles.
void expectCircleLoop(const HatchLoop& loop, std::uint32_t flags, const Vec2& center,
                      double radius) {
    EXPECT_EQ(loop.flags, flags);
    EXPECT_FALSE(loop.polyline.has_value());
    ASSERT_EQ(loop.edges.size(), 1U);
    const auto* arc = std::get_if<Arc>(&loop.edges[0].curve);
    ASSERT_NE(arc, nullptr);
    EXPECT_EQ(arc->center, center);
    EXPECT_EQ(arc->radius, radius);
    EXPECT_EQ(arc->startAngle, 0.0);
    EXPECT_EQ(arc->endAngle, kTwoPi);
}

const std::vector<PolylineVertex> kSquare{{{0, 0}}, {{10, 0}}, {{10, 10}}, {{0, 10}}};

// Case 1: the rectangle and its island.
TEST(HatchBoundary, ARectangleOfLinesWithACircleInside) {
    Document document = Document::newDrawing();
    const auto lines = addRectangle(document, 0, 0, 10, 10);
    const EntityId circle = document.add(Circle{{5, 5}, 2});
    const DetectedBoundary boundary = detectHatchBoundary(document, {1, 1});
    EXPECT_EQ(boundary.outcome, BoundaryOutcome::Enclosed);
    EXPECT_TRUE(boundary.complete);
    EXPECT_FALSE(boundary.nearEntity.has_value());
    EXPECT_TRUE(boundary.openEnds.empty());
    ASSERT_EQ(boundary.loops.size(), 2U);
    expectPolylineLoop(boundary.loops[0], kExternal | kDerived, kSquare);
    expectCircleLoop(boundary.loops[1], kOutermost | kDerived, {5, 5}, 2);
    EXPECT_EQ(boundary.sources,
              (std::vector<EntityId>{lines[0], lines[1], lines[2], lines[3], circle}));
}

// Case 1 bis: the point inside the circle.
TEST(HatchBoundary, APointInsideTheCircleFillsTheCircle) {
    Document document = Document::newDrawing();
    addRectangle(document, 0, 0, 10, 10);
    document.add(Circle{{5, 5}, 2});
    const DetectedBoundary boundary = detectHatchBoundary(document, {5, 5});
    EXPECT_EQ(boundary.outcome, BoundaryOutcome::Enclosed);
    ASSERT_EQ(boundary.loops.size(), 1U);
    expectCircleLoop(boundary.loops[0], kExternal | kDerived, {5, 5}, 2);
}

// Case 2: an island in an island has neither external nor outermost.
TEST(HatchBoundary, AnIslandInAnIslandHasOnlyDerived) {
    Document document = Document::newDrawing();
    addRectangle(document, 0, 0, 10, 10);
    document.add(Circle{{5, 5}, 3});
    document.add(Circle{{6, 5}, 1});
    const DetectedBoundary boundary = detectHatchBoundary(document, {1, 1});
    EXPECT_EQ(boundary.outcome, BoundaryOutcome::Enclosed);
    ASSERT_EQ(boundary.loops.size(), 3U);
    expectPolylineLoop(boundary.loops[0], kExternal | kDerived, kSquare);
    expectCircleLoop(boundary.loops[1], kOutermost | kDerived, {5, 5}, 3);
    expectCircleLoop(boundary.loops[2], kDerived, {6, 5}, 1);
}

// Case 4: a gap wider than the tolerance of millimetres stays open.
TEST(HatchBoundary, AGapWiderThanTheToleranceGivesTheFreeEnds) {
    Document document = Document::newDrawing();
    document.add(Line{{0, 0}, {10, 0}});
    document.add(Line{{10, 2e-6}, {10, 10}});
    document.add(Line{{10, 10}, {0, 10}});
    document.add(Line{{0, 10}, {0, 0}});
    const DetectedBoundary boundary = detectHatchBoundary(document, {5, 5});
    EXPECT_EQ(boundary.outcome, BoundaryOutcome::NotEnclosed);
    EXPECT_TRUE(boundary.complete);
    EXPECT_TRUE(boundary.loops.empty());
    const std::vector<Vec2> ends{{10, 0}, {10, 2e-6}};
    EXPECT_EQ(boundary.openEnds, ends);
}

// Case 5: the point on a line names that line.
TEST(HatchBoundary, APointOnALineNamesItsEntity) {
    Document document = Document::newDrawing();
    const auto lines = addRectangle(document, 0, 0, 10, 10);
    const DetectedBoundary boundary = detectHatchBoundary(document, {5, 0});
    EXPECT_EQ(boundary.outcome, BoundaryOutcome::OnCurve);
    EXPECT_EQ(boundary.nearEntity, lines[0]);
    EXPECT_TRUE(boundary.loops.empty());
    EXPECT_TRUE(boundary.openEnds.empty());
    const DetectedBoundary right = detectHatchBoundary(document, {10, 5});
    EXPECT_EQ(right.nearEntity, lines[1]);
}

TEST(HatchBoundary, AClosedPolylineIsAContourAndItsSegmentsNameIt) {
    Document document = Document::newDrawing();
    const EntityId square = document.add(closedPolyline(kSquare));
    const DetectedBoundary boundary = detectHatchBoundary(document, {5, 5});
    EXPECT_EQ(boundary.outcome, BoundaryOutcome::Enclosed);
    ASSERT_EQ(boundary.loops.size(), 1U);
    expectPolylineLoop(boundary.loops[0], kExternal | kDerived, kSquare);
    EXPECT_EQ(boundary.sources, (std::vector<EntityId>{square}));
    EXPECT_EQ(detectHatchBoundary(document, {0, 5}).nearEntity, square);
}

TEST(HatchBoundary, ASegmentThatIsNotAnArcGetsBulgeZero) {
    // docs/adr/0036, point 7: a bulge of 1e-13 is straight, so no bulge
    // reaches solid-space, the last segment, which closes the loop, included.
    Document document = Document::newDrawing();
    document.add(closedPolyline({{{0, 0}, 1e-13}, {{10, 0}}, {{10, 10}}, {{0, 10}, -1e-13}}));
    const DetectedBoundary boundary = detectHatchBoundary(document, {5, 5});
    ASSERT_EQ(boundary.loops.size(), 1U);
    expectPolylineLoop(boundary.loops[0], kExternal | kDerived, kSquare);
    expectStraight(boundary.loops[0]);
}

TEST(HatchBoundary, AnArcOfAPolylineKeepsItsBulge) {
    Document document = Document::newDrawing();
    document.add(closedPolyline({{{6, 4}, 1.0}, {{2, 4}, 1.0}}));
    const DetectedBoundary boundary = detectHatchBoundary(document, {4, 4.5});
    EXPECT_EQ(boundary.outcome, BoundaryOutcome::Enclosed);
    ASSERT_EQ(boundary.loops.size(), 1U);
    expectPolylineLoop(boundary.loops[0], kExternal | kDerived, {{{2, 4}, 1.0}, {{6, 4}, 1.0}});
}

TEST(HatchBoundary, LayersOffAndFrozenDoNotCountLockedLayersDo) {
    Document document = Document::newDrawing();
    const auto lines = addRectangle(document, 0, 0, 10, 10);
    const LayerId locked = addLayer(document, "Locked", true, false, true);
    moveToLayer(document, lines[0], locked);
    const LayerId off = addLayer(document, "Off", false, false, false);
    const LayerId frozen = addLayer(document, "Frozen", true, true, false);
    const EntityId hidden = document.add(Line{{0, 5}, {10, 5}});
    moveToLayer(document, hidden, off);
    const EntityId cold = document.add(Circle{{5, 5}, 2});
    moveToLayer(document, cold, frozen);

    const DetectedBoundary boundary = detectHatchBoundary(document, {5, 5});
    EXPECT_EQ(boundary.outcome, BoundaryOutcome::Enclosed);
    ASSERT_EQ(boundary.loops.size(), 1U);
    expectPolylineLoop(boundary.loops[0], kExternal | kDerived, kSquare);
    EXPECT_EQ(detectHatchBoundary(document, {5, 0}).nearEntity, lines[0]);
}

TEST(HatchBoundary, AnEllipseAndAnInsertAreNotCandidates) {
    Document document = Document::newDrawing();
    document.add(Ellipse{{5, 5}, {4, 0}, 0.5, 0.0, kTwoPi});
    BlockDefinition frame{"Frame"};
    for (const Line& line : {Line{{0, 0}, {10, 0}}, Line{{10, 0}, {10, 10}},
                             Line{{10, 10}, {0, 10}}, Line{{0, 10}, {0, 0}}}) {
        EntityRecord member{EntityId{}, line};
        member.properties.layer = document.layerZero();
        member.properties.linetype = document.byLayerLinetype();
        frame.entities.push_back(member);
    }
    document.replaceBlock("Frame", frame);
    Insert insert;
    insert.block = "Frame";
    const EntityId inserted = document.add(insert);

    const DetectedBoundary inside = detectHatchBoundary(document, {5, 5});
    EXPECT_EQ(inside.outcome, BoundaryOutcome::NotEnclosed);
    EXPECT_TRUE(inside.complete);
    EXPECT_TRUE(inside.openEnds.empty());
    EXPECT_EQ(detectHatchBoundary(document, {5, 0}).outcome, BoundaryOutcome::NotEnclosed);
    const std::array chosen{inserted};
    const ChosenContours contours = chosenHatchContours(document, chosen);
    EXPECT_TRUE(contours.loops.empty());
    EXPECT_EQ(contours.leftOut, std::vector<EntityId>{inserted});
}

TEST(HatchBoundary, ACurveLeftOutOrAPointBeyondTheDomainIsIncomplete) {
    Document document = Document::newDrawing();
    addRectangle(document, 0, 0, 10, 10);
    document.add(Line{{2, 2}, {2, 2}});  // a point: degenerateCurve
    const DetectedBoundary boundary = detectHatchBoundary(document, {5, 5});
    EXPECT_EQ(boundary.outcome, BoundaryOutcome::Enclosed);
    EXPECT_FALSE(boundary.complete);
    ASSERT_EQ(boundary.loops.size(), 1U);

    const Document empty = Document::newDrawing();
    const DetectedBoundary nothing = detectHatchBoundary(empty, {0, 0});
    EXPECT_EQ(nothing.outcome, BoundaryOutcome::NotEnclosed);
    EXPECT_TRUE(nothing.complete);
    const DetectedBoundary far = detectHatchBoundary(empty, {0x1p50 + 1.0, 0});
    EXPECT_EQ(far.outcome, BoundaryOutcome::NotEnclosed);
    EXPECT_TRUE(far.openEnds.empty());
    EXPECT_FALSE(far.complete);
}

TEST(HatchBoundary, TheToleranceIsThatOfTheUnitsOfTheDrawing) {
    // The gap of 5e-7 is within the floor of the tolerance of millimetres,
    // beyond the tolerance of kilometres at these coordinates.
    Document document = Document::newDrawing();
    document.add(Line{{0, 0}, {10, 0}});
    document.add(Line{{10, 5e-7}, {10, 10}});
    document.add(Line{{10, 10}, {0, 10}});
    document.add(Line{{0, 10}, {0, 0}});
    EXPECT_EQ(detectHatchBoundary(document, {5, 5}).outcome, BoundaryOutcome::Enclosed);
    document.setUnits(7);
    EXPECT_EQ(detectHatchBoundary(document, {5, 5}).outcome, BoundaryOutcome::NotEnclosed);
}

TEST(ChosenHatchContours, DepthsGiveTheSignalsOfTheIslands) {
    Document document = Document::newDrawing();
    const EntityId outer =
        document.add(closedPolyline({{{0, 0}}, {{20, 0}}, {{20, 10}}, {{0, 10}}}));
    const EntityId open =
        document.add(Polyline{std::vector<PolylineVertex>{{{1, 1}}, {{2, 1}}, {{1, 2}}}});
    const EntityId left = document.add(Circle{{5, 5}, 3});
    const EntityId line = document.add(Line{{1, 1}, {2, 2}});
    const EntityId right = document.add(Circle{{15, 5}, 3});
    const EntityId triangle = document.add(closedPolyline({{{14, 4}}, {{16, 4}}, {{15, 6}}}));

    const std::array chosen{outer, open, left, line, right, triangle};
    const ChosenContours contours = chosenHatchContours(document, chosen);
    EXPECT_EQ(contours.taken, (std::vector<EntityId>{outer, left, right, triangle}));
    EXPECT_EQ(contours.leftOut, (std::vector<EntityId>{open, line}));
    ASSERT_EQ(contours.loops.size(), 4U);
    expectPolylineLoop(contours.loops[0], kExternal, {{{0, 0}}, {{20, 0}}, {{20, 10}}, {{0, 10}}});
    expectCircleLoop(contours.loops[1], kOutermost, {5, 5}, 3);
    expectCircleLoop(contours.loops[2], kOutermost, {15, 5}, 3);
    expectPolylineLoop(contours.loops[3], 0U, {{{14, 4}}, {{16, 4}}, {{15, 6}}});

    // In another order, the loops follow it.
    const std::array reversed{triangle, right, line, left, open, outer};
    const ChosenContours back = chosenHatchContours(document, reversed);
    EXPECT_EQ(back.taken, (std::vector<EntityId>{triangle, right, left, outer}));
    EXPECT_EQ(back.leftOut, (std::vector<EntityId>{line, open}));
    ASSERT_EQ(back.loops.size(), 4U);
    EXPECT_EQ(back.loops[0].flags, kPolyline);
    EXPECT_EQ(back.loops[3].flags, kExternal | kPolyline);
}

TEST(ChosenHatchContours, TwoCrossingContoursBothStay) {
    Document document = Document::newDrawing();
    const EntityId outer =
        document.add(closedPolyline({{{0, 0}}, {{20, 0}}, {{20, 10}}, {{0, 10}}}));
    const EntityId first = document.add(closedPolyline({{{2, 2}}, {{8, 2}}, {{8, 8}}, {{2, 8}}}));
    const EntityId second =
        document.add(closedPolyline({{{5, 5}}, {{12, 5}}, {{12, 9}}, {{5, 9}}}));
    const std::array chosen{outer, first, second};
    const ChosenContours contours = chosenHatchContours(document, chosen);
    EXPECT_TRUE(contours.leftOut.empty());
    ASSERT_EQ(contours.loops.size(), 3U);
    EXPECT_EQ(contours.loops[0].flags, kExternal | kPolyline);
    EXPECT_EQ(contours.loops[1].flags, kOutermost | kPolyline);
    EXPECT_EQ(contours.loops[2].flags, kOutermost | kPolyline);
}

TEST(ChosenHatchContours, TinyBulgesBecomeZeroAndBadContoursAreLeftOut) {
    Document document = Document::newDrawing();
    const EntityId square =
        document.add(closedPolyline({{{0, 0}, 1e-13}, {{10, 0}}, {{10, 10}}, {{0, 10}, -1e-13}}));
    const EntityId point = document.add(closedPolyline({{{3, 3}}, {{3, 3}, 1.0}, {{3, 3}}}));
    const EntityId huge = document.add(Circle{{0x1p51, 0}, 1});
    const EntityId missing{0xFFFFFU};  // not in the document
    const std::array chosen{point, square, missing, huge};
    const ChosenContours contours = chosenHatchContours(document, chosen);
    EXPECT_EQ(contours.taken, std::vector<EntityId>{square});
    EXPECT_EQ(contours.leftOut, (std::vector<EntityId>{point, missing, huge}));
    ASSERT_EQ(contours.loops.size(), 1U);
    expectPolylineLoop(contours.loops[0], kExternal, kSquare);
    expectStraight(contours.loops[0]);
}

TEST(ChosenHatchContours, NothingChosenGivesNothing) {
    const Document document = Document::newDrawing();
    const ChosenContours contours = chosenHatchContours(document, {});
    EXPECT_TRUE(contours.loops.empty());
    EXPECT_TRUE(contours.taken.empty());
    EXPECT_TRUE(contours.leftOut.empty());
}

}  // namespace
}  // namespace tg::core
