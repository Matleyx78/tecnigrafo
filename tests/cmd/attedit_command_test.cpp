// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/attedit_command.hpp"

#include "../../cmd/src/name_list.hpp"
#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/color.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

constexpr const char* kOption =
    "Enter an option [Value/Position/Height/Angle/Style/Layer/Color/Next] <N>:";

core::Attribute attributeAt(const char* tag, const char* value, core::Vec2 position) {
    core::Attribute attribute;
    attribute.tag = tag;
    attribute.text.height = 1.0;
    attribute.text.value = value;
    attribute.text.position = position;
    return attribute;
}

class AtteditCommandTest : public testing::Test {
protected:
    AtteditCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string message) { m_messages.push_back(std::move(message)); });
        m_document.replaceBlock("A", core::BlockDefinition{.name = "A"});
        m_document.replaceBlock("B", core::BlockDefinition{.name = "B"});
    }

    core::EntityId addInsert(const char* block, std::vector<core::Attribute> attributes) {
        core::Insert insert;
        insert.block = block;
        insert.attributes = std::move(attributes);
        for (core::Attribute& attribute : insert.attributes) {
            attribute.properties.layer = m_document.layerZero();
        }
        m_editor.beginTransaction("Setup");
        const core::EntityId id = m_editor.add(insert);
        m_editor.commit();
        return id;
    }

    void submit(std::initializer_list<const char*> inputs) {
        for (const char* input : inputs) {
            m_runner.submitText(input);
        }
    }

    [[nodiscard]] const core::Attribute& attribute(core::EntityId insert, std::size_t index) const {
        return std::get<core::Insert>(m_document.find(insert)->entity).attributes.at(index);
    }

    [[nodiscard]] bool heard(const std::string& text) const {
        return std::ranges::find(m_messages, text) != m_messages.end();
    }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const CommandRegistry& registry() const { return m_registry; }

    [[nodiscard]] std::vector<std::string>& messages() { return m_messages; }

private:
    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(AtteditCommandTest, EditsOneAttributeWithEveryOption) {
    const core::EntityId id = addInsert(
        "A", {attributeAt("SIGLA", "R30", {0, 10}), attributeAt("CODICE", "K10", {0, 0})});
    submit({"-LAYER", "New", "ATTRIBUTI", ""});
    messages().clear();
    const std::size_t steps = editor().undoNames().size();

    submit({"ATE", "Y", "a", "codice", "K*", "C", "-1,-1", "1,1"});
    EXPECT_TRUE(heard("1 found"));
    EXPECT_TRUE(heard("1 attributes selected."));
    EXPECT_EQ(runner().promptText(), kOption);
    submit({"V", "R", "K11", "P", "5,-27"});
    EXPECT_EQ(attribute(id, 1).text.value, "K11");
    EXPECT_EQ(attribute(id, 1).text.position, (core::Vec2{5, -27}));
    submit({"H"});
    EXPECT_EQ(runner().promptText(), "Specify new height <1>:");
    submit({"3.5", "A", "15", "S"});
    EXPECT_EQ(runner().promptText(), "Enter new text style <Standard>:");
    submit({"ARIAL"});
    EXPECT_EQ(messages().back(), "Cannot find text style \"ARIAL\".");
    submit({"standard", "L"});
    EXPECT_EQ(runner().promptText(), "Enter new layer name <0>:");
    submit({"ATTRIBUTI", "C"});
    EXPECT_EQ(runner().promptText(), "Enter new color [Truecolor/Colorbook] <BYLAYER>:");
    submit({"1", "V", "C", "1", "2"});
    const core::Attribute& edited = attribute(id, 1);
    EXPECT_EQ(edited.text.value, "K21");  // only the first occurrence
    EXPECT_DOUBLE_EQ(edited.text.height, 3.5);
    EXPECT_NEAR(edited.text.rotation, core::kPi / 12.0, 1e-12);
    EXPECT_EQ(edited.text.style, "Standard");
    EXPECT_EQ(document().layer(edited.properties.layer)->name, "ATTRIBUTI");
    EXPECT_EQ(edited.properties.color, core::Color::fromIndex(1));
    EXPECT_EQ(attribute(id, 0).text.value, "R30");  // not chosen
    submit({"N"});
    EXPECT_FALSE(runner().isActive());
    ASSERT_EQ(editor().undoNames().size(), steps + 1);
    EXPECT_EQ(editor().undoNames().back(), "Attedit");
    ASSERT_TRUE(editor().undo());
    EXPECT_EQ(attribute(id, 1).text.value, "K10");
    EXPECT_EQ(attribute(id, 1).text.position, (core::Vec2{0, 0}));
}

