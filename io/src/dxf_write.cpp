// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_blocks.hpp"
#include "dxf_entities.hpp"
#include "dxf_entity_common.hpp"
#include "dxf_tables.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <exception>
#include <optional>
#include <string_view>
#include <unordered_set>
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
    // The handles of the entities, of the table records and of the
    // retained sections recover the read-time counter without adding groups
    // to HEADER. The records the reader adds to a file that lacks them are
    // counted too: they took their handles when the file was read.
    std::uint64_t next = 1;
    for (const auto& entity : document.rawEntities()) {
        next = std::max(next, entity.id.value + 1);
    }
    // The handled entities too, as the reader counts every handle of ENTITIES:
    // a file whose last handle is a handled entity needs no $HANDSEED.
    for (const auto& record : document.entities()) {
        next = std::max(next, record.id.value + 1);
    }
    for (const auto& layer : document.layers()) {
        next = std::max(next, layer.id.value + 1);
    }
    for (const auto& linetype : document.linetypes()) {
        next = std::max(next, linetype.id.value + 1);
    }
    for (const auto& style : document.textStyles().styles()) {
        next = std::max(next, style.id + 1);
    }
    for (const auto& style : document.dimStyles().styles()) {
        next = std::max(next, style.id + 1);
    }
    // The block definitions, the ATTRIB and the SEQEND of the inserts
    // (docs/tappe/0.8/PT18-blocchi-dxf.md, point 6).
    next = std::max(next, blockHandlesEnd(document));
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

/// The handles of the BLOCK_RECORD of the paper space layouts
/// (*Paper_Space, *Paper_Space0, …), which stay in the raw TABLES section.
std::unordered_set<std::uint64_t> paperSpaceHandles(const core::RawDrawing& raw) {
    std::unordered_set<std::uint64_t> handles;
    const auto paper = [](std::string_view name) {
        constexpr std::string_view kPrefix = "*paper_space";
        return name.size() >= kPrefix.size() &&
               std::ranges::equal(name.substr(0, kPrefix.size()), kPrefix, [](char a, char b) {
                   return std::tolower(static_cast<unsigned char>(a)) == b;
               });
    };
    for (const auto& section : raw.sections) {
        if (section.name != "TABLES") {
            continue;
        }
        bool blockRecord = false;
        bool layout = false;
        std::uint64_t id = 0;
        const auto close = [&] {
            if (blockRecord && layout && id != 0) {
                handles.insert(id);
            }
        };
        for (const auto& group : section.groups) {
            if (group.code == 0) {
                close();
                blockRecord = trimDxf(group.value) == "BLOCK_RECORD";
                layout = false;
                id = 0;
            } else if (group.code == 5) {
                id = parseHandle(group.value).value_or(0);
            } else if (group.code == 2) {
                layout = paper(trimDxf(group.value));
            }
        }
        close();
    }
    return handles;
}

/// The groups of an unhandled entity of the drawing as written: its owner
/// (the first own 330) becomes the model space, as setOwner() does for the
/// entities of the blocks, because an entity copied from a block by EXPLODE
/// still names the BLOCK_RECORD of the definition
/// (docs/tappe/0.8/PT29-explode-blocchi-quote.md, point 4). Without a 330
/// nothing is added. An entity of a paper space layout keeps its owner: the
/// data the model does not handle are kept (docs/adr/0006).
core::RawGroups rawEntityGroups(const core::RawEntity& entity, core::EntityId modelSpace,
                                const std::unordered_set<std::uint64_t>& paperSpace) {
    core::RawGroups groups = entity.groups;
    if (modelSpace.value == 0) {
        return groups;
    }
    const auto at = dxf_groups::firstOwn(groups, dxf_groups::ownGroups(groups), 330);
    if (!at) {
        return groups;
    }
    const std::optional<std::uint64_t> owner = parseHandle(groups[*at].value);
    if (owner != modelSpace.value && !(owner && paperSpace.contains(*owner))) {
        groups[*at].value = handleText(modelSpace.value);
    }
    return groups;
}

