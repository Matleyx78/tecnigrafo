// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Editor::undoNames() and Editor::redoNames(), added for the report of the
// control mode (docs/tappe/0.4-entita-base.md, P1).

#include "tg/core/document.hpp"
#include "tg/core/polyline.hpp"
#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace tg::edit {
namespace {

using Names = std::vector<std::string>;

void addStep(Editor& editor, const std::string& name) {
    editor.beginTransaction(name);
    editor.add(core::Polyline{{{{0.0, 0.0}, 0.0}, {{1.0, 0.0}, 0.0}}});
    editor.commit();
}

TEST(EditorNames, EmptyWithoutSteps) {
    core::Document document;
    const Editor editor(document);
    EXPECT_TRUE(editor.undoNames().empty());
    EXPECT_TRUE(editor.redoNames().empty());
}

TEST(EditorNames, OldestFirstAndTheNextStepLast) {
    core::Document document;
    Editor editor(document);
    addStep(editor, "First");
    addStep(editor, "Second");
    addStep(editor, "Third");
    EXPECT_EQ(editor.undoNames(), (Names{"First", "Second", "Third"}));
    EXPECT_EQ(editor.undoNames().back(), editor.undoName());
    EXPECT_TRUE(editor.redoNames().empty());

    ASSERT_TRUE(editor.undo());
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(editor.undoNames(), (Names{"First"}));
    // The most recently undone step is the next to redo.
    EXPECT_EQ(editor.redoNames(), (Names{"Third", "Second"}));
    EXPECT_EQ(editor.redoNames().back(), editor.redoName());

    ASSERT_TRUE(editor.redo());
    EXPECT_EQ(editor.undoNames(), (Names{"First", "Second"}));
    EXPECT_EQ(editor.redoNames(), (Names{"Third"}));
}

TEST(EditorNames, ANewStepClearsTheRedoNames) {
    core::Document document;
    Editor editor(document);
    addStep(editor, "First");
    ASSERT_TRUE(editor.undo());
    addStep(editor, "Other");
    EXPECT_EQ(editor.undoNames(), (Names{"Other"}));
    EXPECT_TRUE(editor.redoNames().empty());
}

TEST(EditorNames, EmptyAndRolledBackTransactionsLeaveNoName) {
    core::Document document;
    Editor editor(document);
    editor.beginTransaction("Empty");
    editor.commit();
    editor.beginTransaction("Abandoned");
    editor.add(core::Polyline{});
    editor.rollback();
    EXPECT_TRUE(editor.undoNames().empty());
}

}  // namespace
}  // namespace tg::edit
