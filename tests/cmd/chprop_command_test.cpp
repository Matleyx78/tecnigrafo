// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/chprop_command.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/effective_properties.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

constexpr std::string_view kPropertyPrompt =
    "Enter property to change [Color/LAyer/LType/ltScale/LWeight]:";

/// A new drawing with the linetype ACAD_ISO02W100 loaded, the layers "Muri" and
/// "Porte", and two lines on layer 0, both with the default properties:
///
///   second  (0,10)-(10,10)
///   first   (0,0)-(10,0)
class ChpropTest : public testing::Test {
protected:
    ChpropTest()
        : m_document(core::Document::newDrawing()),
          m_editor(m_document),
          m_runner(m_registry, m_editor) {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
        // As a user would: through the commands, so the tables are the real ones.
        type({"-LINETYPE", "Load", "ACAD_ISO02W100", "", "-LAYER", "New", "Muri,Porte", ""});
        m_first = addLine({0, 0}, {10, 0});
        m_second = addLine({0, 10}, {10, 10});
        m_messages.clear();
    }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const CommandRegistry& registry() const { return m_registry; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    [[nodiscard]] core::EntityId first() const { return m_first; }

    [[nodiscard]] core::EntityId second() const { return m_second; }

    /// Sends each line as typed text; "" is Enter.
    void type(std::initializer_list<const char*> lines) {
        for (const char* line : lines) {
            m_runner.submitText(line);
        }
    }

    [[nodiscard]] bool heard(const std::string& text) const {
        return std::ranges::find(m_messages, text) != m_messages.end();
    }

    void selectFirst() { m_runner.click({5, 0}, PickAction::Add); }

    void selectSecond() { m_runner.click({5, 10}, PickAction::Add); }

    [[nodiscard]] core::EntityProperties properties(core::EntityId id) const {
        return m_document.find(id)->properties;
    }

    [[nodiscard]] core::LayerId layerId(std::string_view name) const {
        return m_document.findLayer(name)->id;
    }

    [[nodiscard]] core::LinetypeId linetypeId(std::string_view name) const {
        return m_document.findLinetype(name)->id;
    }

    /// Gives `id` the properties `value`, in a step of its own.
    void seed(core::EntityId id, const core::EntityProperties& value) {
        m_editor.beginTransaction("Seed");
        ASSERT_TRUE(m_editor.setProperties(id, value));
        m_editor.commit();
    }

    /// Non-default values for every property, on layer "Muri".
    [[nodiscard]] core::EntityProperties other() const {
        core::EntityProperties value = properties(m_first);
        value.color = core::Color::fromRgb({255, 128, 0});
        value.layer = layerId("Muri");
        value.linetype = linetypeId("ACAD_ISO02W100");
        value.linetypeScale = 2.0;
        value.lineweight = 50;
        return value;
    }

private:
    core::EntityId addLine(core::Vec2 from, core::Vec2 to) {
        m_editor.beginTransaction("Add");
        const core::EntityId id = m_editor.add(core::Line{from, to});
        m_editor.commit();
        return id;
    }

    core::Document m_document;
    edit::Editor m_editor;
    CommandRegistry m_registry;
    CommandRunner m_runner;
    std::vector<std::string> m_messages;
    core::EntityId m_first;
    core::EntityId m_second;
};

TEST_F(ChpropTest, StartsAtOnceWithTheSelectionAndAsksForTheProperty) {
    selectFirst();
    selectSecond();

    type({"CHPROP"});

    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
    EXPECT_TRUE(runner().acceptsText());
    EXPECT_EQ(runner().prompt().keywords, (std::vector<Keyword>{{"Color", "C"},
                                                                {"LAyer", "LA"},
                                                                {"LType", "LT"},
                                                                {"ltScale", "LTS"},
                                                                {"LWeight", "LW"}}));
    EXPECT_TRUE(messages().empty());
}

TEST_F(ChpropTest, AsksForTheSelectionFirstWhenNothingIsSelected) {
    const std::size_t steps = editor().undoNames().size();

    type({"chprop"});
    EXPECT_EQ(runner().promptText(), "Select objects:");
    runner().click({5, 0}, PickAction::Add);
    runner().click({5, 10}, PickAction::Add);
    type({""});

    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
    EXPECT_EQ(messages().back(), "1 found, 2 total");
    type({"Color", "red", ""});
    EXPECT_EQ(properties(first()).color, core::Color::fromIndex(1));
    EXPECT_EQ(properties(second()).color, core::Color::fromIndex(1));
    EXPECT_EQ(editor().undoNames().size(), steps + 1);
}

TEST_F(ChpropTest, EveryPropertyChangesEverySelectedEntityInOneUndoStep) {
    const core::EntityProperties original = properties(first());
    const std::size_t steps = editor().undoNames().size();
    selectFirst();
    selectSecond();

    type({"CHPROP", "Color", "red", "LAyer", "Muri", "LType", "ACAD_ISO02W100", "ltScale", "2",
          "LWeight", "0.50"});
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
    EXPECT_TRUE(runner().isActive());
    type({""});

    EXPECT_FALSE(runner().isActive());
    for (const core::EntityId id : {first(), second()}) {
        const core::EntityProperties changed = properties(id);
        EXPECT_EQ(changed.color, core::Color::fromIndex(1));
        EXPECT_EQ(changed.layer, layerId("Muri"));
        EXPECT_EQ(changed.linetype, linetypeId("ACAD_ISO02W100"));
        EXPECT_DOUBLE_EQ(changed.linetypeScale, 2.0);
        EXPECT_EQ(changed.lineweight, 50);
    }
    ASSERT_EQ(editor().undoNames().size(), steps + 1);
    EXPECT_EQ(editor().undoName(), "CHPROP");

    ASSERT_TRUE(editor().undo());
    EXPECT_EQ(properties(first()), original);
    EXPECT_EQ(properties(second()), original);
    ASSERT_TRUE(editor().redo());
    EXPECT_EQ(properties(first()).layer, layerId("Muri"));
    EXPECT_EQ(properties(second()).lineweight, 50);
}

TEST_F(ChpropTest, OnlyTheSelectedEntitiesChange) {
    selectFirst();

    type({"CHPROP", "Color", "blue", ""});

    EXPECT_EQ(properties(first()).color, core::Color::fromIndex(5));
    EXPECT_EQ(properties(second()).color, core::Color::byLayer());
}

TEST_F(ChpropTest, QuestionsShowTheValueTheEntitiesShare) {
    selectFirst();
    selectSecond();

    type({"CHPROP", "Color"});
    EXPECT_EQ(runner().promptText(), "New color [Truecolor] <BYLAYER>:");
    type({"", "LAyer"});
    EXPECT_EQ(runner().promptText(), "Enter new layer name <0>:");
    type({"", "LType"});
    EXPECT_EQ(runner().promptText(), "Enter new linetype name <ByLayer>:");
    type({"", "ltScale"});
    EXPECT_EQ(runner().promptText(), "Specify new linetype scale <1.0000>:");
    type({"", "LWeight"});
    EXPECT_EQ(runner().promptText(), "Enter new lineweight <ByLayer>:");
    type({"", ""});

    EXPECT_FALSE(runner().isActive());
}

TEST_F(ChpropTest, QuestionsShowTheNewValueOnceTheEntitiesShareIt) {
    const core::EntityProperties value = other();
    seed(first(), value);
    seed(second(), value);
    selectFirst();
    selectSecond();

    type({"CHPROP", "Color"});
    EXPECT_EQ(runner().promptText(), "New color [Truecolor] <255,128,0>:");
    type({"", "LAyer"});
    EXPECT_EQ(runner().promptText(), "Enter new layer name <Muri>:");
    type({"", "LType"});
    EXPECT_EQ(runner().promptText(), "Enter new linetype name <ACAD_ISO02W100>:");
    type({"", "ltScale"});
    EXPECT_EQ(runner().promptText(), "Specify new linetype scale <2.0000>:");
    type({"", "LWeight"});
    EXPECT_EQ(runner().promptText(), "Enter new lineweight <0.50>:");
    type({"ByBlock", "Color", "byblock", "Color"});
    EXPECT_EQ(runner().promptText(), "New color [Truecolor] <BYBLOCK>:");
    type({"2", "Color"});
    EXPECT_EQ(runner().promptText(), "New color [Truecolor] <2>:");
}

TEST_F(ChpropTest, QuestionsShowVariesWhenTheEntitiesDiffer) {
    seed(first(), other());
    selectFirst();
    selectSecond();

    type({"CHPROP", "Color"});
    EXPECT_EQ(runner().promptText(), "New color [Truecolor] <varies>:");
    type({"", "LAyer"});
    EXPECT_EQ(runner().promptText(), "Enter new layer name <varies>:");
    type({"", "LType"});
    EXPECT_EQ(runner().promptText(), "Enter new linetype name <varies>:");
    type({"", "ltScale"});
    EXPECT_EQ(runner().promptText(), "Specify new linetype scale <varies>:");
    type({"", "LWeight"});
    EXPECT_EQ(runner().promptText(), "Enter new lineweight <varies>:");
}

TEST_F(ChpropTest, MixedEntitiesTakeTheNewValueAndThenShareIt) {
    seed(first(), other());
    selectFirst();
    selectSecond();

    type({"CHPROP", "Color", "red", "Color"});

    EXPECT_EQ(properties(first()).color, core::Color::fromIndex(1));
    EXPECT_EQ(properties(second()).color, core::Color::fromIndex(1));
    EXPECT_EQ(properties(first()).layer, layerId("Muri"));  // the other values stay as they were
    EXPECT_EQ(properties(second()).layer, document().layerZero());
    EXPECT_EQ(runner().promptText(), "New color [Truecolor] <1>:");
}

TEST_F(ChpropTest, EnterAtAValueQuestionLeavesTheEntitiesAsTheyAre) {
    const core::EntityProperties original = properties(first());
    const std::size_t steps = editor().undoNames().size();
    selectFirst();

    type({"CHPROP", "Color", ""});
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
    type({"LAyer", ""});
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
    type({"LType", ""});
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
    type({"ltScale", ""});
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
    type({"LWeight", ""});
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
    type({"Color", "Truecolor", ""});
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
    type({""});

    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(properties(first()), original);
    EXPECT_EQ(editor().undoNames().size(), steps);  // nothing changed: no step
}

TEST_F(ChpropTest, GivingTheValuesTheEntitiesAlreadyHaveMakesNoUndoStep) {
    const std::size_t steps = editor().undoNames().size();
    selectFirst();

    type({"CHPROP", "LAyer", "0", "Color", "bylayer", "ltScale", "1", ""});

    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(editor().undoNames().size(), steps);
}

TEST_F(ChpropTest, InvalidColorsKeepTheQuestion) {
    selectFirst();

    type({"CHPROP", "Color", "bogus"});
    EXPECT_TRUE(heard("Invalid color."));
    EXPECT_EQ(runner().promptText(), "New color [Truecolor] <BYLAYER>:");

    type({"T", "5"});  // an index is not a true color
    EXPECT_EQ(runner().promptText(), "Red, Green, Blue:");
    EXPECT_EQ(std::ranges::count(messages(), "Invalid color."), 2);
    EXPECT_EQ(properties(first()).color, core::Color::byLayer());

    type({"10,20,30"});
    EXPECT_EQ(properties(first()).color, core::Color::fromRgb({10, 20, 30}));
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
}

TEST_F(ChpropTest, ALayerThatDoesNotExistKeepsTheQuestion) {
    selectFirst();

    type({"CHPROP", "LAyer", "Nope"});
    EXPECT_TRUE(heard("Layer \"Nope\" not found."));
    EXPECT_EQ(runner().promptText(), "Enter new layer name <0>:");
    EXPECT_EQ(properties(first()).layer, document().layerZero());

    type({"muri"});  // names compare without regard to case
    EXPECT_EQ(properties(first()).layer, layerId("Muri"));
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
}

TEST_F(ChpropTest, ALinetypeThatIsNotLoadedKeepsTheQuestion) {
    selectFirst();

    type({"CHPROP", "LType", "Dashed"});
    EXPECT_TRUE(heard("Linetype \"Dashed\" is not loaded."));
    EXPECT_EQ(runner().promptText(), "Enter new linetype name <ByLayer>:");

    type({"ByBlock"});
    EXPECT_EQ(properties(first()).linetype, document().byBlockLinetype());
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
}

TEST_F(ChpropTest, ALinetypeScaleMustBePositive) {
    selectFirst();

    type({"CHPROP", "ltScale", "0", "-1", "abc"});
    EXPECT_EQ(std::ranges::count(messages(), "Requires a positive number."), 3);
    EXPECT_EQ(runner().promptText(), "Specify new linetype scale <1.0000>:");
    EXPECT_DOUBLE_EQ(properties(first()).linetypeScale, 1.0);

    type({"0.25"});
    EXPECT_DOUBLE_EQ(properties(first()).linetypeScale, 0.25);
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
}

TEST_F(ChpropTest, InvalidLineweightsKeepTheQuestion) {
    selectFirst();

    type({"CHPROP", "LWeight", "0.33", "abc"});
    EXPECT_EQ(std::ranges::count(messages(), "Invalid lineweight."), 2);
    EXPECT_EQ(runner().promptText(), "Enter new lineweight <ByLayer>:");
    EXPECT_EQ(properties(first()).lineweight, core::lineweight::kByLayer);

    type({"ByBlock"});
    EXPECT_EQ(properties(first()).lineweight, core::lineweight::kByBlock);
    type({"LWeight"});
    EXPECT_EQ(runner().promptText(), "Enter new lineweight <ByBlock>:");
}

TEST_F(ChpropTest, OptionsAcceptNamesAndShortcutsInAnyCase) {
    selectFirst();

    type({"CHPROP", "c", "red", "la", "MURI", "lt", "acad_iso02w100", "LTS", "3", "lw", "0.30"});

    const core::EntityProperties changed = properties(first());
    EXPECT_EQ(changed.color, core::Color::fromIndex(1));
    EXPECT_EQ(changed.layer, layerId("Muri"));
    EXPECT_EQ(changed.linetype, linetypeId("ACAD_ISO02W100"));
    EXPECT_DOUBLE_EQ(changed.linetypeScale, 3.0);
    EXPECT_EQ(changed.lineweight, 30);
}

TEST_F(ChpropTest, AnUnknownOptionKeepsTheQuestion) {
    selectFirst();

    type({"CHPROP", "Thickness", "TRansparency", "10,10"});

    EXPECT_EQ(std::ranges::count(messages(), "Invalid option keyword."), 3);
    EXPECT_EQ(runner().promptText(), kPropertyPrompt);
    EXPECT_TRUE(runner().isActive());
}

TEST_F(ChpropTest, AFrozenLayerCanBeAssignedAndTheEntityDisappears) {
    type({"-LAYER", "Freeze", "Muri", ""});
    selectFirst();

    type({"CHPROP", "LAyer", "Muri", ""});

    EXPECT_EQ(properties(first()).layer, layerId("Muri"));
    EXPECT_FALSE(core::isVisible(document(), *document().find(first())));
    EXPECT_TRUE(core::isVisible(document(), *document().find(second())));
    EXPECT_FALSE(heard("Layer \"Muri\" not found."));
}

TEST_F(ChpropTest, AnEntityGivenALockedLayerCannotBeSelectedAgain) {
    type({"-LAYER", "LOck", "Porte", ""});
    selectFirst();

    type({"CHPROP", "LAyer", "Porte", ""});
    runner().click({5, 0}, PickAction::Add);

    EXPECT_EQ(properties(first()).layer, layerId("Porte"));
    EXPECT_TRUE(runner().selection().isEmpty());
}

TEST_F(ChpropTest, EscKeepsTheChangesAlreadyMade) {
    const std::size_t steps = editor().undoNames().size();
    selectFirst();

    type({"CHPROP", "Color", "red", "LAyer"});
    runner().submit(CancelInput{});

    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_EQ(messages().back(), "*Cancel*");
    EXPECT_EQ(properties(first()).color, core::Color::fromIndex(1));
    ASSERT_EQ(editor().undoNames().size(), steps + 1);
    EXPECT_EQ(editor().undoName(), "CHPROP");

    ASSERT_TRUE(editor().undo());
    EXPECT_EQ(properties(first()).color, core::Color::byLayer());
    ASSERT_TRUE(editor().redo());
    EXPECT_EQ(properties(first()).color, core::Color::fromIndex(1));
}

TEST_F(ChpropTest, EscBeforeAnyChangeLeavesNoUndoStep) {
    const std::size_t steps = editor().undoNames().size();
    selectFirst();

    type({"CHPROP"});
    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().isActive());

