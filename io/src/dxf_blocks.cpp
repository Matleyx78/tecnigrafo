// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The BLOCKS section and the BLOCK_RECORD table (docs/adr/0045, point 1;
// docs/tappe/0.8/PT18-blocchi-dxf.md, points 1, 4, 5 and 6).
//
// The reader takes out of the raw sections the blocks the model holds and
// their records, and leaves a placeholder (kRecordPlaceholder) where each one
// was. After the placeholder of a block its groups stay as read: they are not
// written again, but give the writer the groups of each entity that has not
// changed, which go back as they were (docs/adr/0018, point 6), with the
// spelling of their numbers and the groups AutoCAD leaves out.

#include "dxf_blocks.hpp"

#include "dxf_entity_common.hpp"
#include "dxf_tables.hpp"
#include "tg/core/insert.hpp"
#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <utility>

namespace tg::io {

using dxf_groups::firstOwn;
using dxf_groups::ownGroups;
using dxf_groups::parseInteger;

namespace {

constexpr int kAnonymousBit = 1;
constexpr int kAttributesBit = 2;
constexpr int kExternalBit = 4;

// The order of the groups of BLOCK and of a BLOCK_RECORD in the DXF reference.
constexpr std::array kBlockOrder{2, 70, 10, 20, 30, 3, 1, 4};
constexpr std::array kRecordOrder{2, 340, 70, 280, 281};

bool isMarker(const core::RawGroup& group, std::string_view name) {
    return group.code == 0 && trimDxf(group.value) == name;
}

bool isPlaceholder(const core::RawGroup& group) {
    return group.code == kRecordPlaceholder;
}

std::string plural(std::size_t count, std::string_view singular, std::string_view rest) {
    return std::to_string(count) + " " + std::string{singular} + (count == 1 ? "" : "s") +
           std::string{rest};
}

/// Sets the owner (the first own 330) of `groups` to `owner`, if it has one.
void setOwner(core::RawGroups& groups, std::uint64_t owner) {
    const auto at = firstOwn(groups, ownGroups(groups), 330);
    if (at && parseHandle(groups[*at].value) != owner) {
        groups[*at].value = handleText(owner);
    }
}

/// The handle of an entity: its group 5 before the subclasses, or 0.
std::uint64_t handleOf(const core::RawGroups& groups) {
    for (const auto& group : groups) {
        if (group.code == 5) {
            return parseHandle(group.value).value_or(0);
        }
        if (group.code == 100) {
            break;
        }
    }
    return 0;
}

/// True when the two lists say the same: the same codes, and the same text
/// or the same number in each group.
bool sameGroups(const core::RawGroups& a, const core::RawGroups& b) {
    return std::ranges::equal(a, b, [](const core::RawGroup& x, const core::RawGroup& y) {
        if (x.code != y.code) {
            return false;
        }
        if (x.value == y.value) {
            return true;
        }
        const auto first = parseNumber(trimDxf(x.value));
        return first && first == parseNumber(trimDxf(y.value));
    });
}

/// A block of the BLOCKS groups: from its group 0 BLOCK to the end of the
/// groups of its ENDBLK.
struct Chunk {
    std::size_t begin{0};     ///< the group 0 BLOCK
    std::size_t entities{0};  ///< the first group 0 after BLOCK
    std::size_t endBlock{0};  ///< the group 0 ENDBLK, or `end` when it lacks
    std::size_t end{0};       ///< past the groups of ENDBLK
};

/// The chunk starting at `begin` (a group 0 BLOCK).
Chunk chunkAt(const core::RawGroups& groups, std::size_t begin) {
    Chunk chunk;
    chunk.begin = begin;
    std::size_t i = begin + 1;
    while (i < groups.size() && groups[i].code != 0 && !isPlaceholder(groups[i])) {
        ++i;
    }
    chunk.entities = i;
    while (i < groups.size() && !isMarker(groups[i], "ENDBLK") && !isMarker(groups[i], "BLOCK") &&
           !isMarker(groups[i], "ENDSEC") && !isPlaceholder(groups[i])) {
        ++i;
    }
    if (i == groups.size() || !isMarker(groups[i], "ENDBLK")) {
        chunk.endBlock = chunk.end = i;
        return chunk;
    }
    chunk.endBlock = i++;
    while (i < groups.size() && groups[i].code != 0 && !isPlaceholder(groups[i])) {
        ++i;
    }
    chunk.end = i;
    return chunk;
}

/// The entities between `from` and `to`, each from its group 0.
std::vector<FileEntity> entitiesIn(const core::RawGroups& groups, std::size_t from,
                                   std::size_t to) {
    std::vector<FileEntity> entities;
    for (std::size_t i = from; i < to; ++i) {
        if (groups[i].code == 0 || entities.empty()) {
            entities.push_back({groups[i].code == 0 ? groups[i].value : std::string{}, {}, 0});
            if (groups[i].code == 0) {
                continue;
            }
        }
        entities.back().groups.push_back(groups[i]);
    }
    return entities;
}

/// A block name the table can hold: an anonymous one (*U…) is checked
/// without its star.
bool validBlockName(std::string_view name) {
    return name.starts_with('*') ? core::isValidTableName(name.substr(1))
                                 : core::isValidTableName(name);
}

/// The blocks that stay out of the table (docs/adr/0045, point 1): the
/// layouts and the anonymous blocks of the dimensions.
bool layoutOrDimension(std::string_view name) {
    const std::string key = blockKey(name);
    return key == "*model_space" || key.starts_with("*paper_space") || key.starts_with("*d");
}

/// A BLOCK_RECORD of the TABLES groups.
struct RecordRead {
    std::size_t begin{0};  ///< the group 0 BLOCK_RECORD
    std::size_t end{0};
    std::uint64_t id{0};
    std::string name;
    bool used{false};
};

std::vector<RecordRead> blockRecords(const core::RawGroups& tables) {
    std::vector<RecordRead> records;
    std::string_view table;
    for (std::size_t i = 0; i < tables.size(); ++i) {
        const auto& group = tables[i];
        if (isMarker(group, "TABLE")) {
            table = i + 1 < tables.size() && tables[i + 1].code == 2 ? trimDxf(tables[i + 1].value)
                                                                     : std::string_view{};
            continue;
        }
        if (isMarker(group, "ENDTAB")) {
            table = {};
            continue;
        }
        if (table != "BLOCK_RECORD" || !isMarker(group, "BLOCK_RECORD")) {
            continue;
        }
        RecordRead record;
        record.begin = i;
        std::size_t j = i + 1;
        while (j < tables.size() && tables[j].code != 0 && !isPlaceholder(tables[j])) {
            ++j;
        }
        record.end = j;
        if (i + 1 < j && tables[i + 1].code == 5) {
            record.id = parseHandle(tables[i + 1].value).value_or(0);
        }
        const core::RawGroups body(tables.begin() + static_cast<std::ptrdiff_t>(i + 1),
                                   tables.begin() + static_cast<std::ptrdiff_t>(j));
        if (const auto at = firstOwn(body, ownGroups(body), 2)) {
            record.name = body[*at].value;
        }
        records.push_back(std::move(record));
        i = j - 1;
    }
    return records;
}

/// The integer of the first own group `code` of `groups`, `absent` when there
/// is none, std::nullopt when it is not an integer.
std::optional<long long> integerOf(const core::RawGroups& groups, int code, long long absent) {
    const auto at = firstOwn(groups, ownGroups(groups), code);
    return at ? parseInteger(groups[*at].value) : std::optional<long long>{absent};
}

/// The block definition of `chunk`, without its entities, or std::nullopt
/// when it stays out of the table.
std::optional<core::BlockDefinition> definitionOf(const core::RawGroups& blocks, const Chunk& chunk,
                                                  const RecordRead& record,
                                                  const core::RawGroups& tables) {
    if (chunk.endBlock == chunk.end || chunk.begin + 1 >= chunk.entities ||
        blocks[chunk.begin + 1].code != 5 || chunk.endBlock + 1 >= chunk.end ||
        blocks[chunk.endBlock + 1].code != 5 || record.id == 0) {
        return std::nullopt;
    }
    core::BlockDefinition definition;
    definition.blockId = parseHandle(blocks[chunk.begin + 1].value).value_or(0);
    definition.endBlockId = parseHandle(blocks[chunk.endBlock + 1].value).value_or(0);
    definition.recordId = record.id;
    const core::RawGroups head(blocks.begin() + static_cast<std::ptrdiff_t>(chunk.begin + 2),
                               blocks.begin() + static_cast<std::ptrdiff_t>(chunk.entities));
    const auto own = ownGroups(head);
    const auto name = firstOwn(head, own, 2);
    const auto x = firstOwn(head, own, 10);
    const auto y = firstOwn(head, own, 20);
    const auto flags = integerOf(head, 70, 0);
    if (!name || !x || !y || !flags || definition.blockId == 0 || definition.endBlockId == 0) {
        return std::nullopt;
    }
    definition.name = head[*name].value;
    const auto baseX = parseNumber(trimDxf(head[*x].value));
    const auto baseY = parseNumber(trimDxf(head[*y].value));
    if (!baseX || !baseY || !validBlockName(definition.name) ||
        layoutOrDimension(definition.name) || (*flags & kExternalBit) != 0) {
        return std::nullopt;
    }
    // An external block names its file in group 1 (docs/tappe/0.8, PT18,
    // point 1: "70 bit 4 o 1 xref-dipendente").
    for (std::size_t i = 0; i < head.size(); ++i) {
        if (own[i] && head[i].code == 1 && !trimDxf(head[i].value).empty()) {
            return std::nullopt;
        }
    }
    definition.base = {*baseX, *baseY};
    if (const auto description = firstOwn(head, own, 4)) {
        definition.description = head[*description].value;
    }
    definition.anonymous = (*flags & kAnonymousBit) != 0;
    definition.hasAttributes = (*flags & kAttributesBit) != 0;
    definition.blockExtras = head;
    definition.endBlockExtras.assign(
        blocks.begin() + static_cast<std::ptrdiff_t>(chunk.endBlock + 2),
        blocks.begin() + static_cast<std::ptrdiff_t>(chunk.end));
    // The record: units (70), explodable (280), uniform scale (281).
    definition.recordExtras.assign(tables.begin() + static_cast<std::ptrdiff_t>(record.begin + 2),
                                   tables.begin() + static_cast<std::ptrdiff_t>(record.end));
    const auto units = integerOf(definition.recordExtras, 70, 0);
    const auto explodable = integerOf(definition.recordExtras, 280, 1);
    const auto uniform = integerOf(definition.recordExtras, 281, 0);
    if (!units || *units < 0 || *units > core::Document::kMaxUnits || !explodable || !uniform) {
        return std::nullopt;
    }
    definition.units = static_cast<int>(*units);
    definition.explodable = *explodable != 0;
    definition.uniformScale = *uniform == 1;
    return definition;
}

}  // namespace

bool takeItemProperties(core::EntityRecord& record, BlockEntityReader& reader) {
    if (!reader.properties(record)) {
        return false;
    }
    if (auto* insert = std::get_if<core::Insert>(&record.entity)) {
        for (auto& attribute : insert->attributes) {
            core::EntityRecord groups{attribute.id, core::Text{}, attribute.extras};
            if (!reader.properties(groups)) {
                return false;
            }
            attribute.properties = groups.properties;
        }
    }
    return true;
}

core::RawGroups attributeCommonGroups(const core::Attribute& attribute,
                                      const core::Document& document) {
    core::EntityExtras taken = attribute.extras;
    if (!takeEntityProperties(taken)) {
        taken.common.clear();  // never for groups read: they were valid then
    }
    const core::EntityRecord record{attribute.id, core::Text{}, taken, 0, attribute.properties};
    auto groups = entityCommonGroups(record, document);
    // The writer puts the properties in the order of the DXF reference; the
    // groups read say the same in any order (62, 48, 370 or 62, 370, 48).
    const auto byCode = [](core::RawGroups list) {
        std::ranges::stable_sort(list, {}, &core::RawGroup::code);
        return list;
    };
    if (sameGroups(byCode(groups), byCode(attribute.extras.common))) {
        return attribute.extras.common;
    }
    return groups;
}

BlocksRead readBlocks(core::RawGroups& blocks, core::RawGroups& tables, std::string_view version,
                      std::string_view codePage, int units, BlockEntityReader& reader) {
    BlocksRead read;
    auto records = blockRecords(tables);
    std::unordered_map<std::string, std::size_t> recordByName;
    for (std::size_t i = 0; i < records.size(); ++i) {
        recordByName.emplace(blockKey(records[i].name), i);
    }

    // First the definitions, so that an INSERT finds those that follow it.
    struct Accepted {
        Chunk chunk;
        std::size_t record{0};
    };

    std::vector<Accepted> accepted;
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        if (!isMarker(blocks[i], "BLOCK")) {
            continue;
        }
        const Chunk chunk = chunkAt(blocks, i);
        i = chunk.end - 1;
        std::string name;
        const core::RawGroups head(blocks.begin() + static_cast<std::ptrdiff_t>(chunk.begin + 1),
                                   blocks.begin() + static_cast<std::ptrdiff_t>(chunk.entities));
        if (const auto at = firstOwn(head, ownGroups(head), 2)) {
            name = head[*at].value;
        }
        const auto record = recordByName.find(blockKey(name));
        if (record == recordByName.end() || records[record->second].used ||
            read.names.contains(blockKey(name))) {
            continue;
        }
        auto definition = definitionOf(blocks, chunk, records[record->second], tables);
        if (!definition) {
            continue;
        }
        records[record->second].used = true;
        read.names.insert(blockKey(name));
        read.definitions.push_back(std::move(*definition));
        accepted.push_back({chunk, record->second});
    }
    read.inserts.resize(read.definitions.size());
    for (std::size_t d = 0; d < accepted.size(); ++d) {
        auto& definition = read.definitions[d];
        const Chunk& chunk = accepted[d].chunk;
        auto items = groupItems(entitiesIn(blocks, chunk.entities, chunk.endBlock));
        std::uint64_t order = 0;
        for (auto& item : items) {
            ++order;
            const std::uint64_t id = handleOf(item.front().groups);
            std::optional<core::EntityRecord> record;
            if (id != 0) {
                EntityReadContext context{
                    version, codePage, core::EntityId{definition.recordId}, core::EntityId{id},
                    order,   units};
                record = readItem(item, context, read.names);
            }
            if (record && takeItemProperties(*record, reader)) {
                record->id = core::EntityId{id};
                record->order = order;
                if (std::holds_alternative<core::Insert>(record->entity)) {
                    read.inserts[d].emplace(id, item);
                }
                definition.entities.push_back(std::move(*record));
            } else {
                // Without a handle it stays as written: the writer gives a
                // handled entity one.
                definition.rawEntities.push_back(
                    {core::EntityId{id}, order, item.front().type, rawGroupsOf(item)});
            }
        }
    }
    // The placeholders, from the last position back, so that the positions
    // still to change hold: the records need not be in the order of their
    // blocks.
    std::vector<std::size_t> taken;   // records, which are in table order
    std::vector<std::size_t> starts;  // begin in blocks
    taken.reserve(accepted.size());
    starts.reserve(accepted.size());
    for (const auto& chunk : accepted) {
        taken.push_back(chunk.record);
        starts.push_back(chunk.chunk.begin);
    }
    std::ranges::sort(taken, std::greater<>{});
    std::ranges::sort(starts, std::greater<>{});
    for (const std::size_t index : taken) {
        const auto& record = records[index];
        tables.erase(tables.begin() + static_cast<std::ptrdiff_t>(record.begin),
                     tables.begin() + static_cast<std::ptrdiff_t>(record.end));
        // The placeholder of a record names it, so that each one goes back
        // where it was even when the table order (that of BLOCKS) differs.
        tables.insert(tables.begin() + static_cast<std::ptrdiff_t>(record.begin),
                      {kRecordPlaceholder, handleText(record.id)});
    }
    for (const std::size_t start : starts) {
        blocks.insert(blocks.begin() + static_cast<std::ptrdiff_t>(start),
                      {kRecordPlaceholder, ""});
    }
    return read;
}

