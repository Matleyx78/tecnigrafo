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
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    void type(std::initializer_list<std::string_view> lines) {
        for (const auto line : lines) {
            m_runner.submitText(line);
        }
    }

    [[nodiscard]] const std::string& lastMessage() const { return m_messages.back(); }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] const edit::Editor& editor() const { return m_editor; }

    [[nodiscard]] const CommandRunner& runner() const { return m_runner; }

private:
    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(ChamferCommandTest, DistanceDefaultsAndNumericValidation) {
    type({"CHA", "D", "-5"});
    EXPECT_EQ(lastMessage(), "Value must be positive.");
    type({"5"});
    EXPECT_EQ(runner().prompt().text, "Specify second chamfer distance <5.0000>:");
    type({""});
    EXPECT_TRUE(runner().isActive());
    type({"A", "-1"});
    EXPECT_EQ(lastMessage(), "Value must be positive.");
    type({"6", "bad"});
    EXPECT_EQ(lastMessage(), "Requires numeric distance.");
    type({"-30"});
    EXPECT_EQ(lastMessage(), "Value must be positive.");
}

TEST_F(ChamferCommandTest, InvalidGeometryEndsUnlessMultipleIsActive) {
    (void)document().add(Line{{0, 0}, {10, 0}});
    (void)document().add(Line{{0, 2}, {10, 2}});
    type({"CHAMFER", "5,0", "5,2"});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(lastMessage(), "Lines are parallel.");
    EXPECT_TRUE(editor().undoNames().empty());

    type({"CHAMFER", "M", "5,0", "5,2"});
    EXPECT_TRUE(runner().isActive());
    EXPECT_TRUE(runner().prompt().text.starts_with("Select first line"));
    EXPECT_EQ(lastMessage(), "Lines are parallel.");
    type({""});
    EXPECT_TRUE(editor().undoNames().empty());

    const core::Arc arc{{20, 0}, 5, 0, core::kPi};
    (void)document().add(arc);
    type({"CHAMFER", "20,5", "5,0"});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(lastMessage(), "Cannot chamfer those objects.");
    EXPECT_TRUE(editor().undoNames().empty());
}

TEST_F(ChamferCommandTest, OversizedAndZeroDistancesHaveDifferentDiagnoses) {
    (void)document().add(Line{{200, 0}, {210, 0}});
    (void)document().add(Line{{200, 0}, {200, 10}});
    type({"CHAMFER", "D", "15", "15", "205,0", "200,5"});
    EXPECT_EQ(lastMessage(), "Chamfer distance too large.");
    EXPECT_FALSE(runner().isActive());
    type({"CHAMFER", "D", "0", "15", "205,0", "200,5"});
    EXPECT_EQ(lastMessage(), "Lines are parallel.");
    EXPECT_FALSE(runner().isActive());
    type({"CHAMFER", "D", "15", "0", "200,5", "205,0"});
    EXPECT_EQ(lastMessage(), "Lines are parallel.");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 2U);
    EXPECT_TRUE(editor().undoNames().empty());
}

TEST_F(ChamferCommandTest, OmittedCornerHasItsOwnMessage) {
    (void)document().add(Line{{0, 0}, {1, 0}});
    (void)document().add(Line{{0, 1}, {1, 1.0000000000000002}});
    type({"CHAMFER", "0.5,0", "0.5,1"});
    EXPECT_EQ(lastMessage(), "Result is beyond the drawing limits.");
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(editor().undoNames().empty());
}

TEST_F(ChamferCommandTest, NoTrimAddsSeparateLinesWithThePolylineProperties) {
    const Polyline polyline{{{{0, 10}}, {{0, 0}}, {{10, 0}}}};
    const auto first = document().add(polyline);
    const Polyline shifted{{{{20, 10}}, {{20, 0}}, {{30, 0}}}};
    const auto second = document().add(shifted);
    auto properties = document().find(first)->properties;
    properties.color = core::Color::fromIndex(1);
    document().setProperties(first, properties);
    properties.color = core::Color::fromIndex(2);
    document().setProperties(second, properties);

    type({"CHA", "T", "N", "D", "1", "1", "0,5", "5,0"});
    ASSERT_FALSE(runner().isActive());
    ASSERT_EQ(document().entityCount(), 3U);
    EXPECT_EQ(std::get<Polyline>(document().find(first)->entity), polyline);
    EXPECT_EQ(std::get<Line>(document().entities().back().entity), (Line{{0, 1}, {1, 0}}));
    EXPECT_EQ(document().entities().back().properties, document().find(first)->properties);
    EXPECT_EQ(editor().undoNames(), std::vector<std::string>{"Chamfer"});
    type({"U"});
    EXPECT_EQ(document().entityCount(), 2U);
    type({"REDO"});
    EXPECT_EQ(document().entityCount(), 3U);

    type({"CHA", "P", "25,0"});
    ASSERT_FALSE(runner().isActive());
    ASSERT_EQ(document().entityCount(), 4U);
    EXPECT_EQ(std::get<Polyline>(document().find(second)->entity), shifted);
    EXPECT_EQ(std::get<Line>(document().entities().back().entity), (Line{{20, 1}, {21, 0}}));
    EXPECT_EQ(document().entities().back().properties, document().find(second)->properties);
    type({"U"});
    EXPECT_EQ(document().entityCount(), 3U);
}

}  // namespace
}  // namespace tg::cmd
