// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "battman_dialog.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/insert.hpp"
#include "tg/edit/blocks.hpp"
#include "tg/edit/editor.hpp"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <gtest/gtest.h>

#include <ranges>
#include <string>
#include <vector>

namespace tg::app {
namespace {

core::AttributeDefinition definition(const std::string& tag, const std::string& prompt,
                                     const std::string& value) {
    core::AttributeDefinition result;
    result.tag = tag;
    result.prompt = prompt;
    result.text.value = value;
    result.text.height = 1;
    return result;
}

core::Attribute attribute(std::uint64_t id, const std::string& tag, const std::string& value) {
    core::Attribute result;
    result.id = core::EntityId{id};
    result.tag = tag;
    result.text.value = value;
    return result;
}

std::vector<std::string> attributeTags(const core::Document& document, std::size_t index = 0) {
    const auto& insert = std::get<core::Insert>(document.entities()[index].entity);
    std::vector<std::string> result;
    result.reserve(insert.attributes.size());
    for (const core::Attribute& item : insert.attributes) {
        result.push_back(item.tag);
    }
    return result;
}

std::vector<std::string> attributeValues(const core::Document& document, std::size_t index = 0) {
    const auto& insert = std::get<core::Insert>(document.entities()[index].entity);
    std::vector<std::string> result;
    result.reserve(insert.attributes.size());
    for (const core::Attribute& item : insert.attributes) {
        result.push_back(item.text.value);
    }
    return result;
}

class BattmanDialogTest : public testing::Test {
protected:
    BattmanDialogTest() : m_document(core::Document::newDrawing()), m_editor(m_document) {
        core::BlockDefinition block{"Part"};
        core::AttributeDefinition first;
        first.tag = "FIRST";
        first.prompt = "First prompt";
        first.text.value = "one";
        first.text.position = {1, 0};
        core::AttributeDefinition second = first;
        second.tag = "SECOND";
        second.prompt = "Second prompt";
        second.text.value = "two";
        second.text.position = {2, 0};
        block.entities.push_back({core::EntityId{100}, first});
        block.entities.push_back({core::EntityId{101}, second});
        block.entities[0].order = 1;
        block.entities[1].order = 2;
        block.hasAttributes = true;
        m_document.replaceBlock("Part", block);

        m_editor.beginTransaction("Insert");
        static_cast<void>(edit::addInsert(m_editor, core::Insert{"Part", {0, 0}, 1, 1, 0, {}}));
        m_editor.commit();
        m_dialog = std::make_unique<BattmanDialog>(m_editor, "Part");
    }

    [[nodiscard]] QTableWidget* table() const {
        return m_dialog->findChild<QTableWidget*>(QStringLiteral("battmanDefinitions"));
    }

    [[nodiscard]] QPushButton* button(const char* name) const {
        return m_dialog->findChild<QPushButton*>(QString::fromLatin1(name));
    }

