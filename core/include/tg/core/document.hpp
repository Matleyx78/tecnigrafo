// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/arc.hpp"
#include "tg/core/attribute_definition.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/block_table.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/dim_style_table.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/entity_extras.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/line.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/raw_drawing.hpp"
#include "tg/core/solid.hpp"
#include "tg/core/spatial_index.hpp"
#include "tg/core/text.hpp"
#include "tg/core/text_style_table.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {

/// Every drawable entity. New entity types are added to this variant;
/// code that visits entities then fails to compile until it handles them,
/// which is intentional (see docs/adr/0004). The types after Text arrive in
/// the 0.8 (docs/adr/0044, point 1).
using Entity = std::variant<Polyline, Line, Circle, Arc, Text, Solid, Ellipse, MText,
                            AttributeDefinition, Insert, Dimension, Hatch>;

struct EntityRecord {
    EntityId id;
    Entity entity;
    /// What the file brought that the model does not represent; empty for
    /// entities made by commands (docs/adr/0018, point 4).
    EntityExtras extras{};
    /// Order key, shared with the unhandled entities: the drawing and file
    /// order (docs/adr/0018, point 2).
    std::uint64_t order{0};
    /// Layer, colour, linetype, lineweight and linetype scale
    /// (docs/adr/0023, point 2). Last, so that the records built by position
    /// as {id, entity, extras, order} keep their meaning.
    EntityProperties properties{};

    friend bool operator==(const EntityRecord&, const EntityRecord&) = default;
};

/// The bounding box of the entity's own geometry. Empty for MText, Insert,
/// Dimension and Hatch, whose geometry depends on the document: their box is
/// extent() (generated.hpp), which the spatial index registers.
/// Precondition: isSupported(entity).
[[nodiscard]] BBox boundingBox(const Entity& entity);

/// False for a type whose package is not merged yet (docs/adr/0044, point 1):
/// supported() of the type. True for the five types of before the 0.8.
[[nodiscard]] bool isSupported(const Entity& entity);

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
/// so that they never disagree with the entities (docs/adr/0007). The index
/// holds the extent() of each entity (generated.hpp), which can depend on the
/// block definitions and on which layers are frozen: replaceBlock(),
/// replaceLayer() and setProperties() register it again when it can change.
///
/// Every entity, handled or not, has an order key. The handled entities are
/// kept in key order, and so are the unhandled ones; the writer merges the
/// two lists by key. Identifiers are never reused: neither those of removed
/// entities nor those reserved by tables, blocks and objects.
///
/// The layer and linetype tables keep their records in table order. A record
/// has its DXF handle as identifier, from the same handle space as the
/// entities (docs/adr/0023, point 1). The document checks only its
/// preconditions (existing identifiers, valid names that are not used twice,
/// layers with an index or RGB colour): the rules on which layers can be
/// renamed or deleted are checked by `edit` (docs/adr/0023, point 5).
class Document {
public:
    /// An empty document: no table records, nextId() 1, and a current layer
    /// and linetype with identifier 0. The reader starts from it, because a
    /// file brings its own handles (docs/adr/0023, point 1).
    Document() = default;

    /// A new drawing: the linetypes ByBlock, ByLayer and Continuous, in this
    /// order, and the layer 0, with the handles of start_handles.hpp. No
    /// other handle from 1 to 256 is used, and the first entity added gets
    /// start_handles::kFirstEntity (docs/adr/0023, point 1).
    [[nodiscard]] static Document newDrawing();

    /// INSUNITS of a new document: millimetres (docs/adr/0021).
    static constexpr int kDefaultUnits = 4;
    /// The highest INSUNITS value (docs/adr/0021).
    static constexpr int kMaxUnits = 24;
    /// LWDEFAULT of a new document: 0.25 mm, as in AutoCAD (docs/adr/0025).
    static constexpr int kDefaultLwdefault = 25;

    /// Adds an entity with a new identifier and the next order key, after
    /// every entity the document has held, so it comes last. The entity gets
    /// the current properties. Precondition: isSupported(entity).
    EntityId add(Entity entity);

    /// The entity with identifier `id`, or nullptr if the document has none.
    /// Only handled entities are found. The pointer is valid until the
    /// document is next modified. Constant time.
    [[nodiscard]] const EntityRecord* find(EntityId id) const;

