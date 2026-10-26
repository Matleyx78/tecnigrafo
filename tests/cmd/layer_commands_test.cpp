// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/layer_commands.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/start_handles.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

class LayerCommandsTest : public ::testing::Test {
public:
    LayerCommandsTest()
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

    bool heard(const std::string& message) const {
        return std::ranges::find(messages, message) != messages.end();
    }

    const core::Layer& layer(const char* name) const { return *document.findLayer(name); }

    core::Document document;
    edit::Editor editor;
    CommandRegistry registry;
    CommandRunner runner;
    std::vector<std::string> messages;
};

TEST_F(LayerCommandsTest, MakeNewSetAndClayerPreserveEntityLayerAndSharedHandles) {
    inputs({"-la", "m", "Muri", "", "LINE", "0,0", "10,0", ""});
    ASSERT_NE(document.findLayer("muri"), nullptr);
    EXPECT_EQ(layer("Muri").id.value, core::start_handles::kFirstEntity);
    const auto* entity = document.find(core::EntityId{core::start_handles::kFirstEntity + 1});
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->properties.layer, layer("Muri").id);
    inputs({"-LAYER", "New", " Porte , Finestre ", "Set", "porte", ""});
    EXPECT_EQ(document.current().layer, layer("Porte").id);
    inputs({"CLAYER", "0"});
    EXPECT_EQ(document.current().layer, document.layerZero());
    EXPECT_EQ(editor.undoName(), "CLAYER");
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.current().layer, layer("Porte").id);
    ASSERT_TRUE(editor.redo());
    EXPECT_EQ(document.current().layer, document.layerZero());
}

TEST_F(LayerCommandsTest, ChangesEveryPropertyUsingListsAndWildcards) {
    inputs({"-LAYER", "New", "Muri,Porte,Finestre", "Color", "red", "M*", "Color"});
    inputs({"Truecolor", "255,128,0", "P?rt?", "Ltype", "continuous", "*", "LWeight"});
    inputs({"0.50", "Muri,Porte", "Plot", "No plot", "Finestre", "LOck", "M*", "Freeze"});
    inputs({"P*", "OFF", "F*", ""});
    EXPECT_EQ(layer("Muri").color, core::Color::fromIndex(1));
    EXPECT_EQ(layer("Porte").color, core::Color::fromRgb((core::Rgb{255, 128, 0})));
    EXPECT_EQ(layer("Muri").lineweight, 50);
    EXPECT_EQ(layer("Porte").linetype, document.continuousLinetype());
    EXPECT_TRUE(layer("Muri").locked);
    EXPECT_TRUE(layer("Porte").frozen);
    EXPECT_FALSE(layer("Finestre").on);
    EXPECT_FALSE(layer("Finestre").plot);
    inputs({"-LAYER", "Thaw", "P*", "Unlock", "M*", "ON", "*", "Plot", "", "*", "LWeight"});
    inputs({"Default", "Muri", ""});
    EXPECT_FALSE(layer("Porte").frozen);
    EXPECT_FALSE(layer("Muri").locked);
    EXPECT_TRUE(layer("Finestre").on);
    EXPECT_TRUE(layer("Finestre").plot);
    EXPECT_EQ(layer("Muri").lineweight, core::lineweight::kDefault);
}

TEST_F(LayerCommandsTest, InvalidNamesStayAtQuestionAndDuplicateNamesReturnToOptions) {
    inputs({"-LAYER", "New", "Good,A<B"});
    EXPECT_TRUE(heard("Invalid layer name."));
    EXPECT_EQ(document.findLayer("Good"), nullptr);
    EXPECT_EQ(runner.promptText(), "Enter name list for new layer(s):");
    inputs({"Good", "New", "good", "Rename", "Good", "Bad*"});
    EXPECT_TRUE(heard("Layer \"good\" already exists."));
    EXPECT_EQ(runner.promptText(), "Enter new layer name:");
    inputs({"0"});
    EXPECT_TRUE(heard("Layer \"0\" already exists."));
    inputs({"Rename", "0", "Set", "Ghost", "Ltype", "Ghost", "OFF", "Z*", ""});
    EXPECT_TRUE(heard("Cannot rename layer \"0\"."));
    EXPECT_TRUE(heard("Layer \"Ghost\" not found."));
    EXPECT_TRUE(heard("Linetype \"Ghost\" is not loaded."));
    EXPECT_TRUE(heard("No matching layer names found."));
}

