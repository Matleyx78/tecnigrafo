// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/pedit_join.hpp"

#include "tg/core/join.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

namespace tg::core {
namespace {

const ToleranceRule kMillimetres{4};
constexpr double kEps = 1e-9;

Polyline open(std::vector<PolylineVertex> vertices) {
    return Polyline{std::move(vertices)};
}

std::vector<PolylineVertex> verticesOf(const Polyline& polyline) {
    return {polyline.vertices().begin(), polyline.vertices().end()};
}

void expectVertices(const Polyline& polyline, const std::vector<PolylineVertex>& expected) {
    ASSERT_EQ(polyline.vertexCount(), expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto& vertex = polyline.vertices()[i];
        EXPECT_NEAR(vertex.position.x, expected[i].position.x, kEps) << "vertex " << i;
        EXPECT_NEAR(vertex.position.y, expected[i].position.y, kEps) << "vertex " << i;
        EXPECT_NEAR(vertex.bulge, expected[i].bulge, kEps) << "vertex " << i;
        EXPECT_EQ(vertex.startWidth, expected[i].startWidth) << "vertex " << i;
        EXPECT_EQ(vertex.endWidth, expected[i].endWidth) << "vertex " << i;
    }
}

void expectNothingJoined(const GapJoinResult& result, const Polyline& source) {
    EXPECT_TRUE(result.consumed.empty());
    EXPECT_EQ(result.added, 0U);
    EXPECT_EQ(result.polyline, source);
}

using Indices = std::vector<std::size_t>;

TEST(JoinWithGaps, TwoLinesOneApartInTheThreeTypes) {
    const Polyline source = open({{{0, 0}}, {{2, 0}}});
    const std::array<Entity, 1> others{Line{{2, 1}, {2, 4}}};

    const auto extended = joinWithGaps(source, others, 2.0, JoinType::Extend, kMillimetres);
    expectVertices(extended.polyline, {{{0, 0}}, {{2, 0}}, {{2, 4}}});
    EXPECT_EQ(extended.added, 1U);
    EXPECT_EQ(extended.consumed, Indices{0});

    const auto added = joinWithGaps(source, others, 2.0, JoinType::Add, kMillimetres);
    expectVertices(added.polyline, {{{0, 0}}, {{2, 0}}, {{2, 1}}, {{2, 4}}});
    EXPECT_EQ(added.added, 2U);
    EXPECT_EQ(added.consumed, Indices{0});

    const auto both = joinWithGaps(source, others, 2.0, JoinType::Both, kMillimetres);
    EXPECT_EQ(both.polyline, extended.polyline);
    EXPECT_EQ(both.added, 1U);
}

TEST(JoinWithGaps, OverlappingLinesWithoutACommonEndBridgeOnlyWithBoth) {
    const Polyline source = open({{{0, 0}}, {{2, 0}}});
    const std::array<Entity, 1> others{Line{{1, 0}, {3, 0}}};
    expectNothingJoined(joinWithGaps(source, others, 1.0, JoinType::Extend, kMillimetres), source);
    const auto both = joinWithGaps(source, others, 1.0, JoinType::Both, kMillimetres);
    expectVertices(both.polyline, {{{0, 0}}, {{2, 0}}, {{1, 0}}, {{3, 0}}});
    EXPECT_EQ(both.added, 2U);
}

TEST(JoinWithGaps, OverlappingLinesAtTheStartOfTheSource) {
    const Polyline source = open({{{0, 0}}, {{4, 0}}});
    const std::array<Entity, 1> others{Line{{1, 0}, {-1, 0}}};
    expectNothingJoined(joinWithGaps(source, others, 1.0, JoinType::Extend, kMillimetres), source);
    // Both ends are 1 from the start: it attaches by its start, reversed.
    const auto both = joinWithGaps(source, others, 1.0, JoinType::Both, kMillimetres);
    expectVertices(both.polyline, {{{-1, 0}}, {{1, 0}}, {{0, 0}}, {{4, 0}}});
    EXPECT_EQ(both.added, 2U);
}

TEST(JoinWithGaps, AnArcExtendsToTheNearerPointOfItsCircle) {
    // A quarter circle of centre (0,0), radius 1, from (1,0) to (0,1); the
    // line x = -0.5 meets the circle at (-0.5, +-sqrt(3)/2): the upper point
    // is nearer, so the arc turns by 120 degrees, bulge tan(30 degrees).
    const Polyline source = open({{{1, 0}, tan(kPi / 8.0)}, {{0, 1}}});
    const std::array<Entity, 1> others{Line{{-0.5, 2}, {-0.5, 4}}};
    const auto result = joinWithGaps(source, others, 2.0, JoinType::Extend, kMillimetres);
    const double height = std::numbers::sqrt3 / 2.0;
    expectVertices(result.polyline,
                   {{{1, 0}, std::numbers::inv_sqrt3}, {{-0.5, height}}, {{-0.5, 4}}});
    EXPECT_EQ(result.added, 1U);
}

TEST(JoinWithGaps, ParallelLinesBridgeOnlyWithBoth) {
    const Polyline source = open({{{0, 0}}, {{1, 0}}});
    const std::array<Entity, 1> others{Line{{2, 1}, {3, 1}}};
    expectNothingJoined(joinWithGaps(source, others, 2.0, JoinType::Extend, kMillimetres), source);
    const auto both = joinWithGaps(source, others, 2.0, JoinType::Both, kMillimetres);
    expectVertices(both.polyline, {{{0, 0}}, {{1, 0}}, {{2, 1}}, {{3, 1}}});
    EXPECT_EQ(both.added, 2U);
}

TEST(JoinWithGaps, ClosesWhenTheFreeEndReachesTheOtherAndStops) {
    const Polyline source = open({{{0, 0}}, {{4, 0}}, {{4, 4}}});
    const std::array<Entity, 2> others{Line{{3, 4}, {0, 0}}, Line{{0, 0}, {-1, 0}}};
    const auto result = joinWithGaps(source, others, 1.0, JoinType::Add, kMillimetres);
    EXPECT_TRUE(result.polyline.isClosed());
    expectVertices(result.polyline, {{{0, 0}}, {{4, 0}}, {{4, 4}}, {{3, 4}}});
    EXPECT_EQ(result.consumed, Indices{0});
    EXPECT_EQ(result.added, 2U);
}

TEST(JoinWithGaps, FuzzZeroJoinsAsJoin) {
    const Polyline source = open({{{0, 0}, 0.0, 1.0, 1.0}, {{2, 0}}});
    const std::array<Entity, 3> others{Line{{5, 5}, {6, 6}}, Arc{{3, 0}, 1.0, 0.0, kPi},
                                       Line{{0, 0}, {0, -3}}};
    const auto result = joinWithGaps(source, others, 0.0, JoinType::Extend, kMillimetres);
    const std::array<Entity, 4> all{source, others[0], others[1], others[2]};
    const auto joined = join(all, kMillimetres);
    if (!joined || !joined->source) {
        ADD_FAILURE() << "join() joined nothing";
        return;
    }
    EXPECT_EQ(result.polyline, std::get<Polyline>(*joined->source));
    EXPECT_EQ(result.consumed, Indices({1, 2}));
    EXPECT_EQ(result.added, 2U);
    // An object farther than fuzz never joins, whatever the type.
    for (const JoinType type : {JoinType::Extend, JoinType::Add, JoinType::Both}) {
        EXPECT_EQ(joinWithGaps(source, others, 0.0, type, kMillimetres).consumed, Indices({1, 2}));
    }
}

TEST(JoinWithGaps, CoincidentObjectsComeBeforeTheRankingEvenBelowTheTolerance) {
    const Polyline source = open({{{0, 0}}, {{1, 0}}});
    // Within the tolerance of millimetres, yet farther than fuzz.
    const std::array<Entity, 2> others{Line{{1.5, 0}, {1.5, 1}}, Line{{1.0000000001, 0}, {2, 0}}};
    const auto result = joinWithGaps(source, others, 1e-12, JoinType::Add, kMillimetres);
    EXPECT_EQ(result.consumed, Indices{1});
    EXPECT_EQ(result.added, 1U);
}

TEST(JoinWithGaps, ACollinearCandidateNeitherExtendsNorBridgesWithExtend) {
    const Polyline source = open({{{-4, 0}}, {{0, 0}}});
    const std::array<Entity, 1> others{Line{{1, 0}, {3, 0}}};
    expectNothingJoined(joinWithGaps(source, others, 2.0, JoinType::Extend, kMillimetres), source);
    const auto both = joinWithGaps(source, others, 2.0, JoinType::Both, kMillimetres);
    expectVertices(both.polyline, {{{-4, 0}}, {{0, 0}}, {{1, 0}}, {{3, 0}}});
    EXPECT_EQ(both.added, 2U);
}

TEST(JoinWithGaps, AParallelFirstCandidateLetsTheSecondJoin) {
    const Polyline source = open({{{0, 0}}, {{1, 0}}});
    const std::array<Entity, 2> others{Line{{2, -1}, {4, -1}}, Line{{2, 1}, {2, 3}}};
    const auto result = joinWithGaps(source, others, 2.0, JoinType::Extend, kMillimetres);
    expectVertices(result.polyline, {{{0, 0}}, {{2, 0}}, {{2, 3}}});
    EXPECT_EQ(result.consumed, Indices{1});
    EXPECT_EQ(result.added, 1U);
}

TEST(JoinWithGaps, ACandidateWithBothEndsAsNearAttachesByItsStart) {
    const Polyline source = open({{{0, 2}}, {{0, 1}}});
    const std::array<Entity, 1> others{Line{{-1, 0}, {1, 0}}};
    const auto result = joinWithGaps(source, others, 2.0, JoinType::Add, kMillimetres);
    expectVertices(result.polyline, {{{0, 2}}, {{0, 1}}, {{-1, 0}}, {{1, 0}}});
    EXPECT_EQ(result.added, 2U);
}

TEST(JoinWithGaps, AnExtendedSegmentKeepsTheWidthOfItsEnd) {
    const Polyline source = open({{{0, 0}, 0.0, 1.0, 3.0}, {{1, 0}}});
    const std::array<Entity, 1> others{Line{{2, 1}, {2, 3}}};
    const auto result = joinWithGaps(source, others, 2.0, JoinType::Extend, kMillimetres);
    expectVertices(result.polyline, {{{0, 0}, 0.0, 1.0, 3.0}, {{2, 0}}, {{2, 3}}});
}

TEST(JoinWithGaps, ABridgeAndAnObjectOfTwoSegmentsAddThree) {
    const Polyline source = open({{{0, 0}}, {{1, 0}}});
    const std::array<Entity, 1> others{open({{{2, 0}}, {{3, 0}}, {{4, 0}}})};
    const auto result = joinWithGaps(source, others, 1.0, JoinType::Add, kMillimetres);
    expectVertices(result.polyline, {{{0, 0}}, {{1, 0}}, {{2, 0}}, {{3, 0}}, {{4, 0}}});
    EXPECT_EQ(result.added, 3U);
}

TEST(JoinWithGaps, ClosedPolylinesAndOtherTypesNeverJoin) {
    const Polyline source = open({{{0, 0}}, {{1, 0}}});
    const std::array<Entity, 2> others{Polyline{{{{1, 0}}, {{2, 0}}, {{2, 1}}}, true},
                                       Circle{{2, 0}, 1.0}};
    expectNothingJoined(joinWithGaps(source, others, 2.0, JoinType::Add, kMillimetres), source);
}

TEST(JoinWithGaps, TheLastVertexKeepsItsDataWhenNothingAttachesThere) {
    const Polyline source = open({{{0, 0}}, {{4, 0}, 0.5, 2.0, 3.0}});
    const std::array<Entity, 1> others{Line{{-2, 0}, {-1, 0}}};
    const auto result = joinWithGaps(source, others, 1.0, JoinType::Add, kMillimetres);
    expectVertices(result.polyline, {{{-2, 0}}, {{-1, 0}}, {{0, 0}}, {{4, 0}, 0.5, 2.0, 3.0}});
}

TEST(JoinWithGaps, ReversingTwiceAnOpenPolylineWithDataOnItsLastVertex) {
    const Polyline polyline = open({{{0, 0}, 0.5, 1.0, 2.0}, {{3, 0}, -1.0, 4.0, 5.0}});
    EXPECT_EQ(verticesOf(reversed(polyline)),
              (std::vector<PolylineVertex>{{{3, 0}, -0.5, 2.0, 1.0}, {{0, 0}, -1.0, 4.0, 5.0}}));
    EXPECT_EQ(reversed(reversed(polyline)), polyline);
}

}  // namespace
}  // namespace tg::core