    /// Removes the entity with identifier `id` and returns it, with its
    /// extras and order key, or returns std::nullopt if the document has
    /// none. The identifier is not reused.
    std::optional<EntityRecord> remove(EntityId id);

    /// Replaces the geometry, keeping identifier, order, extras and properties,
    /// and updates the spatial index. Returns the previous entity.
    /// Preconditions: the record exists; isSupported(entity).
    Entity replaceEntity(EntityId id, Entity entity);

    /// Puts back an entity previously returned by remove(), with its
    /// original identifier, extras and order key, at its place in key order.
    /// Preconditions: `record.id` is valid and not already in the document;
    /// isSupported(record.entity).
    void restore(EntityRecord record);

    /// Adds an entity read from a file, with its handle, order and extras.
    /// Preconditions: record.id is not 0 and not used (handled, raw or
    /// reserved); isSupported(record.entity).
    void insert(EntityRecord record);

    /// Adds an unhandled entity. Same precondition on raw.id.
    void insertRaw(RawEntity raw);

    /// Replaces the unhandled entity with the same handle, at its position.
    /// Preconditions: the entity exists and its order key stays unchanged.
    /// Groups are kept opaque; edit uses this for undoable raw data changes.
    void replaceRaw(const RawEntity& raw);

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

    /// Extent of every entity (empty for an empty document), from the
    /// spatial index: it does not visit the entities.
    [[nodiscard]] BBox extents() const;

    /// Appends to `out` the identifiers of the entities whose extent
    /// overlaps `area`, borders included, in no particular order. Entities
    /// with an empty extent (a polyline without vertices) are never found.
    void query(const BBox& area, std::vector<EntityId>& out) const;

    // --- Tables (docs/adr/0023, point 1) -----------------------------------

    /// The layers, in table order.
    [[nodiscard]] std::span<const Layer> layers() const { return m_layers; }

    /// The layer with identifier `id`, or nullptr. The pointer is valid
    /// until the document is next modified.
    [[nodiscard]] const Layer* layer(LayerId id) const;

    /// The layer named `name` (sameTableName), or nullptr.
    [[nodiscard]] const Layer* findLayer(std::string_view name) const;

    /// The linetypes, in table order.
    [[nodiscard]] std::span<const Linetype> linetypes() const { return m_linetypes; }

    [[nodiscard]] const Linetype* linetype(LinetypeId id) const;

    [[nodiscard]] const Linetype* findLinetype(std::string_view name) const;

    /// The record with that name (sameTableName), or the id 0 if the document
    /// has none, as an empty document.
    [[nodiscard]] LinetypeId byLayerLinetype() const;
    [[nodiscard]] LinetypeId byBlockLinetype() const;
    [[nodiscard]] LinetypeId continuousLinetype() const;
    [[nodiscard]] LayerId layerZero() const;

    // --- Text styles, dimension styles, blocks (docs/adr/0044, point 3) ----

    [[nodiscard]] const TextStyleTable& textStyles() const { return m_textStyles; }

    [[nodiscard]] const DimStyleTable& dimStyles() const { return m_dimStyles; }

    [[nodiscard]] const BlockTable& blocks() const { return m_blocks; }

    // Changed only by edit and io, like the entities.

    void replaceTextStyles(TextStyleTable table);

    void replaceDimStyles(DimStyleTable table);

    /// Puts, replaces or (nullopt) removes the definition `name`; then
    /// re-indexes every entity for which dependsOnBlock() is true.
    ///
    /// A new name is appended to the table. A definition whose name is
    /// already there (sameTableName) takes its place, and keeps the name as
    /// it was written there (docs/DECISIONI.md, 2026-11-14, "Revisione delle
    /// interfacce PT4", P6). Removing a name that is not there changes nothing.
    void replaceBlock(std::string name, std::optional<BlockDefinition> definition);

    // --- Current values (docs/adr/0023, point 3) ---------------------------

    [[nodiscard]] const CurrentProperties& current() const { return m_current; }

    /// LTSCALE, the global linetype scale.
    [[nodiscard]] double ltscale() const { return m_ltscale; }

    /// LWDEFAULT, the lineweight shown for kDefault, in hundredths of a mm.
    [[nodiscard]] int lwdefault() const { return m_lwdefault; }

    /// LWDISPLAY: whether lineweights are shown.
    [[nodiscard]] bool lwdisplay() const { return m_lwdisplay; }

    // Changed only by edit and io, like the entities.

