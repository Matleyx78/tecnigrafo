// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <vector>

namespace tg::edit {
namespace {

using core::Document;
using core::Entity;
using core::EntityId;
using core::EntityRecord;
using core::Polyline;
using core::Vec2;

/// A two-point polyline from the origin to (length, 0): different lengths
/// give entities that are easy to tell apart.
Polyline segment(double length) {
    Polyline polyline;
    polyline.addVertex(Vec2{0.0, 0.0});
    polyline.addVertex(Vec2{length, 0.0});
    return polyline;
}

/// The entity with identifier `id`, or std::nullopt if the document has none.
std::optional<Entity> entityOf(const Document& document, EntityId id) {
    const EntityRecord* record = document.find(id);
    return record != nullptr ? std::optional<Entity>{record->entity} : std::nullopt;
}

std::vector<EntityId> idsOf(const Document& document) {
    std::vector<EntityId> ids;
    for (const EntityRecord& record : document.entities()) {
        ids.push_back(record.id);
    }
    return ids;
}

/// Adds one entity in its own committed transaction.
EntityId addCommitted(Editor& editor, double length, const std::string& name = "Setup") {
    editor.beginTransaction(name);
    const EntityId id = editor.add(segment(length));
    editor.commit();
    return id;
}

TEST(Editor, StartsWithNothingToUndoOrRedo) {
    Document document;
    const Editor editor(document);

    EXPECT_EQ(&editor.document(), &document);
    EXPECT_FALSE(editor.inTransaction());
    EXPECT_FALSE(editor.canUndo());
    EXPECT_FALSE(editor.canRedo());
    EXPECT_EQ(editor.undoName(), "");
    EXPECT_EQ(editor.redoName(), "");
}

TEST(Editor, CommitKeepsChangesAndCreatesAnUndoStep) {
    Document document;
    Editor editor(document);

    editor.beginTransaction("Polyline");
    EXPECT_TRUE(editor.inTransaction());
    const EntityId first = editor.add(segment(1.0));
    const EntityId second = editor.add(segment(2.0));
    EXPECT_EQ(document.entityCount(), 2U);  // changes are visible at once
    editor.commit();

    EXPECT_FALSE(editor.inTransaction());
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{first, second}));
    EXPECT_TRUE(editor.canUndo());
    EXPECT_FALSE(editor.canRedo());
    EXPECT_EQ(editor.undoName(), "Polyline");
}

TEST(Editor, UndoAndRedoAMixOfAddsAndRemoves) {
    Document document;
    Editor editor(document);
    const EntityId kept = addCommitted(editor, 1.0);
    const EntityId erased = addCommitted(editor, 2.0);

    editor.beginTransaction("Mix");
    const EntityId added = editor.add(segment(3.0));
    EXPECT_TRUE(editor.remove(erased));
    const EntityId addedToo = editor.add(segment(4.0));
    editor.commit();
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{kept, added, addedToo}));

    EXPECT_TRUE(editor.undo());
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{kept, erased}));
    EXPECT_EQ(entityOf(document, erased), Entity{segment(2.0)});

    EXPECT_TRUE(editor.redo());
    EXPECT_EQ(document.find(erased), nullptr);
    EXPECT_EQ(document.entityCount(), 3U);
    EXPECT_EQ(entityOf(document, added), Entity{segment(3.0)});
    EXPECT_EQ(entityOf(document, addedToo), Entity{segment(4.0)});
}

TEST(Editor, RedoRestoresTheSameIds) {
    Document document;
    Editor editor(document);

    editor.beginTransaction("Polyline");
    const EntityId first = editor.add(segment(1.0));
    const EntityId second = editor.add(segment(2.0));
    editor.commit();

    ASSERT_TRUE(editor.undo());
    EXPECT_TRUE(document.isEmpty());
    ASSERT_TRUE(editor.redo());

    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{first, second}));
    EXPECT_EQ(entityOf(document, first), Entity{segment(1.0)});
    EXPECT_EQ(entityOf(document, second), Entity{segment(2.0)});
}

TEST(Editor, UndoAndRedoWalkThroughSeveralSteps) {
    Document document;
    Editor editor(document);
    const EntityId first = addCommitted(editor, 1.0, "First");
    const EntityId second = addCommitted(editor, 2.0, "Second");

    ASSERT_TRUE(editor.undo());
    ASSERT_TRUE(editor.undo());
    EXPECT_TRUE(document.isEmpty());
    EXPECT_FALSE(editor.undo());

    ASSERT_TRUE(editor.redo());
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{first}));
    ASSERT_TRUE(editor.redo());
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{first, second}));
    EXPECT_FALSE(editor.redo());
}

TEST(Editor, RollbackLeavesNoTrace) {
    Document document;
    Editor editor(document);
    const EntityId first = addCommitted(editor, 1.0);
    const EntityId second = addCommitted(editor, 2.0);
    ASSERT_TRUE(editor.undo());  // leaves a redo step to preserve

    editor.beginTransaction("Discarded");
    editor.add(segment(3.0));
    EXPECT_TRUE(editor.remove(first));
    editor.rollback();

    EXPECT_FALSE(editor.inTransaction());
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{first}));
    EXPECT_EQ(entityOf(document, first), Entity{segment(1.0)});
    EXPECT_EQ(editor.undoName(), "Setup");
    EXPECT_TRUE(editor.canRedo());

    ASSERT_TRUE(editor.redo());
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{first, second}));
}

