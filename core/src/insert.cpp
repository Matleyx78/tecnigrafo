// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/insert.hpp"

#include "tg/core/arc_geometry.hpp"
#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/math.hpp"
#include "tg/core/snap_points.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string_view>
#include <type_traits>
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

/// A block being visited by appendBlockItems(): its entities from `next`.
struct Visit {
    const BlockDefinition* definition{nullptr};
    std::size_t next{0};
    Transform2D map;         ///< from the block to the drawing
    EntityProperties shown;  ///< the properties of its insert, as shown
    LayerState state;        ///< the layers its insert is shown through
    int depth{1};            ///< 1 for an insert of the drawing
};

/// Appends the items of the block of `insert`, an insert of the drawing
/// shown with `shown` through `state`: depth first, in the order of the
/// entities, without recursion (a nested insert is a visit on the stack).
void appendBlockItems(const Document& document, const Insert& insert, const EntityProperties& shown,
                      const LayerState& state, std::vector<GeneratedItem>& out) {
    std::vector<Visit> stack;
    const auto open = [&](const Insert& inner, const Transform2D& outer,
                          const EntityProperties& properties, const LayerState& layers, int depth) {
        const BlockDefinition* definition = document.blocks().find(inner.block);
        if (definition == nullptr || depth > kMaxInsertDepth) {
            return;
        }
        // Composed first, applied once to each original shape (PT4, P18).
        stack.push_back(
            {definition, 0, outer * mapOf(inner, definition->base), properties, layers, depth});
    };
    open(insert, Transform2D::identity(), shown, state, 1);
    while (!stack.empty()) {
        Visit& visit = stack.back();
        if (visit.next == visit.definition->entities.size()) {
            stack.pop_back();
            continue;
        }
        const EntityRecord& record = visit.definition->entities[visit.next++];
        if (!isSupported(record.entity)) {
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
    return valid(insert.scaleX) && valid(insert.scaleY) && insert.attributes.empty();
}

std::vector<GeneratedItem> generateOf(const Document& document, const EntityRecord& record,
                                      const Insert& insert) {
    assert(supported(insert) && kNotSupported);
    const LayerState state = layerState(document, record.properties);
    std::vector<GeneratedItem> items;
    if (state.frozen) {
        return items;
    }
    appendBlockItems(document, insert, record.properties, state, items);
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
                               const Similarity& similarity, MirrorText /*mirrorText*/) {
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
