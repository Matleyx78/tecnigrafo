// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

class EraseCommandTest : public testing::Test {
protected:
    EraseCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
        m_editor.beginTransaction("Seed drawing");
        const core::EntityId first =
            m_editor.add(core::Polyline{{{{0.0, 0.0}, 0.0}, {{10.0, 0.0}, 0.0}}});
        const core::EntityId second =
            m_editor.add(core::Polyline{{{{0.0, 20.0}, 1.0}, {{10.0, 20.0}, 0.0}}});
        const core::EntityId third = m_editor.add(
            core::Polyline{{{{0.0, 40.0}, 0.0}, {{10.0, 40.0}, 0.0}, {{5.0, 50.0}, 0.0}}, true});
        m_editor.commit();
        for (const core::EntityId id : {first, second, third}) {
            m_originals.push_back(*m_document.find(id));
        }
        m_editor.setChangeListener([this] { ++m_notifications; });
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] CommandRegistry& registry() { return m_registry; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    [[nodiscard]] std::size_t notifications() const { return m_notifications; }

    [[nodiscard]] const core::EntityRecord& original(std::size_t index) const {
        return m_originals.at(index);
    }

    void select(std::size_t index, PickAction action = PickAction::Add) {
        const auto& polyline = std::get<core::Polyline>(original(index).entity);
        runner().click(polyline.vertices().front().position, action);
    }

    void expectOriginal(std::size_t index) const {
        const core::EntityRecord* record = document().find(original(index).id);
        ASSERT_NE(record, nullptr);
        EXPECT_EQ(*record, original(index));
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<core::EntityRecord> m_originals;
    std::vector<std::string> m_messages;
    std::size_t m_notifications{0};
};

class EraseCommandNamesTest : public EraseCommandTest,
                              public testing::WithParamInterface<const char*> {};

TEST_P(EraseCommandNamesTest, ErasesPreselectionImmediatelyAndSilently) {
    select(0);
    select(2);
    ASSERT_EQ(runner().selection().size(), 2U);
    runner().submitText(GetParam());
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(runner().promptText(), "Command:");
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(document().find(original(0).id), nullptr);
    EXPECT_EQ(document().find(original(2).id), nullptr);
    expectOriginal(1);
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_TRUE(runner().preview().empty());
    EXPECT_TRUE(messages().empty());
    EXPECT_EQ(editor().undoName(), "Erase");
    EXPECT_EQ(notifications(), 1U);
}

TEST_P(EraseCommandNamesTest, WaitsForSelectionAndConfirmationBeforeErasing) {
    runner().submitText(GetParam());
    EXPECT_TRUE(runner().isActive());
    EXPECT_TRUE(runner().isSelecting());
    EXPECT_EQ(runner().promptText(), "Select objects:");
    EXPECT_FALSE(editor().inTransaction());
    runner().pointerMoved({100.0, 100.0});
    EXPECT_TRUE(runner().preview().empty());
    EXPECT_TRUE(messages().empty());
    select(0);
    select(1);
    EXPECT_EQ(document().entityCount(), 3U);
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_EQ(notifications(), 0U);
    EXPECT_EQ(messages(), (std::vector<std::string>{"1 found, 1 total", "1 found, 2 total"}));
    runner().submitText("");
    EXPECT_EQ(document().entityCount(), 1U);
    expectOriginal(2);
    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_EQ(messages().size(), 2U);
    EXPECT_EQ(editor().undoName(), "Erase");
    EXPECT_EQ(notifications(), 1U);
}

INSTANTIATE_TEST_SUITE_P(NamesAndAliases, EraseCommandNamesTest,
                         testing::Values("ERASE", "E", "erase", "e"));

TEST_F(EraseCommandTest, RegisteredCommandDeclaresSelectionAndHasNoPreview) {
    const auto command = registry().create("ERASE");
    ASSERT_NE(command, nullptr);
    EXPECT_TRUE(command->needsSelection());
    EXPECT_EQ(command->prompt(), (Prompt{"Select objects:", {}}));
    command->pointerMoved({10.0, 20.0});
    EXPECT_TRUE(command->preview().empty());
    EXPECT_EQ(registry().canonicalName("e"), "ERASE");
}

TEST_F(EraseCommandTest, ConfirmingWithoutSelectionLeavesHistoryUnchanged) {
    for (const bool typed : {false, true}) {
        runner().submitText("E");
        if (typed) {
            runner().submitText("");
        } else {
            runner().submit(ConfirmInput{});
        }
        EXPECT_FALSE(runner().isActive());
        EXPECT_FALSE(editor().inTransaction());
        EXPECT_EQ(document().entityCount(), 3U);
        EXPECT_EQ(editor().undoName(), "Seed drawing");
        EXPECT_EQ(notifications(), 0U);
        EXPECT_TRUE(messages().empty());
    }
}

TEST_F(EraseCommandTest, EscAbandonsSelectionWithoutEditing) {
    runner().submitText("ERASE");
    select(1);
    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_EQ(document().entityCount(), 3U);
    expectOriginal(1);
    EXPECT_EQ(editor().undoName(), "Seed drawing");
    EXPECT_EQ(notifications(), 0U);
    EXPECT_EQ(messages(), (std::vector<std::string>{"1 found, 1 total", "*Cancel*"}));
}

TEST_F(EraseCommandTest, UndoRestoresOriginalRecordsAndRedoErasesTheWholeSelection) {
    select(2);
    select(0);
    select(1);
    runner().submitText("E");
    ASSERT_TRUE(document().isEmpty());
    runner().submitText("U");
    ASSERT_EQ(document().entityCount(), 3U);
    for (std::size_t index = 0; index < 3; ++index) {
        expectOriginal(index);
    }
    EXPECT_EQ(editor().undoName(), "Seed drawing");
    EXPECT_EQ(editor().redoName(), "Erase");
    EXPECT_EQ(messages().back(), "Undone: Erase.");
    EXPECT_TRUE(runner().selection().isEmpty());
    runner().submitText("REDO");
    EXPECT_TRUE(document().isEmpty());
    EXPECT_EQ(editor().undoName(), "Erase");
    EXPECT_EQ(messages().back(), "Redone: Erase.");
    EXPECT_EQ(notifications(), 3U);
    EXPECT_FALSE(editor().inTransaction());
}

TEST_F(EraseCommandTest, EmptyLineRepeatsEraseAndAsksForANewSelection) {
    select(0);
    runner().submitText("E");
    runner().submitText("");
    ASSERT_TRUE(runner().isActive());
    EXPECT_EQ(runner().promptText(), "Select objects:");
    EXPECT_FALSE(editor().inTransaction());
    select(1);
    runner().submit(ConfirmInput{});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 1U);
    expectOriginal(2);
    runner().submitText("U");
    expectOriginal(1);
    EXPECT_EQ(document().find(original(0).id), nullptr);
}

