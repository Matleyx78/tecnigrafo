// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_entity_common.hpp"

#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <utility>

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

namespace dxf_groups {

std::vector<bool> ownGroups(const core::RawGroups& groups) {
    std::vector<bool> own(groups.size(), false);
    std::size_t depth = 0;
    for (std::size_t i = 0; i < groups.size(); ++i) {
        const auto& group = groups[i];
        if (group.code == 1001) {
            break;  // XDATA belongs to applications, up to the end
        }
        if (group.code == 102) {
            const auto value = trimDxf(group.value);
            if (value.starts_with('{')) {
                ++depth;
            } else if (value == "}" && depth != 0) {
                --depth;
            }
            continue;
        }
        own[i] = depth == 0;
    }
    return own;
}

std::optional<std::size_t> firstOwn(const core::RawGroups& groups, const std::vector<bool>& own,
                                    int code) {
    for (std::size_t i = 0; i < groups.size(); ++i) {
        if (own[i] && groups[i].code == code) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<long long> parseInteger(std::string_view text) {
    // Integers of 16 and 32 bits: well inside the range of a double.
    constexpr double kLimit = 9.0e15;
    const auto number = parseNumber(trimDxf(text));
    if (!number || *number != std::floor(*number) || std::abs(*number) > kLimit) {
        return std::nullopt;
    }
    return static_cast<long long>(*number);
}

int rgbValue(core::Rgb rgb) {
    constexpr int kRedShift = 16;
    constexpr int kGreenShift = 8;
    return (rgb.r << kRedShift) | (rgb.g << kGreenShift) | rgb.b;
}

core::Rgb rgbOf(long long value) {
    constexpr long long kByte = 0xFF;
    constexpr int kRedShift = 16;
    constexpr int kGreenShift = 8;
    return {static_cast<std::uint8_t>((value >> kRedShift) & kByte),
            static_cast<std::uint8_t>((value >> kGreenShift) & kByte),
            static_cast<std::uint8_t>(value & kByte)};
}

void insertInOrder(core::RawGroups& groups, const core::RawGroups& additions,
                   std::span<const int> order) {
    const auto rank = [order](int code) {
        const auto found = std::ranges::find(order, code);
        return found == order.end() ? -1 : static_cast<int>(found - order.begin());
    };
    for (const auto& addition : additions) {
        const int wanted = rank(addition.code);
        const auto own = ownGroups(groups);
        std::optional<std::size_t> after;
        std::optional<std::size_t> marker;
        for (std::size_t i = 0; i < groups.size(); ++i) {
            if (!own[i]) {
                continue;
            }
            const int current = rank(groups[i].code);
            if (current >= 0 && current < wanted) {
                after = i;
            }
            if (groups[i].code == 100) {
                marker = i;
            }
        }
        std::size_t at = 0;
        if (after) {
            at = *after + 1;
        } else if (marker) {
            at = *marker + 1;
        }
        groups.insert(groups.begin() + static_cast<std::ptrdiff_t>(at), addition);
    }
}

}  // namespace dxf_groups

namespace {

// The order of the AcDbEntity groups in the DXF reference; the properties
// of the model go in by it.
constexpr std::array kCommonOrder{67, 410, 8,   6,   347, 62,  370, 48,
                                  60, 92,  310, 420, 430, 440, 390, 284};

constexpr int kColorByLayer = 256;
constexpr int kColorByBlock = 0;
constexpr long long kMaxRgb = 0xFFFFFF;

}  // namespace

std::optional<EntityPropertyGroups> takeEntityProperties(core::EntityExtras& extras, bool keepLayer,
                                                         bool keepLinetype) {
    using dxf_groups::firstOwn;
    using dxf_groups::parseInteger;
    using dxf_groups::rgbOf;
    auto& common = extras.common;
    const auto own = dxf_groups::ownGroups(common);
    EntityPropertyGroups result;
    std::vector<bool> taken(common.size(), false);

    if (const auto at = firstOwn(common, own, 8)) {
        result.layer = common[*at].value;
        taken[*at] = !keepLayer;
    }
    if (const auto at = firstOwn(common, own, 6)) {
        // ByLayer is the default, written anyway: it stays where it is.
        if (!core::sameTableName(trimDxf(common[*at].value), "ByLayer")) {
            result.linetype = common[*at].value;
            taken[*at] = !keepLinetype;
        }
    }
    const auto at420 = firstOwn(common, own, 420);
    const auto at62 = firstOwn(common, own, 62);
    if (at420) {
        // With a 420 the colour is RGB and the 62 is only its nearest index
        // for older programs: it stays as read (docs/adr/0024, point 2).
        const auto value = parseInteger(common[*at420].value);
        if (!value || *value < 0 || *value > kMaxRgb) {
            return std::nullopt;
        }
        result.color = core::Color::fromRgb(rgbOf(*value));
        taken[*at420] = true;
        if (at62 && !parseInteger(common[*at62].value)) {
            return std::nullopt;
        }
    } else if (at62) {
        const auto value = parseInteger(common[*at62].value);
        if (!value || *value < kColorByBlock || *value > kColorByLayer) {
            return std::nullopt;
        }
        if (*value == kColorByBlock) {
            result.color = core::Color::byBlock();
        } else if (*value != kColorByLayer) {
            result.color = core::Color::fromIndex(static_cast<int>(*value));
        }
        taken[*at62] = *value != kColorByLayer;
    }
    if (const auto at = firstOwn(common, own, 370)) {
        const auto value = parseInteger(common[*at].value);
        if (!value) {
            return std::nullopt;
        }
        int lineweight = static_cast<int>(std::clamp<long long>(*value, -4, 1000));
        if (lineweight != core::lineweight::kByLayer && lineweight != core::lineweight::kByBlock &&
            lineweight != core::lineweight::kDefault && !core::isAllowedLineweight(lineweight)) {
            // docs/adr/0025, point 6: the nearest allowed value.
            lineweight =
                lineweight < 0 ? core::lineweight::kByLayer : core::nearestLineweight(lineweight);
            result.lineweightRounded = true;
        }
        result.lineweight = lineweight;
        taken[*at] = lineweight != core::lineweight::kByLayer || result.lineweightRounded;
    }
    if (const auto at = firstOwn(common, own, 48)) {
        const auto value = parseNumber(trimDxf(common[*at].value));
        if (!value || !(*value > 0.0)) {
            return std::nullopt;
        }
        result.linetypeScale = *value;
        taken[*at] = *value != 1.0;
    }
    core::RawGroups kept;
    for (std::size_t i = 0; i < common.size(); ++i) {
        if (!taken[i]) {
            kept.push_back(std::move(common[i]));
        }
    }
    common = std::move(kept);
    return result;
}

core::RawGroups entityCommonGroups(const core::EntityRecord& record,
                                   const core::Document& document) {
    using dxf_groups::firstOwn;
    using dxf_groups::parseInteger;
    using dxf_groups::rgbValue;
    const auto& properties = record.properties;
    const core::Layer* layer = document.layer(properties.layer);
    const core::Linetype* linetype = document.linetype(properties.linetype);
    const bool byLayerLinetype =
        linetype == nullptr || core::sameTableName(linetype->name, "ByLayer");
    const auto kind = properties.color.kind();

    core::RawGroups common = record.extras.common;
    // The groups read with a default value, the 62 read with a 420 and the
    // names spelt otherwise stay while the model keeps that value; the model
    // writes the others. 420 is always the model's.
    const auto own = dxf_groups::ownGroups(common);
    const auto keep = [&](int code, const auto& stillTrue) {
        const auto at = firstOwn(common, own, code);
        return at && stillTrue(common[*at].value);
    };
    const std::string layerName = layer != nullptr ? layer->name : std::string{"0"};
    const bool keep8 =
        keep(8, [&](const std::string& value) { return core::sameTableName(value, layerName); });
    const bool keep6 = keep(6, [&](const std::string& value) {
        return byLayerLinetype ? core::sameTableName(trimDxf(value), "ByLayer")
                               : core::sameTableName(value, linetype->name);
    });
    const bool keep62 = keep(62, [&](const std::string& value) {
        const auto index = parseInteger(value);
        return index && ((kind == core::Color::Kind::ByLayer && *index == kColorByLayer) ||
                         (kind == core::Color::Kind::Rgb && *index > kColorByBlock &&
                          *index < kColorByLayer));
    });
    const bool keep370 = keep(370, [&](const std::string& value) {
        return parseInteger(value) == properties.lineweight;
    });
    const bool keep48 = keep(48, [&](const std::string& value) {
        return parseNumber(trimDxf(value)) == properties.linetypeScale;
    });
    const std::array<std::pair<int, bool>, 6> kept{
        {{8, keep8}, {6, keep6}, {62, keep62}, {370, keep370}, {48, keep48}, {420, false}}};
    for (const auto& [code, stays] : kept) {
        const auto at = firstOwn(common, dxf_groups::ownGroups(common), code);
        if (at && !stays) {
            common.erase(common.begin() + static_cast<std::ptrdiff_t>(*at));
        }
    }

    core::RawGroups additions;
    if (!keep8) {
        additions.push_back({8, layerName});
    }
    if (!byLayerLinetype && !keep6) {
        additions.push_back({6, linetype->name});
    }
    if (kind == core::Color::Kind::Index) {
        additions.push_back({62, std::to_string(properties.color.index())});
    } else if (kind == core::Color::Kind::ByBlock) {
        additions.push_back({62, std::to_string(kColorByBlock)});
    } else if (kind == core::Color::Kind::Rgb && !keep62) {
        additions.push_back({62, std::to_string(core::nearestAci(properties.color.rgb()))});
    }
    if (properties.lineweight != core::lineweight::kByLayer && !keep370) {
        additions.push_back({370, std::to_string(properties.lineweight)});
    }
    if (properties.linetypeScale != 1.0 && !keep48) {
        additions.push_back({48, formatNumber(properties.linetypeScale)});
    }
    if (kind == core::Color::Kind::Rgb) {
        additions.push_back({420, std::to_string(rgbValue(properties.color.rgb()))});
    }
    dxf_groups::insertInOrder(common, additions, kCommonOrder);
    return common;
}

}  // namespace tg::io
