// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

using core::Polyline;
using core::Vec2;

class PlineCommandTest : public testing::Test {
protected:
    PlineCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    void start() { runner().submitText("PL"); }

    void point(double x, double y) { runner().submit(PointInput{{x, y}}); }

    void triangle() {
        point(0.0, 0.0);
        point(10.0, 0.0);
        point(10.0, 10.0);
    }

    [[nodiscard]] const Polyline& polyline() const {
        return std::get<Polyline>(document().entities().front().entity);
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(PlineCommandTest, StartsByNameAndAliasIgnoringCase) {
    for (const char* name : {"PLINE", "pline", "PL", "pL"}) {
        runner().submitText(name);
        EXPECT_TRUE(runner().isActive());
        EXPECT_TRUE(editor().inTransaction());
        EXPECT_EQ(runner().promptText(), "Specify start point:");
        EXPECT_TRUE(runner().preview().empty());
        runner().submit(CancelInput{});
    }
}

TEST_F(PlineCommandTest, ConfirmsAnOpenPolylineWithStraightSegments) {
    start();
    triangle();
    EXPECT_TRUE(document().isEmpty());
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_FALSE(polyline().isClosed());
    ASSERT_EQ(polyline().vertexCount(), 3U);
    EXPECT_EQ(polyline().vertices()[0].position, (Vec2{0.0, 0.0}));
    EXPECT_EQ(polyline().vertices()[1].position, (Vec2{10.0, 0.0}));
    EXPECT_EQ(polyline().vertices()[2].position, (Vec2{10.0, 10.0}));
    for (const auto& vertex : polyline().vertices()) {
        EXPECT_DOUBLE_EQ(vertex.bulge, 0.0);
    }
    EXPECT_EQ(editor().undoName(), "Polyline");
    EXPECT_TRUE(runner().preview().empty());
}

TEST_F(PlineCommandTest, ConfirmInputCompletesTwoPoints) {
    start();
    point(1.0, 2.0);
    point(3.0, 4.0);
    runner().submit(ConfirmInput{});
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(polyline().vertexCount(), 2U);
    EXPECT_FALSE(polyline().isClosed());
}

TEST_F(PlineCommandTest, ConfirmationWithFewerThanTwoPointsAddsNothing) {
    for (const int count : {0, 1}) {
        start();
        if (count == 1) {
            point(1.0, 2.0);
        }
        runner().submitText("");
        EXPECT_FALSE(runner().isActive());
        EXPECT_FALSE(editor().inTransaction());
        EXPECT_TRUE(document().isEmpty());
        EXPECT_FALSE(editor().canUndo());
    }
}

TEST_F(PlineCommandTest, CloseRequiresAtLeastThreePoints) {
    start();
    point(0.0, 0.0);
    EXPECT_EQ(runner().promptText(), "Specify next point or [Undo]:");
    point(10.0, 0.0);
    runner().submitText("c");
    EXPECT_TRUE(runner().isActive());
    ASSERT_FALSE(messages().empty());
    EXPECT_EQ(messages().back(), "Invalid point or option.");
    EXPECT_TRUE(document().isEmpty());
    point(10.0, 10.0);
    EXPECT_EQ(runner().promptText(), "Specify next point or [Close/Undo]:");
    runner().submitText("c");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_TRUE(polyline().isClosed());
    EXPECT_EQ(polyline().vertexCount(), 3U);
    EXPECT_EQ(polyline().segmentCount(), 3U);
}

TEST_F(PlineCommandTest, CloseAcceptsFullKeywordIgnoringCase) {
    start();
    triangle();
    runner().submitText("cLoSe");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_TRUE(polyline().isClosed());
}

TEST_F(PlineCommandTest, UndoRemovesVertexAndCloseOption) {
    start();
    triangle();
    runner().submitText("uNdO");
    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(runner().promptText(), "Specify next point or [Undo]:");
    EXPECT_TRUE(document().isEmpty());
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(polyline().vertexCount(), 2U);
}

TEST_F(PlineCommandTest, UndoAllPointsReturnsToStartAndClearsPreview) {
    start();
    point(0.0, 0.0);
    point(10.0, 0.0);
    runner().pointerMoved({20.0, 0.0});
    runner().submitText("u");
    runner().submitText("u");
    EXPECT_EQ(runner().promptText(), "Specify start point:");
    EXPECT_TRUE(runner().preview().empty());
    runner().submitText("@1,0");
    EXPECT_EQ(messages().back(), "Invalid point or option.");
    point(5.0, 5.0);
    point(6.0, 5.0);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(polyline().vertexCount(), 2U);
    EXPECT_EQ(polyline().vertices().front().position, (Vec2{5.0, 5.0}));
}

TEST_F(PlineCommandTest, RelativePointAfterUndoUsesRemainingVertex) {
    start();
    triangle();
    runner().submitText("u");
    runner().submitText("@5,0");
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(polyline().vertices().back().position, (Vec2{15.0, 0.0}));
}

TEST_F(PlineCommandTest, UndoAllRestoresPreviousCommandReference) {
    start();
    point(0.0, 0.0);
    point(10.0, 0.0);
    runner().submitText("");
    start();
    point(100.0, 100.0);
    runner().submitText("u");
    runner().submitText("@5,0");
    point(20.0, 0.0);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 2U);
    const auto& second = std::get<Polyline>(document().entities().back().entity);
    EXPECT_EQ(second.vertices().front().position, (Vec2{15.0, 0.0}));
}

TEST_F(PlineCommandTest, CoincidentPointsAreIgnoredWithMessage) {
    start();
    point(0.0, 0.0);
    point(0.0, 0.0);
    point(0.5e-9, 0.0);
    ASSERT_EQ(messages().size(), 2U);
    EXPECT_EQ(messages().back(), "Point coincides with the previous point.");
    runner().submitText("@10,0");
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(polyline().vertexCount(), 2U);
    EXPECT_EQ(polyline().vertices().back().position, (Vec2{10.0, 0.0}));
}

TEST_F(PlineCommandTest, DistinctNearbyPointsAreRetained) {
    start();
    point(0.0, 0.0);
    point(2e-9, 0.0);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(polyline().vertexCount(), 2U);
    EXPECT_TRUE(messages().empty());
}

TEST_F(PlineCommandTest, NonConsecutiveRepeatedPointIsRetained) {
    start();
    point(0.0, 0.0);
    point(10.0, 0.0);
    point(0.0, 0.0);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(polyline().vertexCount(), 3U);
    EXPECT_FALSE(polyline().isClosed());
}

TEST_F(PlineCommandTest, PreviewFollowsPointerWithoutChangingDocument) {
    start();
    runner().pointerMoved({3.0, 4.0});
    EXPECT_TRUE(runner().preview().empty());
    point(0.0, 0.0);
    point(10.0, 0.0);
    runner().pointerMoved({20.0, 30.0});
    ASSERT_EQ(runner().preview().size(), 1U);
    const auto& preview = std::get<Polyline>(runner().preview().front());
    ASSERT_EQ(preview.vertexCount(), 3U);
    EXPECT_EQ(preview.vertices()[0].position, (Vec2{0.0, 0.0}));
    EXPECT_EQ(preview.vertices()[1].position, (Vec2{10.0, 0.0}));
    EXPECT_EQ(preview.vertices()[2].position, (Vec2{20.0, 30.0}));
    EXPECT_FALSE(preview.isClosed());
    EXPECT_TRUE(document().isEmpty());
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(polyline().vertexCount(), 2U);  // pointer is not a committed vertex
}

TEST_F(PlineCommandTest, PreviewWithoutPointerContainsOnlyAcceptedPoints) {
    start();
    point(0.0, 0.0);
    point(10.0, 0.0);
    ASSERT_EQ(runner().preview().size(), 1U);
    EXPECT_EQ(std::get<Polyline>(runner().preview().front()).vertexCount(), 2U);
    runner().pointerMoved({10.0, 0.0});
    EXPECT_EQ(std::get<Polyline>(runner().preview().front()).vertexCount(), 2U);
}

TEST_F(PlineCommandTest, PreviewUpdatesAfterUndo) {
    start();
    triangle();
    runner().pointerMoved({20.0, 20.0});
    runner().submitText("u");
    ASSERT_EQ(runner().preview().size(), 1U);
    const auto& preview = std::get<Polyline>(runner().preview().front());
    ASSERT_EQ(preview.vertexCount(), 3U);
    EXPECT_EQ(preview.vertices()[1].position, (Vec2{10.0, 0.0}));
    EXPECT_EQ(preview.vertices()[2].position, (Vec2{20.0, 20.0}));
}

TEST_F(PlineCommandTest, EscDiscardsPendingGeometry) {
    start();
    triangle();
    runner().submit(CancelInput{});
    EXPECT_TRUE(document().isEmpty());
    EXPECT_FALSE(editor().canUndo());
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(runner().preview().empty());
}

TEST_F(PlineCommandTest, UndoRedoRestoresWholePolylineAndIdentifier) {
    start();
    triangle();
    runner().submitText("c");
    ASSERT_EQ(document().entityCount(), 1U);
    const core::EntityRecord original = document().entities().front();
    runner().submitText("u");
    EXPECT_TRUE(document().isEmpty());
    runner().submitText("redo");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(document().entities().front(), original);
}

TEST_F(PlineCommandTest, EmptyOrCancelledCommandPreservesRedo) {
    start();
    triangle();
    runner().submitText("");
    runner().submitText("u");
    for (const bool cancel : {false, true}) {
        start();
        point(50.0, 50.0);
        if (cancel) {
            runner().submit(CancelInput{});
        } else {
            runner().submitText("");
        }
        EXPECT_TRUE(editor().canRedo());
    }
    runner().submitText("redo");
    EXPECT_EQ(document().entityCount(), 1U);
}

TEST_F(PlineCommandTest, EnterRepeatsWithFreshGeometry) {
    start();
    triangle();
    runner().submitText("");
    runner().submitText("");
    EXPECT_TRUE(runner().isActive());
    EXPECT_TRUE(runner().preview().empty());
    EXPECT_EQ(runner().promptText(), "Specify start point:");
    point(20.0, 20.0);
    point(30.0, 30.0);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(std::get<Polyline>(document().entities().back().entity).vertexCount(), 2U);
}

TEST_F(PlineCommandTest, TypedCoordinatesProduceTheManualSquare) {
    start();
    for (const char* text : {"0,0", "@50,0", "@50<90", "0,50", "c"}) {
        runner().submitText(text);
    }
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_TRUE(polyline().isClosed());
    ASSERT_EQ(polyline().vertexCount(), 4U);
    EXPECT_TRUE(core::nearlyEqual(polyline().vertices()[2].position, {50.0, 50.0}, 1e-9));
    EXPECT_NEAR(polyline().length(), 200.0, 1e-9);
}

TEST_F(PlineCommandTest, PolarAbsoluteAndDirectDistanceUseRunnerResolution) {
    start();
    runner().submitText("10<90");
    runner().pointerMoved({0.0, 20.0});
    runner().submitText("5");
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);
    ASSERT_EQ(polyline().vertexCount(), 2U);
    EXPECT_TRUE(core::nearlyEqual(polyline().vertices()[0].position, {0.0, 10.0}, 1e-9));
    EXPECT_TRUE(core::nearlyEqual(polyline().vertices()[1].position, {0.0, 15.0}, 1e-9));
}

