// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/io/block_import.hpp"

#include "dxf_blocks.hpp"
#include "dxf_entities.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/tolerance.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>

namespace tg::io {
namespace {

/// The groups that name a handle and take the new one when it is renumbered
/// (docs/tappe/0.8/PT19-file-come-blocco.md, point 4): the owner and the
/// reactors (330), the soft and hard pointers (340, 360).
constexpr std::array<std::int16_t, 3> kPointerCodes{330, 340, 360};

/// The text of `path` as io::FileAccess takes it: UTF-8.
std::string utf8(const std::filesystem::path& path) {
    const std::u8string text = path.u8string();
    return {text.begin(), text.end()};
}

/// $INSBASE of the HEADER kept in `raw`, (0,0) if absent.
core::Vec2 insertionBase(const core::RawDrawing& raw) {
    core::Vec2 base{};
    for (const core::RawSection& section : raw.sections) {
        if (section.name != "HEADER") {
            continue;
        }
        bool inside = false;
        for (const core::RawGroup& group : section.groups) {
            if (group.code == 9) {
                inside = trimDxf(group.value) == "$INSBASE";
            } else if (inside && (group.code == 10 || group.code == 20)) {
                const double value = parseNumber(trimDxf(group.value)).value_or(0.0);
                (group.code == 10 ? base.x : base.y) = value;
            }
        }
    }
    return base;
}

/// The groups of an unhandled entity up to its first nested group 0 (the
/// ATTRIB and SEQEND of an INSERT kept whole).
std::span<const core::RawGroup> ownGroups(const core::RawEntity& raw) {
    const auto nested = std::ranges::find(raw.groups, std::int16_t{0}, &core::RawGroup::code);
    return {raw.groups.begin(), nested};
}

/// True for an unhandled entity of paper space (67 not 0), as the reader
/// tells them apart.
bool inPaperSpace(const core::RawEntity& raw) {
    return std::ranges::any_of(ownGroups(raw), [](const core::RawGroup& group) {
        return group.code == 67 && parseNumber(trimDxf(group.value)).value_or(0.0) != 0.0;
    });
}

/// The name of the block an unhandled INSERT names (its group 2), if any.
std::optional<std::string_view> rawInsertBlock(const core::RawEntity& raw) {
    if (trimDxf(raw.type) != "INSERT") {
        return std::nullopt;
    }
    for (const core::RawGroup& group : ownGroups(raw)) {
        if (group.code == 2) {
            return std::string_view{group.value};
        }
    }
    return std::nullopt;
}

/// Every handle of the file, in file order: the first group 5 (105 for a
/// DIMSTYLE record) after each group 0, out of the HEADER.
std::vector<std::uint64_t> fileOrder(const GroupFile& file) {
    std::vector<std::uint64_t> order;
    std::string_view section;
    std::string_view type;
    bool expectingName = false;
    bool taken = true;
    for (const core::RawGroup& group : file.groups) {
        if (group.code == 0) {
            type = trimDxf(group.value);
            expectingName = type == "SECTION";
            taken = false;
            continue;
        }
        if (expectingName && group.code == 2) {
            section = trimDxf(group.value);
            expectingName = false;
            continue;
        }
        const int handleCode = section == "TABLES" && type == "DIMSTYLE" ? 105 : 5;
        if (taken || section == "HEADER" || group.code != handleCode) {
            continue;
        }
        taken = true;
        if (const auto handle = parseHandle(group.value); handle && *handle != 0) {
            order.push_back(*handle);
        }
    }
    return order;
}

/// The handles of what enters, and the new handle of each.
class Renumbering {
public:
    void add(std::uint64_t handle) {
        if (handle != 0) {
            m_entering.insert(handle);
        }
    }

    /// Every handle of `groups` with code 5 (an entity kept whole: its
    /// ATTRIB and SEQEND too).
    void addHandles(const core::RawGroups& groups) {
        for (const core::RawGroup& group : groups) {
            if (group.code == 5) {
                add(parseHandle(group.value).value_or(0));
            }
        }
    }

