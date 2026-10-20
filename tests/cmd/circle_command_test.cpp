// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/circle_command.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

class CircleCommandTest : public testing::Test {
protected:
    CircleCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    [[nodiscard]] const core::Circle& circle() const {
        return std::get<core::Circle>(document().entities().back().entity);
    }

    void inputs(std::initializer_list<const char*> lines) {
        for (const char* line : lines) {
            runner().submitText(line);
        }
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(CircleCommandTest, RadiusDiameterAndRelativePoints) {
    inputs({"circle", "10,20", "5"});
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(circle(), (core::Circle{{10, 20}, 5}));
    inputs({"c", "30,40", "@0,10"});
    EXPECT_EQ(circle(), (core::Circle{{30, 40}, 10}));
    inputs({"C", "50,50", "d", "25"});
    EXPECT_EQ(circle(), (core::Circle{{50, 50}, 12.5}));
    inputs({"C", "100,100", "Diameter", "@20,0"});
    EXPECT_EQ(circle(), (core::Circle{{100, 100}, 10}));
    inputs({"L", "@0,0", "@1,0", ""});
    ASSERT_EQ(document().entityCount(), 5U);
    ASSERT_TRUE(std::holds_alternative<core::Line>(document().entities().back().entity));
    EXPECT_EQ(std::get<core::Line>(document().entities().back().entity).start,
              (core::Vec2{100, 100}));
}

TEST_F(CircleCommandTest, DefaultsAndUndoRedo) {
    inputs({"C", "0,0", ""});
    EXPECT_TRUE(document().isEmpty());
    EXPECT_FALSE(editor().canUndo());
    inputs({"C", "2P", "0,0", "6,8"});
    EXPECT_EQ(circle(), (core::Circle{{3, 4}, 5}));
    EXPECT_EQ(editor().undoName(), "Circle");
    inputs({"U"});
    EXPECT_TRUE(document().isEmpty());
    inputs({"REDO"});
    EXPECT_EQ(circle(), (core::Circle{{3, 4}, 5}));
    inputs({"C", "20,20"});
    EXPECT_EQ(runner().promptText(), "Specify radius of circle or [Diameter] <5.0000>:");
    inputs({"D"});
    EXPECT_EQ(runner().promptText(), "Specify diameter of circle <10.0000>:");
    inputs({""});
    EXPECT_EQ(circle(), (core::Circle{{20, 20}, 5}));
}

TEST_F(CircleCommandTest, InvalidSizesAndCoincidentDiameterStayAtPrompt) {
    inputs({"C", "0,0"});
    const std::string prompt = runner().promptText();
    for (double value : {0.0, -5.0, std::numeric_limits<double>::infinity()}) {
        runner().submit(ValueInput{value});
        EXPECT_EQ(messages().back(), "Value must be positive and nonzero.");
        EXPECT_EQ(runner().promptText(), prompt);
        EXPECT_TRUE(document().isEmpty());
    }
    inputs({"D", "0", "-5", "0,0", "10"});
    EXPECT_EQ(circle(), (core::Circle{{0, 0}, 5}));
    inputs({"C", "2P", "10,10", "10,10"});
    EXPECT_EQ(messages().back(), "Value must be positive and nonzero.");
    EXPECT_EQ(runner().promptText(), "Specify second end point of circle's diameter:");
    inputs({"@6,8"});
    EXPECT_EQ(circle(), (core::Circle{{13, 14}, 5}));
}

TEST_F(CircleCommandTest, ThreePointsBothOrientationsAndCollinearRetry) {
    for (const char* second : {"0,10", "0,-10"}) {
        inputs({"C", "3p", "10,0", second, "-10,0"});
        EXPECT_EQ(circle(), (core::Circle{{0, 0}, 10}));
    }
    inputs({"C", "3P", "0,0", "10,0"});
    for (const char* third : {"20,0", "5,9e-09", "10,0", "0,0"}) {
        inputs({third});
        EXPECT_EQ(messages().back(), "The three points are collinear.");
        EXPECT_EQ(runner().promptText(), "Specify third point on circle:");
        EXPECT_EQ(document().entityCount(), 2U);
    }
    inputs({"0,10"});
    EXPECT_EQ(circle().center, (core::Vec2{5, 5}));
    EXPECT_NEAR(circle().radius, std::sqrt(50.0), 1e-12);
}

TEST_F(CircleCommandTest, EscapeAtEveryPromptPreservesRedo) {
    inputs({"C", "0,0", "5", "U"});
    const std::vector<std::vector<const char*>> prefixes{
        {"C"},       {"C", "0,0"},       {"C", "0,0", "D"},         {"C", "2P"}, {"C", "2P", "0,0"},
        {"C", "3P"}, {"C", "3P", "0,0"}, {"C", "3P", "0,0", "10,0"}};
    for (const auto& prefix : prefixes) {
        for (const char* line : prefix) {
            runner().submitText(line);
        }
        runner().submit(CancelInput{});
        EXPECT_TRUE(document().isEmpty());
        EXPECT_FALSE(editor().inTransaction());
        EXPECT_FALSE(runner().isActive());
        EXPECT_TRUE(editor().canRedo());
    }
}

TEST(CircleCommandPreview, GeometryAndBasePointsFollowEachConstruction) {
    core::Document document;
    edit::Editor editor(document);
    CommandContext context{editor, {}, [](std::string) {
                           }};
    CircleCommand command;
    command.start(context);
    EXPECT_FALSE(command.basePoint());
    command.handle(context, PointInput{{10, 20}});
    EXPECT_EQ(command.basePoint(), (core::Vec2{10, 20}));
    command.pointerMoved({13, 24});
    ASSERT_EQ(command.preview().size(), 1U);
    EXPECT_EQ(std::get<core::Circle>(command.preview().front()), (core::Circle{{10, 20}, 5}));
    command.handle(context, KeywordInput{"Diameter"});
    EXPECT_EQ(std::get<core::Circle>(command.preview().front()), (core::Circle{{10, 20}, 2.5}));
    command.pointerMoved({10, 20});
    EXPECT_TRUE(command.preview().empty());
    command.cancel(context);
    command.start(context);
    command.handle(context, KeywordInput{"2P"});
    command.handle(context, PointInput{{0, 0}});
    command.pointerMoved({6, 8});
    ASSERT_EQ(command.preview().size(), 1U);
    EXPECT_EQ(std::get<core::Circle>(command.preview().front()), (core::Circle{{3, 4}, 5}));
    command.cancel(context);
    command.start(context);
    command.handle(context, KeywordInput{"3P"});
    command.handle(context, PointInput{{10, 0}});
    command.handle(context, PointInput{{0, 10}});
    EXPECT_EQ(command.basePoint(), (core::Vec2{0, 10}));
    command.pointerMoved({-10, 0});
    ASSERT_EQ(command.preview().size(), 1U);
    EXPECT_EQ(std::get<core::Circle>(command.preview().front()), (core::Circle{{0, 0}, 10}));
    command.pointerMoved({20, -10});
    EXPECT_TRUE(command.preview().empty());
    EXPECT_TRUE(document.isEmpty());
    command.cancel(context);
}

}  // namespace
}  // namespace tg::cmd
