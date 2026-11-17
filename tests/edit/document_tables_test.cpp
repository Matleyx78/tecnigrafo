// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/document_tables.hpp"

#include "tg/core/insert.hpp"
#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

namespace tg::edit {
namespace {

TEST(DocumentTables, LayerAndEntityShareOneHistoryStep) {
    auto document = core::Document::newDrawing();
    Editor editor(document);
    core::Layer layer;
    layer.name = "Walls";
    layer.linetype = document.continuousLinetype();

    editor.beginTransaction("Add layer and line");
    const auto layerId = editor.tables().addLayer(layer);
    if (!layerId) {
        ADD_FAILURE() << "Layer creation must succeed";
        return;
    }
    const auto entityId = editor.add(core::Line{{0.0, 0.0}, {1.0, 0.0}});
    auto properties = document.current();
    properties.layer = *layerId;
    ASSERT_TRUE(editor.setProperties(entityId, properties));
    editor.commit();

    ASSERT_TRUE(editor.undo());
    EXPECT_EQ(document.find(entityId), nullptr);
    EXPECT_EQ(document.layer(*layerId), nullptr);
    EXPECT_FALSE(editor.canUndo());

    ASSERT_TRUE(editor.redo());
    ASSERT_NE(document.layer(*layerId), nullptr);
    const auto* entity = document.find(entityId);
    ASSERT_NE(entity, nullptr);
    EXPECT_EQ(entity->properties.layer, *layerId);
    EXPECT_FALSE(editor.canRedo());
}

TEST(DocumentTables, UnchangedStyleTablesPreserveHistoryAndSavedState) {
    core::Document document;
    Editor editor(document);
    editor.beginTransaction("Setup");
    const auto id = editor.add(core::Line{{0.0, 0.0}, {1.0, 0.0}});
    editor.commit();
    ASSERT_TRUE(editor.undo());
    ASSERT_TRUE(editor.canRedo());

    editor.beginTransaction("Unchanged styles");
    // PT6 style tables are empty: distinct styles become constructible in
    // PT11/PT22. Both generic replacement paths must already reject no-ops.
    EXPECT_FALSE(editor.tables().replaceTextStyles(core::TextStyleTable{}));
    EXPECT_FALSE(editor.tables().replaceDimStyles(core::DimStyleTable{}));
    EXPECT_EQ(editor.savepoint().changes, 0U);
    EXPECT_FALSE(editor.isModified());
    editor.commit();

    EXPECT_FALSE(editor.canUndo());
    EXPECT_TRUE(editor.canRedo());
    ASSERT_TRUE(editor.redo());
    EXPECT_NE(document.find(id), nullptr);
}

TEST(DocumentTables, UnchangedOrAbsentBlocksDoNotRecordChanges) {
    core::Document document;
    document.replaceBlock("A", core::BlockDefinition{"A"});
    const auto initial = document.blocks();
    Editor editor(document);

    editor.beginTransaction("Unchanged blocks");
    EXPECT_FALSE(editor.tables().replaceBlock("missing", std::nullopt));
    EXPECT_FALSE(editor.tables().replaceBlock("A", core::BlockDefinition{"A"}));
    EXPECT_FALSE(editor.tables().replaceBlock("a", core::BlockDefinition{"different"}));
    EXPECT_EQ(editor.savepoint().changes, 0U);
    EXPECT_FALSE(editor.isModified());
    editor.commit();
    EXPECT_EQ(document.blocks(), initial);
    EXPECT_FALSE(editor.canUndo());
}

TEST(DocumentTables, ABlockInUseIsNotRemoved) {
    // As AutoCAD's PURGE (docs/tappe/0.8/PT16-blocchi-core.md, point 1).
    core::Document document = core::Document::newDrawing();
    document.replaceBlock("A", core::BlockDefinition{"A"});
    core::Insert insert;
    insert.block = "A";
    document.add(insert);
    const auto initial = document.blocks();
    Editor editor(document);

    editor.beginTransaction("Remove a block in use");
    EXPECT_FALSE(editor.tables().replaceBlock("a", std::nullopt));
    EXPECT_EQ(editor.savepoint().changes, 0U);
    editor.commit();
    EXPECT_EQ(document.blocks(), initial);
    EXPECT_FALSE(editor.canUndo());
}

TEST(DocumentTables, NewBlockUsesTheLookupName) {
    core::Document document;
    Editor editor(document);
    editor.beginTransaction("Add block");
    ASSERT_TRUE(editor.tables().replaceBlock("A", core::BlockDefinition{"different"}));
    editor.commit();
    ASSERT_NE(document.blocks().find("A"), nullptr);
    EXPECT_EQ(document.blocks().find("A")->name, "A");
    EXPECT_EQ(document.blocks().find("different"), nullptr);
    ASSERT_TRUE(editor.undo());
    EXPECT_TRUE(document.blocks().definitions().empty());
    ASSERT_TRUE(editor.redo());
    ASSERT_NE(document.blocks().find("A"), nullptr);
    EXPECT_EQ(document.blocks().find("A")->name, "A");
}

#ifndef NDEBUG
TEST(DocumentTables, AddingLayerRequiresOpenTransaction) {
    auto document = core::Document::newDrawing();
    Editor editor(document);
    core::Layer layer;
    layer.name = "Walls";
    layer.linetype = document.continuousLinetype();

    EXPECT_DEATH(editor.tables().addLayer(layer), "open transaction");
}

TEST(DocumentTables, NewTableChangesRequireOpenTransaction) {
    core::Document document;
    Editor editor(document);
    EXPECT_DEATH(editor.tables().replaceTextStyles(core::TextStyleTable{}), "open transaction");
    EXPECT_DEATH(editor.tables().replaceDimStyles(core::DimStyleTable{}), "open transaction");
    EXPECT_DEATH(editor.tables().replaceBlock("A", core::BlockDefinition{"A"}), "open transaction");
}
#endif

}  // namespace
}  // namespace tg::edit