    [[nodiscard]] std::vector<std::string> tags() const {
        std::vector<std::string> result;
        const core::BlockDefinition* block = m_document.blocks().find("Part");
        if (block == nullptr) {
            return result;
        }
        for (const core::EntityRecord& record : block->entities) {
            if (const auto* definition = std::get_if<core::AttributeDefinition>(&record.entity)) {
                result.push_back(definition->tag);
            }
        }
        return result;
    }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] BattmanDialog& dialog() { return *m_dialog; }

private:
    core::Document m_document;
    edit::Editor m_editor;
    std::unique_ptr<BattmanDialog> m_dialog;
};

TEST_F(BattmanDialogTest, ListsDefinitionsAndMovesEachInAnUndoableStep) {
    ASSERT_EQ(table()->rowCount(), 2);
    EXPECT_EQ(table()->item(0, 0)->text(), QStringLiteral("FIRST"));
    EXPECT_EQ(table()->item(0, 1)->text(), QStringLiteral("First prompt"));
    EXPECT_EQ(table()->item(0, 2)->text(), QStringLiteral("one"));

    table()->selectRow(1);
    button("battmanUp")->click();
    EXPECT_EQ(tags(), (std::vector<std::string>{"SECOND", "FIRST"}));
    EXPECT_EQ(editor().undoName(), "Battman");
    ASSERT_TRUE(editor().undo());
    dialog().refresh();
    EXPECT_EQ(tags(), (std::vector<std::string>{"FIRST", "SECOND"}));
}

TEST_F(BattmanDialogTest, EditsAndRemovesDefinitionsWithUndo) {
    table()->selectRow(0);
    QTimer::singleShot(0, &dialog(), [this] {
        auto* edit = dialog().findChild<QDialog*>();
        ASSERT_NE(edit, nullptr);
        auto* prompt = edit->findChild<QLineEdit*>(QStringLiteral("battmanEditPrompt"));
        ASSERT_NE(prompt, nullptr);
        prompt->setText(QStringLiteral("Changed prompt"));
        edit->accept();
    });
    button("battmanEdit")->click();
    const core::BlockDefinition* block = document().blocks().find("Part");
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(std::get<core::AttributeDefinition>(block->entities[0].entity).prompt,
              "Changed prompt");
    EXPECT_TRUE(dialog().hasUnsynchronizedChanges());
    ASSERT_TRUE(editor().undo());
    dialog().refresh();

    table()->selectRow(1);
    button("battmanRemove")->click();
    EXPECT_EQ(tags(), (std::vector<std::string>{"FIRST"}));
    EXPECT_EQ(editor().undoName(), "Battman");
    ASSERT_TRUE(editor().undo());
    dialog().refresh();
    EXPECT_EQ(tags(), (std::vector<std::string>{"FIRST", "SECOND"}));
}

TEST_F(BattmanDialogTest, SynchronizesValuesInNewOrderAndUndoRestoresInsert) {
    table()->selectRow(1);
    button("battmanUp")->click();
    button("battmanSynchronize")->click();
    ASSERT_EQ(document().entities().size(), 1U);
    const auto synchronizedEntities = document().entities();
    const auto& insert = std::get<core::Insert>(synchronizedEntities.front().entity);
    ASSERT_EQ(insert.attributes.size(), 2U);
    EXPECT_EQ(insert.attributes[0].tag, "SECOND");
    EXPECT_EQ(insert.attributes[0].text.value, "two");
    EXPECT_EQ(insert.attributes[1].tag, "FIRST");
    EXPECT_EQ(insert.attributes[1].text.value, "one");
    EXPECT_FALSE(dialog().hasUnsynchronizedChanges());
    EXPECT_EQ(editor().undoName(), "Battman");
    ASSERT_TRUE(editor().undo());
    const auto restoredEntities = document().entities();
    const auto& restored = std::get<core::Insert>(restoredEntities.front().entity);
    EXPECT_EQ(restored.attributes[0].tag, "FIRST");
    EXPECT_EQ(restored.attributes[1].tag, "SECOND");
}

TEST_F(BattmanDialogTest, ClosingAsksAboutUnsynchronizedChanges) {
    table()->selectRow(1);
    button("battmanUp")->click();
    int questions = 0;
    dialog().setConfirm([&questions](const QString&) {
        ++questions;
        return QMessageBox::No;
    });
    dialog().reject();
    EXPECT_EQ(questions, 1);
    EXPECT_EQ(dialog().result(), QDialog::Rejected);
}

TEST(BattmanDialogBlocksTest, ClosingSynchronizesChangesMadeBeforeSwitchingBlocks) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition part{"Part"};
    part.hasAttributes = true;
    part.entities.push_back({core::EntityId{100}, definition("FIRST", "First", "one"), {}, 1});
    part.entities.push_back({core::EntityId{101}, definition("SECOND", "Second", "two"), {}, 2});
    document.replaceBlock("Part", part);
    core::BlockDefinition other{"Other"};
    other.hasAttributes = true;
    other.entities.push_back({core::EntityId{102}, definition("OTHER", "Other", "other"), {}, 1});
    document.replaceBlock("Other", other);

    edit::Editor editor(document);
    editor.beginTransaction("Inserts");
    static_cast<void>(edit::addInsert(editor, core::Insert{"Part", {}, 1, 1, 0, {}}));
    static_cast<void>(edit::addInsert(editor, core::Insert{"Other", {}, 1, 1, 0, {}}));
    editor.commit();
    BattmanDialog dialog(editor, "Part");
    auto* table = dialog.findChild<QTableWidget*>(QStringLiteral("battmanDefinitions"));
    auto* blocks = dialog.findChild<QComboBox*>(QStringLiteral("battmanBlocks"));
    ASSERT_NE(table, nullptr);
    ASSERT_NE(blocks, nullptr);
    table->selectRow(1);
    dialog.moveUp();
    blocks->setCurrentText(QStringLiteral("Other"));
    dialog.setConfirm([](const QString&) { return QMessageBox::Yes; });

    dialog.reject();

    EXPECT_EQ(attributeTags(document), (std::vector<std::string>{"SECOND", "FIRST"}));
    EXPECT_EQ(editor.undoName(), "Battman");
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(attributeTags(document), (std::vector<std::string>{"FIRST", "SECOND"}));
}

