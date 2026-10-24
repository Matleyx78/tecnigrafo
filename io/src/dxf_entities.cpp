// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entities.hpp"

#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/io/dxf_groups.hpp"

#include <array>
#include <charconv>
#include <cmath>
#include <system_error>
#include <utility>

namespace tg::io {

// Defined in dxf_line.cpp and dxf_circle_arc.cpp.
extern const EntityCodec kLineCodec;
extern const EntityCodec kCircleCodec;
extern const EntityCodec kArcCodec;

const EntityCodec* findCodec(std::string_view type) {
    if (type == "LWPOLYLINE") {
        return &kLwpolylineCodec;
    }
    if (type == "TEXT") {
        return &kTextCodec;
    }
    if (type == "LINE") {
        return &kLineCodec;
    }
    if (type == "CIRCLE") {
        return &kCircleCodec;
    }
    if (type == "ARC") {
        return &kArcCodec;
    }
    return nullptr;
}

const EntityCodec* findCodec(const core::Entity& entity) {
    if (std::holds_alternative<core::Polyline>(entity)) {
        return &kLwpolylineCodec;
    }
    if (std::holds_alternative<core::Text>(entity)) {
        return &kTextCodec;
    }
    if (std::holds_alternative<core::Line>(entity)) {
        return &kLineCodec;
    }
    if (std::holds_alternative<core::Circle>(entity)) {
        return &kCircleCodec;
    }
    if (std::holds_alternative<core::Arc>(entity)) {
        return &kArcCodec;
    }
    return nullptr;
}

std::string_view trimDxf(std::string_view text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

std::optional<std::uint64_t> parseHandle(std::string_view text) {
    text = trimDxf(text);
    std::uint64_t value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, 16);
    if (error != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::string handleText(std::uint64_t value) {
    constexpr std::size_t kHexDigits = 16;
    std::array<char, kHexDigits> buffer{};
    const auto [end, error] =
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, 16);
    (void)error;
    std::string text(buffer.data(), end);
    for (char& ch : text) {
        if (ch >= 'a' && ch <= 'f') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
    }
    return text;
}

std::string_view versionName(DxfVersion version) {
    switch (version) {
        case DxfVersion::R2000:
            return "AC1015";
        case DxfVersion::R2004:
            return "AC1018";
        case DxfVersion::R2007:
            return "AC1021";
        case DxfVersion::R2010:
            return "AC1024";
        case DxfVersion::R2013:
            return "AC1027";
        case DxfVersion::R2018:
            return "AC1032";
    }
    return {};
}

core::EntityId modelSpaceHandle(const core::RawDrawing& raw) {
    for (const auto& section : raw.sections) {
        if (section.name != "TABLES") {
            continue;
        }
        bool blockRecord = false;
        bool model = false;
        core::EntityId id;
        for (const auto& group : section.groups) {
            if (group.code == 0) {
                if (blockRecord && model) {
                    return id;
                }
                blockRecord = trimDxf(group.value) == "BLOCK_RECORD";
                model = false;
                id = {};
            } else if (group.code == 5) {
                id.value = parseHandle(group.value).value_or(0);
            } else if (group.code == 2) {
                model = trimDxf(group.value) == "*Model_Space";
            }
        }
        if (blockRecord && model) {
            return id;
        }
    }
    return {};
}

std::optional<core::EntityRecord> decodeEntity(const EntityCodec* codec,
                                               const core::RawGroups& groups,
                                               EntityReadContext& context) {
    if (codec == nullptr) {
        return std::nullopt;
    }
    // Reactors can also carry 330; only the outer owner places the entity.
    std::size_t depth = 0;
    for (const auto& group : groups) {
        const auto value = trimDxf(group.value);
        if (group.code == 1001) {
            break;  // XDATA belongs to applications, not placement or geometry.
        }
        if (group.code == 102) {
            if (value.starts_with('{')) {
                ++depth;
            } else if (value == "}" && depth != 0) {
                --depth;
            }
            continue;
        }
        if (depth != 0) {
            continue;
        }
        if (group.code == 330 && parseHandle(value) != context.modelSpace.value) {
            return std::nullopt;
        }
        if (group.code == 67 && parseNumber(value) != 0.0) {
            return std::nullopt;
        }
    }
    if (entityExtrusion(groups) == Extrusion::Other) {
        return std::nullopt;
    }
    auto record = codec->read(groups, context);
    if (!record) {
        return record;
    }
    // Placement groups where the codec would not write them keep their
    // anchors; at the usual positions they need none (docs/adr/0018, point 5).
    auto anchors = readPlacementAnchors(groups);
    if (!anchors.empty() && anchors.size() == record->extras.placement.size()) {
        EntityWriteContext write{context.version, context.codePage, context.modelSpace};
        core::RawGroups usual;
        codec->write(*record, write, usual);
        record->extras.placementAnchors = std::move(anchors);
        core::RawGroups restored;
        codec->write(*record, write, restored);
        if (restored == usual) {
            record->extras.placementAnchors.clear();
        }
    }
    return record;
}

}  // namespace tg::io
