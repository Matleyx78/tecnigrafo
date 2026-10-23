// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entities.hpp"
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

Result<std::vector<SectionRange>> sections(const GroupFile& file, std::string_view bytes) {
    const auto& groups = file.groups;
    std::vector<SectionRange> ranges;
    std::unordered_set<std::string> names;
    std::size_t i = 0;
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
        for (std::size_t i = range.begin; i < range.end; ++i) {
            const auto& group = file.groups[i];
            if (group.code == 0) {
                type = trimDxf(group.value);
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
                    continue;
                }
            }
            if (group.code != 5 && (group.code != 105 || type != "DIMSTYLE")) {
                continue;
            }
            const auto id = parseHandle(group.value);
            if (!id || *id == 0 || *id >= kMaximum - 1) {
                return malformed(file.lines[i], "handle is outside the usable handle range");
            }
            if (!handles.used.insert(*id).second) {
                return malformed(file.lines[i], "duplicate handle " + group.value);
            }
            handles.next = std::max(handles.next, *id + 1);
            if (range.name != "ENTITIES") {
                document.reserveHandle({*id});
            }
        }
    }
    return handles;
}

Result<ReadResult> readDrawing(const GroupFile& file, std::string_view bytes) {
    const auto parsed = sections(file, bytes);
    if (!parsed.ok()) {
        return parsed.error();
    }
    ReadResult result;
    result.report.warnings = file.warnings;
    auto handles = collectHandles(file, parsed.value(), result.document);
    if (!handles.ok()) {
        return handles.error();
    }
    result.document.setNextId(handles.value().next);
    core::RawDrawing raw;
    raw.version = file.version;
    raw.codePage = file.codePage;
    for (const auto& range : parsed.value()) {
        core::RawSection section{range.name, {}};
        if (range.name != "ENTITIES") {
            for (std::size_t i = range.begin; i < range.end; ++i) {
                section.groups.push_back(file.groups[i]);
            }
        }
        raw.sections.push_back(std::move(section));
    }
    EntityReadContext context{raw.version, raw.codePage, modelSpaceHandle(raw), {}, 0};
    for (const auto& range : parsed.value()) {
        if (range.name != "ENTITIES") {
            continue;
        }
        std::size_t i = range.begin;
        while (i < range.end) {
            const std::size_t start = i;
            if (file.groups[i].code != 0) {
                return malformed(file.lines[i], "expected an entity type (group 0)");
            }
            core::RawEntity entity;
            entity.type = file.groups[i++].value;
            entity.order = ++context.order;
            if (trimDxf(entity.type).empty()) {
                return malformed(file.lines[start], "expected a nonempty entity type");
            }
            while (i < range.end && file.groups[i].code != 0) {
                const auto& group = file.groups[i];
                if (group.code == 5) {
                    if (entity.id.value != 0) {
                        return malformed(file.lines[i], "multiple handles on one entity");
                    }
                    entity.id.value = *parseHandle(group.value);
                }
                entity.groups.push_back(group);
                ++i;
            }
            if (entity.id.value == 0) {
                constexpr auto kLastUsable = std::numeric_limits<std::uint64_t>::max() - 1;
                if (result.document.nextId() >= kLastUsable) {
                    return malformed(file.lines[start], "no handle available for an entity");
                }
                entity.id.value = result.document.nextId();
                entity.groups.insert(entity.groups.begin(), {5, handleText(entity.id.value)});
                result.report.warnings.push_back(
                    "line " + std::to_string(file.lines[start]) + ": entity " + entity.type +
                    " had no handle; assigned " + handleText(entity.id.value));
            }
            context.id = entity.id;
            auto record = decodeEntity(findCodec(trimDxf(entity.type)), entity.groups, context);
            if (record) {
                record->id = entity.id;
                record->order = entity.order;
                result.document.insert(std::move(*record));
            } else {
                ++result.report.unhandled[std::string{trimDxf(entity.type)}];
                result.document.insertRaw(std::move(entity));
            }
        }
    }
    result.document.setRaw(std::move(raw));
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
