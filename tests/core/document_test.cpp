// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
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

/// What Document::query() must return: every entity whose bounding box
/// overlaps `area`, found by visiting them all. Sorted by id.
std::vector<std::uint64_t> exhaustiveQuery(const Document& document, const BBox& area) {
    std::vector<std::uint64_t> values;
    for (const EntityRecord& record : document.entities()) {
        const BBox box = boundingBox(record.entity);
        const bool overlap = !box.isEmpty() && box.min().x <= area.max().x &&
                             box.max().x >= area.min().x && box.min().y <= area.max().y &&
                             box.max().y >= area.min().y;
        if (overlap) {
            values.push_back(record.id.value);
        }
    }
    std::ranges::sort(values);
    return values;
}

std::vector<std::uint64_t> indexedQuery(const Document& document, const BBox& area) {
    std::vector<EntityId> ids;
    document.query(area, ids);
    std::vector<std::uint64_t> values;
    values.reserve(ids.size());
    for (const EntityId id : ids) {
        values.push_back(id.value);
    }
    std::ranges::sort(values);
    return values;
}

/// Every entity can be found, at its own place in the list.
void expectEveryEntityFound(const Document& document) {
    for (const EntityRecord& record : document.entities()) {
        ASSERT_EQ(document.find(record.id), &record) << "id " << record.id.value;
    }
}

/// Random polylines reproducible on every platform: std::mt19937 is fully
/// specified by the standard, its distributions are not.
class PolylineGenerator {
public:
    explicit PolylineGenerator(std::uint32_t seed) : m_engine(seed) {}

    double unit() { return static_cast<double>(m_engine()) / kRange; }

    double between(double low, double high) { return low + ((high - low) * unit()); }

    std::size_t index(std::size_t count) {
        return static_cast<std::size_t>(unit() * static_cast<double>(count));
    }

    /// Zero to four vertices near a random point, some segments arcs.
    Polyline polyline() {
        Polyline result;
        const Vec2 origin{between(0.0, 500.0), between(0.0, 500.0)};
        const auto vertices = index(5);
        for (std::size_t i = 0; i < vertices; ++i) {
            const double bulge = unit() < 0.3 ? between(-1.0, 1.0) : 0.0;
            result.addVertex(origin + Vec2{between(0.0, 40.0), between(0.0, 40.0)}, bulge);
        }
        result.setClosed(unit() < 0.2);
        return result;
    }

    BBox area() {
        const Vec2 corner{between(-20.0, 520.0), between(-20.0, 520.0)};
        return {corner, corner + Vec2{between(0.0, 150.0), between(0.0, 150.0)}};
    }

private:
    static constexpr double kRange = 4294967296.0;  // 2^32, mt19937's range

    std::mt19937 m_engine;
};

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

TEST(Document, QueryFindsEntitiesByBoundingBox) {
    Document document;
    const EntityId left = document.add(segment({0.0, 0.0}, {10.0, 0.0}));
    const EntityId right = document.add(segment({20.0, 0.0}, {30.0, 10.0}));
    Polyline arc;
    arc.addVertex({40.0, 0.0}, 1.0);  // half circle below the chord
    arc.addVertex({50.0, 0.0});
    const EntityId bulging = document.add(arc);

    EXPECT_EQ(indexedQuery(document, {{5.0, -1.0}, {6.0, 1.0}}),
              std::vector<std::uint64_t>{left.value});
    EXPECT_EQ(indexedQuery(document, {{10.0, 0.0}, {20.0, 0.0}}),
              (std::vector<std::uint64_t>{left.value, right.value}));
    // Only the arc's bounding box reaches 5 units below the chord.
    EXPECT_EQ(indexedQuery(document, {{44.0, -5.0}, {46.0, -4.0}}),
              std::vector<std::uint64_t>{bulging.value});
    EXPECT_TRUE(indexedQuery(document, {{100.0, 100.0}, {200.0, 200.0}}).empty());
}

TEST(Document, FindAfterRemovingFromTheMiddle) {
    Document document;
    std::vector<EntityId> ids;
    for (int i = 0; i < 10; ++i) {
        const auto x = static_cast<double>(i);
        ids.push_back(document.add(segment({x, 0.0}, {x, 1.0})));
    }
    ASSERT_TRUE(document.remove(ids[3]).has_value());
    ASSERT_TRUE(document.remove(ids[7]).has_value());

    EXPECT_EQ(document.find(ids[3]), nullptr);
    EXPECT_EQ(document.find(ids[7]), nullptr);
    for (const std::size_t i : {0U, 1U, 2U, 4U, 5U, 6U, 8U, 9U}) {
        const EntityRecord* found = document.find(ids[i]);
        ASSERT_NE(found, nullptr) << "entity " << i;
        EXPECT_EQ(found->id, ids[i]);
        const auto x = static_cast<double>(i);
        EXPECT_EQ(found->entity, Entity{segment({x, 0.0}, {x, 1.0})});
    }
    expectEveryEntityFound(document);
}

