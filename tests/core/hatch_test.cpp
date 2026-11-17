// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/hatch.hpp"

#include "tg/core/document.hpp"
#include "tg/core/document_query.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/hatch_patterns.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/tolerance.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

// docs/tappe/0.8/PT30-retini-core.md, "Test richiesti".
constexpr int kMillimetres = 4;
constexpr int kMetres = 6;
constexpr int kInches = 0;  // $MEASUREMENT 0: acad.pat
constexpr double kPatternTolerance = 1e-12;

Arc arcDegrees(const Vec2& center, double radius, double start, double end) {
    return Arc{center, radius, degreesToRadians(start), degreesToRadians(end)};
}

/// circle= of the report: an arc from 0 to 2*pi.
Arc wholeCircle(const Vec2& center, double radius) {
    return Arc{center, radius, 0.0, kTwoPi};
}

HatchLoop edgeLoop(std::vector<HatchEdge> edges) {
    HatchLoop loop;
    loop.edges = std::move(edges);
    return loop;
}

HatchLoop polylineLoop(std::vector<PolylineVertex> vertices, bool closed = true) {
    HatchLoop loop;
    loop.flags = 2;
    loop.polyline = Polyline{std::move(vertices), closed};
    return loop;
}

HatchLoop rectangleLoop(const Vec2& a, const Vec2& b) {
    return polylineLoop({{a}, {{b.x, a.y}}, {b}, {{a.x, b.y}}});
}

Hatch solidHatch(std::vector<HatchLoop> loops) {
    Hatch hatch;
    hatch.pattern = "SOLID";
    hatch.solid = true;
    hatch.loops = std::move(loops);
    return hatch;
}

/// The hatch of RT30: the rectangle [0,10] x [0,8] with a hole of centre
/// (5,4) and radius 2.
Hatch rectangleWithHole() {
    Hatch hatch =
        solidHatch({rectangleLoop({0, 0}, {10, 8}), edgeLoop({HatchEdge{wholeCircle({5, 4}, 2)}})});
    hatch.seeds.emplace_back(1, 1);
    return hatch;
}

/// The loop with the gap of 5e-7 between the end of the last edge and the
/// start of the first.
HatchLoop gapLoop() {
    return edgeLoop({HatchEdge{Line{{0, 0}, {10, 0}}}, HatchEdge{Line{{10, 0}, {10, 8}}},
                     HatchEdge{Line{{10, 8}, {0, 8}}}, HatchEdge{Line{{0, 8}, {0.0000005, 0}}}});
}

/// The loops of the region that a solid hatch with `loops` generates.
std::vector<Polyline> regionLoops(const Hatch& hatch) {
    const Document document = Document::newDrawing();
    const EntityRecord record{EntityId{1}, hatch};
    const std::vector<GeneratedItem> items = generateOf(document, record, hatch);
    if (items.size() != 1 || !std::holds_alternative<FilledRegion>(items.front().shape)) {
        ADD_FAILURE() << "expected one filled region";
        return {};
    }
    return std::get<FilledRegion>(items.front().shape).loops;
}

/// The polyline of one loop (point 3).
Polyline polylineOf(const HatchLoop& loop) {
    const std::vector<Polyline> loops = regionLoops(solidHatch({loop}));
    return loops.size() == 1 ? loops.front() : Polyline{};
}

