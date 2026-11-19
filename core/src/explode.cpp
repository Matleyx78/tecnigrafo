// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/explode.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/math.hpp"
#include "tg/core/raw_group.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/text_affine.hpp"
#include "tg/core/transform2d.hpp"
#include "tg/core/vec2.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr std::int16_t kGroupDictionary = 102;  // 102 {…} … 102 }
constexpr std::int16_t kFirstCoordinate = 10;   // DXF: 10–18 X, 20–28 Y,
constexpr std::int16_t kLastCoordinate = 37;    // 30–37 Z
constexpr std::int16_t kFirstXdata = 1001;      // XDATA, to the end

std::string quoted(const std::string& name) {
    return "\"" + name + "\"";
}

/// `groups` without the complete 102 {…} groups, as Editor::addCopy() and
/// insertForCopy() leave them out.
RawGroups withoutDictionaries(const RawGroups& groups) {
    RawGroups retained;
    std::size_t depth = 0;
    for (const RawGroup& group : groups) {
        if (group.code == kGroupDictionary && group.value.starts_with('{')) {
            ++depth;
        } else if (depth > 0) {
            if (group.code == kGroupDictionary && group.value == "}") {
                --depth;
            }
        } else {
            retained.push_back(group);
        }
    }
    return retained;
}

/// A record of an exploded entity: the id 0, the order key, properties and
/// extras of `source`, without its complete 102 groups.
EntityRecord explodedRecord(const EntityRecord& source, Entity entity) {
    EntityExtras extras = source.extras;
    extras.beforeEntity = withoutDictionaries(extras.beforeEntity);
    return EntityRecord{EntityId{}, std::move(entity), std::move(extras), source.order,
                        source.properties};
}

/// True when an own group of the unhandled entity (outside the 102 {…}
/// groups, before XDATA) is a coordinate: a DXF group 10 to 37, which the
/// transformation of the insert would have to move. Here core reads the
/// codes of the groups, which it otherwise keeps opaque (raw_group.hpp),
/// because the rule of the copy depends on them (PT29, point 1).
bool hasCoordinates(const RawGroups& groups) {
    std::size_t depth = 0;
    for (const RawGroup& group : groups) {
        if (group.code == kFirstXdata) {
            return false;
        }
        if (group.code == kGroupDictionary) {
            if (group.value.starts_with('{')) {
                ++depth;
            } else if (group.value == "}" && depth > 0) {
                --depth;
            }
            continue;
        }
        if (depth == 0 && group.code >= kFirstCoordinate && group.code <= kLastCoordinate) {
            return true;
        }
    }
    return false;
}

/// The insert with every attribute ready for a new handle, and the
/// graphics of a dimension too: ids 0.
Entity withNewIdentities(Entity entity) {
    if (auto* insert = std::get_if<Insert>(&entity)) {
        return Entity{insertForCopy(*insert)};
    }
    if (auto* dimension = std::get_if<Dimension>(&entity)) {
        for (EntityRecord& graphic : dimension->graphics) {
            graphic.id = EntityId{};
        }
        for (RawEntity& raw : dimension->rawGraphics) {
            raw.id = EntityId{};
        }
    }
    return entity;
}

/// The nested insert under the non-uniform map `map`, or std::nullopt when
/// the linear part of the composed map is not a rotation and a scale along
/// its own axes. T·Tr(p)·A·Tr(−b) = Tr(T(p))·L·A·Tr(−b), with L the linear
/// part of T: the insertion point is mapped and L·A decomposed as
/// R(r)·S(sx, sy), with sy > 0.
std::optional<Insert> composedInsert(const Insert& inner, const Transform2D& map) {
    const SinCos direction = cardinalSinCos(inner.rotation);
    const Vec2 xAxis{direction.cosine * inner.scaleX, direction.sine * inner.scaleX};
    const Vec2 yAxis{-direction.sine * inner.scaleY, direction.cosine * inner.scaleY};
    const Vec2 first = map.applyToVector(xAxis);
    const Vec2 second = map.applyToVector(yAxis);
    if (dot(first, second) != 0.0) {
        return std::nullopt;
    }
    Insert result = inner;
    result.insertion = map.apply(inner.insertion);
    result.rotation = normalizeAngle(atan2(-second.x, second.y));
    result.scaleY = second.length();
    const SinCos turned = cardinalSinCos(result.rotation);
    result.scaleX = dot(first, Vec2{turned.cosine, turned.sine});
    for (Attribute& attribute : result.attributes) {
        attribute.text = transformedText(attribute.text, map);
    }
    return insertForCopy(result);
}

