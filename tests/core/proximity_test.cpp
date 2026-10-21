// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/proximity.hpp"

#include "tg/core/math.hpp"
#include "tg/core/text.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-10;

Entity curve(std::initializer_list<Vec2> points, bool closed = false) {
    Polyline polyline;
    for (const Vec2& point : points) {
        polyline.addVertex(point);
    }
    polyline.setClosed(closed);
    return polyline;
}

Entity arcEntity(const PolylineSegment& segment) {
    Polyline polyline;
    polyline.addVertex(segment.start, segment.bulge);
    polyline.addVertex(segment.end);
    return polyline;
}

PolylineSegment arcSegment(const Vec2& center, double radius, double startAngle, double sweep) {
    return {center + Vec2{std::cos(startAngle), std::sin(startAngle)} * radius,
            center + Vec2{std::cos(startAngle + sweep), std::sin(startAngle + sweep)} * radius,
            std::tan(sweep / 4.0)};
}

struct ClosestCase {
    std::string name;
    PolylineSegment segment;
    Vec2 point;
    Vec2 expected;
};

class ClosestPointTest : public testing::TestWithParam<ClosestCase> {};

TEST_P(ClosestPointTest, ReturnsNearestPointAndDistance) {
    const auto& test = GetParam();
    const Vec2 result = closestPoint(test.segment, test.point);
    EXPECT_NEAR(result.x, test.expected.x, kTolerance);
    EXPECT_NEAR(result.y, test.expected.y, kTolerance);
    EXPECT_NEAR(distance(test.segment, test.point), distance(test.expected, test.point),
                kTolerance);
    EXPECT_NEAR(distance(arcEntity(test.segment), test.point), distance(test.expected, test.point),
                kTolerance);
}

INSTANTIATE_TEST_SUITE_P(
    Lines, ClosestPointTest,
    testing::Values(ClosestCase{"Interior", {{0, 0}, {10, 0}, 0}, {4, 3}, {4, 0}},
                    ClosestCase{"BeforeStart", {{0, 0}, {10, 0}, 0}, {-2, 3}, {0, 0}},
                    ClosestCase{"AfterEnd", {{0, 0}, {10, 0}, 0}, {12, 3}, {10, 0}},
                    ClosestCase{"AtStart", {{0, 0}, {10, 0}, 0}, {0, 0}, {0, 0}},
                    ClosestCase{"AtEnd", {{0, 0}, {10, 0}, 0}, {10, 0}, {10, 0}},
                    ClosestCase{"Vertical", {{2, -4}, {2, 4}, 0}, {-3, 1}, {2, 1}},
                    ClosestCase{"Diagonal", {{0, 0}, {4, 4}, 0}, {4, 0}, {2, 2}},
                    ClosestCase{"Reversed", {{10, 0}, {0, 0}, 0}, {4, -3}, {4, 0}},
                    ClosestCase{"ZeroLength", {{3, 4}, {3, 4}, 0}, {0, 0}, {3, 4}},
                    ClosestCase{"ZeroLengthBulged", {{3, 4}, {3, 4}, 1}, {0, 0}, {3, 4}},
                    ClosestCase{"TinyBulgeIsStraight", {{0, 0}, {10, 0}, 1e-13}, {4, 3}, {4, 0}},
                    ClosestCase{"LargeCoordinates",
                                {{1e9, 1e9}, {1e9 + 10, 1e9}, 0},
                                {1e9 + 5, 1e9 + 2},
                                {1e9 + 5, 1e9}}),
    [](const testing::TestParamInfo<ClosestCase>& testCase) { return testCase.param.name; });

