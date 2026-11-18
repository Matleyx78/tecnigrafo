// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/polyline.hpp"

#include <gtest/gtest.h>

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

core::Polyline rectangle() {
    core::Polyline result;
    result.addVertex({0, 0});
    result.addVertex({60, 0});
    result.addVertex({60, 40});
    result.addVertex({0, 40});
    result.setClosed(true);
    return result;
}

class HatchTest : public testing::Test {
protected:
    HatchTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setPixelSize(0.1);
        m_runner.setMessageSink(
            [this](std::string message) { m_messages.push_back(std::move(message)); });
        m_runner.setDescriptionCheck([](const std::string& error) { ADD_FAILURE() << error; });
        m_document.add(rectangle());
    }

    void type(std::initializer_list<std::string_view> lines) {
        for (const std::string_view line : lines) {
            m_runner.submitText(line);
        }
    }

    [[nodiscard]] const core::Hatch& hatch() const {
        return std::get<core::Hatch>(m_document.entities().back().entity);
    }

    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(HatchTest, CreatesHatchFromPointAndDoesNotDuplicateSameArea) {
    type({"-HATCH", "30,20", "30,20", ""});

    ASSERT_EQ(m_document.entities().size(), 2U) << testing::PrintToString(m_messages);
    EXPECT_EQ(hatch().pattern, "ANSI31");
    EXPECT_EQ(hatch().islands, core::IslandStyle::Outer);
    EXPECT_EQ(hatch().seeds, (std::vector<core::Vec2>{{30, 20}, {30, 20}}));
    EXPECT_EQ(hatch().loops.size(), 1U);
    EXPECT_EQ(m_editor.undoName(), "Hatch");
}

TEST_F(HatchTest, RemembersNetPropertiesAndReducesAngle) {
    type({"HATCH", "P", "NET", "2", "390", "30,20", ""});

    ASSERT_EQ(m_document.entities().size(), 2U);
    EXPECT_EQ(hatch().pattern, "NET");
    EXPECT_DOUBLE_EQ(hatch().scale, 2.0);
    EXPECT_DOUBLE_EQ(hatch().angle, core::reducedDegreesToRadians(390.0));
    type({"HATCH", "P", "", "", "", "30,20", ""});
    EXPECT_EQ(hatch().pattern, "NET");
    EXPECT_DOUBLE_EQ(hatch().scale, 2.0);
    EXPECT_DOUBLE_EQ(hatch().angle, core::reducedDegreesToRadians(30.0));
}

TEST_F(HatchTest, IslandVariableAcceptsOnlyZeroThroughTwoAndSolidSkipsDensity) {
    type({"HPISLANDDETECTION", "3", "2", "-HATCH", "P", "S", "30,20", ""});

    ASSERT_EQ(m_document.entities().size(), 2U) << testing::PrintToString(m_messages);
    EXPECT_TRUE(hatch().solid);
    EXPECT_EQ(hatch().pattern, "SOLID");
    EXPECT_EQ(hatch().islands, core::IslandStyle::Ignore);
    EXPECT_TRUE(hatch().patternLines.empty());
    EXPECT_NE(std::ranges::find(m_messages, "Requires an integer between 0 and 2."),
              m_messages.end());
}

}  // namespace
}  // namespace tg::cmd
