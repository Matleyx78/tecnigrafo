// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/dashes.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/math.hpp"
#include "tg/script/questions.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-12;

std::vector<DashPiece> piecesFor(const Entity& entity, const LinetypePattern& pattern,
                                 double scale = 1.0) {
    const auto pieces = dashes(entity, pattern, scale);
    EXPECT_TRUE(pieces.has_value());
    return pieces.value_or(std::vector<DashPiece>{});
}

void expectLine(const DashPiece& piece, Vec2 start, Vec2 end) {
    const auto* line = std::get_if<DashLine>(&piece);
    EXPECT_NE(line, nullptr);
    if (line != nullptr) {
        EXPECT_TRUE(nearlyEqual(line->start, start, kTolerance));
        EXPECT_TRUE(nearlyEqual(line->end, end, kTolerance));
    }
}

void expectDot(const DashPiece& piece, Vec2 position, Vec2 direction) {
    const auto* dot = std::get_if<DashPoint>(&piece);
    EXPECT_NE(dot, nullptr);
    if (dot != nullptr) {
        EXPECT_TRUE(nearlyEqual(dot->position, position, kTolerance));
        EXPECT_TRUE(nearlyEqual(dot->direction, direction, kTolerance));
        EXPECT_NEAR(dot->direction.length(), 1.0, kTolerance);
    }
}

void expectArc(const DashPiece& piece, Vec2 center, double radius, double start, double end) {
    const auto* arc = std::get_if<DashArc>(&piece);
    EXPECT_NE(arc, nullptr);
    if (arc != nullptr) {
        EXPECT_TRUE(nearlyEqual(arc->center, center, kTolerance));
        EXPECT_NEAR(arc->radius, radius, kTolerance);
        EXPECT_NEAR(arc->startAngle, start, kTolerance);
        EXPECT_NEAR(arc->endAngle, end, kTolerance);
    }
}

TEST(Dashes, ContinuousShortEmptyAndTextFallbacks) {
    const LinetypePattern pattern{{6, -2}};
    EXPECT_FALSE(dashes(Line{{0, 0}, {7, 0}}, pattern, 1));
    EXPECT_FALSE(dashes(Line{{0, 0}, {0, 0}}, pattern, 1));
    EXPECT_FALSE(dashes(Line{{0, 0}, {20, 0}}, {}, 1));
    EXPECT_FALSE(dashes(Line{{0, 0}, {20, 0}}, {{0, 0}}, 1));
    EXPECT_FALSE(dashes(Polyline{}, pattern, 1));
    EXPECT_FALSE(dashes(Polyline{{{{2, 3}}}}, pattern, 1));
    EXPECT_FALSE(dashes(Text{}, pattern, 1));
    EXPECT_FALSE(dashes(Arc{{0, 0}, 1, 0, kPi}, pattern, 1));
    EXPECT_FALSE(dashes(Circle{{0, 0}, 1}, pattern, 1));
    EXPECT_FALSE(dashes(Line{{0, 0}, {20, 0}}, {{1, -1}},
                        std::numeric_limits<double>::max()));
}

TEST(Dashes, EqualLongAndTruncatedLinesHaveNoZeroLengthStroke) {
    const auto equal = piecesFor(Line{{0, 0}, {8, 0}}, {{6, -2}});
    ASSERT_EQ(equal.size(), 1U);
    expectLine(equal[0], {0, 0}, {6, 0});
    const auto gapEnd = piecesFor(Line{{0, 0}, {16, 0}}, {{6, -2}});
    ASSERT_EQ(gapEnd.size(), 2U);
    expectLine(gapEnd[0], {0, 0}, {6, 0});
    expectLine(gapEnd[1], {8, 0}, {14, 0});
    const auto truncated = piecesFor(Line{{0, 0}, {18, 0}}, {{6, -2}});
    ASSERT_EQ(truncated.size(), 3U);
    expectLine(truncated[2], {16, 0}, {18, 0});
    const auto strokeEnd = piecesFor(Line{{0, 0}, {14, 0}}, {{6, -2}});
    ASSERT_EQ(strokeEnd.size(), 2U);
    expectLine(strokeEnd[1], {8, 0}, {14, 0});
}

