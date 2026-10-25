// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <optional>
#include <string>
#include <utility>
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

TEST(Editor, UnitsCanBeUndoneRedoneAndRolledBackWithEntities) {
    Document document;
    Editor editor(document);
    editor.beginTransaction("Units and polyline");
    const EntityId id = editor.add(segment(1.0));
    EXPECT_TRUE(editor.setUnits(0));
    EXPECT_EQ(document.units(), 0);
    EXPECT_TRUE(editor.setUnits(Document::kMaxUnits));
    editor.commit();

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.units(), Document::kDefaultUnits);
    EXPECT_TRUE(document.isEmpty());
    ASSERT_TRUE(editor.redo());
    EXPECT_EQ(document.units(), Document::kMaxUnits);
    EXPECT_EQ(entityOf(document, id), Entity{segment(1.0)});

    editor.beginTransaction("Discard units");
    EXPECT_TRUE(editor.setUnits(1));
    EXPECT_TRUE(editor.setUnits(2));
    editor.rollback();
    EXPECT_EQ(document.units(), Document::kMaxUnits);
    EXPECT_EQ(editor.undoNames(), (std::vector<std::string>{"Units and polyline"}));
}

TEST(Editor, UnchangedUnitsCreateNoUndoStepOrModification) {
    Document document;
    Editor editor(document);
    editor.beginTransaction("Same units");
    EXPECT_FALSE(editor.setUnits(Document::kDefaultUnits));
    editor.commit();
    EXPECT_FALSE(editor.canUndo());
    EXPECT_FALSE(editor.isModified());
}

TEST(Editor, ModifiedStateFollowsTheSavedPositionInBothDirections) {
    Document document;
    Editor editor(document);
    EXPECT_FALSE(editor.isModified());
    EXPECT_EQ(editor.fileName(), "Drawing1.cadx");
    EXPECT_FALSE(editor.hasFile());

    addCommitted(editor, 1.0);
    EXPECT_TRUE(editor.isModified());
    editor.markSaved("pianta-caffè.cadx");
    EXPECT_FALSE(editor.isModified());
    EXPECT_TRUE(editor.hasFile());
    EXPECT_EQ(editor.fileName(), "pianta-caffè.cadx");

    addCommitted(editor, 2.0);
    EXPECT_TRUE(editor.isModified());
    ASSERT_TRUE(editor.undo());
    EXPECT_FALSE(editor.isModified());
    ASSERT_TRUE(editor.undo());
    EXPECT_TRUE(editor.isModified());
    ASSERT_TRUE(editor.redo());
    EXPECT_FALSE(editor.isModified());
    ASSERT_TRUE(editor.redo());
    EXPECT_TRUE(editor.isModified());
}

TEST(Editor, DiscardedSavedBranchCannotBeReachedAtTheSameStackDepth) {
    Document document;
    Editor editor(document);
    addCommitted(editor, 1.0);
    editor.markSaved("saved.cadx");
    ASSERT_TRUE(editor.undo());
    addCommitted(editor, 2.0);
    EXPECT_TRUE(editor.isModified());
    EXPECT_FALSE(editor.canRedo());
    ASSERT_TRUE(editor.undo());
    EXPECT_TRUE(editor.isModified());
    ASSERT_TRUE(editor.redo());
    EXPECT_TRUE(editor.isModified());
    editor.markSaved("branch.cadx");
    EXPECT_FALSE(editor.isModified());
}

TEST(Editor, BranchingAtOrAfterSavedPositionPreservesIt) {
    Document document;
    Editor editor(document);
    addCommitted(editor, 1.0);
    editor.markSaved("saved.cadx");
    addCommitted(editor, 2.0);
    addCommitted(editor, 3.0);
    ASSERT_TRUE(editor.undo());
    addCommitted(editor, 4.0);
    ASSERT_TRUE(editor.undo());
    EXPECT_TRUE(editor.isModified());
    ASSERT_TRUE(editor.undo());
    EXPECT_FALSE(editor.isModified());
    addCommitted(editor, 5.0);
    ASSERT_TRUE(editor.undo());
    EXPECT_FALSE(editor.isModified());
}

