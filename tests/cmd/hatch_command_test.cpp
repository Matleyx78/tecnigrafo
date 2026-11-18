// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/hatch_boundary.hpp"
#include "tg/core/hatch_patterns.hpp"
#include "tg/core/polyline.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

core::Polyline rectangle(core::Vec2 low = {0, 0}, core::Vec2 high = {60, 40}) {
    core::Polyline result;
    result.addVertex(low);
    result.addVertex({high.x, low.y});
    result.addVertex(high);
    result.addVertex({low.x, high.y});
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
    }

    void type(std::initializer_list<std::string_view> lines) {
        for (const std::string_view line : lines) {
            m_runner.submitText(line);
        }
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] const edit::Editor& editor() const { return m_editor; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    [[nodiscard]] const core::Hatch& hatch() const {
        return std::get<core::Hatch>(m_document.find(m_document.entities().back().id)->entity);
    }

private:
    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(HatchTest, CreatesHatchFromPointAndDoesNotDuplicateSameArea) {
    document().add(rectangle());
    type({"-HATCH", "30,20", "30,20", ""});

    ASSERT_EQ(document().entities().size(), 2U) << testing::PrintToString(messages());
    EXPECT_EQ(hatch().pattern, "ANSI31");
    EXPECT_EQ(hatch().islands, core::IslandStyle::Outer);
    EXPECT_EQ(hatch().seeds, (std::vector<core::Vec2>{{30, 20}}));
    EXPECT_EQ(hatch().loops.size(), 1U);
    EXPECT_EQ(editor().undoName(), "Hatch");
}

TEST_F(HatchTest, RemembersNetPropertiesAndReducesAngle) {
    document().add(rectangle());
    type({"HATCH", "P", "NET", "2", "390", "30,20", ""});

    ASSERT_EQ(document().entities().size(), 2U);
    EXPECT_EQ(hatch().pattern, "NET");
    EXPECT_DOUBLE_EQ(hatch().scale, 2.0);
    EXPECT_DOUBLE_EQ(hatch().angle, core::reducedDegreesToRadians(390.0));
    type({"HATCH", "P", "", "", "", "30,20", ""});
    EXPECT_EQ(hatch().pattern, "NET");
    EXPECT_DOUBLE_EQ(hatch().scale, 2.0);
    EXPECT_DOUBLE_EQ(hatch().angle, core::reducedDegreesToRadians(30.0));
}

TEST_F(HatchTest, IslandVariableAcceptsOnlyZeroThroughTwoAndSolidSkipsDensity) {
    document().add(rectangle());
    type({"HPISLANDDETECTION", "3", "2", "-HATCH", "P", "S", "30,20", ""});

    ASSERT_EQ(document().entities().size(), 2U) << testing::PrintToString(messages());
    EXPECT_TRUE(hatch().solid);
    EXPECT_EQ(hatch().pattern, "SOLID");
    EXPECT_EQ(hatch().islands, core::IslandStyle::Ignore);
    EXPECT_TRUE(hatch().patternLines.empty());
    EXPECT_NE(std::ranges::find(messages(), "Requires an integer between 0 and 2."),
              messages().end());
}

TEST_F(HatchTest, PointFindsOuterBoundaryAndCircleIsland) {
    document().add(rectangle());
    document().add(core::Circle{{30, 20}, 5});

    type({"-HATCH", "10,10", ""});

    ASSERT_EQ(hatch().loops.size(), 2U);
    EXPECT_EQ(hatch().islands, core::IslandStyle::Outer);
    EXPECT_EQ(hatch().seeds, (std::vector<core::Vec2>{{10, 10}}));
}

TEST_F(HatchTest, PointInsideIslandAfterOuterAreaIncludesCircleTwice) {
    document().add(rectangle());
    document().add(core::Circle{{30, 20}, 5});

    type({"-HATCH", "10,10", "30,20", ""});

    EXPECT_EQ(hatch().loops.size(), 3U);
    EXPECT_EQ(hatch().seeds, (std::vector<core::Vec2>{{10, 10}, {30, 20}}));
}

TEST_F(HatchTest, PointOutsideEveryBoundaryCreatesNothing) {
    document().add(rectangle());

    type({"-HATCH", "100,100", ""});

    EXPECT_EQ(document().entities().size(), 1U);
    EXPECT_NE(std::ranges::find(messages(), "Valid hatch boundary not found."), messages().end());
}

TEST_F(HatchTest, SecondObjectSelectionReplacesTheFirst) {
    document().add(rectangle());
    const core::EntityId second = document().add(rectangle({100, 0}, {120, 20}));
    const std::array selected{second};
    const std::vector<core::HatchLoop> expected =
        core::chosenHatchContours(document(), selected).loops;

    type({"-HATCH", "S"});
    runner().click({0, 10}, PickAction::Add);
    type({"", "S"});
    runner().click({100, 10}, PickAction::Add);
    type({"", ""});

    EXPECT_EQ(hatch().loops, expected);
}

TEST_F(HatchTest, UnknownPatternRepeatsThePrompt) {
    document().add(rectangle());

    type({"-HATCH", "P", "MISSING"});

    EXPECT_EQ(messages().back(), "Unknown pattern name.");
    EXPECT_EQ(runner().promptText(), "Enter a pattern name or [Solid] <ANSI31>:");
}

TEST_F(HatchTest, IslandVariableSupportsNormalAndIgnore) {
    document().add(rectangle());
    type({"HPISLANDDETECTION", "0", "-HATCH", "30,20", ""});
    EXPECT_EQ(hatch().islands, core::IslandStyle::Normal);
    type({"HPISLANDDETECTION", "2", "-HATCH", "30,20", ""});
    EXPECT_EQ(hatch().islands, core::IslandStyle::Ignore);
}

TEST_F(HatchTest, TinyPatternIsRejectedAsTooDense) {
    document().add(rectangle());

    type({"-HATCH", "P", "NET", "0.000001", "0", "30,20", ""});

    EXPECT_EQ(document().entities().size(), 1U);
    EXPECT_NE(std::ranges::find(messages(), "Hatch spacing too dense, or dash size too small."),
              messages().end());
}

TEST_F(HatchTest, EnterWithoutBoundariesCreatesNoUndoStep) {
    type({"-HATCH", ""});

    EXPECT_TRUE(document().isEmpty());
    EXPECT_FALSE(editor().canUndo());
}

TEST_F(HatchTest, UndoRemovesTheHatch) {
    document().add(rectangle());
    type({"-HATCH", "30,20", "", "U"});

    ASSERT_EQ(document().entities().size(), 1U);
    EXPECT_TRUE(std::holds_alternative<core::Polyline>(document().entities().front().entity));
}

TEST_F(HatchTest, MeasurementWithDxfPaddingUsesTheImperialPattern) {
    document().add(rectangle());
    core::RawDrawing raw;
    raw.sections = {{"HEADER", {{9, "  $MEASUREMENT  "}, {70, "     0"}}}};
    document().setRaw(std::move(raw));

    type({"-HATCH", "30,20", ""});

    EXPECT_EQ(hatch().patternLines, core::predefinedPatternLines("ANSI31", 1.0, 0.0, 0));
}

TEST_F(HatchTest, FullTurnsProduceTheSameLinesAndPositiveZeroAngle) {
    document().add(rectangle({-10, 0}, {0, 10}));
    std::vector<std::vector<core::Line>> lines;

    for (const std::string_view angle : {"0", "3600", "-3600"}) {
        type({"-HATCH", "P", "NET", "1"});
        runner().submitText(angle);
        type({"-5,5", ""});
        EXPECT_EQ(hatch().angle, 0.0);
        EXPECT_FALSE(std::signbit(hatch().angle));
        lines.push_back(core::patternLines(hatch()).lines);
    }

    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0], lines[1]);
    EXPECT_EQ(lines[0], lines[2]);
    EXPECT_NE(std::ranges::find(lines[0], core::Line{{0, 0}, {0, 10}}), lines[0].end());
}

}  // namespace
}  // namespace tg::cmd
