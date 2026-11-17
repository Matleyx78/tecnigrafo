// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// ATTDEF and ATTRIB (docs/adr/0045, point 6; docs/tappe/0.8, PT18, point 3):
// the groups of TEXT, then those of AcDbAttributeDefinition or AcDbAttribute.
// The groups of the two subclasses are kept as read in extras.specific, with
// those of the model as witnesses, so that an attribute written again has the
// groups it was read with: AutoCAD leaves out the groups with a default value,
// other programs write them all.

#include "dxf_blocks.hpp"
#include "dxf_entity_common.hpp"
#include "dxf_extrusion.hpp"
#include "tg/core/math.hpp"
#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace tg::io {

using dxf_groups::firstOwn;
using dxf_groups::ownGroups;
using dxf_groups::parseInteger;

void fillGroups(core::RawGroups& groups, std::span<const ModelGroup> values,
                std::span<const int> order) {
    const auto same = [](const ModelGroup& wanted, const std::string& text) {
        switch (wanted.kind) {
            case ModelGroup::Kind::Text:
                return text == wanted.text;
            case ModelGroup::Kind::Number:
                return parseNumber(trimDxf(text)) == wanted.value;
            case ModelGroup::Kind::Integer: {
                const auto integer = parseInteger(text);
                return integer && static_cast<double>(*integer) == wanted.value;
            }
            case ModelGroup::Kind::Angle: {
                const auto degrees = parseNumber(trimDxf(text));
                return degrees && core::degreesToRadians(*degrees) == wanted.value;
            }
        }
        return false;
    };
    core::RawGroups additions;
    const auto own = ownGroups(groups);
    for (const auto& wanted : values) {
        if (const auto at = firstOwn(groups, own, wanted.code)) {
            if (!same(wanted, groups[*at].value)) {
                groups[*at].value = wanted.text;
            }
        } else if (!wanted.omitted) {
            additions.push_back({wanted.code, wanted.text});
        }
    }
    dxf_groups::insertInOrder(groups, additions, order);
}

std::size_t seqendStart(const core::RawGroups& specific) {
    const auto found = std::ranges::find_if(specific, [](const core::RawGroup& group) {
        return group.code == 0 && group.value == kSeqendMarker;
    });
    return static_cast<std::size_t>(found - specific.begin());
}

