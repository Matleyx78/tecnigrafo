// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"

#include "tg/core/start_handles.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
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

    const EntityRecord removed = removeOrFail(document, second);
    EXPECT_EQ(removed.id, second);
    EXPECT_EQ(removed.entity, Entity{segment({0.0, 0.0}, {2.0, 0.0})});

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

TEST(Document, RestoreKeepsTheOriginalIdAndPlace) {
    Document document;
    const EntityId first = document.add(segment({0.0, 0.0}, {1.0, 0.0}));
    const EntityId second = document.add(segment({0.0, 0.0}, {2.0, 0.0}));

    document.restore(removeOrFail(document, first));

    const EntityRecord* restored = document.find(first);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(restored->entity, Entity{segment({0.0, 0.0}, {1.0, 0.0})});
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{first, second}));
    expectEveryEntityFound(document);
}

TEST(Document, AddGivesIncreasingOrderKeys) {
    Document document;
    const EntityId first = document.add(segment({0.0, 0.0}, {1.0, 0.0}));
    const EntityId second = document.add(segment({0.0, 0.0}, {2.0, 0.0}));
    ASSERT_NE(document.find(first), nullptr);
    ASSERT_NE(document.find(second), nullptr);
    EXPECT_GT(document.find(second)->order, document.find(first)->order);
    EXPECT_TRUE(document.find(first)->extras.empty());
}

TEST(Document, InsertWithSparseHandlesThenAddTakesAHigherOne) {
    Document document;
    document.insert(EntityRecord{EntityId{0x2F}, segment({0.0, 0.0}, {1.0, 0.0}), {}, 1});
    document.reserveHandle(EntityId{0x150});
    document.insert(EntityRecord{EntityId{0x100}, segment({0.0, 0.0}, {2.0, 0.0}), {}, 2});
    document.reserveHandle(EntityId{0x30});

    EXPECT_EQ(document.nextId(), 0x151U);
    const EntityId added = document.add(segment({0.0, 0.0}, {3.0, 0.0}));
    EXPECT_GT(added.value, 0x150U);
    EXPECT_NE(added.value, 0x30U);
    EXPECT_NE(added.value, 0x150U);
    EXPECT_EQ(idsOf(document), (std::vector<EntityId>{EntityId{0x2F}, EntityId{0x100}, added}));
    EXPECT_GT(document.find(added)->order, 2U);
    expectEveryEntityFound(document);
}

TEST(Document, AddNeverTakesAReservedHandle) {
    Document document;
    document.reserveHandle(EntityId{1});
    document.reserveHandle(EntityId{2});
    const EntityId added = document.add(segment({0.0, 0.0}, {1.0, 0.0}));
    EXPECT_EQ(added.value, 3U);
}

TEST(Document, SetNextIdOnlyMovesForward) {
    Document document;
    EXPECT_EQ(document.nextId(), 1U);
    document.setNextId(0x500);
    EXPECT_EQ(document.nextId(), 0x500U);
    document.setNextId(0x100);
    EXPECT_EQ(document.nextId(), 0x500U);
    EXPECT_EQ(document.add(segment({0.0, 0.0}, {1.0, 0.0})).value, 0x500U);
    document.setNextId(0x10);
    EXPECT_EQ(document.nextId(), 0x501U);
}

TEST(Document, EntitiesComeOutInKeyOrder) {
    Document document;
    document.insert(EntityRecord{EntityId{1}, segment({0.0, 0.0}, {1.0, 0.0}), {}, 10});
    document.insert(EntityRecord{EntityId{3}, segment({0.0, 0.0}, {3.0, 0.0}), {}, 30});
    document.insert(EntityRecord{EntityId{2}, segment({0.0, 0.0}, {2.0, 0.0}), {}, 20});
    const std::vector<EntityId> expected{EntityId{1}, EntityId{2}, EntityId{3}};
    EXPECT_EQ(idsOf(document), expected);
    expectEveryEntityFound(document);

    document.restore(removeOrFail(document, EntityId{2}));
    EXPECT_EQ(idsOf(document), expected);
    EXPECT_EQ(document.find(EntityId{2})->order, 20U);
    expectEveryEntityFound(document);

    // The next entity goes after the highest key, whatever its handle.
    const EntityId added = document.add(segment({0.0, 0.0}, {4.0, 0.0}));
    EXPECT_EQ(idsOf(document).back(), added);
    EXPECT_GT(document.find(added)->order, 30U);
}

