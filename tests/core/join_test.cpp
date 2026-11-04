// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/join.hpp"

#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <span>
#include <vector>

namespace tg::core {
namespace {

const ToleranceRule kMillimetres{4};
constexpr double kDomain = 0x1p50;

void expectPolyline(const std::optional<JoinResult>& result, bool source, bool closed,
                    const std::vector<PolylineVertex>& vertices) {
    if (!result) {
        ADD_FAILURE() << "nothing joined";
        return;
    }
    const auto& entity = source ? result->source : result->created;
    if (!entity) {
        ADD_FAILURE() << "missing joined polyline";
        return;
    }
    const auto* polyline = std::get_if<Polyline>(&*entity);
    ASSERT_NE(polyline, nullptr);
    EXPECT_EQ(polyline->isClosed(), closed);
    EXPECT_EQ(std::vector<PolylineVertex>(polyline->vertices().begin(), polyline->vertices().end()),
              vertices);
}

void expectBoth(const std::span<const Entity> entities, bool closed,
                const std::vector<PolylineVertex>& vertices) {
    expectPolyline(join(entities, kMillimetres), true, closed, vertices);
    expectPolyline(joinToSource(entities, kMillimetres), true, closed, vertices);
}

TEST(Join, AlignedLinesUseTheSourcesDirectionAndItsEndWinsTies) {
    const std::array<Entity, 3> entities{Line{{10, 0}, {0, 0}}, Line{{0, 5e-7}, {5, 0}},
                                         Line{{20, 0}, {30, 0}}};
    for (const auto& result : {join(entities, kMillimetres), joinToSource(entities, kMillimetres)}) {
        if (!result) {
            ADD_FAILURE() << "aligned lines did not join";
            continue;
        }
        EXPECT_EQ(result->source, (Entity{Line{{30, 0}, {0, 0}}}));
        EXPECT_EQ(result->consumed, (std::vector<std::size_t>{1, 2}));
    }
}

TEST(Join, PrefersTheCandidatesStartAndReversesWidthsAndBulgeAtTheHead) {
    const Polyline source{{{{0, 0}}, {{10, 0}}}};
    const Polyline candidate{{{{2e-7, 0}, 0.5, 2, 3}, {{-10, 10}}, {{-2e-7, 0}}}};
    const std::array<Entity, 2> entities{source, candidate};
    expectBoth(entities, false, {{{-2e-7, 0}}, {{-10, 10}, -0.5, 3, 2}, {{0, 0}}, {{10, 0}}});
}

TEST(Join, InteractiveFallbackUsesAChainAndStopsWhenItCloses) {
    const std::array<Entity, 5> entities{Line{{0, 0}, {10, 0}}, Line{{20, 0}, {30, 0}},
                                         Line{{10, 0}, {10, 10}}, Line{{10, 10}, {0, 0}},
                                         Line{{0, 0}, {-10, 0}}};
    const auto result = join(entities, kMillimetres);
    expectPolyline(result, false, true, {{{0, 0}}, {{10, 0}}, {{10, 10}}});
    if (result) {
        EXPECT_EQ(result->consumed, (std::vector<std::size_t>{2, 3}));
    }
    const std::array<Entity, 2> closed{entities[0], Polyline{{{{10, 0}}, {{20, 0}}, {{20, 10}}}, true}};
    EXPECT_FALSE(join(closed, kMillimetres));
    EXPECT_FALSE(joinToSource(closed, kMillimetres));
}

TEST(Join, ClassifiesTheTranslatedSegmentBeforeMovingItsEnd) {
    const Polyline source{{{{0, kDomain}, -9e-13}, {{10000, kDomain}}}};
    const std::array<Entity, 2> entities{source, Line{{10000, kDomain}, {10000, kDomain - 10000}}};
    expectBoth(entities, false, {{{0, kDomain}}, {{10000, kDomain}}, {{10000, kDomain - 10000}}});

    for (const double bulge : {10.0, 100.0}) {
        const Polyline input{{{{-10, 0}}, {{0, 0}, bulge}, {{1e-7, 0}}}};
        const std::array<Entity, 2> tail{input, Line{{1.05e-6, 0}, {1.05e-6, 10}}};
        expectBoth(tail, false,
                   {{{-10, 0}}, {{0, 0}, bulge == 10.0 ? 0.0 : bulge}, {{1.05e-6, 0}},
                    {{1.05e-6, 10}}});
    }
}

TEST(Join, ClassifiesTheIncomingPiecesAtTheHeadAndAtClosure) {
    const Polyline incoming{{{{-10, 0}}, {{0, 0}, 10}, {{1e-7, 0}}}};
    const Polyline reversed{{{{1e-7, 0}, -10}, {{0, 0}}, {{-10, 0}}}};
    const Polyline source{{{{1.05e-6, 0}}, {{1.05e-6, 10}}}};
    const Line line{{1.05e-6, 0}, {1.05e-6, 10}};
    const std::vector<PolylineVertex> expected{{{-10, 0}}, {{0, 0}}, {{1.05e-6, 0}},
                                               {{1.05e-6, 10}}};
    for (const auto& candidate : {incoming, reversed}) {
        expectBoth(std::array<Entity, 2>{source, candidate}, false, expected);
        const std::array<Entity, 2> mixed{line, candidate};
        expectPolyline(join(mixed, kMillimetres), false, false, expected);
        EXPECT_FALSE(joinToSource(mixed, kMillimetres));
    }

    const std::array<Entity, 3> closed{
        Polyline{{{{1.05e-6, 0}}, {{10, 0}}}}, Line{{10, 0}, {0, 10}},
        Polyline{{{{0, 10}}, {{0, 0}, 10}, {{1e-7, 0}}}}};
    expectBoth(closed, true, {{{1.05e-6, 0}}, {{10, 0}}, {{0, 10}}, {{0, 0}}});

    const std::array<Entity, 2> metres{
        Polyline{{{{-0.01, 0}}, {{0, 0}, 10}, {{1e-10, 0}}}},
        Line{{1.05e-9, 0}, {1.05e-9, 0.01}}};
    const ToleranceRule metreRule{6};
    const std::vector<PolylineVertex> scaled{{{-0.01, 0}}, {{0, 0}}, {{1.05e-9, 0}},
                                             {{1.05e-9, 0.01}}};
    expectPolyline(join(metres, metreRule), true, false, scaled);
    expectPolyline(joinToSource(metres, metreRule), true, false, scaled);
    EXPECT_FALSE(join(std::array<Entity, 2>{source, incoming}, metreRule));
}

TEST(Join, ClassifiesAnArcAsItsIncomingSegment) {
    const Line line{{-1.85e-6, 0}, {-1.85e-6, -10}};
    const Arc arc{{0, 0}, 9.5e-7, 0, kPi};
    const std::vector<PolylineVertex> expected{{{9.5e-7, 0}}, {{-1.85e-6, 0}},
                                               {{-1.85e-6, -10}}};
    expectPolyline(join(std::array<Entity, 2>{arc, line}, kMillimetres), false, false, expected);
    expectBoth(std::array<Entity, 2>{Polyline{{{line.start}, {line.end}}}, arc}, false, expected);

    const std::array<Entity, 2> boundary{Arc{{0, 0}, 1e-6, 0, kPi},
                                         Line{{-1.9e-6, 0}, {-1.9e-6, -10}}};
    const auto result = join(boundary, kMillimetres);
    if (!result || !result->created) {
        ADD_FAILURE() << "missing joined polyline in the straightness band";
        return;
    }
    const auto* polyline = std::get_if<Polyline>(&*result->created);
    ASSERT_NE(polyline, nullptr);
    const double bulge = polyline->vertices().front().bulge;
    EXPECT_TRUE(bulge == 0.0 || bulge == std::tan(kPi / 4));
    const auto repeated = join(boundary, kMillimetres);
    if (repeated) {
        EXPECT_EQ(result->created, repeated->created);
    } else {
        ADD_FAILURE() << "repeating the join changed its outcome";
    }
}

TEST(Join, DropsCollapsedPiecesWithTheirWidthsAtEitherEnd) {
    const Polyline first{{{{-10, 0}, 0, 1, 1}, {{0, 0}, 0, 3, 3}, {{5e-7, 0}}}};
    const Polyline second{{{{0, 0}}, {{0, 10}}}};
    const std::vector<PolylineVertex> expected{{{-10, 0}, 0, 1, 1}, {{0, 0}}, {{0, 10}}};
    expectBoth(std::array<Entity, 2>{first, second}, false, expected);
    expectBoth(std::array<Entity, 2>{second, first}, false, expected);
}

TEST(Join, RejectsAnAttachmentThatMovesTheBulgeOutsideTheDomain) {
    constexpr double y = kDomain - 3000;
    const Polyline source{{{{-10000, y}}, {{0, y}, -8192}, {{0.5, y}}}};
    const std::array<Entity, 2> entities{source, Line{{1, y}, {1, y - 10000}}};
    EXPECT_FALSE(join(entities, kMillimetres));
    EXPECT_FALSE(joinToSource(entities, kMillimetres));
}

TEST(Join, RefusesWholeCircleArcsAndEndpointsOutsideTheDomain) {
    const double end = std::nextafter(1.0, 0.0);
    const Arc whole{{0, 0}, 1, 1, end};
    const Polyline polyline{{{{10, 10}}, {{0.5403023058681398, 0.8414709848078965}}}};
    const std::array<Entity, 2> first{polyline, whole};
    const std::array<Entity, 2> second{whole, polyline};
    for (const auto& entities : {first, second}) {
        EXPECT_FALSE(join(entities, kMillimetres));
        EXPECT_FALSE(joinToSource(entities, kMillimetres));
    }

    for (const double sign : {-1.0, 1.0}) {
        const double k = sign * kDomain;
        const Arc arc{{k, 0}, kDomain, sign > 0 ? 0 : kPi, sign > 0 ? kPi / 2 : 3 * kPi / 2};
        const Line line{{k, k - sign}, {k, k}};
        EXPECT_FALSE(join(std::array<Entity, 2>{arc, line}, kMillimetres));
        EXPECT_FALSE(join(std::array<Entity, 2>{line, arc}, kMillimetres));
    }
}

TEST(Join, CoversArcsFromTheSourceAndDistinguishesCloseFromConversion) {
    const Arc source{{0, 0}, 10, 0, kPi / 2};
    const Arc spaced{{0, 0}, 10, degreesToRadians(200), degreesToRadians(300)};
    const Arc wrapping{{0, 0}, 10, degreesToRadians(300), degreesToRadians(30)};
    const std::array<Entity, 2> gap{source, spaced};
    const std::array<Entity, 2> whole{source, wrapping};
    for (const auto& result : {join(gap, kMillimetres), joinToSource(gap, kMillimetres)}) {
        if (!result || !result->source) {
            ADD_FAILURE() << "missing source arc";
            continue;
        }
        const auto* arc = std::get_if<Arc>(&*result->source);
        ASSERT_NE(arc, nullptr);
        EXPECT_NEAR(arc->endAngle, spaced.endAngle, 1e-12);
    }
    for (const auto& result : {join(whole, kMillimetres), joinToSource(whole, kMillimetres),
                               joinToSource(whole, kMillimetres, true)}) {
        if (!result) {
            ADD_FAILURE() << "missing joined circle";
            continue;
        }
        EXPECT_EQ(result->created, (Entity{Circle{{0, 0}, 10}}));
    }
    const auto converted = join(whole, kMillimetres);
    const auto closed = joinToSource(whole, kMillimetres, true);
    EXPECT_TRUE(converted && converted->formsCircle);
    EXPECT_TRUE(closed && !closed->formsCircle);
}

TEST(Join, RoundsAnEqualResultAngleToACircleOnlyWhenSomethingJoins) {
    const Arc source{{0, 0}, 10, 6, std::nextafter(6.0, 0.0)};
    const std::array<Entity, 2> entities{source, Arc{{0, 0}, 10, 1, 2}};
    for (const auto& result : {join(entities, kMillimetres), joinToSource(entities, kMillimetres)}) {
        if (!result) {
            ADD_FAILURE() << "the near-full arc did not join";
            continue;
        }
        EXPECT_TRUE(result->formsCircle);
        EXPECT_EQ(result->consumed, (std::vector<std::size_t>{1}));
    }
    const auto alone = std::span{entities}.first(1);
    EXPECT_FALSE(join(alone, kMillimetres));
    EXPECT_FALSE(joinToSource(alone, kMillimetres));
    const auto closed = joinToSource(alone, kMillimetres, true);
    EXPECT_TRUE(closed && !closed->formsCircle);
}

}  // namespace
}  // namespace tg::core