void expectVertices(const Polyline& polyline, const std::vector<PolylineVertex>& expected) {
    EXPECT_TRUE(polyline.isClosed());
    ASSERT_EQ(polyline.vertexCount(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const PolylineVertex& vertex = polyline.vertices()[i];
        EXPECT_DOUBLE_EQ(vertex.position.x, expected[i].position.x) << i;
        EXPECT_DOUBLE_EQ(vertex.position.y, expected[i].position.y) << i;
        EXPECT_NEAR(vertex.bulge, expected[i].bulge, 1e-15) << i;
    }
}

Hatch transformedHatch(const Hatch& hatch, const Similarity& similarity) {
    const Document document;
    const TransformOutcome outcome =
        transformedOf(document, hatch, similarity, MirrorText::Readable);
    EXPECT_TRUE(outcome.refusal.empty());
    if (!outcome.entity || !std::holds_alternative<Hatch>(*outcome.entity)) {
        ADD_FAILURE() << "expected a hatch";
        return hatch;
    }
    return std::get<Hatch>(*outcome.entity);
}

Similarity mirrorAcrossX() {
    const std::optional<Similarity> mirror = Similarity::reflection({0, 0}, {1, 0});
    EXPECT_TRUE(mirror.has_value());
    return mirror.value_or(Similarity{});
}

Similarity scalingBy(double factor) {
    const std::optional<Similarity> scaling = Similarity::scaling(factor, {0, 0});
    EXPECT_TRUE(scaling.has_value());
    return scaling.value_or(Similarity{});
}

Similarity quarterTurn() {
    return Similarity::rotation(kPi / 2.0, {0, 0});
}

// --- supported() and isClosed() (point 1) ----------------------------------

TEST(HatchSupported, NeedsClosedPolylinesWithVerticesAndEdgeLoopsWithEdges) {
    EXPECT_TRUE(supported(rectangleWithHole()));
    EXPECT_TRUE(supported(Hatch{}));
    EXPECT_FALSE(supported(solidHatch({polylineLoop({{{0, 0}}, {{1, 0}}, {{1, 1}}}, false)})));
    EXPECT_FALSE(supported(solidHatch({polylineLoop({}, true)})));
    EXPECT_FALSE(supported(solidHatch({edgeLoop({})})));
    EXPECT_FALSE(supported(solidHatch({rectangleLoop({0, 0}, {1, 1}), edgeLoop({})})));
}

TEST(HatchClosed, AGapIsClosedOnlyWithinTheToleranceOfTheUnit) {
    const Hatch hatch = solidHatch({gapLoop()});
    EXPECT_TRUE(supported(hatch));
    EXPECT_TRUE(isClosed(hatch.loops.front(), ToleranceRule{kMillimetres}));
    EXPECT_FALSE(isClosed(hatch.loops.front(), ToleranceRule{kMetres}));

    // Scaled by 100 the gap is 5e-5: still supported, no longer closed.
    const Hatch scaled = transformedHatch(hatch, scalingBy(100.0));
    EXPECT_TRUE(supported(scaled));
    EXPECT_FALSE(isClosed(scaled.loops.front(), ToleranceRule{kMillimetres}));
}

TEST(HatchClosed, AnOpenLoopIsSupportedButNotClosed) {
    const HatchLoop loop =
        edgeLoop({HatchEdge{Line{{0, 0}, {10, 0}}}, HatchEdge{Line{{10, 0}, {10, 8}}}});
    EXPECT_TRUE(supported(solidHatch({loop})));
    EXPECT_FALSE(isClosed(loop, ToleranceRule{kMillimetres}));
    expectVertices(polylineOf(loop), {{{0, 0}, 0.0}, {{10, 0}, 0.0}});
}

TEST(HatchClosed, ACircleIsClosedInEveryUnit) {
    const HatchLoop loop = edgeLoop({HatchEdge{wholeCircle({5, 4}, 2)}});
    EXPECT_TRUE(isClosed(loop, ToleranceRule{kMillimetres}));
    EXPECT_TRUE(isClosed(loop, ToleranceRule{kMetres}));
}

TEST(HatchClosed, PolylineLoopsFollowTheirFlag) {
    EXPECT_TRUE(isClosed(rectangleLoop({0, 0}, {1, 1}), ToleranceRule{kMetres}));
    EXPECT_FALSE(
        isClosed(polylineLoop({{{0, 0}}, {{1, 0}}, {{1, 1}}}, false), ToleranceRule{kMetres}));
    EXPECT_FALSE(isClosed(edgeLoop({}), ToleranceRule{kMillimetres}));
}

TEST(HatchClosed, EdgesBeyondTheDomainLeaveTheLoopOpen) {
    // Ends that coincide exactly: only the domain of solid-space (2^50,
    // docs/adr/0035, point 23) leaves these loops open.
    const HatchLoop beyondCoordinate =
        edgeLoop({HatchEdge{Line{{0, 0}, {2e15, 0}}}, HatchEdge{Line{{2e15, 0}, {0, 0}}}});
    const HatchLoop beyondRadius =
        edgeLoop({HatchEdge{arcDegrees({-1e15, -1e15}, 1.2e15, 0.0, 90.0)},
                  HatchEdge{Line{{-1e15, 2e14}, {2e14, -1e15}}}});
    for (const HatchLoop& loop : {beyondCoordinate, beyondRadius}) {
        EXPECT_TRUE(supported(solidHatch({loop})));
        EXPECT_FALSE(isClosed(loop, ToleranceRule{kMillimetres}));
        const Polyline polyline = polylineOf(loop);
        for (const PolylineVertex& vertex : polyline.vertices()) {
            EXPECT_TRUE(std::isfinite(vertex.position.x) && std::isfinite(vertex.position.y));
            EXPECT_TRUE(std::isfinite(vertex.bulge));
        }
    }
}

// --- The polyline of an edge loop (point 3) ---------------------------------

TEST(HatchLoopPolyline, ACircleIsTwoHalfTurnsFromItsStartAngle) {
    expectVertices(polylineOf(edgeLoop({HatchEdge{wholeCircle({5, 4}, 2)}})),
                   {{{7, 4}, 1.0}, {{3, 4}, 1.0}});
}

TEST(HatchLoopPolyline, TheEndOfTheLastEdgeIsNotAVertex) {
    expectVertices(polylineOf(gapLoop()),
                   {{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 8}, 0.0}, {{0, 8}, 0.0}});
}

