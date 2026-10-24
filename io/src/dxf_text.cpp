// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/core/math.hpp"
#include "tg/io/dxf_groups.hpp"

#include <array>
#include <utility>

namespace tg::io {
namespace {
std::optional<core::EntityRecord> readText(const core::RawGroups& groups,
                                           EntityReadContext& context) {
    const Extrusion extrusion = entityExtrusion(groups);
    if (extrusion == Extrusion::Other) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    core::Text text;
    bool hasX = false;
    bool hasY = false;
    bool hasHeight = false;
    bool hasValue = false;
    for (const auto& group : parts.geometry) {
        if (group.code == 1) {
            text.value = group.value;
            hasValue = true;
            continue;
        }
        if (group.code == 7) {
            text.style = group.value;
            continue;
        }
        const auto number = parseNumber(trimDxf(group.value));
        switch (group.code) {
            case 10:
            case 20:
            case 11:
            case 21:
            case 40:
            case 41:
            case 50:
            case 51:
            case 71:
            case 72:
            case 73:
                if (!number) {
                    return std::nullopt;
                }
                break;
            default:
                parts.extras.specific.push_back(group);
                continue;
        }
        switch (group.code) {
            case 10:
                text.position.x = *number;
                hasX = true;
                break;
            case 20:
                text.position.y = *number;
                hasY = true;
                break;
            case 11:
                text.alignmentPoint.x = *number;
                break;
            case 21:
                text.alignmentPoint.y = *number;
                break;
            case 40:
                text.height = *number;
                hasHeight = true;
                break;
            case 41:
                text.widthFactor = *number;
                break;
            case 50:
                text.rotation = core::degreesToRadians(*number);
                break;
            case 51:
                text.obliqueAngle = core::degreesToRadians(*number);
                break;
            case 71: {
                const int flags = static_cast<int>(*number);
                text.mirroredX = (flags & 2) != 0;
                text.mirroredY = (flags & 4) != 0;
                if ((flags & ~6) != 0) {
                    parts.extras.specific.push_back(group);
                }
                break;
            }
            case 72:
                if (*number < 0 || *number > 5) {
                    return std::nullopt;
                }
                text.horizontal = static_cast<core::TextHorizontal>(static_cast<int>(*number));
                break;
            case 73:
                if (*number < 0 || *number > 3) {
                    return std::nullopt;
                }
                text.vertical = static_cast<core::TextVertical>(static_cast<int>(*number));
                break;
            default:
                break;
        }
    }
    if (!hasX || !hasY || !hasHeight || !hasValue || text.height <= 0 || text.widthFactor <= 0) {
        return std::nullopt;
    }
    if (extrusion == Extrusion::Down) {
        // x -> -x turns the baseline direction r into 180 - r and reverses
        // the glyphs: the same text is rotated by -r and mirrored once more
        // along its baseline (docs/adr/0018, point 5).
        text.position = flipX(text.position);
        text.alignmentPoint = flipX(text.alignmentPoint);
        text.rotation =
            core::degreesToRadians(normalizeDegrees(negate(core::radiansToDegrees(text.rotation))));
        text.mirroredX = !text.mirroredX;
        parts.extras.flippedExtrusion = true;
    }
    return core::EntityRecord{context.id, std::move(text), std::move(parts.extras), context.order};
}

void writeText(const core::EntityRecord& record, EntityWriteContext& context,
               core::RawGroups& out) {
    auto text = std::get<core::Text>(record.entity);
    if (record.extras.flippedExtrusion) {
        // The conversion of the reading is its own inverse.
        text.position = flipX(text.position);
        text.alignmentPoint = flipX(text.alignmentPoint);
        text.rotation =
            core::degreesToRadians(normalizeDegrees(negate(core::radiansToDegrees(text.rotation))));
        text.mirroredX = !text.mirroredX;
    }
    writeEntityCommon(record, context, out);
    const std::size_t from = out.size();
    out.push_back({100, "AcDbText"});
    constexpr std::array kThickness{39};
    writePlacement(record.extras, kThickness, out);
    out.push_back({10, formatNumber(text.position.x)});
    out.push_back({20, formatNumber(text.position.y)});
    constexpr std::array kZ{30};
    writePlacement(record.extras, kZ, out);
    out.push_back({40, formatNumber(text.height)});
    out.push_back({1, text.value});
    out.push_back({50, formatNumber(core::radiansToDegrees(text.rotation))});
    out.push_back({41, formatNumber(text.widthFactor)});
    out.push_back({51, formatNumber(core::radiansToDegrees(text.obliqueAngle))});
    out.push_back({7, text.style});
    const int flags = (preservedFlags(record.extras, 71) & ~6) | (text.mirroredX ? 2 : 0) |
                      (text.mirroredY ? 4 : 0);
    out.push_back({71, std::to_string(flags)});
    out.push_back({72, std::to_string(static_cast<int>(text.horizontal))});
    out.push_back({11, formatNumber(text.alignmentPoint.x)});
    out.push_back({21, formatNumber(text.alignmentPoint.y)});
    constexpr std::array kAlignmentZ{31};
    writePlacement(record.extras, kAlignmentZ, out);
    // 38 has no meaning for TEXT, but a group read is written.
    constexpr std::array kExtrusion{210, 220, 230, 38};
    writePlacement(record.extras, kExtrusion, out);
    out.push_back({100, "AcDbText"});
    out.push_back({73, std::to_string(static_cast<int>(text.vertical))});
    writeEntityTail(record.extras, out, 71);
    restorePlacement(record.extras, from, out);
}
}  // namespace

const EntityCodec kTextCodec{"TEXT", readText, writeText};
}  // namespace tg::io
