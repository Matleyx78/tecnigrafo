// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/insert.hpp"

#include "insert_snap.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/attribute_definition.hpp"
#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/math.hpp"
#include "tg/core/raw_group.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/text_affine.hpp"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {
// The message of the precondition supported() (docs/adr/0044, point 1);
// unused in Release, where assertions are absent.
[[maybe_unused]] constexpr const char* kNotSupported = "Insert is not supported";

/// The map of `insert` with the base point `base` (insertTransform()). A
/// quarter turn is exact (cardinalSinCos), as for the other rotations.
Transform2D mapOf(const Insert& insert, const Vec2& base) {
    const SinCos direction = cardinalSinCos(insert.rotation);
    const Transform2D rotation{
        direction.cosine, direction.sine, -direction.sine, direction.cosine, 0.0, 0.0};
    return Transform2D::translation(insert.insertion) * rotation *
           Transform2D::scaling(insert.scaleX, insert.scaleY) * Transform2D::translation(-base);
}

/// The shapes of an entity of a block, in block coordinates, with their
/// properties: `local` carries the properties it is shown with
/// (propertiesInBlock()). Nothing for an insert, which the caller follows,
/// and for an attribute definition, which an insert does not draw.
std::vector<GeneratedItem> localItems(const Document& document, const EntityRecord& local) {
    return std::visit(
        [&](const auto& value) -> std::vector<GeneratedItem> {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, Polyline> || std::is_same_v<T, Line> ||
                          std::is_same_v<T, Circle> || std::is_same_v<T, Arc> ||
                          std::is_same_v<T, Text>) {
                return {GeneratedItem{value, effectiveProperties(document, local)}};
            } else if constexpr (std::is_same_v<T, Insert> ||
                                 std::is_same_v<T, AttributeDefinition>) {
                return {};
            } else {
                return generateOf(document, local, value);
            }
        },
        local.entity);
}

/// Appends the ATTRIBs of `insert`, shown with `shown` through `state`, as
/// texts mapped by `outer`, the map of the block that holds the insert
/// (std::nullopt for an insert of the drawing: they are in its coordinates).
void appendAttributeItems(const Document& document, const Insert& insert,
                          const std::optional<Transform2D>& outer, const EntityProperties& shown,
                          const LayerState& state, std::vector<GeneratedItem>& out) {
    for (const Attribute& attribute : insert.attributes) {
        const EntityProperties properties =
            propertiesInBlock(document, shown, attribute.properties);
        const LayerState layers = layerState(document, properties, state);
        if (layers.frozen) {
            continue;
        }
        EffectiveProperties effective = effectiveProperties(document, properties);
        effective.visible = effective.visible && layers.on && !attribute.invisible;
        if (!outer) {
            out.push_back(GeneratedItem{attribute.text, effective});
            continue;
        }
        for (GeneratedShape& shape : transformedShape(attribute.text, *outer)) {
            out.push_back(GeneratedItem{std::move(shape), effective});
        }
    }
}

/// A block being visited by appendBlockItems(): its entities from `next`.
struct Visit {
    const BlockDefinition* definition{nullptr};
    std::size_t next{0};
    Transform2D map;                   ///< from the block to the drawing
    EntityProperties shown;            ///< the properties of its insert, as shown
    LayerState state;                  ///< the layers its insert is shown through
    int depth{1};                      ///< 1 for an insert of the drawing
    const Insert* insert{nullptr};     ///< its attributes follow its block
    std::optional<Transform2D> outer;  ///< the map of the block that holds it
};

/// Which entities of a block appendBlockItems() visits.
enum class BlockHatches : std::uint8_t {
    Drawn,   ///< all of them: the drawn geometry
    Skipped  ///< all but the hatches: the shapes that give snap points
};

