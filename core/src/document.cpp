// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

namespace tg::core {

BBox boundingBox(const Entity& entity) {
    return std::visit([](const auto& concrete) { return concrete.boundingBox(); }, entity);
}

EntityId Document::add(Entity entity) {
    const EntityId id{m_nextId++};
    m_entities.push_back(EntityRecord{id, std::move(entity)});
    return id;
}

// Linear search: enough for now. A spatial index and an id lookup table
// arrive with selection in stage 0.3 (docs/tappe/0.3-selezione-e-snap.md).
const EntityRecord* Document::find(EntityId id) const {
    const auto it = std::ranges::find(m_entities, id, &EntityRecord::id);
    return it != m_entities.end() ? &*it : nullptr;
}

std::optional<EntityRecord> Document::remove(EntityId id) {
    const auto it = std::ranges::find(m_entities, id, &EntityRecord::id);
    if (it == m_entities.end()) {
        return std::nullopt;
    }
    EntityRecord removed = std::move(*it);
    m_entities.erase(it);
    return removed;
}

void Document::restore(EntityRecord record) {
    assert(record.id.value != 0 && find(record.id) == nullptr);
    // Keep the "never reused" guarantee even for records that did not come
    // from this document's own counter.
    m_nextId = std::max(m_nextId, record.id.value + 1);
    m_entities.push_back(std::move(record));
}

BBox Document::extents() const {
    BBox box;
    for (const EntityRecord& record : m_entities) {
        box.expand(boundingBox(record.entity));
    }
    return box;
}

}  // namespace tg::core
