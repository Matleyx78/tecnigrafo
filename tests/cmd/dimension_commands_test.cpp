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

class DimensionCommandsTest : public testing::Test {
protected:
    DimensionCommandsTest()
        : m_document(core::Document::newDrawing()),
          m_editor(m_document),
          m_runner(m_registry, m_editor) {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string message) { m_messages.push_back(std::move(message)); });
        m_runner.setPixelSize(0.1);
    }

    void point(double x, double y) { m_runner.submit(PointInput{{x, y}}); }

    [[nodiscard]] const core::Dimension& dimension() const {
        return std::get<core::Dimension>(m_document.find(m_document.entities().back().id)->entity);
    }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

private:
    core::Document m_document;
    edit::Editor m_editor;
    CommandRegistry m_registry;
    CommandRunner m_runner;
    std::vector<std::string> m_messages;
};

TEST_F(DimensionCommandsTest, WrittenPointsCreateLinearAndAlignedDimensions) {
    runner().submitText("DIMLINEAR");
    point(0.0, 0.0);
    point(10.0, 10.0);
    point(5.0, 20.0);
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(dimension().definition.kind, core::DimensionKind::Linear);
    EXPECT_DOUBLE_EQ(dimension().definition.rotation, 0.0);
    EXPECT_EQ(messages().back(), "Dimension text = 10");
    EXPECT_EQ(editor().undoName(), "DIMLINEAR");
    runner().submitText("U");

    runner().submitText("DIMALIGNED");
    point(0.0, 0.0);
    point(3.0, 4.0);
    point(0.0, 5.0);
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(dimension().definition.kind, core::DimensionKind::Aligned);
    EXPECT_DOUBLE_EQ(dimension().value.measurement, 5.0);
}

TEST_F(DimensionCommandsTest, LinearAndAlignedMtextExposeTheirEditorRequests) {
    for (const char* command : {"DIMLINEAR", "DIMALIGNED"}) {
        runner().submitText(command);
        point(0.0, 0.0);
        point(30.0, 0.0);
        runner().pointerMoved({15.0, 10.0});
        runner().submitText("Mtext");
        const auto request = runner().textEditing();
        ASSERT_TRUE(request);
        EXPECT_FALSE(request->target);
        EXPECT_EQ(request->value, "<>");
        EXPECT_TRUE(request->measurement);
        EXPECT_TRUE(std::holds_alternative<core::MText>(request->shown));

        runner().submit(TextInput{"FIXED"});
        runner().pointerMoved({15.0, 10.0});
        runner().submitText("Mtext");
        const auto fixedRequest = runner().textEditing();
        ASSERT_TRUE(fixedRequest);
        EXPECT_EQ(fixedRequest->value, "FIXED");
        EXPECT_TRUE(fixedRequest->measurement);
        runner().submit(CancelInput{});
    }
}

TEST_F(DimensionCommandsTest, SelectObjectTextAndRotatedOptionsAreApplied) {
    editor().beginTransaction("Line");
    editor().add(core::Line{{0.0, 0.0}, {10.0, 0.0}});
    editor().commit();
    runner().submitText("DIMLINEAR");
    runner().submitText("");
    runner().click({5.0, 0.0}, PickAction::Add);
    EXPECT_EQ(runner().promptText(),
              "Specify dimension line location or [Mtext/Text/Angle/Horizontal/Vertical/Rotated]:");
    runner().submitText("Mtext");
    EXPECT_EQ(runner().promptText(), "Enter dimension text <10>:");
    runner().submitText("length <>");
    runner().submitText("Rotated");
    EXPECT_EQ(runner().promptText(), "Specify angle of dimension line <0>:");
    runner().submitText("30");
    EXPECT_EQ(runner().promptText(),
              "Specify dimension line location or [Mtext/Text/Angle/Horizontal/Vertical/Rotated]:");
    point(0.0, 5.0);
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(dimension().presentation.textOverride, "length <>");
    EXPECT_NEAR(dimension().definition.rotation, core::kPi / 6.0, 1e-12);
    EXPECT_EQ(messages().back(), "Dimension text = 8,66");
}

TEST_F(DimensionCommandsTest, AutomaticDirectionUsesHorizontalAndVerticalAxes) {
    runner().submitText("DIMLINEAR");
    point(0.0, 0.0);
    point(10.0, 10.0);
    point(5.0, 20.0);
    EXPECT_DOUBLE_EQ(dimension().definition.rotation, 0.0);

    runner().submitText("DIMLINEAR");
    point(0.0, 0.0);
    point(10.0, 10.0);
    point(20.0, 5.0);
    EXPECT_DOUBLE_EQ(dimension().definition.rotation, core::kPi / 2.0);
}

TEST_F(DimensionCommandsTest, ExplicitTextAngleKeepsTextCenteredOnDimensionLine) {
    runner().submitText("DIMLINEAR");
    point(0.0, 20.0);
    point(0.0, 50.0);
    runner().submitText("Angle");
    runner().submitText("90");
    point(10.0, 35.0);

    EXPECT_EQ(dimension().presentation.textPoint, (core::Vec2{10.0, 35.0}));
    EXPECT_EQ(dimension().presentation.textRotation, core::kPi / 2.0);
    EXPECT_EQ(dimension().graphics.size(), 7U);
    const core::EntityId first = document().entities().back().id;

    runner().submitText("DIMALIGNED");
    point(0.0, 0.0);
    point(3.0, 4.0);
    point(0.0, 5.0);
    EXPECT_EQ(document().entities().back().id.value, first.value + 8);
}

TEST_F(DimensionCommandsTest, CoincidentAlignedOriginsRepeatSecondOriginPrompt) {
    runner().submitText("DIMALIGNED");
    point(1.0, 2.0);
    point(1.0, 2.0);
    EXPECT_EQ(document().entityCount(), 0U);
    EXPECT_EQ(messages().back(), "Points coincide.");
    EXPECT_EQ(runner().promptText(), "Specify second extension line origin:");
}

}  // namespace
}  // namespace tg::cmd