    void addRecord(const core::EntityRecord& record) {
        add(record.id.value);
        // The SEQEND of an INSERT stays in extras.specific with its handle.
        addHandles(record.extras.specific);
        if (const auto* insert = std::get_if<core::Insert>(&record.entity)) {
            for (const core::Attribute& attribute : insert->attributes) {
                add(attribute.id.value);
            }
        }
    }

    void addRaw(const core::RawEntity& raw) {
        add(raw.id.value);
        addHandles(raw.groups);
    }

    void addBlock(const core::BlockDefinition& block) {
        add(block.recordId);
        add(block.blockId);
        add(block.endBlockId);
        for (const core::EntityRecord& record : block.entities) {
            addRecord(record);
        }
        for (const core::RawEntity& raw : block.rawEntities) {
            addRaw(raw);
        }
    }

    /// Gives the new handles from `first` on: first in file order, then
    /// those the file did not write (given by the reader), in increasing
    /// order. Returns the handle after the last one given.
    std::uint64_t number(const std::vector<std::uint64_t>& order, std::uint64_t first) {
        std::uint64_t next = first;
        for (const std::uint64_t handle : order) {
            if (m_entering.contains(handle) && !m_new.contains(handle)) {
                m_new.emplace(handle, next++);
            }
        }
        std::vector<std::uint64_t> rest;
        for (const std::uint64_t handle : m_entering) {
            if (!m_new.contains(handle)) {
                rest.push_back(handle);
            }
        }
        std::ranges::sort(rest);
        for (const std::uint64_t handle : rest) {
            m_new.emplace(handle, next++);
        }
        return next;
    }

    [[nodiscard]] std::uint64_t of(std::uint64_t handle) const {
        const auto found = m_new.find(handle);
        return found != m_new.end() ? found->second : handle;
    }

    /// Renumbers the groups of `groups` with a code of kPointerCodes, and
    /// with code 5 if `withHandles`, that name a renumbered handle; the
    /// others stay as read.
    void groups(core::RawGroups& groups, bool withHandles) const {
        for (core::RawGroup& group : groups) {
            const bool pointer =
                std::ranges::find(kPointerCodes, group.code) != kPointerCodes.end();
            if (!pointer && !(withHandles && group.code == 5)) {
                continue;
            }
            const auto handle = parseHandle(group.value);
            if (handle && m_new.contains(*handle)) {
                group.value = handleText(m_new.at(*handle));
            }
        }
    }

private:
    std::unordered_set<std::uint64_t> m_entering;
    std::unordered_map<std::uint64_t, std::uint64_t> m_new;
};

/// The records of the file's tables mapped to those that the entities use
/// after the import: the new handle of a record that enters, the handle of
/// the record of `target` with the same name otherwise.
struct TableMaps {
    std::unordered_map<std::uint64_t, std::uint64_t> layers;
    std::unordered_map<std::uint64_t, std::uint64_t> linetypes;

    [[nodiscard]] static std::uint64_t mapped(
        const std::unordered_map<std::uint64_t, std::uint64_t>& map, std::uint64_t id) {
        const auto found = map.find(id);
        return found != map.end() ? found->second : id;
    }