TEST(Editor, EmptyCommitAndRollbackPreserveSavedStateAndRedo) {
    Document document;
    Editor editor(document);
    addCommitted(editor, 1.0);
    editor.markSaved("saved.cadx");
    editor.beginTransaction("Empty at saved position");
    editor.commit();
    EXPECT_FALSE(editor.isModified());
    ASSERT_TRUE(editor.undo());
    editor.beginTransaction("Empty below saved position");
    EXPECT_FALSE(editor.setUnits(document.units()));
    editor.commit();
    EXPECT_TRUE(editor.isModified());
    editor.beginTransaction("Discarded");
    editor.setUnits(0);
    editor.add(segment(2.0));
    editor.rollback();
    EXPECT_TRUE(editor.isModified());
    ASSERT_TRUE(editor.redo());
    EXPECT_FALSE(editor.isModified());
}

TEST(Editor, SavingAfterUndoKeepsRedoAndMovesTheSavedPosition) {
    Document document;
    Editor editor(document);
    addCommitted(editor, 1.0);
    ASSERT_TRUE(editor.undo());
    editor.markSaved("empty.cadx");
    EXPECT_FALSE(editor.isModified());
    ASSERT_TRUE(editor.redo());
    EXPECT_TRUE(editor.isModified());
    ASSERT_TRUE(editor.undo());
    EXPECT_FALSE(editor.isModified());
}

TEST(Editor, ReplaceDocumentKeepsTheReferenceAndNotifiesAfterResettingState) {
    Document document;
    Editor editor(document);
    const Document& reference = editor.document();
    addCommitted(editor, 1.0);
    addCommitted(editor, 2.0);
    ASSERT_TRUE(editor.undo());

    Document opened;
    opened.setUnits(0);
    const EntityId id = opened.add(segment(3.0));
    int calls = 0;
    editor.setChangeListener([&] {
        ++calls;
        EXPECT_EQ(&editor.document(), &document);
        EXPECT_FALSE(editor.canUndo());
        EXPECT_FALSE(editor.canRedo());
        EXPECT_TRUE(editor.undoNames().empty());
        EXPECT_TRUE(editor.redoNames().empty());
        EXPECT_FALSE(editor.isModified());
        if (calls == 1) {
            EXPECT_TRUE(editor.hasFile());
            EXPECT_EQ(editor.fileName(), "opened.dxf");
            EXPECT_EQ(reference.units(), 0);
            EXPECT_EQ(entityOf(reference, id), Entity{segment(3.0)});
        } else {
            EXPECT_FALSE(editor.hasFile());
            EXPECT_EQ(editor.fileName(), "Drawing1.cadx");
            EXPECT_TRUE(reference.isEmpty());
            EXPECT_EQ(reference.units(), Document::kDefaultUnits);
        }
    });

    editor.replaceDocument(std::move(opened), "opened.dxf");
    EXPECT_EQ(calls, 1);
    editor.replaceDocument(Document{}, "");
    EXPECT_EQ(calls, 2);
}

