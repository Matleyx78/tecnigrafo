// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The ellipse in selection and in the Intersection snap, through
// solid-space's ellipse.hpp (docs/tappe/0.8/PT50-ellisse-selezione-snap.md).

#include "tg/core/block_table.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/document.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/intersections.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/snap_points.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

// nearest() and the points that solid-space constructs are rounded within
// 2^-44 of the magnitude (ellipse.hpp, kRelativeNearestError): for these
// small drawings 1e-12 is far above that error and far below any distance
// that matters.
constexpr double kBuilt = 1e-12;

void expectNear(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kBuilt);
    EXPECT_NEAR(actual.y, expected.y, kBuilt);
}

/// x²/16 + y²/4 = 1: the whole ellipse with half axes 4 and 2.
Ellipse wide() {
    return Ellipse{{0, 0}, {4, 0}, 0.5, 0.0, kTwoPi};
}

/// The arc of wide() from `startDegrees` to `endDegrees`, counter-clockwise.
Ellipse wideArc(double startDegrees, double endDegrees) {
    return Ellipse{
        {0, 0}, {4, 0}, 0.5, degreesToRadians(startDegrees), degreesToRadians(endDegrees)};
}

/// The Endpoint snaps of an arc: its start and its end, from endpoints().
std::pair<Vec2, Vec2> endsOf(const Ellipse& arc) {
    SnapModes onlyEnds{false, false, false, false, false, false, false, false};
    onlyEnds.endpoint = true;
    std::vector<SnapPoint> out;
    appendSnapPointsOf(arc, onlyEnds, out, std::nullopt);
    EXPECT_EQ(out.size(), 2U);
    if (out.size() != 2U) {
        return {};
    }
    return {out[0].point, out[1].point};
}

/// A polyline of (position, bulge) vertices.
Polyline polyline(std::initializer_list<std::pair<Vec2, double>> vertices, bool closed) {
    Polyline result;
    for (const auto& [position, bulge] : vertices) {
        result.addVertex(position, bulge);
    }
    result.setClosed(closed);
    return result;
}

// --- distance and closestPoint ---------------------------------------------

TEST(EllipseQueryNearest, FromOutsideAndFromInside) {
    // Outside, on the major axis: the end of the axis.
    EXPECT_NEAR(distance(wide(), {6, 0}), 2.0, kBuilt);
    const std::optional<Vec2> outside = closestPoint(wide(), {6, 0});
    ASSERT_TRUE(outside.has_value());
    expectNear(outside.value_or(Vec2{}), {4, 0});
    // Inside, on the major axis beyond the centre of curvature of its end
    // (x > 4 - 2²/4 = 3): still the end of the axis.
    EXPECT_NEAR(distance(wide(), {3.5, 0}), 0.5, kBuilt);
    const std::optional<Vec2> inside = closestPoint(wide(), {3.5, 0});
    ASSERT_TRUE(inside.has_value());
    expectNear(inside.value_or(Vec2{}), {4, 0});
}

TEST(EllipseQueryNearest, FromAPointOnTheEllipse) {
    // 2²/16 + 3/4 = 1.
    const Vec2 on{2, std::numbers::sqrt3};
    EXPECT_NEAR(distance(wide(), on), 0.0, kBuilt);
    const std::optional<Vec2> point = closestPoint(wide(), on);
    ASSERT_TRUE(point.has_value());
    expectNear(point.value_or(Vec2{}), on);
}

TEST(EllipseQueryNearest, BeyondTheEndOfAnArcIsThatEndBitForBit) {
    const Ellipse arc = wideArc(30, 120);
    const auto [start, end] = endsOf(arc);
    // The foot of the perpendicular from (6, -2) on the whole ellipse lies
    // below the arc: the nearest point of the arc is its start.
    const std::optional<Vec2> beyondStart = closestPoint(arc, {6, -2});
    ASSERT_TRUE(beyondStart.has_value());
    EXPECT_EQ(beyondStart.value_or(Vec2{}), start);
    EXPECT_NEAR(distance(arc, {6, -2}), (Vec2{6, -2} - start).length(), kBuilt);
    // Beyond the end, on the left.
    const std::optional<Vec2> beyondEnd = closestPoint(arc, {-5, -1});
    ASSERT_TRUE(beyondEnd.has_value());
    EXPECT_EQ(beyondEnd.value_or(Vec2{}), end);
    EXPECT_NEAR(distanceOf(arc, {-5, -1}), (Vec2{-5, -1} - end).length(), kBuilt);
}

// --- intersects -------------------------------------------------------------

