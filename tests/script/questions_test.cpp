// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/questions.hpp"

#include "tg/io/file_access.hpp"
#include "tg/io/result.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
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

TEST(Questions, PrefixSkipsUnimplementedQuestionsAndKeepsComparing) {
    const std::string text =
        "A = LINE start=0,0 end=10,0\n"
        "? intersections A A -> none\n"
        "? length A -> 12\n";
    const auto result = answerQuestionsUntilFrontier(text);
    ASSERT_TRUE(std::holds_alternative<QuestionPrefix>(result));
    const auto& prefix = std::get<QuestionPrefix>(result);
    ASSERT_EQ(prefix.answers.size(), 2U);
    if (prefix.answers[0].actual != "error: question not implemented yet") {
        GTEST_SKIP() << "intersections is now implemented";
    }
    EXPECT_TRUE(prefix.answers[0].skipped);
    EXPECT_FALSE(prefix.answers[1].skipped);
    EXPECT_FALSE(prefix.answers[1].matches);
    EXPECT_EQ(prefix.answers[1].actual, "10");
    EXPECT_FALSE(prefix.frontier);
    // Ordinary answers retain their original semantics.
    EXPECT_FALSE(answers(text)[0].skipped);
}

TEST(Questions, PrefixSkipsAnnouncedArgumentsWhenNotImplemented) {
    const std::string text = "? snap 5,0 aperture=1 from=1,1 -> none\n";
    const auto whole = answerQuestions(text);
    const auto* error = std::get_if<ParseError>(&whole);
    if (error == nullptr || !error->notImplemented) {
        GTEST_SKIP() << "from= is now implemented";
    }
    const auto result = answerQuestionsUntilFrontier(text + "? font-metrics -> wrong\n");
    ASSERT_TRUE(std::holds_alternative<QuestionPrefix>(result));
    const auto& prefix = std::get<QuestionPrefix>(result);
    ASSERT_EQ(prefix.answers.size(), 2U);
    EXPECT_TRUE(prefix.answers[0].skipped);
    EXPECT_FALSE(prefix.answers[1].skipped);
    EXPECT_FALSE(prefix.answers[1].matches);
    EXPECT_FALSE(prefix.frontier);
}

TEST(Questions, PrefixStopsAtChangesWithAndRejectsMalformedLinesAboveIt) {
    const auto result = answerQuestionsUntilFrontier(
        "; oracle: ezdxf\n? font-metrics -> wrong\n; cambia con #346\nmalformed\n");
    ASSERT_TRUE(std::holds_alternative<QuestionPrefix>(result));
    const auto& prefix = std::get<QuestionPrefix>(result);
    ASSERT_EQ(prefix.answers.size(), 1U);
    const Frontier frontier = prefix.frontier.value_or(Frontier{0, {}});
    EXPECT_EQ(frontier.line, 3);
    EXPECT_EQ(frontier.reason, "changes with #346");
    const auto malformed = answerQuestionsUntilFrontier("malformed\n; cambia con #346\n");
    ASSERT_TRUE(std::holds_alternative<ParseError>(malformed));
    EXPECT_EQ(std::get<ParseError>(malformed).line, 1);
    EXPECT_FALSE(std::get<ParseError>(malformed).notImplemented);
}

TEST(Questions, PrefixStopsAtUnimplementedDefinitionsWhenAvailable) {
    // A valid definition of a type of the 0.8 whose package is not merged
    // yet (docs/tappe/0.8/PT9, point 5). Keep the frontier contract covered
    // while one of them is still refused.
    for (const std::string_view entity : {
             "SOLID vertex=0,0 vertex=1,0 vertex=0,1 vertex=1,1",
             "ELLIPSE center=0,0 major-axis=10,0 ratio=0.5",
             R"(MTEXT insertion=0,0 value="A")",
             R"(ATTDEF position=0,0 value="N" tag="T")",
             "DIMENSION kind=radius center=0,0 point=1,0 text-point=1,0 measurement=1",
             R"(HATCH pattern="SOLID" solid=yes)",
         }) {
        const std::string definition = "B = " + std::string{entity} + "\n";
        const auto whole = answerQuestions(definition);
        const auto* error = std::get_if<ParseError>(&whole);
        if (error == nullptr || !error->notImplemented) {
            continue;
        }
        const auto result =
            answerQuestionsUntilFrontier("? font-metrics -> wrong\n" + definition + "malformed\n");
        ASSERT_TRUE(std::holds_alternative<QuestionPrefix>(result));
        const auto& prefix = std::get<QuestionPrefix>(result);
        EXPECT_EQ(prefix.frontier.value_or(Frontier{0, {}}).line, 2);
        EXPECT_EQ(prefix.answers.size(), 1U);
        return;
    }
    GTEST_SKIP() << "no unimplemented definition in the catalogue";
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
             // Not in the catalogue: a typing error, not a mode or argument to come.
             Case{defined + "? snaps A modes=Quadrant -> none", 4},
             Case{defined + "? snaps A modes=endpoint,corner -> none", 4},
             Case{defined + "? snap 5,0 aperture=1 aperure=1 -> none", 4},
             Case{defined + "? snap 5,0 aperture=1 from -> none", 4},
             // `from=` is only an argument of `snap`.
             Case{defined + "? snaps A from=1,1 -> none", 4},
         }) {
        const ParseError error = failure(test.text);
        EXPECT_EQ(error.line, test.line) << test.text;
        EXPECT_FALSE(error.message.empty()) << test.text;
        EXPECT_FALSE(error.notImplemented) << test.text;
    }
}

// The snap modes and the argument of `snap` that docs/verifica/domande.md
// describes from stage 0.7, before the package that realizes them (P7).
constexpr std::array<std::string_view, 4> kCatalogueModes{"quadrant", "intersection",
                                                          "perpendicular", "tangent"};
constexpr std::string_view kCatalogueArgument = "from";