namespace {

constexpr int kInvisibleBit = 1;
constexpr int kConstantBit = 2;
constexpr int kVerifyBit = 4;
constexpr int kPresetBit = 8;
constexpr int kMirrorBits = 6;
constexpr int kMaxHorizontal = 5;
constexpr int kMaxVertical = 3;

// The order of the groups of TEXT in the DXF reference, and of the groups of
// the two subclasses: a group the model adds goes after the last one before it.
constexpr std::array kTextOrder{10, 20, 40, 1, 50, 41, 51, 7, 71, 72, 11, 21};
constexpr std::array kDefinitionOrder{100, 280, 3, 2, 70, 73, 74};
constexpr std::array kAttributeOrder{100, 280, 2, 70, 73, 74};

constexpr std::string_view kDefinitionMarker = "AcDbAttributeDefinition";
constexpr std::string_view kAttributeMarker = "AcDbAttribute";

/// Where the subclass of the attribute starts in `groups` (the geometry of
/// splitEntityGroups(), without the AcDbText markers): its own marker, or
/// the size of `groups` when there is none (a new attribute).
std::size_t subclassStart(const core::RawGroups& groups, std::string_view marker) {
    const auto own = ownGroups(groups);
    for (std::size_t i = 0; i < groups.size(); ++i) {
        if (own[i] && groups[i].code == 100 && trimDxf(groups[i].value) == marker) {
            return i;
        }
    }
    return groups.size();
}

/// The model groups of an attribute read: the text and the groups of its
/// subclass. std::nullopt when the groups are not those of a handled one.
struct AttributeRead {
    core::Text text;
    std::string tag;
    std::string prompt;
    int flags{0};
    core::EntityExtras extras;
};

/// Counts the subclass markers AcDbText before splitEntityGroups() drops
/// them: an attribute with the second marker of TEXT is not one AutoCAD
/// writes, and the writer could not put it back.
bool oneTextMarker(const core::RawGroups& groups) {
    const auto own = ownGroups(groups);
    int count = 0;
    for (std::size_t i = 0; i < groups.size(); ++i) {
        if (own[i] && groups[i].code == 100 && trimDxf(groups[i].value) == "AcDbText") {
            ++count;
        }
    }
    return count == 1;
}

std::optional<AttributeRead> readAttributeGroups(const core::RawGroups& groups,
                                                 std::string_view marker, bool definition) {
    const Extrusion extrusion = entityExtrusion(groups);
    if (extrusion == Extrusion::Other || !oneTextMarker(groups)) {
        return std::nullopt;
    }
    auto parts = splitEntityGroups(groups);
    AttributeRead read;
    core::Text& text = read.text;
    const std::size_t start = subclassStart(parts.geometry, marker);
    if (start == parts.geometry.size()) {
        return std::nullopt;
    }
    std::vector<int> seen;
    const auto once = [&seen](int code) {
        if (std::ranges::find(seen, code) != seen.end()) {
            return false;
        }
        seen.push_back(code);
        return true;
    };
    bool hasValue = false;
    const auto own = ownGroups(parts.geometry);
    for (std::size_t i = 0; i < parts.geometry.size(); ++i) {
        const auto& group = parts.geometry[i];
        if (!own[i]) {
            continue;
        }
        if (group.code == 101) {
            return std::nullopt;  // multiline: docs/adr/0044, point 4
        }
        const bool subclass = i > start;
        const auto number = parseNumber(trimDxf(group.value));
        const int code = subclass ? group.code + 1000 : group.code;
        switch (code) {
            case 1:
                if (!once(code)) {
                    return std::nullopt;
                }
                text.value = group.value;
                hasValue = true;
                break;
            case 7:
                if (!once(code)) {
                    return std::nullopt;
                }
                text.style = group.value;
                break;
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
            case 1070:
            case 1074:
                if (!number || !once(code)) {
                    return std::nullopt;
                }
                break;
            case 1002:
                if (!once(code)) {
                    return std::nullopt;
                }
                read.tag = group.value;
                break;
            case 1003:
                if (!definition) {
                    break;  // not a group of ATTRIB: kept as read
                }
                if (!once(code)) {
                    return std::nullopt;
                }
                read.prompt = group.value;
                break;
            default:
                break;
        }
        if (!number) {
            continue;
        }
        switch (code) {
            case 10:
                text.position.x = *number;
                break;
            case 20:
                text.position.y = *number;
                break;
            case 11:
                text.alignmentPoint.x = *number;
                break;
            case 21:
                text.alignmentPoint.y = *number;
                break;
            case 40:
                text.height = *number;
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
                break;
            }
            case 72:
                if (*number < 0 || *number > kMaxHorizontal) {
                    return std::nullopt;
                }
                text.horizontal = static_cast<core::TextHorizontal>(static_cast<int>(*number));
                break;
            case 1070:
                read.flags = static_cast<int>(*number);
                break;
            case 1074:
                if (*number < 0 || *number > kMaxVertical) {
                    return std::nullopt;
                }
                text.vertical = static_cast<core::TextVertical>(static_cast<int>(*number));
                break;
            default:
                break;
        }
    }
    for (const int required : {10, 20, 40, 1002}) {
        if (std::ranges::find(seen, required) == seen.end()) {
            return std::nullopt;
        }
    }
    if (!hasValue) {
        text.value.clear();  // an attribute may have no value
    }
    if (extrusion == Extrusion::Down) {
        // As TEXT (dxf_text.cpp): x -> -x turns the baseline direction r into
        // 180 - r and reverses the glyphs (docs/adr/0018, point 5).
        text.position = flipX(text.position);
        text.alignmentPoint = flipX(text.alignmentPoint);
        text.rotation =
            core::degreesToRadians(normalizeDegrees(negate(core::radiansToDegrees(text.rotation))));
        text.mirroredX = !text.mirroredX;
        parts.extras.flippedExtrusion = true;
    }
    // Every group of the two subclasses stays, in its place: the model's are
    // witnesses of their place and spelling (fillGroups()).
    parts.extras.specific = std::move(parts.geometry);
    read.extras = std::move(parts.extras);
    return read;
}