void putBlocks(BlocksRead& read, core::Document& document, std::vector<std::string>& warnings) {
    std::size_t cycles = 0;
    for (std::size_t d = 0; d < read.definitions.size(); ++d) {
        auto& definition = read.definitions[d];
        if (document.blocks().wouldCycle(definition)) {
            std::vector<core::EntityRecord> kept;
            for (auto& record : definition.entities) {
                const bool insert = std::holds_alternative<core::Insert>(record.entity);
                core::BlockDefinition probe;
                probe.name = definition.name;
                if (insert) {
                    probe.entities.push_back(record);
                }
                if (insert && document.blocks().wouldCycle(probe)) {
                    // docs/adr/0045, point 1: the insert stays as it was read.
                    const auto& item = read.inserts[d].at(record.id.value);
                    definition.rawEntities.push_back(
                        {record.id, record.order, item.front().type, rawGroupsOf(item)});
                    ++cycles;
                } else {
                    kept.push_back(std::move(record));
                }
            }
            definition.entities = std::move(kept);
            std::ranges::sort(definition.rawEntities, {}, &core::RawEntity::order);
        }
        std::string name = definition.name;
        document.replaceBlock(std::move(name), std::move(definition));
    }
    if (cycles != 0) {
        warnings.push_back(plural(
            cycles, "insert", " that would make a block contain itself kept as unhandled data."));
    }
}

