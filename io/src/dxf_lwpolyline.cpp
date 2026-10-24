// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace tg::io {
namespace {
std::optional<core::EntityRecord> readPolyline(const core::RawGroups& groups,
                                               EntityReadContext& context) {
    const Extrusion extrusion = entityExtrusion(groups);
    if (extrusion == Extrusion::Other) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    std::vector<core::PolylineVertex> vertices;
    std::optional<double> count;
    int flags = 0;
    double width = 0;
    bool hasY = false;
    for (const auto& group : parts.geometry) {
        const auto number = parseNumber(trimDxf(group.value));
        switch (group.code) {
            case 90:
                if (!number || count) {
                    return std::nullopt;
                }
                count = number;
                break;
            case 70:
                if (!number) {
                    return std::nullopt;
                }
                flags = static_cast<int>(*number);
                if ((flags & ~1) != 0) {
                    parts.extras.specific.push_back(group);
                }
                break;
            case 43:
                if (!number) {
                    return std::nullopt;
                }
                width = *number;
                // A constant width of 0 is the default: some writers (ezdxf)
                // still write it, and it is written again as it was.
                if (width == 0.0) {
                    parts.extras.specific.push_back(group);
                }
                break;
            case 10:
                if (!number || (!vertices.empty() && !hasY)) {
                    return std::nullopt;
                }
                vertices.push_back({{*number, 0}});
                hasY = false;
                break;
            case 20:
            case 40:
            case 41:
            case 42:
                if (!number || vertices.empty()) {
                    return std::nullopt;
                }
                if (group.code == 20) {
                    if (hasY) {
                        return std::nullopt;
                    }
                    vertices.back().position.y = *number;
                    hasY = true;
                } else if (group.code == 40) {
                    vertices.back().startWidth = *number;
                } else if (group.code == 41) {
                    vertices.back().endWidth = *number;
                } else {
                    vertices.back().bulge = *number;
                }
                break;
            case 91:
                return std::nullopt;
            default:
                parts.extras.specific.push_back(group);
        }
    }
    if (vertices.empty() || !hasY || !count || *count != static_cast<double>(vertices.size())) {
        return std::nullopt;
    }
    if (extrusion == Extrusion::Down) {
        // In world coordinates x changes sign, and so does the turn of every
        // arc segment: the sign of its bulge (docs/adr/0018, point 5).
        for (auto& vertex : vertices) {
            vertex.position = flipX(vertex.position);
            vertex.bulge = negate(vertex.bulge);
        }
        parts.extras.flippedExtrusion = true;
    }
    core::Polyline polyline{std::move(vertices), (flags & 1) != 0};
    polyline.setConstantWidth(width);
    return core::EntityRecord{context.id, std::move(polyline), std::move(parts.extras),
                              context.order};
}

void writePolyline(const core::EntityRecord& record, EntityWriteContext& context,
                   core::RawGroups& out) {
    const auto& polyline = std::get<core::Polyline>(record.entity);
    const bool flipped = record.extras.flippedExtrusion;
    writeEntityCommon(record, context, out);
    const std::size_t from = out.size();
    out.push_back({100, "AcDbPolyline"});
    out.push_back({90, std::to_string(polyline.vertexCount())});
    const int flags = (preservedFlags(record.extras, 70) & ~1) | (polyline.isClosed() ? 1 : 0);
    out.push_back({70, std::to_string(flags)});
    const auto& specific = record.extras.specific;
    const auto zeroWidth = std::ranges::find(specific, 43, &core::RawGroup::code);
    if (polyline.constantWidth() != 0.0) {
        out.push_back({43, formatNumber(polyline.constantWidth())});
    } else if (zeroWidth != specific.end()) {
        out.push_back(*zeroWidth);
    }
    constexpr std::array kElevation{38, 39};
    writePlacement(record.extras, kElevation, out);
    for (const auto& vertex : polyline.vertices()) {
        // The conversion of the reading is its own inverse.
        out.push_back({10, formatNumber(flipped ? negate(vertex.position.x) : vertex.position.x)});
        out.push_back({20, formatNumber(vertex.position.y)});
        if (vertex.startWidth != 0.0) {
            out.push_back({40, formatNumber(vertex.startWidth)});
        }
        if (vertex.endWidth != 0.0) {
            out.push_back({41, formatNumber(vertex.endWidth)});
        }
        if (vertex.bulge != 0.0) {
            out.push_back({42, formatNumber(flipped ? negate(vertex.bulge) : vertex.bulge)});
        }
    }
    // 30 and 31 have no meaning for LWPOLYLINE, but a group read is written.
    constexpr std::array kExtrusion{210, 220, 230, 30, 31};
    writePlacement(record.extras, kExtrusion, out);
    // The tail of writeEntityTail(), without the flags 70 merged above and
    // the zero width 43 written in its place.
    for (const auto& group : specific) {
        if (group.code != 70 && group.code != 43) {
            out.push_back(group);
        }
    }
    out.insert(out.end(), record.extras.extended.begin(), record.extras.extended.end());
    restorePlacement(record.extras, from, out);
}
}  // namespace

const EntityCodec kLwpolylineCodec{"LWPOLYLINE", readPolyline, writePolyline};
}  // namespace tg::io
