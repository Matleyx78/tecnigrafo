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
#include <limits>
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
std::vector<Polyline> solidRegionLoops(const Hatch& hatch) {
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
    const std::vector<Polyline> loops = solidRegionLoops(solidHatch({loop}));
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
    const std::vector<Polyline> loops = solidRegionLoops(rectangleWithHole());
    ASSERT_EQ(loops.size(), 2U);
    expectVertices(loops[0], {{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 8}, 0.0}, {{0, 8}, 0.0}});
    expectVertices(loops[1], {{{7, 4}, 1.0}, {{3, 4}, 1.0}});
}

TEST(HatchModel, TransformationsPreserveTheOrderedExtrasOfEachLoop) {
    Hatch hatch = rectangleWithHole();
    hatch.loops[0].extras = {{97, "2"}, {330, "A"}, {330, "B"}};
    hatch.loops[1].extras = {{97, "1"}, {330, "C"}};
    const std::vector<RawGroups> expected{hatch.loops[0].extras, hatch.loops[1].extras};

    for (const Similarity& similarity :
         {Similarity::translation({3, 4}), quarterTurn(), scalingBy(2.0), mirrorAcrossX()}) {
        const Hatch changed = transformedHatch(hatch, similarity);
        ASSERT_EQ(changed.loops.size(), expected.size());
        EXPECT_EQ(changed.loops[0].extras, expected[0]);
        EXPECT_EQ(changed.loops[1].extras, expected[1]);
    }
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

// --- Origin, pattern and area (docs/tappe/0.8.1/PT23-proprieta-retini-quote.md)

/// The XDATA of the hatches of autocad-hatchedit: two gradient colours, then
/// ACAD with the origin of the pattern.
EntityExtras hatcheditExtras(const char* x, const char* y) {
    EntityExtras extras;
    extras.extended = {{1001, "GradientColor1ACI"},
                       {1070, "     5"},
                       {1001, "GradientColor2ACI"},
                       {1070, "     2"},
                       {1001, "ACAD"},
                       {1010, x},
                       {1020, y},
                       {1030, "0.0"}};
    return extras;
}

/// The outer rectangle and the island of the hatches of autocad-hatchedit,
/// whose left lower corner is at (x, 0): a 60 x 40 rectangle flagged
/// external, a circle of radius 8 flagged outermost.
Hatch hatcheditShape(double x) {
    Hatch hatch;
    hatch.pattern = "ANSI31";
    HatchLoop outer = polylineLoop({{{x + 60, 40}}, {{x, 40}}, {{x, 0}}, {{x + 60, 0}}});
    outer.flags = 7;
    HatchLoop island = polylineLoop({{{x + 22, 20}, -1.0}, {{x + 38, 20}, -1.0}});
    island.flags = 22;
    hatch.loops = {outer, island};
    hatch.seeds = {{x + 5, 5}, {x + 5, 5}};
    return hatch;
}

/// The ANSI31 family of a hatch of autocad-hatchedit with scale `scale`.
PatternLine ansi31Family(const Vec2& base, double scale) {
    constexpr double kOffset = 2.245064030267288;  // 3.175 / sqrt(2)
    return PatternLine{degreesToRadians(45), base, Vec2{-kOffset, kOffset} * scale, {}};
}

void expectSameFamilies(const std::vector<PatternLine>& actual,
                        const std::vector<PatternLine>& expected) {
    ASSERT_EQ(actual.size(), expected.size());
    for (std::size_t i = 0; i < actual.size(); ++i) {
        SCOPED_TRACE(i);
        EXPECT_NEAR(actual[i].angle, expected[i].angle, kPatternTolerance);
        EXPECT_NEAR(actual[i].origin.x, expected[i].origin.x, kPatternTolerance);
        EXPECT_NEAR(actual[i].origin.y, expected[i].origin.y, kPatternTolerance);
        EXPECT_NEAR(actual[i].offset.x, expected[i].offset.x, kPatternTolerance);
        EXPECT_NEAR(actual[i].offset.y, expected[i].offset.y, kPatternTolerance);
        ASSERT_EQ(actual[i].dashes.size(), expected[i].dashes.size());
        for (std::size_t j = 0; j < actual[i].dashes.size(); ++j) {
            EXPECT_NEAR(actual[i].dashes[j], expected[i].dashes[j], kPatternTolerance);
        }
    }
}

/// The hatch withPattern() gives; std::nullopt fails the test.
Hatch made(const std::optional<Hatch>& hatch) {
    EXPECT_TRUE(hatch.has_value());
    return hatch.value_or(Hatch{});
}

TEST(PatternOrigin, ReadsTheAcadPointAndIsZeroWithout) {
    EXPECT_EQ(patternOrigin(hatcheditExtras("245.0", "5.0")), (Vec2{245, 5}));
    EXPECT_EQ(patternOrigin(EntityExtras{}), (Vec2{0, 0}));
    EntityExtras other;
    other.extended = {{1001, "OTHER"}, {1010, "3.0"}, {1020, "4.0"}};
    EXPECT_EQ(patternOrigin(other), (Vec2{0, 0}));
    EXPECT_EQ(patternOrigin(hatcheditExtras("x", "5.0")), (Vec2{0, 0}));
}

TEST(PatternOrigin, TwoFamiliesOfTheFileMoveByTheSameShift) {
    Hatch hatch = hatcheditShape(0);
    hatch.patternLines = {PatternLine{0.0, {13, 0}, {0, 1}, {}},
                          PatternLine{kPi / 2, {17, 0}, {-1, 0}, {}}};
    const Hatch moved = withPatternOrigin(hatch, {10, 0}, {20, 0});
    EXPECT_EQ(moved.patternLines.at(0).origin, (Vec2{23, 0}));
    EXPECT_EQ(moved.patternLines.at(1).origin, (Vec2{27, 0}));
    EXPECT_EQ(moved.patternLines.at(0).offset, (Vec2{0, 1}));
    EXPECT_EQ(moved.loops, hatch.loops);
}

TEST(PatternOrigin, TheOriginOfCcOfAutocadHatchedit) {
    // E1 has origin 0,0; CC is the same hatch after -HATCHEDIT Origin 245,5.
    Hatch hatch = hatcheditShape(480);
    hatch.patternLines = {ansi31Family({0, 0}, 1)};
    const Hatch moved = withPatternOrigin(hatch, patternOrigin(EntityExtras{}), {245, 5});
    expectSameFamilies(moved.patternLines, {ansi31Family({245, 5}, 1)});
}

TEST(PatternOrigin, XdataIsSetInPlaceOrAdded) {
    const EntityExtras set = withPatternOriginXdata(hatcheditExtras("0.0", "0.0"), {245, 5});
    EXPECT_EQ(set.extended, hatcheditExtras("245.0", "5.0").extended);

    const EntityExtras added = withPatternOriginXdata(EntityExtras{}, {10, -0.5});
    EXPECT_EQ(added.extended,
              (RawGroups{{1001, "ACAD"}, {1010, "10.0"}, {1020, "-0.5"}, {1030, "0.0"}}));
    EXPECT_EQ(patternOrigin(added), (Vec2{10, -0.5}));

    EntityExtras withoutPoint;
    withoutPoint.extended = {{1001, "ACAD"}, {1000, "x"}, {1001, "OTHER"}, {1070, "1"}};
    const EntityExtras inserted = withPatternOriginXdata(withoutPoint, {0.1, 2});
    EXPECT_EQ(inserted.extended, (RawGroups{{1001, "ACAD"},
                                            {1000, "x"},
                                            {1010, "0.1"},
                                            {1020, "2.0"},
                                            {1030, "0.0"},
                                            {1001, "OTHER"},
                                            {1070, "1"}}));

    EntityExtras withoutY;
    withoutY.extended = {{1001, "ACAD"}, {1010, "1.0"}};
    EXPECT_EQ(withPatternOriginXdata(withoutY, {3, 4}).extended,
              (RawGroups{{1001, "ACAD"}, {1010, "3.0"}, {1020, "4.0"}}));
}

TEST(PatternOrigin, XdataKeepsEveryDigitOfTheOrigin) {
    const Vec2 origin{0.1 + 0.2, -1.0 / 3.0};
    EXPECT_EQ(patternOrigin(withPatternOriginXdata(EntityExtras{}, origin)), origin);
}

TEST(WithPattern, ScaleFrom1To2AroundTheOrigin) {
    Hatch hatch = hatcheditShape(0);
    hatch.pattern = "USER1";
    hatch.patternLines = {PatternLine{0.0, {13, 0}, {0, 1}, {0.5, -0.25}}};
    const Hatch scaled = made(withPattern(hatch, {10, 0}, "user1", 2, 0, 1));
    EXPECT_EQ(scaled.pattern, "USER1");
    EXPECT_EQ(scaled.scale, 2);
    expectSameFamilies(scaled.patternLines, {PatternLine{0.0, {16, 0}, {0, 2}, {1.0, -0.5}}});
}

TEST(WithPattern, AQuarterTurnOfAPatternThatIsNotGeneratedIsExact) {
    // B7 of autocad-hatchedit: ANSI37, scale 1, angle 0, origin 0,0.
    Hatch hatch = hatcheditShape(0);
    hatch.pattern = "ANSI37";
    constexpr double kOffset = 2.245064030267288;
    hatch.patternLines = {PatternLine{degreesToRadians(45), {0, 0}, {-kOffset, kOffset}, {}},
                          PatternLine{degreesToRadians(135), {0, 0}, {-kOffset, -kOffset}, {}}};
    const Hatch turned = made(
        withPattern(hatch, {0, 0}, "ANSI37", 1, reducedDegreesToRadians(90), kMetricMeasurement));
    EXPECT_EQ(turned.patternLines.at(0).offset, (Vec2{-kOffset, -kOffset}));
    EXPECT_EQ(turned.patternLines.at(1).offset, (Vec2{kOffset, -kOffset}));
}

TEST(WithPattern, ScaleAndAngleOfAPatternThatIsNotGeneratedGiveTheFamiliesOfDa) {
    // B7 (ANSI37 at 0,0, scale 1, angle 0) with the origin, scale and angle
    // of DA (405,5, scale 2, angle 30): the families AutoCAD wrote in DA.
    Hatch hatch = hatcheditShape(400);
    hatch.pattern = "ANSI37";
    constexpr double kOffset = 2.245064030267288;
    hatch.patternLines = {PatternLine{degreesToRadians(45), {0, 0}, {-kOffset, kOffset}, {}},
                          PatternLine{degreesToRadians(135), {0, 0}, {-kOffset, -kOffset}, {}}};
    const Hatch moved = withPatternOrigin(hatch, {0, 0}, {405, 5});
    const Hatch changed = made(
        withPattern(moved, {405, 5}, "ANSI37", 2, reducedDegreesToRadians(30), kMetricMeasurement));
    expectSameFamilies(
        changed.patternLines,
        {PatternLine{degreesToRadians(74.99999999999999),
                     {405, 5},
                     {-6.133628996935582, 1.643500936401007},
                     {}},
         PatternLine{
             degreesToRadians(165), {405, 5}, {-1.643500936401007, -6.133628996935582}, {}}});
}

TEST(WithPattern, GeneratedPatternsOfAutocadHatchedit) {
    Hatch hatch = hatcheditShape(0);
    hatch.patternLines = {ansi31Family({245, 5}, 1)};
    // CC with scale 2 keeps its origin; BE (origin 0,0) has scale 2.
    const Hatch twice = made(withPattern(hatch, {245, 5}, "ANSI31", 2, 0, 1));
    expectSameFamilies(twice.patternLines, {ansi31Family({245, 5}, 2)});
    const Hatch be = made(withPattern(hatch, {0, 0}, "ANSI31", 2, 0, 1));
    expectSameFamilies(
        be.patternLines,
        {PatternLine{degreesToRadians(45), {0, 0}, {-4.490128060534576, 4.490128060534576}, {}}});
    // C5: angle 30.
    const Hatch c5 = made(withPattern(hatch, {0, 0}, "ansi31", 1, reducedDegreesToRadians(30), 1));
    EXPECT_EQ(c5.pattern, "ANSI31");
    expectSameFamilies(c5.patternLines, {PatternLine{degreesToRadians(74.99999999999999),
                                                     {0, 0},
                                                     {-3.066814498467791, 0.8217504682005037},
                                                     {}}});
}

TEST(WithPattern, NetFollowsTheMeasurementOfTheDrawing) {
    const Hatch hatch = hatcheditShape(0);
    const Hatch metric = made(withPattern(hatch, {0, 0}, "NET", 1, 0, 1));
    const Hatch inches = made(withPattern(hatch, {0, 0}, "NET", 1, 0, 0));
    ASSERT_EQ(metric.patternLines.size(), 2U);
    ASSERT_EQ(inches.patternLines.size(), 2U);
    EXPECT_NEAR(metric.patternLines[0].offset.y, 3.175, kPatternTolerance);
    EXPECT_NEAR(metric.patternLines[1].offset.x, -3.175, kPatternTolerance);
    EXPECT_NEAR(inches.patternLines[0].offset.y, 0.125, kPatternTolerance);
    EXPECT_NEAR(inches.patternLines[1].offset.x, -0.125, kPatternTolerance);
}

TEST(WithPattern, SolidOtherNamesAndScalesThatAreNotPositive) {
    const Hatch hatch = hatcheditShape(0);
    const Hatch solid = made(withPattern(hatch, {245, 5}, "solid", 1, 0, 1));
    EXPECT_EQ(solid.pattern, "SOLID");
    EXPECT_TRUE(solid.solid);
    EXPECT_TRUE(solid.patternLines.empty());
    EXPECT_FALSE(withPattern(hatch, {0, 0}, "ANSI37", 1, 0, 1));
    EXPECT_FALSE(withPattern(hatch, {0, 0}, "NONESISTE", 1, 0, 1));
    for (const double scale : {0.0, -1.0, std::numeric_limits<double>::quiet_NaN(),
                               std::numeric_limits<double>::infinity()}) {
        EXPECT_FALSE(withPattern(hatch, {0, 0}, "ANSI31", scale, 0, 1)) << scale;
    }
    Hatch zero = hatch;
    zero.scale = 0;
    EXPECT_FALSE(withPattern(zero, {0, 0}, "ANSI31", 1, 0, 1));
}

TEST(FilledArea, ARectangleWithAHoleAndTheIslandStyles) {
    Hatch hatch = hatcheditShape(0);
    EXPECT_NEAR(filledArea(hatch), 2400 - (64 * kPi), 1e-9);
    hatch.islands = IslandStyle::Outer;
    EXPECT_NEAR(filledArea(hatch), 2400 - (64 * kPi), 1e-9);
    hatch.islands = IslandStyle::Ignore;
    EXPECT_NEAR(filledArea(hatch), 2400, 1e-9);
    // The hole of RT30, an edge loop without flags: kept by every style when
    // no loop is flagged external.
    EXPECT_NEAR(filledArea(rectangleWithHole()), 80 - (4 * kPi), 1e-9);
}

TEST(FilledArea, AnIslandInAnIslandIsFilledAgain) {
    const Hatch hatch = solidHatch({rectangleLoop({0, 0}, {10, 10}), rectangleLoop({2, 2}, {8, 8}),
                                    rectangleLoop({4, 4}, {6, 6})});
    EXPECT_NEAR(filledArea(hatch), 100 - 36 + 4, 1e-9);
}

}  // namespace
}  // namespace tg::core