INSTANTIATE_TEST_SUITE_P(
    Arcs, ClosestPointTest,
    testing::Values(
        ClosestCase{"CcwOutsideCircle", {{-1, 0}, {1, 0}, 1}, {0, -2}, {0, -1}},
        ClosestCase{"CcwInsideCircle", {{-1, 0}, {1, 0}, 1}, {0, -0.25}, {0, -1}},
        ClosestCase{"CcwOutsideSweep", {{-1, 0}, {1, 0}, 1}, {-2, 1}, {-1, 0}},
        ClosestCase{"CcwOtherEndpoint", {{-1, 0}, {1, 0}, 1}, {2, 1}, {1, 0}},
        ClosestCase{"CwOutsideCircle", {{-1, 0}, {1, 0}, -1}, {0, 2}, {0, 1}},
        ClosestCase{"CwInsideCircle", {{-1, 0}, {1, 0}, -1}, {0, 0.25}, {0, 1}},
        ClosestCase{"CwOutsideSweep", {{-1, 0}, {1, 0}, -1}, {2, -1}, {1, 0}},
        ClosestCase{"CwOtherEndpoint", {{-1, 0}, {1, 0}, -1}, {-2, -1}, {-1, 0}},
        ClosestCase{"Center", {{-1, 0}, {1, 0}, 1}, {0, 0}, {-1, 0}},
        ClosestCase{"ClockwiseCenter", {{-1, 0}, {1, 0}, -1}, {0, 0}, {-1, 0}},
        ClosestCase{"OnArc", {{-1, 0}, {1, 0}, 1}, {0, -1}, {0, -1}},
        ClosestCase{"ExactEndpoint", {{-1, 0}, {1, 0}, 1}, {1, 0}, {1, 0}},
        ClosestCase{"MajorCcw", arcSegment({0, 0}, 1, 0, 1.5 * kPi), {-2, 0}, {-1, 0}},
        ClosestCase{"MajorCw", arcSegment({0, 0}, 1, 0, -1.5 * kPi), {-2, 0}, {-1, 0}},
        ClosestCase{"WrapCcw", arcSegment({0, 0}, 1, -kPi / 4, kPi / 2), {2, 0}, {1, 0}},
        ClosestCase{"WrapCw", arcSegment({0, 0}, 1, kPi / 4, -kPi / 2), {2, 0}, {1, 0}}),
    [](const testing::TestParamInfo<ClosestCase>& testCase) { return testCase.param.name; });

TEST(Proximity, TinyNonzeroLineIsNotCollapsed) {
    const PolylineSegment segment{{0, 0}, {1e-15, 0}, 0};
    const Vec2 nearest = closestPoint(segment, {0.5e-15, 1e-15});
    EXPECT_NEAR(nearest.x, 0.5e-15, 1e-30);
    EXPECT_DOUBLE_EQ(nearest.y, 0);
}

TEST(Proximity, ClosedPolylineIncludesClosingSegment) {
    const Entity open = curve({{0, 0}, {4, 0}, {4, 4}});
    const Entity closed = curve({{0, 0}, {4, 0}, {4, 4}}, true);
    EXPECT_NEAR(distance(open, {2, 2}), 2, kTolerance);
    EXPECT_NEAR(distance(closed, {2, 2}), 0, kTolerance);
}

TEST(Proximity, ClosedPolylineInteriorIsNotItsCurve) {
    EXPECT_NEAR(distance(curve({{-2, -2}, {2, -2}, {2, 2}, {-2, 2}}, true), {0, 0}), 2, kTolerance);
}

TEST(Proximity, OneVertexIsAPointEvenWhenClosed) {
    EXPECT_DOUBLE_EQ(distance(curve({{3, 4}}), {0, 0}), 5);
    EXPECT_DOUBLE_EQ(distance(curve({{3, 4}}, true), {0, 0}), 5);
}

TEST(Proximity, EmptyCurveHasInfiniteDistance) {
    EXPECT_EQ(distance(curve({}), {0, 0}), std::numeric_limits<double>::infinity());
}

TEST(Proximity, NearestSegmentCanBeAnArc) {
    Polyline polyline;
    polyline.addVertex({-1, 0}, 1);
    polyline.addVertex({1, 0});
    polyline.addVertex({1, 5});
    EXPECT_NEAR(distance(Entity{polyline}, {0, -2}), 1, kTolerance);
}

TEST(Proximity, ClosingSegmentCanBeAnArc) {
    Polyline polyline;
    polyline.addVertex({-1, 0});
    polyline.addVertex({1, 0}, 1);
    polyline.setClosed(true);
    EXPECT_NEAR(distance(Entity{polyline}, {0, 2}), 1, kTolerance);
    EXPECT_TRUE(intersects(Entity{polyline}, BBox{{-0.1, 0.9}, {0.1, 1.1}}));
}

struct RectangleCase {
    std::string name;
    Entity entity;
    BBox rectangle;
    bool inside;
    bool crossing;
};

class RectangleSelectionTest : public testing::TestWithParam<RectangleCase> {};