TEST_F(AtteditCommandTest, PositionHeightAndAngleAreRefusedForALockedAttribute) {
    core::Attribute locked = attributeAt("SIGLA", "R30", {0, 0});
    locked.lockPosition = true;
    const core::EntityId id = addInsert("A", {locked});
    submit({"-ATTEDIT", "", "", "", "", "C", "-1,-1", "1,1", "P"});
    EXPECT_EQ(messages().back(), "Attribute position is locked.");
    submit({"H", "A", "V", "C", "30", "31"});
    EXPECT_EQ(attribute(id, 0).text.value, "R31");
    EXPECT_EQ(attribute(id, 0).text.position, (core::Vec2{0, 0}));
    EXPECT_EQ(std::ranges::count(messages(), "Attribute position is locked."), 3);
    submit({""});
    EXPECT_FALSE(runner().isActive());
}

TEST_F(AtteditCommandTest, NothingSelectedEndsTheCommandWithOneStep) {
    const core::EntityId id = addInsert("A", {attributeAt("CODICE", "K10", {0, 0})});
    const std::size_t steps = editor().undoNames().size();
    submit({"-ATTEDIT", "Y", "A", "NO_MATCH_#", "*", "C", "-1,-1", "1,1"});
    EXPECT_TRUE(heard("0 found"));
    EXPECT_EQ(messages().back(), "0 attributes selected.");
    EXPECT_FALSE(runner().isActive());
    // One step for the whole command, even without a change (ADR 0056,
    // point 4), which leaves the drawing as it was.
    ASSERT_EQ(editor().undoNames().size(), steps + 1);
    EXPECT_EQ(editor().undoNames().back(), "Attedit");
    EXPECT_EQ(document().find(id)->properties.color, core::Color::byLayer());
    submit({"U"});
    EXPECT_EQ(messages().back(), "Undone: Attedit.");
    EXPECT_EQ(editor().undoNames().size(), steps);
}

TEST_F(AtteditCommandTest, AnEmptyDrawingStillGetsOneStep) {
    // Without entities the step cannot come from one of them (ADR 0056,
    // point 4): the drawing is left as it was, layer 0 included.
    ASSERT_TRUE(document().entities().empty());
    const core::Layer zero = *document().layer(document().layerZero());
    const std::size_t steps = editor().undoNames().size();
    submit({"-ATTEDIT", "Y", "*", "*", "*", "C", "-1,-1", "1,1"});
    EXPECT_EQ(messages().back(), "0 attributes selected.");
    EXPECT_FALSE(runner().isActive());
    ASSERT_EQ(editor().undoNames().size(), steps + 1);
    EXPECT_EQ(editor().undoNames().back(), "Attedit");
    EXPECT_EQ(*document().layer(document().layerZero()), zero);

    // Globally, with the invisible attributes and so without a selection.
    submit({"-ATTEDIT", "N", "N", "*", "*", "*"});
    EXPECT_EQ(messages().back(), "0 attributes selected.");
    EXPECT_FALSE(runner().isActive());
    ASSERT_EQ(editor().undoNames().size(), steps + 2);
    EXPECT_EQ(editor().undoNames().back(), "Attedit");
    EXPECT_EQ(*document().layer(document().layerZero()), zero);

    ASSERT_TRUE(editor().undo());
    ASSERT_TRUE(editor().undo());
    EXPECT_EQ(editor().undoNames().size(), steps);
    EXPECT_EQ(*document().layer(document().layerZero()), zero);
}

TEST_F(AtteditCommandTest, AnEscapedCommaFiltersTheValues) {
    const core::EntityId first = addInsert("A", {attributeAt("NOTA", "1,2", {0, 0})});
    const core::EntityId second = addInsert("A", {attributeAt("NOTA", "1", {0, 5})});
    submit({"-ATTEDIT", "N", "N", "*", "*", "1`,2"});
    EXPECT_EQ(messages().back(), "1 attributes selected.");
    submit({"1", "3"});
    EXPECT_EQ(attribute(first, 0).text.value, "3,2");
    EXPECT_EQ(attribute(second, 0).text.value, "1");
}

