// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <utility>

namespace tg::core {

BBox boundingBox(const Entity& entity) {
    return std::visit([](const auto& concrete) { return concrete.boundingBox(); }, entity);
}

namespace {

// add() gives the value after the highest one, so the highest is not usable.
constexpr std::uint64_t kLastUsable = std::numeric_limits<std::uint64_t>::max() - 1;

}  // namespace

EntityId Document::add(Entity entity) {
    assert(m_nextId <= kLastUsable && m_nextOrder <= kLastUsable);
    const EntityId id{m_nextId++};
    place(EntityRecord{id, std::move(entity), EntityExtras{}, m_nextOrder++});
    return id;
}

const EntityRecord* Document::find(EntityId id) const {
    const auto it = m_positions.find(id.value);
    return it != m_positions.end() ? &m_entities[it->second] : nullptr;
}

std::optional<EntityRecord> Document::remove(EntityId id) {
    const auto it = m_positions.find(id.value);
    if (it == m_positions.end()) {
        return std::nullopt;
    }
    const std::size_t position = it->second;
    m_positions.erase(it);

    EntityRecord removed = std::move(m_entities[position]);
    // The same entity gives the same box it was indexed with.
    const BBox box = boundingBox(removed.entity);
    if (!box.isEmpty()) {
        [[maybe_unused]] const bool indexed = m_index.erase(id, box);
        assert(indexed && "an entity with a box is always in the index");
    }

    // Erasing keeps the order of the drawing, which is also the order of
    // the DXF file: the entities that follow move back by one. The record
    // keeps its key, so restore() puts it back where it was.
    const auto erased =
        m_entities.erase(m_entities.begin() + static_cast<std::ptrdiff_t>(position));
    for (auto moved = erased; moved != m_entities.end(); ++moved) {
        --m_positions[moved->id.value];
    }
    return removed;
}

void Document::restore(EntityRecord record) {
    assert(record.id.value != 0 && !isUsed(record.id));
    // Keep the "never reused" guarantee even for records that did not come
    // from this document's own counter.
    advancePast(record.id, record.order);
    place(std::move(record));
}

void Document::insert(EntityRecord record) {
    assert(record.id.value != 0 && !isUsed(record.id));
    advancePast(record.id, record.order);
    place(std::move(record));
}

void Document::insertRaw(RawEntity raw) {
    assert(raw.id.value != 0 && !isUsed(raw.id));
    advancePast(raw.id, raw.order);
    m_otherIds.insert(raw.id.value);
    // After the entities with the same key: equal keys keep insertion order.
    const auto where = std::ranges::upper_bound(m_rawEntities, raw.order, {}, &RawEntity::order);
    m_rawEntities.insert(where, std::move(raw));
}

void Document::reserveHandle(EntityId id) {
    assert(id.value != 0);
    m_otherIds.insert(id.value);
    advancePast(id, 0);
}

void Document::setNextId(std::uint64_t next) {
    m_nextId = std::max(m_nextId, next);
}

void Document::setUnits(int units) {
    assert(units >= 0 && units <= kMaxUnits);
    m_units = units;
}

void Document::setRaw(RawDrawing raw) {
    m_raw = std::move(raw);
}

BBox Document::extents() const {
    return m_index.bounds();
}

void Document::query(const BBox& area, std::vector<EntityId>& out) const {
    m_index.query(area, out);
}

bool Document::isUsed(EntityId id) const {
    return m_positions.contains(id.value) || m_otherIds.contains(id.value);
}

void Document::advancePast(EntityId id, std::uint64_t order) {
    assert(id.value <= kLastUsable && order <= kLastUsable);
    m_nextId = std::max(m_nextId, id.value + 1);
    m_nextOrder = std::max(m_nextOrder, order + 1);
}

void Document::place(EntityRecord record) {
    const BBox box = boundingBox(record.entity);
    if (!box.isEmpty()) {
        m_index.insert(record.id, box);
    }
    // After the entities with the same key: equal keys keep insertion order.
    // add() and a file read in order always land at the end; only restore()
    // lands in the middle, and moves the entities that follow by one.
    const auto where = std::ranges::upper_bound(m_entities, record.order, {}, &EntityRecord::order);
    const auto position = static_cast<std::size_t>(where - m_entities.begin());
    const auto inserted = m_entities.insert(where, std::move(record));
    for (auto moved = inserted + 1; moved != m_entities.end(); ++moved) {
        ++m_positions[moved->id.value];
    }
    m_positions.emplace(inserted->id.value, position);
}

}  // namespace tg::core
