// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

class TexteditCommandTest : public testing::Test {
protected:
    TexteditCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string message) { m_messages.push_back(std::move(message)); });
    }

    core::EntityId add(core::Entity entity) {
        m_editor.beginTransaction("Setup");
        const core::EntityId id = m_editor.add(std::move(entity));
        m_editor.commit();
        return id;
    }

    core::EntityId addDimension(core::Dimension dimension) {
        const auto regenerated = core::regenerated(document(), std::move(dimension));
        EXPECT_TRUE(regenerated);
        return add(regenerated.value_or(core::Dimension{}));
    }

    void submit(std::initializer_list<const char*> inputs) {
        for (const char* input : inputs) {
            m_runner.submitText(input);
        }
    }

    [[nodiscard]] const core::Entity& entity(core::EntityId id) const {
        return m_document.find(id)->entity;
    }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

private:
    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(TexteditCommandTest, MultipleEditsTextAndMtextInSeparateUndoSteps) {
    core::Text text;
    text.position = {0, 0};
    text.value = "OLD";
    const core::EntityId textId = add(text);
    core::MText mtext;
    mtext.insertion = {0, -10};
    mtext.width = 30;
    mtext.value = "BEFORE";
    const core::EntityId mtextId = add(mtext);

    submit({"TEXTEDIT", "0,0"});
    const std::optional<TextEditRequest> pendingRequest = runner().textEditing();
    ASSERT_TRUE(pendingRequest);
    const TextEditRequest request = pendingRequest.value_or(TextEditRequest{});
    EXPECT_EQ(request.target, textId);
    EXPECT_EQ(request.value, "OLD");
    EXPECT_TRUE(std::holds_alternative<core::Text>(request.shown));
    EXPECT_FALSE(request.measurement);
    submit({"NEW", "0,-10", "AFTER", ""});

    EXPECT_EQ(std::get<core::Text>(entity(textId)).value, "NEW");
    EXPECT_EQ(std::get<core::MText>(entity(mtextId)).value, "AFTER");
    ASSERT_GE(editor().undoNames().size(), 2U);
    EXPECT_EQ(editor().undoNames()[editor().undoNames().size() - 2], "Textedit");
    EXPECT_EQ(editor().undoNames().back(), "Textedit");
}

TEST_F(TexteditCommandTest, OnePickfirstObjectIsEditedButTwoAreNotConsumed) {
    core::Text first;
    first.position = {0, 0};
    first.value = "FIRST";
    add(first);
    core::Text second;
    second.position = {20, 0};
    second.value = "SECOND";
    add(second);

    runner().click({0, 0}, PickAction::Add);
    submit({"ED"});
    EXPECT_EQ(runner().promptText(), "Enter new text <FIRST>:");
    runner().submit(CancelInput{});

    runner().click({0, 0}, PickAction::Add);
    runner().click({20, 0}, PickAction::Add);
    submit({"ED"});
    EXPECT_EQ(runner().promptText(), "Select an annotation object or [Undo/Mode]:");
    EXPECT_TRUE(runner().selection().isEmpty());
    runner().submit(CancelInput{});
}

TEST_F(TexteditCommandTest, InvalidAttributeTagRepeatsItsPrompt) {
    core::AttributeDefinition definition;
    definition.text.height = 1.0;
    definition.tag = "ALFA";
    definition.prompt = "Prompt";
    const core::EntityId id = add(definition);

    submit({"TEXTEDIT", "0,0", "DUE PAROLE"});
    EXPECT_FALSE(runner().textEditing());
    EXPECT_EQ(messages().back(), "Invalid tag name.");
    EXPECT_EQ(runner().promptText(), "Enter attribute tag <ALFA>:");
    runner().submit(TextInput{""});
    EXPECT_EQ(messages().back(), "Invalid tag name.");
    EXPECT_EQ(runner().promptText(), "Enter attribute tag <ALFA>:");
    runner().submit(TextInput{"BETA"});
    EXPECT_EQ(runner().promptText(), "Enter attribute prompt <Prompt>:");
    EXPECT_EQ(std::get<core::AttributeDefinition>(entity(id)).tag, "BETA");
    runner().submit(CancelInput{});
}

TEST_F(TexteditCommandTest, InsertAttributesNeverExposeTheEditorRequest) {
    document().replaceBlock("ATTRS", core::BlockDefinition{.name = "ATTRS"});
    core::Insert insert;
    insert.block = "ATTRS";
    core::Attribute attribute;
    attribute.tag = "CODE";
    attribute.text.height = 1.0;
    attribute.text.value = "A1";
    attribute.text.position = {30, 0};
    insert.attributes.push_back(attribute);
    add(insert);

    runner().submitText("TEXTEDIT");
    EXPECT_FALSE(runner().textEditing());
    runner().click({30, 0}, PickAction::Add);
    EXPECT_EQ(runner().promptText(), "CODE <A1>:");
    EXPECT_FALSE(runner().textEditing());
    runner().submit(CancelInput{});
}

TEST_F(TexteditCommandTest, EmptyEditorValueKeepsTextAndUndoRevertsTheLastEdit) {
    core::Text text;
    text.value = "OLD";
    const core::EntityId id = add(text);

    submit({"TEXTEDIT", "0,0"});
    const std::size_t undoSteps = editor().undoNames().size();
    runner().submit(TextInput{""});
    EXPECT_EQ(std::get<core::Text>(entity(id)).value, "OLD");
    EXPECT_EQ(editor().undoNames().size(), undoSteps);
    submit({"0,0", "NEW", "U", ""});
    EXPECT_EQ(std::get<core::Text>(entity(id)).value, "OLD");
    EXPECT_EQ(messages().back(), "The command has been completely undone.");
}