/// The groups of TEXT and of the subclass of an attribute, from the groups
/// kept in `specific` and the values of the model.
void writeAttributeGroups(const core::Text& model, const core::EntityExtras& extras,
                          std::string_view marker, std::vector<ModelGroup> subclassValues,
                          std::span<const int> subclassOrder, core::RawGroups& out) {
    core::Text text = model;
    if (extras.flippedExtrusion) {
        // The conversion of the reading is its own inverse.
        text.position = flipX(text.position);
        text.alignmentPoint = flipX(text.alignmentPoint);
        text.rotation =
            core::degreesToRadians(normalizeDegrees(negate(core::radiansToDegrees(text.rotation))));
        text.mirroredX = !text.mirroredX;
    }
    const auto& specific = extras.specific;
    const std::size_t start = subclassStart(specific, marker);
    core::RawGroups first(specific.begin(), specific.begin() + static_cast<std::ptrdiff_t>(start));
    core::RawGroups second(specific.begin() + static_cast<std::ptrdiff_t>(start), specific.end());
    const bool read = start != specific.size();
    if (!read) {
        // A new attribute: the groups of extras.specific (the flags and the
        // locked position an ATTDEF gives its ATTRIB) after the marker.
        first.clear();
        second = specific;
        // Appended and rotated to the front: an insert at begin() makes GCC 13
        // warn of a false null dereference at -O3 (-Wnull-dereference).
        second.push_back({100, std::string{marker}});
        std::rotate(second.begin(), second.end() - 1, second.end());
    }

    const auto firstOwnNumber = [&first](int code) -> std::optional<long long> {
        const auto at = firstOwn(first, ownGroups(first), code);
        return at ? parseInteger(first[*at].value) : std::nullopt;
    };
    const int mirror = static_cast<int>((firstOwnNumber(71).value_or(0) & ~kMirrorBits)) |
                       (text.mirroredX ? 2 : 0) | (text.mirroredY ? 4 : 0);
    using Kind = ModelGroup::Kind;
    const std::vector<ModelGroup> textValues{
        {10, Kind::Number, formatNumber(text.position.x), text.position.x},
        {20, Kind::Number, formatNumber(text.position.y), text.position.y},
        {40, Kind::Number, formatNumber(text.height), text.height},
        {1, Kind::Text, text.value, 0.0, text.value.empty()},
        {50, Kind::Angle, formatNumber(core::radiansToDegrees(text.rotation)), text.rotation,
         text.rotation == 0.0},
        {41, Kind::Number, formatNumber(text.widthFactor), text.widthFactor,
         text.widthFactor == 1.0},
        {51, Kind::Angle, formatNumber(core::radiansToDegrees(text.obliqueAngle)),
         text.obliqueAngle, text.obliqueAngle == 0.0},
        {7, Kind::Text, text.style, 0.0, text.style == "Standard"},
        {71, Kind::Integer, std::to_string(mirror), static_cast<double>(mirror), mirror == 0},
        {72, Kind::Integer, std::to_string(static_cast<int>(text.horizontal)),
         static_cast<double>(text.horizontal), text.horizontal == core::TextHorizontal::Left},
        {11, Kind::Number, formatNumber(text.alignmentPoint.x), text.alignmentPoint.x,
         text.alignmentPoint == core::Vec2{}},
        {21, Kind::Number, formatNumber(text.alignmentPoint.y), text.alignmentPoint.y,
         text.alignmentPoint == core::Vec2{}},
    };
    fillGroups(first, textValues, kTextOrder);
    // 74 is the vertical alignment of an attribute (73 is its field length).
    subclassValues.push_back({74, Kind::Integer, std::to_string(static_cast<int>(text.vertical)),
                              static_cast<double>(text.vertical),
                              text.vertical == core::TextVertical::Baseline});
    fillGroups(second, subclassValues, subclassOrder);

    // The groups of placement where TEXT has them: thickness after the
    // marker, Z after the points, the extrusion at the end of the text.
    const std::size_t from = out.size();
    out.push_back({100, "AcDbText"});
    constexpr std::array kThickness{39};
    writePlacement(extras, kThickness, out);
    const auto own = ownGroups(first);
    bool z = false;
    bool alignmentZ = false;
    for (std::size_t i = 0; i < first.size(); ++i) {
        out.push_back(first[i]);
        if (own[i] && first[i].code == 20 && !z) {
            z = true;
            constexpr std::array kZ{30};
            writePlacement(extras, kZ, out);
        } else if (own[i] && first[i].code == 21 && !alignmentZ) {
            alignmentZ = true;
            constexpr std::array kAlignmentZ{31};
            writePlacement(extras, kAlignmentZ, out);
        }
    }
    if (!alignmentZ) {
        constexpr std::array kAlignmentZ{31};
        writePlacement(extras, kAlignmentZ, out);
    }
    constexpr std::array kExtrusion{210, 220, 230, 38};
    writePlacement(extras, kExtrusion, out);
    out.insert(out.end(), second.begin(), second.end());
    out.insert(out.end(), extras.extended.begin(), extras.extended.end());
    restorePlacement(extras, from, out);
}

std::optional<core::EntityRecord> readAttdef(const core::RawGroups& groups,
                                             EntityReadContext& context) {
    auto read = readAttributeGroups(groups, kDefinitionMarker, true);
    if (!read) {
        return std::nullopt;
    }
    core::AttributeDefinition definition;
    definition.text = std::move(read->text);
    definition.tag = std::move(read->tag);
    definition.prompt = std::move(read->prompt);
    definition.flags.invisible = (read->flags & kInvisibleBit) != 0;
    definition.flags.constant = (read->flags & kConstantBit) != 0;
    definition.flags.verify = (read->flags & kVerifyBit) != 0;
    definition.flags.preset = (read->flags & kPresetBit) != 0;
    if (!core::supported(definition)) {
        return std::nullopt;
    }
    return core::EntityRecord{context.id, std::move(definition), std::move(read->extras),
                              context.order};
}