TEST_P(RectangleSelectionTest, DistinguishesWindowAndCrossing) {
    const auto& test = GetParam();
    EXPECT_EQ(isInside(test.entity, test.rectangle), test.inside);
    EXPECT_EQ(intersects(test.entity, test.rectangle), test.crossing);
}

INSTANTIATE_TEST_SUITE_P(
    Geometry, RectangleSelectionTest,
    testing::Values(
        RectangleCase{"Inside", curve({{-1, 0}, {1, 0}}), {{-2, -2}, {2, 2}}, true, true},
        RectangleCase{"Outside", curve({{3, 0}, {4, 0}}), {{-2, -2}, {2, 2}}, false, false},
        RectangleCase{
            "CrossesHorizontal", curve({{-3, 0}, {3, 0}}), {{-2, -2}, {2, 2}}, false, true},
        RectangleCase{"CrossesVertical", curve({{0, -3}, {0, 3}}), {{-2, -2}, {2, 2}}, false, true},
        RectangleCase{
            "CrossesDiagonal", curve({{-3, -3}, {3, 3}}), {{-2, -2}, {2, 2}}, false, true},
        RectangleCase{"OnBorder", curve({{-2, 2}, {2, 2}}), {{-2, -2}, {2, 2}}, true, true},
        RectangleCase{"AlongBorder", curve({{-3, 2}, {3, 2}}), {{-2, -2}, {2, 2}}, false, true},
        RectangleCase{
            "AlongVerticalBorder", curve({{2, -3}, {2, 3}}), {{-2, -2}, {2, 2}}, false, true},
        RectangleCase{"TouchesCorner", curve({{-3, 1}, {-1, 3}}), {{-2, -2}, {2, 2}}, false, true},
        RectangleCase{"OverlappingBoxesButNoHit",
                      curve({{-3, 1.5}, {-1.5, 3}}),
                      {{-2, -2}, {2, 2}},
                      false,
                      false},
        RectangleCase{"OneVertexInside", curve({{0, 0}, {3, 3}}), {{-2, -2}, {2, 2}}, false, true},
        RectangleCase{"SurroundsRectangle",
                      curve({{-3, -3}, {3, -3}, {3, 3}, {-3, 3}}, true),
                      {{-2, -2}, {2, 2}},
                      false,
                      false},
        RectangleCase{"EmptyCurve", curve({}), {{-2, -2}, {2, 2}}, false, false},
        RectangleCase{"EmptyRectangle", curve({{0, 0}, {1, 1}}), {}, false, false},
        RectangleCase{"SingleVertexInside", curve({{0, 0}}), {{-2, -2}, {2, 2}}, true, true},
        RectangleCase{"SingleVertexOutside", curve({{3, 0}}), {{-2, -2}, {2, 2}}, false, false},
        RectangleCase{"SingleVertexOnBorder", curve({{2, 0}}), {{-2, -2}, {2, 2}}, true, true},
        RectangleCase{"ZeroLengthInside", curve({{0, 0}, {0, 0}}), {{-2, -2}, {2, 2}}, true, true},
        RectangleCase{
            "ZeroLengthOutside", curve({{3, 0}, {3, 0}}), {{-2, -2}, {2, 2}}, false, false},
        RectangleCase{
            "PointRectangleOnLine", curve({{-2, 0}, {2, 0}}), {{0, 0}, {0, 0}}, false, true},
        RectangleCase{
            "PointRectangleOffLine", curve({{-2, -2}, {2, 2}}), {{0, 1}, {0, 1}}, false, false},
        RectangleCase{"PointRectangleOnVertex", curve({{1, 2}}), {{1, 2}, {1, 2}}, true, true},
        RectangleCase{
            "VerticalRectangle", curve({{-2, 0}, {2, 0}}), {{0, -1}, {0, 1}}, false, true},
        RectangleCase{
            "HorizontalRectangle", curve({{0, -2}, {0, 2}}), {{-1, 0}, {1, 0}}, false, true},
        RectangleCase{"CollinearDegenerateRectangle",
                      curve({{0, -2}, {0, 2}}),
                      {{0, -1}, {0, 1}},
                      false,
                      true},
        RectangleCase{"LineInsideDegenerateRectangle",
                      curve({{0, -1}, {0, 1}}),
                      {{0, -2}, {0, 2}},
                      true,
                      true},
        RectangleCase{"DisjointCollinearRectangle",
                      curve({{0, -3}, {0, -2}}),
                      {{0, -1}, {0, 1}},
                      false,
                      false},
        RectangleCase{"ArcInside", arcEntity({{-1, 0}, {1, 0}, 1}), {{-2, -2}, {2, 2}}, true, true},
        RectangleCase{"ArcExtremaOutside",
                      arcEntity({{-1, 0}, {1, 0}, 1}),
                      {{-1, -0.5}, {1, 0.5}},
                      false,
                      true},
        RectangleCase{"ArcCrossesWithoutVertices",
                      arcEntity({{-1, 0}, {1, 0}, 1}),
                      {{-0.25, -1.1}, {0.25, -0.9}},
                      false,
                      true},
        RectangleCase{"ClockwiseArcCrossesWithoutVertices",
                      arcEntity({{-1, 0}, {1, 0}, -1}),
                      {{-0.25, 0.9}, {0.25, 1.1}},
                      false,
                      true},
        RectangleCase{"CircleButNotArc",
                      arcEntity({{-1, 0}, {1, 0}, 1}),
                      {{-0.25, 0.9}, {0.25, 1.1}},
                      false,
                      false},
        RectangleCase{"InsideCircleNotOnCurve",
                      arcEntity({{-1, 0}, {1, 0}, 1}),
                      {{-0.1, -0.6}, {0.1, -0.4}},
                      false,
                      false},
        RectangleCase{"TangentHorizontal",
                      arcEntity({{-1, 0}, {1, 0}, 1}),
                      {{-0.2, -1.2}, {0.2, -1}},
                      false,
                      true},
        RectangleCase{"TangentVertical",
                      arcEntity({{0, -1}, {0, 1}, 1}),
                      {{1, -0.2}, {1.2, 0.2}},
                      false,
                      true},
        RectangleCase{
            "ArcPointRectangle", arcEntity({{-1, 0}, {1, 0}, 1}), {{0, -1}, {0, -1}}, false, true},
        RectangleCase{"ArcPointRectangleOffCurve",
                      arcEntity({{-1, 0}, {1, 0}, 1}),
                      {{0, -0.5}, {0, -0.5}},
                      false,
                      false},
        RectangleCase{
            "ArcLineRectangle", arcEntity({{-1, 0}, {1, 0}, 1}), {{0, -2}, {0, 0}}, false, true},
        RectangleCase{"ArcHorizontalLineRectangle",
                      arcEntity({{-1, 0}, {1, 0}, 1}),
                      {{-2, -0.5}, {2, -0.5}},
                      false,
                      true},
        RectangleCase{
            "ArcEndpointRectangle", arcEntity({{-1, 0}, {1, 0}, 1}), {{1, 0}, {1, 0}}, false, true},
        RectangleCase{"ArcOnlyChordCrosses",
                      arcEntity({{-1, 0}, {1, 0}, 1}),
                      {{-0.1, -0.1}, {0.1, 0.1}},
                      false,
                      false},
        RectangleCase{"ClosingLineCrosses",
                      curve({{0, 0}, {4, 0}, {4, 4}}, true),
                      {{1.9, 1.9}, {2.1, 2.1}},
                      false,
                      true},
        RectangleCase{"OpenLineDoesNotClose",
                      curve({{0, 0}, {4, 0}, {4, 4}}),
                      {{1.9, 1.9}, {2.1, 2.1}},
                      false,
                      false}),
    [](const testing::TestParamInfo<RectangleCase>& testCase) { return testCase.param.name; });