std::optional<std::string> depthWarning(const core::Document& document) {
    // The levels of inserts below a block, at most kMaxInsertDepth + 1.
    constexpr int kCap = core::kMaxInsertDepth + 1;
    std::unordered_map<std::string, int> heights;
    std::function<int(const core::BlockDefinition&, int)> height =
        [&](const core::BlockDefinition& block, int level) -> int {
        const std::string key = blockKey(block.name);
        if (const auto found = heights.find(key); found != heights.end()) {
            return found->second;
        }
        if (level > kCap) {
            return kCap;
        }
        int result = 0;
        for (const auto& record : block.entities) {
            const auto* insert = std::get_if<core::Insert>(&record.entity);
            const auto* child = insert != nullptr ? document.blocks().find(insert->block) : nullptr;
            if (child != nullptr) {
                result = std::max(result, std::min(kCap, 1 + height(*child, level + 1)));
            }
        }
        heights.emplace(key, result);
        return result;
    };
    std::size_t deep = 0;
    for (const auto& record : document.entities()) {
        const auto* insert = std::get_if<core::Insert>(&record.entity);
        const auto* block = insert != nullptr ? document.blocks().find(insert->block) : nullptr;
        if (block != nullptr && 1 + height(*block, 1) > core::kMaxInsertDepth) {
            ++deep;
        }
    }
    if (deep == 0) {
        return std::nullopt;
    }
    return plural(deep, "insert",
                  " nest blocks deeper than " + std::to_string(core::kMaxInsertDepth) +
                      " levels: the deeper ones are not drawn.");
}

