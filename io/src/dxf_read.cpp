// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_blocks.hpp"
#include "dxf_entities.hpp"
#include "dxf_tables.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <algorithm>
#include <exception>
#include <limits>
#include <unordered_set>
#include <utility>

namespace tg::io {
namespace {

struct SectionRange {
    std::string name;
    std::size_t begin{0};
    std::size_t end{0};
};

Error malformed(std::size_t line, std::string message) {
    return {Error::Kind::Malformed, line, std::move(message)};
}

bool isMarker(const core::RawGroup& group, std::string_view name) {
    return group.code == 0 && trimDxf(group.value) == name;
}

/// The DXF reference gives group 999 the meaning of a comment, allowed
/// anywhere in the file.
constexpr int kCommentCode = 999;

/// The comments found outside every section.
struct OutsideComments {
    std::size_t count{0};
    std::size_t firstLine{0};
};

/// Moves `i` past the comments that stand between sections: before the first,
/// after an ENDSEC, before EOF. Inside a section they stay among its groups,
/// kept as read like any other group the model does not represent.
void skipComments(const GroupFile& file, std::size_t& i, OutsideComments& skipped) {
    while (i < file.groups.size() && file.groups[i].code == kCommentCode) {
        if (skipped.count++ == 0) {
            skipped.firstLine = file.lines[i];
        }
        ++i;
    }
}

Result<std::vector<SectionRange>> sections(const GroupFile& file, std::string_view bytes,
                                           std::vector<std::string>& warnings) {
    const auto& groups = file.groups;
    std::vector<SectionRange> ranges;
    std::unordered_set<std::string> names;
    OutsideComments comments;
    std::size_t i = 0;
    skipComments(file, i, comments);
    while (i < groups.size() && !isMarker(groups[i], "EOF")) {
        if (!isMarker(groups[i], "SECTION") || i + 1 >= groups.size() || groups[i + 1].code != 2) {
            return malformed(file.lines[i], "expected SECTION followed by its name");
        }
        std::string name{trimDxf(groups[i + 1].value)};
        if (name.empty() || !names.insert(name).second) {
            return malformed(file.lines[i + 1], "empty or duplicate section name");
        }
        const std::size_t begin = i + 2;
        i = begin;
        while (i < groups.size() && !isMarker(groups[i], "ENDSEC")) {
            if (isMarker(groups[i], "SECTION") || isMarker(groups[i], "EOF")) {
                return malformed(file.lines[i], "expected ENDSEC before SECTION or EOF");
            }
            ++i;
        }
        if (i == groups.size()) {
            return malformed(file.lines.back(), "expected ENDSEC before the end of the file");
        }
        ranges.push_back({std::move(name), begin, i});
        ++i;
        skipComments(file, i, comments);
    }
    if (i == groups.size()) {
        return malformed(file.lines.empty() ? 1 : file.lines.back(), "expected EOF");
    }
    // readGroups intentionally warns about trailing data; the document reader
    // must reject it. Count physical lines, so CRLF and a final bare EOF work.
    std::size_t position = 0;
    for (std::size_t line = 0; line < file.lines[i] + 1 && position < bytes.size(); ++line) {
        const auto newline = bytes.find('\n', position);
        position = newline == std::string_view::npos ? bytes.size() : newline + 1;
    }
    if (!trimDxf(bytes.substr(position)).empty()) {
        return malformed(file.lines[i] + 2, "unexpected data after EOF");
    }
    // A comment is not drawing data, but it is not dropped without a word
    // either (ADR 0006): the report says how many were left out.
    if (comments.count > 0) {
        warnings.push_back("line " + std::to_string(comments.firstLine) + ": ignored " +
                           std::to_string(comments.count) + " comment " +
                           (comments.count == 1 ? "group" : "groups") +
                           " (999) outside any section");
    }
    return ranges;
}

struct Handles {
    std::unordered_set<std::uint64_t> used;
    std::uint64_t next{1};
};

Result<Handles> collectHandles(const GroupFile& file, const std::vector<SectionRange>& ranges,
                               core::Document& document) {
    Handles handles;
    document.setUnits(0);
    constexpr auto kMaximum = std::numeric_limits<std::uint64_t>::max();
    for (const auto& range : ranges) {
        std::string_view variable;
        std::string_view type;
        std::string_view table;
        bool hasHandle = false;
        for (std::size_t i = range.begin; i < range.end; ++i) {
            const auto& group = file.groups[i];
            if (group.code == 0) {
                type = trimDxf(group.value);
                hasHandle = false;
                // The table as TableReader::readTables sees it: every TABLE
                // starts a new one, named by the group 2 right after it, even
                // when the file did not end the previous one with ENDTAB.
                // Otherwise a record read into the document would find its
                // handle reserved here.
                if (type == "TABLE") {
                    table = i + 1 < range.end && file.groups[i + 1].code == 2
                                ? trimDxf(file.groups[i + 1].value)
                                : std::string_view{};
                } else if (type == "ENDTAB") {
                    table = {};
                }
            }
            if (range.name == "HEADER") {
                if (group.code == 9) {
                    variable = trimDxf(group.value);
                    if ((variable == "$INSUNITS" || variable == "$HANDSEED") &&
                        (i + 1 == range.end ||
                         file.groups[i + 1].code != (variable == "$INSUNITS" ? 70 : 5))) {
                        return malformed(file.lines[i], "missing or invalid header variable value");
                    }
                } else if (variable == "$INSUNITS" && group.code == 70) {
                    const auto units = parseNumber(trimDxf(group.value));
                    if (!units || *units < 0 || *units > core::Document::kMaxUnits) {
                        return malformed(file.lines[i], "INSUNITS must be between 0 and 24");
                    }
                    document.setUnits(static_cast<int>(*units));
                } else if (variable == "$HANDSEED" && group.code == 5) {
                    const auto seed = parseHandle(group.value);
                    if (!seed || *seed == 0 || *seed == kMaximum) {
                        return malformed(file.lines[i],
                                         "HANDSEED is outside the usable handle range");
                    }
                    handles.next = std::max(handles.next, *seed);
                }
                continue;
            }
            const bool entities = range.name == "ENTITIES";
            const int handleCode = !entities && type == "DIMSTYLE" ? 105 : 5;
            if (group.code != handleCode || (!entities && hasHandle)) {
                continue;
            }
            hasHandle = true;
            const auto id = parseHandle(group.value);
            if (!id || *id == 0 || *id >= kMaximum - 1) {
                return malformed(file.lines[i], "handle is outside the usable handle range");
            }
            if (!handles.used.insert(*id).second) {
                return malformed(file.lines[i], "duplicate handle " + group.value);
            }
            handles.next = std::max(handles.next, *id + 1);
            // The LAYER and LTYPE records enter the document with their
            // handle; TableReader reserves those it keeps raw.
            const bool record =
                range.name == "TABLES" && type == table && (type == "LAYER" || type == "LTYPE");
            if (range.name != "ENTITIES" && !record) {
                document.reserveHandle({*id});
            }
        }
    }
    return handles;
}

/// The properties of the entities of the blocks come from the tables, as
/// those of the drawing.
class TableEntityReader final : public BlockEntityReader {
public:
    explicit TableEntityReader(TableReader& tables) : m_tables(tables) {}

