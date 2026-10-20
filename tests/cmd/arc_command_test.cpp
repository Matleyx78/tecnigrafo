// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/arc_command.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

class ArcCommandTest : public testing::Test {
protected:
    ArcCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string message) { m_messages.push_back(std::move(message)); });
    }

    void inputs(std::initializer_list<const char*> lines) {
        for (const char* line : lines) {
            m_runner.submitText(line);
        }
    }

    [[nodiscard]] const core::Arc& arc() const {
        return std::get<core::Arc>(m_document.entities().back().entity);
    }

    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(ArcCommandTest, ThreePointsBothOrientations) {
    inputs({"A", "10,0", "0,10", "-10,0"});
    ASSERT_EQ(m_document.entityCount(), 1U);
    EXPECT_NEAR(arc().center.x, 0.0, 1e-12);
    EXPECT_NEAR(arc().center.y, 0.0, 1e-12);
    EXPECT_NEAR(arc().radius, 10.0, 1e-12);
    EXPECT_NEAR(arc().startAngle, 0.0, 1e-12);
    EXPECT_NEAR(arc().endAngle, core::kPi, 1e-12);
    inputs({"ARC", "10,0", "0,-10", "-10,0"});
    ASSERT_EQ(m_document.entityCount(), 2U);
    EXPECT_NEAR(arc().startAngle, core::kPi, 1e-12);
    EXPECT_NEAR(arc().endAngle, 0.0, 1e-12);
    EXPECT_EQ(m_editor.undoName(), "Arc");
    inputs({"U"});
    EXPECT_EQ(m_document.entityCount(), 1U);
    inputs({"REDO"});
    EXPECT_EQ(m_document.entityCount(), 2U);
}

TEST_F(ArcCommandTest, CenterEndCrossesZeroAndTracksFinalPoint) {
    inputs({"A", "0,-10", "C", "0,0", "10,0"});
    ASSERT_EQ(m_document.entityCount(), 1U);
    EXPECT_NEAR(arc().startAngle, 1.5 * core::kPi, 1e-12);
    EXPECT_NEAR(arc().endAngle, 0.0, 1e-12);
    inputs({"L", "@0,0", "@0,1", ""});
    EXPECT_EQ(m_document.entityCount(), 2U);
    const auto& line = std::get<core::Line>(m_document.entities().back().entity);
    EXPECT_EQ(line.start, (core::Vec2{10, 0}));
}

TEST_F(ArcCommandTest, InvalidGeometryRetries) {
    inputs({"A", "0,0", "10,0"});
    inputs({"20,0", "5,1e-9", "10,0"});
    EXPECT_EQ(m_runner.promptText(), "Specify end point of arc:");
    ASSERT_FALSE(m_messages.empty());
    EXPECT_EQ(m_messages.back(), "The three points are collinear.");
    EXPECT_TRUE(m_document.isEmpty());
    inputs({"0,10"});
    EXPECT_EQ(m_document.entityCount(), 1U);
    inputs({"A", "10,0", "C", "10,0"});
    EXPECT_EQ(m_messages.back(), "Center coincides with the start point.");
    EXPECT_EQ(m_runner.promptText(), "Specify center point of arc:");
    inputs({"0,0", "20,0"});
    EXPECT_EQ(m_messages.back(), "End point gives a zero-length arc.");
    EXPECT_EQ(m_runner.promptText(), "Specify end point of arc:");
    inputs({"0,10"});
    EXPECT_EQ(m_document.entityCount(), 2U);
}

TEST_F(ArcCommandTest, CancelAtEveryStageRollsBack) {
    std::vector<std::vector<const char*>> prefixes;
    prefixes.push_back({"A"});
    prefixes.push_back({"A", "10,0"});
    prefixes.push_back({"A", "10,0", "0,10"});
    prefixes.push_back({"A", "10,0", "C"});
    prefixes.push_back({"A", "10,0", "C", "0,0"});
    for (const auto& prefix : prefixes) {
        for (const char* text : prefix) {
            m_runner.submitText(text);
        }
        m_runner.submit(CancelInput{});
        EXPECT_TRUE(m_document.isEmpty());
        EXPECT_FALSE(m_editor.inTransaction());
        EXPECT_FALSE(m_runner.isActive());
    }
}

TEST(ArcCommandPreview, PreviewUsesAvailablePoints) {
    core::Document document;
    edit::Editor editor(document);
    CommandContext context{editor, {}, [](const std::string&) {
                           }};
    ArcCommand command;
    command.start(context);
    EXPECT_FALSE(command.basePoint());
    command.handle(context, PointInput{{10, 0}});
    EXPECT_EQ(command.basePoint(), (core::Vec2{10, 0}));
    command.handle(context, PointInput{{0, 10}});
    command.pointerMoved({-10, 0});
    ASSERT_EQ(command.preview().size(), 1U);
    EXPECT_TRUE(std::holds_alternative<core::Arc>(command.preview().front()));
    command.pointerMoved({20, -10});
    EXPECT_TRUE(command.preview().empty());
    command.cancel(context);

    command.start(context);
    command.handle(context, PointInput{{10, 0}});
    command.handle(context, KeywordInput{"Center"});
    command.handle(context, PointInput{{0, 0}});
    command.pointerMoved({0, 10});
    ASSERT_EQ(command.preview().size(), 1U);
    EXPECT_NEAR(std::get<core::Arc>(command.preview().front()).radius, 10.0, 1e-12);
    command.cancel(context);
}

}  // namespace
}  // namespace tg::cmd
