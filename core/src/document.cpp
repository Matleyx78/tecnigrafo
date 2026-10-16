// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"

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

BBox Document::extents() const {
    BBox box;
    for (const EntityRecord& record : m_entities) {
        box.expand(boundingBox(record.entity));
    }
    return box;
}

}  // namespace tg::core