TEST(HatchLoopPolyline, AnArcRunClockwiseHasANegativeBulge) {
    const Hatch hatch = solidHatch(
        {edgeLoop({HatchEdge{Line{{0, 0}, {10, 0}}}, HatchEdge{arcDegrees({10, 4}, 4, 270.0, 90.0)},
                   HatchEdge{Line{{10, 8}, {0, 8}}}, HatchEdge{Line{{0, 8}, {0, 0}}}})});
    expectVertices(polylineOf(hatch.loops.front()),
                   {{{0, 0}, 0.0}, {{10, 0}, 1.0}, {{10, 8}, 0.0}, {{0, 8}, 0.0}});

    // Mirrored across the X axis the arc stays counter-clockwise, from 270
    // to 90 degrees, and the loop runs it clockwise.
    const Hatch mirrored = transformedHatch(hatch, mirrorAcrossX());
    const Arc& arc = std::get<Arc>(mirrored.loops.front().edges.at(1).curve);
    EXPECT_DOUBLE_EQ(arc.center.x, 10.0);
    EXPECT_DOUBLE_EQ(arc.center.y, -4.0);
    EXPECT_NEAR(radiansToDegrees(arc.startAngle), 270.0, 1e-12);
    EXPECT_NEAR(radiansToDegrees(arc.endAngle), 90.0, 1e-12);
    expectVertices(polylineOf(mirrored.loops.front()),
                   {{{0, 0}, 0.0}, {{10, 0}, -1.0}, {{10, -8}, 0.0}, {{0, -8}, 0.0}});
}

TEST(HatchLoopPolyline, OnATieTheWrittenDirectionWins) {
    expectVertices(polylineOf(edgeLoop({HatchEdge{arcDegrees({0, 0}, 1, 0.0, 180.0)},
                                        HatchEdge{arcDegrees({0, 0}, 1, 180.0, 0.0)}})),
                   {{{1, 0}, 1.0}, {{-1, 0}, 1.0}});
}

TEST(HatchLoopPolyline, ASingleArcIsClosedByItsChord) {
    expectVertices(polylineOf(edgeLoop({HatchEdge{arcDegrees({0, 0}, 1, 0.0, 90.0)}})),
                   {{{1, 0}, tan(kPi / 8.0)}, {{0, 1}, 0.0}});
}

// --- The behaviours of RT30 -------------------------------------------------