    selectFirst();
    type({"CHPROP", "Color"});
    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().isActive());

    EXPECT_FALSE(editor().inTransaction());
    EXPECT_EQ(editor().undoNames().size(), steps);
    EXPECT_EQ(properties(first()), properties(second()));
}

TEST_F(ChpropTest, EscWhileSelectingEndsTheCommandWithNoEffect) {
    const std::size_t steps = editor().undoNames().size();

    type({"CHPROP"});
    runner().submit(CancelInput{});

    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(editor().undoNames().size(), steps);
}

TEST_F(ChpropTest, UndoAndRedoAreCommandsOfTheRunnerAndCountTheSteps) {
    selectFirst();
    type({"CHPROP", "Color", "red", ""});
    selectFirst();
    type({"CHPROP", "LAyer", "Muri", ""});

    type({"U"});
    EXPECT_EQ(messages().back(), "Undone: CHPROP.");
    EXPECT_EQ(properties(first()).layer, document().layerZero());
    EXPECT_EQ(properties(first()).color, core::Color::fromIndex(1));
    type({"U"});
    EXPECT_EQ(properties(first()).color, core::Color::byLayer());
    type({"REDO", "REDO"});
    EXPECT_EQ(messages().back(), "Redone: CHPROP.");
    EXPECT_EQ(properties(first()).layer, layerId("Muri"));
}

TEST_F(ChpropTest, TheDescriptionListsEveryPrompt) {
    const CommandDescription* description = registry().describe("chprop");

    ASSERT_NE(description, nullptr);
    EXPECT_EQ(description->name, "CHPROP");
    EXPECT_TRUE(description->aliases.empty());
    EXPECT_TRUE(description->selectsFirst);
    EXPECT_FALSE(description->summary.empty());
    EXPECT_EQ(description->summary.back(), '.');
    std::vector<std::string> ids;
    for (const PromptDescription& prompt : description->prompts) {
        ids.push_back(prompt.id);
    }
    EXPECT_EQ(ids, (std::vector<std::string>{"property", "color", "rgb", "layer", "linetype",
                                             "linetype-scale", "lineweight"}));
}

}  // namespace
}  // namespace tg::cmd
