// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document.hpp"

#include "tg/core/generated.hpp"
#include "tg/core/start_handles.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {

BBox boundingBox(const Entity& entity) {
    assert(isSupported(entity));
    return std::visit(
        [](const auto& concrete) -> BBox {
            using T = std::decay_t<decltype(concrete)>;
            if constexpr (std::is_same_v<T, Solid> || std::is_same_v<T, Ellipse> ||
                          std::is_same_v<T, AttributeDefinition>) {
                return boundingBox(concrete);
            } else if constexpr (std::is_same_v<T, MText> || std::is_same_v<T, Insert> ||
                                 std::is_same_v<T, Dimension> || std::is_same_v<T, Hatch>) {
                return {};
            } else {
                return concrete.boundingBox();
            }
        },
        entity);
}

bool isSupported(const Entity& entity) {
    return std::visit(
        [](const auto& concrete) {
            using T = std::decay_t<decltype(concrete)>;
            if constexpr (std::is_same_v<T, Polyline> || std::is_same_v<T, Line> ||
                          std::is_same_v<T, Circle> || std::is_same_v<T, Arc> ||
                          std::is_same_v<T, Text>) {
                return true;
            } else {
                return supported(concrete);
            }
        },
        entity);
}

namespace {

// add() gives the value after the highest one, so the highest is not usable.
// Used only by the assertions, absent in Release.
[[maybe_unused]] constexpr std::uint64_t kLastUsable =
    std::numeric_limits<std::uint64_t>::max() - 1;

// The names of the records every drawing has (docs/adr/0025).
constexpr std::string_view kByBlockName = "ByBlock";
constexpr std::string_view kByLayerName = "ByLayer";
constexpr std::string_view kContinuousName = "Continuous";
constexpr std::string_view kLayerZeroName = "0";

// The colour of layer 0 in a new drawing: white, index 7 (docs/adr/0023).
constexpr int kLayerZeroColor = 7;

template <typename Record, typename Id>
const Record* findById(std::span<const Record> records, Id id) {
    const auto found = std::ranges::find(records, id, &Record::id);
    return found != records.end() ? &*found : nullptr;
}

template <typename Record>
const Record* findByName(std::span<const Record> records, std::string_view name) {
    const auto found = std::ranges::find_if(
        records, [name](const Record& record) { return sameTableName(record.name, name); });
    return found != records.end() ? &*found : nullptr;
}

// True when no record other than the one with identifier `id` has `name`.
template <typename Record, typename Id>
bool nameIsFree(std::span<const Record> records, std::string_view name, Id id) {
    return std::ranges::none_of(records, [name, id](const Record& record) {
        return record.id != id && sameTableName(record.name, name);
    });
}

// Removes the record with identifier `id`, with its position.
template <typename Record, typename Id>
std::optional<std::pair<Record, std::size_t>> takeRecord(std::vector<Record>& records, Id id) {
    const auto found = std::ranges::find(records, id, &Record::id);
    if (found == records.end()) {
        return std::nullopt;
    }
    const auto position = static_cast<std::size_t>(found - records.begin());
    Record removed = std::move(*found);
    records.erase(found);
    return std::pair{std::move(removed), position};
}

}  // namespace

Document Document::newDrawing() {
    Document document;
    // In this order in the table, as in AutoCAD (docs/adr/0023, point 1).
    document.addLinetype(Linetype{
        LinetypeId{start_handles::kByBlock}, std::string{kByBlockName}, "", LinetypePattern{}, {}});
    document.addLinetype(Linetype{
        LinetypeId{start_handles::kByLayer}, std::string{kByLayerName}, "", LinetypePattern{}, {}});
    document.addLinetype(Linetype{LinetypeId{start_handles::kContinuous},
                                  std::string{kContinuousName},
                                  "Solid line",
                                  LinetypePattern{},
                                  {}});
    Layer zero;
    zero.id = LayerId{start_handles::kLayerZero};
    zero.name = std::string{kLayerZeroName};
    zero.color = Color::fromIndex(kLayerZeroColor);
    zero.linetype = LinetypeId{start_handles::kContinuous};
    document.addLayer(std::move(zero));

    document.m_current.layer = LayerId{start_handles::kLayerZero};
    document.m_current.linetype = LinetypeId{start_handles::kByLayer};
    document.setNextId(start_handles::kFirstEntity);
    return document;
}

