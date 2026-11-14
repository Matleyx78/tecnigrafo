// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/solid.hpp"

#include "tg/core/document.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/snap_points.hpp"

#include <cassert>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

namespace tg::core {
namespace {
// The message of the precondition supported() (docs/adr/0044, point 1);
// unused in Release, where assertions are absent.
[[maybe_unused]] constexpr const char* kNotSupported = "Solid is not supported yet";
}  // namespace

BBox boundingBox([[maybe_unused]] const Solid& solid) {
    assert(supported(solid) && kNotSupported);
    return {};
}

double distanceOf([[maybe_unused]] const Solid& solid, const Vec2& /*point*/) {
    assert(supported(solid) && kNotSupported);
    return std::numeric_limits<double>::infinity();
}

bool isInsideOf([[maybe_unused]] const Solid& solid, const BBox& /*rectangle*/) {
    assert(supported(solid) && kNotSupported);
    return false;
}

bool intersectsOf([[maybe_unused]] const Solid& solid, const BBox& /*rectangle*/) {
    assert(supported(solid) && kNotSupported);
    return false;
}

void appendSnapPointsOf([[maybe_unused]] const Solid& solid, const SnapModes& /*modes*/,
                        std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(solid) && kNotSupported);
}

bool supported(const Solid& /*solid*/) {
    // The package of Solid makes it true (docs/adr/0044, point 1).
    return false;
}

std::vector<GeneratedItem> generateOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                      [[maybe_unused]] const Solid& solid) {
    assert(supported(solid) && kNotSupported);
    return {};
}

BBox extentOf(const Document& document, const EntityRecord& record, const Solid& solid) {
    assert(supported(solid) && kNotSupported);
    return extentOfItems(generateOf(document, record, solid));
}

double distanceOf(const Document& document, const EntityRecord& record, const Solid& solid,
                  const Vec2& point) {
    assert(supported(solid) && kNotSupported);
    return distanceToDrawnItems(generateOf(document, record, solid), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Solid& solid,
                const BBox& rectangle) {
    assert(supported(solid) && kNotSupported);
    return drawnItemsInside(generateOf(document, record, solid), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Solid& solid,
                  const BBox& rectangle) {
    assert(supported(solid) && kNotSupported);
    return drawnItemsIntersect(generateOf(document, record, solid), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           [[maybe_unused]] const Solid& solid, const SnapModes& /*modes*/,
                           std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(solid) && kNotSupported);
}

TransformOutcome transformedOf(const Document& /*document*/, const Solid& solid,
                               const Similarity& /*similarity*/, MirrorText /*mirrorText*/) {
    assert(supported(solid) && kNotSupported);
    return {Entity{solid}, {}};
}

std::vector<Vec2> gripPointsOf([[maybe_unused]] const Solid& solid) {
    assert(supported(solid) && kNotSupported);
    return {};
}

GripResult moveGripOf(const Document& /*document*/, const Solid& solid, std::size_t /*index*/,
                      const Vec2& /*target*/) {
    assert(supported(solid) && kNotSupported);
    return {GripOutcome::Invalid, Entity{solid}, {}};
}

}  // namespace tg::core
