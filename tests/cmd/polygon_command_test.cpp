// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

class PolygonCommandTest : public testing::Test {
protected:
    PolygonCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string message) { m_messages.push_back(std::move(message)); });
    }

    void text(const char* value) { m_runner.submitText(value); }

    [[nodiscard]] const core::Polyline& polygon() const {
        return std::get<core::Polyline>(m_document.entities().back().entity);
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(PolygonCommandTest, RegistersNameAndAliasAndRejectsInvalidSides) {
    for (const char* name : {"POLYGON", "POL"}) {
        text(name);
        EXPECT_EQ(runner().promptText(), "Enter number of sides <4>:");
        text("2");
        EXPECT_EQ(messages().back(), "Requires an integer between 3 and 1024.");
        text("1025");
        EXPECT_EQ(messages().back(), "Requires an integer between 3 and 1024.");
        runner().submit(CancelInput{});
    }
}

TEST_F(PolygonCommandTest, TypedRadiusUsesHorizontalBaseAndRemembersDefaults) {
    for (const char* answer : {"I", "C"}) {
        text("POLYGON");
        text("4");
        text("0,0");
        text(answer);
        text("10");
        ASSERT_TRUE(polygon().isClosed());
        ASSERT_EQ(polygon().vertexCount(), 4U);
    }
    EXPECT_NEAR(polygon().vertices()[0].position.x, -10.0, 1e-12);
    EXPECT_NEAR(polygon().vertices()[0].position.y, 10.0, 1e-12);
    EXPECT_EQ(editor().undoName(), "Polygon");
    text("POLYGON");
    EXPECT_EQ(runner().promptText(), "Enter number of sides <4>:");
    text("");
    text("20,0");
    EXPECT_EQ(runner().promptText(),
              "Enter an option [Inscribed in circle/Circumscribed about circle] <C>:");
}

TEST_F(PolygonCommandTest, PointRadiusStartsAtPointWithExactCardinalDirections) {
    text("POLYGON");
    text("4");
    text("0,0");
    text("I");
    text("10,0");
    ASSERT_EQ(polygon().vertexCount(), 4U);
    EXPECT_EQ(polygon().vertices()[0].position, (core::Vec2{10, 0}));
    EXPECT_EQ(polygon().vertices()[1].position, (core::Vec2{0, 10}));
    EXPECT_EQ(polygon().vertices()[2].position, (core::Vec2{-10, 0}));
    EXPECT_EQ(polygon().vertices()[3].position, (core::Vec2{0, -10}));
}

TEST_F(PolygonCommandTest, EdgeBuildsCounterclockwisePolygonToItsLeft) {
    text("POLYGON");
    text("5");
    text("E");
    text("100,0");
    text("120,0");
    ASSERT_EQ(polygon().vertexCount(), 5U);
    EXPECT_EQ(polygon().vertices()[0].position, (core::Vec2{100, 0}));
    EXPECT_EQ(polygon().vertices()[1].position, (core::Vec2{120, 0}));
    EXPECT_GT(polygon().vertices()[2].position.y, 0.0);
}

}  // namespace
}  // namespace tg::cmd