TEST_F(TexteditCommandTest, SingleModePersistsAndFinishesAfterOneObject) {
    core::Text text;
    text.value = "OLD";
    const core::EntityId id = add(text);

    submit({"TEXTEDIT", "M", "S", "0,0", "NEW"});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(std::get<core::Text>(entity(id)).value, "NEW");

    submit({"TEXTEDIT"});
    EXPECT_EQ(runner().promptText(), "Select an annotation object or [Mode]:");
    runner().submit(CancelInput{});
}

TEST_F(TexteditCommandTest, NewMtextExposesTheEditorRequest) {
    submit({"MTEXT", "0,0", "20,-10"});
    const std::optional<TextEditRequest> pendingRequest = runner().textEditing();
    ASSERT_TRUE(pendingRequest);
    const TextEditRequest request = pendingRequest.value_or(TextEditRequest{});
    EXPECT_FALSE(request.target);
    EXPECT_TRUE(std::holds_alternative<core::MText>(request.shown));
    EXPECT_TRUE(request.value.empty());
    runner().submit(TextInput{""});
    runner().submitText("");
    EXPECT_TRUE(document().isEmpty());
}

TEST_F(TexteditCommandTest, MtextRequestCarriesShownEntityAndEmptyValueIsKept) {
    core::MText mtext;
    mtext.insertion = {0, 0};
    mtext.width = 20;
    mtext.value = "PROVA";
    const core::EntityId id = add(mtext);

    submit({"TEXTEDIT", "0,0"});
    const auto request = runner().textEditing();
    ASSERT_TRUE(request);
    const TextEditRequest edit = request.value_or(TextEditRequest{});
    EXPECT_EQ(edit.target, id);
    EXPECT_EQ(edit.value, "PROVA");
    EXPECT_EQ(std::get<core::MText>(edit.shown).value, "PROVA");
    EXPECT_FALSE(edit.measurement);
    const std::size_t undoSteps = editor().undoNames().size();
    runner().submit(TextInput{""});
    EXPECT_EQ(std::get<core::MText>(entity(id)).value, "PROVA");
    EXPECT_EQ(editor().undoNames().size(), undoSteps);
}

TEST_F(TexteditCommandTest, EmptyMtextRemainsInTheDrawingWithoutAnUndoStep) {
    core::MText mtext;
    mtext.insertion = {0, 0};
    mtext.width = 20;
    mtext.value = "";
    const core::EntityId id = add(mtext);

    runner().submitText("TEXTEDIT");
    runner().submit(ObjectInput{id, {}, false});
    const std::size_t undoSteps = editor().undoNames().size();
    runner().submit(TextInput{""});

    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_TRUE(std::get<core::MText>(entity(id)).value.empty());
    EXPECT_EQ(editor().undoNames().size(), undoSteps);
}

TEST_F(TexteditCommandTest, LinearDimensionRequestSeparatesShownAndReplacementText) {
    core::Dimension dimension;
    dimension.definition.kind = core::DimensionKind::Linear;
    dimension.definition.point13 = {0, 0};
    dimension.definition.point14 = {30, 0};
    dimension.definition.linePoint = {15, 10};
    dimension.presentation.textOverride = "<> mm";
    const core::EntityId id = addDimension(dimension);
    const auto& made = std::get<core::Dimension>(entity(id));
    const core::Vec2 textPoint = made.presentation.textPoint;

    runner().submitText("TEXTEDIT");
    runner().click(textPoint, PickAction::Add);
    const auto request = runner().textEditing();
    ASSERT_TRUE(request);
    const TextEditRequest edit = request.value_or(TextEditRequest{});
    EXPECT_EQ(edit.target, id);
    EXPECT_EQ(edit.value, "<> mm");
    EXPECT_EQ(edit.measurement, "30");
    EXPECT_EQ(std::get<core::MText>(edit.shown).value, "30 mm");

    runner().submit(ConfirmInput{});
    EXPECT_EQ(std::get<core::Dimension>(entity(id)).presentation.textOverride, "<> mm");
    runner().click(textPoint, PickAction::Add);
    runner().submit(TextInput{""});
    EXPECT_TRUE(std::get<core::Dimension>(entity(id)).presentation.textOverride.empty());
    EXPECT_EQ(editor().undoNames().back(), "Textedit");
    runner().submit(CancelInput{});
}

TEST_F(TexteditCommandTest, AngularDimensionMeasurementIsFormattedInDegrees) {
    core::Dimension dimension;
    dimension.definition.kind = core::DimensionKind::Angular;
    dimension.definition.dxfType = 2;
    dimension.definition.point13 = {0, 0};
    dimension.definition.point14 = {0, 40};
    dimension.definition.linePoint = {40, 0};
    dimension.definition.point15 = {0, 0};
    dimension.definition.point16 = {10, 10};
    const core::EntityId id = addDimension(dimension);
    const auto& made = std::get<core::Dimension>(entity(id));

    runner().submitText("TEXTEDIT");
    runner().click(made.presentation.textPoint, PickAction::Add);
    const auto request = runner().textEditing();
    ASSERT_TRUE(request);
    const TextEditRequest edit = request.value_or(TextEditRequest{});
    const auto variables = core::dimensionVariables(document().dimStyles(), made.presentation.style,
                                                    made.presentation.overrides);
    EXPECT_EQ(edit.measurement,
              core::dimensionText(variables, core::DimensionKind::Angular, 90.0, "<>"));
    runner().submit(CancelInput{});
}

}  // namespace
}  // namespace tg::cmd