void writeAttdef(const core::EntityRecord& record, EntityWriteContext& context,
                 core::RawGroups& out) {
    const auto& definition = std::get<core::AttributeDefinition>(record.entity);
    writeEntityCommon(record, context, out);
    // The bits of the flags the model does not represent stay as read.
    const auto& specific = record.extras.specific;
    const std::size_t start = subclassStart(specific, kDefinitionMarker);
    long long flags = 0;
    if (start != specific.size()) {
        const core::RawGroups second(specific.begin() + static_cast<std::ptrdiff_t>(start),
                                     specific.end());
        if (const auto at = firstOwn(second, ownGroups(second), 70)) {
            flags = parseInteger(second[*at].value).value_or(0);
        }
    }
    flags &= ~(kInvisibleBit | kConstantBit | kVerifyBit | kPresetBit);
    flags |= (definition.flags.invisible ? kInvisibleBit : 0) |
             (definition.flags.constant ? kConstantBit : 0) |
             (definition.flags.verify ? kVerifyBit : 0) |
             (definition.flags.preset ? kPresetBit : 0);
    using Kind = ModelGroup::Kind;
    std::vector<ModelGroup> values{
        {3, Kind::Text, definition.prompt, 0.0, definition.prompt.empty()},
        {2, Kind::Text, definition.tag},
        {70, Kind::Integer, std::to_string(flags), static_cast<double>(flags)},
    };
    writeAttributeGroups(definition.text, record.extras, kDefinitionMarker, std::move(values),
                         kDefinitionOrder, out);
}

}  // namespace

const EntityCodec kAttdefCodec{"ATTDEF", readAttdef, writeAttdef};

std::optional<core::Attribute> readAttribute(const core::RawGroups& groups,
                                             const EntityReadContext& context) {
    // The gate of decodeEntity(), but the owner: an ATTRIB belongs to the
    // INSERT it follows, whatever its 330 says.
    std::size_t depth = 0;
    for (const auto& group : groups) {
        if (group.code == 1001) {
            break;
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
        if (depth == 0 && group.code == 67 && parseNumber(trimDxf(group.value)) != 0.0) {
            return std::nullopt;
        }
    }
    auto read = readAttributeGroups(groups, kAttributeMarker, false);
    if (!read) {
        return std::nullopt;
    }
    core::Attribute attribute;
    attribute.id = context.id;
    attribute.text = std::move(read->text);
    attribute.tag = std::move(read->tag);
    attribute.invisible = (read->flags & kInvisibleBit) != 0;
    attribute.extras = std::move(read->extras);
    if (!core::supported(attribute)) {
        return std::nullopt;
    }
    // Placement groups where the writer would not put them keep their
    // anchors, as decodeEntity() does for the entities.
    auto anchors = readPlacementAnchors(groups);
    if (!anchors.empty() && anchors.size() == attribute.extras.placement.size()) {
        const EntityWriteContext write{context.version, context.codePage, context.modelSpace};
        core::RawGroups usual;
        writeAttribute(attribute, context.modelSpace, write, usual);
        attribute.extras.placementAnchors = std::move(anchors);
        core::RawGroups restored;
        writeAttribute(attribute, context.modelSpace, write, restored);
        if (restored == usual) {
            attribute.extras.placementAnchors.clear();
        }
    }
    return attribute;
}

void writeAttribute(const core::Attribute& attribute, core::EntityId owner,
                    const EntityWriteContext& context, core::RawGroups& out) {
    const core::EntityRecord common{attribute.id, core::Text{}, attribute.extras, 0,
                                    attribute.properties};
    const EntityWriteContext ownerContext{context.version, context.codePage, owner};
    writeEntityCommon(common, ownerContext, out);
    const auto& specific = attribute.extras.specific;
    const std::size_t start = subclassStart(specific, kAttributeMarker);
    const core::RawGroups second(
        specific.begin() + static_cast<std::ptrdiff_t>(std::min(start, specific.size())),
        specific.end());
    // The flags as read, or as an ATTDEF gives them (core::defaultAttributes),
    // with the invisible bit of the model.
    long long flags = 0;
    if (const auto at = firstOwn(second, ownGroups(second), 70)) {
        flags = parseInteger(second[*at].value).value_or(0);
    }
    flags = (flags & ~kInvisibleBit) | (attribute.invisible ? kInvisibleBit : 0);
    using Kind = ModelGroup::Kind;
    std::vector<ModelGroup> values{
        {2, Kind::Text, attribute.tag},
        {70, Kind::Integer, std::to_string(flags), static_cast<double>(flags)},
    };
    writeAttributeGroups(attribute.text, attribute.extras, kAttributeMarker, std::move(values),
                         kAttributeOrder, out);
}

}  // namespace tg::io