/// Appends the items of the block of `insert`, an insert of the drawing
/// shown with `shown` through `state`, and then of its attributes: depth
/// first, in the order of the entities, without recursion (a nested insert
/// is a visit on the stack, its attributes appended when it is closed).
/// With BlockHatches::Skipped the hatches of the blocks, nested ones
/// included, give nothing.
void appendBlockItems(const Document& document, const Insert& insert, const EntityProperties& shown,
                      const LayerState& state, BlockHatches hatches,
                      std::vector<GeneratedItem>& out) {
    std::vector<Visit> stack;
    const auto open = [&](const Insert& inner, const std::optional<Transform2D>& outer,
                          const EntityProperties& properties, const LayerState& layers, int depth) {
        const BlockDefinition* definition = document.blocks().find(inner.block);
        if (definition == nullptr || depth > kMaxInsertDepth) {
            return;
        }
        // Composed first, applied once to each original shape (PT4, P18).
        const Transform2D map =
            outer.value_or(Transform2D::identity()) * mapOf(inner, definition->base);
        stack.push_back({definition, 0, map, properties, layers, depth, &inner, outer});
    };
    open(insert, std::nullopt, shown, state, 1);
    while (!stack.empty()) {
        Visit& visit = stack.back();
        if (visit.next == visit.definition->entities.size()) {
            const Visit closed = visit;
            stack.pop_back();
            appendAttributeItems(document, *closed.insert, closed.outer, closed.shown, closed.state,
                                 out);
            continue;
        }
        const EntityRecord& record = visit.definition->entities[visit.next++];
        if (!isSupported(record.entity) ||
            (hatches == BlockHatches::Skipped && std::holds_alternative<Hatch>(record.entity))) {
            continue;
        }
        const EntityRecord local{record.id,
                                 record.entity,
                                 {},
                                 record.order,
                                 propertiesInBlock(document, visit.shown, record.properties)};
        const LayerState inner = layerState(document, local.properties, visit.state);
        if (inner.frozen) {
            continue;
        }
        if (const auto* nested = std::get_if<Insert>(&record.entity)) {
            // open() can move the stack: `visit` is not used after it.
            const Transform2D map = visit.map;
            open(*nested, map, local.properties, inner, visit.depth + 1);
            continue;
        }
        for (GeneratedItem& item : localItems(document, local)) {
            item.properties.visible = item.properties.visible && inner.on;
            for (GeneratedShape& shape : transformedShape(item.shape, visit.map)) {
                out.push_back(GeneratedItem{std::move(shape), item.properties});
            }
        }
    }
}

// The group of the flags of an ATTRIB, and that of its locked position
// (Attribute; docs/DECISIONI.md, 2026-11-14, "Revisione delle interfacce
// PT4", P21).
constexpr std::int16_t kFlagsCode = 70;
constexpr std::int16_t kLockedPositionCode = 280;
constexpr int kInvisibleBit = 1;
constexpr int kVerifyBit = 4;
constexpr int kPresetBit = 8;

/// The integer of a group 70 as written (leading and trailing blanks
/// allowed), std::nullopt when it is not one.
std::optional<int> flagsValue(std::string_view text) {
    const auto first = text.find_first_not_of(" \t");
    if (first == std::string_view::npos) {
        return std::nullopt;
    }
    text.remove_prefix(first);
    text.remove_suffix(text.size() - text.find_last_not_of(" \t") - 1);
    int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

/// The constant ATTDEF of a block, placed in the drawing by `map`.
PlacedAttribute placedDefinition(const AttributeDefinition& definition, const Transform2D& map) {
    PlacedAttribute placed{definition.tag, definition.text.value, definition.flags,
                           transformedText(definition.text, map), true};
    placed.text.value = definition.text.value;
    return placed;
}

/// True when a shape of `items` is drawn.
bool anyDrawn(const std::vector<GeneratedItem>& items) {
    return std::ranges::any_of(items,
                               [](const GeneratedItem& item) { return item.properties.visible; });
}

}  // namespace

bool dependsOnBlock(const Document& document, const EntityRecord& record, std::string_view name) {
    const auto* insert = std::get_if<Insert>(&record.entity);
    if (insert == nullptr) {
        return false;
    }
    // The blocks reached from the insert, each visited once.
    std::vector<const BlockDefinition*> visited;
    std::vector<std::string_view> pending{insert->block};
    while (!pending.empty()) {
        const std::string_view block = pending.back();
        pending.pop_back();
        if (sameTableName(block, name)) {
            return true;
        }
        const BlockDefinition* definition = document.blocks().find(block);
        if (definition == nullptr || std::ranges::find(visited, definition) != visited.end()) {
            continue;
        }
        visited.push_back(definition);
        for (const EntityRecord& inner : definition->entities) {
            if (const auto* nested = std::get_if<Insert>(&inner.entity)) {
                pending.push_back(nested->block);
            }
        }
    }
    return false;
}

bool isBlockUsed(const Document& document, std::string_view name) {
    const auto depends = [&](const EntityRecord& record) {
        return dependsOnBlock(document, record, name);
    };
    if (std::ranges::any_of(document.entities(), depends)) {
        return true;
    }
    return std::ranges::any_of(document.blocks().definitions(), [&](const BlockDefinition& block) {
        return !sameTableName(block.name, name) && std::ranges::any_of(block.entities, depends);
    });
}