// Valid forms stay recognized after their packages implement them. A missing
// catalogue row must never turn these references into malformed files.
TEST(Questions, StageSevenCatalogueQuestionsAreNeverUnknown) {
    constexpr std::array<std::string_view, 14> kForms{
        "transform A move 1,2",
        "grips A",
        "grip A 0,0 1,2",
        "intersections A B",
        "self-intersections A",
        "side A 1,2",
        "coincident 0,0 1,2",
        "degenerate A segment=0",
        "offset A 1 0,2",
        "fillet A 5,0 B 25,-5 radius=1",
        "tangents 40,0 C",
        "circle-ttr A 5,0 B 25,-5 radius=1",
        "chamfer A 5,0 B 25,-5 distances=1,1",
        "join A B",
    };
    const std::string defined = std::string{kDefinitions} + "C = CIRCLE center=30,0 radius=5\n";
    for (const std::string_view form : kForms) {
        std::string text = defined;
        text += "? ";
        text.append(form);
        text += " -> none\n";
        const auto result = answerQuestions(text);
        if (const auto* error = std::get_if<ParseError>(&result)) {
            EXPECT_TRUE(error->notImplemented) << form << ": " << error->message;
        } else {
            EXPECT_EQ(std::get<std::vector<Answer>>(result).size(), 1U) << form;
        }
        // Test the name, not the arguments: append the typo before its first space.
        const std::size_t space = form.find(' ');
        std::string misspelled{form.substr(0, space)};
        misspelled += "x";
        misspelled.append(form.substr(space));
        text = defined;
        text += "? ";
        text += misspelled;
        text += " -> none\n";
        const ParseError unknown = failure(text);
        EXPECT_FALSE(unknown.notImplemented) << misspelled;
        EXPECT_NE(unknown.message.find("unknown question"), std::string::npos) << misspelled;
    }
}

// What the catalogue describes is never an invalid file: either the program
// has it and the question is answered, or it does not have it yet and the file
// is not implemented yet, so the case is not passed (docs/verifica/casi.md).
// The test does not say which of the two holds today, so the package that
// realizes a mode or `from=` does not have to touch it. If the program does
// not know one of them and does not announce it either, this fails.
TEST(Questions, ModesAndArgumentsOfTheCatalogueAreNeverInvalid) {
    const std::string defined = std::string{kDefinitions};  // lines 1-3
    std::vector<std::string> questions{
        "? snap 5,0 aperture=1 from=1,1 -> none",
        "? snap 5,0 from=1,1 aperture=1 modes=quadrant -> none",
    };
    for (const std::string_view mode : kCatalogueModes) {
        questions.push_back("? snaps A modes=" + std::string{mode} + " -> none");
        questions.push_back("? snaps B modes=endpoint," + std::string{mode} + ",center -> none");
        questions.push_back("? snap 5,0 aperture=1 modes=" + std::string{mode} + " -> none");
    }
    for (const std::string& question : questions) {
        const std::variant<std::vector<Answer>, ParseError> result =
            answerQuestions(defined + question);
        if (const auto* error = std::get_if<ParseError>(&result)) {
            EXPECT_TRUE(error->notImplemented) << question << ": " << error->message;
            EXPECT_EQ(error->line, 4) << question;
            EXPECT_NE(error->message.find("not implemented yet"), std::string::npos)
                << error->message;
        }
    }
}

// A typing error of a mode or of `from=` (the wrong case, one letter more or
// less) is still a malformed file, even though they are announced. Derived
// from the same names, so it holds whichever the program has.
TEST(Questions, TypingErrorsOfCatalogueModesAndArgumentsStayInvalid) {
    const std::string defined = std::string{kDefinitions};  // lines 1-3
    const auto typos = [](std::string_view name) {
        std::string capital{name};
        capital[0] = static_cast<char>(capital[0] - 'a' + 'A');
        return std::array<std::string, 3>{capital, std::string{name} + "x",
                                          std::string{name.substr(0, name.size() - 1)}};
    };
    for (const std::string_view mode : kCatalogueModes) {
        for (const std::string& typo : typos(mode)) {
            std::string text = defined;
            text += "? snaps A modes=";
            text += typo;
            text += " -> none";
            const ParseError error = failure(text);
            EXPECT_EQ(error.line, 4) << typo;
            EXPECT_FALSE(error.notImplemented) << typo;
        }
    }
    for (const std::string& typo : typos(kCatalogueArgument)) {
        std::string text = defined;
        text += "? snap 5,0 aperture=1 ";
        text += typo;
        text += "=1,1 -> none";
        const ParseError error = failure(text);
        EXPECT_EQ(error.line, 4) << typo;
        EXPECT_FALSE(error.notImplemented) << typo;
    }
}

// ---------------------------------------------------------------------------
// Layers, linetypes and properties (stage 0.6, package P5)

constexpr std::string_view kTables =
    "T = LTYPE name=\"Tratteggio\" pattern=5,-2,0,-2 description=\"mio\"\n"
    "I = LTYPE name=\"ACAD_ISO02W100\"\n"
    "L = LAYER name=\"Muri esterni\" color=1 linetype=\"Tratteggio\" lineweight=0.5 off frozen "
    "locked noplot\n"
    "Z = LAYER name=\"0\" color=3 locked\n";

TEST(Questions, LayerAndLinetypeDefinitionsAndEntitiesThatUseThem) {
    const std::vector<Answer> list =
        answers(std::string{kTables} +
                "!current layer=\"Muri esterni\" color=2 linetype=\"ACAD_ISO02W100\"\n"
                "!current lineweight=0.35 ltscale=2\n"
                "A = LINE start=0,0 end=10,0 layer=\"muri ESTERNI\" color=255,0,0 "
                "linetype=\"Tratteggio\" ltscale=2 lineweight=0.35\n"
                "B = LWPOLYLINE vertex=0,0 vertex=1,0 linetype-generation=yes\n"
                "C = CIRCLE center=0,0 radius=1 layer=\"0\" linetype=\"ByBlock\" color=byblock\n"
                "? length A -> 10\n"
                "? length B -> 1\n");
    ASSERT_EQ(list.size(), 2U);
    EXPECT_TRUE(list[0].matches) << list[0].actual;
    EXPECT_TRUE(list[1].matches) << list[1].actual;
}