TEST(EllipseQueryRectangle, ACrossingRectangleMeetsTheCurve) {
    EXPECT_TRUE(intersects(wide(), BBox{{3, -1}, {5, 1}}));
    EXPECT_TRUE(intersectsOf(wide(), BBox{{3, -1}, {5, 1}}));
    // Borders included: touching the end of the major axis.
    EXPECT_TRUE(intersects(wide(), BBox{{4, -1}, {5, 1}}));
    EXPECT_FALSE(intersects(wide(), BBox{{4.001, -1}, {5, 1}}));
    EXPECT_FALSE(intersects(wide(), BBox{}));
}

TEST(EllipseQueryRectangle, ARectangleInsideTheWholeEllipseDoesNotMeetIt) {
    EXPECT_FALSE(intersects(wide(), BBox{{-1, -1}, {1, 1}}));
}

TEST(EllipseQueryRectangle, ARectangleWithOnlyTheEndOfAnArc) {
    // Below the X axis the arc from 0° has only its start (4, 0); the rest of
    // the ellipse crosses the rectangle, but it is not the arc.
    const BBox below{{3, -1}, {5, 0}};
    EXPECT_TRUE(intersects(wideArc(0, 90), below));
    EXPECT_FALSE(intersects(wideArc(10, 90), below));
    EXPECT_TRUE(intersects(wide(), below));
}

// --- pickEntity and selectInRectangle ---------------------------------------

TEST(EllipseQueryDocument, AClickAndACrossingPickAnEllipse) {
    Document document = Document::newDrawing();
    const EntityId ellipse = document.add(wide());
    EXPECT_EQ(pickEntity(document, {4.05, 0}, 0.1), ellipse);
    EXPECT_FALSE(pickEntity(document, {4.2, 0}, 0.1).has_value());
    // From the centre the curve is 2 away.
    EXPECT_FALSE(pickEntity(document, {0, 0}, 1.0).has_value());

    const BBox across{{3, -1}, {5, 1}};
    EXPECT_TRUE(selectInRectangle(document, across, RectangleMode::Window).empty());
    EXPECT_EQ(selectInRectangle(document, across, RectangleMode::Crossing),
              (std::vector<EntityId>{ellipse}));
    const BBox around{{-5, -3}, {5, 3}};
    EXPECT_EQ(selectInRectangle(document, around, RectangleMode::Window),
              (std::vector<EntityId>{ellipse}));
    EXPECT_TRUE(
        selectInRectangle(document, BBox{{-1, -1}, {1, 1}}, RectangleMode::Crossing).empty());
}

TEST(EllipseQueryDocument, AStretchedInsertPicksThroughItsEllipse) {
    Document document = Document::newDrawing();
    BlockDefinition definition{"Round"};
    EntityRecord member{EntityId{}, Entity{Circle{{0, 0}, 1}}};
    member.properties.layer = document.layerZero();
    member.properties.linetype = document.byLayerLinetype();
    definition.entities.push_back(member);
    document.replaceBlock("Round", definition);
    Insert stretched;
    stretched.block = "Round";
    stretched.insertion = {10, 0};
    stretched.scaleX = 2.0;
    // The circle becomes an ellipse with half axes 2 and 1, centred at (10, 0).
    const EntityId insert = document.add(stretched);

    EXPECT_EQ(pickEntity(document, {12.05, 0}, 0.1), insert);
    EXPECT_EQ(pickEntity(document, {10, 1.05}, 0.1), insert);
    EXPECT_FALSE(pickEntity(document, {10, 0}, 0.5).has_value());

    const BBox across{{11.5, -0.5}, {12.5, 0.5}};
    EXPECT_TRUE(selectInRectangle(document, across, RectangleMode::Window).empty());
    EXPECT_EQ(selectInRectangle(document, across, RectangleMode::Crossing),
              (std::vector<EntityId>{insert}));
    EXPECT_EQ(selectInRectangle(document, BBox{{7, -2}, {13, 2}}, RectangleMode::Window),
              (std::vector<EntityId>{insert}));
    EXPECT_TRUE(selectInRectangle(document, BBox{{9.5, -0.5}, {10.5, 0.5}}, RectangleMode::Crossing)
                    .empty());
}

// --- intersect --------------------------------------------------------------

TEST(EllipseQueryIntersect, ALineCrossesTwice) {
    const Intersections found =
        intersect(wide(), Extension::None, Line{{-5, 1}, {5, 1}}, Extension::None);
    EXPECT_TRUE(found.complete);
    EXPECT_TRUE(found.shared.empty());
    ASSERT_EQ(found.crossings.size(), 2U);
    // Along the ellipse, from parameter 0: first the right point, then the
    // left one (x² = 16 * 3/4).
    expectNear(found.crossings[0].point, {2.0 * std::numbers::sqrt3, 1});
    expectNear(found.crossings[1].point, {-2.0 * std::numbers::sqrt3, 1});
    for (const Crossing& crossing : found.crossings) {
        EXPECT_EQ(crossing.contact, Contact::Crossing);
        EXPECT_EQ(crossing.onA, (CurvePosition{0, Location::Inside, 0.0}));
        EXPECT_EQ(crossing.onB, (CurvePosition{0, Location::Inside, 0.0}));
    }
}

