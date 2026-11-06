// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

using core::Line;
using core::Polyline;

class ChamferCommandTest : public testing::Test {
protected:
    ChamferCommandTest() {
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

    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(ChamferCommandTest, DistanceDefaultsAndNumericValidation) {
    type({"CHA", "D", "-5"});
    EXPECT_EQ(m_messages.back(), "Value must be positive.");
    type({"5"});
    EXPECT_EQ(m_runner.prompt().text, "Specify second chamfer distance <5.0000>:");
    type({""});
    EXPECT_TRUE(m_runner.isActive());
    type({"A", "-1"});
    EXPECT_EQ(m_messages.back(), "Value must be positive.");
    type({"6", "bad"});
    EXPECT_EQ(m_messages.back(), "Requires numeric distance.");
    type({"-30"});
    EXPECT_EQ(m_messages.back(), "Value must be positive.");
}

TEST_F(ChamferCommandTest, InvalidGeometryEndsUnlessMultipleIsActive) {
    (void)m_document.add(Line{{0, 0}, {10, 0}});
    (void)m_document.add(Line{{0, 2}, {10, 2}});
    type({"CHAMFER", "5,0", "5,2"});
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_EQ(m_messages.back(), "Lines are parallel.");
    EXPECT_TRUE(m_editor.undoNames().empty());

    type({"CHAMFER", "M", "5,0", "5,2"});
    EXPECT_TRUE(m_runner.isActive());
    EXPECT_TRUE(m_runner.prompt().text.starts_with("Select first line"));
    EXPECT_EQ(m_messages.back(), "Lines are parallel.");
    type({""});
    EXPECT_TRUE(m_editor.undoNames().empty());

    const core::Arc arc{{20, 0}, 5, 0, core::kPi};
    (void)m_document.add(arc);
    type({"CHAMFER", "20,5", "5,0"});
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_EQ(m_messages.back(), "Cannot chamfer those objects.");
    EXPECT_TRUE(m_editor.undoNames().empty());
}

TEST_F(ChamferCommandTest, OversizedAndZeroDistancesHaveDifferentDiagnoses) {
    (void)m_document.add(Line{{200, 0}, {210, 0}});
    (void)m_document.add(Line{{200, 0}, {200, 10}});
    type({"CHAMFER", "D", "15", "15", "205,0", "200,5"});
    EXPECT_EQ(m_messages.back(), "Chamfer distance too large.");
    EXPECT_FALSE(m_runner.isActive());
    type({"CHAMFER", "D", "0", "15", "205,0", "200,5"});
    EXPECT_EQ(m_messages.back(), "Lines are parallel.");
    EXPECT_FALSE(m_runner.isActive());
    type({"CHAMFER", "D", "15", "0", "200,5", "205,0"});
    EXPECT_EQ(m_messages.back(), "Lines are parallel.");
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_EQ(m_document.entityCount(), 2U);
    EXPECT_TRUE(m_editor.undoNames().empty());
}

TEST_F(ChamferCommandTest, OmittedCornerHasItsOwnMessage) {
    (void)m_document.add(Line{{0, 0}, {1, 0}});
    (void)m_document.add(Line{{0, 1}, {1, 1.0000000000000002}});
    type({"CHAMFER", "0.5,0", "0.5,1"});
    EXPECT_EQ(m_messages.back(), "Result is beyond the drawing limits.");
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_TRUE(m_editor.undoNames().empty());
}

TEST_F(ChamferCommandTest, NoTrimAddsSeparateLinesWithThePolylineProperties) {
    const Polyline polyline{{{{0, 10}}, {{0, 0}}, {{10, 0}}}};
    const auto first = m_document.add(polyline);
    const Polyline shifted{{{{20, 10}}, {{20, 0}}, {{30, 0}}}};
    const auto second = m_document.add(shifted);
    auto properties = m_document.find(first)->properties;
    properties.color = core::Color::fromIndex(1);
    m_document.setProperties(first, properties);
    properties.color = core::Color::fromIndex(2);
    m_document.setProperties(second, properties);

    type({"CHA", "T", "N", "D", "1", "1", "0,5", "5,0"});
    ASSERT_FALSE(m_runner.isActive());
    ASSERT_EQ(m_document.entityCount(), 3U);
    EXPECT_EQ(std::get<Polyline>(m_document.find(first)->entity), polyline);
    EXPECT_EQ(std::get<Line>(m_document.entities().back().entity), (Line{{0, 1}, {1, 0}}));
    EXPECT_EQ(m_document.entities().back().properties, m_document.find(first)->properties);
    EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Chamfer"});
    type({"U"});
    EXPECT_EQ(m_document.entityCount(), 2U);
    type({"REDO"});
    EXPECT_EQ(m_document.entityCount(), 3U);

    type({"CHA", "P", "25,0"});
    ASSERT_FALSE(m_runner.isActive());
    ASSERT_EQ(m_document.entityCount(), 4U);
    EXPECT_EQ(std::get<Polyline>(m_document.find(second)->entity), shifted);
    EXPECT_EQ(std::get<Line>(m_document.entities().back().entity), (Line{{20, 1}, {21, 0}}));
    EXPECT_EQ(m_document.entities().back().properties, m_document.find(second)->properties);
    type({"U"});
    EXPECT_EQ(m_document.entityCount(), 3U);
}

}  // namespace
}  // namespace tg::cmd