TEST_F(LayerCommandsTest, CurrentCannotBeFrozenAndOffNeedsExplicitConfirmation) {
    inputs({"-LAYER", "New", "Frozen", "Freeze", "Frozen", "Set", "Frozen", "Make"});
    inputs({"Frozen", "Freeze", "0", "OFF", "0"});
    EXPECT_TRUE(heard("Cannot set current to a frozen layer."));
    EXPECT_TRUE(heard("Cannot freeze layer \"0\".  It is the CURRENT layer."));
    EXPECT_EQ(runner.promptText(), "Really want layer 0 (the CURRENT layer) off? [Yes/No] <No>:");
    inputs({"", "OFF", "0", "No"});
    EXPECT_TRUE(layer("0").on);
    inputs({"OFF", "0", "Yes", ""});
    EXPECT_FALSE(layer("0").on);
    EXPECT_FALSE(layer("0").frozen);
}

TEST_F(LayerCommandsTest, ClayerErrorsKeepQuestionAndEnterDoesNotAddUndoStep) {
    inputs({"-LAYER", "New", "Frozen", "Freeze", "Frozen", "", "CLAYER", "Ghost"});
    EXPECT_TRUE(heard("Layer \"Ghost\" not found."));
    EXPECT_EQ(runner.promptText(), "Enter new value for CLAYER <\"0\">:");
    inputs({"Frozen"});
    EXPECT_TRUE(heard("Cannot set current to a frozen layer."));
    EXPECT_TRUE(runner.isActive());
    inputs({""});
    EXPECT_FALSE(runner.isActive());
    EXPECT_EQ(editor.undoNames().size(), 1U);
}

TEST_F(LayerCommandsTest, AllOptionsShareOneUndoStepAndEscKeepsCompletedWork) {
    inputs({"-LAYER", "New", "Muri", "Color", "1", "Muri", "LWeight", "0.50", "Muri", ""});
    EXPECT_EQ(editor.undoNames(), (std::vector<std::string>{"Layer"}));
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.findLayer("Muri"), nullptr);
    ASSERT_TRUE(editor.redo());
    EXPECT_EQ(layer("Muri").color, core::Color::fromIndex(1));
    EXPECT_EQ(layer("Muri").lineweight, 50);
    inputs({"-LAYER", "New", "Porte", "Rename", "Porte"});
    runner.submit(CancelInput{});
    EXPECT_NE(document.findLayer("Porte"), nullptr);
    EXPECT_EQ(editor.undoNames().size(), 2U);
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.findLayer("Porte"), nullptr);
}

TEST_F(LayerCommandsTest, RenameWarnsAndPreservesOtherUnhandledData) {
    // The warning concerns other raw data; entity group 8 is updated by Editor.
    core::RawDrawing raw;
    raw.sections.push_back({"HEADER", {{1, "Muri"}}});
    document.setRaw(std::move(raw));
    inputs({"-LAYER", "New", "Muri", "Rename", "Muri", "Pareti", ""});
    EXPECT_NE(document.findLayer("Pareti"), nullptr);
    EXPECT_EQ(document.findLayer("Muri"), nullptr);
    EXPECT_TRUE(heard("Layer \"Muri\" is also named in data Tecnigrafo does not handle; that data "
                      "keeps the old name."));
    EXPECT_EQ(document.raw().sections.back().groups.front().value, "Muri");
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.findLayer("Pareti"), nullptr);
}

TEST_F(LayerCommandsTest, ListsMatchingLayersAndCurrentLayerWithoutChangingHistory) {
    inputs({"-LAYER", "New", "Muri,Porte", "", "-LAYER", "?", "m*,Muri", ""});
    EXPECT_TRUE(heard("Muri  on thawed unlocked 7 \"Continuous\" default"));
    EXPECT_TRUE(heard("Current layer: \"0\""));
    EXPECT_EQ(editor.undoNames().size(), 1U);
    inputs({"-LAYER", "?", "", ""});
    EXPECT_TRUE(heard("0  on thawed unlocked 7 \"Continuous\" default"));
    EXPECT_TRUE(heard("Porte  on thawed unlocked 7 \"Continuous\" default"));
}

TEST_F(LayerCommandsTest, RejectsInvalidPropertyValuesAndSpecialLinetypes) {
    inputs({"-LAYER", "Color", "ByLayer", "256", "0", "T", "256,0,0", "0,0,0", "0"});
    inputs({"LWeight", "ByBlock", "0.12", "0.13", "0", "Ltype", "ByLayer", "Ltype"});
    inputs({"ByBlock", ""});
    EXPECT_TRUE(heard("Requires a color index from 1 to 255."));
    EXPECT_TRUE(heard("Requires red, green, blue values from 0 to 255."));
    EXPECT_TRUE(heard("Requires a valid lineweight."));
    EXPECT_TRUE(heard("Linetype \"ByLayer\" is not loaded."));
    EXPECT_TRUE(heard("Linetype \"ByBlock\" is not loaded."));
    EXPECT_EQ(layer("0").color, core::Color::fromRgb((core::Rgb{0, 0, 0})));
    EXPECT_EQ(layer("0").lineweight, 13);
}

}  // namespace
}  // namespace tg::cmd