    /// Appends a layer. If layer.id is 0 it gets a new identifier from the
    /// handle counter, never reused; otherwise that handle, which must not
    /// be used. Preconditions: a valid name not used by another layer; an
    /// index or RGB colour; an existing linetype that is neither ByLayer nor
    /// ByBlock; an allowed lineweight or kDefault.
    LayerId addLayer(Layer layer);

    /// Puts a layer back at `position` in the table, as undo does after
    /// removeLayer(). Same preconditions as addLayer() with a non-zero id;
    /// position <= layers().size().
    void insertLayer(Layer layer, std::size_t position);

    /// Removes the layer with identifier `id` and returns it with its
    /// position, or std::nullopt if there is none. The identifier is not
    /// reused.
    std::optional<std::pair<Layer, std::size_t>> removeLayer(LayerId id);

    /// Changes every field of the layer with the same id. Preconditions: the
    /// layer exists, and the new values satisfy those of addLayer(). Freezing
    /// or thawing it registers again the extent of every entity, which can
    /// include shapes on that layer.
    void replaceLayer(const Layer& layer);

    /// Appends a linetype, with the same rules on the identifier as
    /// addLayer(). Precondition: a valid name not used by another linetype.
    LinetypeId addLinetype(Linetype linetype);

    /// Puts a linetype back at `position`, as insertLayer().
    void insertLinetype(Linetype linetype, std::size_t position);

    /// Removes a linetype, as removeLayer().
    std::optional<std::pair<Linetype, std::size_t>> removeLinetype(LinetypeId id);

    /// Precondition: the entity is found, and the layer and linetype of
    /// `properties` exist.
    void setProperties(EntityId id, const EntityProperties& properties);

    /// Precondition: the layer and linetype of `current` exist.
    void setCurrent(const CurrentProperties& current);

    void setLtscale(double scale);  ///< precondition: > 0
    void setLwdefault(int value);   ///< precondition: an allowed value
    void setLwdisplay(bool on);

private:
    /// True when `id` belongs to a handled entity, an unhandled one or a
    /// reserved handle.
    [[nodiscard]] bool isUsed(EntityId id) const;

    /// Moves the identifier and order counters past `id` and `order`, so
    /// that add() never gives them again.
    void advancePast(EntityId id, std::uint64_t order);

    /// Puts the record at its place in key order and registers it.
    void place(EntityRecord record);

    /// Registers the extent of `record` in the spatial index, if not empty.
    void index(const EntityRecord& record);

    /// Removes the entity `id` from the spatial index, if it is there.
    void unindex(EntityId id);

    /// Registers again the extent of the entity at `position`.
    void reindex(std::size_t position);

    /// Gives a table record a new handle if `id` is 0, otherwise registers
    /// it; returns the record's handle.
    std::uint64_t claimRecordHandle(std::uint64_t id);

    /// Asserts the preconditions of a layer with identifier already set.
    void checkLayer(const Layer& layer) const;

    /// Asserts the preconditions of a linetype with identifier already set.
    void checkLinetype(const Linetype& linetype) const;

    /// Asserts that the layer and linetype of `properties` exist.
    void checkProperties(const EntityProperties& properties) const;

    std::vector<EntityRecord> m_entities;                        ///< in order key
    std::unordered_map<std::uint64_t, std::size_t> m_positions;  ///< id -> index in m_entities
    SpatialIndex m_index;  ///< entities with a non-empty extent
    /// The extent each entity of m_index was registered with: it can no
    /// longer be computed from the entity once a block or a layer changes.
    std::unordered_map<std::uint64_t, BBox> m_indexed;
    std::vector<RawEntity> m_rawEntities;          ///< in order key
    std::unordered_set<std::uint64_t> m_otherIds;  ///< unhandled and reserved handles
    std::uint64_t m_nextId{1};
    std::uint64_t m_nextOrder{1};
    int m_units{kDefaultUnits};
    RawDrawing m_raw;
    std::vector<Layer> m_layers;        ///< in table order
    std::vector<Linetype> m_linetypes;  ///< in table order
    CurrentProperties m_current;
    double m_ltscale{1.0};
    int m_lwdefault{kDefaultLwdefault};
    bool m_lwdisplay{false};
    TextStyleTable m_textStyles;
    DimStyleTable m_dimStyles;
    BlockTable m_blocks;
};

}  // namespace tg::core
