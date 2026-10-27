// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/property_commands.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

class PropertyCommandsTest : public ::testing::Test {
public:
    PropertyCommandsTest()
        : document(core::Document::newDrawing()), editor(document), runner(registry, editor) {
        registerBuiltinCommands(registry);
        runner.setMessageSink(
            [this](std::string message) { messages.push_back(std::move(message)); });
    }

    void inputs(std::initializer_list<const char*> lines) {
        for (const char* line : lines) {
            runner.submitText(line);
        }
    }

    bool heard(const std::string& text) const {
        return std::ranges::find(messages, text) != messages.end();
    }

    core::Document document;
    edit::Editor editor;
    CommandRegistry registry;
    CommandRunner runner;
    std::vector<std::string> messages;
};

TEST_F(PropertyCommandsTest, ColorSpellingsAndAliases) {
    const std::vector<std::string> names{"red",  "yellow",  "green", "cyan",
                                         "blue", "magenta", "white"};
    int index = 1;
    for (const auto& name : names) {
        runner.submitText("COL");
        runner.submitText(name);
        EXPECT_EQ(document.current().color, core::Color::fromIndex(index));
        ++index;
    }
    inputs({"COLOUR", "0"});
    EXPECT_EQ(document.current().color, core::Color::byBlock());
    EXPECT_EQ(runner.promptText(), "Command:");
    inputs({"-COLOR", "256"});
    EXPECT_EQ(document.current().color, core::Color::byLayer());
    inputs({"COLOR", "255"});
    EXPECT_EQ(document.current().color, core::Color::fromIndex(255));
    inputs({"COLOR", "byblock", "COLOR", "BYLAYER", "COLOR", "0,128,255"});
    EXPECT_EQ(document.current().color, core::Color::fromRgb((core::Rgb{0, 128, 255})));
}

TEST_F(PropertyCommandsTest, InvalidRgbKeepsItsQuestionAndEscDoesNotChangeColor) {
    inputs({"COLOR", "300"});
    EXPECT_TRUE(heard("Invalid color."));
    EXPECT_EQ(runner.promptText(), "Enter default object color [Truecolor] <BYLAYER>:");
    inputs({"T", "256,0,0", "7"});
    EXPECT_EQ(runner.promptText(), "Red, Green, Blue:");
    EXPECT_EQ(document.current().color, core::Color::byLayer());
    runner.submit(CancelInput{});
    EXPECT_TRUE(editor.undoNames().empty());
    inputs({"COLOR", "Truecolor", "0,0,0"});
    EXPECT_EQ(document.current().color, core::Color::fromRgb((core::Rgb{0, 0, 0})));
}

TEST_F(PropertyCommandsTest, LoadSetListAndErrorsShareOneUndoStep) {
    inputs({"LT", "Load", "acad_iso02w100,ACAD_ISO10W100", "Set", "acad_iso02w100", "?"});
    EXPECT_TRUE(heard("\"ACAD_ISO02W100\" Tratto ISO __ __ __ __ __ __ __ __ __ __ __ __ "));
    inputs({"Load", "ACAD_ISO02W100", "Load", "Unknown", "Set", "Unknown"});
    EXPECT_TRUE(heard("Linetype \"ACAD_ISO02W100\" is already loaded."));
    EXPECT_TRUE(heard("Linetype \"Unknown\" not found."));
    EXPECT_TRUE(heard("Linetype \"Unknown\" is not loaded."));
    inputs({"Set", "?"});
    EXPECT_EQ(runner.promptText(), "Specify linetype name or [?] <ACAD_ISO02W100>:");
    inputs({"ByBlock", ""});
    EXPECT_EQ(document.current().linetype, document.byBlockLinetype());
    EXPECT_EQ(editor.undoNames(), (std::vector<std::string>{"LINETYPE"}));
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.findLinetype("ACAD_ISO02W100"), nullptr);
    EXPECT_EQ(document.current().linetype, document.byLayerLinetype());
    ASSERT_TRUE(editor.redo());
    EXPECT_NE(document.findLinetype("ACAD_ISO02W100"), nullptr);
    EXPECT_EQ(document.current().linetype, document.byBlockLinetype());
}

TEST_F(PropertyCommandsTest, LinetypeEscKeepsCompletedLoadsAndOpenedDefinitions) {
    inputs({"-LT", "Load", "ACAD_ISO02W100", "Set"});
    runner.submit(CancelInput{});
    ASSERT_NE(document.findLinetype("ACAD_ISO02W100"), nullptr);
    EXPECT_EQ(editor.undoName(), "LINETYPE");
    inputs({"LTYPE", "Set", "ByLayer", "", "LINETYPE", "", "-LINETYPE", "Load", "", ""});
    EXPECT_EQ(editor.undoNames().size(), 1U);
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.findLinetype("ACAD_ISO02W100"), nullptr);
}

