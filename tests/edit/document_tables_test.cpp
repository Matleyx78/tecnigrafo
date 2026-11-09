// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/document_tables.hpp"

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
    ASSERT_TRUE(layerId.has_value());
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

#ifndef NDEBUG
TEST(DocumentTables, AddingLayerRequiresOpenTransaction) {
    auto document = core::Document::newDrawing();
    Editor editor(document);
    core::Layer layer;
    layer.name = "Walls";
    layer.linetype = document.continuousLinetype();

    EXPECT_DEATH(editor.tables().addLayer(layer), "open transaction");
}
#endif

}  // namespace
}  // namespace tg::edit