EntityId Document::add(Entity entity) {
    assert(isSupported(entity));
    assert(m_nextId <= kLastUsable && m_nextOrder <= kLastUsable);
    const EntityId id{m_nextId++};
    place(EntityRecord{id, std::move(entity), EntityExtras{}, m_nextOrder++, m_current});
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
    unindex(id);

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

Entity Document::replaceEntity(EntityId id, Entity entity) {
    assert(isSupported(entity));
    const auto it = m_positions.find(id.value);
    assert(it != m_positions.end() && "the replaced entity must exist");
    // A broken precondition must not become undefined behavior in Release.
    if (it == m_positions.end()) {
        std::abort();
    }
    EntityRecord& record = m_entities[it->second];
    unindex(id);
    Entity previous = std::exchange(record.entity, std::move(entity));
    index(record);
    return previous;
}

void Document::restore(EntityRecord record) {
    assert(record.id.value != 0 && !isUsed(record.id));
    assert(isSupported(record.entity));
    // Keep the "never reused" guarantee even for records that did not come
    // from this document's own counter.
    advancePast(record.id, record.order);
    place(std::move(record));
}

void Document::insert(EntityRecord record) {
    assert(record.id.value != 0 && !isUsed(record.id));
    assert(isSupported(record.entity));
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

void Document::replaceRaw(const RawEntity& raw) {
    const auto found = std::ranges::find(m_rawEntities, raw.id, &RawEntity::id);
    assert(found != m_rawEntities.end());
    assert(found->order == raw.order);
    *found = raw;
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

const Layer* Document::layer(LayerId id) const {
    return findById(std::span{m_layers}, id);
}

const Layer* Document::findLayer(std::string_view name) const {
    return findByName(std::span{m_layers}, name);
}

const Linetype* Document::linetype(LinetypeId id) const {
    return findById(std::span{m_linetypes}, id);
}

const Linetype* Document::findLinetype(std::string_view name) const {
    return findByName(std::span{m_linetypes}, name);
}

LinetypeId Document::byLayerLinetype() const {
    const Linetype* found = findLinetype(kByLayerName);
    return found != nullptr ? found->id : LinetypeId{};
}

LinetypeId Document::byBlockLinetype() const {
    const Linetype* found = findLinetype(kByBlockName);
    return found != nullptr ? found->id : LinetypeId{};
}

LinetypeId Document::continuousLinetype() const {
    const Linetype* found = findLinetype(kContinuousName);
    return found != nullptr ? found->id : LinetypeId{};
}

LayerId Document::layerZero() const {
    const Layer* found = findLayer(kLayerZeroName);
    return found != nullptr ? found->id : LayerId{};
}

LayerId Document::addLayer(Layer layer) {
    layer.id = LayerId{claimRecordHandle(layer.id.value)};
    checkLayer(layer);
    m_layers.push_back(std::move(layer));
    return m_layers.back().id;
}

void Document::insertLayer(Layer layer, std::size_t position) {
    assert(layer.id.value != 0 && position <= m_layers.size());
    claimRecordHandle(layer.id.value);
    checkLayer(layer);
    m_layers.insert(m_layers.begin() + static_cast<std::ptrdiff_t>(position), std::move(layer));
}

std::optional<std::pair<Layer, std::size_t>> Document::removeLayer(LayerId id) {
    auto removed = takeRecord(m_layers, id);
    if (removed) {
        // Free for insertLayer(); the counter is already past it, so add()
        // and addLayer() never give it again.
        m_otherIds.erase(id.value);
    }
    return removed;
}

void Document::replaceLayer(const Layer& layer) {
    const auto found = std::ranges::find(m_layers, layer.id, &Layer::id);
    assert(found != m_layers.end());
    checkLayer(layer);
    const bool thawedOrFrozen = found->frozen != layer.frozen;
    *found = layer;
    if (thawedOrFrozen) {
        // The extent leaves out the shapes on frozen layers (generated.hpp).
        for (std::size_t position = 0; position < m_entities.size(); ++position) {
            reindex(position);
        }
    }
}

LinetypeId Document::addLinetype(Linetype linetype) {
    linetype.id = LinetypeId{claimRecordHandle(linetype.id.value)};
    checkLinetype(linetype);
    m_linetypes.push_back(std::move(linetype));
    return m_linetypes.back().id;
}

void Document::insertLinetype(Linetype linetype, std::size_t position) {
    assert(linetype.id.value != 0 && position <= m_linetypes.size());
    claimRecordHandle(linetype.id.value);
    checkLinetype(linetype);
    m_linetypes.insert(m_linetypes.begin() + static_cast<std::ptrdiff_t>(position),
                       std::move(linetype));
}

std::optional<std::pair<Linetype, std::size_t>> Document::removeLinetype(LinetypeId id) {
    auto removed = takeRecord(m_linetypes, id);
    if (removed) {
        m_otherIds.erase(id.value);
    }
    return removed;
}

void Document::setProperties(EntityId id, const EntityProperties& properties) {
    const auto it = m_positions.find(id.value);
    assert(it != m_positions.end());
    // A broken precondition must not become undefined behavior in Release.
    if (it == m_positions.end()) {
        std::abort();
    }
    checkProperties(properties);
    m_entities[it->second].properties = properties;
    reindex(it->second);
}

void Document::replaceTextStyles(TextStyleTable table) {
    m_textStyles = table;
}

void Document::replaceDimStyles(DimStyleTable table) {
    m_dimStyles = table;
}

void Document::replaceBlock(std::string name, std::optional<BlockDefinition> definition) {
    // The entities that depend on the definition before or after the change.
    std::unordered_set<std::uint64_t> dependent;
    const auto collect = [&] {
        for (const EntityRecord& record : m_entities) {
            if (dependsOnBlock(*this, record, name)) {
                dependent.insert(record.id.value);
            }
        }
    };
    collect();
    std::vector<BlockDefinition>& definitions = m_blocks.m_definitions;
    const auto found = std::ranges::find_if(
        definitions, [&](const BlockDefinition& block) { return sameTableName(block.name, name); });
    if (!definition) {
        if (found != definitions.end()) {
            definitions.erase(found);
        }
    } else if (found != definitions.end()) {
        std::string written = std::move(found->name);
        *found = std::move(*definition);
        found->name = std::move(written);
    } else {
        definition->name = name;
        definitions.push_back(std::move(*definition));
    }
    collect();
    for (const std::uint64_t id : dependent) {
        reindex(m_positions.at(id));
    }
}

void Document::setCurrent(const CurrentProperties& current) {
    checkProperties(current);
    m_current = current;
}

void Document::setLtscale(double scale) {
    assert(scale > 0.0);
    m_ltscale = scale;
}

void Document::setLwdefault(int value) {
    assert(isAllowedLineweight(value));
    m_lwdefault = value;
}

void Document::setLwdisplay(bool on) {
    m_lwdisplay = on;
}

std::uint64_t Document::claimRecordHandle(std::uint64_t id) {
    if (id == 0) {
        assert(m_nextId <= kLastUsable);
        id = m_nextId++;
    } else {
        assert(!isUsed(EntityId{id}));
        advancePast(EntityId{id}, 0);
    }
    m_otherIds.insert(id);
    return id;
}

void Document::checkLayer([[maybe_unused]] const Layer& layer) const {
    assert(isValidTableName(layer.name));
    assert(nameIsFree(std::span{m_layers}, layer.name, layer.id));
    assert(layer.color.kind() == Color::Kind::Index || layer.color.kind() == Color::Kind::Rgb);
    assert(linetype(layer.linetype) != nullptr);
    assert(layer.linetype != byLayerLinetype() && layer.linetype != byBlockLinetype());
    assert(layer.lineweight == lineweight::kDefault || isAllowedLineweight(layer.lineweight));
}

void Document::checkLinetype([[maybe_unused]] const Linetype& linetype) const {
    assert(isValidTableName(linetype.name));
    assert(nameIsFree(std::span{m_linetypes}, linetype.name, linetype.id));
}

void Document::checkProperties([[maybe_unused]] const EntityProperties& properties) const {
    assert(layer(properties.layer) != nullptr);
    assert(linetype(properties.linetype) != nullptr);
    assert(properties.linetypeScale > 0.0);
    assert(properties.lineweight == lineweight::kByLayer ||
           properties.lineweight == lineweight::kByBlock ||
           properties.lineweight == lineweight::kDefault ||
           isAllowedLineweight(properties.lineweight));
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
    index(record);
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

void Document::index(const EntityRecord& record) {
    const BBox box = extent(*this, record);
    if (!box.isEmpty()) {
        m_index.insert(record.id, box);
        m_indexed.insert_or_assign(record.id.value, box);
    }
}

void Document::unindex(EntityId id) {
    const auto found = m_indexed.find(id.value);
    if (found == m_indexed.end()) {
        return;
    }
    [[maybe_unused]] const bool indexed = m_index.erase(id, found->second);
    assert(indexed && "an entity with an extent is always in the index");
    m_indexed.erase(found);
}

void Document::reindex(std::size_t position) {
    const EntityRecord& record = m_entities[position];
    const BBox box = extent(*this, record);
    const auto found = m_indexed.find(record.id.value);
    const bool same = found == m_indexed.end()
                          ? box.isEmpty()
                          : !box.isEmpty() && box.min() == found->second.min() &&
                                box.max() == found->second.max();
    if (!same) {
        unindex(record.id);
        index(record);
    }
}

}  // namespace tg::core
