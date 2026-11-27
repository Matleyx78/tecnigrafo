// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "attribute_dialogs.hpp"

#include "cad_view.hpp"
#include "command_line.hpp"
#include "document_session.hpp"
#include "main_window.hpp"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QTimer>
#include <gtest/gtest.h>

#include <cmath>
#include <tuple>
#include <utility>

namespace tg::app {
namespace {

TEST(AttributeDefinitionDialogTest, ReturnsTheThreeEditedFieldsAndKeepsEverythingElse) {
    core::AttributeDefinition original;
    original.tag = "PART";
    original.prompt = "Part number";
    original.text.value = "A-1";
    original.text.height = 2.5;
    original.lockPosition = true;
    const AttributeDefinitionDialog dialog(original);

    dialog.findChild<QLineEdit*>(QStringLiteral("attributeTag"))->setText(QStringLiteral("CODE"));
    dialog.findChild<QLineEdit*>(QStringLiteral("attributePrompt"))
        ->setText(QStringLiteral("Code"));
    dialog.findChild<QLineEdit*>(QStringLiteral("attributeDefault"))
        ->setText(QStringLiteral("B-2"));

    const core::AttributeDefinition edited = dialog.definition();
    EXPECT_EQ(edited.tag, "CODE");
    EXPECT_EQ(edited.prompt, "Code");
    EXPECT_EQ(edited.text.value, "B-2");
    EXPECT_EQ(edited.text.height, 2.5);
    EXPECT_TRUE(edited.lockPosition);
}

TEST(InsertAttributesDialogTest, ListsAttributesInOrderAndReturnsTheirValues) {
    core::Insert insert;
    for (const auto& [tag, value] :
         {std::pair{"FIRST", "one"}, {"SECOND", "two"}, {"THIRD", "three"}}) {
        core::Attribute attribute;
        attribute.tag = tag;
        attribute.text.value = value;
        insert.attributes.push_back(attribute);
    }
    const InsertAttributesDialog dialog(insert);

    const auto edits = dialog.findChildren<QLineEdit*>();
    ASSERT_EQ(edits.size(), 3);
    edits[1]->setText(QStringLiteral("changed"));

    EXPECT_EQ(dialog.values(), (std::vector<std::string>{"one", "changed", "three"}));
}

TEST(InsertAttributesDialogTest, DoubleClickEditsThePickedInsertAndSupportsUndoAndCancel) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.resize(800, 600);
    window.show();
    DocumentSession* session = window.activeSession();
    ASSERT_NE(session, nullptr);

    session->editor().beginTransaction("Setup");
    core::EntityId firstId;
    core::EntityId secondId;
    for (const auto& [x, third] : {std::pair{0.0, "first"}, std::pair{20.0, "second"}}) {
        core::Insert insert;
        insert.block = "ATTRS";
        insert.insertion = {x, 0.0};
        for (const auto& [tag, value, y] :
             {std::tuple{"NUMBER", "1", 0.0}, std::tuple{"TYPE", "door", -2.0},
              std::tuple{"NOTE", third, -4.0}}) {
            core::Attribute attribute;
            attribute.tag = tag;
            attribute.text.value = value;
            attribute.text.height = 1.0;
            attribute.text.position = {x, y};
            insert.attributes.push_back(attribute);
        }
        const core::EntityId id = session->editor().add(insert);
        if (x == 0.0) {
            firstId = id;
        } else {
            secondId = id;
        }
    }
    session->editor().commit();
    window.view()->setView({10.0, -2.0}, 0.1);

    const auto doubleClickSecond = [&window] {
        const core::Vec2 screen = window.view()->viewport().worldToScreen({20.0, 0.0});
        const QPointF local{screen.x, screen.y};
        QMouseEvent event(QEvent::MouseButtonDblClick, local, window.view()->mapToGlobal(local),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(window.view(), &event);
    };
    const auto values = [session](core::EntityId id) {
        std::vector<std::string> result;
        const auto& insert = std::get<core::Insert>(session->document().find(id)->entity);
        for (const core::Attribute& attribute : insert.attributes) {
            result.push_back(attribute.text.value);
        }
        return result;
    };

    QTimer::singleShot(0, &window, [&window] {
        auto* dialog = window.findChild<InsertAttributesDialog*>();
        ASSERT_NE(dialog, nullptr);
        dialog->findChild<QLineEdit*>(QStringLiteral("attributeValue1"))
            ->setText(QStringLiteral("window"));
        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
    });
    doubleClickSecond();

    EXPECT_EQ(values(firstId), (std::vector<std::string>{"1", "door", "first"}));
    EXPECT_EQ(values(secondId), (std::vector<std::string>{"1", "window", "second"}));
    EXPECT_EQ(session->runner().promptText(), "Select an annotation object or [Undo/Mode]:");
    session->runner().submitText("U");
    EXPECT_EQ(values(secondId), (std::vector<std::string>{"1", "door", "second"}));
    session->runner().submit(cmd::ConfirmInput{});

    QTimer::singleShot(0, &window, [&window] {
        auto* dialog = window.findChild<InsertAttributesDialog*>();
        ASSERT_NE(dialog, nullptr);
        dialog->findChild<QLineEdit*>(QStringLiteral("attributeValue1"))
            ->setText(QStringLiteral("discarded"));
        dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Cancel)->click();
    });
    doubleClickSecond();
    EXPECT_EQ(values(secondId), (std::vector<std::string>{"1", "door", "second"}));
}

}  // namespace
}  // namespace tg::app
