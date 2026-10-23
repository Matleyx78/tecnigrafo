// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "tg/io/dxf_groups.hpp"

#include <array>
#include <utility>

namespace tg::io {
namespace {
std::optional<core::EntityRecord> readPolyline(const core::RawGroups& groups,
                                               EntityReadContext& context) {
    auto parts = splitEntityGroups(groups);
    for (const auto& group : parts.extras.placement) {
        if (group.code == 230 && parseNumber(trimDxf(group.value)) != 1.0) {
            return std::nullopt;
        }
    }
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
    core::Polyline polyline{std::move(vertices), (flags & 1) != 0};
    polyline.setConstantWidth(width);
    return core::EntityRecord{context.id, std::move(polyline), std::move(parts.extras),
                              context.order};
}

void writePolyline(const core::EntityRecord& record, EntityWriteContext& context,
                   core::RawGroups& out) {
    const auto& polyline = std::get<core::Polyline>(record.entity);
    writeEntityCommon(record, context, out);
    out.push_back({100, "AcDbPolyline"});
    out.push_back({90, std::to_string(polyline.vertexCount())});
    const int flags = (preservedFlags(record.extras, 70) & ~1) | (polyline.isClosed() ? 1 : 0);
    out.push_back({70, std::to_string(flags)});
    if (polyline.constantWidth() != 0.0) {
        out.push_back({43, formatNumber(polyline.constantWidth())});
    }
    constexpr std::array kElevation{38, 39};
    writePlacement(record.extras, kElevation, out);
    for (const auto& vertex : polyline.vertices()) {
        out.push_back({10, formatNumber(vertex.position.x)});
        out.push_back({20, formatNumber(vertex.position.y)});
        if (vertex.startWidth != 0.0) {
            out.push_back({40, formatNumber(vertex.startWidth)});
        }
        if (vertex.endWidth != 0.0) {
            out.push_back({41, formatNumber(vertex.endWidth)});
        }
        if (vertex.bulge != 0.0) {
            out.push_back({42, formatNumber(vertex.bulge)});
        }
    }
    constexpr std::array kExtrusion{210, 220, 230};
    writePlacement(record.extras, kExtrusion, out);
    writeEntityTail(record.extras, out, 70);
}
}  // namespace

const EntityCodec kLwpolylineCodec{"LWPOLYLINE", readPolyline, writePolyline};
}  // namespace tg::io