TEST(Questions, ErrorsOfLayersLinetypesAndCurrentValues) {
    struct Case {
        std::string text;
        int line;
    };

    const std::string tables{kTables};
    for (const Case& test : {
             // The name of a definition is used once, whatever it defines.
             Case{tables + "L = LINE start=0,0 end=1,0", 5},
             Case{tables + "A = LINE start=0,0 end=1,0\nA = LAYER name=\"X\"", 6},
             // The name of a record is used once, without case; layer 0 once more.
             Case{tables + "M = LAYER name=\"MURI esterni\"", 5},
             Case{tables + "M = LAYER name=\"0\"", 5},
             Case{tables + "M = LTYPE name=\"tratteggio\" pattern=1", 5},
             Case{"M = LTYPE name=\"Continuous\" pattern=1", 1},
             Case{"M = LTYPE name=\"ByLayer\" pattern=1", 1},
             // Names not defined above.
             Case{"A = LINE start=0,0 end=1,0 layer=\"Muri\"", 1},
             Case{"A = LINE start=0,0 end=1,0 linetype=\"Tratteggio\"", 1},
             Case{R"(L = LAYER name="Muri" linetype="Tratteggio")", 1},
             Case{"!current layer=\"Muri\"", 1},
             Case{"!current linetype=\"Tratteggio\"", 1},
             // Malformed definitions and directives.
             Case{"L = LAYER", 1},
             Case{"L = LAYER name=\"A\" color=bylayer", 1},
             Case{"T = LTYPE name=\"Mio\"", 1},
             Case{"A = LINE start=0,0 end=1,0 color=rosso", 1},
             Case{"!current", 1},
             Case{"!current vertex=0,0", 1},
             Case{"!current color=1 color=2", 1},
             Case{"! current color=1", 1},
         }) {
        const ParseError error = failure(test.text);
        EXPECT_EQ(error.line, test.line) << test.text;
        EXPECT_FALSE(error.message.empty()) << test.text;
        EXPECT_FALSE(error.notImplemented) << test.text;
    }
}

// Colour 7 is white in the table of ADR 0024, point 3; colour 2 is yellow.
TEST(Questions, AciAndEffectiveAnswerFromTheDefinitions) {
    const std::vector<Answer> list = answers(
        std::string{kDefinitions} +
        "L = LAYER name=\"Spento\" color=2 off\n"
        "E = LINE start=0,0 end=1,0 layer=\"Spento\"\n"
        "? aci 1 -> 255,0,0\n"
        "? effective A -> color=255,255,255 linetype=\"Continuous\" ltscale=1 lineweight=0.25 "
        "visible=yes locked=no\n"
        "? effective E -> color=255,255,0 linetype=\"Continuous\" ltscale=1 lineweight=0.25 "
        "visible=no locked=no\n");
    ASSERT_EQ(list.size(), 3U);
    for (const Answer& answer : list) {
        EXPECT_TRUE(answer.matches) << answer.line << "\n  obtained: " << answer.actual;
    }
}

TEST(Questions, QuickSelectAnswersFromTheDefinitions) {
    expectAnswer("quickselect type=LINE", "none");
    expectAnswer("quickselect type=LWPOLYLINE", "A B");
    expectAnswer("quickselect within=B color=bylayer", "B");
}

/// Files kept in memory, by name.
class MemoryFiles final : public io::FileAccess {
public:
    explicit MemoryFiles(std::map<std::string, std::string, std::less<>> files)
        : m_files(std::move(files)) {}

    [[nodiscard]] io::Result<std::string> read(std::string_view name) override {
        const auto found = m_files.find(name);
        if (found == m_files.end()) {
            return io::Error{io::Error::Kind::File, 0, "does not exist"};
        }
        return found->second;
    }

    [[nodiscard]] std::optional<io::Error> write(std::string_view name,
                                                 std::string_view bytes) override {
        m_files.insert_or_assign(std::string{name}, std::string{bytes});
        return std::nullopt;
    }

    [[nodiscard]] bool exists(std::string_view name) const override {
        return m_files.contains(name);
    }

private:
    std::map<std::string, std::string, std::less<>> m_files;
};

// A DXF R2000 written by hand, with a LINE (handle 2F) on layer 0 and no
// tables: io reads the tables from package 0.6-P10.
constexpr std::string_view kDxf =
    "  0\nSECTION\n  2\nHEADER\n"
    "  9\n$ACADVER\n  1\nAC1015\n"
    "  0\nENDSEC\n"
    "  0\nSECTION\n  2\nENTITIES\n"
    "  0\nLINE\n  5\n2F\n100\nAcDbEntity\n  8\n0\n100\nAcDbLine\n"
    " 10\n0.0\n 20\n0.0\n 30\n0.0\n 11\n10.0\n 21\n5.0\n 31\n0.0\n"
    "  0\nENDSEC\n  0\nEOF\n";

TEST(Questions, PrefixSkipsUnhandledFileEntitiesAndContinues) {
    const std::string dxf =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nHATCH\n5\n30\n100\nAcDbEntity\n8\n0\n"
        "0\nENDSEC\n0\nEOF\n";
    MemoryFiles files{{{"hatch.dxf", dxf}}};
    const auto result = answerQuestionsUntilFrontier(
        "D = DXF file=hatch.dxf\n? bbox D:30 -> empty\n? version D -> AC1015\n", &files);
    ASSERT_TRUE(std::holds_alternative<QuestionPrefix>(result));
    const auto& prefix = std::get<QuestionPrefix>(result);
    ASSERT_EQ(prefix.answers.size(), 2U);
    EXPECT_TRUE(prefix.answers[0].skipped);
    EXPECT_FALSE(prefix.answers[1].skipped);
    EXPECT_TRUE(prefix.answers[1].matches);
    EXPECT_FALSE(prefix.frontier);
}