TEST(HatchModel, ASolidNormalHatchGeneratesOneRegionOfAllItsLoops) {
    const std::vector<Polyline> loops = regionLoops(rectangleWithHole());
    ASSERT_EQ(loops.size(), 2U);
    expectVertices(loops[0], {{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 8}, 0.0}, {{0, 8}, 0.0}});
    expectVertices(loops[1], {{{7, 4}, 1.0}, {{3, 4}, 1.0}});
}

TEST(HatchModel, PatternsAndOtherIslandStylesGenerateNothingYet) {
    const Document document = Document::newDrawing();
    Hatch pattern = rectangleWithHole();
    pattern.solid = false;
    pattern.pattern = "ANSI31";
    EXPECT_TRUE(generateOf(document, EntityRecord{EntityId{1}, pattern}, pattern).empty());
    Hatch outer = rectangleWithHole();
    outer.islands = IslandStyle::Outer;
    EXPECT_TRUE(generateOf(document, EntityRecord{EntityId{1}, outer}, outer).empty());
}

/// A drawing with the hatch of RT30 on a layer of its own.
struct HatchDrawing {
    Document document = Document::newDrawing();
    LayerId layer;
    EntityId id;

    explicit HatchDrawing(const Hatch& hatch) {
        Layer own;
        own.name = "Own";
        own.linetype = document.continuousLinetype();
        layer = document.addLayer(own);
        CurrentProperties current = document.current();
        current.layer = layer;
        document.setCurrent(current);
        id = document.add(hatch);
    }

    [[nodiscard]] const EntityRecord& record() const { return *document.find(id); }

    void freeze() {
        Layer changed = *document.layer(layer);
        changed.frozen = true;
        document.replaceLayer(changed);
    }
};

TEST(HatchModel, TheExtentIsThatOfTheLoopsWhateverTheFill) {
    Hatch pattern = rectangleWithHole();
    pattern.solid = false;
    pattern.pattern = "ANSI31";
    for (const Hatch& hatch : {rectangleWithHole(), pattern}) {
        HatchDrawing drawing{hatch};
        const BBox box = extent(drawing.document, drawing.record());
        EXPECT_EQ(box.min(), (Vec2{0, 0}));
        EXPECT_EQ(box.max(), (Vec2{10, 8}));
        drawing.freeze();
        EXPECT_TRUE(extent(drawing.document, drawing.record()).isEmpty());
    }
}

TEST(HatchModel, SelectionUsesTheFilledRegion) {
    const HatchDrawing drawing{rectangleWithHole()};
    const EntityRecord& record = drawing.record();
    EXPECT_EQ(distance(drawing.document, record, {1, 1}), 0.0);
    // In the hole, 2 from its edge.
    EXPECT_NEAR(distance(drawing.document, record, {5, 4}), 2.0, 1e-12);
    EXPECT_EQ(pickEntity(drawing.document, {1, 1}, 0.5), std::optional{drawing.id});
    EXPECT_EQ(pickEntity(drawing.document, {5, 4}, 1.0), std::nullopt);
    EXPECT_TRUE(isInside(drawing.document, record, BBox{{0, 0}, {10, 8}}));
    EXPECT_FALSE(intersects(drawing.document, record, BBox{{4.5, 3.5}, {5.5, 4.5}}));
    EXPECT_TRUE(intersects(drawing.document, record, BBox{{0.5, 0.5}, {1.5, 1.5}}));
}

TEST(HatchModel, HasNeitherSnapsNorGrips) {
    const HatchDrawing drawing{rectangleWithHole()};
    SnapModes every;
    every.perpendicular = true;
    every.tangent = true;
    every.quadrant = true;
    std::vector<SnapPoint> points;
    appendSnapPoints(drawing.document, drawing.record(), SnapModes{}, points, std::nullopt);
    appendSnapPoints(drawing.document, drawing.record(), every, points, Vec2{20, 20});
    EXPECT_TRUE(points.empty());
    EXPECT_TRUE(gripPoints(drawing.record().entity).empty());
}

