// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/color.hpp"
#include "tg/core/document.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/hatch_patterns.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

// docs/tappe/0.8/PT42-righe-dei-motivi.md, "Test richiesti".
constexpr double kTolerance = 1e-9;
constexpr double kOnAxis = 1e-12;
constexpr std::uint32_t kExternal = 1;
constexpr std::uint32_t kOutermost = 16;

HatchLoop rectangleLoop(const Vec2& a, const Vec2& b, std::uint32_t flags = kExternal) {
    HatchLoop loop;
    loop.flags = flags | 2U;
    loop.polyline = Polyline{{{a}, {{b.x, a.y}}, {b}, {{a.x, b.y}}}, true};
    return loop;
}

HatchLoop circleLoop(const Vec2& center, double radius) {
    HatchLoop loop;
    loop.edges = {HatchEdge{Arc{center, radius, 0.0, kTwoPi}}};
    return loop;
}

Hatch patternHatch(std::vector<PatternLine> lines, std::vector<HatchLoop> loops) {
    Hatch hatch;
    hatch.pattern = "USER";
    hatch.patternLines = std::move(lines);
    hatch.loops = std::move(loops);
    return hatch;
}

Hatch metricHatch(const char* name, std::vector<HatchLoop> loops) {
    return patternHatch(predefinedPatternLines(name, 1.0, 0.0, kMetricMeasurement),
                        std::move(loops));
}

PatternLine family(double degrees, const Vec2& origin, const Vec2& offset,
                   std::vector<double> dashes = {}) {
    return PatternLine{degreesToRadians(degrees), origin, offset, std::move(dashes)};
}

std::vector<Line> linesOf(const Hatch& hatch) {
    const PatternLines pattern = patternLines(hatch);
    EXPECT_EQ(pattern.outcome, PatternOutcome::Done);
    EXPECT_TRUE(pattern.complete);
    return pattern.lines;
}

void expectLine(const Line& line, const Vec2& start, const Vec2& end) {
    EXPECT_NEAR(line.start.x, start.x, kTolerance);
    EXPECT_NEAR(line.start.y, start.y, kTolerance);
    EXPECT_NEAR(line.end.x, end.x, kTolerance);
    EXPECT_NEAR(line.end.y, end.y, kTolerance);
}