// --- Writing -------------------------------------------------------------------

namespace {

/// The name of the layer of `properties`, "0" without one.
std::string layerName(const core::Document& document, const core::EntityProperties& properties) {
    const core::Layer* layer = document.layer(properties.layer);
    return layer != nullptr ? layer->name : std::string{"0"};
}

/// Gives handles to the attributes without one and to the SEQEND of an
/// INSERT; true if something changed.
class HandlePlanner {
public:
    HandlePlanner(const core::Document& document, std::uint64_t& next)
        : m_document(document), m_next(next) {}

    std::uint64_t take() { return m_next++; }

    bool plan(core::Insert& insert, core::RawGroups& specific, std::uint64_t owner,
              const core::EntityProperties& properties) {
        bool changed = false;
        for (auto& attribute : insert.attributes) {
            if (attribute.id.value == 0) {
                attribute.id = core::EntityId{take()};
                setOwner(attribute.extras.beforeEntity, owner);
                changed = true;
            }
        }
        const std::size_t start = seqendStart(specific);
        if (insert.attributes.empty() && start == specific.size()) {
            return changed;
        }
        if (start == specific.size()) {
            specific.push_back({0, std::string{kSeqendMarker}});
            specific.push_back({5, handleText(take())});
            specific.push_back({330, handleText(owner)});
            specific.push_back({100, "AcDbEntity"});
            specific.push_back({8, layerName(m_document, properties)});
            return true;
        }
        core::RawGroups seqend(specific.begin() + static_cast<std::ptrdiff_t>(start) + 1,
                               specific.end());
        const std::uint64_t handle = handleOf(seqend);
        if (handle != 0 && m_seqends.insert(handle).second) {
            return changed;
        }
        // A copy shares the SEQEND of its original: it takes a new one.
        const auto at = std::ranges::find(seqend, 5, &core::RawGroup::code);
        if (at != seqend.end()) {
            at->value = handleText(take());
        } else {
            seqend.insert(seqend.begin(), {5, handleText(take())});
        }
        setOwner(seqend, owner);
        specific.resize(start + 1);
        specific.insert(specific.end(), seqend.begin(), seqend.end());
        return true;
    }

private:
    const core::Document& m_document;
    std::uint64_t& m_next;
    std::unordered_set<std::uint64_t> m_seqends;
};

bool hasTable(const core::RawGroups& tables, std::string_view name) {
    for (std::size_t i = 0; i + 1 < tables.size(); ++i) {
        if (isMarker(tables[i], "TABLE") && tables[i + 1].code == 2 &&
            trimDxf(tables[i + 1].value) == name) {
            return true;
        }
    }
    return false;
}

}  // namespace

