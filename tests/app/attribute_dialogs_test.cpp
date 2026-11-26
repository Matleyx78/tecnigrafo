// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "attribute_dialogs.hpp"

#include <QLineEdit>
#include <gtest/gtest.h>

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

}  // namespace
}  // namespace tg::app
