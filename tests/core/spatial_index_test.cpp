// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/spatial_index.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

/// The ids found by `index` in `area`, sorted.
std::vector<std::uint64_t> found(const SpatialIndex& index, const BBox& area) {
    std::vector<EntityId> ids;
    index.query(area, ids);
    std::vector<std::uint64_t> values;
    values.reserve(ids.size());
    for (const EntityId id : ids) {
        values.push_back(id.value);
    }
    std::ranges::sort(values);
    return values;
}

/// Reference model: every entry in a plain list, searched one by one.
class Reference {
public:
    void insert(std::uint64_t id, const BBox& box) {
        m_position.emplace(id, m_entries.size());
        m_entries.emplace_back(id, box);
    }

    void erase(std::uint64_t id) {
        const auto it = m_position.find(id);
        ASSERT_NE(it, m_position.end()) << "id " << id << " is not in the reference";
        const std::size_t position = it->second;
        m_position.erase(it);
        if (position + 1 != m_entries.size()) {
            m_entries[position] = m_entries.back();
            m_position[m_entries[position].first] = position;
        }
        m_entries.pop_back();
    }

    [[nodiscard]] std::vector<std::uint64_t> query(const BBox& area) const {
        std::vector<std::uint64_t> values;
        for (const auto& [id, box] : m_entries) {
            const bool overlap = box.min().x <= area.max().x && box.max().x >= area.min().x &&
                                 box.min().y <= area.max().y && box.max().y >= area.min().y;
            if (overlap) {
                values.push_back(id);
            }
        }
        std::ranges::sort(values);
        return values;
    }

    [[nodiscard]] BBox bounds() const {
        BBox box;
        for (const auto& entry : m_entries) {
            box.expand(entry.second);
        }
        return box;
    }

    [[nodiscard]] const std::vector<std::pair<std::uint64_t, BBox>>& entries() const {
        return m_entries;
    }

private:
    std::vector<std::pair<std::uint64_t, BBox>> m_entries;
    std::unordered_map<std::uint64_t, std::size_t> m_position;
};

/// Random boxes reproducible on every platform: std::mt19937 is fully
/// specified by the standard, while the standard distributions are not, so
/// raw numbers are turned into doubles here.
class BoxGenerator {
public:
    explicit BoxGenerator(std::uint32_t seed) : m_engine(seed) {}

    /// Uniform in [0, 1).
    double unit() { return static_cast<double>(m_engine()) / kRange; }

    double between(double low, double high) { return low + ((high - low) * unit()); }

    std::size_t index(std::size_t count) {
        return static_cast<std::size_t>(unit() * static_cast<double>(count));
    }

    /// A mix of points, horizontal and vertical segments, small boxes and a
    /// few large ones, in a 1000 x 1000 area.
    BBox box() {
        const Vec2 corner{between(0.0, 1000.0), between(0.0, 1000.0)};
        const double kind = unit();
        if (kind < 0.2) {
            return {corner, corner};
        }
        if (kind < 0.3) {
            return {corner, corner + Vec2{between(0.0, 80.0), 0.0}};
        }
        if (kind < 0.4) {
            return {corner, corner + Vec2{0.0, between(0.0, 80.0)}};
        }
        if (kind < 0.95) {
            return {corner, corner + Vec2{between(0.0, 30.0), between(0.0, 30.0)}};
        }
        return {corner, corner + Vec2{between(0.0, 500.0), between(0.0, 500.0)}};
    }

private:
    static constexpr double kRange = 4294967296.0;  // 2^32, mt19937's range

    std::mt19937 m_engine;
};

/// Compares index and reference on `queries` random areas.
void expectSameResults(const SpatialIndex& index, const Reference& reference,
                       BoxGenerator& generator, int queries) {
    for (int i = 0; i < queries; ++i) {
        const BBox area = generator.box();
        ASSERT_EQ(found(index, area), reference.query(area));
    }
    // An area covering everything, and one far away.
    const BBox all{{-1.0, -1.0}, {2000.0, 2000.0}};
    ASSERT_EQ(found(index, all), reference.query(all));
    ASSERT_TRUE(found(index, {{5000.0, 5000.0}, {6000.0, 6000.0}}).empty());
}

TEST(SpatialIndex, EmptyIndex) {
    const SpatialIndex index;
    EXPECT_TRUE(index.isEmpty());
    EXPECT_EQ(index.size(), 0U);
    EXPECT_TRUE(index.bounds().isEmpty());
    EXPECT_TRUE(found(index, {{-1e9, -1e9}, {1e9, 1e9}}).empty());
    EXPECT_TRUE(index.checkInvariants());
}

