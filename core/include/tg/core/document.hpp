// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/arc.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/entity_extras.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/line.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/raw_drawing.hpp"
#include "tg/core/spatial_index.hpp"
#include "tg/core/text.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace tg::core {

/// Every drawable entity. New entity types are added to this variant;
/// code that visits entities then fails to compile until it handles them,
/// which is intentional (see docs/adr/0004).
using Entity = std::variant<Polyline, Line, Circle, Arc, Text>;

struct EntityRecord {
    EntityId id;
    Entity entity;
    /// What the file brought that the model does not represent; empty for
    /// entities made by commands (docs/adr/0018, point 4).
    EntityExtras extras{};
    /// Order key, shared with the unhandled entities: the drawing and file
    /// order (docs/adr/0018, point 2).
    std::uint64_t order{0};

    friend bool operator==(const EntityRecord&, const EntityRecord&) = default;
};

[[nodiscard]] BBox boundingBox(const Entity& entity);

/// A drawing: the owner of all entities, handled or not, and of what its
/// file brought that the model does not represent (docs/adr/0018).
///
/// The operations that modify a document (add, remove, restore, insert, …)
/// are public for technical reasons only: outside modules `edit` and `io`,
/// documents are changed exclusively through `edit::Editor`, so that every
/// change can be undone (docs/adr/0005).
///
/// The document keeps a spatial index of its entities and a table from
/// identifier to position, both updated by add, remove, restore and insert,
/// so that they never disagree with the entities (docs/adr/0007).
///
/// Every entity, handled or not, has an order key. The handled entities are
/// kept in key order, and so are the unhandled ones; the writer merges the
/// two lists by key. Identifiers are never reused: neither those of removed
/// entities nor those reserved by tables, blocks and objects.
class Document {
public:
    /// INSUNITS of a new document: millimetres (docs/adr/0021).
    static constexpr int kDefaultUnits = 4;
    /// The highest INSUNITS value (docs/adr/0021).
    static constexpr int kMaxUnits = 24;

    /// Adds an entity with a new identifier and the next order key, after
    /// every entity the document has held, so it comes last.
    EntityId add(Entity entity);

    /// The entity with identifier `id`, or nullptr if the document has none.
    /// Only handled entities are found. The pointer is valid until the
    /// document is next modified. Constant time.
    [[nodiscard]] const EntityRecord* find(EntityId id) const;

    /// Removes the entity with identifier `id` and returns it, with its
    /// extras and order key, or returns std::nullopt if the document has
    /// none. The identifier is not reused.
    std::optional<EntityRecord> remove(EntityId id);

    /// Puts back an entity previously returned by remove(), with its
    /// original identifier, extras and order key, at its place in key order.
    /// Precondition: `record.id` is valid and not already in the document.
    void restore(EntityRecord record);

    /// Adds an entity read from a file, with its handle, order and extras.
    /// Precondition: record.id is not 0 and not used (handled, raw or reserved).
    void insert(EntityRecord record);

    /// Adds an unhandled entity. Same precondition on raw.id.
    void insertRaw(RawEntity raw);

    /// The unhandled entities, in order. They are not in entities(), the
    /// spatial index, extents() or query().
    [[nodiscard]] std::span<const RawEntity> rawEntities() const { return m_rawEntities; }

    /// Marks a handle as used by a table, block or object, so that add()
    /// never takes it.
    void reserveHandle(EntityId id);

    /// The identifier add() will give next; setNextId() only moves it forward.
    [[nodiscard]] std::uint64_t nextId() const { return m_nextId; }

    void setNextId(std::uint64_t next);

    /// INSUNITS, 0 to 24 (docs/adr/0021); 4, millimetres, for a new document.
    [[nodiscard]] int units() const { return m_units; }

    void setUnits(int units);  ///< precondition: 0 <= units <= 24

    [[nodiscard]] const RawDrawing& raw() const { return m_raw; }

    void setRaw(RawDrawing raw);

    /// The handled entities, in order.
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
    /// True when `id` belongs to a handled entity, an unhandled one or a
    /// reserved handle.
    [[nodiscard]] bool isUsed(EntityId id) const;

    /// Moves the identifier and order counters past `id` and `order`, so
    /// that add() never gives them again.
    void advancePast(EntityId id, std::uint64_t order);

    /// Puts the record at its place in key order and registers it.
    void place(EntityRecord record);

    std::vector<EntityRecord> m_entities;                        ///< in order key
    std::unordered_map<std::uint64_t, std::size_t> m_positions;  ///< id -> index in m_entities
    SpatialIndex m_index;                          ///< entities with a non-empty bounding box
    std::vector<RawEntity> m_rawEntities;          ///< in order key
    std::unordered_set<std::uint64_t> m_otherIds;  ///< unhandled and reserved handles
    std::uint64_t m_nextId{1};
    std::uint64_t m_nextOrder{1};
    int m_units{kDefaultUnits};
    RawDrawing m_raw;
};

}  // namespace tg::core