TEST_F(AtteditCommandTest, CrossingIsAnUnwrittenKeywordOfSelectAttributes) {
    addInsert("A", {attributeAt("CODICE", "K10", {0, 0})});
    submit({"-ATTEDIT", "Y", "*", "*", "*", "C"});
    EXPECT_TRUE(runner().isActive());
    EXPECT_FALSE(heard("Invalid input."));
    EXPECT_EQ(runner().promptText(), "Specify first corner:");
}

TEST_F(AtteditCommandTest, TheClickAppliesTheTagFilterBeforeTheNearest) {
    // Two visible attributes one over the other: CODICE is found under SIGLA.
    const core::EntityId id =
        addInsert("A", {attributeAt("SIGLA", "R30", {0, 0}), attributeAt("CODICE", "K10", {0, 0})});
    submit({"-ATTEDIT", "Y", "*", "CODICE", "*"});
    runner().click({0.5, 0.5}, PickAction::Add);
    EXPECT_EQ(messages().back(), "1 attributes selected.");
    ASSERT_EQ(runner().highlighted().size(), 1U);
    EXPECT_EQ(runner().highlighted().front(), id);
    submit({"V", "", "K11", ""});
    EXPECT_EQ(attribute(id, 0).text.value, "R30");
    EXPECT_EQ(attribute(id, 1).text.value, "K11");
}

TEST_F(AtteditCommandTest, TheClickAppliesTheBlockFilterBeforeTheNearest) {
    // Two inserts one over the other with the same CODICE: the later one,
    // of B, would win a tie; the filter of the block takes the one of A.
    const core::EntityId first = addInsert("A", {attributeAt("CODICE", "K10", {0, 0})});
    const core::EntityId second = addInsert("B", {attributeAt("CODICE", "K10", {0, 0})});
    submit({"-ATTEDIT", "Y", "A", "CODICE", "K10"});
    runner().click({0.5, 0.5}, PickAction::Add);
    EXPECT_EQ(messages().back(), "1 attributes selected.");
    submit({"V", "R", "K11", "N"});
    EXPECT_EQ(attribute(first, 0).text.value, "K11");
    EXPECT_EQ(attribute(second, 0).text.value, "K10");
}

TEST_F(AtteditCommandTest, GlobalEditingReplacesEveryOccurrence) {
    core::Attribute hidden = attributeAt("NOTA", "da cablare, poi cablare", {0, 20});
    hidden.invisible = true;
    const core::EntityId first = addInsert("A", {attributeAt("CODICE", "K10", {0, 0}), hidden});
    const core::EntityId second = addInsert("B", {attributeAt("CODICE", "K20", {20, 0})});

    // Not only the visible ones: the invisible attribute too, without a selection.
    submit({"-ATTEDIT", "N", "N"});
    EXPECT_TRUE(heard("Performing global editing of attribute values."));
    EXPECT_TRUE(heard("Drawing must be regenerated afterwards."));
    submit({"A", "NOTA", "*"});
    EXPECT_EQ(messages().back(), "1 attributes selected.");
    submit({"cablare", "verificare"});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(attribute(first, 1).text.value, "da verificare, poi verificare");

    // Only the visible ones, with a selection: the invisible one is left out.
    submit({"-ATTEDIT", "N", "", "*", "*", "*", "C", "-1,-1", "30,30"});
    EXPECT_TRUE(heard("2 found"));
    EXPECT_EQ(messages().back(), "2 attributes selected.");
    submit({"K", "KA"});
    EXPECT_EQ(attribute(first, 0).text.value, "KA10");
    EXPECT_EQ(attribute(second, 0).text.value, "KA20");
    EXPECT_EQ(attribute(first, 1).text.value, "da verificare, poi verificare");
    EXPECT_EQ(editor().undoNames().back(), "Attedit");
}

TEST_F(AtteditCommandTest, OneAtATimeFollowsTheInsertsAndTheirAttributes) {
    const core::EntityId first =
        addInsert("A", {attributeAt("UNO", "1", {0, 0}), attributeAt("DUE", "2", {0, 5})});
    const core::EntityId second = addInsert("B", {attributeAt("TRE", "3", {20, 0})});
    submit({"-ATTEDIT", "", "", "", "", "W", "-1,-5", "40,10"});
    EXPECT_EQ(messages().back(), "3 attributes selected.");
    submit({"V", "", "a", "N", "V", "", "b", "", "V", "", "c"});
    EXPECT_EQ(attribute(first, 0).text.value, "a");
    EXPECT_EQ(attribute(first, 1).text.value, "b");
    EXPECT_EQ(attribute(second, 0).text.value, "c");
    EXPECT_TRUE(runner().isActive());
    submit({""});
    EXPECT_FALSE(runner().isActive());
}

