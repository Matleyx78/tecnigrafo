// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/text.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <iomanip>
#include <numbers>
#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

using core::Arc;
using core::Circle;
using core::Entity;
using core::EntityId;
using core::Line;
using core::Polyline;
using core::PolylineVertex;
using core::Text;
using core::Vec2;

// 2^51: beyond the domain of solid-space, 2^50 (docs/adr/0035, point 23).
constexpr double kBeyondDomain = 2251799813685248.0;

/// `value` rounded to 1e-9, as text: the entities a command builds go
/// through atan2, cos and sin, so they are compared to that precision.
std::string number(double value) {
    constexpr double kScale = 1e9;
    double rounded = std::round(value * kScale) / kScale;
    if (rounded == 0.0) {
        rounded = 0.0;  // no "-0"
    }
    std::ostringstream out;
    out << std::setprecision(15) << rounded;
    return out.str();
}

std::string point(const Vec2& p) {
    return number(p.x) + "," + number(p.y);
}

/// The entity as text, angles in degrees.
std::string summary(const Entity& entity) {
    if (const auto* line = std::get_if<Line>(&entity)) {
        return "LINE " + point(line->start) + " " + point(line->end);
    }
    if (const auto* arc = std::get_if<Arc>(&entity)) {
        return "ARC " + point(arc->center) + " r=" + number(arc->radius) + " " +
               number(core::radiansToDegrees(arc->startAngle)) + "-" +
               number(core::radiansToDegrees(arc->endAngle));
    }
    if (const auto* circle = std::get_if<Circle>(&entity)) {
        return "CIRCLE " + point(circle->center) + " r=" + number(circle->radius);
    }
    if (const auto* polyline = std::get_if<Polyline>(&entity)) {
        std::string text = polyline->isClosed() ? "PLINE closed" : "PLINE";
        for (const PolylineVertex& vertex : polyline->vertices()) {
            text += " " + point(vertex.position);
            if (vertex.bulge != 0.0) {
                text += " b=" + number(vertex.bulge);
            }
            if (vertex.startWidth != 0.0 || vertex.endWidth != 0.0) {
                text += " w=" + number(vertex.startWidth) + "/" + number(vertex.endWidth);
            }
        }
        return text;
    }
    return "TEXT";
}

Line lineAt(double x1, double y1, double x2, double y2) {
    return Line{{x1, y1}, {x2, y2}};
}

Arc arcAt(double cx, double cy, double radius, double startDegrees, double endDegrees) {
    return Arc{
        {cx, cy}, radius, core::degreesToRadians(startDegrees), core::degreesToRadians(endDegrees)};
}

Polyline polylineOf(std::vector<PolylineVertex> vertices, bool closed = false) {
    return Polyline{std::move(vertices), closed};
}

class TrimExtendTest : public testing::Test {
protected:
    TrimExtendTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
        m_runner.setPixelSize(0.1);
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    /// Adds `entity` outside the undo history, as a drawing that was opened.
    EntityId seed(Entity entity) { return m_document.add(std::move(entity)); }

    void type(const std::string& text) { runner().submitText(text); }

    /// A pick at an Object prompt, at the exact point: no Shift.
    void pick(double x, double y) { runner().submit(PointInput{{x, y}}); }

    /// A click with Shift held.
    void shiftPick(double x, double y) { runner().click({x, y}, PickAction::Remove); }

    /// Starts `command` in quick mode (true) or in standard mode with every
    /// edge (false).
    void start(const std::string& command, bool quick) {
        if (!quick) {
            type("TRIMEXTENDMODE");
            type("0");
        }
        type(command);
        if (!quick) {
            type("");  // <select all>
        }
    }

    /// Every entity, in document order, as "<id> <summary>".
    [[nodiscard]] std::vector<std::string> drawing() const {
        std::vector<std::string> result;
        for (const core::EntityRecord& record : m_document.entities()) {
            result.push_back(std::to_string(record.id.value) + " " + summary(record.entity));
        }
        return result;
    }