TEST_F(PlineCommandTest, InvalidTextKeepsGeometryAndPrompt) {
    start();
    point(0.0, 0.0);
    runner().submitText("@10");
    EXPECT_EQ(runner().promptText(), "Specify next point or [Undo]:");
    EXPECT_EQ(messages().back(), "Invalid point or option.");
    point(10.0, 0.0);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(polyline().vertexCount(), 2U);
}

// Issue #64, with the steps of its report.
TEST_F(PlineCommandTest, OrthoLeavesTheFirstClickOfANewPlineWhereItIs) {
    DrawingAids aids;
    aids.objectSnap = false;
    aids.ortho = true;
    runner().setDrawingAids(aids);
    start();
    runner().click({0.0, 0.0}, PickAction::Add);
    runner().click({50.0, 3.0}, PickAction::Add);
    runner().submitText("");
    start();
    runner().click({80.0, 60.0}, PickAction::Add);  // off both axes of (50,0)
    runner().click({120.0, 65.0}, PickAction::Add);
    runner().submitText("");

    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(polyline().vertices()[1].position, (Vec2{50.0, 0.0}));
    const auto& second = std::get<Polyline>(document().entities().back().entity);
    ASSERT_EQ(second.vertexCount(), 2U);
    EXPECT_EQ(second.vertices()[0].position, (Vec2{80.0, 60.0}));
    EXPECT_EQ(second.vertices()[1].position, (Vec2{120.0, 60.0}));
}