BlockWritePlan planBlocks(const core::Document& document, const core::RawDrawing& raw,
                          std::uint64_t& nextHandle) {
    BlockWritePlan plan;
    const auto definitions = document.blocks().definitions();
    plan.blocks.assign(definitions.begin(), definitions.end());
    HandlePlanner planner{document, nextHandle};
    for (auto& block : plan.blocks) {
        for (std::uint64_t* handle : {&block.recordId, &block.blockId, &block.endBlockId}) {
            if (*handle == 0) {
                *handle = planner.take();
            }
        }
        for (auto& record : block.entities) {
            if (record.id.value == 0) {
                record.id = core::EntityId{planner.take()};
            }
            if (auto* insert = std::get_if<core::Insert>(&record.entity)) {
                planner.plan(*insert, record.extras.specific, record.id.value, record.properties);
            }
        }
    }
    for (const auto& record : document.entities()) {
        const auto* insert = std::get_if<core::Insert>(&record.entity);
        if (insert == nullptr) {
            continue;
        }
        core::Insert planned = *insert;
        core::RawGroups specific = record.extras.specific;
        if (planner.plan(planned, specific, record.id.value, record.properties)) {
            plan.inserts.emplace(record.id.value, std::move(planned));
            plan.insertSpecific.emplace(record.id.value, std::move(specific));
        }
    }
    bool tables = false;
    for (const auto& section : raw.sections) {
        tables = tables || (section.name == "TABLES" && hasTable(section.groups, "BLOCK_RECORD"));
    }
    if (!tables && !plan.blocks.empty()) {
        plan.recordTable = planner.take();
    }
    return plan;
}

core::EntityRecord recordForWriting(const core::EntityRecord& record,
                                    const core::Document& document, const BlockWritePlan& plan) {
    core::EntityRecord out = record;
    out.extras.common = entityCommonGroups(record, document);
    if (auto* insert = std::get_if<core::Insert>(&out.entity)) {
        if (const auto found = plan.inserts.find(record.id.value); found != plan.inserts.end()) {
            *insert = found->second;
            out.extras.specific = plan.insertSpecific.at(record.id.value);
        }
        for (auto& attribute : insert->attributes) {
            attribute.extras.common = attributeCommonGroups(attribute, document);
        }
    }
    return out;
}