TEST(Proximity, ArcBoundingBoxIsAcceptedExactly) {
    const Entity entity = arcEntity(arcSegment({4, -3}, 2, 0.3, 4.5));
    EXPECT_TRUE(isInside(entity, boundingBox(entity)));
    EXPECT_TRUE(intersects(entity, boundingBox(entity)));
}

TEST(Proximity, MajorArcDoesNotIntersectItsMissingQuadrant) {
    const Entity entity = arcEntity(arcSegment({0, 0}, 1, 0, 1.5 * kPi));
    EXPECT_FALSE(intersects(entity, BBox{{0.6, -0.8}, {0.8, -0.6}}));
    EXPECT_TRUE(intersects(entity, BBox{{-1.1, -0.1}, {-0.9, 0.1}}));
}

TEST(Proximity, ClockwiseMajorArcDoesNotIntersectItsMissingQuadrant) {
    const Entity entity = arcEntity(arcSegment({0, 0}, 1, 0, -1.5 * kPi));
    EXPECT_FALSE(intersects(entity, BBox{{0.6, 0.6}, {0.8, 0.8}}));
    EXPECT_TRUE(intersects(entity, BBox{{-1.1, -0.1}, {-0.9, 0.1}}));
}

TEST(Proximity, TinyGeometryRetainsItsScale) {
    const Entity entity = arcEntity({{-1e-9, 0}, {1e-9, 0}, 1});
    EXPECT_NEAR(distance(entity, {0, -2e-9}), 1e-9, 1e-23);
    EXPECT_TRUE(intersects(entity, BBox{{-1e-10, -1.1e-9}, {1e-10, -0.9e-9}}));
    EXPECT_FALSE(intersects(entity, BBox{{-1e-10, -0.6e-9}, {1e-10, -0.4e-9}}));
}

