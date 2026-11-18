// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// DIMENSION (docs/adr/0047, points 1 and 6; docs/adr/0044, points 4 and 5;
// docs/tappe/0.8/PT25-quote-dxf.md). The groups of its subclasses are kept as
// read in extras.specific, with those of the model as witnesses
// (fillGroups()), as INSERT does; after them, behind the markers
// kDimensionRecordMarker, kDimensionBlockMarker and kDimensionEndMarker, the
// groups of its anonymous block *D… (dxf_blocks.cpp attaches them).

#include "dxf_blocks.hpp"
#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/math.hpp"
#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace tg::io {

using dxf_groups::firstOwn;
using dxf_groups::ownGroups;
using dxf_groups::parseInteger;

namespace {

// Group 70 of DIMENSION (DXF reference): the type in the 3 low bits, 32 when
// the block is referenced by this dimension only, 128 when the text is where
// the user put it.
constexpr int kTypeMask = 7;
constexpr int kBlockOnlyBit = 32;
constexpr int kUserTextBit = 128;

// The DXF types of the five kinds of the stage (docs/adr/0047, point 1).
constexpr int kLinearType = 0;
constexpr int kAlignedType = 1;
constexpr int kAngularType = 2;
constexpr int kDiameterType = 3;
constexpr int kRadiusType = 4;

constexpr std::string_view kDimensionMarker = "AcDbDimension";

// The order of the groups of DIMENSION in the DXF reference: a group the
// model adds goes after the last one before it. 50 is not here: it is the
// only group of AcDbRotatedDimension, the last subclass, and goes at the end.
constexpr std::array kDimensionOrder{280, 2,  10, 20, 11, 21, 12, 22, 70, 71, 72,
                                     41,  42, 73, 74, 75, 1,  53, 51, 3,  13, 23,
                                     14,  24, 34, 15, 25, 35, 16, 26, 36, 40, 52};

/// The groups the model reads; the others stay in extras.specific only.
constexpr std::array kModelCodes{1,  2,  3,  10, 20, 11, 21, 13, 23, 14,
                                 24, 15, 25, 16, 26, 42, 50, 53, 70};

/// The DXF style name when group 3 is absent (DXF reference).
constexpr std::string_view kAbsentStyle = "Standard";

core::DimensionKind kindOf(int type) {
    switch (type) {
        case kLinearType:
            return core::DimensionKind::Linear;
        case kAlignedType:
            return core::DimensionKind::Aligned;
        case kAngularType:
            return core::DimensionKind::Angular;
        case kDiameterType:
            return core::DimensionKind::Diameter;
        case kRadiusType:
            return core::DimensionKind::Radius;
        default:
            return core::DimensionKind::Other;
    }
}

/// The definition points a kind uses, by the code of their x (docs/adr/0047,
/// point 3; the oracle's rows): written even when the file lacked them.
bool usesPoint(core::DimensionKind kind, int code) {
    switch (kind) {
        case core::DimensionKind::Linear:
        case core::DimensionKind::Aligned:
            return code == 13 || code == 14;
        case core::DimensionKind::Angular:
            return code == 13 || code == 14 || code == 15 || code == 16;
        case core::DimensionKind::Diameter:
        case core::DimensionKind::Radius:
            return code == 15;
        case core::DimensionKind::Other:
            return false;
    }
    return false;
}

core::Vec2* pointOf(core::DimensionDefinition& definition, int code) {
    switch (code) {
        case 10:
        case 20:
            return &definition.linePoint;
        case 13:
        case 23:
            return &definition.point13;
        case 14:
        case 24:
            return &definition.point14;
        case 15:
        case 25:
            return &definition.point15;
        case 16:
        case 26:
            return &definition.point16;
        default:
            return nullptr;
    }
}

/// The groups of the subclasses of a new dimension, in the order AutoCAD
/// writes them; fillGroups() puts the values of the model in.
core::RawGroups templateGroups(const core::Dimension& dimension) {
    // 71 5: the text attached at its middle, as AutoCAD writes it.
    core::RawGroups groups{{100, std::string{kDimensionMarker}},
                           {280, "0"},
                           {2, ""},
                           {10, "0.0"},
                           {20, "0.0"},
                           {30, "0.0"},
                           {11, "0.0"},
                           {21, "0.0"},
                           {31, "0.0"},
                           {70, std::to_string(kBlockOnlyBit)},
                           {71, "5"},
                           {42, "0.0"},
                           {3, ""}};
    const auto point = [&groups](std::int16_t code) {
        constexpr std::int16_t kY = 10;  // the y of a point: its x + 10, the z + 20
        constexpr std::int16_t kZ = 20;
        groups.push_back({code, "0.0"});
        groups.push_back({static_cast<std::int16_t>(code + kY), "0.0"});
        groups.push_back({static_cast<std::int16_t>(code + kZ), "0.0"});
    };
    switch (dimension.definition.kind) {
        case core::DimensionKind::Linear:
            groups.push_back({100, "AcDbAlignedDimension"});
            point(13);
            point(14);
            groups.push_back({100, "AcDbRotatedDimension"});
            break;
        case core::DimensionKind::Aligned:
            groups.push_back({100, "AcDbAlignedDimension"});
            point(13);
            point(14);
            break;
        case core::DimensionKind::Angular:
            groups.push_back({100, "AcDb2LineAngularDimension"});
            point(13);
            point(14);
            point(15);
            point(16);
            break;
        case core::DimensionKind::Diameter:
            groups.push_back({100, "AcDbDiametricDimension"});
            point(15);
            groups.push_back({40, "0.0"});
            break;
        case core::DimensionKind::Radius:
            groups.push_back({100, "AcDbRadialDimension"});
            point(15);
            groups.push_back({40, "0.0"});
            break;
        case core::DimensionKind::Other:
            break;
    }
    return groups;
}

std::optional<core::EntityRecord> readDimension(const core::RawGroups& groups,
                                                EntityReadContext& context) {
    // With extrusion (0,0,-1) the text point (11) and the graphics are in the
    // object system while the other points are in the world one: such a
    // dimension stays unhandled, kept as read (docs/adr/0018, point 3).
    if (entityExtrusion(groups) != Extrusion::Up) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    core::Dimension dimension;
    dimension.presentation.style = std::string{kAbsentStyle};
    std::vector<int> seen;
    long long flags = 0;
    const auto own = ownGroups(parts.geometry);
    for (std::size_t i = 0; i < parts.geometry.size(); ++i) {
        const auto& group = parts.geometry[i];
        const int code = group.code;
        if (!own[i] || std::ranges::find(kModelCodes, code) == kModelCodes.end()) {
            continue;
        }
        if (std::ranges::find(seen, code) != seen.end()) {
            return std::nullopt;  // the model has one value for each
        }
        seen.push_back(code);
        switch (code) {
            case 1:
                dimension.presentation.textOverride = group.value;
                continue;
            case 2:
                if (trimDxf(group.value).empty()) {
                    return std::nullopt;  // no block: nothing to show
                }
                continue;
            case 3:
                dimension.presentation.style = group.value;
                continue;
            case 70: {
                const auto value = parseInteger(group.value);
                if (!value) {
                    return std::nullopt;
                }
                flags = *value;
                continue;
            }
            default:
                break;
        }
        const auto number = parseNumber(trimDxf(group.value));
        if (!number) {
            return std::nullopt;
        }
        if (auto* point = pointOf(dimension.definition, code)) {
            (code < 20 ? point->x : point->y) = *number;
        } else if (code == 11) {
            dimension.presentation.textPoint.x = *number;
        } else if (code == 21) {
            dimension.presentation.textPoint.y = *number;
        } else if (code == 42) {
            dimension.value.measurement = *number;
        } else if (code == 50) {
            dimension.definition.rotation = core::degreesToRadians(*number);
        } else if (code == 53) {
            dimension.presentation.textRotation = core::degreesToRadians(*number);
        }
    }
    if (std::ranges::find(seen, 2) == seen.end()) {
        return std::nullopt;  // a block that is not named is missing
    }
    dimension.definition.dxfType = static_cast<int>(flags & kTypeMask);
    dimension.definition.kind = kindOf(dimension.definition.dxfType);
    dimension.presentation.userTextPosition = (flags & kUserTextBit) != 0;
    parts.extras.specific = std::move(parts.geometry);
    if (!core::supported(dimension)) {
        return std::nullopt;
    }
    return core::EntityRecord{context.id, std::move(dimension), std::move(parts.extras),
                              context.order};
}

/// Removes the first own group `code` of `groups`, if any.
void removeOwn(core::RawGroups& groups, int code) {
    if (const auto at = firstOwn(groups, ownGroups(groups), code)) {
        groups.erase(groups.begin() + static_cast<std::ptrdiff_t>(*at));
    }
}

/// The groups of the subclasses of `record` with the values of the model.
core::RawGroups subclassGroups(const core::EntityRecord& record) {
    const auto& dimension = std::get<core::Dimension>(record.entity);
    const auto& definition = dimension.definition;
    const auto& presentation = dimension.presentation;
    const auto& specific = record.extras.specific;
    core::RawGroups groups(specific.begin(), specific.begin() + static_cast<std::ptrdiff_t>(
                                                                    dimensionBlockStart(specific)));
    if (groups.empty()) {
        groups = templateGroups(dimension);
    }
    auto own = ownGroups(groups);
    bool marker = false;
    for (std::size_t i = 0; i < groups.size() && !marker; ++i) {
        marker = own[i] && groups[i].code == 100;
    }
    if (!marker) {
        // Appended and rotated to the front: an insert at begin() makes GCC 13
        // warn of a false null dereference at -O3 (-Wnull-dereference).
        groups.push_back({100, std::string{kDimensionMarker}});
        std::rotate(groups.begin(), groups.end() - 1, groups.end());
    }
    if (!presentation.textRotation) {
        removeOwn(groups, 53);  // automatic: AutoCAD writes no 53
    }
    own = ownGroups(groups);
    const bool hasRotation = firstOwn(groups, own, 50).has_value();
    const long long read = [&] {
        const auto at = firstOwn(groups, own, 70);
        return at ? parseInteger(groups[*at].value).value_or(0) : 0;
    }();
    const long long flags = (read & ~static_cast<long long>(kTypeMask | kUserTextBit)) |
                            (definition.dxfType & kTypeMask) |
                            (presentation.userTextPosition ? kUserTextBit : 0);
    using Kind = ModelGroup::Kind;
    std::vector<ModelGroup> values{
        {10, Kind::Number, formatNumber(definition.linePoint.x), definition.linePoint.x,
         definition.linePoint.x == 0.0},
        {20, Kind::Number, formatNumber(definition.linePoint.y), definition.linePoint.y,
         definition.linePoint.y == 0.0},
        {11, Kind::Number, formatNumber(presentation.textPoint.x), presentation.textPoint.x,
         presentation.textPoint.x == 0.0},
        {21, Kind::Number, formatNumber(presentation.textPoint.y), presentation.textPoint.y,
         presentation.textPoint.y == 0.0},
        {70, Kind::Integer, std::to_string(flags), static_cast<double>(flags), flags == 0},
        {42, Kind::Number, formatNumber(dimension.value.measurement), dimension.value.measurement,
         dimension.value.measurement == 0.0},
        {1, Kind::Text, presentation.textOverride, 0.0, presentation.textOverride.empty()},
        {3, Kind::Text, presentation.style, 0.0, presentation.style == kAbsentStyle},
    };
    if (presentation.textRotation) {
        values.push_back({53, Kind::Angle,
                          formatNumber(core::radiansToDegrees(*presentation.textRotation)),
                          *presentation.textRotation});
    }
    // A point is added when its kind uses it and it is not the value an
    // absent group means; one the file has is always written.
    for (const auto& [x, point] :
         std::array<std::pair<int, core::Vec2>, 4>{{{13, definition.point13},
                                                    {14, definition.point14},
                                                    {15, definition.point15},
                                                    {16, definition.point16}}}) {
        const bool uses = usesPoint(definition.kind, x);
        values.push_back({static_cast<std::int16_t>(x), Kind::Number, formatNumber(point.x),
                          point.x, !uses || point.x == 0.0});
        values.push_back({static_cast<std::int16_t>(x + 10), Kind::Number, formatNumber(point.y),
                          point.y, !uses || point.y == 0.0});
    }
    fillGroups(groups, values, kDimensionOrder);
    const bool linear = definition.kind == core::DimensionKind::Linear;
    const std::vector<ModelGroup> rotation{
        {50, Kind::Angle, formatNumber(core::radiansToDegrees(definition.rotation)),
         definition.rotation, true}};
    if (hasRotation || (linear && definition.rotation != 0.0)) {
        if (!hasRotation) {
            groups.push_back({50, rotation.front().text});
        }
        fillGroups(groups, rotation, kDimensionOrder);
    }
    return groups;
}

void writeDimension(const core::EntityRecord& record, EntityWriteContext& context,
                    core::RawGroups& out) {
    const auto& extras = record.extras;
    writeEntityCommon(record, context, out);
    const std::size_t from = out.size();
    const auto groups = subclassGroups(record);
    const auto own = ownGroups(groups);
    // Each z after its y, the extrusion before the style, as AutoCAD has them;
    // other positions come back by the anchors (restorePlacement()).
    constexpr std::array kZ0{30};
    constexpr std::array kZ1{31};
    constexpr std::array kZ2{32};
    constexpr std::array kZ3{33};
    constexpr std::array kExtrusion{39, 210, 220, 230, 38};
    std::vector<int> done;
    const auto once = [&done](int code) {
        if (std::ranges::find(done, code) != done.end()) {
            return false;
        }
        done.push_back(code);
        return true;
    };
    for (std::size_t i = 0; i < groups.size(); ++i) {
        const int code = own[i] ? groups[i].code : 0;
        if (code == 3 && once(3)) {
            writePlacement(extras, kExtrusion, out);
        }
        out.push_back(groups[i]);
        if (code == 20 && once(20)) {
            writePlacement(extras, kZ0, out);
        } else if (code == 21 && once(21)) {
            writePlacement(extras, kZ1, out);
        } else if (code == 22 && once(22)) {
            writePlacement(extras, kZ2, out);
        } else if (code == 23 && once(23)) {
            writePlacement(extras, kZ3, out);
        }
    }
    if (std::ranges::find(done, 3) == done.end()) {
        writePlacement(extras, kExtrusion, out);
    }
    out.insert(out.end(), extras.extended.begin(), extras.extended.end());
    restorePlacement(extras, from, out);
}

}  // namespace

const EntityCodec kDimensionCodec{"DIMENSION", readDimension, writeDimension};

std::size_t dimensionBlockStart(const core::RawGroups& specific) {
    const auto found =
        std::ranges::find_if(specific, [](const core::RawGroup& group) { return group.code == 0; });
    return static_cast<std::size_t>(found - specific.begin());
}

std::string dimensionBlockName(const core::RawGroups& specific) {
    const core::RawGroups groups(
        specific.begin(),
        specific.begin() + static_cast<std::ptrdiff_t>(dimensionBlockStart(specific)));
    const auto at = firstOwn(groups, ownGroups(groups), 2);
    return at ? groups[*at].value : std::string{};
}

core::RawGroups namedDimensionGroups(const core::Dimension& dimension,
                                     const core::RawGroups& specific, const std::string& name) {
    core::RawGroups groups(specific.begin(), specific.begin() + static_cast<std::ptrdiff_t>(
                                                                    dimensionBlockStart(specific)));
    if (groups.empty()) {
        groups = templateGroups(dimension);
    }
    const std::array values{ModelGroup{2, ModelGroup::Kind::Text, name}};
    fillGroups(groups, values, kDimensionOrder);
    return groups;
}

}  // namespace tg::io