namespace {

/// The groups of a BLOCK after its handle.
core::RawGroups blockGroups(const core::BlockDefinition& block) {
    const int modelFlags =
        (block.anonymous ? kAnonymousBit : 0) | (block.hasAttributes ? kAttributesBit : 0);
    if (block.blockExtras.empty()) {
        core::RawGroups groups{{330, handleText(block.recordId)},
                               {100, "AcDbEntity"},
                               {8, "0"},
                               {100, "AcDbBlockBegin"},
                               {2, block.name},
                               {70, std::to_string(modelFlags)},
                               {10, formatNumber(block.base.x)},
                               {20, formatNumber(block.base.y)},
                               {30, "0.0"},
                               {3, block.name},
                               {1, ""}};
        if (!block.description.empty()) {
            groups.push_back({4, block.description});
        }
        return groups;
    }
    core::RawGroups groups = block.blockExtras;
    setOwner(groups, block.recordId);
    const long long flags =
        (integerOf(groups, 70, 0).value_or(0) & ~(kAnonymousBit | kAttributesBit)) | modelFlags;
    using Kind = ModelGroup::Kind;
    const std::vector<ModelGroup> values{
        {2, Kind::Text, block.name},
        {70, Kind::Integer, std::to_string(flags), static_cast<double>(flags)},
        {10, Kind::Number, formatNumber(block.base.x), block.base.x},
        {20, Kind::Number, formatNumber(block.base.y), block.base.y},
        {3, Kind::Text, block.name},
        {4, Kind::Text, block.description, 0.0, block.description.empty()},
    };
    fillGroups(groups, values, kBlockOrder);
    return groups;
}

core::RawGroups endBlockGroups(const core::BlockDefinition& block) {
    if (block.endBlockExtras.empty()) {
        return {{330, handleText(block.recordId)},
                {100, "AcDbEntity"},
                {8, "0"},
                {100, "AcDbBlockEnd"}};
    }
    core::RawGroups groups = block.endBlockExtras;
    setOwner(groups, block.recordId);
    return groups;
}

class BlockWriter {
public:
    BlockWriter(const core::Document& document, const BlockWritePlan& plan,
                std::string_view version, std::string_view codePage)
        : m_document(document), m_version(version), m_codePage(codePage) {
        for (const auto& block : plan.blocks) {
            m_names.insert(blockKey(block.name));
        }
    }

    /// Writes `block`; `source` holds the groups it was read from, if any.
    void write(const core::BlockDefinition& block, const core::RawGroups* source,
               const Chunk* chunk, core::RawGroups& out) const {
        out.push_back({0, "BLOCK"});
        out.push_back({5, handleText(block.blockId)});
        const auto head = blockGroups(block);
        out.insert(out.end(), head.begin(), head.end());
        std::unordered_map<std::uint64_t, EntityItem> originals;
        if (source != nullptr && chunk != nullptr) {
            for (auto& item : groupItems(entitiesIn(*source, chunk->entities, chunk->endBlock))) {
                const std::uint64_t id = handleOf(item.front().groups);
                if (id != 0) {
                    originals.emplace(id, std::move(item));
                }
            }
        }
        std::size_t m = 0;
        std::size_t r = 0;
        const auto& managed = block.entities;
        const auto& raw = block.rawEntities;
        while (m < managed.size() || r < raw.size()) {
            if (r < raw.size() && (m == managed.size() || raw[r].order < managed[m].order)) {
                out.push_back({0, raw[r].type});
                out.insert(out.end(), raw[r].groups.begin(), raw[r].groups.end());
                ++r;
                continue;
            }
            const auto found = originals.find(managed[m].id.value);
            writeEntity(block, managed[m], found != originals.end() ? &found->second : nullptr,
                        out);
            ++m;
        }
        out.push_back({0, "ENDBLK"});
        out.push_back({5, handleText(block.endBlockId)});
        const auto end = endBlockGroups(block);
        out.insert(out.end(), end.begin(), end.end());
    }

private:
    void writeEntity(const core::BlockDefinition& block, const core::EntityRecord& record,
                     const EntityItem* original, core::RawGroups& out) const {
        const auto* codec = findCodec(record.entity);
        if (codec == nullptr) {
            return;  // never: every handled type has a codec
        }
        core::EntityRecord written = record;
        written.extras.common = entityCommonGroups(record, m_document);
        setOwner(written.extras.beforeEntity, block.recordId);
        if (auto* insert = std::get_if<core::Insert>(&written.entity)) {
            for (auto& attribute : insert->attributes) {
                attribute.extras.common = attributeCommonGroups(attribute, m_document);
            }
        }
        EntityWriteContext context{m_version, m_codePage, core::EntityId{block.recordId}};
        core::RawGroups groups{{0, std::string{codec->type}}};
        codec->write(written, context, groups);
        if (original != nullptr && sameAsRead(*original, block, record, *codec, groups)) {
            const auto read = rawGroupsOf(*original);
            out.push_back({0, original->front().type});
            out.insert(out.end(), read.begin(), read.end());
            return;
        }
        out.insert(out.end(), groups.begin(), groups.end());
    }