TEST(Proximity, TranslatedArcRetainsIntersections) {
    const Entity entity = arcEntity({{1e9 - 1, 1e9}, {1e9 + 1, 1e9}, 1});
    EXPECT_NEAR(distance(entity, {1e9, 1e9 - 2}), 1, kTolerance);
    EXPECT_TRUE(intersects(entity, BBox{{1e9 - 0.1, 1e9 - 1.1}, {1e9 + 0.1, 1e9 - 0.9}}));
    EXPECT_FALSE(intersects(entity, BBox{{1e9 - 0.1, 1e9 - 0.6}, {1e9 + 0.1, 1e9 - 0.4}}));
}

// Independent oracle for straight tessellation chords: slab clipping, rather
// than the production implementation's intersections with four sides.
bool chordIntersects(const Vec2& start, const Vec2& end, const BBox& rectangle) {
    double enter = 0.0;
    double leave = 1.0;
    const auto clip = [&enter, &leave](double origin, double delta, double low, double high) {
        if (!(std::abs(delta) > 0.0)) {
            return origin >= low && origin <= high;
        }
        const double first = (low - origin) / delta;
        const double second = (high - origin) / delta;
        enter = std::max(enter, std::min(first, second));
        leave = std::min(leave, std::max(first, second));
        return enter <= leave;
    };
    return clip(start.x, end.x - start.x, rectangle.min().x, rectangle.max().x) &&
           clip(start.y, end.y - start.y, rectangle.min().y, rectangle.max().y);
}

double chordDistance(const Vec2& start, const Vec2& end, const Vec2& point) {
    const Vec2 delta = end - start;
    const double lengthSquared = delta.lengthSquared();
    if (!(lengthSquared > 0.0)) {
        return distance(start, point);
    }
    const double parameter = std::clamp(dot(point - start, delta) / lengthSquared, 0.0, 1.0);
    return distance(start + delta * parameter, point);
}

TEST(Proximity, RandomArcsAgreeWithDenseTessellation) {
    // A fixed seed makes geometry regressions reproducible; this is not security-sensitive.
    // NOLINTNEXTLINE(cert-msc32-c,cert-msc51-cpp)
    std::mt19937 generator{0xC0FFEEU};
    const auto unit = [&generator] {
        return static_cast<double>(generator()) / static_cast<double>(std::mt19937::max());
    };
    constexpr int kArcCount = 256;
    constexpr double kChordError = 1e-5;
    for (int i = 0; i < kArcCount; ++i) {
        SCOPED_TRACE(i);
        const Vec2 center{200.0 * unit() - 100.0, 200.0 * unit() - 100.0};
        const double radius = 0.25 + 20.0 * unit();
        const double startAngle = kTwoPi * unit();
        const double sweep = (i % 2 == 0 ? 1.0 : -1.0) * (0.05 + 6.0 * unit());
        const PolylineSegment segment = arcSegment(center, radius, startAngle, sweep);
        const Entity entity = arcEntity(segment);
        std::vector<Vec2> samples{segment.start};
        segment.tessellate(kChordError, samples);
        ASSERT_GT(samples.size(), 2U);
        for (int query = 0; query < 4; ++query) {
            const Vec2 point = center + Vec2{4.0 * unit() - 2.0, 4.0 * unit() - 2.0} * radius;
            double sampledDistance = std::numeric_limits<double>::infinity();
            for (std::size_t j = 1; j < samples.size(); ++j) {
                sampledDistance =
                    std::min(sampledDistance, chordDistance(samples[j - 1], samples[j], point));
            }
            EXPECT_NEAR(distance(segment, point), sampledDistance, kChordError + kTolerance);
            const Vec2 halfSize{radius * (0.1 + unit()), radius * (0.1 + unit())};
            const BBox rectangle{point - halfSize, point + halfSize};
            bool sampledIntersection = false;
            BBox sampledBox;
            for (std::size_t j = 0; j < samples.size(); ++j) {
                sampledBox.expand(samples[j]);
                if (j > 0 && chordIntersects(samples[j - 1], samples[j], rectangle)) {
                    sampledIntersection = true;
                }
            }
            EXPECT_EQ(intersects(entity, rectangle), sampledIntersection);
            EXPECT_EQ(isInside(entity, rectangle),
                      rectangle.contains(sampledBox.min()) && rectangle.contains(sampledBox.max()));
        }
    }
}