TEST(Document, RemovedFromTheMiddleComesBackInTheMiddleAfterOtherChanges) {
    Document document;
    std::vector<EntityId> ids;
    for (int i = 0; i < 6; ++i) {
        const auto x = static_cast<double>(i);
        ids.push_back(document.add(segment({x, 0.0}, {x, 1.0})));
    }
    EntityRecord second = removeOrFail(document, ids[1]);
    EntityRecord fourth = removeOrFail(document, ids[3]);
    const EntityId added = document.add(segment({9.0, 0.0}, {9.0, 1.0}));

    document.restore(std::move(fourth));
    document.restore(std::move(second));
    EXPECT_EQ(idsOf(document),
              (std::vector<EntityId>{ids[0], ids[1], ids[2], ids[3], ids[4], ids[5], added}));
    expectEveryEntityFound(document);
}

TEST(Document, ExtrasAndOrderSurviveRemoveAndRestore) {
    EntityExtras extras;
    extras.beforeEntity = {{102, "{ACAD_REACTORS"}, {330, "1F"}, {102, "}"}};
    extras.common = {{8, "Walls"}, {62, "1"}};
    extras.specific = {{100, "AcDbLine"}};
    extras.placement = {{30, "5.0"}, {39, "2.5"}};
    extras.extended = {{1001, "ACAD"}, {1000, "note"}};
    extras.flippedExtrusion = true;
    const EntityRecord original{EntityId{0x2A}, segment({0.0, 0.0}, {1.0, 1.0}), extras, 7};

    Document document;
    document.insert(original);
    ASSERT_NE(document.find(EntityId{0x2A}), nullptr);
    EXPECT_EQ(*document.find(EntityId{0x2A}), original);

    EntityRecord removed = removeOrFail(document, EntityId{0x2A});
    EXPECT_EQ(removed, original);
    document.restore(std::move(removed));
    ASSERT_NE(document.find(EntityId{0x2A}), nullptr);
    EXPECT_EQ(*document.find(EntityId{0x2A}), original);
}

TEST(Document, UnitsAreMillimetresByDefault) {
    Document document;
    EXPECT_EQ(document.units(), 4);
    document.setUnits(1);
    EXPECT_EQ(document.units(), 1);
    document.setUnits(0);
    EXPECT_EQ(document.units(), 0);
    document.setUnits(24);
    EXPECT_EQ(document.units(), 24);
}