    /// True when the groups `groups` written for `record` say what the
    /// groups it was read from say: then those go back as they were.
    bool sameAsRead(const EntityItem& original, const core::BlockDefinition& block,
                    const core::EntityRecord& record, const EntityCodec& codec,
                    const core::RawGroups& groups) const {
        EntityReadContext context{m_version, m_codePage,   core::EntityId{block.recordId},
                                  record.id, record.order, m_document.units()};
        const auto decoded = readItem(original, context, m_names);
        if (!decoded || decoded->entity.index() != record.entity.index()) {
            return false;
        }
        EntityWriteContext write{m_version, m_codePage, core::EntityId{block.recordId}};
        core::RawGroups again{{0, std::string{codec.type}}};
        codec.write(*decoded, write, again);
        return sameGroups(groups, again);
    }

    const core::Document& m_document;
    std::string_view m_version;
    std::string_view m_codePage;
    std::unordered_set<std::string> m_names;
};

}  // namespace

core::RawGroups writeBlocks(const core::RawGroups& section, const core::Document& document,
                            const BlockWritePlan& plan, std::string_view version,
                            std::string_view codePage) {
    const BlockWriter writer{document, plan, version, codePage};
    // The groups each definition was read from, by the handle of its BLOCK.
    std::unordered_map<std::uint64_t, Chunk> sources;
    for (std::size_t i = 0; i + 1 < section.size(); ++i) {
        if (isPlaceholder(section[i]) && isMarker(section[i + 1], "BLOCK")) {
            const Chunk chunk = chunkAt(section, i + 1);
            if (chunk.begin + 1 < chunk.entities && section[chunk.begin + 1].code == 5) {
                sources.emplace(parseHandle(section[chunk.begin + 1].value).value_or(0), chunk);
            }
        }
    }
    core::RawGroups out;
    std::size_t next = 0;
    const auto writeNext = [&] {
        const auto& block = plan.blocks[next++];
        const auto found = sources.find(block.blockId);
        writer.write(block, found != sources.end() ? &section : nullptr,
                     found != sources.end() ? &found->second : nullptr, out);
    };
    std::size_t i = 0;
    while (i < section.size()) {
        if (!isPlaceholder(section[i])) {
            out.push_back(section[i++]);
            continue;
        }
        // The groups of a definition read: written from the model.
        i = i + 1 < section.size() && isMarker(section[i + 1], "BLOCK")
                ? chunkAt(section, i + 1).end
                : i + 1;
        if (next < plan.blocks.size()) {
            writeNext();
        }
    }
    while (next < plan.blocks.size()) {
        writeNext();
    }
    return out;
}

namespace {

core::RawGroups recordGroups(const core::BlockDefinition& block, std::string_view owner) {
    core::RawGroups out{{0, "BLOCK_RECORD"}, {5, handleText(block.recordId)}};
    if (block.recordExtras.empty()) {
        out.insert(out.end(), {{330, std::string{owner}},
                               {100, "AcDbSymbolTableRecord"},
                               {100, "AcDbBlockTableRecord"},
                               {2, block.name},
                               {340, "0"},
                               {70, std::to_string(block.units)},
                               {280, block.explodable ? "1" : "0"},
                               {281, block.uniformScale ? "1" : "0"}});
        return out;
    }
    core::RawGroups groups = block.recordExtras;
    using Kind = ModelGroup::Kind;
    const std::vector<ModelGroup> values{
        {2, Kind::Text, block.name},
        {70, Kind::Integer, std::to_string(block.units), static_cast<double>(block.units),
         block.units == 0},
        {280, Kind::Integer, block.explodable ? "1" : "0", block.explodable ? 1.0 : 0.0,
         block.explodable},
        {281, Kind::Integer, block.uniformScale ? "1" : "0", block.uniformScale ? 1.0 : 0.0,
         !block.uniformScale},
    };
    fillGroups(groups, values, kRecordOrder);
    out.insert(out.end(), groups.begin(), groups.end());
    return out;
}

}  // namespace

