// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/hatch.hpp"

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
[[maybe_unused]] constexpr const char* kNotSupported = "Hatch is not supported yet";
}  // namespace

bool supported(const Hatch& /*hatch*/) {
    // The package of Hatch makes it true (docs/adr/0044, point 1).
    return false;
}

std::vector<GeneratedItem> generateOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                      [[maybe_unused]] const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    return {};
}

BBox extentOf(const Document& document, const EntityRecord& record, const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    return extentOfItems(generateOf(document, record, hatch));
}

double distanceOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                  const Vec2& point) {
    assert(supported(hatch) && kNotSupported);
    return distanceToDrawnItems(generateOf(document, record, hatch), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                const BBox& rectangle) {
    assert(supported(hatch) && kNotSupported);
    return drawnItemsInside(generateOf(document, record, hatch), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                  const BBox& rectangle) {
    assert(supported(hatch) && kNotSupported);
    return drawnItemsIntersect(generateOf(document, record, hatch), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           [[maybe_unused]] const Hatch& hatch, const SnapModes& /*modes*/,
                           std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(hatch) && kNotSupported);
}

TransformOutcome transformedOf(const Document& /*document*/, const Hatch& hatch,
                               const Similarity& /*similarity*/, MirrorText /*mirrorText*/) {
    assert(supported(hatch) && kNotSupported);
    return {Entity{hatch}, {}};
}

std::vector<Vec2> gripPointsOf([[maybe_unused]] const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    return {};
}

GripResult moveGripOf(const Document& /*document*/, const Hatch& hatch, std::size_t /*index*/,
                      const Vec2& /*target*/) {
    assert(supported(hatch) && kNotSupported);
    return {GripOutcome::Invalid, Entity{hatch}, {}};
}

}  // namespace tg::core
