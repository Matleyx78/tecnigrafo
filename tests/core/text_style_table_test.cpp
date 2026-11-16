// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"

#include <gtest/gtest.h>

namespace tg::core {
namespace {

TEST(TextStyleTableTest, FindsAndKeepsHandleAndPositionWhenReplacing) {
    Document document = Document::newDrawing();
    TextStyleTable table = document.textStyles();
    TextStyle custom;
    custom.name = "Custom";
    custom.font = "romans.shx";
    const auto id = table.put(std::move(custom), document);
    TextStyle replacement;
    replacement.name = "custom";
    replacement.font = "arial.ttf";
    replacement.widthFactor = 0.8;
    table.put(std::move(replacement), document);

    ASSERT_EQ(table.styles().size(), 2U);
    EXPECT_EQ(table.styles()[1].id, id);
    EXPECT_EQ(table.styles()[1].name, "Custom");
    EXPECT_EQ(table.find("CUSTOM"), &table.styles()[1]);
}

TEST(TextStyleTableTest, StoresCurrentStyle) {
    Document document = Document::newDrawing();
    TextStyleTable table = document.textStyles();
    TextStyle note;
    note.name = "Note";
    table.put(std::move(note), document);
    table.setCurrent("Note");
    EXPECT_EQ(table.current(), "Note");
}

}  // namespace
}  // namespace tg::core
