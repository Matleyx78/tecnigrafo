// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "battman_dialog.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/insert.hpp"
#include "tg/edit/blocks.hpp"
#include "tg/edit/editor.hpp"

#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace tg::app {
namespace {

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
    EXPECT_EQ(m_editor.undoName(), "Battman");
    ASSERT_TRUE(m_editor.undo());
    m_dialog->refresh();
    EXPECT_EQ(tags(), (std::vector<std::string>{"FIRST", "SECOND"}));
}

TEST_F(BattmanDialogTest, EditsAndRemovesDefinitionsWithUndo) {
    table()->selectRow(0);
    QTimer::singleShot(0, m_dialog.get(), [this] {
        auto* edit = m_dialog->findChild<QDialog*>();
        ASSERT_NE(edit, nullptr);
        auto* prompt = edit->findChild<QLineEdit*>(QStringLiteral("battmanEditPrompt"));
        ASSERT_NE(prompt, nullptr);
        prompt->setText(QStringLiteral("Changed prompt"));
        edit->accept();
    });
    button("battmanEdit")->click();
    const core::BlockDefinition* block = m_document.blocks().find("Part");
    ASSERT_NE(block, nullptr);
    EXPECT_EQ(std::get<core::AttributeDefinition>(block->entities[0].entity).prompt,
              "Changed prompt");
    EXPECT_TRUE(m_dialog->hasUnsynchronizedChanges());
    ASSERT_TRUE(m_editor.undo());
    m_dialog->refresh();

    table()->selectRow(1);
    button("battmanRemove")->click();
    EXPECT_EQ(tags(), (std::vector<std::string>{"FIRST"}));
    EXPECT_EQ(m_editor.undoName(), "Battman");
    ASSERT_TRUE(m_editor.undo());
    m_dialog->refresh();
    EXPECT_EQ(tags(), (std::vector<std::string>{"FIRST", "SECOND"}));
}

TEST_F(BattmanDialogTest, SynchronizesValuesInNewOrderAndUndoRestoresInsert) {
    table()->selectRow(1);
    button("battmanUp")->click();
    button("battmanSynchronize")->click();
    ASSERT_EQ(m_document.entities().size(), 1U);
    const auto& insert = std::get<core::Insert>(m_document.entities().front().entity);
    ASSERT_EQ(insert.attributes.size(), 2U);
    EXPECT_EQ(insert.attributes[0].tag, "SECOND");
    EXPECT_EQ(insert.attributes[0].text.value, "two");
    EXPECT_EQ(insert.attributes[1].tag, "FIRST");
    EXPECT_EQ(insert.attributes[1].text.value, "one");
    EXPECT_FALSE(m_dialog->hasUnsynchronizedChanges());
    EXPECT_EQ(m_editor.undoName(), "Battman");
    ASSERT_TRUE(m_editor.undo());
    const auto& restored = std::get<core::Insert>(m_document.entities().front().entity);
    EXPECT_EQ(restored.attributes[0].tag, "FIRST");
    EXPECT_EQ(restored.attributes[1].tag, "SECOND");
}

TEST_F(BattmanDialogTest, ClosingAsksAboutUnsynchronizedChanges) {
    table()->selectRow(1);
    button("battmanUp")->click();
    int questions = 0;
    m_dialog->setConfirm([&questions](const QString&) {
        ++questions;
        return QMessageBox::No;
    });
    m_dialog->reject();
    EXPECT_EQ(questions, 1);
    EXPECT_EQ(m_dialog->result(), QDialog::Rejected);
}

}  // namespace
}  // namespace tg::app
