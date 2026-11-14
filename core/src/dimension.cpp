// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/dimension.hpp"

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
[[maybe_unused]] constexpr const char* kNotSupported = "Dimension is not supported yet";
}  // namespace

bool supported(const Dimension& /*dimension*/) {
    // The package of Dimension makes it true (docs/adr/0044, point 1).
    return false;
}

std::vector<GeneratedItem> generateOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                      [[maybe_unused]] const Dimension& dimension) {
    assert(supported(dimension) && kNotSupported);
    return {};
}

BBox extentOf(const Document& document, const EntityRecord& record, const Dimension& dimension) {
    assert(supported(dimension) && kNotSupported);
    return extentOfItems(generateOf(document, record, dimension));
}

double distanceOf(const Document& document, const EntityRecord& record, const Dimension& dimension,
                  const Vec2& point) {
    assert(supported(dimension) && kNotSupported);
    return distanceToDrawnItems(generateOf(document, record, dimension), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Dimension& dimension,
                const BBox& rectangle) {
    assert(supported(dimension) && kNotSupported);
    return drawnItemsInside(generateOf(document, record, dimension), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Dimension& dimension,
                  const BBox& rectangle) {
    assert(supported(dimension) && kNotSupported);
    return drawnItemsIntersect(generateOf(document, record, dimension), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           [[maybe_unused]] const Dimension& dimension, const SnapModes& /*modes*/,
                           std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(dimension) && kNotSupported);
}

TransformOutcome transformedOf(const Document& /*document*/, const Dimension& dimension,
                               const Similarity& /*similarity*/, MirrorText /*mirrorText*/) {
    assert(supported(dimension) && kNotSupported);
    return {Entity{dimension}, {}};
}

std::vector<Vec2> gripPointsOf([[maybe_unused]] const Dimension& dimension) {
    assert(supported(dimension) && kNotSupported);
    return {};
}

GripResult moveGripOf(const Document& /*document*/, const Dimension& dimension,
                      std::size_t /*index*/, const Vec2& /*target*/) {
    assert(supported(dimension) && kNotSupported);
    return {GripOutcome::Invalid, Entity{dimension}, {}};
}

}  // namespace tg::core
