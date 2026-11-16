// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/core/solid.hpp"
#include "tg/io/dxf_groups.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <utility>

namespace tg::io {

extern const EntityCodec kSolidCodec;

namespace {

std::optional<core::EntityRecord> readSolid(const core::RawGroups& groups,
                                            EntityReadContext& context) {
    const Extrusion extrusion = entityExtrusion(groups);
    if (extrusion == Extrusion::Other) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    std::array<std::optional<double>, 8> values;
    for (const auto& group : parts.geometry) {
        if (group.code == 100 && trimDxf(group.value) == "AcDbTrace") {
            continue;
        }
        std::size_t index = values.size();
        if (group.code >= 10 && group.code <= 13) {
            index = static_cast<std::size_t>(group.code - 10) * 2;
        } else if (group.code >= 20 && group.code <= 23) {
            index = static_cast<std::size_t>(group.code - 20) * 2 + 1;
        } else {
            parts.extras.specific.push_back(group);
            continue;
        }
        const auto number = parseNumber(trimDxf(group.value));
        if (!number || values[index]) {
            return std::nullopt;
        }
        values[index] = *number;
    }
    for (std::size_t i = 0; i < 6; ++i) {
        if (!values[i]) {
            return std::nullopt;
        }
    }
    if (values[6].has_value() != values[7].has_value()) {
        return std::nullopt;
    }
    const bool fourthPresent = values[6].has_value();
    core::Solid solid{
        {core::Vec2{*values[0], *values[1]}, core::Vec2{*values[2], *values[3]},
         core::Vec2{*values[4], *values[5]},
         fourthPresent ? core::Vec2{*values[6], *values[7]} : core::Vec2{*values[4], *values[5]}},
        fourthPresent};
    if (extrusion == Extrusion::Down) {
        for (core::Vec2& vertex : solid.vertices) {
            vertex = flipX(vertex);
        }
        parts.extras.flippedExtrusion = true;
    }
    if (std::ranges::all_of(solid.vertices, [&](const core::Vec2& vertex) {
            return vertex == solid.vertices[0];
        })) {
        return std::nullopt;
    }
    return core::EntityRecord{context.id, solid, std::move(parts.extras), context.order};
}

void writeSolid(const core::EntityRecord& record, EntityWriteContext& context,
                core::RawGroups& out) {
    const auto& solid = std::get<core::Solid>(record.entity);
    const auto& extras = record.extras;
    writeEntityCommon(record, context, out);
    const std::size_t from = out.size();
    out.push_back({100, "AcDbTrace"});
    for (std::size_t i = 0; i < solid.vertices.size(); ++i) {
        if (i == 3 && !solid.fourthVertexPresent) {
            break;
        }
        const core::Vec2 vertex =
            extras.flippedExtrusion ? flipX(solid.vertices[i]) : solid.vertices[i];
        out.push_back({static_cast<std::int16_t>(10 + i), formatNumber(vertex.x)});
        out.push_back({static_cast<std::int16_t>(20 + i), formatNumber(vertex.y)});
        const std::array z{static_cast<int>(30 + i)};
        writePlacement(extras, z, out);
    }
    constexpr std::array kThickness{39};
    writePlacement(extras, kThickness, out);
    constexpr std::array kExtrusion{210, 220, 230, 38};
    writePlacement(extras, kExtrusion, out);
    writeEntityTail(extras, out);
    restorePlacement(extras, from, out);
}

}  // namespace

const EntityCodec kSolidCodec{"SOLID", readSolid, writeSolid};

}  // namespace tg::io
