// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

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

    void submit(std::initializer_list<const char*> inputs) {
        for (const char* input : inputs) {
            m_runner.submitText(input);
        }
    }

    [[nodiscard]] const core::Entity& entity(core::EntityId id) const {
        return m_document.find(id)->entity;
    }

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
    ASSERT_TRUE(m_runner.textEditing());
    EXPECT_EQ(m_runner.textEditing()->target, textId);
    EXPECT_EQ(m_runner.textEditing()->value, "OLD");
    submit({"NEW", "0,-10", "AFTER", ""});

    EXPECT_EQ(std::get<core::Text>(entity(textId)).value, "NEW");
    EXPECT_EQ(std::get<core::MText>(entity(mtextId)).value, "AFTER");
    ASSERT_GE(m_editor.undoNames().size(), 2U);
    EXPECT_EQ(m_editor.undoNames()[m_editor.undoNames().size() - 2], "Textedit");
    EXPECT_EQ(m_editor.undoNames().back(), "Textedit");
}

TEST_F(TexteditCommandTest, EmptyEditorValueKeepsTextAndUndoRevertsTheLastEdit) {
    core::Text text;
    text.value = "OLD";
    const core::EntityId id = add(text);

    submit({"TEXTEDIT", "0,0"});
    m_runner.submit(TextInput{""});
    EXPECT_EQ(std::get<core::Text>(entity(id)).value, "OLD");
    submit({"0,0", "NEW", "U", ""});
    EXPECT_EQ(std::get<core::Text>(entity(id)).value, "OLD");
    EXPECT_EQ(m_messages.back(), "The command has been completely undone.");
}

TEST_F(TexteditCommandTest, SingleModePersistsAndFinishesAfterOneObject) {
    core::Text text;
    text.value = "OLD";
    const core::EntityId id = add(text);

    submit({"TEXTEDIT", "M", "S", "0,0", "NEW"});
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_EQ(std::get<core::Text>(entity(id)).value, "NEW");

    submit({"TEXTEDIT"});
    EXPECT_EQ(m_runner.promptText(), "Select an annotation object or [Mode]:");
    m_runner.submit(CancelInput{});
}

TEST_F(TexteditCommandTest, NewMtextExposesTheEditorRequest) {
    submit({"MTEXT", "0,0", "20,-10"});
    ASSERT_TRUE(m_runner.textEditing());
    EXPECT_FALSE(m_runner.textEditing()->target);
    EXPECT_TRUE(std::holds_alternative<core::MText>(m_runner.textEditing()->shown));
    m_runner.submit(TextInput{""});
    m_runner.submitText("");
    EXPECT_TRUE(m_document.isEmpty());
}

}  // namespace
}  // namespace tg::cmd