TEST(Dashes, ScaledPatternFollowsTheLineDirection) {
    const auto pieces = piecesFor(Line{{30, 40}, {0, 0}}, {{6, -2}}, 2);
    ASSERT_EQ(pieces.size(), 4U);
    expectLine(pieces[0], {30, 40}, {22.8, 30.4});
    expectLine(pieces[1], {20.4, 27.2}, {13.2, 17.6});
    expectLine(pieces[3], {1.2, 1.6}, {0, 0});
    EXPECT_FALSE(dashes(Line{{0, 0}, {15, 0}}, {{6, -2}}, 2));
    const auto half = piecesFor(Line{{0, 0}, {8, 0}}, {{6, -2}}, 0.5);
    ASSERT_EQ(half.size(), 2U);
    expectLine(half[1], {4, 0}, {7, 0});
}

TEST(Dashes, InitialAndTerminalDotsAreKeptIncludingConsecutiveDots) {
    const auto pieces = piecesFor(Line{{0, 0}, {16, 0}}, {{6, -2, 0}});
    ASSERT_EQ(pieces.size(), 4U);
    expectDot(pieces[1], {8, 0}, {1, 0});
    expectDot(pieces[3], {16, 0}, {1, 0});
    const auto initial = piecesFor(Line{{0, 0}, {0, 2}}, {{0, 0, -1}});
    ASSERT_EQ(initial.size(), 6U);
    expectDot(initial.front(), {0, 0}, {0, 1});
    expectDot(initial.back(), {0, 2}, {0, 1});
    const auto decimal = piecesFor(Line{{0, 0}, {0.3, 0}}, {{0.1, -0.2, 0}});
    ASSERT_EQ(decimal.size(), 2U);
    expectDot(decimal.back(), {0.3, 0}, {1, 0});
}

TEST(Dashes, LeadingAndOnlyGapsProduceNoSpuriousStrokes) {
    const auto pieces = piecesFor(Line{{0, 0}, {12, 0}}, {{-2, 5}});
    ASSERT_EQ(pieces.size(), 2U);
    expectLine(pieces[0], {2, 0}, {7, 0});
    expectLine(pieces[1], {9, 0}, {12, 0});
    const auto gaps = dashes(Line{{0, 0}, {1e20, 0}}, {{-1}}, 1);
    ASSERT_TRUE(gaps);
    EXPECT_TRUE(gaps->empty());
}

TEST(Dashes, ArcsCrossZeroAndCirclesStartAtZero) {
    const auto arc = piecesFor(Arc{{2, 3}, 2, 1.5 * kPi, 0.5 * kPi}, {{kPi, -kPi}});
    ASSERT_EQ(arc.size(), 1U);
    expectArc(arc[0], {2, 3}, 2, 1.5 * kPi, 0);
    const auto circle = piecesFor(Circle{{4, 5}, 2}, {{kPi, -kPi}});
    ASSERT_EQ(circle.size(), 2U);
    expectArc(circle[0], {4, 5}, 2, 0, 0.5 * kPi);
    expectArc(circle[1], {4, 5}, 2, kPi, 1.5 * kPi);
    const auto truncated = piecesFor(Arc{{0, 0}, 10, 0, kPi}, {{12, -3}});
    ASSERT_EQ(truncated.size(), 3U);
    expectArc(truncated.back(), {0, 0}, 10, 3, kPi);
}

TEST(Dashes, ArcDotsHaveUnitTangents) {
    const auto pieces = piecesFor(Arc{{0, 0}, 2, 0, kPi}, {{0, -kPi}});
    ASSERT_EQ(pieces.size(), 3U);
    expectDot(pieces[0], {2, 0}, {0, 1});
    expectDot(pieces[1], {0, 2}, {-1, 0});
    expectDot(pieces[2], {-2, 0}, {0, -1});
}