TEST_F(PlineCommandTest, OrthoFollowsTheVertexLeftByUndo) {
    DrawingAids aids;
    aids.objectSnap = false;
    aids.ortho = true;
    runner().setDrawingAids(aids);
    start();
    runner().click({80.0, 60.0}, PickAction::Add);
    runner().click({120.0, 65.0}, PickAction::Add);
    runner().submitText("u");                        // back to (80,60)
    runner().click({90.0, 100.0}, PickAction::Add);  // nearer to the vertical
    runner().submitText("");

    ASSERT_EQ(document().entityCount(), 1U);
    ASSERT_EQ(polyline().vertexCount(), 2U);
    EXPECT_EQ(polyline().vertices()[0].position, (Vec2{80.0, 60.0}));
    EXPECT_EQ(polyline().vertices()[1].position, (Vec2{80.0, 100.0}));
}

TEST_F(PlineCommandTest, UndoingEveryVertexLeavesTheNextClickWhereItIs) {
    DrawingAids aids;
    aids.objectSnap = false;
    aids.ortho = true;
    runner().setDrawingAids(aids);
    start();
    runner().click({0.0, 0.0}, PickAction::Add);
    runner().click({50.0, 3.0}, PickAction::Add);
    runner().submitText("");

    // Undo restores the last point of the previous PLINE, (50,0), for "@";
    // ortho has no base point again.
    start();
    runner().click({80.0, 60.0}, PickAction::Add);
    runner().submitText("u");
    runner().click({100.0, 90.0}, PickAction::Add);
    runner().click({130.0, 95.0}, PickAction::Add);
    runner().submitText("");

    ASSERT_EQ(document().entityCount(), 2U);
    const auto& second = std::get<Polyline>(document().entities().back().entity);
    ASSERT_EQ(second.vertexCount(), 2U);
    EXPECT_EQ(second.vertices()[0].position, (Vec2{100.0, 90.0}));
    EXPECT_EQ(second.vertices()[1].position, (Vec2{130.0, 90.0}));
}

TEST_F(PlineCommandTest, EditorUndoRedoAreUnavailableDuringPline) {
    start();
    triangle();
    runner().submitText("");
    start();
    point(20.0, 20.0);
    EXPECT_FALSE(editor().canUndo());
    EXPECT_FALSE(editor().canRedo());
    EXPECT_FALSE(editor().undo());
    EXPECT_FALSE(editor().redo());
    runner().submit(CancelInput{});
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_TRUE(editor().canUndo());
}

}  // namespace
}  // namespace tg::cmd
