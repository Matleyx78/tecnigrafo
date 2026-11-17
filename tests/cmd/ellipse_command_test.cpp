// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/ellipse_command.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

class EllipseCommandTest : public testing::Test {
protected:
    EllipseCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string message) { m_messages.push_back(std::move(message)); });
    }

    void inputs(std::initializer_list<const char*> lines) {
        for (const char* line : lines) {
            m_runner.submitText(line);
        }
    }

    [[nodiscard]] const core::Ellipse& ellipse() const {
        return std::get<core::Ellipse>(m_document.entities().back().entity);
    }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(EllipseCommandTest, AxisEndpointsAndDistance) {
    inputs({"ELLIPSE", "0,0", "40,0", "10"});
    EXPECT_EQ(ellipse(), (core::Ellipse{{20, 0}, {-20, 0}, 0.5, 0.0, core::kTwoPi}));
    EXPECT_EQ(editor().undoName(), "Ellipse");
    inputs({"U"});
    EXPECT_TRUE(document().isEmpty());
}

TEST_F(EllipseCommandTest, CenterAndShortSpecifiedAxis) {
    inputs({"ELLIPSE", "Center", "10,-145", "25,-137", "6"});
    EXPECT_EQ(ellipse().center, (core::Vec2{10, -145}));
    EXPECT_EQ(ellipse().majorAxis, (core::Vec2{15, 8}));
    EXPECT_NEAR(ellipse().ratio, 6.0 / 17.0, 1e-15);

    inputs({"ELLIPSE", "C", "0,0", "0,5", "10"});
    EXPECT_EQ(ellipse().majorAxis, (core::Vec2{-10, 0}));
    EXPECT_EQ(ellipse().ratio, 0.5);
}

TEST_F(EllipseCommandTest, RotationUsesCosineOfAngle) {
    inputs({"ELLIPSE", "200,0", "240,0", "Rotation", "60"});
    EXPECT_EQ(ellipse().center, (core::Vec2{220, 0}));
    EXPECT_EQ(ellipse().majorAxis, (core::Vec2{-20, 0}));
    EXPECT_NEAR(ellipse().ratio, 0.5, 1e-15);
}

TEST_F(EllipseCommandTest, ArcAnglesAndIncludedAngle) {
    inputs({"ELLIPSE", "Arc", "0,-60", "40,-60", "10", "30", "200"});
    EXPECT_NEAR(ellipse().startParameter, core::degreesToRadians(49.10660535086909), 1e-14);
    EXPECT_NEAR(ellipse().endParameter, core::degreesToRadians(216.05238873238784), 1e-14);

    inputs({"ELLIPSE", "A", "C", "100,-60", "120,-60", "6", "-45", "I", "120"});
    EXPECT_NEAR(ellipse().startParameter, core::degreesToRadians(286.6992442339936), 1e-14);
    EXPECT_NEAR(ellipse().endParameter, core::degreesToRadians(445.40417460781634), 1e-14);
}

TEST_F(EllipseCommandTest, CancelAtEveryStageRollsBack) {
    const std::vector<std::vector<const char*>> prefixes{
        {"ELLIPSE"},
        {"ELLIPSE", "0,0"},
        {"ELLIPSE", "0,0", "40,0"},
        {"ELLIPSE", "C"},
        {"ELLIPSE", "C", "0,0"},
        {"ELLIPSE", "A", "0,0", "40,0", "10"},
        {"ELLIPSE", "A", "0,0", "40,0", "10", "30"}};
    for (const auto& prefix : prefixes) {
        for (const char* input : prefix) {
            runner().submitText(input);
        }
        runner().submit(CancelInput{});
        EXPECT_TRUE(document().isEmpty());
        EXPECT_FALSE(editor().inTransaction());
    }
}

TEST(EllipseCommandPreview, PreviewsAxisAndDistance) {
    core::Document document;
    edit::Editor editor(document);
    edit::DocumentFile file;
    CommandContext context{{[](const std::string&) {
                           }},
                           editor,
                           file,
                           {}};
    EllipseCommand command;
    command.start(context);
    command.handle(context, PointInput{{0, 0}});
    command.pointerMoved({40, 0});
    ASSERT_EQ(command.preview().size(), 1U);
    command.handle(context, PointInput{{40, 0}});
    command.pointerMoved({20, 10});
    ASSERT_EQ(command.preview().size(), 1U);
    EXPECT_EQ(std::get<core::Ellipse>(command.preview().front()).ratio, 0.5);
}

}  // namespace
}  // namespace tg::cmd