TEST(SpatialIndex, SingleEntry) {
    SpatialIndex index;
    const BBox box{{1.0, 2.0}, {3.0, 5.0}};
    index.insert(EntityId{7}, box);

    EXPECT_FALSE(index.isEmpty());
    EXPECT_EQ(index.size(), 1U);
    EXPECT_EQ(index.bounds().min(), box.min());
    EXPECT_EQ(index.bounds().max(), box.max());
    EXPECT_TRUE(index.checkInvariants());

    EXPECT_EQ(found(index, {{0.0, 0.0}, {2.0, 3.0}}), std::vector<std::uint64_t>{7});
    EXPECT_EQ(found(index, {{1.5, 2.5}, {1.6, 2.6}}), std::vector<std::uint64_t>{7});
    EXPECT_TRUE(found(index, {{4.0, 4.0}, {6.0, 6.0}}).empty());
    EXPECT_TRUE(found(index, BBox{}).empty());

    EXPECT_TRUE(index.erase(EntityId{7}, box));
    EXPECT_TRUE(index.isEmpty());
    EXPECT_TRUE(index.bounds().isEmpty());
    EXPECT_TRUE(found(index, box).empty());
    EXPECT_TRUE(index.checkInvariants());
}

TEST(SpatialIndex, PointAndSegmentBoxes) {
    SpatialIndex index;
    const BBox point{{5.0, 5.0}, {5.0, 5.0}};
    const BBox horizontal{{0.0, 1.0}, {10.0, 1.0}};
    const BBox vertical{{2.0, 0.0}, {2.0, 10.0}};
    index.insert(EntityId{1}, point);
    index.insert(EntityId{2}, horizontal);
    index.insert(EntityId{3}, vertical);

    // A degenerate query area: the very point.
    EXPECT_EQ(found(index, point), std::vector<std::uint64_t>{1});
    // A point lying on both segments.
    EXPECT_EQ(found(index, {{2.0, 1.0}, {2.0, 1.0}}), (std::vector<std::uint64_t>{2, 3}));
    // A vertical line crossing the horizontal segment only.
    EXPECT_EQ(found(index, {{8.0, -5.0}, {8.0, 5.0}}), std::vector<std::uint64_t>{2});
    EXPECT_TRUE(found(index, {{5.0, 5.1}, {6.0, 6.0}}).empty());

    EXPECT_TRUE(index.erase(EntityId{1}, point));
    EXPECT_TRUE(found(index, point).empty());
    EXPECT_TRUE(index.checkInvariants());
}

TEST(SpatialIndex, TouchingBordersAreFound) {
    SpatialIndex index;
    index.insert(EntityId{1}, {{0.0, 0.0}, {1.0, 1.0}});
    index.insert(EntityId{2}, {{1.0, 0.0}, {2.0, 1.0}});  // shares the side x = 1
    index.insert(EntityId{3}, {{2.0, 1.0}, {3.0, 2.0}});  // touches 2 at a corner

    EXPECT_EQ(found(index, {{1.0, 0.5}, {1.0, 0.5}}), (std::vector<std::uint64_t>{1, 2}));
    EXPECT_EQ(found(index, {{2.0, 1.0}, {2.0, 1.0}}), (std::vector<std::uint64_t>{2, 3}));
    // An area whose border touches box 1 from the left.
    EXPECT_EQ(found(index, {{-1.0, 0.0}, {0.0, 1.0}}), std::vector<std::uint64_t>{1});
    // An area just beyond box 3.
    EXPECT_TRUE(found(index, {{3.0000001, 0.0}, {4.0, 4.0}}).empty());
}