void expectLines(const std::vector<Line>& lines,
                 const std::vector<std::pair<Vec2, Vec2>>& expected) {
    ASSERT_EQ(lines.size(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        SCOPED_TRACE(i);
        expectLine(lines[i], expected[i].first, expected[i].second);
    }
}

Hatch transformedHatch(const Hatch& hatch, const Similarity& similarity) {
    const Document document;
    const TransformOutcome outcome =
        transformedOf(document, hatch, similarity, MirrorText::Readable);
    if (!outcome.entity || !std::holds_alternative<Hatch>(*outcome.entity)) {
        ADD_FAILURE() << "expected a hatch";
        return hatch;
    }
    return std::get<Hatch>(*outcome.entity);
}

/// The number of lines of length 10 that lie on the line x = `x` (vertical)
/// or y = `y` (horizontal), within the rounding of the ends of solid-space.
std::size_t sidesOfLengthTen(const std::vector<Line>& lines, std::optional<double> x,
                             std::optional<double> y) {
    std::size_t count = 0;
    for (const Line& line : lines) {
        const auto on = [](double a, double b) {
            return std::abs(a - b) <= kOnAxis;
        };
        const bool along = x ? (on(line.start.x, *x) && on(line.end.x, *x))
                             : (on(line.start.y, *y) && on(line.end.y, *y));
        if (along && std::abs(distance(line.start, line.end) - 10.0) <= kTolerance) {
            ++count;
        }
    }
    return count;
}

// --- The bridge and the island styles (points 1 and 2) ---------------------

TEST(HatchPattern, RegionLoopsFollowTheIslandStyle) {
    Hatch hatch = patternHatch(
        {}, {rectangleLoop({0, 0}, {12, 12}, kExternal),
             rectangleLoop({2, 2}, {10, 10}, kOutermost), rectangleLoop({4, 4}, {8, 8}, 0)});
    EXPECT_EQ(regionLoops(hatch), (std::vector<std::size_t>{0, 1, 2}));
    hatch.islands = IslandStyle::Outer;
    EXPECT_EQ(regionLoops(hatch), (std::vector<std::size_t>{0, 1}));
    hatch.islands = IslandStyle::Ignore;
    EXPECT_EQ(regionLoops(hatch), (std::vector<std::size_t>{0}));
    // No loop flagged external: all of them, whatever the style.
    for (HatchLoop& loop : hatch.loops) {
        loop.flags = 2;
    }
    EXPECT_EQ(regionLoops(hatch), (std::vector<std::size_t>{0, 1, 2}));
    hatch.islands = IslandStyle::Outer;
    EXPECT_EQ(regionLoops(hatch), (std::vector<std::size_t>{0, 1, 2}));
}

TEST(HatchPattern, IslandStylesCutTheLines) {
    // One horizontal line, y = 6, across nested squares flagged 1, 16 and 0
    // as AutoCAD writes them.
    Hatch hatch =
        patternHatch({family(0.0, {0, 6}, {0, 100})}, {rectangleLoop({0, 0}, {12, 12}, kExternal),
                                                       rectangleLoop({2, 2}, {10, 10}, kOutermost),
                                                       rectangleLoop({4, 4}, {8, 8}, 0)});
    expectLines(linesOf(hatch), {{{0, 6}, {2, 6}}, {{4, 6}, {8, 6}}, {{10, 6}, {12, 6}}});
    hatch.islands = IslandStyle::Outer;
    expectLines(linesOf(hatch), {{{0, 6}, {2, 6}}, {{10, 6}, {12, 6}}});
    hatch.islands = IslandStyle::Ignore;
    expectLines(linesOf(hatch), {{{0, 6}, {12, 6}}});
    for (HatchLoop& loop : hatch.loops) {
        loop.flags = 2;
    }
    expectLines(linesOf(hatch), {{{0, 6}, {2, 6}}, {{4, 6}, {8, 6}}, {{10, 6}, {12, 6}}});
}

TEST(HatchPattern, ASolidFillKeepsOnlyTheLoopsOfItsStyle) {
    const Document document = Document::newDrawing();
    Hatch hatch = patternHatch(
        {}, {rectangleLoop({0, 0}, {12, 12}, kExternal),
             rectangleLoop({2, 2}, {10, 10}, kOutermost), rectangleLoop({4, 4}, {8, 8}, 0)});
    hatch.pattern = "SOLID";
    hatch.solid = true;
    const auto loopsOf = [&](IslandStyle style) {
        hatch.islands = style;
        const std::vector<GeneratedItem> items =
            generateOf(document, EntityRecord{EntityId{1}, hatch}, hatch);
        EXPECT_EQ(items.size(), 1U);
        if (items.size() != 1 || !std::holds_alternative<FilledRegion>(items[0].shape)) {
            return std::size_t{0};
        }
        return std::get<FilledRegion>(items[0].shape).loops.size();
    };
    EXPECT_EQ(loopsOf(IslandStyle::Normal), 3U);
    EXPECT_EQ(loopsOf(IslandStyle::Outer), 2U);
    EXPECT_EQ(loopsOf(IslandStyle::Ignore), 1U);
}

// --- The lines of a pattern (point 3) ---------------------------------------

TEST(HatchPattern, Ansi31OnASquareInTheOrderOfK) {
    // Metric ANSI31: lines at 45 degrees, step 3.175, so y - x = k * 3.175 *
    // sqrt(2) for k = -2 ... 2.
    const double c = 3.175 * std::sqrt(2.0);
    Hatch hatch = metricHatch("ANSI31", {rectangleLoop({0, 0}, {10, 10})});
    expectLines(linesOf(hatch), {{{2 * c, 0}, {10, 10 - (2 * c)}},
                                 {{c, 0}, {10, 10 - c}},
                                 {{0, 0}, {10, 10}},
                                 {{0, c}, {10 - c, 10}},
                                 {{0, 2 * c}, {10 - (2 * c), 10}}});

    // A hole of radius 2 in the middle: the line y = x crosses it and gives
    // two stretches; the others miss it.
    hatch.loops.push_back(circleLoop({5, 5}, 2));
    const double r = std::sqrt(2.0);
    expectLines(linesOf(hatch), {{{2 * c, 0}, {10, 10 - (2 * c)}},
                                 {{c, 0}, {10, 10 - c}},
                                 {{0, 0}, {5 - r, 5 - r}},
                                 {{5 + r, 5 + r}, {10, 10}},
                                 {{0, c}, {10 - c, 10}},
                                 {{0, 2 * c}, {10 - (2 * c), 10}}});
}

TEST(HatchPattern, NetWithItsOriginInACornerDrawsTheLinesAlongTheSides) {
    const Hatch hatch = metricHatch("NET", {rectangleLoop({0, 0}, {10, 10})});
    const std::vector<Line> lines = linesOf(hatch);
    ASSERT_EQ(lines.size(), 8U);
    // The horizontal family first, k = 0 on the bottom side; then the
    // vertical one, k = -3 ... 0, the last on the left side.
    expectLine(lines[0], {0, 0}, {10, 0});
    expectLine(lines[7], {0, 0}, {0, 10});
    EXPECT_EQ(sidesOfLengthTen(lines, std::nullopt, 0.0), 1U);
    EXPECT_EQ(sidesOfLengthTen(lines, 0.0, std::nullopt), 1U);
}

TEST(HatchPattern, TheVerticalNetLineIsOnTheRightAndLeftSides) {
    for (const bool left : {false, true}) {
        SCOPED_TRACE(left);
        const Vec2 low = left ? Vec2{0, 0} : Vec2{-10, 0};
        const Hatch hatch = metricHatch("NET", {rectangleLoop(low, low + Vec2{10, 10})});
        EXPECT_EQ(sidesOfLengthTen(linesOf(hatch), 0.0, std::nullopt), 1U);

        // The family at 90 degrees plus a whole turn: still exactly vertical.
        Hatch turned = hatch;
        turned.patternLines[1].angle += kTwoPi;
        EXPECT_EQ(sidesOfLengthTen(linesOf(turned), 0.0, std::nullopt), 1U);

        // MIRROR on the x axis: the family at -90 degrees, direction (0, -1).
        const std::optional<Similarity> mirror = Similarity::reflection({0, 0}, {1, 0});
        ASSERT_TRUE(mirror.has_value());
        const Hatch mirrored = transformedHatch(hatch, *mirror);
        EXPECT_EQ(cardinalSinCos(mirrored.patternLines[1].angle).cosine, 0.0);
        EXPECT_EQ(cardinalSinCos(mirrored.patternLines[1].angle).sine, -1.0);
        EXPECT_EQ(sidesOfLengthTen(linesOf(mirrored), 0.0, std::nullopt), 1U);
    }
}

TEST(HatchPattern, QuarterTurnsKeepTheLineAlongTheSide) {
    // The square as a closed polyline, with one vertical family at the step of
    // NET whose line k = 0 is its left side, rotated with ROTATE.
    const Hatch hatch =
        patternHatch({family(90.0, {0, 0}, {-3.175, 0})}, {rectangleLoop({0, 0}, {10, 10})});

    // Every turn takes the left side to y = 0.
    struct Turn {
        double degrees;
        Vec2 center;
    };

    for (const Turn& turn : {Turn{90.0, {0, 0}}, Turn{-90.0, {0, 0}}, Turn{90.0, {5, 5}}}) {
        SCOPED_TRACE(turn.degrees);
        const Hatch rotated = transformedHatch(
            hatch, Similarity::rotation(degreesToRadians(turn.degrees), turn.center));
        const SinCos d = cardinalSinCos(rotated.patternLines[0].angle);
        EXPECT_EQ(d.sine, 0.0);
        EXPECT_EQ(std::abs(d.cosine), 1.0);
        const std::vector<Line> lines = linesOf(rotated);
        EXPECT_EQ(sidesOfLengthTen(lines, std::nullopt, 0.0), 1U);
        // The inside line 3.175 from that side stays inside, of length 10.
        std::size_t inner = 0;
        for (const Line& line : lines) {
            const double offset = std::abs(line.start.y);
            if (std::abs(offset - 3.175) <= kTolerance &&
                std::abs(line.end.y - line.start.y) <= kOnAxis &&
                std::abs(distance(line.start, line.end) - 10.0) <= kTolerance) {
                ++inner;
            }
        }
        EXPECT_EQ(inner, 1U);
    }
}

TEST(HatchPattern, DashesStartFromTheOriginOfEachLine) {
    // Dash 5, space 2.5, dot, space 2.5: period 10. Horizontal lines y = 4k,
    // origin (0, 0), offset (1.5, 4): line k starts its dashes at
    // p_k = (1.5 k, 4 k), so line 2 is shifted by 1.5 against line 1.
    const Hatch hatch = patternHatch({family(0.0, {0, 0}, {1.5, 4}, {5, -2.5, 0, -2.5})},
                                     {rectangleLoop({0, 1}, {20, 9})});
    expectLines(linesOf(hatch), {{{1.5, 4}, {6.5, 4}},
                                 {{9, 4}, {9, 4}},
                                 {{11.5, 4}, {16.5, 4}},
                                 {{19, 4}, {19, 4}},
                                 {{0.5, 8}, {0.5, 8}},
                                 {{3, 8}, {8, 8}},
                                 {{10.5, 8}, {10.5, 8}},
                                 {{13, 8}, {18, 8}}});
}

TEST(HatchPattern, DashesOnAnObliqueLineKeepTheirPhase) {
    // Lines at 30 degrees; offset = 1.5 d + 4 n.
    const double degrees = 30.0;
    const SinCos sc = cardinalSinCos(degreesToRadians(degrees));
    const Vec2 d{sc.cosine, sc.sine};
    const Vec2 n{-sc.sine, sc.cosine};
    const Vec2 origin{1, 2};
    const Vec2 offset = (d * 1.5) + (n * 4.0);
    const Hatch hatch = patternHatch({family(degrees, origin, offset, {5, -2.5, 0, -2.5})},
                                     {rectangleLoop({-20, -20}, {20, 20})});
    const std::vector<Line> lines = linesOf(hatch);
    ASSERT_FALSE(lines.empty());
    std::size_t dashes = 0;
    std::size_t dots = 0;
    for (const Line& line : lines) {
        // The line k it lies on, and the position of its start from p_k.
        const double k = std::round(dot(line.start - origin, n) / 4.0);
        const Vec2 base = origin + (offset * k);
        const double u = dot(line.start - base, d);
        const double length = distance(line.start, line.end);
        const double phase = u - (10.0 * std::floor(u / 10.0));
        const bool clipped = std::abs(line.start.x) >= 20 - kTolerance ||
                             std::abs(line.start.y) >= 20 - kTolerance ||
                             std::abs(line.end.x) >= 20 - kTolerance ||
                             std::abs(line.end.y) >= 20 - kTolerance;
        if (clipped) {
            continue;
        }
        if (length == 0.0) {
            EXPECT_NEAR(phase, 7.5, 1e-6) << k;
            ++dots;
        } else {
            EXPECT_NEAR(length, 5.0, 1e-6) << k;
            EXPECT_TRUE(std::abs(phase) <= 1e-6 || std::abs(phase - 10.0) <= 1e-6) << k << " " << u;
            ++dashes;
        }
    }
    EXPECT_GT(dashes, 10U);
    EXPECT_GT(dots, 10U);
}

TEST(HatchPattern, ADashThatStartsBeforeTheStretchStartsAtItsEnd) {
    // Line y = 4 from x = 0 to 10, dashes measured from (-2, 4): the dash
    // [-2, 3] begins with the end of the stretch itself.
    const Hatch hatch =
        patternHatch({family(0.0, {-2, 0}, {0, 4}, {5, -5})}, {rectangleLoop({0, 2}, {10, 6})});
    expectLines(linesOf(hatch), {{{0, 4}, {3, 4}}, {{8, 4}, {10, 4}}});
}

TEST(HatchPattern, PeriodsOfZeroAndGapsOnly) {
    Hatch hatch =
        patternHatch({family(0.0, {0, 0}, {0, 4}, {0, 0})}, {rectangleLoop({0, 2}, {10, 6})});
    expectLines(linesOf(hatch), {{{0, 4}, {10, 4}}});
    hatch.patternLines[0].dashes = {-1, -2};
    EXPECT_TRUE(linesOf(hatch).empty());
}

// --- Limits and incomplete results ------------------------------------------

TEST(HatchPattern, ATinyStepOnALargeAreaIsTooDense) {
    Hatch hatch =
        patternHatch({family(45.0, {0, 0}, {-1e-6, 1e-6})}, {rectangleLoop({0, 0}, {100, 100})});
    PatternLines pattern = patternLines(hatch);
    EXPECT_EQ(pattern.outcome, PatternOutcome::TooDense);
    EXPECT_TRUE(pattern.lines.empty());
    const Document document = Document::newDrawing();
    EXPECT_TRUE(generateOf(document, EntityRecord{EntityId{1}, hatch}, hatch).empty());

    // Few lines, but more than a million dashes.
    hatch.patternLines = {family(0.0, {0, 0}, {0, 200}, {1e-5, -1e-5})};
    pattern = patternLines(hatch);
    EXPECT_EQ(pattern.outcome, PatternOutcome::TooDense);
    EXPECT_TRUE(pattern.lines.empty());

    // A dash and a dot every 0.5 along 250000: exactly a million are drawn,
    // one more is too many.
    hatch.loops = {rectangleLoop({0, -1}, {250000, 1})};
    hatch.patternLines = {family(0.0, {0, 0}, {0, 1e6}, {0.25, 0.0, -0.25})};
    pattern = patternLines(hatch);
    EXPECT_EQ(pattern.outcome, PatternOutcome::Done);
    EXPECT_EQ(pattern.lines.size(), kMaxHatchLines);
    hatch.loops = {rectangleLoop({0, -1}, {250000.125, 1})};
    pattern = patternLines(hatch);
    EXPECT_EQ(pattern.outcome, PatternOutcome::TooDense);
    EXPECT_TRUE(pattern.lines.empty());
}

TEST(HatchPattern, AFamilyWithAZeroStepGivesNoLines) {
    // The offset runs along the lines: no step.
    const Hatch hatch = patternHatch({family(0.0, {0, 0}, {3, 0}), family(0.0, {0, 5}, {0, 100})},
                                     {rectangleLoop({0, 0}, {10, 10})});
    expectLines(linesOf(hatch), {{{0, 5}, {10, 5}}});
}

TEST(HatchPattern, NoLoopNoLine) {
    const Hatch hatch = patternHatch({family(0.0, {0, 0}, {0, 1})}, {});
    const PatternLines pattern = patternLines(hatch);
    EXPECT_EQ(pattern.outcome, PatternOutcome::Done);
    EXPECT_TRUE(pattern.lines.empty());
    EXPECT_TRUE(pattern.complete);
}

TEST(HatchPattern, ALoopLeftOutBySolidSpaceMakesTheResultIncomplete) {
    // A loop whose vertices are all the same point is exactly a point.
    HatchLoop point;
    point.flags = 2;
    point.polyline = Polyline{{{{5, 5}}, {{5, 5}}, {{5, 5}}}, true};
    const Hatch hatch =
        patternHatch({family(0.0, {0, 5}, {0, 100})}, {rectangleLoop({0, 0}, {10, 10}), point});
    const PatternLines pattern = patternLines(hatch);
    EXPECT_EQ(pattern.outcome, PatternOutcome::Done);
    EXPECT_FALSE(pattern.complete);
    expectLines(pattern.lines, {{{0, 5}, {10, 5}}});
}

// --- The generated geometry (point 4) ---------------------------------------

TEST(HatchPattern, TheLinesAreDrawnContinuousWithTheOtherPropertiesOfTheHatch) {
    Document document = Document::newDrawing();
    Linetype dashed;
    dashed.name = "Dashed";
    dashed.pattern.elements = {10.0, -5.0};
    const LinetypeId dashedId = document.addLinetype(std::move(dashed));
    const Hatch hatch = metricHatch("ANSI31", {rectangleLoop({0, 0}, {10, 10})});
    EntityRecord record{EntityId{1}, hatch};
    record.properties.linetype = dashedId;
    record.properties.color = Color::fromIndex(1);
    const std::vector<GeneratedItem> items = generateOf(document, record, hatch);
    ASSERT_EQ(items.size(), 5U);
    for (const GeneratedItem& item : items) {
        EXPECT_TRUE(std::holds_alternative<Line>(item.shape));
        ASSERT_NE(item.properties.linetype, nullptr);
        EXPECT_EQ(item.properties.linetype->name, "Continuous");
        EXPECT_EQ(item.properties.color, aciToRgb(1));
    }

    // A document without tables has no Continuous record: the same.
    const Document bare;
    for (const GeneratedItem& item : generateOf(bare, record, hatch)) {
        ASSERT_NE(item.properties.linetype, nullptr);
        EXPECT_EQ(item.properties.linetype->name, "Continuous");
    }
}

}  // namespace
}  // namespace tg::core
