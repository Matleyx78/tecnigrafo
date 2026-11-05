// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/line.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/text.hpp"

#include <gtest/gtest.h>

#include <initializer_list>
#include <numbers>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

using core::Arc;
using core::Circle;
using core::Line;

Line horizontal(double y) {
    return {{0, y}, {20, y}};
}

core::Polyline openPolyline(double y) {
    core::Polyline polyline;
    polyline.addVertex({150, y});
    polyline.addVertex({170, y});
    return polyline;
}

class OffsetTest : public testing::Test {
protected:
    OffsetTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setPixelSize(0.1);
        m_runner.setMessageSink([this](std::string message) {
            m_messages.push_back(std::move(message));
        });
        m_runner.setDescriptionCheck([this](const std::string& error) { ADD_FAILURE() << error; });
    }

    void type(std::initializer_list<std::string_view> lines) {
        for (const std::string_view line : lines) {
            m_runner.submitText(line);
        }
    }

    void expectCopy(std::string_view object, std::string_view side, const core::Entity& expected) {
        const auto count = m_document.entities().size();
        type({object, side});
        ASSERT_EQ(m_document.entities().size(), count + 1);
        EXPECT_EQ(m_document.entities().back().entity, expected);
    }

    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(OffsetTest, OffsetsEveryCurveTypeOnEitherSideByDistanceAndThrough) {
    constexpr double kQuarterTurn = std::numbers::pi / 2.0;
    m_document.add(horizontal(0));
    m_document.add(Circle{{50, 0}, 10});
    m_document.add(Arc{{100, 0}, 10, 0, kQuarterTurn});
    m_document.add(openPolyline(0));

    type({"O", "2"});
    expectCopy("10,0", "10,8", horizontal(2));
    expectCopy("10,0", "10,-8", horizontal(-2));
    expectCopy("60,0", "50,20", Circle{{50, 0}, 12});
    expectCopy("60,0", "50,5", Circle{{50, 0}, 8});
    expectCopy("110,0", "100,20", Arc{{100, 0}, 12, 0, kQuarterTurn});
    expectCopy("110,0", "100,5", Arc{{100, 0}, 8, 0, kQuarterTurn});
    expectCopy("160,0", "160,8", openPolyline(2));
    expectCopy("160,0", "160,-8", openPolyline(-2));
    type({""});

    type({"OFFSET", "Through"});
    expectCopy("10,0", "10,4", horizontal(4));
    expectCopy("10,0", "10,-6", horizontal(-6));
    expectCopy("60,0", "50,15", Circle{{50, 0}, 15});
    expectCopy("60,0", "50,6", Circle{{50, 0}, 6});
    expectCopy("110,0", "100,15", Arc{{100, 0}, 15, 0, kQuarterTurn});
    expectCopy("110,0", "100,6", Arc{{100, 0}, 6, 0, kQuarterTurn});
    expectCopy("160,0", "160,4", openPolyline(4));
    expectCopy("160,0", "160,-6", openPolyline(-6));
    type({"Exit"});
    EXPECT_EQ(m_editor.undoName(), "Offset");
    EXPECT_FALSE(m_runner.isActive());
}

TEST_F(OffsetTest, RejectsAnInvalidDistanceAndRemembersTheLastMode) {
    m_document.add(horizontal(0));
    type({"OFFSET", "0", "-1"});
    ASSERT_EQ(m_messages.size(), 3U);
    EXPECT_EQ(m_messages[1], "Value must be positive and nonzero.");
    EXPECT_EQ(m_messages[2], m_messages[1]);

    // Two points specify a distance without using a previous command's last point.
    type({"0,5", "3,9"});
    expectCopy("10,0", "10,1", horizontal(5));
    type({"", "OFFSET", ""});
    expectCopy("10,0", "10,-1", horizontal(-5));
    type({"", "OFFSET", "T", "", "OFFSET", ""});
    expectCopy("10,0", "10,3", horizontal(3));
    type({"Exit"});
}

TEST_F(OffsetTest, FailedAttemptsCreateNoCopyOrUndoStep) {
    constexpr double kQuarterTurn = std::numbers::pi / 2.0;
    m_document.add(horizontal(0));
    m_document.add(Arc{{50, 0}, 10, 0, kQuarterTurn});
    m_document.add(Line{{80, 0}, {80, 0}});
    core::Text text;
    text.position = {100, 0};
    text.value = "Text";
    m_document.add(text);

    type({"OFFSET", "10", "60,0", "50,5", "80,0", "80,5", "100,0", ""});
    type({"OFFSET", "T", "10,0", "5,0", ""});
    EXPECT_EQ(m_document.entities().size(), 4U);
    EXPECT_FALSE(m_editor.canUndo());
    ASSERT_EQ(m_messages.size(), 6U);
    EXPECT_EQ(m_messages[1], "Cannot offset that object.");
    EXPECT_EQ(m_messages[2], m_messages[1]);
    EXPECT_EQ(m_messages[3], m_messages[1]);
    EXPECT_EQ(m_messages[5], m_messages[1]);
}

TEST_F(OffsetTest, EraseAndCurrentLayerCarryOverAndUndoRestoresTheSource) {
    const auto source = m_document.add(horizontal(0));
    type({"-LAYER", "Make", "Target", "", "OFFSET", "E", "Y", "L", "C", "5"});
    type({"10,0", "10,6"});
    EXPECT_EQ(m_document.find(source), nullptr);
    ASSERT_EQ(m_document.entities().size(), 1U);
    EXPECT_EQ(m_document.entities().front().entity, core::Entity{horizontal(5)});
    EXPECT_EQ(m_document.entities().front().properties.layer, m_document.current().layer);

    type({"U"});
    ASSERT_EQ(m_document.entities().size(), 1U);
    EXPECT_EQ(m_document.entities().front().id, source);
    type({"", "OFFSET", ""});
    type({"10,0", "10,-6", ""});
    ASSERT_EQ(m_document.entities().size(), 1U);
    EXPECT_EQ(m_document.entities().front().entity, core::Entity{horizontal(-5)});
    EXPECT_EQ(m_document.entities().front().properties.layer, m_document.current().layer);
}

TEST_F(OffsetTest, MultipleUndoUsesThePreviousCopyAndEscapeCommitsBoth) {
    m_document.add(horizontal(0));
    type({"OFFSET", "5", "10,0", "M", "10,5", "10,10", "U", "10,10"});
    ASSERT_EQ(m_document.entities().size(), 3U);
    EXPECT_EQ(m_document.entities()[1].entity, core::Entity{horizontal(5)});
    EXPECT_EQ(m_document.entities()[2].entity, core::Entity{horizontal(10)});

    m_runner.submit(CancelInput{});
    ASSERT_FALSE(m_runner.isActive());
    EXPECT_EQ(m_editor.undoName(), "Offset");
    EXPECT_EQ(m_document.entities().size(), 3U);
    type({"U"});
    EXPECT_EQ(m_document.entities().size(), 1U);
    type({"REDO"});
    ASSERT_EQ(m_document.entities().size(), 3U);
    EXPECT_EQ(m_document.entities()[1].entity, core::Entity{horizontal(5)});
    EXPECT_EQ(m_document.entities()[2].entity, core::Entity{horizontal(10)});
}

}  // namespace
}  // namespace tg::cmd