TEST(Proximity, AnArcEntityAgreesWithTheSameArcInAPolyline) {
    // A half circle of radius 5 centred on (5,0), counter-clockwise from
    // (10,0) to (0,0): bulge 1 in a polyline, angles 0 and π in an Arc.
    const Entity polyline = arcEntity(PolylineSegment{{10.0, 0.0}, {0.0, 0.0}, 1.0});
    const Entity arc = Arc{{5.0, 0.0}, 5.0, 0.0, kPi};
    for (const Vec2& point :
         {Vec2{5.0, 9.0}, Vec2{5.0, -3.0}, Vec2{12.0, 1.0}, Vec2{5.0, 0.0}, Vec2{-1.0, -1.0}}) {
        EXPECT_NEAR(distance(arc, point), distance(polyline, point), kTolerance);
    }
    for (const BBox& rectangle :
         {BBox{{4.0, 4.0}, {6.0, 6.0}}, BBox{{4.0, -2.0}, {6.0, 2.0}},
          BBox{{-1.0, -1.0}, {11.0, 6.0}}, BBox{{9.0, -1.0}, {11.0, 1.0}}}) {
        EXPECT_EQ(intersects(arc, rectangle), intersects(polyline, rectangle));
        EXPECT_EQ(isInside(arc, rectangle), isInside(polyline, rectangle));
    }
}

TEST(Proximity, LineAgreesWithATwoPointPolyline) {
    const Entity line = Line{{1.0, 2.0}, {7.0, -3.0}};
    const Entity polyline = curve({{1.0, 2.0}, {7.0, -3.0}});
    for (const Vec2& point : {Vec2{0.0, 0.0}, Vec2{4.0, 0.0}, Vec2{9.0, -9.0}}) {
        EXPECT_EQ(distance(line, point), distance(polyline, point));
    }
    for (const BBox& rectangle : {BBox{{3.0, -1.0}, {5.0, 1.0}}, BBox{{-5.0, 5.0}, {0.0, 9.0}}}) {
        EXPECT_EQ(intersects(line, rectangle), intersects(polyline, rectangle));
    }
}

// --- Text: a region (docs/adr/0013, point 6) -----------------------------

/// "Hello" 2 high at (10,20), turned by `rotation`.
Text hello(double rotation = 0.0) {
    Text text;
    text.position = {10.0, 20.0};
    text.height = 2.0;
    text.rotation = rotation;
    text.value = "Hello";
    return text;
}

TEST(TextProximity, InsideTheBoxTheDistanceIsZero) {
    for (const double rotation : {0.0, kPi / 2.0, degreesToRadians(30.0), kPi}) {
        const Text text = hello(rotation);
        const std::array<Vec2, 4> box = textBox(text);
        const Vec2 middle = (box[0] + box[2]) * 0.5;
        for (const Vec2& point :
             {middle, text.position, box[0], box[1], box[2], box[3], (box[1] + box[2]) * 0.5}) {
            EXPECT_EQ(distance(Entity{text}, point), 0.0) << rotation;
            EXPECT_EQ(closestPoint(text, point), point) << rotation;
        }
    }
}