/// Why an entity of the block refuses, if it does.
struct Refusals {
    bool objects{false};  ///< blockHasUnexplodableObjects()
    bool nested{false};   ///< blockHasUnrepresentableNestedBlock()
};

/// Appends the images of `record` under the similarity.
void explodeSimilar(const Document& document, const EntityRecord& record,
                    const Similarity& similarity, Refusals& refusals, Exploded& out) {
    TransformOutcome outcome =
        transformedIn(document, record.entity, similarity, MirrorText::Mirrored);
    if (!outcome.entity) {
        refusals.objects = true;
        return;
    }
    out.entities.push_back(explodedRecord(record, withNewIdentities(std::move(*outcome.entity))));
}

/// Appends the images of `record` under the affine map that is not a
/// similarity.
void explodeAffine(const EntityRecord& record, const Transform2D& map, Refusals& refusals,
                   Exploded& out) {
    std::visit(
        [&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, MText> || std::is_same_v<T, Dimension> ||
                          std::is_same_v<T, Hatch>) {
                refusals.objects = true;
            } else if constexpr (std::is_same_v<T, Insert>) {
                if (std::optional<Insert> inner = composedInsert(value, map)) {
                    out.entities.push_back(explodedRecord(record, Entity{std::move(*inner)}));
                } else {
                    refusals.nested = true;
                }
            } else if constexpr (std::is_same_v<T, AttributeDefinition>) {
                AttributeDefinition definition = value;
                definition.text = transformedText(value.text, map);
                out.entities.push_back(explodedRecord(record, Entity{std::move(definition)}));
            } else {
                for (const GeneratedShape& shape : transformedShape(GeneratedShape{value}, map)) {
                    if (std::optional<Entity> entity = shapeEntity(shape)) {
                        out.entities.push_back(explodedRecord(record, std::move(*entity)));
                    }
                }
            }
        },
        record.entity);
}

}  // namespace

std::string cannotExplodeBlock(const std::string& name) {
    return "Block " + quoted(name) + " cannot be exploded.";
}

std::string blockHasUnexplodableObjects(const std::string& name) {
    return "Block " + quoted(name) + " contains objects Tecnigrafo cannot explode.";
}

std::string blockHasUnrepresentableNestedBlock(const std::string& name) {
    return "Block " + quoted(name) + " contains a nested block that cannot stay a block reference.";
}

Exploded explodeInsert(const Document& document, const Insert& insert) {
    Exploded result;
    const BlockDefinition* definition = document.blocks().find(insert.block);
    if (definition == nullptr || !definition->explodable) {
        result.refusal =
            cannotExplodeBlock(definition != nullptr ? definition->name : insert.block);
        return result;
    }
    const Transform2D map = insertTransform(document, insert);
    const std::optional<Similarity> similarity =
        isSimilarity(insert) ? similarityOf(map) : std::nullopt;
    Refusals refusals;
    for (const EntityRecord& record : definition->entities) {
        if (!isSupported(record.entity)) {
            refusals.objects = true;
        } else if (similarity) {
            explodeSimilar(document, record, *similarity, refusals, result);
        } else {
            explodeAffine(record, map, refusals, result);
        }
    }
    for (const RawEntity& raw : definition->rawEntities) {
        if (hasCoordinates(raw.groups)) {
            refusals.objects = true;
        } else {
            RawEntity copy = raw;
            copy.id = EntityId{};
            result.raw.push_back(std::move(copy));
        }
    }
    if (refusals.objects || refusals.nested) {
        result.entities.clear();
        result.raw.clear();
        result.refusal = refusals.objects ? blockHasUnexplodableObjects(definition->name)
                                          : blockHasUnrepresentableNestedBlock(definition->name);
    }
    return result;
}

Exploded explodeDimension(const Document& /*document*/, const Dimension& dimension) {
    Exploded result;
    if (!dimension.rawGraphics.empty()) {
        result.refusal = std::string{kDimensionHasUnexplodableObjects};
        return result;
    }
    for (const EntityRecord& graphic : dimension.graphics) {
        result.entities.push_back(explodedRecord(graphic, withNewIdentities(graphic.entity)));
    }
    return result;
}

}  // namespace tg::core