TEST(HatchModel, AReflectionMovesLoopsAndSeedsAndNegatesTheAngle) {
    Hatch hatch;
    hatch.pattern = "ANSI31";
    hatch.angle = degreesToRadians(30.0);
    hatch.scale = 2.0;
    hatch.doubled = true;
    hatch.islands = IslandStyle::Outer;
    hatch.loops = {rectangleLoop({0, 0}, {10, 8}), edgeLoop({HatchEdge{wholeCircle({5, 4}, 2)}})};
    hatch.seeds = {{1, 1}, {9, 7}};

    const Hatch mirrored = transformedHatch(hatch, mirrorAcrossX());
    EXPECT_NEAR(radiansToDegrees(mirrored.angle), -30.0, 1e-12);
    EXPECT_EQ(mirrored.scale, 2.0);
    EXPECT_TRUE(mirrored.doubled);
    EXPECT_EQ(mirrored.islands, IslandStyle::Outer);
    ASSERT_EQ(mirrored.seeds.size(), 2U);
    EXPECT_EQ(mirrored.seeds[0], (Vec2{1, -1}));
    EXPECT_EQ(mirrored.seeds[1], (Vec2{9, -7}));
    expectVertices(mirrored.loops.at(0).polyline.value_or(Polyline{}),
                   {{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, -8}, 0.0}, {{0, -8}, 0.0}});
    // A whole circle stays a whole circle.
    EXPECT_EQ(std::get<Arc>(mirrored.loops.at(1).edges.at(0).curve), wholeCircle({5, -4}, 2));
    EXPECT_TRUE(supported(mirrored));
}

TEST(HatchModel, TheFamiliesTurnWithTheHatch) {
    Hatch hatch = rectangleWithHole();
    hatch.patternLines.push_back(PatternLine{degreesToRadians(30.0), {1, 0}, {0, 1}, {}});
    const Hatch turned = transformedHatch(hatch, quarterTurn());
    EXPECT_NEAR(radiansToDegrees(turned.patternLines.at(0).angle), 120.0, 1e-12);
    EXPECT_EQ(turned.patternLines.at(0).origin, (Vec2{0, 1}));
    EXPECT_EQ(turned.patternLines.at(0).offset, (Vec2{-1, 0}));
}

TEST(HatchModel, ScaleAndDashesGrowWithTheFactor) {
    Hatch hatch = rectangleWithHole();
    hatch.scale = 1.5;
    hatch.patternLines.push_back(PatternLine{0.0, {1, 2}, {0, 1}, {0.5, -0.25}});
    const Hatch scaled = transformedHatch(hatch, scalingBy(2.0));
    EXPECT_EQ(scaled.scale, 3.0);
    const PatternLine& line = scaled.patternLines.at(0);
    EXPECT_EQ(line.origin, (Vec2{2, 4}));
    EXPECT_EQ(line.offset, (Vec2{0, 2}));
    EXPECT_EQ(line.dashes, (std::vector<double>{1.0, -0.5}));
    EXPECT_EQ(std::get<Arc>(scaled.loops.at(1).edges.at(0).curve), wholeCircle({10, 8}, 4));
}

// --- The predefined patterns (point 2) --------------------------------------

/// The step between two lines of the family, offset . (-sin, cos).
double stepOf(const PatternLine& line) {
    const SinCos d = sinCos(line.angle);
    return dot(line.offset, Vec2{-d.sine, d.cosine});
}

/// The shift along a line, offset . (cos, sin).
double shiftOf(const PatternLine& line) {
    const SinCos d = sinCos(line.angle);
    return dot(line.offset, Vec2{d.cosine, d.sine});
}

void expectLine(const PatternLine& line, double degrees, const Vec2& offset, double step) {
    EXPECT_NEAR(line.angle, degreesToRadians(degrees), kPatternTolerance);
    EXPECT_EQ(line.origin, (Vec2{0, 0}));
    EXPECT_NEAR(line.offset.x, offset.x, kPatternTolerance);
    EXPECT_NEAR(line.offset.y, offset.y, kPatternTolerance);
    EXPECT_NEAR(stepOf(line), step, kPatternTolerance);
    EXPECT_TRUE(line.dashes.empty());
}