AttributeFlags attributeFlags(const Attribute& attribute) {
    AttributeFlags flags;
    flags.invisible = attribute.invisible;
    for (const RawGroup& group : attribute.extras.specific) {
        if (group.code != kFlagsCode) {
            continue;
        }
        if (const std::optional<int> value = flagsValue(group.value)) {
            flags.verify = (*value & kVerifyBit) != 0;
            flags.preset = (*value & kPresetBit) != 0;
        }
    }
    return flags;
}

bool supported(const Attribute& attribute) {
    return validAttributeText(attribute.text) && validAttributeTag(attribute.tag);
}

std::vector<Attribute> defaultAttributes(const Document& document, const Insert& insert) {
    std::vector<Attribute> attributes;
    const BlockDefinition* block = document.blocks().find(insert.block);
    if (block == nullptr) {
        return attributes;
    }
    const Transform2D map = insertTransform(document, insert);
    for (const EntityRecord& record : block->entities) {
        const auto* definition = std::get_if<AttributeDefinition>(&record.entity);
        if (definition == nullptr || definition->flags.constant || !supported(*definition)) {
            continue;
        }
        Attribute attribute;
        attribute.text = transformedText(definition->text, map);
        attribute.text.value = definition->text.value;
        attribute.tag = definition->tag;
        attribute.invisible = definition->flags.invisible;
        attribute.properties = record.properties;
        if (definition->flags.verify || definition->flags.preset) {
            const int flags = (definition->flags.invisible ? kInvisibleBit : 0) |
                              (definition->flags.verify ? kVerifyBit : 0) |
                              (definition->flags.preset ? kPresetBit : 0);
            attribute.extras.specific.push_back({kFlagsCode, std::to_string(flags)});
        }
        for (const RawGroup& group : record.extras.specific) {
            if (group.code == kLockedPositionCode) {
                attribute.extras.specific.push_back(group);
            }
        }
        attributes.push_back(std::move(attribute));
    }
    return attributes;
}

std::vector<PlacedAttribute> attributesOf(const Document& document, const Insert& insert) {
    std::vector<PlacedAttribute> placed;
    placed.reserve(insert.attributes.size());
    for (const Attribute& attribute : insert.attributes) {
        placed.push_back({attribute.tag, attribute.text.value, attributeFlags(attribute),
                          attribute.text, false});
    }
    const BlockDefinition* block = document.blocks().find(insert.block);
    if (block == nullptr) {
        return placed;
    }
    const Transform2D map = insertTransform(document, insert);
    for (const EntityRecord& record : block->entities) {
        const auto* definition = std::get_if<AttributeDefinition>(&record.entity);
        if (definition != nullptr && definition->flags.constant && supported(*definition)) {
            placed.push_back(placedDefinition(*definition, map));
        }
    }
    return placed;
}

std::optional<PlacedAttribute> findAttribute(const Document& document, const Insert& insert,
                                             std::string_view tag) {
    std::vector<PlacedAttribute> placed = attributesOf(document, insert);
    const auto found = std::ranges::find_if(placed, [tag](const PlacedAttribute& attribute) {
        return sameTableName(attribute.tag, tag);
    });
    if (found == placed.end()) {
        return std::nullopt;
    }
    return std::move(*found);
}

Insert insertForCopy(const Insert& insert) {
    Insert copy = insert;
    for (Attribute& attribute : copy.attributes) {
        attribute.id = EntityId{};
        RawGroups retained;
        std::size_t depth = 0;
        for (const RawGroup& group : attribute.extras.beforeEntity) {
            if (group.code == 102 && group.value.starts_with('{')) {
                ++depth;
            } else if (depth > 0) {
                if (group.code == 102 && group.value == "}") {
                    --depth;
                }
            } else {
                retained.push_back(group);
            }
        }
        attribute.extras.beforeEntity = std::move(retained);
    }
    return copy;
}

Transform2D insertTransform(const Document& document, const Insert& insert) {
    const BlockDefinition* definition = document.blocks().find(insert.block);
    return mapOf(insert, definition != nullptr ? definition->base : Vec2{});
}

bool isSimilarity(const Insert& insert) {
    return std::abs(insert.scaleX) == std::abs(insert.scaleY);
}

bool supported(const Insert& insert) {
    const auto valid = [](double scale) {
        return std::isfinite(scale) && scale != 0.0;
    };
    return valid(insert.scaleX) && valid(insert.scaleY) &&
           std::ranges::all_of(insert.attributes,
                               [](const Attribute& attribute) { return supported(attribute); });
}

