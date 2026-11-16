// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/ellipse.hpp"

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
[[maybe_unused]] constexpr const char* kNotSupported = "Ellipse is not supported yet";
}  // namespace

BBox boundingBox([[maybe_unused]] const Ellipse& ellipse) {
    assert(supported(ellipse) && kNotSupported);
    return {};
}

double distanceOf([[maybe_unused]] const Ellipse& ellipse, const Vec2& /*point*/) {
    assert(supported(ellipse) && kNotSupported);
    return std::numeric_limits<double>::infinity();
}

bool isInsideOf([[maybe_unused]] const Ellipse& ellipse, const BBox& /*rectangle*/) {
    assert(supported(ellipse) && kNotSupported);
    return false;
}

bool intersectsOf([[maybe_unused]] const Ellipse& ellipse, const BBox& /*rectangle*/) {
    assert(supported(ellipse) && kNotSupported);
    return false;
}

void appendSnapPointsOf([[maybe_unused]] const Ellipse& ellipse, const SnapModes& /*modes*/,
                        std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(ellipse) && kNotSupported);
}

bool supported(const Ellipse& /*ellipse*/) {
    // The package of Ellipse makes it true (docs/adr/0044, point 1).
    return false;
}

std::vector<GeneratedItem> generateOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                      [[maybe_unused]] const Ellipse& ellipse) {
    assert(supported(ellipse) && kNotSupported);
    return {};
}

BBox extentOf(const Document& document, const EntityRecord& record, const Ellipse& ellipse) {
    assert(supported(ellipse) && kNotSupported);
    return extentOfItems(generateOf(document, record, ellipse));
}

double distanceOf(const Document& document, const EntityRecord& record, const Ellipse& ellipse,
                  const Vec2& point) {
    assert(supported(ellipse) && kNotSupported);
    return distanceToDrawnItems(generateOf(document, record, ellipse), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Ellipse& ellipse,
                const BBox& rectangle) {
    assert(supported(ellipse) && kNotSupported);
    return drawnItemsInside(generateOf(document, record, ellipse), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Ellipse& ellipse,
                  const BBox& rectangle) {
    assert(supported(ellipse) && kNotSupported);
    return drawnItemsIntersect(generateOf(document, record, ellipse), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           [[maybe_unused]] const Ellipse& ellipse, const SnapModes& /*modes*/,
                           std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(ellipse) && kNotSupported);
}

TransformOutcome transformedOf(const Document& /*document*/, const Ellipse& ellipse,
                               const Similarity& /*similarity*/, MirrorText /*mirrorText*/) {
    assert(supported(ellipse) && kNotSupported);
    return {Entity{ellipse}, {}};
}

std::vector<Vec2> gripPointsOf([[maybe_unused]] const Ellipse& ellipse) {
    assert(supported(ellipse) && kNotSupported);
    return {};
}

GripResult moveGripOf(const Document& /*document*/, const Ellipse& ellipse, std::size_t /*index*/,
                      const Vec2& /*target*/) {
    assert(supported(ellipse) && kNotSupported);
    return {GripOutcome::Invalid, Entity{ellipse}, {}};
}

}  // namespace tg::core
