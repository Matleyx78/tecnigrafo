// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/attribute_definition.hpp"

#include "tg/core/document.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/snap_points.hpp"

#include <cassert>
#include <cstddef>
#include <optional>
#include <vector>

namespace tg::core {
namespace {
// The message of the precondition supported() (docs/adr/0044, point 1);
// unused in Release, where assertions are absent.
[[maybe_unused]] constexpr const char* kNotSupported = "AttributeDefinition is not supported yet";
}  // namespace

BBox boundingBox([[maybe_unused]] const AttributeDefinition& definition) {
    assert(supported(definition) && kNotSupported);
    return {};
}

bool supported(const AttributeDefinition& /*definition*/) {
    // The package of AttributeDefinition makes it true (docs/adr/0044, point 1).
    return false;
}

std::vector<GeneratedItem> generateOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                      [[maybe_unused]] const AttributeDefinition& definition) {
    assert(supported(definition) && kNotSupported);
    return {};
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
                               const Similarity& /*similarity*/, MirrorText /*mirrorText*/) {
    assert(supported(definition) && kNotSupported);
    return {Entity{definition}, {}};
}

std::vector<Vec2> gripPointsOf([[maybe_unused]] const AttributeDefinition& definition) {
    assert(supported(definition) && kNotSupported);
    return {};
}

GripResult moveGripOf(const Document& /*document*/, const AttributeDefinition& definition,
                      std::size_t /*index*/, const Vec2& /*target*/) {
    assert(supported(definition) && kNotSupported);
    return {GripOutcome::Invalid, Entity{definition}, {}};
}

}  // namespace tg::core
