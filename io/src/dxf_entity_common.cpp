// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"

#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace tg::io {

EntityParts splitEntityGroups(const core::RawGroups& groups) {
    EntityParts parts;
    enum class Section : std::uint8_t { Before, Common, Geometry, Extended };
    Section section = Section::Before;
    constexpr std::array kPlacement{30, 31, 38, 39, 210, 220, 230};
    for (const auto& group : groups) {
        if (group.code == 1001) {
            section = Section::Extended;
        }
        if (section == Section::Extended) {
            parts.extras.extended.push_back(group);
        } else if (group.code == 100) {
            section = trimDxf(group.value) == "AcDbEntity" ? Section::Common : Section::Geometry;
            // Subclass markers belong to the codec; unknown markers are retained.
            const auto name = trimDxf(group.value);
            if (section == Section::Geometry && name != "AcDbPolyline" && name != "AcDbText") {
                parts.geometry.push_back(group);
            }
        } else if (section == Section::Before) {
            if (group.code != 5) {
                parts.extras.beforeEntity.push_back(group);
            }
        } else if (section == Section::Common) {
            if (group.code != 67) {
                parts.extras.common.push_back(group);
            }
        } else if (std::ranges::find(kPlacement, group.code) != kPlacement.end()) {
            parts.extras.placement.push_back(group);
        } else {
            parts.geometry.push_back(group);
        }
    }
    return parts;
}

void writeEntityCommon(const core::EntityRecord& record, const EntityWriteContext& context,
                       core::RawGroups& out) {
    out.push_back({5, handleText(record.id.value)});
    const auto& extras = record.extras;
    if (extras.beforeEntity.empty() && context.modelSpace.value != 0) {
        out.push_back({330, handleText(context.modelSpace.value)});
    } else {
        out.insert(out.end(), extras.beforeEntity.begin(), extras.beforeEntity.end());
    }
    out.push_back({100, "AcDbEntity"});
    if (extras.common.empty()) {
        out.push_back({8, "0"});
    } else {
        out.insert(out.end(), extras.common.begin(), extras.common.end());
    }
}

void writePlacement(const core::EntityExtras& extras, std::span<const int> codes,
                    core::RawGroups& out) {
    for (const auto& group : extras.placement) {
        if (std::ranges::find(codes, group.code) != codes.end()) {
            out.push_back(group);
        }
    }
}

void writeEntityTail(const core::EntityExtras& extras, core::RawGroups& out, int flagCode) {
    for (const auto& group : extras.specific) {
        if (group.code != flagCode) {
            out.push_back(group);
        }
    }
    out.insert(out.end(), extras.extended.begin(), extras.extended.end());
}

int preservedFlags(const core::EntityExtras& extras, int code) {
    for (const auto& group : extras.specific) {
        if (group.code == code) {
            return static_cast<int>(parseNumber(trimDxf(group.value)).value_or(0));
        }
    }
    return 0;
}

}  // namespace tg::io