TEST_F(EraseCommandTest, ConfirmInputRepeatsEraseWithTheNewPreselection) {
    select(0);
    runner().submitText("ERASE");
    select(2);
    runner().submit(ConfirmInput{});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 1U);
    expectOriginal(1);
    EXPECT_TRUE(messages().empty());
}

TEST_F(EraseCommandTest, WindowSelectionErasesOnlyEnclosedEntities) {
    runner().submitText("E");
    runner().click({-5.0, -10.0}, PickAction::Add);
    runner().pointerMoved({15.0, 25.0});
    ASSERT_TRUE(runner().selectionRectangle().has_value());
    EXPECT_TRUE(runner().preview().empty());
    runner().click({15.0, 25.0}, PickAction::Add);
    ASSERT_EQ(runner().selection().size(), 2U);
    runner().submit(ConfirmInput{});
    EXPECT_EQ(document().entityCount(), 1U);
    expectOriginal(2);
    EXPECT_EQ(messages(), (std::vector<std::string>{"2 found, 2 total"}));
}

TEST_F(EraseCommandTest, RemovedAndDuplicatePicksDoNotEraseExtraEntities) {
    runner().submitText("ERASE");
    select(0);
    select(0);
    select(1);
    select(0, PickAction::Remove);
    runner().submitText("");
    EXPECT_EQ(document().entityCount(), 2U);
    expectOriginal(0);
    expectOriginal(2);
    EXPECT_EQ(document().find(original(1).id), nullptr);
}

TEST_F(EraseCommandTest, StaleIdsAreSkippedAndRemainingEntitiesAreErased) {
    select(0);
    select(2);
    editor().beginTransaction("External removal");
    ASSERT_TRUE(editor().remove(original(0).id));
    editor().commit();
    runner().submitText("ERASE");
    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(editor().inTransaction());
    ASSERT_EQ(document().entityCount(), 1U);
    expectOriginal(1);
    EXPECT_TRUE(messages().empty());
    runner().submitText("U");
    expectOriginal(2);
    EXPECT_EQ(document().find(original(0).id), nullptr);
    EXPECT_EQ(editor().undoName(), "External removal");
}

TEST_F(EraseCommandTest, EntirelyStaleSelectionCreatesNoUndoStepAndPreservesRedo) {
    select(0);
    editor().beginTransaction("External removal");
    ASSERT_TRUE(editor().remove(original(0).id));
    editor().commit();
    editor().beginTransaction("Temporary addition");
    const core::EntityId temporary = editor().add(core::Polyline{});
    editor().commit();
    ASSERT_TRUE(editor().undo());
    runner().submitText("E");
    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(editor().undoName(), "External removal");
    EXPECT_EQ(editor().redoName(), "Temporary addition");
    EXPECT_TRUE(editor().canRedo());
    EXPECT_TRUE(messages().empty());
    runner().submitText("REDO");
    EXPECT_NE(document().find(temporary), nullptr);
}

TEST_F(EraseCommandTest, EmptyAndCancelledErasePreserveRedo) {
    select(0);
    runner().submitText("E");
    runner().submitText("U");
    for (const bool cancel : {false, true}) {
        runner().submitText("ERASE");
        if (cancel) {
            select(1);
            runner().submit(CancelInput{});
        } else {
            runner().submitText("");
        }
        EXPECT_TRUE(editor().canRedo());
        EXPECT_EQ(editor().redoName(), "Erase");
    }
    runner().submitText("REDO");
    EXPECT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(document().find(original(0).id), nullptr);
    expectOriginal(1);
    expectOriginal(2);
}

}  // namespace
}  // namespace tg::cmd