TEST(TextProximity, OutsideTheDistanceIsToTheBox) {
    const Text text = hello();
    const std::array<Vec2, 4> box = textBox(text);
    // Left of the box, beside its side.
    EXPECT_NEAR(distance(Entity{text}, {7.0, 21.0}), 3.0, kTolerance);
    EXPECT_NEAR(closestPoint(text, {7.0, 21.0}).x, 10.0, kTolerance);
    EXPECT_NEAR(closestPoint(text, {7.0, 21.0}).y, 21.0, kTolerance);
    // Above the box, beside its top.
    EXPECT_NEAR(distance(Entity{text}, {12.0, box[3].y + 2.0}), 2.0, kTolerance);
    // Beyond a corner: the distance to the corner.
    const Vec2 beyond = box[2] + Vec2{3.0, 4.0};
    EXPECT_NEAR(distance(Entity{text}, beyond), 5.0, kTolerance);
    EXPECT_NEAR(distance(closestPoint(text, beyond), box[2]), 0.0, kTolerance);

    // The same, turned by a quarter: the box rises from the position.
    const Text turned = hello(kPi / 2.0);
    const std::array<Vec2, 4> turnedBox = textBox(turned);
    const Vec2 right{turnedBox[0].x + 2.0, 22.0};
    EXPECT_NEAR(distance(Entity{turned}, right), 2.0, kTolerance);
    EXPECT_NEAR(closestPoint(turned, right).x, turnedBox[0].x, kTolerance);
    EXPECT_NEAR(closestPoint(turned, right).y, 22.0, kTolerance);
}

TEST(TextProximity, AnEmptyTextIsASegment) {
    Text empty = hello();
    empty.value.clear();
    const std::array<Vec2, 4> box = textBox(empty);
    EXPECT_EQ(distance(Entity{empty}, (box[0] + box[3]) * 0.5), 0.0);
    EXPECT_NEAR(distance(Entity{empty}, Vec2{12.0, 21.0}), 2.0, kTolerance);
}

TEST(TextProximity, WindowTakesTheWholeBox) {
    const Text text = hello(degreesToRadians(30.0));
    const BBox bounds = text.boundingBox();
    EXPECT_TRUE(isInside(Entity{text}, bounds));
    EXPECT_TRUE(isInside(Entity{text}, BBox{bounds.min() - Vec2{1, 1}, bounds.max() + Vec2{1, 1}}));
    // Without a corner of the box, the window does not take it.
    EXPECT_FALSE(isInside(Entity{text}, BBox{bounds.min() + Vec2{0.1, 0.0}, bounds.max()}));
}

TEST(TextProximity, CrossingTouchesTheRegion) {
    const Text text = hello();
    const std::array<Vec2, 4> box = textBox(text);
    // A rectangle inside the box touches it: the text is a region.
    EXPECT_TRUE(intersects(Entity{text}, BBox{{11.0, 20.0}, {12.0, 21.0}}));
    // Across one side, or a point on the border.
    EXPECT_TRUE(intersects(Entity{text}, BBox{{9.0, 20.0}, {10.5, 21.0}}));
    EXPECT_TRUE(intersects(Entity{text}, BBox{box[1], box[1]}));
    // Disjoint, left of the box.
    EXPECT_FALSE(intersects(Entity{text}, BBox{{5.0, 20.0}, {9.9, 21.0}}));
}

TEST(TextProximity, CrossingATurnedTextUsesTheTurnedBox) {
    // At 45 degrees the bounding box is larger than the text: a rectangle in
    // a corner of the bounding box, away from the text, does not touch it.
    const Text text = hello(kPi / 4.0);
    const BBox bounds = text.boundingBox();
    const BBox corner{bounds.max() - Vec2{0.5, 0.5}, bounds.max()};
    EXPECT_FALSE(intersects(Entity{text}, corner));
    EXPECT_FALSE(isInside(Entity{text}, corner));
    // A rectangle around the middle of the box touches it.
    const std::array<Vec2, 4> box = textBox(text);
    const Vec2 middle = (box[0] + box[2]) * 0.5;
    EXPECT_TRUE(intersects(Entity{text}, BBox{middle - Vec2{0.1, 0.1}, middle + Vec2{0.1, 0.1}}));
    // A thin rectangle across the whole box, beyond it on both sides.
    EXPECT_TRUE(intersects(
        Entity{text}, BBox{{bounds.min().x - 1.0, middle.y}, {bounds.max().x + 1.0, middle.y}}));
}

}  // namespace
}  // namespace tg::core