TEST_F(PropertyCommandsTest, PositiveScalesRejectInvalidInputAndDoNotRecordUnchangedValues) {
    inputs({"LTS", "-1", "0", "nan", "inf", "1,2", "2"});
    EXPECT_TRUE(heard("Requires a positive number."));
    EXPECT_DOUBLE_EQ(document.ltscale(), 2.0);
    inputs({"CELTSCALE", "0", "1e-2"});
    EXPECT_DOUBLE_EQ(document.current().linetypeScale, 0.01);
    inputs({"LTSCALE", "2", "CELTSCALE", "0.01"});
    EXPECT_EQ(editor.undoNames(), (std::vector<std::string>{"LTSCALE", "CELTSCALE"}));
    ASSERT_TRUE(editor.undo());
    EXPECT_DOUBLE_EQ(document.current().linetypeScale, 1.0);
    ASSERT_TRUE(editor.undo());
    EXPECT_DOUBLE_EQ(document.ltscale(), 1.0);
    ASSERT_TRUE(editor.redo());
    ASSERT_TRUE(editor.redo());
    EXPECT_DOUBLE_EQ(document.current().linetypeScale, 0.01);
}

TEST_F(PropertyCommandsTest, EveryAllowedLineweightAndSpecialSpelling) {
    for (const int weight : core::lineweight::kAllowed) {
        runner.submitText("LINEWEIGHT");
        runner.submitText(core::formatLineweight(weight) + "mm");
        EXPECT_EQ(document.current().lineweight, weight);
    }
    inputs({"LW", "ByBlock"});
    EXPECT_EQ(document.current().lineweight, core::lineweight::kByBlock);
    inputs({"-LWEIGHT", "Default"});
    EXPECT_EQ(document.current().lineweight, core::lineweight::kDefault);
    inputs({"LWEIGHT", "ByLayer", "LWEIGHT", "?", "0.45"});
    EXPECT_TRUE(heard("Invalid lineweight."));
    EXPECT_EQ(runner.promptText(), "Enter default lineweight for new objects or [?]:");
    EXPECT_EQ(document.current().lineweight, core::lineweight::kByLayer);
    inputs({"0.50"});
    EXPECT_EQ(document.current().lineweight, 50);
}

TEST_F(PropertyCommandsTest, EnterAndEscLeaveEverySingleValueUnchanged) {
    for (const char* name : {"COLOR", "LTSCALE", "CELTSCALE", "LWEIGHT", "LWDISPLAY"}) {
        runner.submitText(name);
        runner.submitText("");
        EXPECT_FALSE(runner.isActive());
        runner.submitText(name);
        runner.submit(CancelInput{});
        EXPECT_FALSE(runner.isActive());
    }
    inputs({"COLOR", "Truecolor", ""});
    EXPECT_FALSE(runner.isActive());
    EXPECT_TRUE(editor.undoNames().empty());
    EXPECT_FALSE(editor.inTransaction());
}

TEST_F(PropertyCommandsTest, NewEntitiesInheritCurrentValuesAndDisplayHasNoUndoStep) {
    inputs({"COLOR", "red", "CELTSCALE", "2", "LWEIGHT", "0.30"});
    inputs({"-LINETYPE", "Set", "Continuous", "", "LTSCALE", "3"});
    const auto undoBeforeDisplay = editor.undoNames();
    inputs({"LWDISPLAY", "invalid"});
    EXPECT_TRUE(heard("Requires ON or OFF."));
    EXPECT_EQ(runner.promptText(), "Enter new value for LWDISPLAY <OFF>:");
    inputs({"ON"});
    EXPECT_TRUE(document.lwdisplay());
    EXPECT_EQ(editor.undoNames(), undoBeforeDisplay);
    inputs({"LINE", "0,0", "10,0", ""});
    const auto* entity = document.find(core::EntityId{257});
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->properties, document.current());
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.find(core::EntityId{257}), nullptr);
    EXPECT_TRUE(document.lwdisplay());
    ASSERT_TRUE(editor.redo());
    ASSERT_NE(document.find(core::EntityId{257}), nullptr);
    EXPECT_EQ(document.find(core::EntityId{257})->properties, document.current());
    inputs({"LWDISPLAY", "off"});
    EXPECT_FALSE(document.lwdisplay());
}

}  // namespace
}  // namespace tg::cmd
