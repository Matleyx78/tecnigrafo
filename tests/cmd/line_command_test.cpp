// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

using core::Line;

class LineCommandTest : public testing::Test {
protected:
    LineCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
        m_runner.setPixelSize(0.1);
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    [[nodiscard]] const Line& line(std::size_t index) const {
        return std::get<Line>(document().entities()[index].entity);
    }

    void point(double x, double y) { runner().submit(PointInput{{x, y}}); }

    void triangle() {
        point(0.0, 0.0);
        point(10.0, 0.0);
        point(10.0, 10.0);
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(LineCommandTest, NamesAndAliasesAddSeparateSegmentsImmediately) {
    for (const char* name : {"LINE", "L", "line", "l"}) {
        runner().submitText(name);
        EXPECT_EQ(runner().promptText(), "Specify first point:");
        triangle();
        ASSERT_EQ(document().entityCount(), 2U);
        EXPECT_EQ(line(0), (Line{{0.0, 0.0}, {10.0, 0.0}}));
        EXPECT_EQ(line(1), (Line{{10.0, 0.0}, {10.0, 10.0}}));
        EXPECT_TRUE(editor().inTransaction());
        runner().submitText("");
        EXPECT_FALSE(runner().isActive());
        EXPECT_FALSE(editor().inTransaction());
        EXPECT_EQ(editor().undoName(), "Line");
        runner().submitText("U");
        EXPECT_TRUE(document().isEmpty());
    }
}

TEST_F(LineCommandTest, CloseRequiresThreePointsAndAddsTheClosingSegment) {
    for (const char* keyword : {"c", "cLoSe"}) {
        runner().submitText("L");
        point(0.0, 0.0);
        EXPECT_EQ(runner().promptText(), "Specify next point or [Undo]:");
        point(10.0, 0.0);
        runner().submitText("C");
        EXPECT_EQ(messages().back(), "Invalid point or option.");
        EXPECT_TRUE(runner().isActive());
        point(10.0, 10.0);
        EXPECT_EQ(runner().promptText(), "Specify next point or [Close/Undo]:");
        runner().submitText(keyword);
        EXPECT_FALSE(runner().isActive());
        ASSERT_EQ(document().entityCount(), 3U);
        EXPECT_EQ(line(2), (Line{{10.0, 10.0}, {0.0, 0.0}}));
        runner().submitText("U");
    }
}

TEST_F(LineCommandTest, UndoRemovesTheLastSegmentAndResumesFromThePreviousPoint) {
    runner().submitText("L");
    triangle();
    const core::EntityId removed = document().entities().back().id;
    runner().submitText("uNdO");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(runner().promptText(), "Specify next point or [Undo]:");
    runner().submitText("@0,20");
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(line(1), (Line{{10.0, 0.0}, {10.0, 20.0}}));
    EXPECT_NE(document().entities().back().id, removed);
    const std::vector<core::EntityRecord> originals{document().entities().begin(),
                                                    document().entities().end()};
    runner().submitText("");
    runner().submitText("U");
    EXPECT_TRUE(document().isEmpty());
    runner().submitText("REDO");
    EXPECT_EQ(
        std::vector<core::EntityRecord>(document().entities().begin(), document().entities().end()),
        originals);
}

TEST_F(LineCommandTest, UndoFirstPointRestoresThePreviousCommandReference) {
    runner().submitText("L");
    triangle();
    runner().submitText("");
    runner().submitText("L");
    point(100.0, 100.0);
    runner().submitText("U");
    EXPECT_EQ(runner().promptText(), "Specify first point:");
    EXPECT_TRUE(runner().preview().empty());
    runner().submitText("@5,0");
    runner().submitText("@5,0");
    ASSERT_EQ(document().entityCount(), 3U);
    EXPECT_EQ(line(2), (Line{{15.0, 10.0}, {20.0, 10.0}}));
    runner().submitText("");
}

TEST_F(LineCommandTest, EscKeepsSegmentsInOneUndoStepWithStableIdentifiers) {
    runner().submitText("L");
    triangle();
    const std::vector<core::EntityRecord> originals{document().entities().begin(),
                                                    document().entities().end()};
    runner().pointerMoved({50.0, 50.0});
    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(runner().preview().empty());
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(messages().back(), "*Cancel*");
    runner().submitText("U");
    EXPECT_TRUE(document().isEmpty());
    EXPECT_FALSE(editor().canUndo());
    runner().submitText("REDO");
    EXPECT_EQ(
        std::vector<core::EntityRecord>(document().entities().begin(), document().entities().end()),
        originals);
}

TEST_F(LineCommandTest, EmptyAndFullyUndoneCommandsPreserveRedo) {
    runner().submitText("L");
    triangle();
    runner().submitText("");
    runner().submitText("U");
    for (const bool cancel : {false, true}) {
        for (const int points : {0, 1, 3}) {
            runner().submitText("L");
            for (int index = 0; index < points; ++index) {
                point(static_cast<double>(index), 0.0);
            }
            for (int index = 0; index < points; ++index) {
                runner().submitText("U");
            }
            runner().submit(cancel ? Input{CancelInput{}} : Input{ConfirmInput{}});
            EXPECT_FALSE(runner().isActive());
            EXPECT_FALSE(editor().inTransaction());
            EXPECT_TRUE(document().isEmpty());
            EXPECT_FALSE(editor().canUndo());
            EXPECT_TRUE(editor().canRedo());
        }
    }
}

TEST_F(LineCommandTest, EscBeforeAnySegmentLeavesNoEntityOrUndoStep) {
    for (const bool firstPoint : {false, true}) {
        runner().submitText("L");
        if (firstPoint) {
            point(1.0, 2.0);
        }
        runner().submit(CancelInput{});
        EXPECT_TRUE(document().isEmpty());
        EXPECT_FALSE(editor().canUndo());
        EXPECT_FALSE(editor().inTransaction());
    }
}

TEST_F(LineCommandTest, CoincidentPointsKeepThePromptAndLastPoint) {
    runner().submitText("L");
    point(0.0, 0.0);
    point(0.0, 0.0);
    point(0.5e-9, 0.0);
    ASSERT_EQ(messages().size(), 2U);
    EXPECT_EQ(messages().back(), "Point coincides with the previous point.");
    EXPECT_EQ(runner().promptText(), "Specify next point or [Undo]:");
    EXPECT_TRUE(document().isEmpty());
    runner().submitText("@10,0");
    point(10.0, 0.0);
    runner().submitText("@0,10");
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(line(0), (Line{{0.0, 0.0}, {10.0, 0.0}}));
    EXPECT_EQ(line(1), (Line{{10.0, 0.0}, {10.0, 10.0}}));
    runner().submitText("");
}

TEST_F(LineCommandTest, PreviewContainsOnlyTheSegmentToThePointerAndFollowsUndo) {
    runner().submitText("L");
    runner().pointerMoved({40.0, 50.0});
    EXPECT_TRUE(runner().preview().empty());
    triangle();
    ASSERT_EQ(runner().preview().size(), 1U);
    EXPECT_EQ(std::get<Line>(runner().preview().front()), (Line{{10.0, 10.0}, {40.0, 50.0}}));
    runner().submitText("U");
    ASSERT_EQ(runner().preview().size(), 1U);
    EXPECT_EQ(std::get<Line>(runner().preview().front()), (Line{{10.0, 0.0}, {40.0, 50.0}}));
    runner().pointerMoved({10.0, 0.0});
    EXPECT_TRUE(runner().preview().empty());
    runner().submit(ConfirmInput{});
    EXPECT_EQ(document().entityCount(), 1U);
}

TEST_F(LineCommandTest, SnapFindsEndpointsAndMidpointsOfUncommittedSegments) {
    runner().submitText("L");
    point(0.0, 0.0);
    point(100.0, 0.0);
    runner().pointerMoved({50.2, 0.3});
    EXPECT_EQ(runner().activeSnap(),
              (std::optional{core::SnapPoint{{50.0, 0.0}, core::SnapKind::Midpoint}}));
    runner().click({50.2, 0.3}, PickAction::Add);
    EXPECT_EQ(line(1), (Line{{100.0, 0.0}, {50.0, 0.0}}));
    runner().pointerMoved({0.2, 0.3});
    EXPECT_EQ(runner().activeSnap(),
              (std::optional{core::SnapPoint{{0.0, 0.0}, core::SnapKind::Endpoint}}));
    runner().click({0.2, 0.3}, PickAction::Add);
    EXPECT_EQ(line(2), (Line{{50.0, 0.0}, {0.0, 0.0}}));
    runner().submitText("");
}

TEST_F(LineCommandTest, OrthoConstrainsPreviewClicksAndTypedDistanceFromTheLastPoint) {
    DrawingAids aids;
    aids.objectSnap = false;
    aids.ortho = true;
    runner().setDrawingAids(aids);
    runner().submitText("L");
    runner().click({3.0, 4.0}, PickAction::Add);
    runner().click({20.0, 6.0}, PickAction::Add);
    EXPECT_EQ(line(0), (Line{{3.0, 4.0}, {20.0, 4.0}}));
    runner().pointerMoved({21.0, 30.0});
    EXPECT_EQ(std::get<Line>(runner().preview().front()), (Line{{20.0, 4.0}, {20.0, 30.0}}));
    runner().submitText("10");
    EXPECT_EQ(line(1), (Line{{20.0, 4.0}, {20.0, 14.0}}));
    runner().submitText("");
}

TEST_F(LineCommandTest, ClickWindowAndCrossingSelectIndividualLinesForErase) {
    runner().submitText("L");
    triangle();
    runner().submitText("");
    const core::EntityId first = document().entities().front().id;
    runner().click({5.0, 0.1}, PickAction::Add);
    ASSERT_EQ(runner().selection().size(), 1U);
    EXPECT_EQ(runner().selection().ids().front(), first);
    runner().submit(CancelInput{});
    runner().click({-1.0, -1.0}, PickAction::Add);
    runner().click({11.0, 1.0}, PickAction::Add);
    ASSERT_EQ(runner().selection().size(), 1U);
    EXPECT_EQ(runner().selection().ids().front(), first);
    runner().submit(CancelInput{});
    runner().click({6.0, 1.0}, PickAction::Add);
    runner().click({4.0, -1.0}, PickAction::Add);
    ASSERT_EQ(runner().selection().size(), 1U);
    EXPECT_EQ(runner().selection().ids().front(), first);
    runner().submitText("ERASE");
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(document().find(first), nullptr);
    runner().submitText("U");
    EXPECT_NE(document().find(first), nullptr);
}

TEST_F(LineCommandTest, CloseDoesNotAddADegenerateSegmentWhenAlreadyAtTheFirstPoint) {
    runner().submitText("L");
    point(0.0, 0.0);
    point(10.0, 0.0);
    point(0.0, 0.0);
    runner().submitText("C");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 2U);
    runner().submitText("L");
    runner().submitText("@1,0");
    runner().submitText("@1,0");
    EXPECT_EQ(line(2), (Line{{1.0, 0.0}, {2.0, 0.0}}));
    runner().submitText("");
}

}  // namespace
}  // namespace tg::cmd
