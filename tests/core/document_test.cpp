// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"

#include <gtest/gtest.h>

#include <optional>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

/// A two-point polyline from `from` to `to`.
Polyline segment(const Vec2& from, const Vec2& to) {
    Polyline polyline;
    polyline.addVertex(from);
    polyline.addVertex(to);
    return polyline;
}

/// Removes `id` from `document`, failing the test if it is missing.
EntityRecord removeOrFail(Document& document, EntityId id) {
    std::optional<EntityRecord> removed = document.remove(id);
    if (!removed) {
        ADD_FAILURE() << "entity " << id.value << " is not in the document";
        return {};
    }
    return std::move(*removed);
}

std::vector<EntityId> idsOf(const Document& document) {
    std::vector<EntityId> ids;
    for (const EntityRecord& record : document.entities()) {
        ids.push_back(record.id);
    }
    return ids;
}

TEST(Document, AssignsUniqueIdsAndComputesExtents) {
    Document document;
    EXPECT_TRUE(document.extents().isEmpty());

    const EntityId first = document.add(segment({0.0, 0.0}, {1.0, 1.0}));
    const EntityId second = document.add(segment({5.0, 5.0}, {6.0, 7.0}));

    EXPECT_NE(first, second);
    EXPECT_NE(first.value, 0U);
    EXPECT_EQ(document.entityCount(), 2U);
    EXPECT_EQ(document.extents().min(), Vec2(0.0, 0.0));
    EXPECT_EQ(document.extents().max(), Vec2(6.0, 7.0));
}

TEST(Document, FindsExistingAndMissingIds) {
    Document document;
    const Polyline line = segment({1.0, 2.0}, {3.0, 4.0});
    const EntityId id = document.add(line);

    const EntityRecord* found = document.find(id);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, id);
    EXPECT_EQ(found->entity, Entity{line});

    EXPECT_EQ(document.find(EntityId{id.value + 1}), nullptr);
    EXPECT_EQ(document.find(EntityId{}), nullptr);
}

TEST(Document, RemoveReturnsTheRecordAndKeepsOrder) {
    Document document;
    const EntityId first = document.add(segment({0.0, 0.0}, {1.0, 0.0}));
    const EntityId second = document.add(segment({0.0, 0.0}, {2.0, 0.0}));
    const EntityId third = document.add(segment({0.0, 0.0}, {3.0, 0.0}));

    const std::optional<EntityRecord> removed = document.remove(second);
    const std::optional<EntityRecord> expected =
        EntityRecord{second, Entity{segment({0.0, 0.0}, {2.0, 0.0})}};
    EXPECT_EQ(removed, expected);

    EXPECT_EQ(document.find(second), nullptr);
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{first, third}));
}

TEST(Document, RemovingAMissingIdChangesNothing) {
    Document document;
    const EntityId id = document.add(segment({0.0, 0.0}, {1.0, 0.0}));

    EXPECT_FALSE(document.remove(EntityId{id.value + 10}).has_value());
    EXPECT_FALSE(document.remove(EntityId{}).has_value());
    EXPECT_EQ(document.entityCount(), 1U);
}

TEST(Document, RestoreKeepsTheOriginalIdAndAppends) {
    Document document;
    const EntityId first = document.add(segment({0.0, 0.0}, {1.0, 0.0}));
    const EntityId second = document.add(segment({0.0, 0.0}, {2.0, 0.0}));

    document.restore(removeOrFail(document, first));

    const EntityRecord* restored = document.find(first);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->entity, Entity{segment({0.0, 0.0}, {1.0, 0.0})});
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{second, first}));
}

TEST(Document, RemovedIdsAreNeverReassigned) {
    Document document;
    const EntityId first = document.add(segment({0.0, 0.0}, {1.0, 0.0}));
    const EntityId second = document.add(segment({0.0, 0.0}, {2.0, 0.0}));
    ASSERT_TRUE(document.remove(second).has_value());
    ASSERT_TRUE(document.remove(first).has_value());

    const EntityId third = document.add(segment({0.0, 0.0}, {3.0, 0.0}));
    EXPECT_NE(third, first);
    EXPECT_NE(third, second);
    EXPECT_GT(third.value, second.value);
}

TEST(Document, RestoringAForeignIdAdvancesTheCounter) {
    Document document;
    document.restore(EntityRecord{EntityId{41}, segment({0.0, 0.0}, {1.0, 0.0})});

    const EntityId next = document.add(segment({0.0, 0.0}, {2.0, 0.0}));
    EXPECT_GT(next.value, 41U);
}

TEST(Document, ExtentsFollowRemoveAndRestore) {
    Document document;
    document.add(segment({0.0, 0.0}, {1.0, 1.0}));
    const EntityId far = document.add(segment({10.0, 10.0}, {20.0, 30.0}));

    EntityRecord removed = removeOrFail(document, far);
    EXPECT_EQ(document.extents().max(), Vec2(1.0, 1.0));

    document.restore(std::move(removed));
    EXPECT_EQ(document.extents().max(), Vec2(20.0, 30.0));

    ASSERT_TRUE(document.remove(far).has_value());
    ASSERT_TRUE(document.remove(document.entities().front().id).has_value());
    EXPECT_TRUE(document.extents().isEmpty());
}

}  // namespace
}  // namespace tg::core
