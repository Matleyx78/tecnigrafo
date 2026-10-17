// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/selection_set.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace tg::cmd {
namespace {

using core::EntityId;

std::vector<std::uint64_t> values(const SelectionSet& selection) {
    std::vector<std::uint64_t> result;
    for (const EntityId id : selection.ids()) {
        result.push_back(id.value);
    }
    return result;
}

TEST(SelectionSet, StartsEmpty) {
    const SelectionSet selection;
    EXPECT_TRUE(selection.isEmpty());
    EXPECT_EQ(selection.size(), 0U);
    EXPECT_TRUE(selection.ids().empty());
    EXPECT_FALSE(selection.contains(EntityId{1}));
}

TEST(SelectionSet, KeepsTheOrderOfChoice) {
    SelectionSet selection;
    EXPECT_TRUE(selection.add(EntityId{5}));
    EXPECT_TRUE(selection.add(EntityId{2}));
    EXPECT_TRUE(selection.add(EntityId{9}));
    EXPECT_EQ(values(selection), (std::vector<std::uint64_t>{5, 2, 9}));
    EXPECT_EQ(selection.size(), 3U);
    EXPECT_FALSE(selection.isEmpty());
}

TEST(SelectionSet, IgnoresDuplicates) {
    SelectionSet selection;
    EXPECT_TRUE(selection.add(EntityId{5}));
    EXPECT_TRUE(selection.add(EntityId{2}));
    EXPECT_FALSE(selection.add(EntityId{5}));
    EXPECT_EQ(values(selection), (std::vector<std::uint64_t>{5, 2}));
}

TEST(SelectionSet, RemoveKeepsTheOrderOfTheOthers) {
    SelectionSet selection;
    for (const std::uint64_t value : {4U, 7U, 1U, 8U}) {
        selection.add(EntityId{value});
    }
    EXPECT_TRUE(selection.remove(EntityId{7}));
    EXPECT_FALSE(selection.remove(EntityId{7}));
    EXPECT_FALSE(selection.remove(EntityId{3}));
    EXPECT_EQ(values(selection), (std::vector<std::uint64_t>{4, 1, 8}));
    EXPECT_FALSE(selection.contains(EntityId{7}));
    EXPECT_TRUE(selection.contains(EntityId{1}));
}

TEST(SelectionSet, ARemovedEntityCanBeAddedAgainAtTheEnd) {
    SelectionSet selection;
    selection.add(EntityId{1});
    selection.add(EntityId{2});
    selection.remove(EntityId{1});
    EXPECT_TRUE(selection.add(EntityId{1}));
    EXPECT_EQ(values(selection), (std::vector<std::uint64_t>{2, 1}));
}

TEST(SelectionSet, ClearEmptiesIt) {
    SelectionSet selection;
    selection.add(EntityId{1});
    selection.add(EntityId{2});
    selection.clear();
    EXPECT_TRUE(selection.isEmpty());
    EXPECT_FALSE(selection.contains(EntityId{1}));
    EXPECT_TRUE(selection.add(EntityId{1}));
    EXPECT_EQ(values(selection), (std::vector<std::uint64_t>{1}));
}

}  // namespace
}  // namespace tg::cmd
