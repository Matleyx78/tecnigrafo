// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/spatial_index.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <unordered_map>
#include <variant>
#include <vector>

namespace tg::core {

/// Every drawable entity. New entity types are added to this variant;
/// code that visits entities then fails to compile until it handles them,
/// which is intentional (see docs/adr/0004).
using Entity = std::variant<Polyline>;

struct EntityRecord {
    EntityId id;
    Entity entity;

    friend bool operator==(const EntityRecord&, const EntityRecord&) = default;
};

[[nodiscard]] BBox boundingBox(const Entity& entity);

/// A drawing: the owner of all entities.
///
/// The operations that modify a document (add, remove, restore) are public
/// for technical reasons only: outside module `edit`, documents are changed
/// exclusively through `edit::Editor`, so that every change can be undone
/// (docs/adr/0005).
///
/// The document keeps a spatial index of its entities and a table from
/// identifier to position, both updated by add, remove and restore, so that
/// they never disagree with the entities (docs/adr/0007).
class Document {
public:
    /// Adds an entity with a new identifier, at the end of the entity list.
    EntityId add(Entity entity);

    /// The entity with identifier `id`, or nullptr if the document has none.
    /// The pointer is valid until the document is next modified. Constant
    /// time.
    [[nodiscard]] const EntityRecord* find(EntityId id) const;

    /// Removes the entity with identifier `id` and returns it, or returns
    /// std::nullopt if the document has none. The identifier is not reused.
    std::optional<EntityRecord> remove(EntityId id);

    /// Puts back an entity previously returned by remove(), with its original
    /// identifier, at the end of the entity list.
    /// Precondition: `record.id` is valid and not already in the document.
    void restore(EntityRecord record);

    [[nodiscard]] std::span<const EntityRecord> entities() const { return m_entities; }

    [[nodiscard]] std::size_t entityCount() const { return m_entities.size(); }

    [[nodiscard]] bool isEmpty() const { return m_entities.empty(); }

    /// Bounding box of every entity (empty for an empty document), from the
    /// spatial index: it does not visit the entities.
    [[nodiscard]] BBox extents() const;

    /// Appends to `out` the identifiers of the entities whose bounding box
    /// overlaps `area`, borders included, in no particular order. Entities
    /// with an empty bounding box (a polyline without vertices) are never
    /// found.
    void query(const BBox& area, std::vector<EntityId>& out) const;

private:
    /// Puts the record at the end of the entity list and registers it.
    void append(EntityRecord record);

    std::vector<EntityRecord> m_entities;
    std::unordered_map<std::uint64_t, std::size_t> m_positions;  ///< id -> index in m_entities
    SpatialIndex m_index;  ///< entities with a non-empty bounding box
    std::uint64_t m_nextId{1};
};

}  // namespace tg::core