TEST(Document, ANewDocumentHasNoRawData) {
    const Document document;
    EXPECT_EQ(document.raw(), RawDrawing{});
    EXPECT_TRUE(document.rawEntities().empty());
    EXPECT_EQ(document.nextId(), 1U);
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

// --- Tables and properties (docs/adr/0023) --------------------------------

/// A layer named `name`, with the defaults of a new layer and `linetype`.
Layer namedLayer(std::string name, LinetypeId linetype) {
    Layer layer;
    layer.name = std::move(name);
    layer.linetype = linetype;
    return layer;
}

/// The value of `optional`, failing the test if it is empty.
template <typename T>
T valueOrFail(std::optional<T> optional) {
    if (!optional) {
        ADD_FAILURE() << "the record is not in the document";
        return {};
    }
    return std::move(*optional);
}

/// The names of the layers, in table order.
std::vector<std::string> layerNames(const Document& document) {
    std::vector<std::string> names;
    for (const Layer& layer : document.layers()) {
        names.push_back(layer.name);
    }
    return names;
}

TEST(DocumentTables, NewDrawingHasTheStartRecordsWithTheirHandles) {
    const Document document = Document::newDrawing();

    ASSERT_EQ(document.linetypes().size(), 3U);
    EXPECT_EQ(document.linetypes()[0].name, "ByBlock");
    EXPECT_EQ(document.linetypes()[0].id.value, start_handles::kByBlock);
    EXPECT_EQ(document.linetypes()[1].name, "ByLayer");
    EXPECT_EQ(document.linetypes()[1].id.value, start_handles::kByLayer);
    EXPECT_EQ(document.linetypes()[2].name, "Continuous");
    EXPECT_EQ(document.linetypes()[2].id.value, start_handles::kContinuous);
    for (const Linetype& linetype : document.linetypes()) {
        EXPECT_TRUE(linetype.pattern.continuous()) << linetype.name;
    }
    EXPECT_EQ(document.byBlockLinetype().value, 256U);
    EXPECT_EQ(document.byLayerLinetype().value, 255U);
    EXPECT_EQ(document.continuousLinetype().value, 254U);

    ASSERT_EQ(document.layers().size(), 1U);
    const Layer& zero = document.layers()[0];
    EXPECT_EQ(zero.name, "0");
    EXPECT_EQ(zero.id.value, start_handles::kLayerZero);
    EXPECT_EQ(zero.color, Color::fromIndex(7));
    EXPECT_EQ(zero.linetype, document.continuousLinetype());
    EXPECT_EQ(zero.lineweight, lineweight::kDefault);
    EXPECT_TRUE(zero.on);
    EXPECT_FALSE(zero.frozen);
    EXPECT_FALSE(zero.locked);
    EXPECT_TRUE(zero.plot);
    EXPECT_EQ(document.layerZero().value, 253U);
    EXPECT_EQ(document.layer(LayerId{253}), &zero);

    EXPECT_EQ(document.current().layer, document.layerZero());
    EXPECT_EQ(document.current().color, Color::byLayer());
    EXPECT_EQ(document.current().linetype, document.byLayerLinetype());
    EXPECT_EQ(document.current().lineweight, lineweight::kByLayer);
    EXPECT_DOUBLE_EQ(document.current().linetypeScale, 1.0);
    EXPECT_DOUBLE_EQ(document.ltscale(), 1.0);
    EXPECT_EQ(document.lwdefault(), 25);
    EXPECT_FALSE(document.lwdisplay());

    EXPECT_TRUE(document.isEmpty());
    EXPECT_TRUE(document.rawEntities().empty());
    EXPECT_EQ(document.nextId(), 257U);
    EXPECT_EQ(start_handles::kFirstEntity, 257U);
}

TEST(DocumentTables, TheFirstEntityOfANewDrawingIs257) {
    Document document = Document::newDrawing();
    EXPECT_EQ(document.add(segment({0.0, 0.0}, {1.0, 0.0})).value, 257U);
    EXPECT_EQ(document.add(segment({0.0, 1.0}, {1.0, 1.0})).value, 258U);
}

TEST(DocumentTables, ANewDrawingLeavesTheOtherLowHandlesFree) {
    // Every handle from 1 to 256 that the map does not give can still be
    // inserted: insert() requires it unused.
    Document document = Document::newDrawing();
    for (std::uint64_t handle = 1; handle < start_handles::kLowest; ++handle) {
        document.insert(
            EntityRecord{EntityId{handle}, segment({0.0, 0.0}, {1.0, 0.0}), {}, handle});
    }
    EXPECT_EQ(document.entityCount(), start_handles::kLowest - 1);
    EXPECT_EQ(document.nextId(), 257U);
    const EntityId highest{start_handles::kLowest - 1};
    ASSERT_NE(document.find(highest), nullptr);
    EXPECT_EQ(document.find(highest)->id, highest);
}

TEST(DocumentTables, AnEmptyDocumentHasNoRecords) {
    Document document;
    EXPECT_TRUE(document.layers().empty());
    EXPECT_TRUE(document.linetypes().empty());
    EXPECT_EQ(document.nextId(), 1U);
    EXPECT_EQ(document.layerZero(), LayerId{});
    EXPECT_EQ(document.byLayerLinetype(), LinetypeId{});
    EXPECT_EQ(document.byBlockLinetype(), LinetypeId{});
    EXPECT_EQ(document.continuousLinetype(), LinetypeId{});
    EXPECT_EQ(document.current(), CurrentProperties{});
    EXPECT_EQ(document.findLayer("0"), nullptr);
    EXPECT_EQ(document.layer(LayerId{253}), nullptr);
    EXPECT_DOUBLE_EQ(document.ltscale(), 1.0);
    EXPECT_EQ(document.lwdefault(), 25);
    EXPECT_FALSE(document.lwdisplay());

    // The entities of an empty document keep the identifiers of the past.
    EXPECT_EQ(document.add(segment({0.0, 0.0}, {1.0, 0.0})).value, 1U);
    EXPECT_EQ(document.entities()[0].properties, EntityProperties{});
}

TEST(DocumentTables, ARecordReadWithALowHandleConflictsWithNothing) {
    // As a file read: layer 0 at 0x10, as AutoCAD writes it, and a linetype
    // at 253, the handle of layer 0 in a new drawing.
    Document document;
    document.insert(EntityRecord{EntityId{1}, segment({0.0, 0.0}, {1.0, 0.0}), {}, 1});
    const LinetypeId continuous =
        document.addLinetype(Linetype{LinetypeId{253}, "Continuous", "Solid line", {}, {}});
    Layer zero = namedLayer("0", continuous);
    zero.id = LayerId{0x10};
    EXPECT_EQ(document.addLayer(zero).value, 0x10U);

    EXPECT_EQ(continuous.value, 253U);
    EXPECT_EQ(document.layerZero().value, 0x10U);
    EXPECT_EQ(document.continuousLinetype().value, 253U);
    EXPECT_EQ(document.find(EntityId{1})->id.value, 1U);
    // The counter moves past every handle used, as for reserved handles.
    EXPECT_EQ(document.nextId(), 254U);
    EXPECT_EQ(document.add(segment({0.0, 1.0}, {1.0, 1.0})).value, 254U);
}

TEST(DocumentTables, AddGivesTheCurrentProperties) {
    Document document = Document::newDrawing();
    const EntityId first = document.add(segment({0.0, 0.0}, {1.0, 0.0}));
    EXPECT_EQ(document.find(first)->properties, document.current());

    const LayerId walls = document.addLayer(namedLayer("Walls", document.continuousLinetype()));
    CurrentProperties current;
    current.layer = walls;
    current.color = Color::fromIndex(1);
    current.linetype = document.continuousLinetype();
    current.lineweight = 50;
    current.linetypeScale = 2.0;
    document.setCurrent(current);
    EXPECT_EQ(document.current(), current);

    const EntityId second = document.add(segment({0.0, 1.0}, {1.0, 1.0}));
    EXPECT_EQ(document.find(second)->properties, current);
    // The entity added before keeps its own.
    EXPECT_EQ(document.find(first)->properties.layer, document.layerZero());
    EXPECT_EQ(document.find(first)->properties.color, Color::byLayer());
}

TEST(DocumentTables, NamesAreFoundWithoutCase) {
    Document document = Document::newDrawing();
    const LayerId walls = document.addLayer(namedLayer("Walls", document.continuousLinetype()));
    ASSERT_NE(document.findLayer("WALLS"), nullptr);
    EXPECT_EQ(document.findLayer("walls")->id, walls);
    EXPECT_EQ(document.findLayer("Walls")->name, "Walls");
    EXPECT_EQ(document.findLayer("Wall"), nullptr);
    ASSERT_NE(document.findLinetype("CONTINUOUS"), nullptr);
    EXPECT_EQ(document.findLinetype("bylayer")->id, document.byLayerLinetype());
    EXPECT_EQ(document.findLinetype("Dashed"), nullptr);
}

TEST(DocumentTables, AddedRecordsGetNewIdentifiersAtTheEnd) {
    Document document = Document::newDrawing();
    const LinetypeId dashed =
        document.addLinetype(Linetype{{}, "Dashed", "__ __", LinetypePattern{{5.0, -2.5}}, {}});
    const LayerId walls = document.addLayer(namedLayer("Walls", dashed));
    EXPECT_EQ(dashed.value, 257U);
    EXPECT_EQ(walls.value, 258U);
    EXPECT_EQ(document.linetypes().back().id, dashed);
    EXPECT_EQ(document.layers().back().id, walls);
    EXPECT_EQ(document.layer(walls)->linetype, dashed);
    EXPECT_EQ(document.linetype(dashed)->pattern.elements.size(), 2U);
    EXPECT_EQ(document.add(segment({0.0, 0.0}, {1.0, 0.0})).value, 259U);
}

TEST(DocumentTables, RemoveAndInsertLayerAtTheSamePosition) {
    Document document = Document::newDrawing();
    const LinetypeId continuous = document.continuousLinetype();
    document.addLayer(namedLayer("A", continuous));
    Layer b = namedLayer("B", continuous);
    b.color = Color::fromRgb({10, 20, 30});
    b.locked = true;
    b.extras = {RawGroup{1001, "APP"}};
    const LayerId bId = document.addLayer(b);
    document.addLayer(namedLayer("C", continuous));
    const std::vector<Layer> before{document.layers().begin(), document.layers().end()};

    auto [removed, position] = valueOrFail(document.removeLayer(bId));
    EXPECT_EQ(position, 2U);
    EXPECT_EQ(removed.name, "B");
    EXPECT_EQ(removed.extras, b.extras);
    EXPECT_EQ(layerNames(document), (std::vector<std::string>{"0", "A", "C"}));
    EXPECT_EQ(document.layer(bId), nullptr);
    EXPECT_FALSE(document.removeLayer(bId).has_value());

    document.insertLayer(std::move(removed), position);
    EXPECT_EQ(std::vector<Layer>(document.layers().begin(), document.layers().end()), before);
}

TEST(DocumentTables, RemoveAndInsertLinetypeAtTheSamePosition) {
    Document document = Document::newDrawing();
    const LinetypeId dashed =
        document.addLinetype(Linetype{{}, "Dashed", "", LinetypePattern{{5.0, -2.5}}, {}});
    auto [removed, position] = valueOrFail(document.removeLinetype(document.continuousLinetype()));
    EXPECT_EQ(position, 2U);
    EXPECT_EQ(document.continuousLinetype(), LinetypeId{});
    EXPECT_EQ(document.linetypes()[2].id, dashed);

    document.insertLinetype(std::move(removed), position);
    EXPECT_EQ(document.continuousLinetype().value, 254U);
    EXPECT_EQ(document.linetypes()[2].name, "Continuous");
    EXPECT_EQ(document.linetypes()[3].id, dashed);
}

TEST(DocumentTables, IdentifiersAreNeverReused) {
    Document document = Document::newDrawing();
    const EntityId entity = document.add(segment({0.0, 0.0}, {1.0, 0.0}));
    EXPECT_EQ(entity.value, 257U);
    ASSERT_TRUE(document.remove(entity).has_value());

    const LayerId walls = document.addLayer(namedLayer("Walls", document.continuousLinetype()));
    EXPECT_EQ(walls.value, 258U);
    ASSERT_TRUE(document.removeLayer(walls).has_value());

    // A removed layer leaves its name free, never its identifier.
    const LayerId again = document.addLayer(namedLayer("Walls", document.continuousLinetype()));
    EXPECT_EQ(again.value, 259U);
    EXPECT_EQ(document.add(segment({0.0, 0.0}, {1.0, 0.0})).value, 260U);
}

TEST(DocumentTables, ReplaceLayerChangesEveryFieldButTheIdentifier) {
    Document document = Document::newDrawing();
    const LayerId walls = document.addLayer(namedLayer("Walls", document.continuousLinetype()));
    Layer changed = *document.layer(walls);
    changed.name = "WALLS";  // the same name in another case is not a double
    changed.color = Color::fromIndex(3);
    changed.lineweight = 35;
    changed.on = false;
    changed.frozen = true;
    changed.plot = false;
    document.replaceLayer(changed);
    EXPECT_EQ(*document.layer(walls), changed);
    EXPECT_EQ(document.layers().size(), 2U);
    EXPECT_EQ(document.layers()[1].id, walls);
}

TEST(DocumentTables, SetPropertiesAndRemoveRestoreKeepThem) {
    Document document = Document::newDrawing();
    const LayerId walls = document.addLayer(namedLayer("Walls", document.continuousLinetype()));
    const EntityId id = document.add(segment({0.0, 0.0}, {1.0, 0.0}));

    EntityProperties properties;
    properties.layer = walls;
    properties.color = Color::fromRgb({1, 2, 3});
    properties.linetype = document.byBlockLinetype();
    properties.lineweight = lineweight::kByBlock;
    properties.linetypeScale = 0.5;
    document.setProperties(id, properties);
    EXPECT_EQ(document.find(id)->properties, properties);

    EntityRecord removed = removeOrFail(document, id);
    EXPECT_EQ(removed.properties, properties);
    document.restore(std::move(removed));
    EXPECT_EQ(document.find(id)->properties, properties);
}

TEST(DocumentTables, CurrentValuesChange) {
    Document document = Document::newDrawing();
    document.setLtscale(2.5);
    document.setLwdefault(18);
    document.setLwdisplay(true);
    EXPECT_DOUBLE_EQ(document.ltscale(), 2.5);
    EXPECT_EQ(document.lwdefault(), 18);
    EXPECT_TRUE(document.lwdisplay());
}

}  // namespace
}  // namespace tg::core
