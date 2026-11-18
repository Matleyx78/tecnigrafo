// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// INSERT with the ATTRIB that follow it and their SEQEND (docs/adr/0045,
// point 2; docs/tappe/0.8, PT18, point 2). The groups of AcDbBlockReference
// are kept as read in extras.specific, with those of the model as witnesses
// (fillGroups()), and after them, behind the marker kSeqendMarker, the groups
// of the SEQEND.

#include "dxf_blocks.hpp"
#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/core/math.hpp"
#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

namespace tg::io {

using dxf_groups::ownGroups;
using dxf_groups::parseInteger;

namespace {

constexpr std::string_view kReferenceMarker = "AcDbBlockReference";

// The order of the groups of INSERT in the DXF reference: a group the model
// adds goes after the last one before it.
constexpr std::array kInsertOrder{100, 66, 2, 10, 20, 41, 42, 43, 50, 70, 71, 44, 45};

std::optional<core::EntityRecord> readInsert(const core::RawGroups& groups,
                                             EntityReadContext& context) {
    const Extrusion extrusion = entityExtrusion(groups);
    if (extrusion == Extrusion::Other) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    core::Insert insert;
    std::vector<int> seen;
    const auto own = ownGroups(parts.geometry);
    for (std::size_t i = 0; i < parts.geometry.size(); ++i) {
        const auto& group = parts.geometry[i];
        if (!own[i]) {
            continue;
        }
        const int code = group.code;
        const bool model = code == 2 || code == 10 || code == 20 || code == 41 || code == 42 ||
                           code == 50 || code == 66 || code == 70 || code == 71;
        if (!model) {
            continue;
        }
        if (std::ranges::find(seen, code) != seen.end()) {
            return std::nullopt;
        }
        seen.push_back(code);
        if (code == 2) {
            insert.block = group.value;
            continue;
        }
        const auto number = parseNumber(trimDxf(group.value));
        if (!number) {
            return std::nullopt;
        }
        switch (code) {
            case 10:
                insert.insertion.x = *number;
                break;
            case 20:
                insert.insertion.y = *number;
                break;
            case 41:
                insert.scaleX = *number;
                break;
            case 42:
                insert.scaleY = *number;
                break;
            case 50:
                insert.rotation = core::degreesToRadians(*number);
                break;
            case 70:
            case 71:
                // Columns and rows: a MINSERT stays unhandled (docs/adr/0044,
                // point 4).
                if (*number > 1.0) {
                    return std::nullopt;
                }
                break;
            default:
                break;
        }
    }
    for (const int required : {2, 10, 20}) {
        if (std::ranges::find(seen, required) == seen.end()) {
            return std::nullopt;
        }
    }
    if (trimDxf(insert.block).empty() || insert.scaleX == 0.0 || insert.scaleY == 0.0) {
        return std::nullopt;  // a scale of zero does not exist (docs/adr/0045, point 2)
    }
    if (extrusion == Extrusion::Down) {
        // x -> -x: F·T(p)·R(r)·S(sx, sy) = T(Fp)·R(-r)·S(-sx, sy), F = diag(-1, 1).
        insert.insertion = flipX(insert.insertion);
        insert.rotation = core::degreesToRadians(
            normalizeDegrees(negate(core::radiansToDegrees(insert.rotation))));
        insert.scaleX = negate(insert.scaleX);
        parts.extras.flippedExtrusion = true;
    }
    parts.extras.specific = std::move(parts.geometry);
    if (!core::supported(insert)) {
        return std::nullopt;
    }
    return core::EntityRecord{context.id, std::move(insert), std::move(parts.extras),
                              context.order};
}

void writeInsert(const core::EntityRecord& record, EntityWriteContext& context,
                 core::RawGroups& out) {
    core::Insert insert = std::get<core::Insert>(record.entity);
    const auto& extras = record.extras;
    if (extras.flippedExtrusion) {
        // The conversion of the reading is its own inverse.
        insert.insertion = flipX(insert.insertion);
        insert.rotation = core::degreesToRadians(
            normalizeDegrees(negate(core::radiansToDegrees(insert.rotation))));
        insert.scaleX = negate(insert.scaleX);
    }
    writeEntityCommon(record, context, out);
    const std::size_t from = out.size();
    const auto& specific = extras.specific;
    const std::size_t seqend = seqendStart(specific);
    core::RawGroups groups(specific.begin(),
                           specific.begin() + static_cast<std::ptrdiff_t>(seqend));
    const auto own = ownGroups(groups);
    bool marker = false;
    for (std::size_t i = 0; i < groups.size() && !marker; ++i) {
        marker = own[i] && groups[i].code == 100;
    }
    if (!marker) {
        // Appended and rotated to the front: an insert at begin() makes GCC 13
        // warn of a false null dereference at -O3 (-Wnull-dereference).
        groups.push_back({100, std::string{kReferenceMarker}});
        std::rotate(groups.begin(), groups.end() - 1, groups.end());
    }
    const bool follows = !insert.attributes.empty() || seqend != specific.size();
    using Kind = ModelGroup::Kind;
    const std::vector<ModelGroup> values{
        {66, Kind::Integer, follows ? "1" : "0", follows ? 1.0 : 0.0, !follows},
        {2, Kind::Text, insert.block},
        {10, Kind::Number, formatNumber(insert.insertion.x), insert.insertion.x},
        {20, Kind::Number, formatNumber(insert.insertion.y), insert.insertion.y},
        {41, Kind::Number, formatNumber(insert.scaleX), insert.scaleX, insert.scaleX == 1.0},
        {42, Kind::Number, formatNumber(insert.scaleY), insert.scaleY, insert.scaleY == 1.0},
        {50, Kind::Angle, formatNumber(core::radiansToDegrees(insert.rotation)), insert.rotation,
         insert.rotation == 0.0},
    };
    fillGroups(groups, values, kInsertOrder);
    // Z after the point, the extrusion at the end, as the DXF reference has them.
    const auto filled = ownGroups(groups);
    bool z = false;
    for (std::size_t i = 0; i < groups.size(); ++i) {
        out.push_back(groups[i]);
        if (filled[i] && groups[i].code == 20 && !z) {
            z = true;
            constexpr std::array kZ{30};
            writePlacement(extras, kZ, out);
        }
    }
    constexpr std::array kExtrusion{39, 210, 220, 230, 38};
    writePlacement(extras, kExtrusion, out);
    out.insert(out.end(), extras.extended.begin(), extras.extended.end());
    restorePlacement(extras, from, out);
    for (const auto& attribute : insert.attributes) {
        out.push_back({0, "ATTRIB"});
        writeAttribute(attribute, record.id, context, out);
    }
    if (seqend != specific.size()) {
        out.push_back({0, "SEQEND"});
        out.insert(out.end(), specific.begin() + static_cast<std::ptrdiff_t>(seqend) + 1,
                   specific.end());
    }
}

}  // namespace

const EntityCodec kInsertCodec{"INSERT", readInsert, writeInsert};

std::vector<EntityItem> groupItems(std::vector<FileEntity> entities) {
    std::vector<EntityItem> items;
    std::size_t i = 0;
    while (i < entities.size()) {
        EntityItem item;
        const bool insert = trimDxf(entities[i].type) == "INSERT";
        item.push_back(std::move(entities[i++]));
        if (insert) {
            while (i < entities.size() && trimDxf(entities[i].type) == "ATTRIB") {
                item.push_back(std::move(entities[i++]));
            }
            if (i < entities.size() && trimDxf(entities[i].type) == "SEQEND") {
                item.push_back(std::move(entities[i++]));
            }
        }
        items.push_back(std::move(item));
    }
    return items;
}

core::RawGroups rawGroupsOf(const EntityItem& item) {
    core::RawGroups groups;
    for (std::size_t i = 0; i < item.size(); ++i) {
        if (i > 0) {
            groups.push_back({0, item[i].type});
        }
        groups.insert(groups.end(), item[i].groups.begin(), item[i].groups.end());
    }
    return groups;
}

std::string blockKey(std::string_view name) {
    std::string key{name};
    for (char& ch : key) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return key;
}

namespace {

/// The handle of an entity: its first group 5, or 0.
std::uint64_t handleOf(const core::RawGroups& groups) {
    for (const auto& group : groups) {
        if (group.code == 5) {
            return parseHandle(group.value).value_or(0);
        }
        if (group.code == 100) {
            break;  // the handle comes before the subclasses
        }
    }
    return 0;
}

/// What the own group 66 of `specific` says: false when it is missing or 0,
/// true when it is 1, std::nullopt for any other value or spelling.
std::optional<bool> attributesFollow(const core::RawGroups& specific) {
    const auto own = ownGroups(specific);
    for (std::size_t i = 0; i < specific.size(); ++i) {
        if (own[i] && specific[i].code == 66) {
            const auto value = parseInteger(specific[i].value);
            if (value != 0 && value != 1) {
                return std::nullopt;
            }
            return value == 1;
        }
    }
    return false;
}

}  // namespace

std::vector<std::uint64_t> itemHandles(const EntityItem& item) {
    // The handle as the reader checked it in ENTITIES: the group 5 wherever
    // it is, also after a subclass marker, where handleOf() does not look.
    // One handle for each entity of the item, so the first is the item's.
    std::vector<std::uint64_t> handles;
    handles.reserve(item.size());
    for (const auto& entity : item) {
        const auto group = std::ranges::find(entity.groups, 5, &core::RawGroup::code);
        handles.push_back(group == entity.groups.end() ? 0 : parseHandle(group->value).value_or(0));
    }
    return handles;
}

std::optional<core::EntityRecord> readItem(const EntityItem& item, EntityReadContext& context,
                                           const std::unordered_set<std::string>& blocks) {
    if (item.empty()) {
        return std::nullopt;
    }
    const auto type = trimDxf(item.front().type);
    if (type != "INSERT") {
        return decodeEntity(findCodec(type), item.front().groups, context);
    }
    // ATTRIB not ended by a SEQEND: the sequence is not one the writer can
    // give back, so the INSERT stays unhandled with them.
    const bool seqend = trimDxf(item.back().type) == "SEQEND";
    if (item.size() > 1 && !seqend) {
        return std::nullopt;
    }
    auto record = decodeEntity(&kInsertCodec, item.front().groups, context);
    if (!record) {
        return std::nullopt;
    }
    // writeInsert() gives group 66 the value 1 when a SEQEND follows and 0
    // otherwise: an INSERT whose group 66 says something else, as a 1 with
    // no sequence after it, stays unhandled, so that the value is not lost
    // (docs/adr/0018).
    if (attributesFollow(record->extras.specific) != std::optional<bool>{seqend}) {
        return std::nullopt;
    }
    auto& insert = std::get<core::Insert>(record->entity);
    if (!blocks.contains(blockKey(insert.block))) {
        return std::nullopt;  // a block that is not there, or external
    }
    const std::size_t attributes = seqend ? item.size() - 1 : item.size();
    for (std::size_t i = 1; i < attributes; ++i) {
        EntityReadContext attributeContext = context;
        attributeContext.id = core::EntityId{handleOf(item[i].groups)};
        if (attributeContext.id.value == 0) {
            return std::nullopt;
        }
        auto attribute = readAttribute(item[i].groups, attributeContext);
        if (!attribute) {
            return std::nullopt;  // docs/adr/0044, point 4: the INSERT as a whole
        }
        insert.attributes.push_back(std::move(*attribute));
    }
    if (seqend) {
        auto& specific = record->extras.specific;
        specific.push_back({0, std::string{kSeqendMarker}});
        specific.insert(specific.end(), item.back().groups.begin(), item.back().groups.end());
    }
    return record;
}

}  // namespace tg::io