TEST(Dashes, PolylineRestartsAndDrawsShortSegmentsWhole) {
    Polyline polyline;
    polyline.addVertex({0, 0});
    polyline.addVertex({10, 0});
    polyline.addVertex({10, 2}, 1);
    polyline.addVertex({12, 2});
    const auto pieces = piecesFor(polyline, {{6, -2}});
    ASSERT_EQ(pieces.size(), 4U);
    expectLine(pieces[0], {0, 0}, {6, 0});
    expectLine(pieces[1], {8, 0}, {10, 0});
    expectLine(pieces[2], {10, 0}, {10, 2});
    expectArc(pieces[3], {11, 2}, 1, kPi, 0);
    Polyline shortOnly;
    shortOnly.addVertex({0, 0});
    shortOnly.addVertex({2, 0});
    shortOnly.addVertex({2, 2});
    EXPECT_FALSE(dashes(shortOnly, {{6, -2}}, 1));
}

TEST(Dashes, ClockwiseBulgesSwapArcEndsAndKeepTravelTangents) {
    Polyline polyline;
    polyline.addVertex({-2, 0}, -1);
    polyline.addVertex({2, 0});
    const auto pieces = piecesFor(polyline, {{kPi, 0, -kPi}});
    ASSERT_EQ(pieces.size(), 2U);
    expectArc(pieces[0], {0, 0}, 2, 0.5 * kPi, kPi);
    expectDot(pieces[1], {0, 2}, {1, 0});
}

TEST(Dashes, ContinuousGenerationSplitsStrokesAcrossLineAndArc) {
    Polyline polyline;
    polyline.addVertex({0, 0});
    polyline.addVertex({20, 0}, -1);
    polyline.addVertex({40, 0});
    polyline.setContinuousLinetype(true);
    const auto pieces = piecesFor(polyline, {{12, -3}});
    ASSERT_EQ(pieces.size(), 5U);
    expectLine(pieces[0], {0, 0}, {12, 0});
    expectLine(pieces[1], {15, 0}, {20, 0});
    expectArc(pieces[2], {30, 0}, 10, kPi - 0.7, kPi);
    expectArc(pieces[3], {30, 0}, 10, kPi - 2.2, kPi - 1);
    expectArc(pieces[4], {30, 0}, 10, 0, kPi - 2.5);
}

TEST(Dashes, ContinuousGenerationCarriesGapsAndEmitsVertexDotsOnce) {
    Polyline polyline;
    polyline.addVertex({0, 0});
    polyline.addVertex({8, 0});
    polyline.addVertex({8, 0});  // zero-length segments do not consume the pattern
    polyline.addVertex({8, 8});
    polyline.setContinuousLinetype(true);
    const auto pieces = piecesFor(polyline, {{6, -2, 0}});
    ASSERT_EQ(pieces.size(), 4U);
    expectLine(pieces[0], {0, 0}, {6, 0});
    expectDot(pieces[1], {8, 0}, {0, 1});
    expectLine(pieces[2], {8, 0}, {8, 6});
    expectDot(pieces[3], {8, 8}, {0, 1});
    polyline = Polyline{{{{0, 0}}, {{7, 0}}, {{7, 3}}}};
    polyline.setContinuousLinetype(true);
    const auto gap = piecesFor(polyline, {{6, -2}});
    ASSERT_EQ(gap.size(), 2U);
    expectLine(gap[1], {7, 1}, {7, 3});
}

TEST(Dashes, ClosedPolylineIncludesTheClosingSegment) {
    Polyline polyline{{{{0, 0}}, {{10, 0}}, {{10, 10}}, {{0, 10}}}, true};
    EXPECT_FALSE(dashes(polyline, {{12, -3}}, 1));
    polyline.setContinuousLinetype(true);
    const auto pieces = piecesFor(polyline, {{12, -3}});
    ASSERT_EQ(pieces.size(), 5U);
    expectLine(pieces[0], {0, 0}, {10, 0});
    expectLine(pieces[1], {10, 0}, {10, 2});
    expectLine(pieces[4], {0, 10}, {0, 0});
    polyline.setClosed(false);
    EXPECT_EQ(piecesFor(polyline, {{12, -3}}).size(), 4U);
}