void writeBlockRecords(core::RawGroups& tables, const BlockWritePlan& plan) {
    std::size_t begin = tables.size();
    for (std::size_t i = 0; i + 1 < tables.size(); ++i) {
        if (isMarker(tables[i], "TABLE") && tables[i + 1].code == 2 &&
            trimDxf(tables[i + 1].value) == "BLOCK_RECORD") {
            begin = i;
            break;
        }
    }
    if (begin == tables.size()) {
        if (plan.blocks.empty()) {
            return;
        }
        const std::string owner = handleText(plan.recordTable);
        core::RawGroups table{
            {0, "TABLE"}, {2, "BLOCK_RECORD"},      {5, owner},
            {330, "0"},   {100, "AcDbSymbolTable"}, {70, std::to_string(plan.blocks.size())}};
        for (const auto& block : plan.blocks) {
            const auto record = recordGroups(block, owner);
            table.insert(table.end(), record.begin(), record.end());
        }
        table.push_back({0, "ENDTAB"});
        tables.insert(tables.end(), table.begin(), table.end());
        return;
    }
    std::size_t end = begin + 1;
    while (end < tables.size() && !isMarker(tables[end], "ENDTAB") &&
           !isMarker(tables[end], "TABLE")) {
        ++end;
    }
    // The header of the table: its handle names the owner of the records,
    // its 70 counts them.
    std::size_t header = begin + 1;
    while (header < end && tables[header].code != 0 && !isPlaceholder(tables[header])) {
        ++header;
    }
    std::string owner{"0"};
    std::optional<std::size_t> count;
    for (std::size_t i = begin + 1; i < header; ++i) {
        if (tables[i].code == 5 && owner == "0") {
            owner = trimDxf(tables[i].value);
        } else if (tables[i].code == 70 && !count) {
            count = i;
        }
    }
    const auto placeholders = static_cast<std::size_t>(
        std::count_if(tables.begin() + static_cast<std::ptrdiff_t>(header),
                      tables.begin() + static_cast<std::ptrdiff_t>(end), isPlaceholder));
    // Each record where the reader took it out, if its definition is still
    // there; the others at the end, in table order.
    std::unordered_map<std::uint64_t, std::size_t> byRecord;
    for (std::size_t i = 0; i < plan.blocks.size(); ++i) {
        byRecord.emplace(plan.blocks[i].recordId, i);
    }
    std::vector<bool> written(plan.blocks.size(), false);
    core::RawGroups body;
    for (std::size_t i = begin; i < end; ++i) {
        if (!isPlaceholder(tables[i])) {
            body.push_back(tables[i]);
            continue;
        }
        const auto found = byRecord.find(parseHandle(tables[i].value).value_or(0));
        if (found != byRecord.end() && !written[found->second]) {
            written[found->second] = true;
            const auto record = recordGroups(plan.blocks[found->second], owner);
            body.insert(body.end(), record.begin(), record.end());
        }
    }
    for (std::size_t i = 0; i < plan.blocks.size(); ++i) {
        if (!written[i]) {
            const auto record = recordGroups(plan.blocks[i], owner);
            body.insert(body.end(), record.begin(), record.end());
        }
    }
    if (count) {
        // The count follows the records added and removed.
        auto& group = body[*count - begin];
        const long long read = parseInteger(group.value).value_or(0);
        const long long now =
            std::max<long long>(0, read + static_cast<long long>(plan.blocks.size()) -
                                       static_cast<long long>(placeholders));
        if (now != read) {
            group.value = std::to_string(now);
        }
    }
    tables.erase(tables.begin() + static_cast<std::ptrdiff_t>(begin),
                 tables.begin() + static_cast<std::ptrdiff_t>(end));
    tables.insert(tables.begin() + static_cast<std::ptrdiff_t>(begin), body.begin(), body.end());
}

std::uint64_t blockHandlesEnd(const core::Document& document) {
    std::uint64_t next = 1;
    const auto see = [&next](std::uint64_t handle) {
        next = std::max(next, handle + 1);
    };
    const auto insertHandles = [&](const core::EntityRecord& record) {
        const auto* insert = std::get_if<core::Insert>(&record.entity);
        if (insert == nullptr) {
            return;
        }
        for (const auto& attribute : insert->attributes) {
            see(attribute.id.value);
        }
        const auto& specific = record.extras.specific;
        const std::size_t start = seqendStart(specific);
        if (start != specific.size()) {
            see(handleOf(core::RawGroups(specific.begin() + static_cast<std::ptrdiff_t>(start) + 1,
                                         specific.end())));
        }
    };
    for (const auto& block : document.blocks().definitions()) {
        see(block.recordId);
        see(block.blockId);
        see(block.endBlockId);
        for (const auto& record : block.entities) {
            see(record.id.value);
            insertHandles(record);
        }
        for (const auto& raw : block.rawEntities) {
            see(raw.id.value);
        }
    }
    for (const auto& record : document.entities()) {
        insertHandles(record);
    }
    return next;
}

}  // namespace tg::io
