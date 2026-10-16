// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/polyline.hpp"

#include <cstdint>
#include <span>
#include <variant>
#include <vector>

namespace tg::core {

/// Every drawable entity. New entity types are added to this variant;
/// code that visits entities then fails to compile until it handles them,
/// which is intentional (see docs/adr/0004).
using Entity = std::variant<Polyline>;

/// Stable identifier of an entity inside its document. Never reused.
struct EntityId {
    std::uint64_t value{0};

    friend constexpr bool operator==(const EntityId&, const EntityId&) = default;
};

struct EntityRecord {
    EntityId id;
    Entity entity;
};

[[nodiscard]] BBox boundingBox(const Entity& entity);

/// A drawing: the owner of all entities.
class Document {
public:
    EntityId add(Entity entity);

    [[nodiscard]] std::span<const EntityRecord> entities() const { return m_entities; }

    [[nodiscard]] std::size_t entityCount() const { return m_entities.size(); }

    [[nodiscard]] bool isEmpty() const { return m_entities.empty(); }

    /// Bounding box of every entity (empty for an empty document).
    [[nodiscard]] BBox extents() const;

private:
    std::vector<EntityRecord> m_entities;
    std::uint64_t m_nextId{1};
};

}  // namespace tg::core