TEST(EllipseQueryIntersect, ATangentCircleTouches) {
    // The circle of radius 2 on (0, 4) touches the top of the ellipse, whose
    // radius of curvature there is 4²/2 = 8.
    const Intersections found =
        intersect(wide(), Extension::None, Circle{{0, 4}, 2}, Extension::None);
    EXPECT_TRUE(found.complete);
    ASSERT_EQ(found.crossings.size(), 1U);
    EXPECT_EQ(found.crossings[0].contact, Contact::Touching);
    expectNear(found.crossings[0].point, {0, 2});
}

TEST(EllipseQueryIntersect, AVertexOfAClosedPolylineOnceOnTheFollowingPiece) {
    // Vertex 1, (4, 0), lies on the ellipse; the pieces meet it nowhere else.
    const Polyline triangle = polyline({{{6, 0}, 0.0}, {{4, 0}, 0.0}, {{6, -2}, 0.0}}, true);
    const Intersections found = intersect(wide(), Extension::None, triangle, Extension::None);
    EXPECT_TRUE(found.complete);
    ASSERT_EQ(found.crossings.size(), 1U);
    EXPECT_EQ(found.crossings[0].point, (Vec2{4, 0}));
    EXPECT_EQ(found.crossings[0].onB, (CurvePosition{1, Location::AtStart, 0.0}));

    // Vertex 0 of a closed polyline: on piece 0.
    const Polyline rotated = polyline({{{4, 0}, 0.0}, {{6, -2}, 0.0}, {{6, 0}, 0.0}}, true);
    const Intersections first = intersect(wide(), Extension::None, rotated, Extension::None);
    ASSERT_EQ(first.crossings.size(), 1U);
    EXPECT_EQ(first.crossings[0].onB, (CurvePosition{0, Location::AtStart, 0.0}));
}

TEST(EllipseQueryIntersect, TwoEllipsesInOneCall) {
    // x²/16 + y²/4 = 1 and x²/4 + y²/16 = 1: x² = y² = 16/5.
    const Ellipse tall{{0, 0}, {0, 4}, 0.5, 0.0, kTwoPi};
    const Intersections found = intersect(wide(), Extension::None, tall, Extension::None);
    EXPECT_TRUE(found.complete);
    EXPECT_TRUE(found.shared.empty());
    ASSERT_EQ(found.crossings.size(), 4U);
    const double c = std::sqrt(16.0 / 5.0);
    // Along the first ellipse: the four quadrants in order.
    expectNear(found.crossings[0].point, {c, c});
    expectNear(found.crossings[1].point, {-c, c});
    expectNear(found.crossings[2].point, {-c, -c});
    expectNear(found.crossings[3].point, {c, -c});
    for (const Crossing& crossing : found.crossings) {
        EXPECT_EQ(crossing.contact, Contact::Crossing);
    }
}

TEST(EllipseQueryIntersect, ACircleEqualToAnEllipseOfRatioOneIsSharedWhole) {
    const Ellipse round{{0, 0}, {3, 0}, 1.0, 0.0, kTwoPi};
    const Intersections found =
        intersect(round, Extension::None, Circle{{0, 0}, 3}, Extension::None);
    EXPECT_TRUE(found.complete);
    EXPECT_TRUE(found.crossings.empty());
    ASSERT_EQ(found.shared.size(), 1U);
    EXPECT_TRUE(found.shared[0].whole);
}

TEST(EllipseQueryIntersect, TheEllipseAsBSwapsThePositions) {
    const Polyline triangle = polyline({{{6, 0}, 0.0}, {{4, 0}, 0.0}, {{6, -2}, 0.0}}, true);
    const Line line{{-5, 1}, {5, 1}};
    for (const Entity& other : {Entity{line}, Entity{triangle}}) {
        const Intersections asA = intersect(wide(), Extension::None, other, Extension::None);
        const Intersections asB = intersect(other, Extension::None, wide(), Extension::None);
        EXPECT_EQ(asB.complete, asA.complete);
        ASSERT_EQ(asB.crossings.size(), asA.crossings.size());
        for (std::size_t i = 0; i < asA.crossings.size(); ++i) {
            EXPECT_EQ(asB.crossings[i].point, asA.crossings[i].point);
            EXPECT_EQ(asB.crossings[i].contact, asA.crossings[i].contact);
            EXPECT_EQ(asB.crossings[i].onA, asA.crossings[i].onB);
            EXPECT_EQ(asB.crossings[i].onB, asA.crossings[i].onA);
        }
    }
}