std::optional<Error> appendEntities(const core::Document& document, EntityWriteContext& context,
                                    const BlockWritePlan& plan,
                                    const std::unordered_set<std::uint64_t>& paperSpace,
                                    core::RawGroups& out) {
    const auto managed = document.entities();
    const auto raw = document.rawEntities();
    std::size_t m = 0;
    std::size_t r = 0;
    while (m < managed.size() || r < raw.size()) {
        if (r < raw.size() && (m == managed.size() || raw[r].order < managed[m].order)) {
            out.push_back({0, raw[r].type});
            const core::RawGroups groups = rawEntityGroups(raw[r], context.modelSpace, paperSpace);
            out.insert(out.end(), groups.begin(), groups.end());
            ++r;
        } else {
            const auto* codec = findCodec(managed[m].entity);
            if (codec == nullptr) {
                return Error{Error::Kind::Unsupported, 0, "no DXF writer for a managed entity"};
            }
            // The codecs write extras.common as it is: there go the
            // properties of the model (docs/adr/0023, point 2); an INSERT
            // takes the handles of the plan for its ATTRIB and SEQEND.
            const core::EntityRecord record = recordForWriting(managed[m], document, plan);
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
    // Then the handles the blocks, the attributes and the SEQEND lack.
    const BlockWritePlan plan = planBlocks(document, *raw, nextId);
    const std::string codePage = raw->codePage.empty() ? "ANSI_1252" : raw->codePage;
    const bool conversion = source.sections.empty() || target != source.version;
    // An absent DWGCODEPAGE was decoded with the ANSI_1252 fallback.
    const std::array<bool, 4> addMissing{conversion, conversion || codePage != "ANSI_1252",
                                         conversion || nextId > readNextId(document),
                                         conversion || document.units() != 0};
    EntityWriteContext context{target, codePage, modelSpaceHandle(*raw)};
    const std::unordered_set<std::uint64_t> paperSpace = paperSpaceHandles(*raw);
    core::RawGroups out;
    bool hasHeader = false;
    bool hasEntities = false;
    bool hasTables = false;
    bool hasBlocks = false;
    for (const auto& section : raw->sections) {
        hasHeader = hasHeader || section.name == "HEADER";
        hasEntities = hasEntities || section.name == "ENTITIES";
        hasTables = hasTables || section.name == "TABLES";
        hasBlocks = hasBlocks || section.name == "BLOCKS";
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
        auto tables = newTables(document, nextHandle);
        writeBlockRecords(tables, plan);
        if (!tables.empty()) {
            out.push_back({0, "SECTION"});
            out.push_back({2, "TABLES"});
            out.insert(out.end(), tables.begin(), tables.end());
            out.push_back({0, "ENDSEC"});
        }
    };
    // A file without BLOCKS gets one, before ENTITIES, only when the
    // document has block definitions or dimensions.
    const auto appendNewBlocks = [&] {
        hasBlocks = true;
        if (!plan.noBlocks()) {
            out.push_back({0, "SECTION"});
            out.push_back({2, "BLOCKS"});
            const auto blocks = writeBlocks({}, document, plan, target, codePage);
            out.insert(out.end(), blocks.begin(), blocks.end());
            out.push_back({0, "ENDSEC"});
        }
    };
    for (const auto& section : raw->sections) {
        if (!hasTables && section.name != "HEADER" && section.name != "CLASSES") {
            appendNewTables();
        }
        if (!hasBlocks && section.name == "ENTITIES") {
            appendNewBlocks();
        }
        out.push_back({0, "SECTION"});
        out.push_back({2, section.name});
        if (section.name == "ENTITIES") {
            if (auto error = appendEntities(document, context, plan, paperSpace, out)) {
                return *error;
            }
        } else if (section.name == "HEADER") {
            const auto header = headerProperties(headerGroups(section.groups, target, codePage,
                                                              nextId, document.units(), addMissing),
                                                 document, conversion);
            out.insert(out.end(), header.begin(), header.end());
        } else if (section.name == "TABLES") {
            core::RawGroups tables;
            writeTables(section.groups, document, nextHandle, tables);
            writeBlockRecords(tables, plan);
            out.insert(out.end(), tables.begin(), tables.end());
        } else if (section.name == "BLOCKS") {
            const auto blocks = writeBlocks(section.groups, document, plan, target, codePage);
            out.insert(out.end(), blocks.begin(), blocks.end());
        } else {
            out.insert(out.end(), section.groups.begin(), section.groups.end());
        }
        out.push_back({0, "ENDSEC"});
    }
    if (!hasTables) {
        appendNewTables();
    }
    if (!hasBlocks) {
        appendNewBlocks();
    }
    if (!hasEntities && (!document.entities().empty() || !document.rawEntities().empty())) {
        out.push_back({0, "SECTION"});
        out.push_back({2, "ENTITIES"});
        if (auto error = appendEntities(document, context, plan, paperSpace, out)) {
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