TEST(PredefinedPatterns, Ansi31) {
    const auto inch = predefinedPatternLines("ANSI31", 1.0, 0.0, kInches);
    ASSERT_EQ(inch.size(), 1U);
    expectLine(inch[0], 45.0, {-0.0883883476483184, 0.0883883476483185}, 0.125);

    const auto metric = predefinedPatternLines("ANSI31", 1.0, 0.0, kMetricMeasurement);
    ASSERT_EQ(metric.size(), 1U);
    expectLine(metric[0], 45.0, {-2.245064030267288, 2.245064030267288}, 3.175);

    const auto turned =
        predefinedPatternLines("ANSI31", 2.0, degreesToRadians(30.0), kMetricMeasurement);
    ASSERT_EQ(turned.size(), 1U);
    expectLine(turned[0], 75.0, {-6.133628996935584, 1.643500936401007}, 6.35);
}

TEST(PredefinedPatterns, Net) {
    for (const bool metric : {true, false}) {
        const double step = metric ? 3.175 : 0.125;
        const auto lines =
            predefinedPatternLines("NET", 1.0, 0.0, metric ? kMetricMeasurement : kInches);
        ASSERT_EQ(lines.size(), 2U);
        expectLine(lines[0], 0.0, {0, step}, step);
        expectLine(lines[1], 90.0, {-step, 0}, step);
        // Exactly, with the quarter turns of cardinalSinCos.
        EXPECT_EQ(lines[0].offset, (Vec2{0, step}));
        EXPECT_EQ(lines[1].offset, (Vec2{-step, 0}));
    }
}

TEST(PredefinedPatterns, SolidAndUnknownNamesHaveNoLinesAndCaseDoesNotMatter) {
    EXPECT_TRUE(predefinedPatternLines("SOLID", 1.0, 0.0, kMetricMeasurement).empty());
    EXPECT_TRUE(predefinedPatternLines("ANSI32", 1.0, 0.0, kMetricMeasurement).empty());
    EXPECT_TRUE(predefinedPatternLines("", 1.0, 0.0, kInches).empty());
    EXPECT_EQ(predefinedPatternLines("ansi31", 1.0, 0.0, kMetricMeasurement),
              predefinedPatternLines("ANSI31", 1.0, 0.0, kMetricMeasurement));
    EXPECT_EQ(predefinedPatternLines("Net", 1.0, 0.0, kInches),
              predefinedPatternLines("NET", 1.0, 0.0, kInches));
}

TEST(PredefinedPatterns, TransformedAsAVectorOfTheDrawing) {
    Hatch hatch = rectangleWithHole();
    hatch.pattern = "ANSI31";
    hatch.solid = false;
    hatch.patternLines = predefinedPatternLines("ANSI31", 1.0, 0.0, kMetricMeasurement);

    const Hatch turned = transformedHatch(hatch, quarterTurn());
    expectLine(turned.patternLines.at(0), 135.0, {-2.245064030267288, -2.245064030267288}, 3.175);

    const Hatch mirrored = transformedHatch(hatch, mirrorAcrossX());
    expectLine(mirrored.patternLines.at(0), -45.0, {-2.245064030267288, -2.245064030267288},
               -3.175);
    EXPECT_NEAR(shiftOf(mirrored.patternLines.at(0)), 0.0, kPatternTolerance);
}

TEST(PredefinedPatterns, AReflectionNegatesTheStepAndKeepsTheShift) {
    Hatch hatch = rectangleWithHole();
    hatch.patternLines.push_back(PatternLine{0.0, {0, 0}, {2, 3}, {1.0, -0.5}});
    const PatternLine line = transformedHatch(hatch, mirrorAcrossX()).patternLines.at(0);
    EXPECT_EQ(line.offset, (Vec2{2, -3}));
    EXPECT_NEAR(stepOf(line), -3.0, kPatternTolerance);
    EXPECT_NEAR(shiftOf(line), 2.0, kPatternTolerance);
    EXPECT_EQ(line.dashes, (std::vector<double>{1.0, -0.5}));
}

}  // namespace
}  // namespace tg::core