std::vector<GeneratedItem> generateOf(const Document& document, const EntityRecord& record,
                                      const Insert& insert) {
    assert(supported(insert) && kNotSupported);
    const LayerState state = layerState(document, record.properties);
    std::vector<GeneratedItem> items;
    if (state.frozen) {
        return items;
    }
    appendBlockItems(document, insert, record.properties, state, BlockHatches::Drawn, items);
    return items;
}

std::vector<GeneratedItem> snapItemsOf(const Document& document, const EntityRecord& record,
                                       const Insert& insert) {
    assert(supported(insert) && kNotSupported);
    const LayerState state = layerState(document, record.properties);
    std::vector<GeneratedItem> items;
    if (state.frozen) {
        return items;
    }
    appendBlockItems(document, insert, record.properties, state, BlockHatches::Skipped, items);
    // Only the drawn shapes, as generate().
    std::erase_if(items, [](const GeneratedItem& item) { return !item.properties.visible; });
    return items;
}

BBox extentOf(const Document& document, const EntityRecord& record, const Insert& insert) {
    assert(supported(insert) && kNotSupported);
    BBox box = extentOfItems(generateOf(document, record, insert));
    if (!layerState(document, record.properties).frozen) {
        box.expand(insert.insertion);
    }
    return box;
}

double distanceOf(const Document& document, const EntityRecord& record, const Insert& insert,
                  const Vec2& point) {
    assert(supported(insert) && kNotSupported);
    return distanceToDrawnItems(generateOf(document, record, insert), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Insert& insert,
                const BBox& rectangle) {
    assert(supported(insert) && kNotSupported);
    const std::vector<GeneratedItem> items = generateOf(document, record, insert);
    if (anyDrawn(items)) {
        return drawnItemsInside(items, rectangle);
    }
    return isVisible(document, record) && rectangle.contains(insert.insertion);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Insert& insert,
                  const BBox& rectangle) {
    assert(supported(insert) && kNotSupported);
    const std::vector<GeneratedItem> items = generateOf(document, record, insert);
    if (anyDrawn(items)) {
        return drawnItemsIntersect(items, rectangle);
    }
    return isVisible(document, record) && rectangle.contains(insert.insertion);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           const Insert& insert, const SnapModes& modes,
                           std::vector<SnapPoint>& out, const std::optional<Vec2>& /*from*/) {
    assert(supported(insert) && kNotSupported);
    if (modes.insertion) {
        out.push_back({insert.insertion, SnapKind::Insertion});
    }
}

TransformOutcome transformedOf(const Document& /*document*/, const Insert& insert,
                               const Similarity& similarity, MirrorText mirrorText) {
    assert(supported(insert) && kNotSupported);
    Insert result = insert;
    const double factor = similarity.scale();
    result.insertion = similarity.apply(insert.insertion);
    result.scaleY = insert.scaleY * factor;
    if (similarity.reflects()) {
        // p' = t + s·R(θ)·F·p, θ = 2φ for the axis at angle φ (ADR 0031,
        // point 3): F·R(r)·S(sx, sy) = R(−r)·S(sx, −sy), and
        // R(θ − r)·S(sx, −sy) = R(θ − r + π)·S(−sx, sy).
        result.scaleX = -insert.scaleX * factor;
        result.rotation = normalizeAngle(similarity.rotationAngle() - insert.rotation + kPi);
    } else {
        result.scaleX = insert.scaleX * factor;
        result.rotation = normalizeAngle(insert.rotation + similarity.rotationAngle());
    }
    for (Attribute& attribute : result.attributes) {
        attribute.text =
            std::get<Text>(transformed(Entity{attribute.text}, similarity, mirrorText));
    }
    return {Entity{std::move(result)}, {}};
}

std::vector<Vec2> gripPointsOf(const Insert& insert) {
    assert(supported(insert) && kNotSupported);
    return {insert.insertion};
}

GripResult moveGripOf(const Document& /*document*/, const Insert& insert,
                      [[maybe_unused]] std::size_t index, const Vec2& target) {
    assert(supported(insert) && kNotSupported);
    assert(index == 0);
    if (!std::isfinite(target.x) || !std::isfinite(target.y)) {
        return {GripOutcome::Invalid, Entity{insert}, {}};
    }
    Insert moved = insert;
    moved.insertion = target;
    return {GripOutcome::Moved, Entity{std::move(moved)}, {}};
}

}  // namespace tg::core