TEST(BattmanDialogHistoryTest, ReordersAndEditsWhileSynchronizationPreservesValues) {
    core::Document document = core::Document::newDrawing();
    core::BlockDefinition block{"BATT"};
    block.hasAttributes = true;
    block.entities.push_back({core::EntityId{100}, definition("TRE", "Tre", "T0"), {}, 1});
    block.entities.push_back({core::EntityId{101}, definition("DUE", "Due", "D0"), {}, 2});
    block.entities.push_back({core::EntityId{102}, definition("UNO", "Uno", "U0"), {}, 3});
    document.replaceBlock("BATT", block);
    edit::Editor editor(document);
    editor.beginTransaction("Insert");
    static_cast<void>(
        editor.add(core::Insert{"BATT",
                                {},
                                1,
                                1,
                                0,
                                {attribute(200, "TRE", "I1-3"), attribute(201, "DUE", "I1-2"),
                                 attribute(202, "UNO", "I1-1")}}));
    editor.commit();
    BattmanDialog dialog(editor, "BATT");
    auto* table = dialog.findChild<QTableWidget*>(QStringLiteral("battmanDefinitions"));
    ASSERT_NE(table, nullptr);

    const auto applyAndCheckUndo = [&document, &editor,
                                    &dialog](const std::function<void()>& operation) {
        const core::BlockDefinition beforeBlock = *document.blocks().find("BATT");
        const std::vector<core::EntityRecord> beforeEntities(document.entities().begin(),
                                                             document.entities().end());
        operation();
        EXPECT_EQ(editor.undoName(), "Battman");
        ASSERT_TRUE(editor.undo());
        EXPECT_EQ(*document.blocks().find("BATT"), beforeBlock);
        EXPECT_TRUE(std::ranges::equal(document.entities(), beforeEntities));
        ASSERT_TRUE(editor.redo());
        dialog.refresh();
    };
    table->selectRow(2);
    applyAndCheckUndo([&dialog] { dialog.moveUp(); });
    applyAndCheckUndo([&dialog] { dialog.moveUp(); });
    table->selectRow(2);
    applyAndCheckUndo([&dialog] { dialog.moveUp(); });
    EXPECT_EQ(table->item(0, 0)->text(), QStringLiteral("UNO"));
    EXPECT_EQ(table->item(1, 0)->text(), QStringLiteral("DUE"));
    EXPECT_EQ(table->item(2, 0)->text(), QStringLiteral("TRE"));

    applyAndCheckUndo([&dialog] { dialog.synchronize(); });
    EXPECT_EQ(attributeTags(document), (std::vector<std::string>{"UNO", "DUE", "TRE"}));
    EXPECT_EQ(attributeValues(document), (std::vector<std::string>{"I1-1", "I1-2", "I1-3"}));

    editor.beginTransaction("More inserts");
    static_cast<void>(
        editor.add(core::Insert{"BATT",
                                {1, 0},
                                1,
                                1,
                                0,
                                {attribute(210, "UNO", "I2-1"), attribute(211, "DUE", "I2-2"),
                                 attribute(212, "TRE", "I2-3")}}));
    static_cast<void>(
        editor.add(core::Insert{"BATT",
                                {2, 0},
                                1,
                                1,
                                0,
                                {attribute(220, "UNO", "I3-1"), attribute(221, "DUE", "I3-2"),
                                 attribute(222, "TRE", "I3-3")}}));
    editor.commit();
    table->selectRow(1);
    applyAndCheckUndo([&dialog] {
        QTimer::singleShot(0, &dialog, [&dialog] {
            auto* edit = dialog.findChild<QDialog*>();
            ASSERT_NE(edit, nullptr);
            auto* prompt = edit->findChild<QLineEdit*>(QStringLiteral("battmanEditPrompt"));
            ASSERT_NE(prompt, nullptr);
            prompt->setText(QStringLiteral("Due cambiato"));
            edit->accept();
        });
        dialog.editSelected();
    });
    const core::BlockDefinition* edited = document.blocks().find("BATT");
    ASSERT_NE(edited, nullptr);
    EXPECT_EQ(std::get<core::AttributeDefinition>(edited->entities[1].entity).prompt,
              "Due cambiato");

    applyAndCheckUndo([&dialog] { dialog.synchronize(); });
    for (std::size_t index = 0; index < 3; ++index) {
        EXPECT_EQ(attributeTags(document, index), (std::vector<std::string>{"UNO", "DUE", "TRE"}));
        const std::string number = std::to_string(index + 1);
        EXPECT_EQ(attributeValues(document, index),
                  (std::vector<std::string>{"I" + number + "-1", "I" + number + "-2",
                                            "I" + number + "-3"}));
    }
}

}  // namespace
}  // namespace tg::app
