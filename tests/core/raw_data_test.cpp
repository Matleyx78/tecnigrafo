// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"
#include "tg/core/entity_extras.hpp"
#include "tg/core/raw_drawing.hpp"
#include "tg/core/raw_group.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

Polyline segment(const Vec2& from, const Vec2& to) {
    Polyline polyline;
    polyline.addVertex(from);
    polyline.addVertex(to);
    return polyline;
}

/// An unhandled HATCH far from every handled entity, with its handle.
RawEntity hatch(std::uint64_t handle, std::uint64_t order) {
    return RawEntity{EntityId{handle}, order, "HATCH",
                     RawGroups{{5, "1A"}, {100, "AcDbEntity"}, {8, "0"}, {10, "1000.0"}}};
}

std::vector<std::uint64_t> handlesOf(const Document& document) {
    std::vector<std::uint64_t> handles;
    for (const RawEntity& raw : document.rawEntities()) {
        handles.push_back(raw.id.value);
    }
    return handles;
}

TEST(RawGroup, KeepsTheValueAsText) {
    const RawGroup group{10, "1.50"};
    EXPECT_EQ(group.code, 10);
    EXPECT_EQ(group.value, "1.50");
    // "1.50" and "1.5" are the same number but not the same text.
    EXPECT_NE(group, (RawGroup{10, "1.5"}));
    EXPECT_NE(group, (RawGroup{20, "1.50"}));
    EXPECT_EQ(group, (RawGroup{10, "1.50"}));
}

TEST(EntityExtras, EmptyOnlyWithoutAnyData) {
    EXPECT_TRUE(EntityExtras{}.empty());

    EntityExtras extras;
    extras.flippedExtrusion = true;
    EXPECT_FALSE(extras.empty());

    for (RawGroups EntityExtras::*const part :
         {&EntityExtras::beforeEntity, &EntityExtras::common, &EntityExtras::specific,
          &EntityExtras::placement, &EntityExtras::extended}) {
        EntityExtras one;
        (one.*part).push_back({8, "Walls"});
        EXPECT_FALSE(one.empty());
        EXPECT_NE(one, EntityExtras{});
    }
}

TEST(EntityExtras, CopiesCompareEqual) {
    EntityExtras extras;
    extras.common = {{8, "Walls"}, {62, "3"}};
    extras.extended = {{1001, "APP"}, {1070, "12"}};
    const EntityExtras copy = extras;
    EXPECT_EQ(copy, extras);

    EntityExtras reordered = extras;
    std::ranges::reverse(reordered.common);
    EXPECT_NE(reordered, extras);
}

TEST(RawDrawing, EmptyForANewDrawingAndComparedByValue) {
    const RawDrawing empty;
    EXPECT_TRUE(empty.version.empty());
    EXPECT_TRUE(empty.codePage.empty());
    EXPECT_TRUE(empty.sections.empty());
    EXPECT_TRUE(empty.nativeParts.empty());

    RawDrawing drawing;
    drawing.version = "AC1015";
    drawing.codePage = "ANSI_1252";
    drawing.sections = {RawSection{"HEADER", {{9, "$EXTMIN"}, {10, "0.0"}}},
                        RawSection{"ENTITIES", {}},
                        RawSection{"OBJECTS", {{0, "DICTIONARY"}, {5, "C"}}}};
    drawing.nativeParts = {NativePart{"thumbnail.png", std::string("\x89PNG\0\x01", 6)}};

    RawDrawing copy = drawing;
    EXPECT_EQ(copy, drawing);
    copy.nativeParts.front().bytes.back() = '\x02';
    EXPECT_NE(copy, drawing);
}

TEST(Document, SetRawKeepsTheDrawingData) {
    RawDrawing drawing;
    drawing.version = "AC1032";
    drawing.sections = {RawSection{"TABLES", {{0, "TABLE"}, {2, "LAYER"}}},
                        RawSection{"ENTITIES", {}}};

    Document document;
    document.setRaw(drawing);
    EXPECT_EQ(document.raw(), drawing);

    const Document copy = document;
    EXPECT_EQ(copy.raw(), drawing);
}

TEST(Document, UnhandledEntitiesStayOutOfTheModel) {
    Document document;
    const EntityId line = document.add(segment({0.0, 0.0}, {1.0, 1.0}));
    document.insertRaw(hatch(0x40, 5));

    ASSERT_EQ(document.entityCount(), 1U);
    EXPECT_EQ(document.entities().front().id, line);
    EXPECT_EQ(document.find(EntityId{0x40}), nullptr);

    std::vector<EntityId> found;
    document.query({{-1e9, -1e9}, {1e9, 1e9}}, found);
    EXPECT_EQ(found, std::vector<EntityId>{line});
    EXPECT_EQ(document.extents().min(), Vec2(0.0, 0.0));
    EXPECT_EQ(document.extents().max(), Vec2(1.0, 1.0));

    ASSERT_EQ(document.rawEntities().size(), 1U);
    EXPECT_EQ(document.rawEntities().front(), hatch(0x40, 5));
    EXPECT_FALSE(document.remove(EntityId{0x40}).has_value());
    EXPECT_EQ(document.rawEntities().size(), 1U);
}

TEST(Document, RawEntitiesComeOutInKeyOrder) {
    Document document;
    document.insertRaw(hatch(0x10, 10));
    document.insertRaw(hatch(0x30, 30));
    document.insertRaw(hatch(0x20, 20));
    EXPECT_EQ(handlesOf(document), (std::vector<std::uint64_t>{0x10, 0x20, 0x30}));
}

TEST(Document, OrderKeysAreSharedWithUnhandledEntities) {
    Document document;
    document.insert(EntityRecord{EntityId{0x2F}, segment({0.0, 0.0}, {1.0, 0.0}), {}, 1});
    document.insertRaw(hatch(0x100, 2));

    // add() goes after both kinds, with a handle higher than both.
    const EntityId added = document.add(segment({0.0, 0.0}, {2.0, 0.0}));
    EXPECT_GT(added.value, 0x100U);
    ASSERT_NE(document.find(added), nullptr);
    EXPECT_GT(document.find(added)->order, 2U);
}

TEST(Document, UnhandledEntitiesAndExtrasSurviveACopy) {
    EntityExtras extras;
    extras.common = {{8, "Doors"}};
    Document document;
    document.insert(EntityRecord{EntityId{0x2F}, segment({0.0, 0.0}, {1.0, 0.0}), extras, 1});
    document.insertRaw(hatch(0x30, 2));
    document.setUnits(6);

    Document copy = document;
    ASSERT_TRUE(copy.remove(EntityId{0x2F}).has_value());
    EXPECT_EQ(document.find(EntityId{0x2F})->extras, extras);
    EXPECT_EQ(copy.rawEntities().size(), 1U);
    EXPECT_EQ(copy.units(), 6);

    const Document moved = std::move(copy);
    EXPECT_EQ(moved.rawEntities().front(), hatch(0x30, 2));
    EXPECT_EQ(moved.nextId(), document.nextId());
}

}  // namespace
}  // namespace tg::core