// --- findSnap ---------------------------------------------------------------

TEST(EllipseQuerySnap, IntersectionOfALineAndAnEllipse) {
    Document document = Document::newDrawing();
    document.add(wide());
    document.add(Line{{-5, 1}, {5, 1}});
    SnapModes onlyIntersection{false, false, false, false, false, false, false, false};
    onlyIntersection.intersection = true;
    const std::optional<SnapPoint> snap = findSnap(document, {3.5, 1.05}, 0.3, onlyIntersection);
    ASSERT_TRUE(snap.has_value());
    const SnapPoint found = snap.value_or(SnapPoint{});
    EXPECT_EQ(found.kind, SnapKind::Intersection);
    expectNear(found.point, {2.0 * std::numbers::sqrt3, 1});
}

TEST(EllipseQuerySnap, EndpointOfAnEllipticalArc) {
    Document document = Document::newDrawing();
    const Ellipse arc = wideArc(30, 120);
    document.add(arc);
    SnapModes onlyEnds{false, false, false, false, false, false, false, false};
    onlyEnds.endpoint = true;
    const auto [start, end] = endsOf(arc);
    const std::optional<SnapPoint> snap = findSnap(document, start + Vec2{0.05, 0}, 0.2, onlyEnds);
    EXPECT_EQ(snap, (SnapPoint{start, SnapKind::Endpoint}));
    // The start of the arc is near (4 cos 30°, 2 sin 30°).
    expectNear(start, {2.0 * std::numbers::sqrt3, 1});
    const std::optional<SnapPoint> other = findSnap(document, end + Vec2{0, 0.05}, 0.2, onlyEnds);
    EXPECT_EQ(other, (SnapPoint{end, SnapKind::Endpoint}));
}

// --- data solid-space refuses and an arc reduced to a point -----------------

TEST(EllipseQueryRefused, AnEllipseBeyondTheDomainStaysOut) {
    // A centre beyond 2^50 (ellipse.hpp, outOfDomain).
    const Ellipse far{{1125899906842625.0, 0}, {10, 0}, 0.5, 0.0, kTwoPi};
    EXPECT_EQ(distance(far, {0, 0}), std::numeric_limits<double>::infinity());
    EXPECT_FALSE(closestPoint(far, {0, 0}).has_value());
    EXPECT_FALSE(intersects(far, BBox{{-1, -1}, {1, 1}}));
    const Intersections found =
        intersect(far, Extension::None, Line{{-5, 0}, {5, 0}}, Extension::None);
    EXPECT_FALSE(found.complete);
    EXPECT_TRUE(found.crossings.empty());

    // A parameter beyond kMaxTrigArgument.
    const Ellipse turning{{0, 0}, {10, 0}, 0.5, 0.0, 1e8};
    EXPECT_EQ(distance(turning, {0, 0}), std::numeric_limits<double>::infinity());
    EXPECT_FALSE(closestPoint(turning, {0, 0}).has_value());
    EXPECT_FALSE(intersects(turning, BBox{{-11, -6}, {11, 6}}));
}

TEST(EllipseQueryRefused, AnArcReducedToAPointIsItsStartPoint) {
    // 19080° = 53 turns, as a double in radians (solid-space's case
    // ellisse-regola-arc-ridotta-a-un-punto.json): the kernel reduces the
    // arc to one direction (degenerateCurve).
    const Ellipse point{{0, 0}, {10, 0}, 0.5, 0.0, 0x1.4d02421c87558p+8};
    EXPECT_NEAR(distance(point, {13, 4}), 5.0, kBuilt);
    EXPECT_EQ(closestPoint(point, {13, 4}), (Vec2{10, 0}));
    EXPECT_TRUE(intersects(point, BBox{{9, -1}, {11, 1}}));
    EXPECT_FALSE(intersects(point, BBox{{-11, -1}, {-9, 1}}));
    // It meets nothing, and that refusal leaves nothing out.
    const Intersections found =
        intersect(point, Extension::None, Line{{10, -5}, {10, 5}}, Extension::None);
    EXPECT_TRUE(found.complete);
    EXPECT_TRUE(found.crossings.empty());

    Document document = Document::newDrawing();
    const EntityId id = document.add(point);
    EXPECT_EQ(pickEntity(document, {10.1, 0}, 0.5), id);
    EXPECT_EQ(selectInRectangle(document, BBox{{9, -1}, {11, 1}}, RectangleMode::Crossing),
              (std::vector<EntityId>{id}));
}

}  // namespace
}  // namespace tg::core
