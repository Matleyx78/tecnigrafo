// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/block_table.hpp"

#include "tg/core/document.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>

namespace tg::core {
namespace {

/// A definition named `name` whose only entity inserts `inner`.
BlockDefinition inserting(std::string name, std::string inner) {
    BlockDefinition definition{std::move(name)};
    Insert insert;
    insert.block = std::move(inner);
    definition.entities.push_back(EntityRecord{EntityId{}, insert});
    return definition;
}

TEST(BlockTable, FindComparesNamesWithoutCase) {
    Document document = Document::newDrawing();
    document.replaceBlock("Door", BlockDefinition{"Door"});
    document.replaceBlock("Window", BlockDefinition{"Window"});
    const BlockTable& table = document.blocks();
    ASSERT_NE(table.find("door"), nullptr);
    EXPECT_EQ(table.find("DOOR")->name, "Door");
    EXPECT_EQ(table.find("Window"), &table.definitions()[1]);
    EXPECT_EQ(table.find("Wall"), nullptr);
}

TEST(BlockTable, ABlockThatInsertsItselfWouldCycle) {
    const Document document = Document::newDrawing();
    EXPECT_TRUE(document.blocks().wouldCycle(inserting("A", "a")));
    EXPECT_FALSE(document.blocks().wouldCycle(BlockDefinition{"A"}));
    // A block that is not there is a missing reference, not a cycle.
    EXPECT_FALSE(document.blocks().wouldCycle(inserting("A", "Missing")));
}

TEST(BlockTable, ACycleThroughTwoLevelsIsFound) {
    // C inserts B, B inserts A: redefining A to insert C closes the loop.
    Document document = Document::newDrawing();
    document.replaceBlock("A", BlockDefinition{"A"});
    document.replaceBlock("B", inserting("B", "A"));
    document.replaceBlock("C", inserting("C", "B"));
    const BlockTable& table = document.blocks();
    EXPECT_TRUE(table.wouldCycle(inserting("A", "C")));
    EXPECT_TRUE(table.wouldCycle(inserting("A", "B")));
    // D inserting C is a chain, not a cycle; C inserting A again neither.
    EXPECT_FALSE(table.wouldCycle(inserting("D", "C")));
    EXPECT_FALSE(table.wouldCycle(inserting("C", "A")));
    // B redefined without the insert of A: the old B does not count.
    EXPECT_FALSE(table.wouldCycle(BlockDefinition{"B"}));
}

TEST(BlockTable, TheNewFieldsTakePartInEquality) {
    const BlockDefinition a{"A"};
    BlockDefinition b{"A"};
    EXPECT_EQ(a, b);
    b.base = {1.0, 2.0};
    EXPECT_NE(a, b);
    b = a;
    b.explodable = false;
    EXPECT_NE(a, b);
    b = a;
    b.entities.push_back(EntityRecord{EntityId{300}, Line{{0, 0}, {1, 0}}});
    EXPECT_NE(a, b);
    EXPECT_TRUE(a.explodable);
    EXPECT_FALSE(a.uniformScale);
    EXPECT_EQ(a.units, 0);
}

}  // namespace
}  // namespace tg::core