TEST_F(AtteditCommandTest, EveryPromptMatchesTheDescription) {
    const CommandDescription description = AtteditCommand::description();
    EXPECT_EQ(description.name, "-ATTEDIT");
    EXPECT_EQ(description.aliases, (std::vector<std::string>{"ATE", "ATTEDIT"}));
    addInsert("A", {attributeAt("CODICE", "K10", {0, 0})});
    const auto check = [this, &description] {
        const Prompt shown = runner().prompt();
        EXPECT_TRUE(std::ranges::any_of(description.prompts, [&shown](
                                                                 const PromptDescription& prompt) {
            return matches(prompt, shown);
        })) << shown.text;
    };
    for (const char* input :
         {"-ATTEDIT", "Y", "*",        "*", "*", "C", "-1,-1", "1,1",     "V", "C",
          "K",        "J", "V",        "R", "X", "P", "1,1",   "H",       "2", "A",
          "10",       "S", "Standard", "L", "0", "C", "T",     "255,0,0", "N"}) {
        if (runner().isActive()) {
            check();
        }
        runner().submitText(input);
    }
    EXPECT_FALSE(runner().isActive());
}

// The wildcards of AutoCAD in the filters (docs/adr/0056, point 4).
TEST(AtteditNameList, TakesEveryWildcardOfAutoCad) {
    using detail::matchesName;
    EXPECT_TRUE(matchesName("K#", "K1"));
    EXPECT_FALSE(matchesName("K#", "KA"));
    EXPECT_TRUE(matchesName("@1", "a1"));
    EXPECT_FALSE(matchesName("@1", "11"));
    EXPECT_TRUE(matchesName("A.B", "A-B"));
    EXPECT_FALSE(matchesName("A.B", "A1B"));
    EXPECT_FALSE(matchesName("A.B", "AxB"));
    EXPECT_TRUE(matchesName("[A-C]1", "b1"));
    EXPECT_FALSE(matchesName("[A-C]1", "D1"));
    EXPECT_TRUE(matchesName("[XZ]", "z"));
    EXPECT_FALSE(matchesName("[XZ]", "Y"));
    EXPECT_TRUE(matchesName("[~A-C]1", "D1"));
    EXPECT_FALSE(matchesName("[~A-C]1", "a1"));
    EXPECT_TRUE(matchesName("~K*", "R30"));
    EXPECT_FALSE(matchesName("~K*", "k10"));
    EXPECT_TRUE(matchesName("`*", "*"));
    EXPECT_FALSE(matchesName("`*", "A"));
    EXPECT_TRUE(matchesName("`#`[", "#["));
    EXPECT_TRUE(matchesName("[A", "[a"));  // without its ']'
    EXPECT_TRUE(matchesName("~", "~"));
}

TEST(AtteditNameList, SplitsOnlyAtTheCommasThatAreNotEscaped) {
    using detail::matchesNameList;
    EXPECT_TRUE(matchesNameList("`,", ","));
    EXPECT_FALSE(matchesNameList("`,", ""));
    EXPECT_TRUE(matchesNameList("A`,B,C", "a,b"));
    EXPECT_TRUE(matchesNameList("A`,B,C", "C"));
    EXPECT_FALSE(matchesNameList("A`,B,C", "A"));
    EXPECT_FALSE(matchesNameList("A`,B,C", "B"));
    EXPECT_TRUE(matchesNameList("``,X", "`"));  // the comma after "``" splits
    EXPECT_TRUE(matchesNameList("``,X", "x"));
    EXPECT_TRUE(matchesNameList(" ` , X ", " "));  // the escaped space stays
    EXPECT_TRUE(matchesNameList(" ` , X ", "X"));
    EXPECT_TRUE(matchesNameList("K*, ,R?0", "R30"));
    EXPECT_TRUE(matchesNameList("K*,,R?0", ""));
}

}  // namespace
}  // namespace tg::cmd