TEST(Editor, RollbackOfAnEntityAddedAndRemovedInTheSameTransaction) {
    Document document;
    Editor editor(document);

    editor.beginTransaction("Scratch");
    const EntityId id = editor.add(segment(1.0));
    EXPECT_TRUE(editor.remove(id));
    editor.rollback();

    EXPECT_TRUE(document.isEmpty());
    EXPECT_FALSE(editor.canUndo());
}

TEST(Editor, UndoOfAnEntityAddedAndRemovedInTheSameTransaction) {
    Document document;
    Editor editor(document);
    const EntityId kept = addCommitted(editor, 1.0);

    editor.beginTransaction("Scratch");
    const EntityId temporary = editor.add(segment(2.0));
    EXPECT_TRUE(editor.remove(temporary));
    EXPECT_TRUE(editor.remove(kept));
    editor.commit();
    EXPECT_TRUE(document.isEmpty());

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{kept}));
    ASSERT_TRUE(editor.redo());
    EXPECT_TRUE(document.isEmpty());
}

TEST(Editor, EmptyCommitCreatesNoStepAndKeepsRedo) {
    Document document;
    Editor editor(document);
    addCommitted(editor, 1.0);
    ASSERT_TRUE(editor.undo());

    editor.beginTransaction("Nothing");
    editor.commit();

    EXPECT_FALSE(editor.canUndo());
    EXPECT_TRUE(editor.canRedo());
    EXPECT_EQ(editor.redoName(), "Setup");
}

TEST(Editor, RemovingAMissingEntityRecordsNothing) {
    Document document;
    Editor editor(document);
    const EntityId id = addCommitted(editor, 1.0);

    editor.beginTransaction("Erase");
    EXPECT_FALSE(editor.remove(EntityId{id.value + 1}));
    EXPECT_FALSE(editor.remove(EntityId{}));
    editor.commit();

    EXPECT_EQ(document.entityCount(), 1U);
    EXPECT_EQ(editor.undoName(), "Setup");  // "Erase" changed nothing
}

TEST(Editor, NewCommitAfterUndoClearsRedo) {
    Document document;
    Editor editor(document);
    const EntityId undone = addCommitted(editor, 1.0, "First");
    ASSERT_TRUE(editor.undo());
    ASSERT_TRUE(editor.canRedo());

    const EntityId fresh = addCommitted(editor, 2.0, "Second");

    EXPECT_FALSE(editor.canRedo());
    EXPECT_EQ(editor.redoName(), "");
    EXPECT_FALSE(editor.redo());
    EXPECT_NE(fresh, undone);  // identifiers are never reused
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{fresh}));
}

TEST(Editor, UndoAndRedoAreRefusedWhileATransactionIsOpen) {
    Document document;
    Editor editor(document);
    const EntityId first = addCommitted(editor, 1.0);
    addCommitted(editor, 2.0);
    ASSERT_TRUE(editor.undo());

    editor.beginTransaction("Busy");
    EXPECT_FALSE(editor.canUndo());
    EXPECT_FALSE(editor.canRedo());
    EXPECT_FALSE(editor.undo());
    EXPECT_FALSE(editor.redo());
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{first}));
    editor.rollback();

    EXPECT_TRUE(editor.canUndo());
    EXPECT_TRUE(editor.canRedo());
}

TEST(Editor, StepNamesFollowTheStacks) {
    Document document;
    Editor editor(document);
    addCommitted(editor, 1.0, "Polyline");
    addCommitted(editor, 2.0, "Erase");
    EXPECT_EQ(editor.undoName(), "Erase");
    EXPECT_EQ(editor.redoName(), "");

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(editor.undoName(), "Polyline");
    EXPECT_EQ(editor.redoName(), "Erase");

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(editor.undoName(), "");
    EXPECT_EQ(editor.redoName(), "Polyline");

    ASSERT_TRUE(editor.redo());
    EXPECT_EQ(editor.undoName(), "Polyline");
    EXPECT_EQ(editor.redoName(), "Erase");
}

TEST(Editor, ListenerIsCalledAfterEveryChangeOfState) {
    Document document;
    Editor editor(document);
    int calls = 0;
    std::size_t countSeen = 0;
    editor.setChangeListener([&] {
        ++calls;
        countSeen = editor.document().entityCount();  // state is already final
    });

    editor.beginTransaction("Polyline");
    editor.add(segment(1.0));
    EXPECT_EQ(calls, 0);  // no call inside a transaction
    editor.commit();
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(countSeen, 1U);

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(calls, 2);
    EXPECT_EQ(countSeen, 0U);

    ASSERT_TRUE(editor.redo());
    EXPECT_EQ(calls, 3);
    EXPECT_EQ(countSeen, 1U);

    editor.beginTransaction("Discarded");
    editor.add(segment(2.0));
    editor.rollback();
    EXPECT_EQ(calls, 4);
    EXPECT_EQ(countSeen, 1U);

    editor.beginTransaction("Nothing");
    editor.commit();
    EXPECT_EQ(calls, 5);

    ASSERT_FALSE(editor.redo());  // nothing to redo: no call
    EXPECT_EQ(calls, 5);

    editor.setChangeListener({});
    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(calls, 5);
}

TEST(EditorDeathTest, TransactionPreconditionsAreChecked) {
#ifdef NDEBUG
    GTEST_SKIP() << "preconditions are checked with assert, disabled in this build";
#endif
    Document document;
    Editor editor(document);

    EXPECT_DEATH(editor.add(segment(1.0)), "open transaction");
    EXPECT_DEATH(editor.commit(), "open transaction");
    EXPECT_DEATH(editor.rollback(), "open transaction");

    editor.beginTransaction("Open");
    EXPECT_DEATH(editor.beginTransaction("Again"), "already open");
}

}  // namespace
}  // namespace tg::edit
