// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

namespace tg::core {

BBox boundingBox(const Entity& entity) {
    return std::visit([](const auto& concrete) { return concrete.boundingBox(); }, entity);
}

EntityId Document::add(Entity entity) {
    const EntityId id{m_nextId++};
    append(EntityRecord{id, std::move(entity)});
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
    // the DXF file: the entities that follow move back by one.
    const auto erased =
        m_entities.erase(m_entities.begin() + static_cast<std::ptrdiff_t>(position));
    for (auto moved = erased; moved != m_entities.end(); ++moved) {
        --m_positions[moved->id.value];
    }
    return removed;
}

void Document::restore(EntityRecord record) {
    assert(record.id.value != 0 && find(record.id) == nullptr);
    // Keep the "never reused" guarantee even for records that did not come
    // from this document's own counter.
    m_nextId = std::max(m_nextId, record.id.value + 1);
    append(std::move(record));
}

BBox Document::extents() const {
    return m_index.bounds();
}

void Document::query(const BBox& area, std::vector<EntityId>& out) const {
    m_index.query(area, out);
}

void Document::append(EntityRecord record) {
    const BBox box = boundingBox(record.entity);
    if (!box.isEmpty()) {
        m_index.insert(record.id, box);
    }
    m_positions.emplace(record.id.value, m_entities.size());
    m_entities.push_back(std::move(record));
}

}  // namespace tg::core