TEST(Questions, LayersAndLinetypesOfADrawing) {
    MemoryFiles files{{{"plan.dxf", std::string{kDxf}}}};
    const std::string text =
        "D = DXF file=plan.dxf\n"
        "? layers D -> none\n"
        "? linetypes D -> none\n"
        "? entity D 2F -> LINE start=0,0 end=10,5\n";
    std::variant<std::vector<Answer>, ParseError> result = answerQuestions(text, &files);
    ASSERT_TRUE(std::holds_alternative<std::vector<Answer>>(result));
    const auto& list = std::get<std::vector<Answer>>(result);
    ASSERT_EQ(list.size(), 3U);
    for (const Answer& answer : list) {
        EXPECT_TRUE(answer.matches) << answer.line << ": " << answer.actual;
    }

    for (const std::string_view question : {"? layers -> none", "? layers D D -> none",
                                            "? layers X -> none", "? linetypes A -> none"}) {
        std::variant<std::vector<Answer>, ParseError> wrong = answerQuestions(
            "D = DXF file=plan.dxf\nA = LINE start=0,0 end=1,0\n" + std::string{question}, &files);
        ASSERT_TRUE(std::holds_alternative<ParseError>(wrong)) << question;
        EXPECT_EQ(std::get<ParseError>(wrong).line, 3) << question;
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

/// The one answer of `text`, which must match.
void expectMatch(const std::string& text) {
    const std::vector<Answer> list = answers(text);
    ASSERT_EQ(list.size(), 1U) << text;
    EXPECT_TRUE(list[0].matches) << text << "\n  obtained: " << list[0].actual;
}

TEST(Questions, ToPolylineAndReverseAnswerWithThePolyline) {
    const std::string defined =
        "L = LINE start=-2,3 end=4,7\n"
        "A = ARC center=0,0 radius=10 start-angle=0 end-angle=90\n"
        "P = LWPOLYLINE vertex=0,0 bulge=0.5 start-width=1 end-width=2 vertex=10,0 bulge=0.25\n";
    expectMatch(defined + "? to-polyline L -> LWPOLYLINE closed=no vertex=-2,3 vertex=4,7\n");
    expectMatch(defined +
                "? to-polyline A -> LWPOLYLINE closed=no vertex=10,0 "
                "bulge=0.41421356237309503 vertex=0,10\n");
    expectMatch(defined +
                "? reverse P -> LWPOLYLINE closed=no vertex=10,0 bulge=-0.5 start-width=2 "
                "end-width=1 vertex=0,0 bulge=0.25\n");
}

// The example of the catalogue (docs/verifica/domande.md, "Polilinee").
TEST(Questions, PeditJoinAnswersTheExampleOfTheCatalogue) {
    const std::string defined =
        "S = LWPOLYLINE vertex=-4,0 vertex=0,0\n"
        "L = LINE start=1,0 end=3,0\n";
    expectMatch(defined + "? pedit-join S L fuzz=2 type=extend -> none\n");
    expectMatch(defined +
                "? pedit-join S L fuzz=2 type=both -> S LWPOLYLINE closed=no vertex=-4,0 "
                "vertex=0,0 vertex=1,0 vertex=3,0 added=2 consumed=L\n");
    expectMatch(defined +
                "? pedit-join S L units=4 type=add fuzz=2 -> S LWPOLYLINE closed=no "
                "vertex=-4,0 vertex=0,0 vertex=1,0 vertex=3,0 added=2 consumed=L\n");
}

TEST(Questions, PeditJoinWithoutTypeExtends) {
    const std::string defined =
        "X = LWPOLYLINE vertex=0,0 vertex=1,0\n"
        "V = LINE start=2,1 end=2,3\n";
    expectMatch(defined +
                "? pedit-join X V fuzz=2 -> X LWPOLYLINE closed=no vertex=0,0 vertex=2,0 "
                "vertex=2,3 added=1 consumed=V\n");
}

TEST(Questions, PeditJoinNamesTheObjectsInTheOrderTheyAttach) {
    // B attaches at the end first, then A at the start.
    const std::string defined =
        "S = LWPOLYLINE vertex=0,0 vertex=4,0\n"
        "A = LINE start=-2,0 end=-1,0\n"
        "B = LINE start=5,0 end=6,0\n";
    expectMatch(defined +
                "? pedit-join S A B fuzz=2 type=add -> S LWPOLYLINE closed=no vertex=-2,0 "
                "vertex=-1,0 vertex=0,0 vertex=4,0 vertex=5,0 vertex=6,0 added=4 "
                "consumed=B,A\n");
}

TEST(Questions, PolylineQuestionsRejectWrongArguments) {
    const std::string defined =
        "S = LWPOLYLINE vertex=0,0 vertex=1,0\n"
        "L = LINE start=2,0 end=3,0\n"
        "C = CIRCLE center=0,0 radius=1\n";  // lines 1-3
    for (const std::string_view question : {
             "to-polyline S",
             "to-polyline C",
             "reverse L",
             "pedit-join L S fuzz=1",
             "pedit-join S L",
             "pedit-join S L fuzz=x",
             "pedit-join S L fuzz=-1",
             "pedit-join S L fuzz=1 type=join",
             "pedit-join S fuzz=1",
         }) {
        const std::string text = defined + "? " + std::string{question} + " -> none\n";
        const ParseError error = failure(text);
        EXPECT_EQ(error.line, 4) << question;
        EXPECT_FALSE(error.message.empty()) << question;
        EXPECT_FALSE(error.notImplemented) << question;
    }
}

TEST(Questions, TypesOfStageEightAreNotSupportedUntilTheirPackage) {
    // docs/tappe/0.8/PT9, point 5: the case is not passed, it is not invalid.
    for (const std::string_view entity : {
             "SOLID vertex=0,0 vertex=1,0 vertex=0,1 vertex=1,1",
             "ELLIPSE center=0,0 major-axis=10,0 ratio=0.5",
             R"(MTEXT insertion=0,0 value="A")",
             R"(ATTDEF position=0,0 value="N" tag="T")",
             "DIMENSION kind=radius center=0,0 point=1,0 text-point=1,0 measurement=1",
             R"(HATCH pattern="SOLID" solid=yes)",
         }) {
        const std::string text = "A = " + std::string{entity} + "\n";
        const auto whole = answerQuestions(text);
        const auto* error = std::get_if<ParseError>(&whole);
        if (error == nullptr) {
            continue;  // its package is merged
        }
        const std::string type{entity.substr(0, entity.find(' '))};
        EXPECT_EQ(error->line, 1) << entity;
        EXPECT_EQ(error->message, type + " is not supported yet.");
        EXPECT_TRUE(error->notImplemented) << entity;
    }
    // A line with an error is an error of the file, supported or not.
    const ParseError error = failure("A = SOLID vertex=0,0\n");
    EXPECT_FALSE(error.notImplemented);
    EXPECT_EQ(error.message, "a SOLID has four vertices");
}

TEST(Questions, BlockAndEndBlockDefineABlockThatAnInsertUses) {
    // docs/tappe/0.8/PT16-blocchi-core.md, point 8.
    const std::vector<Answer> list = answers(
        "!block \"TG_EL\" base=5,0\n"
        "E1 = LINE start=0,0 end=10,0\n"
        "!endblock\n"
        "I = INSERT block=\"TG_EL\" insertion=10,20 scale=1,1 rotation=0\n"
        "? generated I -> LINE start=5,20 end=15,20\n"
        "? window 4,19 16,21 -> I\n");
    ASSERT_EQ(list.size(), 2U);
    EXPECT_TRUE(list[0].matches) << list[0].actual;
    EXPECT_TRUE(list[1].matches) << list[1].actual;
}

TEST(Questions, AnEntityOfABlockIsNotInTheDrawing) {
    // The line of the block is not an entity of the drawing; its name is
    // taken, and the numbering goes on after it.
    const std::vector<Answer> list = answers(
        "!block \"B\" base=0,0\n"
        "E1 = LINE start=0,0 end=10,0\n"
        "!endblock\n"
        "? window -1,-1 11,1 -> none\n");
    ASSERT_EQ(list.size(), 1U);
    EXPECT_TRUE(list[0].matches) << list[0].actual;
    EXPECT_EQ(failure("!block \"B\" base=0,0\nE1 = LINE start=0,0 end=1,0\n!endblock\n"
                      "E1 = LINE start=0,0 end=1,0\n")
                  .message,
              "the name 'E1' is already defined");
}

TEST(Questions, MalformedBlocksAreErrorsOfTheFile) {
    const auto expectError = [](std::string_view text, int line, std::string_view message) {
        const ParseError error = failure(text);
        EXPECT_EQ(error.line, line) << text;
        EXPECT_EQ(error.message, message) << text;
        EXPECT_FALSE(error.notImplemented) << text;
    };
    expectError("!block \"A\" base=0,0\n!block \"B\" base=0,0\n", 2,
                "a !block inside the block 'A': close it first with !endblock");
    expectError("!endblock\n", 1, "!endblock without a !block");
    expectError("!block \"A\" base=0,0\nL = LINE start=0,0 end=1,0\n", 2,
                "the file ends inside the block 'A': close it with !endblock");
    expectError("!block \"A\" base=0,0\n!endblock\n!block \"a\" base=1,1\n", 3,
                "the block 'a' is already defined");
    expectError("!block A base=0,0\n", 1, "expected !block \"NAME\" base=x,y");
    expectError("!block \"A\"\n", 1, "expected !block \"NAME\" base=x,y");
    expectError("!block \"A<\" base=0,0\n", 1,
                "invalid block name 'A<': a table name (docs/adr/0023, point 1)");
    expectError("!block \"A\" base=0,0\nR = LAYER name=\"R\"\n", 2,
                "only entities are defined inside the block 'A'");
    expectError("!block \"A\" base=0,0\n!endblock now\n", 2, "!endblock takes no argument");
    expectError("I = INSERT block=\"Missing\" insertion=0,0\n", 1,
                "unknown block 'Missing': define it above with !block");
}

TEST(Questions, GeneratedOfAnEntityOfBeforeTheStageIsTheEntityItself) {
    const std::vector<Answer> list = answers(
        "L = LINE start=0,0 end=10,0\n"
        "? generated L -> LINE start=0,0 end=10,0\n"
        "C = CIRCLE center=1,2 radius=3 color=1\n"
        "? generated C -> CIRCLE center=1,2 radius=3 color=255,0,0\n"
        "P = LWPOLYLINE closed=yes vertex=0,0 bulge=1 vertex=10,0\n"
        "? generated P -> LWPOLYLINE closed=yes vertex=0,0 bulge=1 vertex=10,0\n"
        "Z = LAYER name=\"Spento\" off\n"
        "H = LINE start=0,0 end=1,0 layer=\"Spento\"\n"
        "? generated H -> none\n"
        "K = LAYER name=\"Bloccato\" locked\n"
        "B = ARC center=0,0 radius=1 start-angle=0 end-angle=90 layer=\"Bloccato\"\n"
        "? generated B -> ARC center=0,0 radius=1 start-angle=0 end-angle=90 locked=yes\n");
    ASSERT_EQ(list.size(), 5U);
    for (const Answer& answer : list) {
        EXPECT_TRUE(answer.matches) << "line " << answer.line << ": " << answer.actual;
    }
}

TEST(Questions, GeneratedComparesTheShapesInOrder) {
    const std::vector<Answer> list = answers(
        "L = LINE start=0,0 end=10,0\n"
        "? generated L -> LINE start=10,0 end=0,0\n"
        "? generated L -> none\n");
    ASSERT_EQ(list.size(), 2U);
    EXPECT_FALSE(list[0].matches);
    EXPECT_FALSE(list[1].matches);
    EXPECT_EQ(list[0].actual, "LINE start=0,0 end=10,0");
}

TEST(Questions, GeneratedRejectsWrongArguments) {
    for (const std::string_view question : {"generated", "generated L L", "generated X"}) {
        const std::string text =
            "L = LINE start=0,0 end=10,0\n? " + std::string{question} + " -> none\n";
        const ParseError error = failure(text);
        EXPECT_EQ(error.line, 2) << question;
        EXPECT_FALSE(error.notImplemented) << question;
    }
}

// docs/verifica/domande.md, "Retini (tappa 0.8)": two cases for each
// question, in the form of the table.
TEST(Questions, HatchBoundaryAnswersTheOutcomeOfTheDetection) {
    const std::string square =
        "E0 = LINE start=0,0 end=10,0\n"
        "E1 = LINE start=10,0 end=10,10\n"
        "E2 = LINE start=10,10 end=0,10\n"
        "E3 = LINE start=0,10 end=0,0\n";
    const std::vector<Answer> list =
        answers(square + "E4 = CIRCLE center=5,5 radius=2\n" +
                "? hatch-boundary 1,1 -> enclosed loop=external+derived+polyline vertex=0,0 "
                "vertex=10,0 vertex=10,10 vertex=0,10 loop=outermost+derived+edges circle=5,5;2\n"
                "? hatch-boundary 5,0 -> on-curve E0\n"
                "? hatch-boundary -3,5 units=6 -> not-enclosed\n"
                "P = LINE start=2,2 end=2,2\n"
                "? hatch-boundary 0,5 -> on-curve E3 incomplete\n");
    ASSERT_EQ(list.size(), 4U);
    for (const Answer& answer : list) {
        EXPECT_TRUE(answer.matches) << "line " << answer.line << ": " << answer.actual;
    }
    expectMatch(
        "E0 = LINE start=0,0 end=10,0\n"
        "E1 = LINE start=10,2e-06 end=10,10\n"
        "E2 = LINE start=10,10 end=0,10\n"
        "E3 = LINE start=0,10 end=0,0\n"
        "? hatch-boundary 5,5 -> not-enclosed open=10,0 open=10,2e-06\n");
    // A gap within the tolerance of millimetres is beyond that of kilometres.
    const std::string gap =
        "E0 = LINE start=0,0 end=10,0\n"
        "E1 = LINE start=10,5e-07 end=10,10\n"
        "E2 = LINE start=10,10 end=0,10\n"
        "E3 = LINE start=0,10 end=0,0\n";
    expectMatch(gap + "? hatch-boundary 10,2.5e-07 -> on-curve E0\n");
    expectMatch(gap + "? hatch-boundary 5,5 units=7 -> not-enclosed open=10,0 open=10,5e-07\n");
}

TEST(Questions, HatchContoursAnswersTheLoopsThenTheEntitiesLeftOut) {
    const std::string defined =
        "Outer = LWPOLYLINE closed=yes vertex=0,0 vertex=20,0 vertex=20,10 vertex=0,10\n"
        "Open = LWPOLYLINE vertex=1,1 vertex=2,1 vertex=1,2\n"
        "Inner = CIRCLE center=5,5 radius=3\n"
        "Line = LINE start=1,1 end=2,2\n"
        "Triangle = LWPOLYLINE closed=yes vertex=4,4 vertex=6,4 vertex=5,6\n";
    expectMatch(defined +
                "? hatch-contours Outer Open Inner Line Triangle -> loop=external+polyline "
                "vertex=0,0 vertex=20,0 vertex=20,10 vertex=0,10 loop=outermost+edges "
                "circle=5,5;3 loop=polyline vertex=4,4 vertex=6,4 vertex=5,6 left-out=Open "
                "left-out=Line\n");
    expectMatch(defined + "? hatch-contours Line Open -> left-out=Line left-out=Open\n");
    const std::vector<Answer> none = answers(defined + "? hatch-contours Line -> none\n");
    ASSERT_EQ(none.size(), 1U);
    EXPECT_FALSE(none[0].matches);
    EXPECT_EQ(none[0].actual, "left-out=Line");
}

TEST(Questions, HatchQuestionsRejectWrongArguments) {
    const std::string defined = "L = LINE start=0,0 end=10,0\n";
    for (const std::string_view question : {
             "hatch-boundary",
             "hatch-boundary x",
             "hatch-boundary 1,1 units=25",
             "hatch-boundary 1,1 units=1.5",
             "hatch-boundary 1,1 fuzz=2",
             "hatch-boundary 1,1 units=4 units=4",
             "hatch-contours",
             "hatch-contours X",
         }) {
        const std::string text = defined + "? " + std::string{question} + " -> none\n";
        const ParseError error = failure(text);
        EXPECT_EQ(error.line, 2) << question;
        EXPECT_FALSE(error.message.empty()) << question;
        EXPECT_FALSE(error.notImplemented) << question;
    }
}

// --- Dimensions defined in a case (docs/tappe/0.8/PT23-quote-lineari.md,
// point 9) ---

constexpr std::string_view kDimension =
    "Q = DIMENSION kind=linear origin1=0,0 origin2=40,0 line-point=40,10 text-point=0,0 "
    "measurement=0\n";

/// The answer to `question` after `definitions`.
std::string actualAnswer(std::string_view definitions, std::string_view question) {
    const std::vector<Answer> list =
        answers(std::string{definitions} + "? " + std::string{question} + " -> ?\n");
    return list.size() == 1 ? list[0].actual : std::string{};
}

/// Checks that `question` after `definitions` gives `expected`, within the
/// tolerances of the cases.
void expectDimensionAnswer(std::string_view definitions, std::string_view question,
                           std::string_view expected) {
    const std::vector<Answer> list =
        answers(std::string{definitions} + "? " + std::string{question} + " -> " +
                std::string{expected} + "\n");
    ASSERT_EQ(list.size(), 1U) << question;
    EXPECT_TRUE(list[0].matches) << question << "\n  expected: " << expected
                                 << "\n  obtained: " << list[0].actual;
}

TEST(QuestionsDimension, ADefinedDimensionIsRegenerated) {
    const std::string generated = actualAnswer(kDimension, "generated Q");
    EXPECT_TRUE(
        generated.starts_with("LINE start=0,0.625 end=0,11.25; "
                              "LINE start=40,0.625 end=40,11.25; "
                              "LINE start=2.5,10 end=37.5,10; SOLID "))
        << generated;
    EXPECT_NE(generated.find(R"(value="40")"), std::string::npos) << generated;
    expectDimensionAnswer(kDimension, "transform Q move 0,0",
                          R"(DIMENSION kind=linear style="ISO-25" origin1=0,0 origin2=40,0 )"
                          "line-point=40,10 rotation=0 text-point=20,11.875 measurement=40");
    const std::string aligned = actualAnswer(
        "A = DIMENSION kind=aligned origin1=0,0 origin2=24,18 line-point=18,26 text-point=0,0 "
        "measurement=0 overrides=DIMASZ:2\n",
        "generated A");
    EXPECT_NE(aligned.find(R"(value="30")"), std::string::npos) << aligned;
    EXPECT_NE(aligned.find("SOLID"), std::string::npos) << aligned;
}

TEST(QuestionsDimension, CoincidentAlignedIsRefusedAndZeroLinearIsValid) {
    const ParseError error = failure(
        "A = DIMENSION kind=aligned origin1=0,0 origin2=0,0 line-point=5,5 text-point=0,0 "
        "measurement=0\n");
    EXPECT_EQ(error.message, "Dimension cannot be regenerated by Tecnigrafo.");
    EXPECT_FALSE(error.notImplemented);
    const std::string zero = actualAnswer(
        "Z = DIMENSION kind=linear origin1=0,0 origin2=0,10 line-point=0,20 text-point=0,0 "
        "measurement=0\n",
        "generated Z");
    EXPECT_NE(zero.find(R"(value="0")"), std::string::npos) << zero;
}

TEST(QuestionsDimension, TransformUsesTheDocument) {
    expectDimensionAnswer(kDimension, "transform Q rotate 90 0,0",
                          R"(DIMENSION kind=linear style="ISO-25" origin1=0,0 origin2=0,40 )"
                          "line-point=-10,40 rotation=90 text-point=-11.875,20 measurement=40");
    expectDimensionAnswer(kDimension, "transform Q scale 2 0,0",
                          R"(DIMENSION kind=linear style="ISO-25" origin1=0,0 origin2=80,0 )"
                          "line-point=80,20 rotation=0 text-point=40,21.875 measurement=80");
    // Readable after the reflection: the text above the line, upright.
    expectDimensionAnswer(kDimension, "transform Q mirror 0,0 1,0 mirrtext=1",
                          R"(DIMENSION kind=linear style="ISO-25" origin1=0,0 origin2=40,0 )"
                          "line-point=40,-10 rotation=0 text-point=20,-8.125 measurement=40");
    // A translation moves points and graphics without regenerating.
    expectDimensionAnswer(kDimension, "transform Q move 5,7",
                          R"(DIMENSION kind=linear style="ISO-25" origin1=5,7 origin2=45,7 )"
                          "line-point=45,17 rotation=0 text-point=25,18.875 measurement=40");
}

TEST(QuestionsDimension, GripUsesTheDocument) {
    expectDimensionAnswer(kDimension, "grip Q 40,10 40,20",
                          R"(DIMENSION kind=linear style="ISO-25" origin1=0,0 origin2=40,0 )"
                          "line-point=40,20 rotation=0 text-point=20,21.875 measurement=40");
    // An Invalid outcome: the aligned dimension would have coincident points.
    expectDimensionAnswer(
        "A = DIMENSION kind=aligned origin1=0,0 origin2=24,18 line-point=24,28 text-point=0,0 "
        "measurement=0\n",
        "grip A 24,18 0,0", "invalid");
}

// Importing a file as a block (docs/verifica/domande.md, "Importazione di un
// file come blocco"; docs/tappe/0.8/PT19-file-come-blocco.md).

/// A DXF text from "code value" pairs separated by '|' or new lines; a pair
/// without a blank has an empty value.
std::string dxfOf(std::string_view pairs) {
    std::string text;
    std::size_t start = 0;
    while (start < pairs.size()) {
        const std::size_t end = std::min(pairs.find_first_of("|\n", start), pairs.size());
        const std::string_view pair = pairs.substr(start, end - start);
        const std::size_t blank = pair.find(' ');
        text += std::string{pair.substr(0, blank)} + "\n";
        text += (blank == std::string_view::npos ? std::string{}
                                                 : std::string{pair.substr(blank + 1)}) +
                "\n";
        start = end + 1;
    }
    return text;
}

// A file in millimetres with the four tables, each with a record the
// target has (Continuous, Shared, Standard, Standard) and one it lacks (Dots,
// New, Mono, Fine); the block Child, which the target has, and Leaf, which
// it lacks, whose LINE names the layer New (340), its record (360) and its
// BLOCK (reactor); model space inserts both. Handles by hand: the target
// gives 101 to Shared and 102 to the LINE of its Child, so the file is
// renumbered from 103 in its order: Dots 103, New 104, Mono 105, Fine 106,
// record Leaf 107, BLOCK 108, LINE 109, ENDBLK 10A, INSERT 10B and 10C.
constexpr std::string_view kImported =
    "0 SECTION|2 HEADER|9 $ACADVER|1 AC1032|9 $INSUNITS|70 4\n"
    "0 ENDSEC\n"
    "0 SECTION|2 TABLES\n"
    "0 TABLE|2 LTYPE|5 5\n"
    "0 LTYPE|5 14|2 Continuous|70 0|3 Other solid line|72 65|73 0|40 0.0\n"
    "0 LTYPE|5 15|2 Dots|70 0|3 . . .|72 65|73 0|40 0.0\n"
    "0 ENDTAB\n"
    "0 TABLE|2 LAYER|5 2\n"
    "0 LAYER|5 10|2 Shared|70 0|62 2|6 Continuous\n"
    "0 LAYER|5 11|2 New|70 0|62 3|6 Dots\n"
    "0 ENDTAB\n"
    "0 TABLE|2 STYLE|5 3\n"
    "0 STYLE|5 16|2 Standard|70 0|40 0.0|41 1.0|50 0.0|71 0|42 2.5|3 arial.ttf|4\n"
    "0 STYLE|5 17|2 Mono|70 0|40 0.0|41 0.8|50 0.0|71 0|42 2.5|3 cour.ttf|4\n"
    "0 ENDTAB\n"
    "0 TABLE|2 DIMSTYLE|5 A|100 AcDbSymbolTable|70 2|100 AcDbDimStyleTable\n"
    "0 DIMSTYLE|105 18|100 AcDbSymbolTableRecord|100 AcDbDimStyleTableRecord|2 Standard|70 0|340 "
    "16\n"
    "0 DIMSTYLE|105 19|100 AcDbSymbolTableRecord|100 AcDbDimStyleTableRecord|2 Fine|70 0|140 "
    "5.0|340 17\n"
    "0 ENDTAB\n"
    "0 TABLE|2 BLOCK_RECORD|70 2\n"
    "0 BLOCK_RECORD|5 20|2 Child|70 0\n"
    "0 BLOCK_RECORD|5 24|2 Leaf|70 0\n"
    "0 ENDTAB\n"
    "0 ENDSEC\n"
    "0 SECTION|2 BLOCKS\n"
    "0 BLOCK|5 21|330 20|2 Child|70 0|10 0|20 0|30 0|3 Child\n"
    "0 ENDBLK|5 23|330 20\n"
    "0 BLOCK|5 25|330 24|2 Leaf|70 0|10 0|20 0|30 0|3 Leaf\n"
    "0 LINE|5 26|330 24|340 11|360 24|102 {ACAD_REACTORS|330 25|102 }|100 AcDbEntity|8 New|100 "
    "AcDbLine|10 0|20 0|11 2|21 0\n"
    "0 ENDBLK|5 27|330 24\n"
    "0 ENDSEC\n"
    "0 SECTION|2 ENTITIES\n"
    "0 INSERT|5 30|100 AcDbEntity|8 Shared|100 AcDbBlockReference|2 Leaf|10 5|20 6\n"
    "0 INSERT|5 31|100 AcDbEntity|8 0|100 AcDbBlockReference|2 Child|10 1|20 1\n"
    "0 ENDSEC\n"
    "0 EOF\n";

constexpr std::string_view kTarget =
    "L = LAYER name=\"Shared\" color=1\n"
    "!block \"Child\" base=0,0\n"
    "A = LINE start=0,0 end=1,0\n"
    "!endblock\n"
    "I = BLOCK-IMPORT file=full.dxf target-units=6\n";

/// The answers to `questions` after kTarget, with full.dxf and self.dxf.
std::variant<std::vector<Answer>, ParseError> importAnswers(std::string_view questions) {
    MemoryFiles files{{{"full.dxf", dxfOf(kImported)},
                       {"self.dxf", dxfOf("0 SECTION|2 ENTITIES|0 INSERT|5 10|2 self|10 0|20 0|"
                                          "0 ENDSEC|0 EOF")}}};
    return answerQuestions(std::string{kTarget} + std::string{questions}, &files);
}

TEST(Questions, ImportAnswersEveryFieldOfTheImport) {
    const std::string questions =
        "? import-error I -> none\n"
        "? import-definition I -> name=\"full\" base=0,0 units=4 entities=[10B INSERT "
        "block=\"Leaf\" insertion=5,6 scale=1,1 rotation=0 layer=\"Shared\"; 10C INSERT "
        "block=\"Child\" insertion=1,1 scale=1,1 rotation=0]\n"
        "? import-nested I -> name=\"Leaf\" base=0,0 units=0 entities=[109 LINE start=0,0 "
        "end=2,0 layer=\"New\"]\n"
        "? import-layers I -> id=104 name=\"New\" color=3 linetype=\"Dots\" "
        "lineweight=default\n"
        "? import-linetypes I -> id=103 name=\"Dots\" pattern=continuous description=\". . .\"\n"
        "? import-text-styles I -> id=105 name=\"Mono\" font=\"cour.ttf\" height=0 "
        "width-factor=0.8 oblique=0 backwards=no upside-down=no\n"
        "? import-dim-styles I -> id=106 name=\"Fine\" DIMADEC:0 DIMASZ:0.18 DIMAZIN:0 "
        "DIMCEN:0.09 DIMCLRD:0 DIMCLRE:0 DIMCLRT:0 DIMDEC:4 DIMDSEP:. DIMEXE:0.18 "
        "DIMEXO:0.0625 DIMGAP:0.09 DIMLFAC:1 DIMLWD:-2 DIMLWE:-2 DIMPOST: DIMRND:0 DIMSCALE:1 "
        "DIMSD1:0 DIMSD2:0 DIMSE1:0 DIMSE2:0 DIMTAD:0 DIMTIH:1 DIMTOH:1 DIMTXSTY:Mono DIMTXT:5 "
        "DIMZIN:0\n"
        "? import-unit-factor I -> 0.001\n"
        "? import-next-id I -> 10D\n"
        // The owner, the pointers and the reactor follow the renumbering.
        "? import-groups I 109 -> 5=\"109\" 330=\"107\" 340=\"104\" 360=\"107\" "
        "102=\"{ACAD_REACTORS\" 330=\"108\" 102=\"}\"\n"
        "? import-groups I 107 -> 5=\"107\" 2=\"Leaf\" 70=\"0\"\n"
        "? import-groups I 108 -> 5=\"108\" 330=\"107\" 2=\"Leaf\" 70=\"0\" 10=\"0\" 20=\"0\" "
        "30=\"0\" 3=\"Leaf\"\n"
        "? import-groups I 10A -> 5=\"10A\" 330=\"107\"\n"
        // The handle 105 of the dimension style and its text style (340).
        "? import-groups I 106 -> 5=\"106\" 105=\"106\" 100=\"AcDbSymbolTableRecord\" "
        "100=\"AcDbDimStyleTableRecord\" 70=\"0\" 140=\"5.0\" 340=\"105\"\n";
    std::variant<std::vector<Answer>, ParseError> result = importAnswers(questions);
    ASSERT_TRUE(std::holds_alternative<std::vector<Answer>>(result))
        << std::get<ParseError>(result).message;
    const auto& list = std::get<std::vector<Answer>>(result);
    ASSERT_EQ(list.size(), 14U);
    for (const Answer& answer : list) {
        EXPECT_TRUE(answer.matches) << answer.line << "\n  obtained: " << answer.actual;
    }
}

TEST(Questions, ImportKeepsTheErrorOfAFileThatInsertsItself) {
    std::variant<std::vector<Answer>, ParseError> result = importAnswers(
        "S = BLOCK-IMPORT file=self.dxf\n"
        "? import-error S -> \"Block \\\"self\\\" references itself.\"\n"
        "? import-next-id S -> 101\n");
    ASSERT_TRUE(std::holds_alternative<std::vector<Answer>>(result))
        << std::get<ParseError>(result).message;
    const auto& list = std::get<std::vector<Answer>>(result);
    ASSERT_EQ(list.size(), 2U);
    EXPECT_TRUE(list[0].matches) << list[0].actual;
    // Every other question on a failed import answers the error.
    EXPECT_FALSE(list[1].matches);
    EXPECT_EQ(list[1].actual, "error: Block \"self\" references itself.");
}

TEST(Questions, ImportRejectsWrongArguments) {
    for (const std::string_view question : {
             "import-error",        "import-error I I",     "import-error X",
             "import-error A",      "import-definition",    "import-definition X",
             "import-nested",       "import-nested X",      "import-layers",
             "import-layers X",     "import-linetypes",     "import-linetypes X",
             "import-text-styles",  "import-text-styles X", "import-dim-styles",
             "import-dim-styles X", "import-unit-factor",   "import-unit-factor I I",
             "import-next-id",      "import-next-id X",     "import-groups I",
             "import-groups X 109", "import-groups I zz",   "import-groups I 101",
             "import-groups I 0",
         }) {
        const std::variant<std::vector<Answer>, ParseError> result =
            importAnswers("? " + std::string{question} + " -> none\n");
        ASSERT_TRUE(std::holds_alternative<ParseError>(result)) << question;
        const auto& error = std::get<ParseError>(result);
        EXPECT_EQ(error.line, 6) << question;
        EXPECT_FALSE(error.notImplemented) << question;
    }
    // The definition: arguments, files and names.
    for (const std::string_view definition : {
             "J = BLOCK-IMPORT",
             "J = BLOCK-IMPORT full.dxf",
             "J = BLOCK-IMPORT file=full.dxf target-units=25",
             "J = BLOCK-IMPORT file=full.dxf target-units=x",
             "J = BLOCK-IMPORT file=full.dxf target-units=4 target-units=4",
             "J = BLOCK-IMPORT file=full.dxf file=self.dxf",
             "J = BLOCK-IMPORT file=missing.dxf",
             "I = BLOCK-IMPORT file=full.dxf",
         }) {
        const std::variant<std::vector<Answer>, ParseError> result =
            importAnswers(std::string{definition} + "\n");
        ASSERT_TRUE(std::holds_alternative<ParseError>(result)) << definition;
        EXPECT_EQ(std::get<ParseError>(result).line, 6) << definition;
    }
}

}  // namespace
}  // namespace tg::script