TEST(EditorDeathTest, DrawingStatePreconditionsAreChecked) {
#ifdef NDEBUG
    GTEST_SKIP() << "preconditions are checked with assert, disabled in this build";
#endif
    Document document;
    Editor editor(document);
    EXPECT_DEATH(editor.setUnits(0), "open transaction");
    editor.beginTransaction("Units");
    EXPECT_DEATH(editor.setUnits(-1), "invalid units");
    EXPECT_DEATH(editor.setUnits(Document::kMaxUnits + 1), "invalid units");
    EXPECT_DEATH(editor.replaceDocument(Document{}, ""), "no open transaction");
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

// Counters intentionally are not compared: undo never reuses an allocated handle.
struct DrawingState {
    std::vector<core::EntityRecord> entities;
    std::vector<core::RawEntity> rawEntities;
    std::vector<core::Layer> layers;
    std::vector<core::Linetype> linetypes;
    core::RawDrawing raw;
    core::CurrentProperties current;
    double ltscale;
    int units;
    bool lwdisplay;

    friend bool operator==(const DrawingState&, const DrawingState&) = default;
};

DrawingState stateOf(const Document& doc) {
    return {{doc.entities().begin(), doc.entities().end()},
            {doc.rawEntities().begin(), doc.rawEntities().end()},
            {doc.layers().begin(), doc.layers().end()},
            {doc.linetypes().begin(), doc.linetypes().end()},
            doc.raw(),
            doc.current(),
            doc.ltscale(),
            doc.units(),
            doc.lwdisplay()};
}

core::Layer namedLayer(const Document& doc, const std::string& name) {
    core::Layer layer;
    layer.name = name;
    layer.linetype = doc.continuousLinetype();
    layer.extras = {{1000, "preserved"}};
    return layer;
}

void expectRoundTrip(Editor& editor, const DrawingState& before, const DrawingState& after) {
    for (int cycle = 0; cycle < 2; ++cycle) {
        ASSERT_TRUE(editor.undo());
        EXPECT_EQ(stateOf(editor.document()), before);
        ASSERT_TRUE(editor.redo());
        EXPECT_EQ(stateOf(editor.document()), after);
    }
}

TEST(Editor, LayerCreationRemovalAndTablePositionsRoundTripExactly) {
    Document doc = Document::newDrawing();
    Editor editor(doc);
    const auto before = stateOf(doc);
    editor.beginTransaction("Layers");
    const auto first = editor.addLayer(namedLayer(doc, "First"));
    const auto second = editor.addLayer(namedLayer(doc, "Second"));
    const auto third = editor.addLayer(namedLayer(doc, "Third"));
    ASSERT_TRUE(first && second && third);
    editor.commit();
    const auto added = stateOf(doc);
    expectRoundTrip(editor, before, added);
    editor.markSaved("layers.cadx");
    editor.beginTransaction("Remove middle");
    EXPECT_TRUE(editor.removeLayer(*second));
    editor.commit();
    const auto removed = stateOf(doc);
    EXPECT_EQ(doc.layers()[2].id, *third);
    expectRoundTrip(editor, added, removed);
    EXPECT_TRUE(editor.isModified());
    ASSERT_TRUE(editor.undo());
    EXPECT_FALSE(editor.isModified());
    editor.beginTransaction("Discard removal");
    EXPECT_TRUE(editor.removeLayer(*first));
    EXPECT_TRUE(editor.removeLayer(*third));
    editor.rollback();
    EXPECT_EQ(stateOf(doc), added);
}

TEST(Editor, RenameRestoresRawSpellingsAndPreservesOtherReferences) {
    Document doc = Document::newDrawing();
    const auto layer = doc.addLayer(namedLayer(doc, "Walls"));
    core::RawEntity hatch;
    hatch.id = EntityId{1000};
    hatch.order = 1;
    hatch.type = "HATCH";
    hatch.groups = {{5, "3E8"}, {8, "wALLS"}, {1001, "APP"}, {1000, "Walls"}};
    doc.insertRaw(hatch);
    core::RawDrawing raw;
    core::RawSection blocks;
    blocks.name = "BLOCKS";
    blocks.groups.push_back({0, "LINE"});
    blocks.groups.push_back({8, "WALLS"});
    blocks.groups.push_back({1001, "APP"});
    blocks.groups.push_back({1000, "Walls"});
    blocks.groups.push_back({0, "ARC"});
    blocks.groups.push_back({8, "walls"});
    blocks.groups.push_back({0, "LINE"});
    blocks.groups.push_back({8, "0"});
    raw.sections = {blocks, {"OBJECTS", {{1, "Walls"}}}};
    raw.nativeParts = {{"extensions.json", "{}"}};
    doc.setRaw(raw);
    Editor editor(doc);
    const auto before = stateOf(doc);
    editor.beginTransaction("Rename twice");
    EXPECT_TRUE(editor.renameLayer(layer, "Partitions"));
    EXPECT_TRUE(editor.renameLayer(layer, "Dividers"));
    editor.commit();
    EXPECT_EQ(doc.rawEntities()[0].groups[1].value, "Dividers");
    EXPECT_EQ(doc.rawEntities()[0].groups[3].value, "Walls");
    EXPECT_EQ(doc.raw().sections[0].groups[1].value, "Dividers");
    EXPECT_EQ(doc.raw().sections[0].groups[3].value, "Walls");
    EXPECT_EQ(doc.raw().sections[0].groups[5].value, "Dividers");
    EXPECT_EQ(doc.raw().sections[1], raw.sections[1]);
    const auto after = stateOf(doc);
    expectRoundTrip(editor, before, after);
    editor.beginTransaction("Discard rename");
    EXPECT_TRUE(editor.renameLayer(layer, "Other"));
    editor.rollback();
    EXPECT_EQ(stateOf(doc), after);
}

TEST(Editor, LayerStatesAndPropertiesPreserveIdentityNameAndExtras) {
    Document doc = Document::newDrawing();
    const auto id = doc.addLayer(namedLayer(doc, "Walls"));
    Editor editor(doc);
    const auto before = stateOf(doc);
    auto changed = *doc.layer(id);
    changed.name = "Ignored";
    changed.extras.clear();
    changed.color = core::Color::fromIndex(1);
    changed.lineweight = 50;
    changed.on = false;
    changed.frozen = true;
    changed.locked = true;
    changed.plot = false;
    editor.beginTransaction("States");
    EXPECT_TRUE(editor.changeLayer(changed));
    editor.commit();
    EXPECT_EQ(doc.layer(id)->name, "Walls");
    EXPECT_EQ(doc.layer(id)->extras, before.layers[1].extras);
    EXPECT_FALSE(doc.layer(id)->on);
    EXPECT_TRUE(doc.layer(id)->frozen);
    EXPECT_TRUE(doc.layer(id)->locked);
    EXPECT_FALSE(doc.layer(id)->plot);
    expectRoundTrip(editor, before, stateOf(doc));
}

TEST(Editor, LinetypeCurrentScalePropertiesAndEntitiesShareOneStep) {
    Document doc = Document::newDrawing();
    Editor editor(doc);
    const auto before = stateOf(doc);
    editor.beginTransaction("Mixed");
    core::Linetype linetype;
    linetype.name = "Custom";
    linetype.description = "Test pattern";
    linetype.extras = {{1000, "preserved"}};
    const auto type = editor.addLinetype(linetype);
    const auto layer = editor.addLayer(namedLayer(doc, "Walls"));
    ASSERT_TRUE(type && layer);
    auto current = doc.current();
    current.layer = *layer;
    current.linetype = *type;
    current.linetypeScale = 2.5;
    current.lineweight = 50;
    current.color = core::Color::fromIndex(3);
    EXPECT_TRUE(editor.setCurrent(current));
    const auto id = editor.add(segment(2));
    EXPECT_EQ(doc.find(id)->properties, current);
    auto properties = current;
    properties.color = core::Color::byBlock();
    EXPECT_TRUE(editor.setProperties(id, properties));
    EXPECT_TRUE(editor.setLtscale(3.5));
    EXPECT_TRUE(editor.setUnits(0));
    editor.commit();
    EXPECT_EQ(editor.undoNames(), (std::vector<std::string>{"Mixed"}));
    const auto after = stateOf(doc);
    expectRoundTrip(editor, before, after);
    editor.beginTransaction("Discard properties");
    EXPECT_TRUE(editor.setCurrent(before.current));
    EXPECT_TRUE(editor.setProperties(id, current));
    EXPECT_TRUE(editor.setLtscale(1.0));
    EXPECT_TRUE(editor.remove(id));
    editor.rollback();
    EXPECT_EQ(stateOf(doc), after);
}

TEST(Editor, ColourBookIsRemovedOnlyOnColourChangeAndRestoredExactly) {
    Document doc = Document::newDrawing();
    const auto id = doc.add(segment(1));
    auto record = doc.remove(id);
    record->extras.common = {{440, "1"}, {430, "Catalog$Colour"}, {60, "0"}, {430, "Other"}};
    record->extras.extended = {{1000, "preserved"}};
    doc.restore(*record);
    Editor editor(doc);
    const auto before = stateOf(doc);
    editor.beginTransaction("Scale only");
    auto properties = doc.find(id)->properties;
    properties.linetypeScale = 2;
    EXPECT_TRUE(editor.setProperties(id, properties));
    EXPECT_EQ(doc.find(id)->extras, record->extras);
    editor.rollback();
    EXPECT_EQ(stateOf(doc), before);
    editor.beginTransaction("Colour");
    properties.color = core::Color::fromRgb({12, 34, 56});
    EXPECT_TRUE(editor.setProperties(id, properties));
    editor.commit();
    EXPECT_EQ(doc.find(id)->extras.common, (core::RawGroups{{440, "1"}, {60, "0"}}));
    expectRoundTrip(editor, before, stateOf(doc));
}

TEST(Editor, RejectedAndUnchangedLayerEditsKeepRedoAndSavedState) {
    Document doc = Document::newDrawing();
    const auto layer = doc.addLayer(namedLayer(doc, "Walls"));
    auto frozen = namedLayer(doc, "Frozen");
    frozen.frozen = true;
    const auto frozenId = doc.addLayer(frozen);
    Editor editor(doc);
    addCommitted(editor, 1);
    ASSERT_TRUE(editor.undo());
    const auto before = stateOf(doc);
    editor.beginTransaction("Nothing");
    EXPECT_FALSE(editor.addLayer(namedLayer(doc, "walls")));
    EXPECT_FALSE(editor.addLayer(namedLayer(doc, "bad/name")));
    EXPECT_FALSE(editor.renameLayer(layer, "Walls"));
    EXPECT_FALSE(editor.renameLayer(layer, "0"));
    EXPECT_FALSE(editor.renameLayer({}, "Other"));
    EXPECT_FALSE(editor.renameLayer(doc.layerZero(), "Other"));
    EXPECT_FALSE(editor.removeLayer(doc.layerZero()));
    EXPECT_FALSE(editor.removeLayer({}));
    EXPECT_FALSE(editor.changeLayer(*doc.layer(layer)));
    auto zero = *doc.layer(doc.layerZero());
    zero.frozen = true;
    EXPECT_FALSE(editor.changeLayer(zero));
    auto missing = zero;
    missing.id = {};
    EXPECT_FALSE(editor.changeLayer(missing));
    EXPECT_FALSE(editor.setCurrent(doc.current()));
    auto current = doc.current();
    current.layer = frozenId;
    EXPECT_FALSE(editor.setCurrent(current));
    current.layer = {};
    EXPECT_FALSE(editor.setCurrent(current));
    EXPECT_FALSE(editor.setProperties({}, doc.current()));
    EXPECT_FALSE(editor.setLtscale(doc.ltscale()));
    for (double invalid : {0.0, -1.0, std::numeric_limits<double>::infinity(),
                           std::numeric_limits<double>::quiet_NaN()}) {
        EXPECT_FALSE(editor.setLtscale(invalid));
    }
    core::Linetype type;
    type.name = "continuous";
    EXPECT_FALSE(editor.addLinetype(type));
    type.name = "";
    EXPECT_FALSE(editor.addLinetype(type));
    editor.commit();
    EXPECT_EQ(stateOf(doc), before);
    EXPECT_FALSE(editor.canUndo());
    EXPECT_TRUE(editor.canRedo());
    EXPECT_FALSE(editor.isModified());
}

TEST(Editor, CurrentLayerMayBeOffOrLockedButNotRemovedOrFrozen) {
    Document doc = Document::newDrawing();
    const auto id = doc.addLayer(namedLayer(doc, "Current"));
    Editor editor(doc);
    const auto before = stateOf(doc);
    editor.beginTransaction("Current");
    auto current = doc.current();
    current.layer = id;
    EXPECT_TRUE(editor.setCurrent(current));
    auto layer = *doc.layer(id);
    layer.on = false;
    layer.locked = true;
    EXPECT_TRUE(editor.changeLayer(layer));
    EXPECT_FALSE(editor.removeLayer(id));
    layer.frozen = true;
    EXPECT_FALSE(editor.changeLayer(layer));
    const auto entity = editor.add(segment(1));
    EXPECT_EQ(doc.find(entity)->properties, current);
    editor.commit();
    expectRoundTrip(editor, before, stateOf(doc));
}

TEST(Editor, LwdisplayHasNoUndoStepPreservesRedoAndTracksSavedValue) {
    Document doc = Document::newDrawing();
    Editor editor(doc);
    int notifications = 0;
    editor.setChangeListener([&] { ++notifications; });
    editor.setLwdisplay(false);
    EXPECT_EQ(notifications, 0);
    editor.setLwdisplay(true);
    EXPECT_EQ(notifications, 1);
    EXPECT_TRUE(editor.isModified());
    EXPECT_FALSE(editor.canUndo());
    EXPECT_FALSE(editor.canRedo());
    editor.markSaved("view.cadx");
    EXPECT_FALSE(editor.isModified());
    addCommitted(editor, 1);
    ASSERT_TRUE(editor.undo());
    editor.setLwdisplay(false);
    EXPECT_TRUE(editor.canRedo());
    EXPECT_TRUE(editor.isModified());
    ASSERT_TRUE(editor.redo());
    EXPECT_FALSE(doc.lwdisplay());
    ASSERT_TRUE(editor.undo());
    EXPECT_TRUE(editor.isModified());
    editor.setLwdisplay(true);
    EXPECT_FALSE(editor.isModified());
    editor.beginTransaction("View setting is not transactional");
    editor.setLwdisplay(false);
    editor.rollback();
    EXPECT_FALSE(doc.lwdisplay());
    EXPECT_TRUE(editor.isModified());
    editor.replaceDocument(Document::newDrawing(), "");
    EXPECT_FALSE(editor.isModified());
}

TEST(Editor, OpenTransactionReportsPropertyModificationsAndRollbackClearsThem) {
    Document doc = Document::newDrawing();
    Editor editor(doc);
    editor.beginTransaction("Scale");
    EXPECT_TRUE(editor.setLtscale(2));
    EXPECT_TRUE(editor.isModified());
    editor.rollback();
    EXPECT_FALSE(editor.isModified());
}

TEST(EditorDeathTest, LayerAndPropertyEditsRequireATransaction) {
#ifdef NDEBUG
    GTEST_SKIP() << "preconditions are checked with assert, disabled in this build";
#endif
    Document doc = Document::newDrawing();
    Editor editor(doc);
    EXPECT_DEATH(editor.addLayer(namedLayer(doc, "Walls")), "open transaction");
    EXPECT_DEATH(editor.renameLayer({}, "Name"), "open transaction");
    EXPECT_DEATH(editor.removeLayer({}), "open transaction");
    EXPECT_DEATH(editor.changeLayer(*doc.layer(doc.layerZero())), "open transaction");
    EXPECT_DEATH(editor.addLinetype(core::Linetype{}), "open transaction");
    EXPECT_DEATH(editor.setProperties({}, doc.current()), "open transaction");
    EXPECT_DEATH(editor.setCurrent(doc.current()), "open transaction");
    EXPECT_DEATH(editor.setLtscale(2), "open transaction");
}

}  // namespace
}  // namespace tg::edit