    bool properties(core::EntityRecord& record) override {
        return m_tables.entityProperties(record);
    }

private:
    TableReader& m_tables;
};

/// Gives `properties` the values finish() gave the entity `id`, if any.
void resolve(const TableReader& tables, core::EntityId id, core::EntityProperties& properties) {
    if (const core::EntityProperties* resolved = tables.resolvedProperties(id)) {
        properties = *resolved;
    }
}

void resolve(const TableReader& tables, core::EntityRecord& record) {
    resolve(tables, record.id, record.properties);
    if (auto* insert = std::get_if<core::Insert>(&record.entity)) {
        for (auto& attribute : insert->attributes) {
            resolve(tables, attribute.id, attribute.properties);
        }
    }
}

/// An entity of ENTITIES read: handled, or kept as read.
struct ReadEntity {
    std::optional<core::EntityRecord> record;
    core::RawEntity raw;
};

/// The entities of the ENTITIES section between `begin` and `end`; an entity
/// without a handle gets one, with a warning.
Result<std::vector<FileEntity>> fileEntities(const GroupFile& file, std::size_t begin,
                                             std::size_t end, core::Document& document,
                                             std::vector<std::string>& warnings) {
    std::vector<FileEntity> entities;
    std::size_t i = begin;
    while (i < end) {
        const std::size_t start = i;
        if (file.groups[i].code != 0) {
            return malformed(file.lines[i], "expected an entity type (group 0)");
        }
        FileEntity entity{file.groups[i++].value, {}, file.lines[start]};
        if (trimDxf(entity.type).empty()) {
            return malformed(file.lines[start], "expected a nonempty entity type");
        }
        bool handle = false;
        while (i < end && file.groups[i].code != 0) {
            const auto& group = file.groups[i];
            if (group.code == 5) {
                if (handle) {
                    return malformed(file.lines[i], "multiple handles on one entity");
                }
                handle = true;
            }
            entity.groups.push_back(group);
            ++i;
        }
        if (!handle) {
            constexpr auto kLastUsable = std::numeric_limits<std::uint64_t>::max() - 1;
            const std::uint64_t id = document.nextId();
            if (id >= kLastUsable) {
                return malformed(file.lines[start], "no handle available for an entity");
            }
            document.setNextId(id + 1);
            // Appended and rotated to the front: an insert at begin() makes GCC 13
            // warn of a false null dereference at -O3 (-Wnull-dereference).
            entity.groups.push_back({5, handleText(id)});
            std::rotate(entity.groups.begin(), entity.groups.end() - 1, entity.groups.end());
            warnings.push_back("line " + std::to_string(file.lines[start]) + ": entity " +
                               entity.type + " had no handle; assigned " + handleText(id));
        }
        entities.push_back(std::move(entity));
    }
    return entities;
}

Result<ReadResult> readDrawing(const GroupFile& file, std::string_view bytes) {
    ReadResult result;
    result.report.warnings = file.warnings;
    const auto parsed = sections(file, bytes, result.report.warnings);
    if (!parsed.ok()) {
        return parsed.error();
    }
    auto handles = collectHandles(file, parsed.value(), result.document);
    if (!handles.ok()) {
        return handles.error();
    }
    auto& document = result.document;
    auto& warnings = result.report.warnings;
    document.setNextId(handles.value().next);
    core::RawDrawing raw;
    raw.version = file.version;
    raw.codePage = file.codePage;
    TableReader tables{document};
    std::optional<std::size_t> tablesSection;
    std::optional<std::size_t> blocksSection;
    for (const auto& range : parsed.value()) {
        core::RawSection section{range.name, {}};
        if (range.name != "ENTITIES") {
            for (std::size_t i = range.begin; i < range.end; ++i) {
                section.groups.push_back(file.groups[i]);
            }
        }
        if (range.name == "TABLES") {
            const std::span<const std::size_t> lines{file.lines.data() + range.begin,
                                                     range.end - range.begin};
            tables.readTables(section.groups, lines, warnings);
            tablesSection = raw.sections.size();
        } else if (range.name == "HEADER") {
            tables.readHeader(section.groups);
        } else if (range.name == "BLOCKS") {
            blocksSection = raw.sections.size();
        }
        raw.sections.push_back(std::move(section));
    }
    // The definitions before the entities, so that an INSERT knows the
    // blocks there are (docs/tappe/0.8/PT18-blocchi-dxf.md, point 4).
    TableEntityReader entityReader{tables};
    BlocksRead blocks;
    if (blocksSection) {
        core::RawGroups noTables;
        blocks = readBlocks(raw.sections[*blocksSection].groups,
                            tablesSection ? raw.sections[*tablesSection].groups : noTables,
                            raw.version, raw.codePage, document.units(), entityReader);
    }
    EntityReadContext context{raw.version, raw.codePage,    modelSpaceHandle(raw), {},
                              0,           document.units()};
    std::vector<ReadEntity> entities;
    for (const auto& range : parsed.value()) {
        if (range.name != "ENTITIES") {
            continue;
        }
        auto read = fileEntities(file, range.begin, range.end, document, warnings);
        if (!read.ok()) {
            return read.error();
        }
        for (auto& item : groupItems(std::move(read).value())) {
            const auto ids = itemHandles(item);
            // The handles of the ATTRIB and of the SEQEND are spent too.
            for (std::size_t i = 1; i < ids.size(); ++i) {
                if (ids[i] != 0) {
                    document.reserveHandle(core::EntityId{ids[i]});
                }
            }
            ReadEntity entity;
            entity.raw.id = core::EntityId{ids.front()};
            entity.raw.order = ++context.order;
            entity.raw.type = item.front().type;
            context.id = entity.raw.id;
            entity.record = readItem(item, context, blocks.names);
            if (entity.record && takeItemProperties(*entity.record, entityReader)) {
                entity.record->id = entity.raw.id;
                entity.record->order = entity.raw.order;
            } else {
                entity.record.reset();
                ++result.report.unhandled[std::string{trimDxf(entity.raw.type)}];
                entity.raw.groups = rawGroupsOf(item);
            }
            entities.push_back(std::move(entity));
        }
    }
    tables.finish(warnings, handles.value().used);
    for (auto& block : blocks.definitions) {
        for (auto& record : block.entities) {
            resolve(tables, record);
        }
    }
    putBlocks(blocks, document, warnings);
    for (auto& entity : entities) {
        if (entity.record) {
            resolve(tables, *entity.record);
            document.insert(std::move(*entity.record));
        } else {
            document.insertRaw(std::move(entity.raw));
        }
    }
    if (auto deep = depthWarning(document)) {
        warnings.push_back(std::move(*deep));
    }
    document.setRaw(std::move(raw));
    return result;
}

}  // namespace

Result<ReadResult> readDxf(std::string_view bytes) {
    try {
        const auto groups = readGroups(bytes);
        if (!groups.ok()) {
            return groups.error();
        }
        return readDrawing(groups.value(), bytes);
    } catch (const std::exception&) {
        return Error{Error::Kind::TooLarge, 0, "unable to allocate the DXF drawing"};
    }
}

}  // namespace tg::io