    void clearMessages() { m_messages.clear(); }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

using Lines = std::vector<std::string>;

/// Both modes, for the cases that do not depend on the mode.
class TrimExtendModeTest : public TrimExtendTest, public testing::WithParamInterface<bool> {
protected:
    [[nodiscard]] static bool quick() { return GetParam(); }
};

INSTANTIATE_TEST_SUITE_P(Modes, TrimExtendModeTest, testing::Bool(),
                         [](const testing::TestParamInfo<bool>& param) {
                             return param.param ? std::string{"Quick"} : std::string{"Standard"};
                         });

// --- The cases of autocad-taglio.dxf ----------------------------------------

TEST_P(TrimExtendModeTest, TrimCasesOfAutocadTaglio) {
    // The figures a, b, c, d, e, g and h of docs/tappe/0.7/dxf-di-prova.md.
    seed(lineAt(0, 0, 20, 0));
    seed(lineAt(10, -10, 10, 10));
    seed(Circle{{100, 0}, 10});
    seed(lineAt(85, 0, 115, 0));
    seed(arcAt(200, 0, 10, 0, 180));
    seed(lineAt(195, -5, 195, 15));
    seed(lineAt(205, -5, 205, 15));
    seed(polylineOf({{{300, 0}}, {{320, 0}, 1.0}, {{340, 0}}, {{360, 0}}}));
    seed(lineAt(330, -20, 330, 20));
    seed(polylineOf({{{400, -10}}, {{420, -10}}, {{420, 10}}, {{400, 10}}}, true));
    seed(lineAt(410, -20, 410, 20));
    seed(Circle{{610, 0}, 10});
    seed(lineAt(600, 10, 620, 10));
    seed(polylineOf({{{700, 0}}, {{720, 20}}, {{720, 0}}, {{700, 20}}}));
    start("TRIM", quick());
    for (const Vec2& at : {Vec2{15, 0}, Vec2{100, 10}, Vec2{200, 10}, Vec2{350, 0}, Vec2{420, 0},
                           Vec2{602, 10}, Vec2{703, 3}}) {
        pick(at.x, at.y);
    }
    type("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(drawing(),
              (Lines{"1 LINE 0,0 10,0", "2 LINE 10,-10 10,10",
                     // the circle becomes an arc with the same handle
                     "3 ARC 100,0 r=10 180-0", "4 LINE 85,0 115,0",
                     // the arc cut in two keeps the handle on its first piece
                     "5 ARC 200,0 r=10 0-60", "6 LINE 195,-5 195,15", "7 LINE 205,-5 205,15",
                     // a polyline is an edge of itself at its vertices
                     "8 PLINE 300,0 320,0 b=1 340,0", "9 LINE 330,-20 330,20",
                     "10 PLINE 420,10 400,10 400,-10 420,-10", "11 LINE 410,-20 410,20",
                     "12 CIRCLE 610,0 r=10",
                     // a tangent edge cuts at the point of tangency
                     "13 LINE 610,10 620,10", "14 PLINE 710,10 720,20 720,0 700,20",
                     "15 ARC 200,0 r=10 120-180"}));
    EXPECT_EQ(editor().undoNames().back(), "Trim");
}

TEST_P(TrimExtendModeTest, ExtendCasesOfAutocadTaglio) {
    // The figures i, j, k and l.
    seed(lineAt(0, 200, 10, 200));
    seed(lineAt(20, 190, 20, 210));
    seed(arcAt(100, 200, 10, 0, 90));
    seed(lineAt(92, 190, 92, 220));
    seed(polylineOf({{{200, 200}}, {{210, 200}}, {{210, 205}}}));
    seed(lineAt(195, 215, 215, 215));
    seed(lineAt(300, 200, 305, 200));
    seed(Circle{{320, 200}, 5});
    start("EXTEND", quick());
    pick(8, 200);
    pick(100.8716, 209.9619);
    pick(210, 204);
    pick(304, 200);
    type("");
    EXPECT_EQ(drawing(), (Lines{"1 LINE 0,200 20,200", "2 LINE 20,190 20,210",
                                "3 ARC 100,200 r=10 0-143.130102354", "4 LINE 92,190 92,220",
                                "5 PLINE 200,200 210,200 210,215", "6 LINE 195,215 215,215",
                                "7 LINE 300,200 315,200", "8 CIRCLE 320,200 r=5"}));
    EXPECT_EQ(editor().undoNames().back(), "Extend");
}

// --- Prompts and messages of the two modes -----------------------------------

TEST_F(TrimExtendTest, QuickModeAsksForTheObjectAtOnce) {
    type("TRIM");
    EXPECT_EQ(messages().back(), "Current settings: Trim mode = Quick, Projection=UCS, Edge=None");
    EXPECT_EQ(runner().promptText(), "Select object to trim or shift-select to extend or [mOde]:");
    type("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(editor().undoNames().empty());  // nothing done, no step
}

TEST_F(TrimExtendTest, StandardModeAsksForTheEdgesFirst) {
    type("TRIMEXTENDMODE");
    type("0");
    clearMessages();
    type("EX");
    EXPECT_EQ(messages(), (Lines{"Current settings: Projection=UCS, Edge=None, Mode=Standard",
                                 "Select boundary edges ..."}));
    EXPECT_EQ(runner().promptText(), "Select objects or [mOde] <select all>:");
    type("");
    EXPECT_EQ(runner().promptText(), "Select object to extend or shift-select to trim or [mOde]:");
}

TEST_F(TrimExtendTest, ScriptsAlwaysUseStandardMode) {
    runner().setScriptSequences(true);
    type("TRIM");
    EXPECT_EQ(messages(), (Lines{"Current settings: Projection=UCS, Edge=None, Mode=Standard",
                                 "Select cutting edges ..."}));
    EXPECT_EQ(runner().promptText(), "Select objects or [mOde] <select all>:");
}

TEST_F(TrimExtendTest, ScriptsStayInStandardModeAfterTheModeOption) {
    // Review of #491: Quick changes TRIMEXTENDMODE, but the script goes on in
    // standard mode, so an object without cuts is not erased.
    runner().setScriptSequences(true);
    seed(lineAt(0, 0, 10, 0));
    type("TRIM");
    type("O");
    EXPECT_EQ(runner().promptText(), "Enter a trim mode option [Quick/Standard] <Standard>:");
    type("Q");
    EXPECT_EQ(runner().promptText(), "Select objects or [mOde] <select all>:");
    type("");
    clearMessages();
    pick(5, 0);
    EXPECT_EQ(messages(), (Lines{"Object does not intersect an edge."}));
    EXPECT_EQ(drawing(), (Lines{"1 LINE 0,0 10,0"}));
    type("");
    EXPECT_FALSE(runner().isActive());
    type("TRIMEXTENDMODE");
    EXPECT_EQ(runner().promptText(), "Enter new value for TRIMEXTENDMODE <1>:");
}

TEST_F(TrimExtendTest, StandardModeCutsOnlyAtTheChosenEdges) {
    const EntityId host = seed(lineAt(0, 0, 30, 0));
    seed(lineAt(10, -10, 10, 10));
    seed(lineAt(20, -10, 20, 10));
    type("TRIMEXTENDMODE");
    type("0");
    type("TRIM");
    clearMessages();
    type("20,5");  // the only edge, chosen with a typed point
    EXPECT_EQ(messages(), (Lines{"1 found, 1 total"}));
    type("");
    EXPECT_EQ(runner().promptText(), "Select object to trim or shift-select to extend or [mOde]:");
    pick(15, 0);  // x=10 is not an edge: the piece up to x=20 goes
    type("");
    EXPECT_EQ(drawing(), (Lines{"1 LINE 20,0 30,0", "2 LINE 10,-10 10,10", "3 LINE 20,-10 20,10"}));
    EXPECT_EQ(host.value, 1U);
}

// --- Trimming -----------------------------------------------------------------

TEST_F(TrimExtendTest, TrimInTheMiddleOfALineMakesACopyOfTheSecondPiece) {
    const EntityId host = seed(lineAt(0, 0, 30, 0));
    seed(lineAt(10, -10, 10, 10));
    seed(lineAt(20, -10, 20, 10));
    type("TRIM");
    pick(15, 0);
    EXPECT_EQ(runner().promptText(),
              "Select object to trim or shift-select to extend or [mOde/Undo]:");
    type("");
    EXPECT_EQ(drawing(), (Lines{"1 LINE 0,0 10,0", "2 LINE 10,-10 10,10", "3 LINE 20,-10 20,10",
                                "4 LINE 20,0 30,0"}));
    EXPECT_EQ(host.value, 1U);
}

TEST_F(TrimExtendTest, TrimInTheMiddleOfAPolylineSegmentInterpolatesTheWidths) {
    seed(polylineOf({{{0, 50}, 0.0, 1.0, 4.0}, {{30, 50}}}));
    seed(lineAt(10, 40, 10, 60));
    seed(lineAt(20, 40, 20, 60));
    type("TRIM");
    pick(15, 50);
    type("");
    EXPECT_EQ(drawing(), (Lines{"1 PLINE 0,50 w=1/2 10,50", "2 LINE 10,40 10,60",
                                "3 LINE 20,40 20,60", "4 PLINE 20,50 w=3/4 30,50"}));
}

TEST_F(TrimExtendTest, TrimInACurvedPolylineSegment) {
    // A half circle from (-10,0) to (10,0) through (0,-10), cut by x=-5 and x=5.
    seed(polylineOf({{{-10, 0}, 1.0}, {{10, 0}}}));
    seed(lineAt(-5, -20, -5, 5));
    seed(lineAt(5, -20, 5, 5));
    type("TRIM");
    pick(0, -10);
    type("");
    const Lines result = drawing();
    ASSERT_EQ(result.size(), 4U);
    // Each piece left is a sixth of the circle: a bulge of tan(60 / 4 degrees).
    const std::string bulge = number(std::tan(core::degreesToRadians(15.0)));
    const double y = -5.0 * std::numbers::sqrt3;
    EXPECT_EQ(result[0], "1 PLINE -10,0 b=" + bulge + " " + point({-5, y}));
    EXPECT_EQ(result[3], "4 PLINE " + point({5, y}) + " b=" + bulge + " 10,0");
}

TEST_F(TrimExtendTest, APickOnAVertexAfterASegmentThatIsAPointTakesThePieceThatFollows) {
    // Review of #491: the pick at (10,0) is the end of segment 0, the cut at
    // that vertex is the start of segment 2, after the point segment 1.
    seed(polylineOf({{{0, 0}}, {{10, 0}}, {{10, 0}}, {{10, 10}}}));
    type("TRIM");
    pick(10, 0);
    type("");
    EXPECT_EQ(drawing(), (Lines{"1 PLINE 0,0 10,0 10,0"}));
}

TEST_F(TrimExtendTest, TrimAClosedPolylineThroughItsVertexZero) {
    // dxf-di-prova.md, "Che cosa resta da controllare", point 1.
    seed(polylineOf({{{380, 0}}, {{400, 0}}, {{400, 10}}, {{380, 10}}}, true));
    seed(lineAt(390, -10, 390, 20));
    type("TRIM");
    pick(383, 0);
    type("");
    EXPECT_EQ(drawing(),
              (Lines{"1 PLINE 390,0 400,0 400,10 380,10 380,0", "2 LINE 390,-10 390,20"}));
}

TEST_P(TrimExtendModeTest, AnOverlappingEdgeDoesNotCut) {
    seed(lineAt(0, -40, 20, -40));
    seed(lineAt(5, -40, 30, -40));
    start("TRIM", quick());
    pick(2, -40);
    if (quick()) {
        EXPECT_EQ(drawing(), (Lines{"2 LINE 5,-40 30,-40"}));  // no cuts: erased
    } else {
        EXPECT_EQ(messages().back(), "Object does not intersect an edge.");
        EXPECT_EQ(drawing(), (Lines{"1 LINE 0,-40 20,-40", "2 LINE 5,-40 30,-40"}));
    }
}

TEST_F(TrimExtendTest, TwoEdgesThroughTheSamePointMakeOneCut) {
    seed(lineAt(0, 60, 20, 60));
    seed(lineAt(10, 50, 10, 70));
    seed(lineAt(0, 50, 20, 70));
    type("TRIM");
    pick(18, 60);
    type("");
    EXPECT_EQ(drawing(), (Lines{"1 LINE 0,60 10,60", "2 LINE 10,50 10,70", "3 LINE 0,50 20,70"}));
}

TEST_F(TrimExtendTest, ADegeneratePieceDoesNotStay) {
    // Millimetres: 1e-10 and 1e-8 are within the rule (docs/adr/0036).
    seed(lineAt(0, 100, 20, 100));
    seed(lineAt(1e-10, 90, 1e-10, 110));
    seed(lineAt(0, 150, 20, 150));
    seed(lineAt(10, 140, 10, 160));
    seed(lineAt(19.99999999, 140, 19.99999999, 160));
    type("TRIM");
    pick(15, 100);  // only a degenerate piece is left: the line goes
    pick(15, 150);  // the piece after the second edge is degenerate: no copy
    type("");
    EXPECT_EQ(drawing(), (Lines{"2 LINE 0,90 0,110", "3 LINE 0,150 10,150", "4 LINE 10,140 10,160",
                                "5 LINE 19.99999999,140 19.99999999,160"}));
}

TEST_F(TrimExtendTest, ADegeneratePolylinePieceDoesNotStay) {
    // The example of B4: every segment of the piece is degenerate.
    seed(polylineOf({{{0, 0}}, {{10, 0}}, {{10, 10}}}));
    seed(lineAt(5e-7, -5, 5e-7, 5));
    type("TRIM");
    pick(1, 0);
    type("");
    EXPECT_EQ(drawing(), (Lines{"1 PLINE 10,0 10,10", "2 LINE 5e-07,-5 5e-07,5"}));
}

TEST_P(TrimExtendModeTest, AnObjectWithoutCuts) {
    seed(lineAt(0, 0, 10, 0));
    start("TRIM", quick());
    clearMessages();
    pick(5, 0);
    if (quick()) {
        EXPECT_TRUE(drawing().empty());
        EXPECT_TRUE(messages().empty());
        EXPECT_EQ(runner().promptText(),
                  "Select object to trim or shift-select to extend or [mOde/Undo]:");
    } else {
        EXPECT_EQ(messages(), (Lines{"Object does not intersect an edge."}));
        EXPECT_EQ(drawing(), (Lines{"1 LINE 0,0 10,0"}));
        EXPECT_EQ(runner().promptText(),
                  "Select object to trim or shift-select to extend or [mOde]:");
    }
}

TEST_P(TrimExtendModeTest, AnObjectBeyondTheDomainWithoutCutsStays) {
    // solid-space refuses the host: no cuts, an incomplete answer (B18).
    seed(lineAt(kBeyondDomain, 0, kBeyondDomain + 1, 0));
    seed(lineAt(0, -10, 0, 10));
    start("TRIM", quick());
    clearMessages();
    pick(kBeyondDomain, 0);
    EXPECT_EQ(messages(), (Lines{"Cannot trim that object."}));
    EXPECT_EQ(drawing().size(), 2U);
    type("");
    EXPECT_TRUE(editor().undoNames().empty());
}

TEST_P(TrimExtendModeTest, ATextIsNeitherTrimmedNorAnEdge) {
    Text text;
    text.position = {10, 20};
    text.value = "Hello";
    seed(text);
    seed(lineAt(14, 15, 14, 30));
    start("TRIM", quick());
    clearMessages();
    pick(11, 21);  // inside the box of the text
    EXPECT_TRUE(messages().empty());
    EXPECT_EQ(runner().promptText(), "Select object to trim or shift-select to extend or [mOde]:");
    pick(14, 28);  // the text does not cut the line
    if (quick()) {
        EXPECT_EQ(drawing(), (Lines{"1 TEXT"}));
    } else {
        EXPECT_EQ(messages(), (Lines{"Object does not intersect an edge."}));
    }
}

// --- Extending ----------------------------------------------------------------

TEST_F(TrimExtendTest, ExtendALineAtBothEndsWithTheEndOnATie) {
    seed(lineAt(0, -10, 0, 10));
    seed(lineAt(30, -10, 30, 10));
    seed(lineAt(10, 0, 20, 0));
    type("EXTEND");
    pick(15, 0);  // the midpoint: the end
    EXPECT_EQ(drawing().back(), "3 LINE 10,0 30,0");
    pick(12, 0);  // nearer the start
    type("");
    EXPECT_EQ(drawing().back(), "3 LINE 0,0 30,0");
}

TEST_F(TrimExtendTest, ExtendAnArcBackwardsAtItsStart) {
    // The arc 0-90 degrees, to an edge on the Y axis: it becomes 270-90.
    seed(arcAt(0, 0, 10, 0, 90));
    seed(lineAt(0, -20, 0, 20));
    type("EXTEND");
    pick(9.5, 3);
    type("");
    EXPECT_EQ(drawing().front(), "1 ARC 0,0 r=10 270-90");
}

TEST_F(TrimExtendTest, ExtendAnArcBeyondAFullTurnAtBothEnds) {
    seed(arcAt(0, 50, 10, 0, 90));
    seed(lineAt(-15, 44, 15, 44));
    type("EXTEND");
    pick(8, 56);  // nearer the start
    pick(0, 60);  // the end
    type("");
    EXPECT_EQ(drawing().front(), "1 ARC 0,50 r=10 323.130102354-216.869897646");
}

TEST_F(TrimExtendTest, ExtendAnOpenPolylineAtItsStartAndEnd) {
    seed(polylineOf({{{110, 0}}, {{120, 0}}, {{120, 10}}}));
    seed(lineAt(100, -10, 100, 10));
    seed(lineAt(110, 20, 130, 20));
    type("EXTEND");
    pick(111, 0);
    pick(120, 9);
    type("");
    EXPECT_EQ(drawing().front(), "1 PLINE 100,0 120,0 120,20");
}

TEST_F(TrimExtendTest, ExtendCurvedPolylineSegments) {
    seed(polylineOf({{{90, 42}}, {{106, 42}, 0.5}, {{106, 58}}}));
    seed(lineAt(94, 40, 94, 65));
    seed(polylineOf({{{206, 42}, 0.5}, {{206, 58}}, {{220, 58}}}));
    seed(lineAt(194, 35, 194, 60));
    type("EXTEND");
    pick(108, 56);  // the last segment, curved
    pick(208, 44);  // the first segment, curved
    type("");
    EXPECT_EQ(drawing(), (Lines{"1 PLINE 90,42 106,42 b=1 94,58", "2 LINE 94,40 94,65",
                                "3 PLINE 194,42 b=1 206,58 220,58", "4 LINE 194,35 194,60"}));
}

TEST_F(TrimExtendTest, ExtendAPolylineWithASegmentThatIsAPointAtItsEnd) {
    // The example of B3: the vertices of the point segment follow the end.
    seed(polylineOf({{{0, 0}}, {{0, 0}}, {{10, 0}}}));
    seed(lineAt(-5, -5, -5, 5));
    type("EXTEND");
    pick(1, 0);
    type("");
    EXPECT_EQ(drawing().front(), "1 PLINE -5,0 -5,0 10,0");
}

TEST_F(TrimExtendTest, CirclesClosedPolylinesAndLinesWithoutAnEdgeDoNotChange) {
    seed(Circle{{300, 0}, 10});
    seed(lineAt(320, -15, 320, 15));
    seed(polylineOf({{{300, 40}}, {{310, 40}}, {{310, 50}}, {{300, 50}}}, true));
    seed(lineAt(300, 100, 310, 100));
    const Lines before = drawing();
    type("EXTEND");
    clearMessages();
    pick(310, 0);
    pick(310, 45);
    pick(309, 100);
    EXPECT_TRUE(messages().empty());
    EXPECT_EQ(runner().promptText(), "Select object to extend or shift-select to trim or [mOde]:");
    type("");
    EXPECT_EQ(drawing(), before);
    EXPECT_TRUE(editor().undoNames().empty());
}

TEST_P(TrimExtendModeTest, ExtendingBeyondTheDomainFails) {
    seed(lineAt(kBeyondDomain, 0, kBeyondDomain + 1, 0));
    seed(lineAt(0, -10, 0, 10));
    start("EXTEND", quick());
    clearMessages();
    pick(kBeyondDomain, 0);
    EXPECT_EQ(messages(), (Lines{"Cannot extend that object."}));
    EXPECT_EQ(drawing().front(),
              "1 LINE " + point({kBeyondDomain, 0}) + " " + point({kBeyondDomain + 1, 0}));
}

// --- Shift, mOde, Undo, Esc ---------------------------------------------------

TEST_F(TrimExtendTest, ShiftDoesTheOtherCommand) {
    seed(lineAt(0, 0, 30, 0));
    seed(lineAt(10, -10, 10, 30));
    seed(lineAt(20, -10, 20, 30));
    seed(lineAt(0, 20, 8, 20));
    type("TRIM");
    shiftPick(7, 20);  // TRIM with Shift extends
    EXPECT_EQ(drawing()[3], "4 LINE 0,20 10,20");
    type("");
    type("EXTEND");
    shiftPick(15, 0);  // EXTEND with Shift trims
    type("");
    EXPECT_EQ(drawing(), (Lines{"1 LINE 0,0 10,0", "2 LINE 10,-10 10,30", "3 LINE 20,-10 20,30",
                                "4 LINE 0,20 10,20", "5 LINE 20,0 30,0"}));
}

TEST_F(TrimExtendTest, ModeOptionSwitchesModesAndTrimextendmode) {
    seed(lineAt(0, 0, 30, 0));
    seed(lineAt(10, -10, 10, 10));
    type("TRIM");
    type("O");
    EXPECT_EQ(runner().promptText(), "Enter a trim mode option [Quick/Standard] <Quick>:");
    type("");  // keeps quick mode
    EXPECT_EQ(runner().promptText(), "Select object to trim or shift-select to extend or [mOde]:");
    clearMessages();
    type("mode");
    type("S");
    EXPECT_EQ(messages(), (Lines{"Select cutting edges ..."}));
    EXPECT_EQ(runner().promptText(), "Select objects or [mOde] <select all>:");
    type("O");
    EXPECT_EQ(runner().promptText(), "Enter a trim mode option [Quick/Standard] <Standard>:");
    type("Q");
    EXPECT_EQ(runner().promptText(), "Select object to trim or shift-select to extend or [mOde]:");
    type("O");
    type("S");
    type("");
    type("");
    EXPECT_FALSE(runner().isActive());
    // The option changed TRIMEXTENDMODE.
    type("TRIMEXTENDMODE");
    EXPECT_EQ(runner().promptText(), "Enter new value for TRIMEXTENDMODE <0>:");
}

TEST_F(TrimExtendTest, UndoGoesBackOneOperation) {
    seed(lineAt(0, 0, 30, 0));
    seed(lineAt(10, -10, 10, 10));
    seed(lineAt(20, -10, 20, 10));
    type("TRIM");
    pick(15, 0);
    pick(25, 0);
    EXPECT_EQ(drawing(), (Lines{"1 LINE 0,0 10,0", "2 LINE 10,-10 10,10", "3 LINE 20,-10 20,10"}));
    type("U");
    EXPECT_EQ(drawing().back(), "4 LINE 20,0 30,0");
    type("U");
    EXPECT_EQ(drawing().front(), "1 LINE 0,0 30,0");
    EXPECT_EQ(drawing().size(), 3U);
    // With nothing left to undo the option is no longer offered.
    EXPECT_EQ(runner().promptText(), "Select object to trim or shift-select to extend or [mOde]:");
    type("");
    EXPECT_TRUE(editor().undoNames().empty());
}

TEST_F(TrimExtendTest, EscKeepsTwoOperationsInOneUndoStep) {
    seed(lineAt(0, 0, 30, 0));
    seed(lineAt(10, -10, 10, 30));
    seed(lineAt(20, -10, 20, 30));
    seed(lineAt(0, 20, 8, 20));
    type("TRIM");
    pick(15, 0);
    shiftPick(7, 20);
    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().isActive());
    const Lines done = drawing();
    EXPECT_EQ(done, (Lines{"1 LINE 0,0 10,0", "2 LINE 10,-10 10,30", "3 LINE 20,-10 20,30",
                           "4 LINE 0,20 10,20", "5 LINE 20,0 30,0"}));
    EXPECT_EQ(editor().undoNames(), (Lines{"Trim"}));
    type("U");
    EXPECT_EQ(drawing(), (Lines{"1 LINE 0,0 30,0", "2 LINE 10,-10 10,30", "3 LINE 20,-10 20,30",
                                "4 LINE 0,20 8,20"}));
    type("REDO");
    EXPECT_EQ(drawing(), done);
}

// --- TRIMEXTENDMODE -------------------------------------------------------------

TEST_F(TrimExtendTest, TrimextendmodeAcceptsOnlyZeroAndOne) {
    type("TRIMEXTENDMODE");
    EXPECT_EQ(runner().promptText(), "Enter new value for TRIMEXTENDMODE <1>:");
    for (const char* wrong : {"2", "-1", "abc", "0.5"}) {
        clearMessages();
        type(wrong);
        EXPECT_EQ(messages(), (Lines{"Requires an integer between 0 and 1."})) << wrong;
        EXPECT_EQ(runner().promptText(), "Enter new value for TRIMEXTENDMODE <1>:");
    }
    type("0");
    EXPECT_FALSE(runner().isActive());
    type("TRIMEXTENDMODE");
    EXPECT_EQ(runner().promptText(), "Enter new value for TRIMEXTENDMODE <0>:");
    type("");  // Enter keeps the value
    type("TRIM");
    EXPECT_EQ(runner().promptText(), "Select objects or [mOde] <select all>:");
    EXPECT_TRUE(editor().undoNames().empty());  // a value of the session
}

}  // namespace
}  // namespace tg::cmd
