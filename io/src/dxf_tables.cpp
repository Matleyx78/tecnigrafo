// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_tables.hpp"

#include "dxf_entity_common.hpp"
#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <utility>

namespace tg::io {

using dxf_groups::firstOwn;
using dxf_groups::ownGroups;
using dxf_groups::parseInteger;
using dxf_groups::rgbOf;
using dxf_groups::rgbValue;

namespace {

constexpr std::string_view kByBlockName = "ByBlock";
constexpr std::string_view kByLayerName = "ByLayer";
constexpr std::string_view kContinuousName = "Continuous";
constexpr std::string_view kContinuousDescription = "Solid line";
constexpr std::string_view kLayerZeroName = "0";

// The colour of a layer without a 62, and of a layer created by the reader
// (docs/adr/0023, point 5).
constexpr int kDefaultLayerColor = 7;
// Bits of the flags (70) of a LAYER record that the model represents.
constexpr long long kFrozenBit = 1;
constexpr long long kLockedBit = 4;
constexpr long long kMaxColorIndex = 255;
constexpr long long kMaxRgb = 0xFFFFFF;

// The groups of a LAYER record in the order AutoCAD writes them; the groups
// of the model go in by it.
constexpr std::array kLayerOrder{2, 70, 62, 420, 6, 290, 370};
// The same for LTYPE: name, flags, description, alignment, count, length.
constexpr std::array kLinetypeOrder{2, 70, 3, 72, 73, 40};
// The groups of the elements of an LTYPE record (docs/adr/0025, point 1).
constexpr std::array kElementCodes{73, 40, 49, 74, 75, 340, 46, 50, 44, 45, 9};

// The tables a LTYPE or LAYER table goes before when the file lacks it, in
// the order of AutoCAD.
constexpr std::array<std::string_view, 7> kAfterLinetypes{"LAYER", "STYLE",    "VIEW",        "UCS",
                                                          "APPID", "DIMSTYLE", "BLOCK_RECORD"};
constexpr std::array<std::string_view, 6> kAfterLayers{"STYLE", "VIEW",     "UCS",
                                                       "APPID", "DIMSTYLE", "BLOCK_RECORD"};

bool isMarker(const core::RawGroup& group, std::string_view name) {
    return group.code == 0 && trimDxf(group.value) == name;
}

std::string plural(int count, std::string_view singular, std::string_view rest) {
    return std::to_string(count) + " " + std::string{singular} + (count == 1 ? "" : "s") +
           std::string{rest};
}

/// Removes the group at `at`, if any.
void eraseAt(core::RawGroups& groups, std::optional<std::size_t> at) {
    if (at) {
        groups.erase(groups.begin() + static_cast<std::ptrdiff_t>(*at));
    }
}

/// The groups a new record starts with, after its handle.
core::RawGroups recordPreamble(std::string_view owner, std::string_view subclass) {
    return {
        {330, std::string{owner}}, {100, "AcDbSymbolTableRecord"}, {100, std::string{subclass}}};
}

/// A value the model writes for a group: its text, and the number a group
/// read from the file must have to be written again as it was.
struct Wanted {
    std::int16_t code{0};
    std::string text;
    double number{0.0};
};

/// Writes the model's values in `groups`: the first own group of each
/// wanted code keeps its text if it says the same number, otherwise gets
/// the model's text; the own groups of `codes` the model does not want are
/// removed; the wanted groups the record lacks are put in by `order`.
void applyWanted(core::RawGroups& groups, std::span<const int> codes,
                 const std::vector<Wanted>& wanted, std::span<const int> order) {
    for (const int code : codes) {
        const auto at = firstOwn(groups, ownGroups(groups), code);
        if (!at) {
            continue;
        }
        const auto found = std::ranges::find(wanted, code, &Wanted::code);
        if (found == wanted.end()) {
            eraseAt(groups, at);
        } else if (parseNumber(trimDxf(groups[*at].value)) != found->number) {
            groups[*at].value = found->text;
        }
    }
    core::RawGroups additions;
    for (const auto& value : wanted) {
        if (!firstOwn(groups, ownGroups(groups), value.code)) {
            additions.push_back({value.code, value.text});
        }
    }
    dxf_groups::insertInOrder(groups, additions, order);
}

Wanted wantedInteger(std::int16_t code, long long value) {
    return {code, std::to_string(value), static_cast<double>(value)};
}

/// The groups of a LAYER record, from group 0 on.
core::RawGroups layerRecord(const core::Layer& layer, const core::Document& document,
                            std::string_view owner) {
    core::RawGroups body =
        layer.extras.empty() ? recordPreamble(owner, "AcDbLayerTableRecord") : layer.extras;
    const auto own = ownGroups(body);
    const auto at70 = firstOwn(body, own, 70);
    const auto at62 = firstOwn(body, own, 62);
    const auto at420 = firstOwn(body, own, 420);
    const bool new370 = layer.extras.empty() || firstOwn(body, own, 370).has_value() ||
                        layer.lineweight != core::lineweight::kDefault;
    const bool new290 = firstOwn(body, own, 290).has_value() || !layer.plot;

    // The bits of the flags the model does not represent stay as read.
    long long flags = at70 ? parseInteger(body[*at70].value).value_or(0) : 0;
    flags &= ~(kFrozenBit | kLockedBit);
    flags |= (layer.frozen ? kFrozenBit : 0) | (layer.locked ? kLockedBit : 0);

    long long index = 0;
    std::vector<Wanted> wanted;
    wanted.push_back(wantedInteger(70, flags));
    if (layer.color.kind() == core::Color::Kind::Rgb) {
        const int rgb = rgbValue(layer.color.rgb());
        // The 62 read with an unchanged RGB is written again as it was
        // (docs/adr/0024, point 2); otherwise the nearest index.
        const auto read420 = at420 ? parseInteger(body[*at420].value) : std::nullopt;
        const auto read62 = at62 ? parseInteger(body[*at62].value) : std::nullopt;
        index = read420 == rgb && read62 && *read62 != 0 ? std::llabs(*read62)
                                                         : core::nearestAci(layer.color.rgb());
        wanted.push_back(wantedInteger(62, layer.on ? index : -index));
        wanted.push_back(wantedInteger(420, rgb));
    } else {
        index = layer.color.index();
        wanted.push_back(wantedInteger(62, layer.on ? index : -index));
    }
    if (new290) {
        wanted.push_back(wantedInteger(290, layer.plot ? 1 : 0));
    }
    if (new370) {
        wanted.push_back(wantedInteger(370, layer.lineweight));
    }
    constexpr std::array kModelCodes{70, 62, 420, 290, 370};
    applyWanted(body, kModelCodes, wanted, kLayerOrder);

    const core::Linetype* linetype = document.linetype(layer.linetype);
    const std::string linetypeName =
        linetype != nullptr ? linetype->name : std::string{kContinuousName};
    core::RawGroups names{{2, layer.name}};
    const auto at6 = firstOwn(body, ownGroups(body), 6);
    if (!at6) {
        names.push_back({6, linetypeName});
    } else if (!core::sameTableName(body[*at6].value, linetypeName)) {
        body[*at6].value = linetypeName;
    }
    dxf_groups::insertInOrder(body, names, kLayerOrder);

    core::RawGroups out{{0, "LAYER"}, {5, handleText(layer.id.value)}};
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

/// The own 49 groups of an LTYPE record, as numbers; std::nullopt if one
/// is not a number.
std::optional<std::vector<double>> patternOf(const core::RawGroups& groups) {
    const auto own = ownGroups(groups);
    std::vector<double> elements;
    for (std::size_t i = 0; i < groups.size(); ++i) {
        if (own[i] && groups[i].code == 49) {
            const auto value = parseNumber(trimDxf(groups[i].value));
            if (!value) {
                return std::nullopt;
            }
            elements.push_back(*value);
        }
    }
    return elements;
}

/// The groups of an LTYPE record, from group 0 on.
core::RawGroups linetypeRecord(const core::Linetype& linetype, std::string_view owner) {
    core::RawGroups body;
    if (linetype.extras.empty()) {
        body = recordPreamble(owner, "AcDbLinetypeTableRecord");
        body.push_back({70, "0"});
    } else {
        body = linetype.extras;
    }
    // The elements as read while the pattern is the same: the complex ones
    // (74 not 0) need them (docs/adr/0025, point 1). Otherwise new ones.
    if (linetype.extras.empty() || patternOf(body) != linetype.pattern.elements) {
        auto own = ownGroups(body);
        std::optional<std::size_t> first;
        for (std::size_t i = body.size(); i-- > 0;) {
            if (own[i] && std::ranges::find(kElementCodes, body[i].code) != kElementCodes.end()) {
                body.erase(body.begin() + static_cast<std::ptrdiff_t>(i));
                first = i;
            }
        }
        // The alignment code, always 65 ('A').
        constexpr std::string_view kAlignment = "65";
        if (!firstOwn(body, ownGroups(body), 72)) {
            dxf_groups::insertInOrder(body, {{72, std::string{kAlignment}}}, kLinetypeOrder);
            first.reset();
        }
        core::RawGroups elements{{73, std::to_string(linetype.pattern.elements.size())},
                                 {40, formatNumber(linetype.pattern.length())}};
        for (const double element : linetype.pattern.elements) {
            elements.push_back({49, formatNumber(element)});
            elements.push_back({74, "0"});
        }
        if (first) {
            body.insert(body.begin() + static_cast<std::ptrdiff_t>(*first), elements.begin(),
                        elements.end());
        } else {
            const auto at72 = firstOwn(body, ownGroups(body), 72);
            const std::size_t at = at72 ? *at72 + 1 : body.size();
            body.insert(body.begin() + static_cast<std::ptrdiff_t>(at), elements.begin(),
                        elements.end());
        }
    }
    const core::RawGroups names{{2, linetype.name}, {3, linetype.description}};
    dxf_groups::insertInOrder(body, names, kLinetypeOrder);

    core::RawGroups out{{0, "LTYPE"}, {5, handleText(linetype.id.value)}};
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

/// A table of the TABLES section as read: its header (from 0 TABLE to the
/// first record), its records and placeholders, and its end.
struct TableParts {
    std::string name;
    core::RawGroups header;
    std::vector<core::RawGroups> items;  ///< a placeholder is one kRecordPlaceholder group
    core::RawGroups end;
};

bool isPlaceholder(const core::RawGroups& item) {
    return item.size() == 1 && item.front().code == kRecordPlaceholder;
}

/// The table that starts at `i` (a 0 TABLE group); `i` moves past it.
TableParts readTable(const core::RawGroups& groups, std::size_t& i) {
    TableParts table;
    table.header.push_back(groups[i++]);
    while (i < groups.size() && groups[i].code != 0 && groups[i].code != kRecordPlaceholder) {
        if (groups[i].code == 2 && table.name.empty()) {
            table.name = trimDxf(groups[i].value);
        }
        table.header.push_back(groups[i++]);
    }
    while (i < groups.size() && !isMarker(groups[i], "ENDTAB") && !isMarker(groups[i], "TABLE")) {
        core::RawGroups item{groups[i++]};
        if (item.front().code != kRecordPlaceholder) {
            while (i < groups.size() && groups[i].code != 0 &&
                   groups[i].code != kRecordPlaceholder) {
                item.push_back(groups[i++]);
            }
        }
        table.items.push_back(std::move(item));
    }
    if (i < groups.size() && isMarker(groups[i], "ENDTAB")) {
        table.end.push_back(groups[i++]);
        while (i < groups.size() && groups[i].code != 0) {
            table.end.push_back(groups[i++]);
        }
    }
    return table;
}

/// Writes a LAYER or LTYPE table with the records of the document.
void writeRecordTable(const TableParts& table, const core::Document& document,
                      core::RawGroups& out) {
    const bool layers = table.name == "LAYER";
    const std::size_t records = layers ? document.layers().size() : document.linetypes().size();
    const auto placeholders =
        static_cast<std::size_t>(std::ranges::count_if(table.items, isPlaceholder));
    std::string owner{"0"};
    bool counted = false;
    for (const auto& group : table.header) {
        if (group.code == 5 && owner == "0") {
            owner = trimDxf(group.value);
        }
        if (group.code == 70 && !counted) {
            counted = true;
            // The count follows the records added and removed: AutoCAD does
            // not count ByBlock and ByLayer, so it is not set to the total.
            const long long read = parseInteger(group.value).value_or(0);
            const long long now = std::max<long long>(
                0, read + static_cast<long long>(records) - static_cast<long long>(placeholders));
            out.push_back(now == read ? group : core::RawGroup{70, std::to_string(now)});
            continue;
        }
        out.push_back(group);
    }
    std::size_t next = 0;
    const auto writeNext = [&] {
        const auto groups = layers ? layerRecord(document.layers()[next], document, owner)
                                   : linetypeRecord(document.linetypes()[next], owner);
        out.insert(out.end(), groups.begin(), groups.end());
        ++next;
    };
    for (const auto& item : table.items) {
        if (!isPlaceholder(item)) {
            out.insert(out.end(), item.begin(), item.end());
        } else if (next < records) {
            writeNext();
        }
    }
    while (next < records) {
        writeNext();
    }
    if (table.end.empty()) {
        out.push_back({0, "ENDTAB"});
    } else {
        out.insert(out.end(), table.end.begin(), table.end.end());
    }
}

/// A table the file lacks, with a new handle.
void writeNewTable(std::string_view name, const core::Document& document, std::uint64_t& nextHandle,
                   core::RawGroups& out) {
    TableParts table;
    table.name = name;
    table.header = {{0, "TABLE"}, {2, std::string{name}},   {5, handleText(nextHandle++)},
                    {330, "0"},   {100, "AcDbSymbolTable"}, {70, "0"}};
    writeRecordTable(table, document, out);
}

bool isDefaultLayerZero(const core::Layer& layer, const core::Document& document) {
    return layer.name == kLayerZeroName &&
           layer.color == core::Color::fromIndex(kDefaultLayerColor) &&
           layer.linetype == document.continuousLinetype() &&
           layer.lineweight == core::lineweight::kDefault && layer.on && !layer.frozen &&
           !layer.locked && layer.plot && layer.extras.empty();
}

bool isMandatoryLinetype(const core::Linetype& linetype) {
    return (core::sameTableName(linetype.name, kByBlockName) ||
            core::sameTableName(linetype.name, kByLayerName) ||
            core::sameTableName(linetype.name, kContinuousName)) &&
           linetype.pattern.continuous() && linetype.extras.empty();
}

}  // namespace

// --- Reading ---------------------------------------------------------------

std::uint64_t TableReader::recordHandle(core::RawGroups& record, std::string_view type,
                                        std::size_t line, std::vector<std::string>& warnings) {
    const auto at = std::ranges::find(record, 5, &core::RawGroup::code);
    if (at != record.end()) {
        const auto handle = parseHandle(at->value).value_or(0);
        record.erase(at);
        return handle;
    }
    // A record without a handle gets one, as an entity (dxf_read.cpp).
    const std::uint64_t handle = m_document.nextId();
    m_document.setNextId(handle + 1);
    warnings.push_back("line " + std::to_string(line) + ": " + std::string{type} +
                       " record had no handle; assigned " + handleText(handle));
    return handle;
}

void TableReader::readLinetype(core::RawGroups record, std::size_t line,
                               std::vector<std::string>& warnings) {
    const auto own = ownGroups(record);
    const auto at2 = firstOwn(record, own, 2);
    const auto at3 = firstOwn(record, own, 3);
    const std::string name = at2 ? record[*at2].value : std::string{};
    const auto pattern = patternOf(record);
    if (!core::isValidTableName(name) || !pattern) {
        ++m_invalidLinetypes;
        return;
    }
    if (m_document.findLinetype(name) != nullptr) {
        ++m_duplicateLinetypes;
        return;
    }
    core::Linetype linetype;
    linetype.name = name;
    linetype.description = at3 ? record[*at3].value : std::string{};
    linetype.pattern.elements = *pattern;
    // Name and description are the model's; the rest stays as read.
    eraseAt(record, std::max(at2, at3));
    eraseAt(record, at2 && at3 ? std::min(at2, at3) : std::nullopt);
    linetype.id = core::LinetypeId{recordHandle(record, "LTYPE", line, warnings)};
    linetype.extras = std::move(record);
    m_document.addLinetype(std::move(linetype));
}

void TableReader::readLayer(core::RawGroups record, std::size_t line,
                            std::vector<std::string>& warnings) {
    const auto own = ownGroups(record);
    const auto at2 = firstOwn(record, own, 2);
    const auto at6 = firstOwn(record, own, 6);
    const auto at420 = firstOwn(record, own, 420);
    const std::string name = at2 ? record[*at2].value : std::string{};
    // The value of an integer group: `absent` when the record lacks it,
    // std::nullopt when it is not an integer.
    const auto integer = [&](int code, long long absent) -> std::optional<long long> {
        const auto found = firstOwn(record, own, code);
        return found ? parseInteger(record[*found].value) : std::optional<long long>{absent};
    };
    const auto flags = integer(70, 0);
    const auto color = integer(62, kDefaultLayerColor);
    const auto rgb = integer(420, 0);
    const auto plot = integer(290, 1);
    const auto weight = integer(370, core::lineweight::kDefault);
    if (!flags || !color || !rgb || !plot || !weight || !core::isValidTableName(name)) {
        ++m_invalidLayers;
        return;
    }
    // docs/adr/0024, point 2: a layer colour is an index (negative when the
    // layer is off) or an RGB; anything else keeps the record raw.
    const long long index = *color;
    if (index == 0 || std::llabs(index) > kMaxColorIndex || *rgb < 0 || *rgb > kMaxRgb) {
        ++m_invalidLayers;
        return;
    }
    if (layerPending(name)) {
        ++m_duplicateLayers;
        return;
    }
    PendingLayer pending;
    core::Layer& layer = pending.layer;
    layer.name = name;
    layer.color = at420 ? core::Color::fromRgb(rgbOf(*rgb))
                        : core::Color::fromIndex(static_cast<int>(std::llabs(index)));
    layer.on = index > 0;
    layer.frozen = (*flags & kFrozenBit) != 0;
    layer.locked = (*flags & kLockedBit) != 0;
    layer.plot = *plot != 0;
    auto lineweight = static_cast<int>(std::clamp<long long>(*weight, -4, 1000));
    if (lineweight != core::lineweight::kDefault && !core::isAllowedLineweight(lineweight)) {
        // docs/adr/0025, point 6; a layer is never ByLayer nor ByBlock.
        lineweight =
            lineweight < 0 ? core::lineweight::kDefault : core::nearestLineweight(lineweight);
        ++m_roundedLineweights;
    }
    layer.lineweight = lineweight;
    pending.linetype = at6 ? record[*at6].value : std::string{kContinuousName};
    // The name is the model's, written from the table; the 6 stays until
    // finish() knows how its linetype is spelt.
    eraseAt(record, at2);
    layer.id = core::LayerId{recordHandle(record, "LAYER", line, warnings)};
    layer.extras = std::move(record);
    m_layers.push_back(std::move(pending));
}

void TableReader::readTables(core::RawGroups& groups, std::span<const std::size_t> lines,
                             std::vector<std::string>& warnings) {
    // The layers wait for finish(), so a LAYER table before the LTYPE one
    // finds its linetypes all the same.
    core::RawGroups out;
    std::string table;
    std::size_t i = 0;
    while (i < groups.size()) {
        const auto& group = groups[i];
        if (isMarker(group, "TABLE")) {
            table = i + 1 < groups.size() && groups[i + 1].code == 2
                        ? std::string{trimDxf(groups[i + 1].value)}
                        : std::string{};
            m_layerTable = m_layerTable || table == "LAYER";
            m_linetypeTable = m_linetypeTable || table == "LTYPE";
        } else if (isMarker(group, "ENDTAB")) {
            table.clear();
        }
        const bool linetype = table == "LTYPE" && isMarker(group, "LTYPE");
        if (!linetype && !(table == "LAYER" && isMarker(group, "LAYER"))) {
            out.push_back(group);
            ++i;
            continue;
        }
        const std::size_t start = i++;
        core::RawGroups record;
        while (i < groups.size() && groups[i].code != 0) {
            record.push_back(groups[i++]);
        }
        const std::size_t line = start < lines.size() ? lines[start] : 0;
        const auto layersBefore = m_layers.size();
        const auto linetypesBefore = m_document.linetypes().size();
        if (linetype) {
            readLinetype(record, line, warnings);
        } else {
            readLayer(record, line, warnings);
        }
        if (m_layers.size() > layersBefore || m_document.linetypes().size() > linetypesBefore) {
            out.push_back({kRecordPlaceholder, ""});
            continue;
        }
        // Kept raw: its handle stays reserved, as for any other object.
        const auto handle = std::ranges::find(record, 5, &core::RawGroup::code);
        if (handle != record.end()) {
            if (const auto value = parseHandle(handle->value); value && *value != 0) {
                m_document.reserveHandle(core::EntityId{*value});
            }
        }
        out.push_back(group);
        out.insert(out.end(), record.begin(), record.end());
    }
    groups = std::move(out);
}

bool TableReader::layerPending(std::string_view name) const {
    return std::ranges::any_of(m_layers, [name](const PendingLayer& pending) {
        return core::sameTableName(pending.layer.name, name);
    });
}

void TableReader::readHeader(core::RawGroups& header) {
    core::RawGroups out;
    std::string_view variable;
    for (const auto& group : header) {
        if (group.code == 9) {
            variable = trimDxf(group.value);
            out.push_back(group);
            continue;
        }
        const auto integer = parseInteger(group.value);
        const auto number = parseNumber(trimDxf(group.value));
        if (variable == "$CLAYER" && group.code == 8) {
            if (core::isValidTableName(group.value)) {
                m_currentLayer = group.value;
                continue;  // the writer puts the current layer's name here
            }
        } else if (variable == "$CELTYPE" && group.code == 6) {
            if (core::isValidTableName(group.value)) {
                m_currentLinetype = group.value;
                continue;
            }
        } else if (variable == "$CECOLOR" && group.code == 62 && integer) {
            constexpr long long kByLayer = 256;
            if (*integer == kByLayer) {
                m_current.color = core::Color::byLayer();
            } else if (*integer == 0) {
                m_current.color = core::Color::byBlock();
            } else if (*integer > 0 && *integer <= kMaxColorIndex) {
                m_current.color = core::Color::fromIndex(static_cast<int>(*integer));
            }
        } else if (variable == "$CECOLOR" && group.code == 420 && integer && *integer >= 0 &&
                   *integer <= kMaxRgb) {
            m_current.color = core::Color::fromRgb(rgbOf(*integer));
        } else if (variable == "$CELWEIGHT" && group.code == 370 && integer) {
            const auto value = static_cast<int>(std::clamp<long long>(*integer, -4, 1000));
            if (value == core::lineweight::kByLayer || value == core::lineweight::kByBlock ||
                value == core::lineweight::kDefault || core::isAllowedLineweight(value)) {
                m_current.lineweight = value;
            } else if (value >= 0) {
                m_current.lineweight = core::nearestLineweight(value);
                ++m_roundedLineweights;
            }
        } else if (variable == "$CELTSCALE" && group.code == 40 && number && *number > 0.0) {
            m_current.linetypeScale = *number;
        } else if (variable == "$LTSCALE" && group.code == 40 && number && *number > 0.0) {
            m_document.setLtscale(*number);
        } else if (variable == "$LWDISPLAY" && group.code == 290 && integer) {
            m_document.setLwdisplay(*integer != 0);
        } else if (variable == "$LWDEFAULT" && group.code == 370 && integer) {
            const auto value = static_cast<int>(std::clamp<long long>(*integer, 0, 1000));
            if (!core::isAllowedLineweight(value) || value != *integer) {
                ++m_roundedLineweights;
            }
            m_document.setLwdefault(core::nearestLineweight(value));
        }
        out.push_back(group);
    }
    header = std::move(out);
}

std::optional<core::LayerId> TableReader::knownLayer(std::string_view name) const {
    const auto found = std::ranges::find_if(m_layers, [name](const PendingLayer& pending) {
        return core::sameTableName(pending.layer.name, name);
    });
    if (found != m_layers.end()) {
        return found->layer.id;
    }
    return std::nullopt;
}

bool TableReader::entityProperties(core::EntityRecord& record) {
    const core::EntityExtras original = record.extras;
    auto groups = takeEntityProperties(record.extras);
    if (!groups) {
        return false;
    }
    // The layers of the file are known by their handles, the linetypes are
    // in the document already; what the file lacks waits for finish().
    const auto layer = knownLayer(groups->layer);
    const core::Linetype* linetype = groups->linetype ? m_document.findLinetype(*groups->linetype)
                                                      : m_document.findLinetype(kByLayerName);
    // A name the tables cannot hold keeps the entity unhandled.
    if ((!layer && !core::isValidTableName(groups->layer)) ||
        (linetype == nullptr && groups->linetype && !core::isValidTableName(*groups->linetype))) {
        return false;
    }
    // A name spelt otherwise than its record stays in the extras, so that
    // it is written again as read (docs/adr/0018, point 6).
    const std::string* layerSpelling = nullptr;
    for (const auto& pending : m_layers) {
        if (layer && pending.layer.id == *layer) {
            layerSpelling = &pending.layer.name;
        }
    }
    const bool keepLayer = layerSpelling != nullptr && *layerSpelling != groups->layer;
    const bool keepLinetype =
        linetype != nullptr && groups->linetype && linetype->name != *groups->linetype;
    if (keepLayer || keepLinetype) {
        record.extras = original;
        groups = takeEntityProperties(record.extras, keepLayer, keepLinetype);
        if (!groups) {
            return false;  // never: the same groups were valid above
        }
    }
    if (groups->lineweightRounded) {
        ++m_roundedLineweights;
    }
    auto& properties = record.properties;
    properties.layer = layer.value_or(core::LayerId{});
    properties.linetype = linetype != nullptr ? linetype->id : core::LinetypeId{};
    properties.color = groups->color;
    properties.lineweight = groups->lineweight;
    properties.linetypeScale = groups->linetypeScale;
    if (!layer || linetype == nullptr) {
        m_entities.push_back(
            {record.id, properties, std::move(groups->layer), std::move(groups->linetype)});
    }
    return true;
}

bool TableReader::needsRecords() const {
    if (m_layerTable || m_linetypeTable || !m_layers.empty() || !m_document.linetypes().empty()) {
        return true;
    }
    const bool defaultCurrent =
        m_current.color == core::Color::byLayer() &&
        m_current.lineweight == core::lineweight::kByLayer && m_current.linetypeScale == 1.0 &&
        core::sameTableName(m_currentLayer.value_or(std::string{kLayerZeroName}), kLayerZeroName) &&
        core::sameTableName(m_currentLinetype.value_or(std::string{kByLayerName}), kByLayerName);
    return !defaultCurrent || std::ranges::any_of(m_entities, [](const PendingEntity& pending) {
        return pending.linetype || !core::sameTableName(pending.layer, kLayerZeroName);
    });
}

core::LinetypeId TableReader::linetypeNamed(std::string_view name) {
    if (const core::Linetype* found = m_document.findLinetype(name)) {
        return found->id;
    }
    // docs/adr/0023, point 6: a linetype without dashes, drawn continuous.
    ++m_missingLinetypes;
    return m_document.addLinetype(core::Linetype{{}, std::string{name}, "", {}, {}});
}

core::LayerId TableReader::layerNamed(std::string_view name) {
    if (const core::Layer* found = m_document.findLayer(name)) {
        return found->id;
    }
    // docs/adr/0023, points 5 and 6: the values of a new layer 0.
    ++m_missingLayers;
    core::Layer layer;
    layer.name = name;
    layer.linetype = m_document.continuousLinetype();
    return m_document.addLayer(std::move(layer));
}

void TableReader::finish(std::vector<std::string>& warnings,
                         const std::unordered_set<std::uint64_t>& usedHandles) {
    const auto warn = [&warnings](int count, std::string_view singular, std::string_view rest) {
        if (count != 0) {
            warnings.push_back(plural(count, singular, rest));
        }
    };
    if (!needsRecords()) {
        warn(m_roundedLineweights, "lineweight", " rounded.");
        return;
    }
    // Every drawing has these (docs/adr/0023, point 1; 0025, point 2): a
    // file may leave them implied, and that is not an inconsistency.
    //
    // Nothing is written for them, so they must not spend the handles from
    // $HANDSEED on: the first entity drawn after OPEN takes $HANDSEED. They
    // take the lowest free handles below it; only a file that spends them
    // all gives them the next ones.
    std::uint64_t candidate = 1;
    const auto lowFreeHandle = [&]() -> std::uint64_t {
        while (candidate < m_document.nextId()) {
            const std::uint64_t handle = candidate++;
            if (!usedHandles.contains(handle)) {
                return handle;
            }
        }
        return 0;  // none below $HANDSEED: the document gives the next one
    };
    for (const std::string_view name : {kByBlockName, kByLayerName, kContinuousName}) {
        if (m_document.findLinetype(name) == nullptr) {
            m_document.addLinetype(
                core::Linetype{core::LinetypeId{lowFreeHandle()},
                               std::string{name},
                               std::string{name == kContinuousName ? kContinuousDescription : ""},
                               {},
                               {}});
        }
    }
    const core::LinetypeId byLayer = m_document.byLayerLinetype();
    for (auto& pending : m_layers) {
        const core::Linetype* named = m_document.findLinetype(pending.linetype);
        if (core::sameTableName(pending.linetype, kByLayerName) ||
            core::sameTableName(pending.linetype, kByBlockName) ||
            (named == nullptr && !core::isValidTableName(pending.linetype))) {
            // A layer is never ByLayer nor ByBlock (docs/adr/0023, point 1).
            ++m_replacedLayerLinetypes;
            pending.layer.linetype = m_document.continuousLinetype();
        } else {
            pending.layer.linetype = linetypeNamed(pending.linetype);
        }
        // The 6 stays as a witness of its spelling only when it spells the
        // name of its linetype otherwise ("CONTINUOUS" for an implied
        // Continuous): renaming a layer looks for its name in the extras.
        auto& extras = pending.layer.extras;
        const auto at6 = firstOwn(extras, ownGroups(extras), 6);
        const core::Linetype* linetype = m_document.linetype(pending.layer.linetype);
        if (at6 && linetype != nullptr && extras[*at6].value == linetype->name) {
            eraseAt(extras, at6);
        }
        m_document.addLayer(std::move(pending.layer));
    }
    if (m_document.findLayer(kLayerZeroName) == nullptr) {
        core::Layer zero;
        zero.name = kLayerZeroName;
        zero.linetype = m_document.continuousLinetype();
        m_document.addLayer(std::move(zero));
    }
    for (auto& pending : m_entities) {
        auto& properties = pending.properties;
        if (properties.layer == core::LayerId{}) {
            properties.layer = layerNamed(pending.layer);
        }
        if (properties.linetype == core::LinetypeId{}) {
            properties.linetype = pending.linetype ? linetypeNamed(*pending.linetype) : byLayer;
        }
        m_document.setProperties(pending.id, properties);
    }
    m_current.layer = layerNamed(m_currentLayer.value_or(std::string{kLayerZeroName}));
    m_current.linetype = m_currentLinetype ? linetypeNamed(*m_currentLinetype) : byLayer;
    m_document.setCurrent(m_current);

    warn(m_missingLayers, "missing layer", " added.");
    warn(m_missingLinetypes, "missing linetype", " added.");
    warn(m_duplicateLayers, "duplicate layer", " kept as unhandled data.");
    warn(m_duplicateLinetypes, "duplicate linetype", " kept as unhandled data.");
    warn(m_invalidLayers, "invalid layer", " kept as unhandled data.");
    warn(m_invalidLinetypes, "invalid linetype", " kept as unhandled data.");
    warn(m_replacedLayerLinetypes, "layer linetype", " replaced by Continuous.");
    warn(m_roundedLineweights, "lineweight", " rounded.");
}

// --- Writing ---------------------------------------------------------------

bool needsLayerTable(const core::Document& document) {
    const auto layers = document.layers();
    return layers.size() > 1 || (layers.size() == 1 && !isDefaultLayerZero(layers[0], document));
}

bool needsLinetypeTable(const core::Document& document) {
    return !std::ranges::all_of(document.linetypes(), isMandatoryLinetype);
}

void writeTables(const core::RawGroups& tables, const core::Document& document,
                 std::uint64_t& nextHandle, core::RawGroups& out) {
    // A table already in the section is never added a second time, wherever
    // it stands.
    bool linetypes = false;
    bool layers = false;
    for (std::size_t i = 0; i + 1 < tables.size(); ++i) {
        if (isMarker(tables[i], "TABLE") && tables[i + 1].code == 2) {
            const auto name = trimDxf(tables[i + 1].value);
            linetypes = linetypes || name == "LTYPE";
            layers = layers || name == "LAYER";
        }
    }
    const auto addMissing = [&](std::string_view before) {
        if (!linetypes && (before.empty() ||
                           std::ranges::find(kAfterLinetypes, before) != kAfterLinetypes.end())) {
            linetypes = true;
            if (needsLinetypeTable(document)) {
                writeNewTable("LTYPE", document, nextHandle, out);
            }
        }
        if (!layers &&
            (before.empty() || std::ranges::find(kAfterLayers, before) != kAfterLayers.end())) {
            layers = true;
            if (needsLayerTable(document)) {
                writeNewTable("LAYER", document, nextHandle, out);
            }
        }
    };
    std::size_t i = 0;
    while (i < tables.size()) {
        if (!isMarker(tables[i], "TABLE")) {
            out.push_back(tables[i++]);
            continue;
        }
        const auto table = readTable(tables, i);
        if (table.name == "LTYPE" || table.name == "LAYER") {
            (table.name == "LTYPE" ? linetypes : layers) = true;
            writeRecordTable(table, document, out);
            continue;
        }
        addMissing(table.name);
        out.insert(out.end(), table.header.begin(), table.header.end());
        for (const auto& item : table.items) {
            out.insert(out.end(), item.begin(), item.end());
        }
        out.insert(out.end(), table.end.begin(), table.end.end());
    }
    addMissing({});
}

std::uint64_t tablesToAdd(const core::RawDrawing& raw, const core::Document& document) {
    bool linetypes = false;
    bool layers = false;
    for (const auto& section : raw.sections) {
        if (section.name != "TABLES") {
            continue;
        }
        const auto& groups = section.groups;
        for (std::size_t i = 0; i + 1 < groups.size(); ++i) {
            if (isMarker(groups[i], "TABLE") && groups[i + 1].code == 2) {
                const auto name = trimDxf(groups[i + 1].value);
                linetypes = linetypes || name == "LTYPE";
                layers = layers || name == "LAYER";
            }
        }
    }
    return (!linetypes && needsLinetypeTable(document) ? 1U : 0U) +
           (!layers && needsLayerTable(document) ? 1U : 0U);
}

core::RawGroups newTables(const core::Document& document, std::uint64_t& nextHandle) {
    core::RawGroups out;
    if (needsLinetypeTable(document) || needsLayerTable(document)) {
        writeTables({}, document, nextHandle, out);
    }
    return out;
}

}  // namespace tg::io
