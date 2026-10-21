// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/text_command.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <numbers>

namespace tg::cmd {
namespace {

TEST(TextCommandTest, LiteralLinesDefaultsAndSingleUndoStep) {
    core::Document document;
    edit::Editor editor(document);
    CommandRegistry registry;
    registerBuiltinCommands(registry);
    CommandRunner runner(registry, editor);
    for (const char* input : {"DT", "10,20", "3", "90", "  è %%c  ", "10,20", "Close", ""}) {
        runner.submitText(input);
    }
    ASSERT_EQ(document.entityCount(), 3U);
    const auto& first = std::get<core::Text>(document.entities()[0].entity);
    EXPECT_EQ(first.value, "  è %%c  ");
    EXPECT_EQ(first.position, (core::Vec2{10, 20}));
    EXPECT_DOUBLE_EQ(first.height, 3);
    EXPECT_DOUBLE_EQ(first.rotation, std::numbers::pi / 2);
    const auto& second = std::get<core::Text>(document.entities()[1].entity);
    EXPECT_EQ(second.value, "10,20");
    EXPECT_NEAR(second.position.x, 15, 1e-12);
    EXPECT_NEAR(second.position.y, 20, 1e-12);
    EXPECT_EQ(std::get<core::Text>(document.entities()[2].entity).value, "Close");
    EXPECT_EQ(editor.undoName(), "Text");
    runner.submitText("U");
    EXPECT_TRUE(document.isEmpty());
    runner.submitText("REDO");
    EXPECT_EQ(document.entityCount(), 3U);
    runner.submitText("DTEXT");
    runner.submitText("0,0");
    EXPECT_EQ(runner.promptText(), "Specify height <3.0000>:");
    runner.submitText("");
    EXPECT_EQ(runner.promptText(), "Specify rotation angle of text <90>:");
    runner.submit(CancelInput{});
}

TEST(TextCommandTest, EscapeAtEveryStageAndInvalidHeightsPreserveRedo) {
    core::Document document;
    edit::Editor editor(document);
    CommandContext context{editor, {}, [](const std::string&) {
                           }};
    TextCommand command;
    for (int stage = 0; stage < 4; ++stage) {
        command.start(context);
        if (stage > 0) {
            command.handle(context, PointInput{{0, 0}});
        }
        if (stage > 1) {
            for (const double height : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                                        std::numeric_limits<double>::quiet_NaN()}) {
                command.handle(context, ValueInput{height});
                EXPECT_EQ(command.prompt().kind, PromptKind::Distance);
            }
            command.handle(context, ConfirmInput{});
        }
        if (stage > 2) {
            command.handle(context, ConfirmInput{});
        }
        command.cancel(context);
        EXPECT_TRUE(document.isEmpty());
        EXPECT_FALSE(editor.inTransaction());
        EXPECT_FALSE(editor.canUndo());
        EXPECT_TRUE(command.preview().empty());
    }
    command.start(context);
    command.handle(context, PointInput{{0, 0}});
    command.handle(context, ConfirmInput{});
    command.handle(context, ConfirmInput{});
    command.handle(context, TextInput{"One"});
    command.handle(context, TextInput{"Two"});
    command.cancel(context);
    EXPECT_EQ(document.entityCount(), 2U);
    editor.undo();
    EXPECT_TRUE(document.isEmpty());
    command.start(context);
    command.cancel(context);
    EXPECT_TRUE(editor.canRedo());
}

TEST(TextCommandTest, AbandonedSetupKeepsDefaultsAndFirstLineUpdatesThem) {
    core::Document document;
    edit::Editor editor(document);
    CommandContext context{editor, {}, [](const std::string&) {
                           }};
    TextCommand command;
    for (const bool escape : {false, true}) {
        command.start(context);
        command.handle(context, PointInput{{0, 0}});
        command.handle(context, ValueInput{7});
        command.handle(context, ValueInput{std::numbers::pi / 4});
        EXPECT_DOUBLE_EQ(context.defaults.textHeight, 2.5);
        EXPECT_DOUBLE_EQ(context.defaults.textRotation, 0);
        if (escape) {
            command.cancel(context);
        } else {
            EXPECT_EQ(command.handle(context, ConfirmInput{}), Status::Finished);
        }
        EXPECT_TRUE(document.isEmpty());
        EXPECT_FALSE(editor.canUndo());
    }
    command.start(context);
    command.handle(context, PointInput{{0, 0}});
    command.handle(context, ValueInput{7});
    command.handle(context, ValueInput{std::numbers::pi / 4});
    command.handle(context, TextInput{"Confirmed"});
    EXPECT_DOUBLE_EQ(context.defaults.textHeight, 7);
    EXPECT_DOUBLE_EQ(context.defaults.textRotation, std::numbers::pi / 4);
    command.cancel(context);
    editor.undo();
    // Undo affects the drawing; the session still remembers the created text.
    EXPECT_DOUBLE_EQ(context.defaults.textHeight, 7);
    EXPECT_DOUBLE_EQ(context.defaults.textRotation, std::numbers::pi / 4);
}

TEST(TextCommandTest, PointMeasurementsAndPreviewOfNextBaseline) {
    core::Document document;
    edit::Editor editor(document);
    CommandContext context{editor, {}, [](const std::string&) {
                           }};
    TextCommand command;
    command.start(context);
    EXPECT_FALSE(command.basePoint());
    EXPECT_TRUE(command.preview().empty());
    command.handle(context, PointInput{{10, 20}});
    EXPECT_EQ(command.basePoint(), (core::Vec2{10, 20}));
    command.handle(context, PointInput{{13, 24}});
    EXPECT_DOUBLE_EQ(context.defaults.textHeight, 2.5);
    command.handle(context, PointInput{{10, 30}});
    EXPECT_DOUBLE_EQ(context.defaults.textRotation, 0);
    EXPECT_EQ(command.prompt().kind, PromptKind::Text);
    core::Text character;
    character.position = {10, 20};
    character.height = 5;
    character.rotation = std::numbers::pi / 2;
    character.value = "M";
    auto checkPreview = [&] {
        ASSERT_EQ(command.preview().size(), 1U);
        const auto& box = std::get<core::Polyline>(command.preview().front());
        EXPECT_TRUE(box.isClosed());
        ASSERT_EQ(box.vertexCount(), 4U);
        const auto corners = core::textBox(character);
        for (std::size_t i = 0; i < corners.size(); ++i) {
            EXPECT_NEAR(box.vertices()[i].position.x, corners.at(i).x, 1e-12);
            EXPECT_NEAR(box.vertices()[i].position.y, corners.at(i).y, 1e-12);
        }
    };
    checkPreview();
    command.pointerMoved({999, 999});
    checkPreview();
    command.handle(context, TextInput{"Hello"});
    EXPECT_DOUBLE_EQ(context.defaults.textHeight, 5);
    EXPECT_NEAR(context.defaults.textRotation, std::numbers::pi / 2, 1e-12);
    character.position.x += 25.0 / 3.0;
    checkPreview();
    EXPECT_EQ(document.entityCount(), 1U);
    command.cancel(context);
    EXPECT_TRUE(command.preview().empty());
}

}  // namespace
}  // namespace tg::cmd
