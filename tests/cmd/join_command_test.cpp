// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <array>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

using core::Arc;
using core::Circle;
using core::Line;
using core::Polyline;

constexpr std::string_view kCircleQuestion =
    "Arc segments combined form a circle. Convert to circle? [Yes/No] <Yes>:";

class JoinCommandTest : public testing::Test {
protected:
    JoinCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setPixelSize(0.01);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
        m_runner.setDescriptionCheck([](const std::string& text) { ADD_FAILURE() << text; });
    }

    void type(std::initializer_list<std::string_view> lines) {
        for (const auto line : lines) {
            m_runner.submitText(line);
        }
    }

    core::EntityId addArc(core::Vec2 center, double start, double end) {
        return m_document.add(Arc{center, 10, start * (core::kPi / 180), end * (core::kPi / 180)});
    }

    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(JoinCommandTest, MixedSelectionConsumesOnlyTheChainAndCopiesSourceProperties) {
    const auto source = m_document.add(Line{{0, 0}, {10, 0}});
    const auto detached = m_document.add(Line{{20, 0}, {30, 0}});
    const auto adjacent = m_document.add(Line{{10, 0}, {10, 10}});
    auto properties = m_document.current();
    properties.color = core::Color::fromIndex(3);
    m_document.setProperties(source, properties);

    type({"JOIN", "5,0", "25,0", "10,5", ""});
    EXPECT_EQ(m_messages.back(),
              "2 objects converted to 1 polyline, 1 object discarded from the operation.");
    ASSERT_EQ(m_document.entityCount(), 2U);
    EXPECT_EQ(m_document.find(source), nullptr);
    EXPECT_EQ(m_document.find(adjacent), nullptr);
    EXPECT_NE(m_document.find(detached), nullptr);
    EXPECT_EQ(m_document.entities().back().properties, properties);
    const auto* polyline = std::get_if<Polyline>(&m_document.entities().back().entity);
    ASSERT_NE(polyline, nullptr);
    ASSERT_EQ(polyline->vertexCount(), 3U);
    EXPECT_EQ(polyline->vertices()[1].position, (core::Vec2{10, 0}));
    EXPECT_EQ(polyline->vertices()[2].position, (core::Vec2{10, 10}));
    EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Join"});

    type({"U"});
    EXPECT_EQ(m_document.entityCount(), 3U);
    type({"REDO"});
    EXPECT_EQ(m_document.entityCount(), 2U);
}

TEST_F(JoinCommandTest, InteractiveCircleConversionCanBeDeclinedCancelledOrAccepted) {
    const auto source = addArc({0, 0}, 0, 90);
    const auto other = addArc({0, 0}, 270, 45);

    for (const auto answer : {"No", "Esc", "Yes", ""}) {
        type({"JOIN", "0,10", "0,-10", ""});
        ASSERT_EQ(m_runner.promptText(), kCircleQuestion);
        EXPECT_EQ(m_document.entityCount(), 2U);
        EXPECT_FALSE(m_editor.canUndo());

        const std::string_view choice{answer};
        const auto messagesBefore = m_messages.size();
        if (choice == "Esc") {
            m_runner.submit(CancelInput{});
        } else {
            type({choice});
        }
        EXPECT_FALSE(m_runner.isActive());
        if (choice == "No" || choice == "Esc") {
            EXPECT_EQ(m_messages.size(), messagesBefore + (choice == "Esc" ? 1U : 0U));
            EXPECT_EQ(m_document.entityCount(), 2U);
            EXPECT_FALSE(m_editor.canUndo());
            continue;
        }

        ASSERT_EQ(m_document.entityCount(), 1U);
        EXPECT_EQ(m_document.entities().front().entity, (core::Entity{Circle{{0, 0}, 10}}));
        EXPECT_EQ(m_messages.back(), "2 arcs joined and converted to circle");
        EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Join"});
        type({"U"});
        EXPECT_NE(m_document.find(source), nullptr);
        EXPECT_NE(m_document.find(other), nullptr);
        type({"REDO"});
        ASSERT_EQ(m_document.entityCount(), 1U);
        EXPECT_TRUE(std::holds_alternative<Circle>(m_document.entities().front().entity));
        type({"U"});
    }
}

TEST_F(JoinCommandTest, InteractiveArcsThatDoNotReachAFullTurnJoinWithoutAQuestion) {
    const auto source = addArc({0, 0}, 0, 90);
    addArc({0, 0}, 200, 300);

    type({"JOIN", "0,10", "5,-8.660254", ""});
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_EQ(m_messages.back(), "2 arcs joined into 1 arc");
    ASSERT_EQ(m_document.entityCount(), 1U);
    const auto* record = m_document.find(source);
    ASSERT_NE(record, nullptr);
    const auto* arc = std::get_if<Arc>(&record->entity);
    ASSERT_NE(arc, nullptr);
    EXPECT_DOUBLE_EQ(arc->startAngle, 0.0);
    EXPECT_NEAR(arc->endAngle, 300 * (core::kPi / 180), 1e-12);
}

TEST_F(JoinCommandTest, ScriptArcsAskBeforeConvertingAFullTurn) {
    m_runner.setScriptSequences(true);
    addArc({0, 0}, 0, 90);
    addArc({0, 0}, 300, 30);

    type({"JOIN", "0,10", "5,-8.660254", ""});
    ASSERT_EQ(m_runner.promptText(), kCircleQuestion);
    type({"N"});
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_EQ(m_document.entityCount(), 2U);
    EXPECT_FALSE(m_editor.canUndo());

    type({"JOIN", "0,10", "5,-8.660254", ""});
    ASSERT_EQ(m_runner.promptText(), kCircleQuestion);
    type({"Y"});
    ASSERT_EQ(m_document.entityCount(), 1U);
    EXPECT_EQ(m_document.entities().front().entity, (core::Entity{Circle{{0, 0}, 10}}));
    EXPECT_EQ(m_messages.back(), "2 arcs joined and converted to circle");
    EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Join"});
}

TEST_F(JoinCommandTest, ScriptCloseReportsExactlyWhatWasAbsorbedOrDiscarded) {
    m_runner.setScriptSequences(true);
    const auto source = addArc({80, 0}, 0, 60);
    const auto other = addArc({80, 0}, 90, 150);
    const auto detached = addArc({50, 0}, 0, 180);

    struct Case {
        bool absorb;
        bool discard;
        std::string_view message;
    };

    const auto cases = std::to_array<Case>({
        {false, false, "Arc converted to circle."},
        {true, false, "2 arcs joined and converted to circle"},
        {false, true, "Arc converted to circle, 1 object discarded from the operation."},
        {true, true,
         "2 arcs joined and converted to circle, 1 object discarded from the operation."},
    });
    for (const auto& test : cases) {
        type({"JOIN", "88.660254,5"});
        if (test.absorb) {
            type({"75,8.660254"});
        }
        if (test.discard) {
            type({"50,10"});
        }
        ASSERT_EQ(m_runner.promptText(), "Select arcs to join to source or [cLose]:");
        type({"L"});
        EXPECT_FALSE(m_runner.isActive());
        EXPECT_EQ(m_messages.back(), test.message);
        EXPECT_EQ(m_document.find(source), nullptr);
        EXPECT_EQ(m_document.find(other) == nullptr, test.absorb);
        EXPECT_NE(m_document.find(detached), nullptr);
        EXPECT_EQ(m_document.entities().back().entity, (core::Entity{Circle{{80, 0}, 10}}));
        EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Join"});
        type({"U"});
    }
}

}  // namespace
}  // namespace tg::cmd