    void properties(core::EntityProperties& properties) const {
        properties.layer.value = mapped(layers, properties.layer.value);
        properties.linetype.value = mapped(linetypes, properties.linetype.value);
    }
};

/// Renumbers every handle of an entity and maps its properties.
void renumber(core::EntityExtras& extras, const Renumbering& numbers) {
    for (core::RawGroups* groups : {&extras.beforeEntity, &extras.common, &extras.specific,
                                    &extras.placement, &extras.extended}) {
        numbers.groups(*groups, true);
    }
}

void renumber(core::EntityRecord& record, const Renumbering& numbers, const TableMaps& tables) {
    record.id.value = numbers.of(record.id.value);
    renumber(record.extras, numbers);
    tables.properties(record.properties);
    if (auto* insert = std::get_if<core::Insert>(&record.entity)) {
        for (core::Attribute& attribute : insert->attributes) {
            attribute.id.value = numbers.of(attribute.id.value);
            renumber(attribute.extras, numbers);
            tables.properties(attribute.properties);
        }
    }
}

void renumber(core::RawEntity& raw, const Renumbering& numbers) {
    raw.id.value = numbers.of(raw.id.value);
    numbers.groups(raw.groups, true);
}

void renumber(core::BlockDefinition& block, const Renumbering& numbers, const TableMaps& tables) {
    block.recordId = numbers.of(block.recordId);
    block.blockId = numbers.of(block.blockId);
    block.endBlockId = numbers.of(block.endBlockId);
    numbers.groups(block.recordExtras, false);
    numbers.groups(block.blockExtras, false);
    numbers.groups(block.endBlockExtras, false);
    for (core::EntityRecord& record : block.entities) {
        renumber(record, numbers, tables);
    }
    for (core::RawEntity& raw : block.rawEntities) {
        renumber(raw, numbers);
    }
}

/// The names of the blocks inserted by the entities of `block`, handled or
/// not.
std::vector<std::string_view> insertedBlocks(const core::BlockDefinition& block) {
    std::vector<std::string_view> names;
    for (const core::EntityRecord& record : block.entities) {
        if (const auto* insert = std::get_if<core::Insert>(&record.entity)) {
            names.emplace_back(insert->block);
        }
    }
    for (const core::RawEntity& raw : block.rawEntities) {
        if (const auto name = rawInsertBlock(raw)) {
            names.push_back(*name);
        }
    }
    return names;
}

/// True when `definition` would insert itself, directly or through the
/// blocks of `nested` or of `target` (docs/tappe/0.8/PT19, point 6).
bool referencesItself(const core::BlockDefinition& definition,
                      const std::vector<core::BlockDefinition>& nested,
                      const core::Document& target) {
    const auto find = [&](std::string_view name) -> const core::BlockDefinition* {
        const auto found = std::ranges::find_if(nested, [name](const core::BlockDefinition& block) {
            return core::sameTableName(block.name, name);
        });
        return found != nested.end() ? &*found : target.blocks().find(name);
    };
    std::vector<const core::BlockDefinition*> pending{&definition};
    std::unordered_set<const core::BlockDefinition*> seen;
    while (!pending.empty()) {
        const core::BlockDefinition* block = pending.back();
        pending.pop_back();
        for (const std::string_view name : insertedBlocks(*block)) {
            if (core::sameTableName(name, definition.name)) {
                return true;
            }
            const core::BlockDefinition* inner = find(name);
            if (inner != nullptr && seen.insert(inner).second) {
                pending.push_back(inner);
            }
        }
    }
    return false;
}

/// The handled and unhandled entities of the model space of `document`, in
/// their order; the unhandled ones of paper space are left out.
void modelSpace(const core::Document& document, core::BlockDefinition& definition) {
    for (const core::EntityRecord& record : document.entities()) {
        definition.entities.push_back(record);
    }
    for (const core::RawEntity& raw : document.rawEntities()) {
        if (!inPaperSpace(raw)) {
            definition.rawEntities.push_back(raw);
        }
    }
}

/// Moves the handled entities of `definition` by -`base` and gives it the
/// base (0,0), when it has no unhandled entity and every move is accepted;
/// otherwise keeps `base` and every entity as read (docs/DECISIONI.md,
/// 2026-11-13, "Buchi della revisione della #756").
void placeAtOrigin(const core::Document& document, core::BlockDefinition& definition,
                   const core::Vec2& base) {
    definition.base = base;
    if (base == core::Vec2{} || !definition.rawEntities.empty()) {
        return;
    }
    const core::Similarity move = core::Similarity::translation(-base);
    std::vector<core::Entity> moved;
    moved.reserve(definition.entities.size());
    for (const core::EntityRecord& record : definition.entities) {
        core::TransformOutcome outcome =
            core::transformedIn(document, record.entity, move, core::MirrorText::Readable);
        if (!outcome.entity) {
            return;
        }
        moved.push_back(std::move(*outcome.entity));
    }
    for (std::size_t i = 0; i < moved.size(); ++i) {
        definition.entities[i].entity = std::move(moved[i]);
    }
    definition.base = core::Vec2{};
}

/// The records of `records` whose name `has` does not find: they enter,
/// and their handles (`idOf`) go to `numbers`.
template <typename Record, typename Id, typename Has>
std::vector<Record> entering(std::span<const Record> records, const Id& idOf, const Has& has,
                             Renumbering& numbers) {
    std::vector<Record> result;
    for (const Record& record : records) {
        if (!has(record.name)) {
            numbers.add(idOf(record));
            result.push_back(record);
        }
    }
    return result;
}

}  // namespace

Result<BlockImport> importBlock(const std::filesystem::path& source, const core::Document& target,
                                FileAccess& files) {
    Result<std::string> bytes = files.read(utf8(source));
    if (!bytes.ok()) {
        return bytes.error();
    }
    Result<ReadResult> read = readDxf(bytes.value());
    if (!read.ok()) {
        return read.error();
    }
    const Result<GroupFile> groups = readGroups(bytes.value());
    if (!groups.ok()) {
        return groups.error();
    }
    const core::Document& file = read.value().document;

    BlockImport result;
    core::BlockDefinition& definition = result.definition;
    definition.name = utf8(source.stem());
    definition.units = file.units();
    modelSpace(file, definition);
    placeAtOrigin(file, definition, insertionBase(file.raw()));

    // Tables (docs/adr/0045, point 7): only the names the drawing lacks.
    Renumbering numbers;
    result.layers = entering(
        file.layers(), [](const core::Layer& layer) { return layer.id.value; },
        [&target](std::string_view name) { return target.findLayer(name) != nullptr; }, numbers);
    result.linetypes = entering(
        file.linetypes(), [](const core::Linetype& linetype) { return linetype.id.value; },
        [&target](std::string_view name) { return target.findLinetype(name) != nullptr; }, numbers);
    result.textStyles = entering(
        file.textStyles().styles(), [](const core::TextStyle& style) { return style.id; },
        [&target](std::string_view name) { return target.textStyles().find(name) != nullptr; },
        numbers);
    result.dimStyles = entering(
        file.dimStyles().styles(), [](const core::DimStyle& style) { return style.id; },
        [&target](std::string_view name) { return target.dimStyles().find(name) != nullptr; },
        numbers);
    for (const core::BlockDefinition& block : file.blocks().definitions()) {
        if (target.blocks().find(block.name) == nullptr &&
            !core::sameTableName(block.name, definition.name)) {
            result.nested.push_back(block);
        }
    }
    if (referencesItself(definition, result.nested, target)) {
        return Error{Error::Kind::Unsupported, 0,
                     "Block \"" + definition.name + "\" references itself."};
    }

    // Handles (point 4): renumbered from target.nextId() in file order.
    numbers.addBlock(definition);
    for (const core::BlockDefinition& block : result.nested) {
        numbers.addBlock(block);
    }
    result.nextId = numbers.number(fileOrder(groups.value()), target.nextId());

    TableMaps tables;
    for (const core::Layer& layer : file.layers()) {
        const core::Layer* kept = target.findLayer(layer.name);
        tables.layers.emplace(layer.id.value,
                              kept != nullptr ? kept->id.value : numbers.of(layer.id.value));
    }
    for (const core::Linetype& linetype : file.linetypes()) {
        const core::Linetype* kept = target.findLinetype(linetype.name);
        tables.linetypes.emplace(linetype.id.value,
                                 kept != nullptr ? kept->id.value : numbers.of(linetype.id.value));
    }
    for (core::Layer& layer : result.layers) {
        layer.id.value = numbers.of(layer.id.value);
        layer.linetype.value = TableMaps::mapped(tables.linetypes, layer.linetype.value);
        numbers.groups(layer.extras, false);
    }
    for (core::Linetype& linetype : result.linetypes) {
        linetype.id.value = numbers.of(linetype.id.value);
        numbers.groups(linetype.extras, false);
    }
    for (core::TextStyle& style : result.textStyles) {
        style.id = numbers.of(style.id);
        numbers.groups(style.extras, false);
    }
    for (core::DimStyle& style : result.dimStyles) {
        style.id = numbers.of(style.id);
        numbers.groups(style.extras, false);
    }
    renumber(definition, numbers, tables);
    for (core::BlockDefinition& block : result.nested) {
        renumber(block, numbers, tables);
    }

    // Units (point 5).
    const int from = file.units();
    const int to = target.units();
    if (from != 0 && to != 0 && from != to) {
        result.unitFactor = core::unitInMeters(from) / core::unitInMeters(to);
    }
    return result;
}

}  // namespace tg::io