TEST(SpatialIndex, LargeAndSmallCoordinates) {
    SpatialIndex index;
    Reference reference;
    std::uint64_t id = 1;
    // Survey coordinates in millimetres, and sub-micrometre details.
    for (int i = 0; i < 200; ++i) {
        const double big = 1e9 + (i * 1e3);
        const BBox far{{big, -big}, {big + 500.0, -big + 500.0}};
        index.insert(EntityId{id}, far);
        reference.insert(id++, far);
        const double tiny = i * 1e-9;
        const BBox small{{tiny, tiny}, {tiny + 5e-10, tiny + 5e-10}};
        index.insert(EntityId{id}, small);
        reference.insert(id++, small);
    }
    EXPECT_TRUE(index.checkInvariants());
    const BBox nearFar{{1e9 + 10'000.0, -1e9 - 300'000.0}, {1e9 + 20'000.0, 0.0}};
    EXPECT_EQ(found(index, nearFar), reference.query(nearFar));
    EXPECT_EQ(found(index, nearFar).size(), 11U);
    const BBox nearSmall{{10e-9, 10e-9}, {20e-9, 20e-9}};
    EXPECT_EQ(found(index, nearSmall), reference.query(nearSmall));
    EXPECT_EQ(found(index, nearSmall).size(), 11U);
    EXPECT_EQ(index.bounds().min(), reference.bounds().min());
    EXPECT_EQ(index.bounds().max(), reference.bounds().max());
}

TEST(SpatialIndex, EraseOfAnAbsentIdChangesNothing) {
    SpatialIndex index;
    const BBox box{{0.0, 0.0}, {1.0, 1.0}};
    EXPECT_FALSE(index.erase(EntityId{1}, box));
    EXPECT_TRUE(index.checkInvariants());

    for (std::uint64_t id = 1; id <= 50; ++id) {
        const auto x = static_cast<double>(id);
        index.insert(EntityId{id}, {{x, x}, {x + 1.0, x + 1.0}});
    }
    EXPECT_FALSE(index.erase(EntityId{99}, box));
    EXPECT_FALSE(index.erase(EntityId{99}, {{-100.0, -100.0}, {100.0, 100.0}}));
    EXPECT_FALSE(index.erase(EntityId{1}, BBox{}));
    EXPECT_EQ(index.size(), 50U);

    EXPECT_TRUE(index.erase(EntityId{3}, {{3.0, 3.0}, {4.0, 4.0}}));
    EXPECT_FALSE(index.erase(EntityId{3}, {{3.0, 3.0}, {4.0, 4.0}}));  // already gone
    EXPECT_EQ(index.size(), 49U);
    EXPECT_TRUE(index.checkInvariants());
}

TEST(SpatialIndex, ManyIdenticalBoxes) {
    // Every box the same point: no split heuristic can tell them apart.
    SpatialIndex index;
    const BBox point{{4.0, 4.0}, {4.0, 4.0}};
    for (std::uint64_t id = 1; id <= 1000; ++id) {
        index.insert(EntityId{id}, point);
    }
    EXPECT_TRUE(index.checkInvariants());
    EXPECT_EQ(found(index, point).size(), 1000U);
    for (std::uint64_t id = 1; id <= 1000; id += 2) {
        ASSERT_TRUE(index.erase(EntityId{id}, point));
    }
    EXPECT_TRUE(index.checkInvariants());
    EXPECT_EQ(found(index, point).size(), 500U);
}

TEST(SpatialIndex, CollinearPoints) {
    // Boxes with no area at all: splits must still balance the tree.
    SpatialIndex index;
    Reference reference;
    for (std::uint64_t id = 1; id <= 600; ++id) {
        const auto x = static_cast<double>((id * 37) % 600);
        const BBox point{{x, 2.0}, {x, 2.0}};
        index.insert(EntityId{id}, point);
        reference.insert(id, point);
    }
    EXPECT_TRUE(index.checkInvariants());
    const BBox range{{100.0, 0.0}, {199.5, 5.0}};
    EXPECT_EQ(found(index, range), reference.query(range));
    EXPECT_EQ(found(index, range).size(), 100U);
}

TEST(SpatialIndex, RandomInsertionsAndErasuresMatchExhaustiveSearch) {
    constexpr std::uint64_t kCount = 10'000;
    constexpr std::uint64_t kCheckEvery = 97;
    BoxGenerator generator(20261016U);
    SpatialIndex index;
    Reference reference;

    for (std::uint64_t id = 1; id <= kCount; ++id) {
        const BBox box = generator.box();
        index.insert(EntityId{id}, box);
        reference.insert(id, box);
        if (id % kCheckEvery == 0) {
            ASSERT_TRUE(index.checkInvariants()) << "after inserting " << id;
            expectSameResults(index, reference, generator, 3);
        }
    }
    ASSERT_EQ(index.size(), kCount);
    ASSERT_TRUE(index.checkInvariants());
    expectSameResults(index, reference, generator, 200);

    // Erase everything, in random order.
    std::vector<std::pair<std::uint64_t, BBox>> pending = reference.entries();
    std::uint64_t step = 0;
    while (!pending.empty()) {
        const std::size_t pick = generator.index(pending.size());
        const auto [id, box] = pending[pick];
        pending[pick] = pending.back();
        pending.pop_back();

        ASSERT_TRUE(index.erase(EntityId{id}, box)) << "id " << id;
        reference.erase(id);
        if (++step % kCheckEvery == 0) {
            ASSERT_TRUE(index.checkInvariants()) << "after erasing " << step;
            expectSameResults(index, reference, generator, 3);
            ASSERT_EQ(index.bounds().min(), reference.bounds().min());
            ASSERT_EQ(index.bounds().max(), reference.bounds().max());
        }
    }
    EXPECT_TRUE(index.isEmpty());
    EXPECT_TRUE(index.bounds().isEmpty());
    EXPECT_TRUE(index.checkInvariants());
}

TEST(SpatialIndex, InterleavedInsertionsAndErasures) {
    // As in an editing session: entities come and go, undo puts them back
    // with the same identifier.
    constexpr int kSteps = 20'000;
    BoxGenerator generator(42U);
    SpatialIndex index;
    Reference reference;
    std::vector<std::pair<std::uint64_t, BBox>> removed;
    std::uint64_t nextId = 1;

    for (int step = 1; step <= kSteps; ++step) {
        const double action = generator.unit();
        const auto& present = reference.entries();
        if (action < 0.5 || present.empty()) {
            const BBox box = generator.box();
            index.insert(EntityId{nextId}, box);
            reference.insert(nextId++, box);
        } else if (action < 0.8) {
            const auto entry = present[generator.index(present.size())];
            ASSERT_TRUE(index.erase(EntityId{entry.first}, entry.second));
            reference.erase(entry.first);
            removed.push_back(entry);
        } else if (!removed.empty()) {
            const auto entry = removed.back();  // "undo": same id, same box
            removed.pop_back();
            index.insert(EntityId{entry.first}, entry.second);
            reference.insert(entry.first, entry.second);
        }
        ASSERT_EQ(index.size(), reference.entries().size());
        if (step % 211 == 0) {
            ASSERT_TRUE(index.checkInvariants()) << "at step " << step;
            expectSameResults(index, reference, generator, 3);
        }
    }
    EXPECT_TRUE(index.checkInvariants());
    expectSameResults(index, reference, generator, 100);
}

TEST(SpatialIndex, BoundsShrinkAfterErasure) {
    SpatialIndex index;
    const BBox left{{-50.0, 0.0}, {-40.0, 10.0}};
    index.insert(EntityId{1}, left);
    for (std::uint64_t id = 2; id <= 40; ++id) {
        const auto x = static_cast<double>(id);
        index.insert(EntityId{id}, {{x, 0.0}, {x + 1.0, 1.0}});
    }
    const BBox right{{400.0, 300.0}, {410.0, 320.0}};
    index.insert(EntityId{41}, right);
    EXPECT_EQ(index.bounds().min(), Vec2(-50.0, 0.0));
    EXPECT_EQ(index.bounds().max(), Vec2(410.0, 320.0));

    EXPECT_TRUE(index.erase(EntityId{1}, left));
    EXPECT_TRUE(index.erase(EntityId{41}, right));
    EXPECT_EQ(index.bounds().min(), Vec2(2.0, 0.0));
    EXPECT_EQ(index.bounds().max(), Vec2(41.0, 1.0));
    EXPECT_TRUE(index.checkInvariants());
}

TEST(SpatialIndex, CopyIsIndependentOfTheOriginal) {
    SpatialIndex original;
    for (std::uint64_t id = 1; id <= 300; ++id) {
        const auto x = static_cast<double>(id);
        original.insert(EntityId{id}, {{x, x}, {x + 2.0, x + 2.0}});
    }
    const SpatialIndex copy = original;

    const BBox area{{0.0, 0.0}, {1000.0, 1000.0}};
    for (std::uint64_t id = 1; id <= 300; id += 3) {
        const auto x = static_cast<double>(id);
        ASSERT_TRUE(original.erase(EntityId{id}, {{x, x}, {x + 2.0, x + 2.0}}));
    }
    original.insert(EntityId{1000}, {{-5.0, -5.0}, {-4.0, -4.0}});

    EXPECT_EQ(copy.size(), 300U);
    EXPECT_EQ(found(copy, area).size(), 300U);
    EXPECT_TRUE(copy.checkInvariants());
    EXPECT_EQ(original.size(), 201U);
    EXPECT_TRUE(original.checkInvariants());

    const SpatialIndex moved = std::move(original);
    EXPECT_EQ(moved.size(), 201U);
    EXPECT_TRUE(moved.checkInvariants());
}

TEST(SpatialIndex, ClearEmptiesTheIndexForReuse) {
    SpatialIndex index;
    for (std::uint64_t id = 1; id <= 100; ++id) {
        const auto x = static_cast<double>(id);
        index.insert(EntityId{id}, {{x, 0.0}, {x, 0.0}});
    }
    index.clear();
    EXPECT_TRUE(index.isEmpty());
    EXPECT_TRUE(index.bounds().isEmpty());
    EXPECT_TRUE(found(index, {{0.0, -1.0}, {200.0, 1.0}}).empty());
    EXPECT_TRUE(index.checkInvariants());

    index.insert(EntityId{5}, {{1.0, 1.0}, {2.0, 2.0}});
    EXPECT_EQ(found(index, {{0.0, 0.0}, {3.0, 3.0}}), std::vector<std::uint64_t>{5});
    EXPECT_TRUE(index.checkInvariants());
}

}  // namespace
}  // namespace tg::core
