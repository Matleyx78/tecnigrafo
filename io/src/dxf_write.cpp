// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entities.hpp"
#include "tg/io/dxf_groups.hpp"
#include "tg/io/io.hpp"

#include <algorithm>
#include <array>
#include <exception>
#include <utility>

namespace tg::io {
namespace {

core::RawGroups headerGroups(const core::RawGroups& original, std::string_view version,
                             std::string_view codePage, std::uint64_t nextId, int units) {
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
        const auto* const where = std::ranges::find(kNames, trimDxf(group.value));
        if (group.code != 9 || where == kNames.end()) {
            out.push_back(group);
            continue;
        }
        const auto index = static_cast<std::size_t>(where - kNames.begin());
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
        if (!found.at(i)) {
            out.push_back({9, std::string{kNames.at(i)}});
            out.push_back(values.at(i));
        }
    }
    return out;
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
            out.push_back({0, std::string{codec->type}});
            codec->write(managed[m], context, out);
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
        auto built = makeSkeleton(nextId);
        if (!built.ok()) {
            return built.error();
        }
        skeleton = std::move(built).value();
        raw = &skeleton.raw;
        nextId = skeleton.nextId;
    }
    const std::string codePage = raw->codePage.empty() ? "ANSI_1252" : raw->codePage;
    EntityWriteContext context{target, codePage, modelSpaceHandle(*raw)};
    core::RawGroups out;
    bool hasHeader = false;
    bool hasEntities = false;
    for (const auto& section : raw->sections) {
        hasHeader = hasHeader || section.name == "HEADER";
        hasEntities = hasEntities || section.name == "ENTITIES";
    }
    if (!hasHeader) {
        out = {{0, "SECTION"}, {2, "HEADER"}};
        const auto header = headerGroups({}, target, codePage, nextId, document.units());
        out.insert(out.end(), header.begin(), header.end());
        out.push_back({0, "ENDSEC"});
    }
    for (const auto& section : raw->sections) {
        out.push_back({0, "SECTION"});
        out.push_back({2, section.name});
        if (section.name == "ENTITIES") {
            if (auto error = appendEntities(document, context, out)) {
                return *error;
            }
        } else if (section.name == "HEADER") {
            const auto header =
                headerGroups(section.groups, target, codePage, nextId, document.units());
            out.insert(out.end(), header.begin(), header.end());
        } else {
            out.insert(out.end(), section.groups.begin(), section.groups.end());
        }
        out.push_back({0, "ENDSEC"});
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