TEST(Dashes, PieceLimitIncludesPointsAndPiecesSplitAtVertices) {
    const double limit = static_cast<double>(kMaxDashPieces);
    const auto exact = dashes(Line{{0, 0}, {2 * limit, 0}}, {{1, -1}}, 1);
    ASSERT_TRUE(exact);
    EXPECT_EQ(exact->size(), kMaxDashPieces);
    EXPECT_FALSE(dashes(Line{{0, 0}, {2 * limit + 1, 0}}, {{1, -1}}, 1));
    const auto points = dashes(Line{{0, 0}, {limit - 1, 0}}, {{0, -1}}, 1);
    ASSERT_TRUE(points);
    EXPECT_EQ(points->size(), kMaxDashPieces);
    EXPECT_FALSE(dashes(Line{{0, 0}, {limit, 0}}, {{0, -1}}, 1));
    EXPECT_FALSE(dashes(Circle{{0, 0}, limit}, {{1, -1}}, 1));
    Polyline polyline;
    // A single dash spans all these vertices; each segment is still a piece.
    for (std::size_t i = 0; i <= kMaxDashPieces + 1; ++i) {
        polyline.addVertex({static_cast<double>(i), 0});
    }
    polyline.setContinuousLinetype(true);
    EXPECT_FALSE(dashes(polyline, {{limit + 0.5, -0.5}}, 1));
    polyline.setContinuousLinetype(false);
    EXPECT_FALSE(dashes(polyline, {{1, -1}}, 1));  // every segment is continuous
}

TEST(Dashes, EffectiveScaleCombinesDocumentAndEntityValues) {
    Document document = Document::newDrawing();
    const auto type = document.addLinetype({{}, "Custom", "", {{3, -1}}, {}});
    document.setLtscale(2);
    const auto id = document.add(Line{{0, 0}, {16, 0}});
    auto properties = document.find(id)->properties;
    properties.linetype = type;
    properties.linetypeScale = 0.5;
    document.setProperties(id, properties);
    const auto& record = *document.find(id);
    const auto effective = effectiveProperties(document, record);
    const auto pieces = piecesFor(record.entity, effective.linetype->pattern, effective.linetypeScale);
    ASSERT_EQ(pieces.size(), 4U);
    expectLine(pieces[0], {0, 0}, {3, 0});
    expectLine(pieces[3], {12, 0}, {15, 0});
}

TEST(DashQuestions, ResolveEffectiveLinetypeWithoutChangingExistingEntityScales) {
    const auto result = script::answerQuestions(
        "T = LTYPE name=\"Custom\" pattern=3,-1,0\n"
        "L = LAYER name=\"Dashed\" linetype=\"Custom\"\n"
        "A = LINE start=0,0 end=16,0 layer=\"Dashed\" ltscale=2\n"
        "? dashes A -> LINE 0,0 6,0; POINT 8,0; LINE 8,0 14,0; POINT 16,0\n"
        "!current ltscale=0.5\n"
        "? dashes A -> LINE 0,0 6,0; POINT 8,0; LINE 8,0 14,0; POINT 16,0\n"
        "B = LINE start=0,0 end=16,0 layer=\"Dashed\" linetype=\"ByBlock\"\n"
        "? dashes B -> continuous\n"
        "S = LTYPE name=\"Spaces\" pattern=-1\n"
        "C = LINE start=0,0 end=16,0 linetype=\"Spaces\"\n"
        "? dashes C -> none\n");
    const auto* answers = std::get_if<std::vector<script::Answer>>(&result);
    ASSERT_NE(answers, nullptr);
    if (answers != nullptr) {
        ASSERT_EQ(answers->size(), 4U);
        for (const auto& answer : *answers) {
            EXPECT_TRUE(answer.matches) << answer.actual;
        }
    }
}

TEST(DashQuestions, RejectMissingExtraAndUnknownArguments) {
    for (const std::string_view question : {"dashes", "dashes A A", "dashes missing"}) {
        const auto result = script::answerQuestions(
            "A = LINE start=0,0 end=10,0\n? " + std::string{question} + " -> continuous\n");
        EXPECT_TRUE(std::holds_alternative<script::ParseError>(result)) << question;
    }
}

}  // namespace
}  // namespace tg::core
