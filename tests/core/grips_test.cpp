// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/grips.hpp"

#include "tg/core/math.hpp"
#include "tg/core/text_metrics.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <variant>

namespace tg::core {
namespace {

constexpr double kTolerance = 1e-9;

void expectPoint(const Vec2& actual, const Vec2& expected) {
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}

TEST(Grips, PointOrderForEveryEntity) {
    const auto line = gripPoints(Line{{1.0, 2.0}, {5.0, 6.0}});
    ASSERT_EQ(line.size(), 3U);
    expectPoint(line[0], {1.0, 2.0});
    expectPoint(line[1], {5.0, 6.0});
    expectPoint(line[2], {3.0, 4.0});
    const auto arc = gripPoints(Arc{{}, 10.0, 0.0, kPi});
    ASSERT_EQ(arc.size(), 4U);
    expectPoint(arc[0], {10.0, 0.0});
    expectPoint(arc[1], {-10.0, 0.0});
    expectPoint(arc[2], {0.0, 10.0});
    expectPoint(arc[3], {});
    const auto circle = gripPoints(Circle{{2.0, 3.0}, 4.0});
    ASSERT_EQ(circle.size(), 5U);
    expectPoint(circle[0], {2.0, 3.0});
    expectPoint(circle[1], {6.0, 3.0});
    expectPoint(circle[2], {2.0, 7.0});
    expectPoint(circle[3], {-2.0, 3.0});
    expectPoint(circle[4], {2.0, -1.0});
    Polyline poly{{{{0.0, 0.0}, 1.0}, {{20.0, 0.0}}, {{20.0, 20.0}}}};
    const auto open = gripPoints(poly);
    ASSERT_EQ(open.size(), 5U);
    expectPoint(open[1], {10.0, -10.0});
    expectPoint(open[3], {20.0, 10.0});
    poly.setClosed(true);
    const auto closed = gripPoints(poly);
    ASSERT_EQ(closed.size(), 6U);
    expectPoint(closed.back(), {10.0, 10.0});
    EXPECT_TRUE(gripPoints(Polyline{}).empty());
    Text text;
    text.position = {1.0, 2.0};
    text.alignmentPoint = {4.0, 5.0};
    expectPoint(gripPoints(text)[0], text.position);
    text.horizontal = TextHorizontal::Center;
    expectPoint(gripPoints(text)[0], text.alignmentPoint);
    for (const auto horizontal : {TextHorizontal::Aligned, TextHorizontal::Fit}) {
        text.horizontal = horizontal;
        const auto points = gripPoints(text);
        ASSERT_EQ(points.size(), 2U);
        expectPoint(points[0], text.position);
        expectPoint(points[1], text.alignmentPoint);
    }
}

TEST(Grips, LineCircleAndArcMoves) {
    const Entity line = Line{{0.0, 0.0}, {20.0, 0.0}};
    expectPoint(std::get<Line>(moveGrip(line, 0, {2.0, 3.0}).entity).start, {2.0, 3.0});
    expectPoint(std::get<Line>(moveGrip(line, 1, {22.0, 3.0}).entity).end, {22.0, 3.0});
    const auto translated = std::get<Line>(moveGrip(line, 2, {15.0, 5.0}).entity);
    expectPoint(translated.start, {5.0, 5.0});
    expectPoint(translated.end, {25.0, 5.0});
    const Entity circle = Circle{{}, 10.0};
    expectPoint(std::get<Circle>(moveGrip(circle, 0, {5.0, 5.0}).entity).center, {5.0, 5.0});
    for (std::size_t i = 1; i < 5; ++i) {
        EXPECT_NEAR(std::get<Circle>(moveGrip(circle, i, {9.0, 12.0}).entity).radius, 15.0,
                    kTolerance);
    }
    const Entity arc = Arc{{}, 10.0, 0.0, kPi / 2.0};
    const auto points = gripPoints(arc);
    for (std::size_t i = 0; i < 3; ++i) {
        const Vec2 target =
            i == 0 ? Vec2{12.0, 0.0} : i == 1 ? Vec2{0.0, 12.0} : Vec2{9.0, 9.0};
        const auto moved = moveGrip(arc, i, target);
        ASSERT_EQ(moved.outcome, GripOutcome::Moved);
        const auto result = std::get<Arc>(moved.entity);
        for (std::size_t j = 0; j < 3; ++j) {
            EXPECT_NEAR(distance(result.center, j == i ? target : points[j]), result.radius,
                        kTolerance);
        }
    }
    expectPoint(std::get<Arc>(moveGrip(arc, 3, {5.0, 5.0}).entity).center, {5.0, 5.0});
}

TEST(Grips, PolylineRebuildsAdjacentArcsAndKeepsProperties) {
    Polyline poly{
        {{{0.0, 0.0}, 1.0, 1.0, 2.0}, {{20.0, 0.0}}, {{40.0, 0.0}, -1.0}, {{60.0, 0.0}}}};
    poly.setConstantWidth(0.5);
    poly.setContinuousLinetype(true);
    const auto moved = moveGrip(poly, 3, {30.0, 5.0});
    ASSERT_EQ(moved.outcome, GripOutcome::Moved);
    const auto result = std::get<Polyline>(moved.entity);
    EXPECT_NEAR(result.vertices()[0].bulge, 1.219803902718557, kTolerance);
    EXPECT_NEAR(result.vertices()[2].bulge, -0.7207592200561265, kTolerance);
    EXPECT_DOUBLE_EQ(result.constantWidth(), 0.5);
    EXPECT_TRUE(result.continuousLinetype());
    EXPECT_DOUBLE_EQ(result.vertices()[0].startWidth, 1.0);
    EXPECT_DOUBLE_EQ(result.vertices()[0].endWidth, 2.0);
    const auto vertex = moveGrip(poly, 2, {20.0, 5.0});
    EXPECT_NEAR(std::get<Polyline>(vertex.entity).vertices()[0].bulge, 1.219803902718557,
                kTolerance);
    const auto curved = moveGrip(poly, 1, {10.0, 15.0});
    EXPECT_NEAR(std::get<Polyline>(curved.entity).vertices()[0].bulge, -1.5, kTolerance);
    const Polyline closed{{{{0.0, 0.0}, 1.0}, {{20.0, 0.0}}, {{20.0, 20.0}, 1.0}}, true};
    const auto closedMove = moveGrip(closed, 0, {4.0, 4.0});
    ASSERT_EQ(closedMove.outcome, GripOutcome::Moved);
    const auto closedResult = std::get<Polyline>(closedMove.entity);
    EXPECT_TRUE(closedResult.isClosed());
    EXPECT_NEAR(closedResult.vertices()[0].bulge, 1.4770329614269007, kTolerance);
    EXPECT_NEAR(closedResult.vertices()[2].bulge, 1.2807764064044151, kTolerance);
}

TEST(Grips, TextInsertionAndSecondAlignmentPoint) {
    Text text;
    text.position = {600.0, 20.0};
    text.alignmentPoint = {620.0, 20.0};
    text.value = "Tg2";
    for (const auto horizontal :
         {TextHorizontal::Left, TextHorizontal::Aligned, TextHorizontal::Fit}) {
        text.horizontal = horizontal;
        const auto moved = std::get<Text>(moveGrip(text, 0, {605.0, 25.0}).entity);
        expectPoint(moved.position, {605.0, 25.0});
        expectPoint(moved.alignmentPoint, {625.0, 25.0});
        if (horizontal != TextHorizontal::Left) {
            const auto stretched = std::get<Text>(moveGrip(text, 1, {625.0, 25.0}).entity);
            EXPECT_NEAR(stretched.rotation, std::atan2(5.0, 25.0), kTolerance);
            const double length = std::hypot(25.0, 5.0);
            if (horizontal == TextHorizontal::Aligned) {
                EXPECT_NEAR(stretched.height, length / textWidth(U"Tg2"), kTolerance);
            } else {
                EXPECT_DOUBLE_EQ(stretched.height, text.height);
                EXPECT_NEAR(stretched.widthFactor, length / (textWidth(U"Tg2") * text.height),
                            kTolerance);
            }
        }
    }
}

TEST(Grips, InvalidMoveReturnsExactInput) {
    const auto check = [](const Entity& entity, std::size_t index, const Vec2& target) {
        const auto result = moveGrip(entity, index, target);
        EXPECT_EQ(result.outcome, GripOutcome::Invalid);
        EXPECT_EQ(result.entity, entity);
    };
    check(Line{{}, {10.0, 0.0}}, 0, {10.0, 0.0});
    check(Circle{{}, 10.0}, 1, {});
    check(Arc{{}, 10.0, 0.0, kPi}, 2, {});
    check(Polyline{{{{0.0, 0.0}, 1.0}, {{20.0, 0.0}}}}, 1, {});
    Text text;
    text.value = "Tg2";
    text.horizontal = TextHorizontal::Aligned;
    text.alignmentPoint = {20.0, 0.0};
    check(text, 1, {});
    check(Line{{}, {10.0, 0.0}}, 0, {std::numeric_limits<double>::infinity(), 0.0});
}

}  // namespace
}  // namespace tg::core
