// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

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

class FilletCommandTest : public testing::Test {
protected:
    FilletCommandTest() {
        registerBuiltinCommands(m_registry);
        m_document.setUnits(4);
        m_runner.setPixelSize(0.01);
        m_runner.setMessageSink([this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    void type(std::initializer_list<std::string_view> lines) {
        for (const auto line : lines) {
            m_runner.submitText(line);
        }
    }

    [[nodiscard]] std::string lastMessage() const { return m_messages.back(); }

    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(FilletCommandTest, ErasedSourcesAndTheirArcShareOneUndoStep) {
    const Line first{{0, 0}, {1, 0}};
    const Line second{{0, 0}, {0, 1}};
    const auto firstId = m_document.add(first);
    const auto secondId = m_document.add(second);

    type({"F", "R", "1", "1,0", "0,1"});
    ASSERT_FALSE(m_runner.isActive());
    ASSERT_EQ(m_document.entityCount(), 1U);
    EXPECT_EQ(m_document.find(firstId), nullptr);
    EXPECT_EQ(m_document.find(secondId), nullptr);
    ASSERT_TRUE(std::holds_alternative<Arc>(m_document.entities().front().entity));
    EXPECT_EQ(std::get<Arc>(m_document.entities().front().entity).center, (core::Vec2{1, 1}));
    EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Fillet"});

    type({"U"});
    ASSERT_EQ(m_document.entityCount(), 2U);
    ASSERT_NE(m_document.find(firstId), nullptr);
    ASSERT_NE(m_document.find(secondId), nullptr);
    EXPECT_EQ(m_document.find(firstId)->entity, core::Entity{first});
    EXPECT_EQ(m_document.find(secondId)->entity, core::Entity{second});
    type({"REDO"});
    ASSERT_EQ(m_document.entityCount(), 1U);
    EXPECT_TRUE(std::holds_alternative<Arc>(m_document.entities().front().entity));
}

TEST_F(FilletCommandTest, ShiftAppliesTheCornerWithoutChangingTheDefaultRadius) {
    const auto first = m_document.add(Line{{-10, 0}, {20, 0}});
    const auto second = m_document.add(Line{{0, -10}, {0, 20}});
    type({"FILLET", "R", "-2"});
    EXPECT_EQ(lastMessage(), "Value must be positive.");
    type({"2", "10,0"});
    m_runner.click({0, 10}, PickAction::Remove);
    ASSERT_EQ(m_document.entityCount(), 2U);
    ASSERT_NE(m_document.find(first), nullptr);
    ASSERT_NE(m_document.find(second), nullptr);
    EXPECT_EQ(m_document.find(first)->entity, (core::Entity{Line{{0, 0}, {20, 0}}}));
    EXPECT_EQ(m_document.find(second)->entity, (core::Entity{Line{{0, 0}, {0, 20}}}));
    type({"FILLET", "R"});
    EXPECT_EQ(m_runner.promptText(), "Specify fillet radius <2.0000>:");
}

TEST_F(FilletCommandTest, ParallelLinesUseTheSemicircleAndUndoWithinMultiple) {
    m_document.add(Line{{100, 0}, {130, 0}});
    m_document.add(Line{{100, 20}, {130, 20}});
    type({"FILLET", "R", "5", "M", "120,0", "120,20"});
    ASSERT_EQ(m_document.entityCount(), 3U);
    ASSERT_TRUE(std::holds_alternative<Arc>(m_document.entities().back().entity));
    const auto& arc = std::get<Arc>(m_document.entities().back().entity);
    EXPECT_EQ(arc.center, (core::Vec2{130, 10}));
    EXPECT_EQ(arc.radius, 10.0);

    type({"U"});
    EXPECT_EQ(m_document.entityCount(), 2U);
    type({"120,0", "120,20"});
    m_runner.submit(CancelInput{});
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_EQ(m_document.entityCount(), 3U);
    EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Fillet"});
}

TEST_F(FilletCommandTest, NoTrimKeepsPolylineAndCopiesItsPropertiesInBothModes) {
    for (const bool whole : {false, true}) {
        const double offset = whole ? 20.0 : 0.0;
        Polyline polyline;
        polyline.addVertex({offset, 10});
        polyline.addVertex({offset, 0});
        polyline.addVertex({offset + 10, 0});
        const auto id = m_document.add(polyline);
        auto properties = m_document.current();
        properties.color = core::Color::fromIndex(3);
        m_document.setProperties(id, properties);
        const auto before = m_document.entityCount();

        if (whole) {
            type({"FILLET", "P", "25,0"});
            EXPECT_EQ(lastMessage(), "1 lines were filleted");
        } else {
            type({"FILLET", "R", "1", "T", "N", "0,5", "5,0"});
        }
        ASSERT_EQ(m_document.entityCount(), before + 1);
        ASSERT_NE(m_document.find(id), nullptr);
        EXPECT_EQ(m_document.find(id)->entity, core::Entity{polyline});
        EXPECT_EQ(m_document.entities().back().properties, properties);
        ASSERT_TRUE(std::holds_alternative<Arc>(m_document.entities().back().entity));
        EXPECT_EQ(std::get<Arc>(m_document.entities().back().entity).center,
                  (core::Vec2{offset + 1, 1}));
        EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Fillet"});
        type({"U"});
        EXPECT_EQ(m_document.entityCount(), before);
    }
}

TEST_F(FilletCommandTest, ImpossibleCornerEndsUnlessMultipleIsActive) {
    m_document.add(Circle{{200, 0}, 5});
    m_document.add(Circle{{200, 0}, 10});
    type({"FILLET", "R", "5", "200,5", "200,10"});
    EXPECT_EQ(lastMessage(), "Cannot fillet those objects.");
    EXPECT_FALSE(m_runner.isActive());

    type({"FILLET", "M", "200,5", "200,10"});
    EXPECT_EQ(lastMessage(), "Cannot fillet those objects.");
    EXPECT_EQ(m_runner.promptText(), "Select first object or [Undo/Polyline/Radius/Trim/Multiple]:");
    type({""});
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_EQ(m_document.entityCount(), 2U);
    EXPECT_FALSE(m_editor.canUndo());
}

TEST_F(FilletCommandTest, RadiusTooLargeEndsUnlessMultipleAndLeavesBothLines) {
    const Line first{{2, 0}, {5, 0}};
    const Line reversed{first.end, first.start};
    const Line second{{0, 0}, {0, 30}};
    const auto id = m_document.add(first);
    m_document.add(second);
    type({"FILLET", "R", "10", "3,0", "0,20"});
    EXPECT_EQ(lastMessage(), "Radius is too large.");
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_FALSE(m_editor.canUndo());

    m_document.replaceEntity(id, reversed);
    type({"FILLET", "T", "N", "M", "3,0", "0,20"});
    EXPECT_EQ(lastMessage(), "Radius is too large.");
    EXPECT_EQ(m_runner.promptText(), "Select first object or [Undo/Polyline/Radius/Trim/Multiple]:");
    type({""});
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_FALSE(m_editor.canUndo());
    ASSERT_EQ(m_document.entityCount(), 2U);
    EXPECT_EQ(m_document.entities().front().entity, core::Entity{reversed});
    EXPECT_EQ(m_document.entities().back().entity, core::Entity{second});
}

TEST_F(FilletCommandTest, AmbiguousPickReportsItsOwnDiagnostic) {
    m_document.add(Circle{{0, 0}, 10});
    m_document.add(Line{{-20, -10}, {20, -10}});
    type({"FILLET", "R", "5", "10,0", "0,-10"});
    EXPECT_EQ(lastMessage(), "Pick a point that is not on the other object.");
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_FALSE(m_editor.canUndo());
}

TEST_F(FilletCommandTest, BeyondDomainAlsoWithShiftAndParallelLines) {
    m_document.add(Line{{0, 0}, {1, 0}});
    m_document.add(Line{{0, 1}, {1, 1.0000000000000002}});
    type({"FILLET", "R", "1", "0.5,0", "0.5,1"});
    EXPECT_EQ(lastMessage(), "Result is beyond the drawing limits.");
    EXPECT_FALSE(m_runner.isActive());
    type({"FILLET", "0.5,0"});
    m_runner.click({0.5, 1}, PickAction::Remove);
    EXPECT_EQ(lastMessage(), "Result is beyond the drawing limits.");
    EXPECT_FALSE(m_runner.isActive());

    constexpr double far = 0x1p50;
    m_document.add(Line{{far, -far}, {0, 0}});
    m_document.add(Line{{0, far}, {far, 0}});
    type({"FILLET"});
    m_runner.submit(PointInput{{far, -far}});
    m_runner.submit(PointInput{{0, far}});
    EXPECT_EQ(lastMessage(), "Result is beyond the drawing limits.");
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_FALSE(m_editor.canUndo());
}

TEST_F(FilletCommandTest, SettingsFinishInScriptSequencesAndAllowAnOversizedInput) {
    m_document.add(Line{{0, 0}, {10, 0}});
    m_document.add(Line{{0, 0}, {0, 10}});
    m_runner.setScriptSequences(true);
    type({"FILLET", "R", "2e15"});
    EXPECT_FALSE(m_runner.isActive());
    type({"FILLET", "T", "N"});
    EXPECT_FALSE(m_runner.isActive());
    type({"FILLET", "5,0", "0,5"});
    EXPECT_EQ(lastMessage(), "Cannot fillet those objects.");
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_FALSE(m_editor.canUndo());
}

}  // namespace
}  // namespace tg::cmd
