// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/core/line.hpp"
#include "tg/io/dxf_groups.hpp"

#include <array>
#include <optional>
#include <utility>

namespace tg::io {

extern const EntityCodec kLineCodec;

namespace {

std::optional<core::EntityRecord> readLine(const core::RawGroups& groups,
                                           EntityReadContext& context) {
    const Extrusion extrusion = entityExtrusion(groups);
    if (extrusion == Extrusion::Other) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    // Start 10/20, end 11/21: each once. A line of length zero is a valid
    // DXF LINE and is not degenerate for docs/adr/0018, point 3.
    std::array<std::optional<double>, 4> values;
    bool marker = false;
    for (const auto& group : parts.geometry) {
        if (group.code == 100 && !marker && trimDxf(group.value) == "AcDbLine") {
            marker = true;
            continue;
        }
        std::size_t index = values.size();
        switch (group.code) {
            case 10:
                index = 0;
                break;
            case 20:
                index = 1;
                break;
            case 11:
                index = 2;
                break;
            case 21:
                index = 3;
                break;
            default:
                parts.extras.specific.push_back(group);
                continue;
        }
        const auto number = parseNumber(trimDxf(group.value));
        if (!number || values.at(index)) {
            return std::nullopt;
        }
        values.at(index) = number;
    }
    for (const auto& value : values) {
        if (!value) {
            return std::nullopt;
        }
    }
    // LINE has its points in world coordinates whatever its extrusion: only
    // the direction is remembered, for the record (docs/adr/0018, point 5).
    parts.extras.flippedExtrusion = extrusion == Extrusion::Down;
    core::Line line{{*values[0], *values[1]}, {*values[2], *values[3]}};
    return core::EntityRecord{context.id, line, std::move(parts.extras), context.order};
}

void writeLine(const core::EntityRecord& record, EntityWriteContext& context,
               core::RawGroups& out) {
    const auto& line = std::get<core::Line>(record.entity);
    const auto& extras = record.extras;
    writeEntityCommon(record, context, out);
    const std::size_t from = out.size();
    // The order of the DXF reference: thickness, start, end, extrusion.
    out.push_back({100, "AcDbLine"});
    constexpr std::array kThickness{39};
    writePlacement(extras, kThickness, out);
    out.push_back({10, formatNumber(line.start.x)});
    out.push_back({20, formatNumber(line.start.y)});
    constexpr std::array kStartZ{30};
    writePlacement(extras, kStartZ, out);
    out.push_back({11, formatNumber(line.end.x)});
    out.push_back({21, formatNumber(line.end.y)});
    constexpr std::array kEndZ{31};
    writePlacement(extras, kEndZ, out);
    // 38 (elevation) has no meaning for LINE, but a group read is written.
    constexpr std::array kExtrusion{210, 220, 230, 38};
    writePlacement(extras, kExtrusion, out);
    writeEntityTail(extras, out);
    restorePlacement(extras, from, out);
}

}  // namespace

const EntityCodec kLineCodec{"LINE", readLine, writeLine};

}  // namespace tg::io
