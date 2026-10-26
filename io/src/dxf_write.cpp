// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entities.hpp"
#include "dxf_entity_common.hpp"
#include "dxf_tables.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <utility>

namespace tg::io {
namespace {

core::RawGroups headerGroups(const core::RawGroups& original, std::string_view version,
                             std::string_view codePage, std::uint64_t nextId, int units,
                             const std::array<bool, 4>& addMissing) {
    constexpr std::array<std::string_view, 4> kNames{"$ACADVER", "$DWGCODEPAGE", "$HANDSEED",
                                                     "$INSUNITS"};
    const std::array<core::RawGroup, 4> values{{{1, std::string{version}},
                                                {3, std::string{codePage}},
                                                {5, handleText(nextId)},
                                                {70, std::to_string(units)}}};
    std::array<bool, 4> found{};
    core::RawGroups out;
    for (std::size_t i = 0; i < original.size(); ++i) {
        const auto& group = original[i];
        const auto* const where =
            std::ranges::find(kNames.data(), kNames.data() + kNames.size(), trimDxf(group.value));
        if (group.code != 9 || where == kNames.data() + kNames.size()) {
            out.push_back(group);
            continue;
        }
        const auto index = static_cast<std::size_t>(where - kNames.data());
        found.at(index) = true;
        out.push_back(group);
        // Keep an unchanged spelling (e.g. INSUNITS " 4") byte for byte.
        if (i + 1 < original.size() && original[i + 1].code == values.at(index).code) {
            const auto& old = original[++i];
            const bool same = index == 3 ? parseNumber(trimDxf(old.value)) == units
                                         : trimDxf(old.value) == values.at(index).value;
            out.push_back(same ? old : values.at(index));
        } else {
            out.push_back(values.at(index));
        }
    }
    for (std::size_t i = 0; i < found.size(); ++i) {
        if (!found.at(i) && addMissing.at(i)) {
            out.push_back({9, std::string{kNames.at(i)}});
            out.push_back(values.at(i));
        }
    }
    return out;
}

std::uint64_t readNextId(const core::Document& document) {
    // The handles of the unhandled entities, of the table records and of the
    // retained sections recover the read-time counter without adding groups
    // to HEADER. The records the reader adds to a file that lacks them are
    // counted too: they took their handles when the file was read.
    std::uint64_t next = 1;
    for (const auto& entity : document.rawEntities()) {
        next = std::max(next, entity.id.value + 1);
    }
    for (const auto& layer : document.layers()) {
        next = std::max(next, layer.id.value + 1);
    }
    for (const auto& linetype : document.linetypes()) {
        next = std::max(next, linetype.id.value + 1);
    }
    for (const auto& section : document.raw().sections) {
        if (section.name == "HEADER" || section.name == "ENTITIES") {
            continue;
        }
        std::string_view type;
        bool hasHandle = false;
        for (const auto& group : section.groups) {
            if (group.code == 0) {
                type = trimDxf(group.value);
                hasHandle = false;
            } else if (!hasHandle && group.code == (type == "DIMSTYLE" ? 105 : 5)) {
                hasHandle = true;
                next = std::max(next, parseHandle(group.value).value_or(0) + 1);
            }
        }
    }
    return next;
}

std::optional<Error> appendEntities(const core::Document& document, EntityWriteContext& context,
                                    core::RawGroups& out) {
    const auto managed = document.entities();
    const auto raw = document.rawEntities();
    std::size_t m = 0;
    std::size_t r = 0;
    while (m < managed.size() || r < raw.size()) {
        if (r < raw.size() && (m == managed.size() || raw[r].order < managed[m].order)) {
            out.push_back({0, raw[r].type});
            out.insert(out.end(), raw[r].groups.begin(), raw[r].groups.end());
            ++r;
        } else {
            const auto* codec = findCodec(managed[m].entity);
            if (codec == nullptr) {
                return Error{Error::Kind::Unsupported, 0, "no DXF writer for a managed entity"};
            }
            // The codecs write extras.common as it is: there go the
            // properties of the model (docs/adr/0023, point 2).
            core::EntityRecord record = managed[m];
            record.extras.common = entityCommonGroups(managed[m], document);
            out.push_back({0, std::string{codec->type}});
            codec->write(record, context, out);
            ++m;
        }
    }
    return std::nullopt;
}

Result<std::string> writeDrawing(const core::Document& document, DxfVersion version) {
    const auto target = versionName(version);
    const auto& source = document.raw();
    if (target.empty() || (!source.version.empty() && target < source.version) ||
        (source.sections.empty() && version != DxfVersion::R2018)) {
        return Error{Error::Kind::Unsupported, 0,
                     "cannot write an older or unsupported DXF version"};
    }
    if (target != source.version && !source.version.empty() && source.version < "AC1021" &&
        !source.codePage.empty() && source.codePage != "ANSI_1250" &&
        source.codePage != "ANSI_1252") {
        return Error{Error::Kind::Unsupported, 0,
                     "cannot safely convert code page " + source.codePage};
    }
    DxfSkeleton skeleton;
    const core::RawDrawing* raw = &source;
    std::uint64_t nextId = document.nextId();
    if (source.sections.empty()) {
        // A new drawing: the skeleton has the fixed handles of
        // start_handles.hpp, whatever the number of entities.
        auto built = makeStartSkeleton(document);
        if (!built.ok()) {
            return built.error();
        }
        skeleton = std::move(built).value();
        raw = &skeleton.raw;
        nextId = skeleton.nextId;
    }
    // The LTYPE and LAYER tables the file lacks and the document needs take
    // their handles here, before $HANDSEED is written.
    std::uint64_t nextHandle = nextId;
    nextId += tablesToAdd(*raw, document);
    const std::string codePage = raw->codePage.empty() ? "ANSI_1252" : raw->codePage;
    const bool conversion = source.sections.empty() || target != source.version;
    // An absent DWGCODEPAGE was decoded with the ANSI_1252 fallback.
    const std::array<bool, 4> addMissing{conversion, conversion || codePage != "ANSI_1252",
                                         conversion || nextId > readNextId(document),
                                         conversion || document.units() != 0};
    EntityWriteContext context{target, codePage, modelSpaceHandle(*raw)};
    core::RawGroups out;
    bool hasHeader = false;
    bool hasEntities = false;
    bool hasTables = false;
    for (const auto& section : raw->sections) {
        hasHeader = hasHeader || section.name == "HEADER";
        hasEntities = hasEntities || section.name == "ENTITIES";
        hasTables = hasTables || section.name == "TABLES";
    }
    if (!hasHeader) {
        out = {{0, "SECTION"}, {2, "HEADER"}};
        const auto header = headerProperties(
            headerGroups({}, target, codePage, nextId, document.units(), addMissing), document,
            conversion);
        out.insert(out.end(), header.begin(), header.end());
        out.push_back({0, "ENDSEC"});
    }
    // A file without TABLES gets one, before the sections that follow it,
    // only when the document has layers or linetypes to write.
    const auto appendNewTables = [&] {
        hasTables = true;
        const auto tables = newTables(document, nextHandle);
        if (!tables.empty()) {
            out.push_back({0, "SECTION"});
            out.push_back({2, "TABLES"});
            out.insert(out.end(), tables.begin(), tables.end());
            out.push_back({0, "ENDSEC"});
        }
    };
    for (const auto& section : raw->sections) {
        if (!hasTables && section.name != "HEADER" && section.name != "CLASSES") {
            appendNewTables();
        }
        out.push_back({0, "SECTION"});
        out.push_back({2, section.name});
        if (section.name == "ENTITIES") {
            if (auto error = appendEntities(document, context, out)) {
                return *error;
            }
        } else if (section.name == "HEADER") {
            const auto header = headerProperties(headerGroups(section.groups, target, codePage,
                                                              nextId, document.units(), addMissing),
                                                 document, conversion);
            out.insert(out.end(), header.begin(), header.end());
        } else if (section.name == "TABLES") {
            writeTables(section.groups, document, nextHandle, out);
        } else {
            out.insert(out.end(), section.groups.begin(), section.groups.end());
        }
        out.push_back({0, "ENDSEC"});
    }
    if (!hasTables) {
        appendNewTables();
    }
    if (!hasEntities && (!document.entities().empty() || !document.rawEntities().empty())) {
        out.push_back({0, "SECTION"});
        out.push_back({2, "ENTITIES"});
        if (auto error = appendEntities(document, context, out)) {
            return *error;
        }
        out.push_back({0, "ENDSEC"});
    }
    out.push_back({0, "EOF"});
    for (const auto& group : out) {
        if (group.value.find_first_of("\r\n") != std::string::npos ||
            !detail::isValidUtf8(group.value)) {
            return Error{Error::Kind::Unsupported, 0,
                         "cannot encode a DXF group containing newlines or invalid UTF-8"};
        }
    }
    auto encoding = TextEncoding::Utf8;
    if (target < "AC1021") {
        encoding = codePage == "ANSI_1250" ? TextEncoding::Ansi1250 : TextEncoding::Ansi1252;
    }
    return writeGroups(out, encoding);
}

}  // namespace

Result<std::string> writeDxf(const core::Document& document, DxfVersion version) {
    try {
        return writeDrawing(document, version);
    } catch (const std::exception&) {
        return Error{Error::Kind::TooLarge, 0, "unable to allocate the DXF output"};
    }
}

}  // namespace tg::io
