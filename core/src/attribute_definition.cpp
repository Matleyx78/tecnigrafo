// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/attribute_definition.hpp"

#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {
// The message of the precondition supported() (docs/adr/0044, point 1);
// unused in Release, where assertions are absent.
[[maybe_unused]] constexpr const char* kNotSupported = "AttributeDefinition is not supported";

bool finite(const Vec2& point) {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

/// The definition with the geometry of `text`, a moved or transformed
/// shownText(): its own default value back.
AttributeDefinition withGeometry(const AttributeDefinition& definition, const Entity& text) {
    AttributeDefinition result = definition;
    result.text = std::get<Text>(text);
    result.text.value = definition.text.value;
    return result;
}

}  // namespace

Text shownText(const AttributeDefinition& definition) {
    Text text = definition.text;
    text.value = definition.tag;
    return text;
}

bool validAttributeText(const Text& text) {
    return finite(text.position) && finite(text.alignmentPoint) && std::isfinite(text.height) &&
           text.height > 0.0 && std::isfinite(text.widthFactor) && text.widthFactor > 0.0 &&
           std::isfinite(text.rotation) && std::isfinite(text.obliqueAngle);
}

bool validAttributeTag(std::string_view tag) {
    const auto blank = [](char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r';
    };
    return !tag.empty() && std::ranges::none_of(tag, blank);
}

BBox boundingBox(const AttributeDefinition& definition) {
    assert(supported(definition) && kNotSupported);
    return shownText(definition).boundingBox();
}

bool supported(const AttributeDefinition& definition) {
    return validAttributeText(definition.text) && validAttributeTag(definition.tag);
}

std::vector<GeneratedItem> generateOf(const Document& document, const EntityRecord& record,
                                      const AttributeDefinition& definition) {
    assert(supported(definition) && kNotSupported);
    if (layerState(document, record.properties).frozen) {
        return {};
    }
    // Drawn even when invisible and whatever ATTDISP, as in AutoCAD: only the
    // attributes of the inserts follow them (docs/adr/0056, point 1).
    return {GeneratedItem{shownText(definition), effectiveProperties(document, record)}};
}

BBox extentOf(const Document& document, const EntityRecord& record,
              const AttributeDefinition& definition) {
    assert(supported(definition) && kNotSupported);
    return extentOfItems(generateOf(document, record, definition));
}

double distanceOf(const Document& document, const EntityRecord& record,
                  const AttributeDefinition& definition, const Vec2& point) {
    assert(supported(definition) && kNotSupported);
    return distanceToDrawnItems(generateOf(document, record, definition), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record,
                const AttributeDefinition& definition, const BBox& rectangle) {
    assert(supported(definition) && kNotSupported);
    return drawnItemsInside(generateOf(document, record, definition), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record,
                  const AttributeDefinition& definition, const BBox& rectangle) {
    assert(supported(definition) && kNotSupported);
    return drawnItemsIntersect(generateOf(document, record, definition), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           [[maybe_unused]] const AttributeDefinition& definition,
                           const SnapModes& /*modes*/, std::vector<SnapPoint>& /*out*/,
                           const std::optional<Vec2>& /*from*/) {
    assert(supported(definition) && kNotSupported);
}

TransformOutcome transformedOf(const Document& /*document*/, const AttributeDefinition& definition,
                               const Similarity& similarity, MirrorText mirrorText) {
    assert(supported(definition) && kNotSupported);
    const Entity text = transformed(Entity{shownText(definition)}, similarity, mirrorText);
    return {Entity{withGeometry(definition, text)}, {}};
}

std::vector<Vec2> gripPointsOf(const AttributeDefinition& definition) {
    assert(supported(definition) && kNotSupported);
    return gripPoints(Entity{shownText(definition)});
}

GripResult moveGripOf(const Document& /*document*/, const AttributeDefinition& definition,
                      std::size_t index, const Vec2& target) {
    assert(supported(definition) && kNotSupported);
    GripResult moved = moveGrip(Entity{shownText(definition)}, index, target);
    if (moved.outcome != GripOutcome::Moved) {
        return {moved.outcome, Entity{definition}, std::move(moved.refusal)};
    }
    return {GripOutcome::Moved, Entity{withGeometry(definition, moved.entity)}, {}};
}

}  // namespace tg::core
