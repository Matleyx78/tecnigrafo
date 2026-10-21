// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/questions.hpp"

#include <gtest/gtest.h>

#include <string>
#include <variant>
#include <vector>

namespace tg::script {
namespace {

// Two polylines drawn by hand, with every answer computed by hand:
// A is an open "L" from (0,0) to (10,0) to (10,10);
// B is a half circle from (20,0) to (30,0), bulge 1 (counter-clockwise), so
// it passes below the axis: centre (25,0), radius 5, lowest point (25,-5).
constexpr std::string_view kDefinitions =
    "; polilinee scritte a mano\n"
    "A = LWPOLYLINE closed=no vertex=0,0 vertex=10,0 vertex=10,10\n"
    "B = LWPOLYLINE closed=no vertex=20,0 bulge=1 vertex=30,0\n";

std::vector<Answer> answers(std::string_view text) {
    std::variant<std::vector<Answer>, ParseError> result = answerQuestions(text);
    if (const auto* error = std::get_if<ParseError>(&result)) {
        ADD_FAILURE() << "line " << error->line << ": " << error->message;
        return {};
    }
    return std::get<std::vector<Answer>>(std::move(result));
}

ParseError failure(std::string_view text) {
    std::variant<std::vector<Answer>, ParseError> result = answerQuestions(text);
    if (!std::holds_alternative<ParseError>(result)) {
        ADD_FAILURE() << "expected an error for:\n" << text;
        return {};
    }
    return std::get<ParseError>(std::move(result));
}

/// Asks one question about kDefinitions and checks that the program gives
/// the answer computed by hand.
void expectAnswer(std::string_view question, std::string_view expected) {
    const std::string text =
        std::string{kDefinitions} + "? " + std::string{question} + " -> " + std::string{expected};
    const std::vector<Answer> list = answers(text);
    ASSERT_EQ(list.size(), 1U) << question;
    EXPECT_TRUE(list[0].matches) << question << "\n  expected: " << expected
                                 << "\n  obtained: " << list[0].actual;
}

TEST(Questions, BoundingBox) {
    expectAnswer("bbox A", "0,0 10,10");
    expectAnswer("bbox B", "20,-5 30,0");  // exact: the arc reaches y = -5
}

TEST(Questions, EmptyPolylineHasNoBoxNoPointAndInfiniteDistance) {
    const std::vector<Answer> list = answers(
        "E = LWPOLYLINE\n"
        "? bbox E -> empty\n"
        "? length E -> 0\n"
        "? distance E 1,1 -> inf\n"
        "? closest E 1,1 -> none\n"
        "? snaps E -> none\n");
    ASSERT_EQ(list.size(), 5U);
    for (const Answer& answer : list) {
        EXPECT_TRUE(answer.matches) << answer.expected << " / " << answer.actual;
    }
}

TEST(Questions, Length) {
    expectAnswer("length A", "20");
    expectAnswer("length B", "15.707963267948966");  // 5 pi
}

TEST(Questions, DistanceAndClosestPoint) {
    expectAnswer("distance A 5,5", "5");
    expectAnswer("closest A 5,5", "5,0");      // tie: the earlier segment wins
    expectAnswer("closest A 12,15", "10,10");  // beyond the end
    expectAnswer("distance B 25,-10", "5");
    expectAnswer("closest B 25,-10", "25,-5");
    // The centre is at the same distance from every point of the arc: the
    // start point is the answer.
    expectAnswer("distance B 25,0", "5");
    expectAnswer("closest B 25,0", "20,0");
    // Above the axis, outside the swept angle: the nearer end point, the
    // start on a tie. sqrt(5^2 + 10^2) = 11.180339887498949.
    expectAnswer("distance B 25,10", "11.180339887498949");
    expectAnswer("closest B 25,10", "20,0");
}

TEST(Questions, InsideAndCrosses) {
    expectAnswer("inside A -1,-1 11,11", "yes");
    expectAnswer("inside A 11,11 -1,-1", "yes");  // corners in any order
    expectAnswer("inside A 1,1 11,11", "no");
    expectAnswer("inside B 19,-5 31,1", "yes");
    expectAnswer("inside B 19,-4.9 31,1", "no");  // the arc goes below -4.9
    expectAnswer("crosses A 2,2 8,8", "no");      // inside the L, touching nothing
    expectAnswer("crosses A 9,5 11,6", "yes");
    expectAnswer("crosses A -1,-1 11,11", "yes");  // wholly inside
    expectAnswer("crosses B 24,-6 26,-4", "yes");
    expectAnswer("crosses B 24,-2 26,-1", "no");  // under the chord, above the arc
}

TEST(Questions, SnapPointsInAnyOrder) {
    expectAnswer("snaps A",
                 "Endpoint 0,0; Endpoint 10,0; Endpoint 10,10; Midpoint 5,0; Midpoint 10,5");
    expectAnswer("snaps A modes=midpoint", "Midpoint 10,5; Midpoint 5,0");
    expectAnswer("snaps A modes=none", "none");
    expectAnswer("snaps B", "Center 25,0; Midpoint 25,-5; Endpoint 20,0; Endpoint 30,0");
    expectAnswer("snaps B modes=endpoint,center", "Endpoint 20,0; Endpoint 30,0; Center 25,0");
}

TEST(Questions, Pick) {
    expectAnswer("pick 5,1 tolerance=2", "A");
    expectAnswer("pick 5,5 tolerance=1", "none");  // inside the L, far from it
    expectAnswer("pick 25,-5.5 tolerance=1", "B");
    expectAnswer("pick 25,0 tolerance=4", "none");  // the centre of the arc
}

TEST(Questions, WindowAndCrossing) {
    expectAnswer("window -1,-1 11,11", "A");
    expectAnswer("window -1,-6 31,11", "A B");
    expectAnswer("window -1,-6 31,11", "B A");  // names in any order
    expectAnswer("window 0,0 9,9", "none");
    expectAnswer("crossing 9,-1 21,1", "A B");
    expectAnswer("crossing 12,1 18,5", "none");
}

TEST(Questions, Snap) {
    expectAnswer("snap 10.5,0.5 aperture=2", "Endpoint 10,0");
    expectAnswer("snap 25,-4.5 aperture=1", "Midpoint 25,-5");
    // Passing over the arc, far from its end points and midpoint: the
    // centre. (27,-4.5) is 5 - sqrt(24.25) = 0.076 from the arc.
    expectAnswer("snap 27,-4.5 aperture=1", "Center 25,0");
    expectAnswer("snap 27,-4.5 aperture=1 modes=endpoint", "none");
    expectAnswer("snap 27,-4.5 modes=endpoint,midpoint aperture=1", "none");
    expectAnswer("snap 50,50 aperture=1", "none");
}

TEST(Questions, AQuestionSeesOnlyTheEntitiesAbove) {
    const std::vector<Answer> list = answers(
        "A = LWPOLYLINE vertex=0,0 vertex=10,0\n"
        "? window -1,-1 21,1 -> A\n"
        "B = LWPOLYLINE vertex=11,0 vertex=20,0\n"
        "? window -1,-1 21,1 -> A B\n");
    ASSERT_EQ(list.size(), 2U);
    EXPECT_TRUE(list[0].matches);
    EXPECT_TRUE(list[1].matches);
}

TEST(Questions, AWrongAnswerIsADifference) {
    const std::string text = std::string{kDefinitions} +
                             "? length A -> 20\n"
                             "? length A -> 21\n"
                             "? bbox B   ->   20,-5   30,0\n";
    const std::variant<std::vector<Difference>, ParseError> result = checkQuestions(text);
    ASSERT_TRUE(std::holds_alternative<std::vector<Difference>>(result));
    const auto& differences = std::get<std::vector<Difference>>(result);
    ASSERT_EQ(differences.size(), 1U);
    EXPECT_EQ(differences[0], (Difference{5, "21", "20"}));
}

TEST(Questions, ToleranceAppliesFromItsLineOn) {
    const std::string text = std::string{kDefinitions} +
                             "? length B -> 15.7079\n"
                             "!tolerance absolute 0.001\n"
                             "? length B -> 15.7079\n";
    const std::vector<Answer> list = answers(text);
    ASSERT_EQ(list.size(), 2U);
    EXPECT_FALSE(list[0].matches);
    EXPECT_TRUE(list[1].matches);
}

TEST(Questions, WrongTextAnswersReportMeasuredValues) {
    const std::string text =
        "? text-width \"Hello\" -> 2.5\n"
        "? font-metrics -> ascent=1.3 descent=-0.3\n";
    const std::variant<std::vector<Difference>, ParseError> result = checkQuestions(text);
    ASSERT_TRUE(std::holds_alternative<std::vector<Difference>>(result));
    const auto& differences = std::get<std::vector<Difference>>(result);
    ASSERT_EQ(differences.size(), 2U);
    EXPECT_EQ(differences[0].line, 1);
    EXPECT_EQ(differences[0].expected, "2.5");
    // Advances and hhea fields read independently from Liberation Sans 2.1.5.
    EXPECT_EQ(differences[0].actual, "3.3122782114975156");
    EXPECT_EQ(differences[1].line, 2);
    EXPECT_EQ(differences[1].expected, "ascent=1.3 descent=-0.3");
    EXPECT_EQ(differences[1].actual, "ascent=1.3158268275372604 descent=-0.3080198722498226");
}

TEST(Questions, CommentsBlankLinesAndIndentation) {
    const std::vector<Answer> list = answers(
        "\n"
        "   ; commento rientrato\n"
        "  A = LWPOLYLINE vertex=0,0 vertex=10,0  \n"
        "\t? length A -> 10\n"
        "?length A -> 10\n");
    ASSERT_EQ(list.size(), 2U);
    EXPECT_EQ(list[0].line, 4);
    EXPECT_TRUE(list[0].matches);
    EXPECT_TRUE(list[1].matches);
}

TEST(Questions, ErrorsGiveTheirLine) {
    struct Case {
        std::string text;
        int line;
    };

    const std::string defined = std::string{kDefinitions};  // lines 1-3
    for (const Case& test : {
             Case{"something", 1},
             Case{defined + "? length A", 4},
             Case{defined + "? length A ->", 4},
             Case{defined + "? -> 3", 4},
             Case{defined + "?", 4},
             Case{defined + "? volume A -> 3", 4},
             Case{defined + "? length C -> 3", 4},
             Case{defined + "? length -> 3", 4},
             Case{defined + "? length A B -> 3", 4},
             Case{defined + "? distance A 1;1 -> 3", 4},
             Case{defined + "? inside A 0,0 -> yes", 4},
             Case{defined + "? snaps A modes=corner -> none", 4},
             Case{defined + "? snaps A tolerance=1 -> none", 4},
             Case{defined + "? pick 0,0 -> none", 4},
             Case{defined + "? pick 0,0 tolerance=-1 -> none", 4},
             Case{defined + "? pick 0,0 tolerance=1 tolerance=2 -> none", 4},
             Case{defined + "? snap 0,0 -> none", 4},
             Case{defined + "? snap 0,0 modes=endpoint -> none", 4},
             Case{defined + "? window 0,0 -> none", 4},
             Case{defined + "A = LWPOLYLINE", 4},
             Case{"1A = LWPOLYLINE", 1},
             Case{"none = LWPOLYLINE", 1},
             Case{"A B = LWPOLYLINE", 1},
             Case{" = LWPOLYLINE", 1},
             Case{"A = POINT 0,0", 1},
             Case{"A = LWPOLYLINE vertex=1", 1},
             Case{"!tolerance absolute -1", 1},
             Case{"!level 2", 1},
             Case{"? text-width \"open -> 3", 1},
         }) {
        const ParseError error = failure(test.text);
        EXPECT_EQ(error.line, test.line) << test.text;
        EXPECT_FALSE(error.message.empty()) << test.text;
        EXPECT_FALSE(error.notImplemented) << test.text;
    }
}

TEST(Questions, ObtainedFileHasTheActualAnswers) {
    const std::string text =
        "A = LWPOLYLINE vertex=0,0 vertex=10,0\n"
        "? length A -> 11\n"
        "; fine\n"
        "? text-width \"a -> b\" -> 2\n";
    const std::vector<Answer> list = answers(text);
    EXPECT_EQ(withActualAnswers(text, list),
              "A = LWPOLYLINE vertex=0,0 vertex=10,0\n"
              "? length A -> 10\n"
              "; fine\n"
              "? text-width \"a -> b\" -> 3.757274662881476\n");
}

}  // namespace
}  // namespace tg::script