TEST(Document, ExtentsShrinkAfterRemovals) {
    Document document;
    const EntityId low = document.add(segment({-100.0, -50.0}, {-90.0, -40.0}));
    document.add(segment({0.0, 0.0}, {10.0, 10.0}));
    const EntityId high = document.add(segment({200.0, 300.0}, {210.0, 320.0}));
    EXPECT_EQ(document.extents().min(), Vec2(-100.0, -50.0));
    EXPECT_EQ(document.extents().max(), Vec2(210.0, 320.0));

    ASSERT_TRUE(document.remove(low).has_value());
    EXPECT_EQ(document.extents().min(), Vec2(0.0, 0.0));
    ASSERT_TRUE(document.remove(high).has_value());
    EXPECT_EQ(document.extents().max(), Vec2(10.0, 10.0));
}

TEST(Document, EntityWithoutVerticesIsKeptButNeverFound) {
    Document document;
    const EntityId line = document.add(segment({0.0, 0.0}, {1.0, 1.0}));
    const EntityId empty = document.add(Polyline{});

    EXPECT_EQ(document.entityCount(), 2U);
    ASSERT_NE(document.find(empty), nullptr);
    EXPECT_EQ(document.find(empty)->entity, Entity{Polyline{}});
    EXPECT_EQ(indexedQuery(document, {{-1e9, -1e9}, {1e9, 1e9}}),
              std::vector<std::uint64_t>{line.value});
    EXPECT_EQ(document.extents().max(), Vec2(1.0, 1.0));

    EntityRecord removed = removeOrFail(document, empty);
    EXPECT_EQ(document.entityCount(), 1U);
    document.restore(std::move(removed));
    ASSERT_NE(document.find(empty), nullptr);
    expectEveryEntityFound(document);
}

TEST(Document, QueryMatchesExhaustiveSearchWhileEditing) {
    // Additions, removals and restorations in random order, as in an
    // editing session with undo and redo.
    constexpr int kSteps = 3000;
    constexpr int kCheckEvery = 50;
    PolylineGenerator generator(16102026U);
    Document document;
    std::vector<EntityRecord> removed;

    for (int step = 1; step <= kSteps; ++step) {
        const double action = generator.unit();
        if (action < 0.5 || document.isEmpty()) {
            document.add(generator.polyline());
        } else if (action < 0.8) {
            const auto entities = document.entities();
            const EntityId id = entities[generator.index(entities.size())].id;
            removed.push_back(removeOrFail(document, id));
        } else if (!removed.empty()) {
            document.restore(std::move(removed.back()));
            removed.pop_back();
        }
        if (step % kCheckEvery == 0) {
            for (int i = 0; i < 5; ++i) {
                const BBox area = generator.area();
                ASSERT_EQ(indexedQuery(document, area), exhaustiveQuery(document, area))
                    << "at step " << step;
            }
            expectEveryEntityFound(document);
            BBox all;
            for (const EntityRecord& record : document.entities()) {
                all.expand(boundingBox(record.entity));
            }
            ASSERT_EQ(document.extents().min(), all.min()) << "at step " << step;
            ASSERT_EQ(document.extents().max(), all.max()) << "at step " << step;
        }
    }
    for (const EntityRecord& record : removed) {
        EXPECT_EQ(document.find(record.id), nullptr);
    }
}

TEST(Document, CopyIsIndependentOfTheOriginal) {
    Document original;
    std::vector<EntityId> ids;
    for (int i = 0; i < 100; ++i) {
        const auto x = static_cast<double>(i);
        ids.push_back(original.add(segment({x, 0.0}, {x + 0.5, 1.0})));
    }
    const Document copy = original;
    const BBox everything{{-1.0, -1.0}, {200.0, 2.0}};
    const std::vector<std::uint64_t> before = indexedQuery(copy, everything);

    for (std::size_t i = 0; i < ids.size(); i += 2) {
        ASSERT_TRUE(original.remove(ids[i]).has_value());
    }
    original.add(segment({500.0, 500.0}, {501.0, 501.0}));

    EXPECT_EQ(copy.entityCount(), 100U);
    EXPECT_EQ(indexedQuery(copy, everything), before);
    EXPECT_EQ(copy.extents().max(), Vec2(99.5, 1.0));
    expectEveryEntityFound(copy);
    EXPECT_EQ(original.entityCount(), 51U);
    EXPECT_EQ(indexedQuery(original, everything).size(), 50U);
    expectEveryEntityFound(original);

    const Document moved = std::move(original);
    EXPECT_EQ(moved.entityCount(), 51U);
    EXPECT_EQ(moved.extents().max(), Vec2(501.0, 501.0));
    expectEveryEntityFound(moved);
}

}  // namespace
}  // namespace tg::core
