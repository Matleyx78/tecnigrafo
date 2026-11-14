// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/insert.hpp"

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
[[maybe_unused]] constexpr const char* kNotSupported = "Insert is not supported yet";
}  // namespace

bool dependsOnBlock(const Document& /*document*/, const EntityRecord& /*record*/,
                    std::string_view /*name*/) {
    return false;
}

bool supported(const Insert& /*insert*/) {
    // The package of Insert makes it true (docs/adr/0044, point 1).
    return false;
}

std::vector<GeneratedItem> generateOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                      [[maybe_unused]] const Insert& insert) {
    assert(supported(insert) && kNotSupported);
    return {};
}

BBox extentOf(const Document& document, const EntityRecord& record, const Insert& insert) {
    assert(supported(insert) && kNotSupported);
    return extentOfItems(generateOf(document, record, insert));
}

double distanceOf(const Document& document, const EntityRecord& record, const Insert& insert,
                  const Vec2& point) {
    assert(supported(insert) && kNotSupported);
    return distanceToDrawnItems(generateOf(document, record, insert), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Insert& insert,
                const BBox& rectangle) {
    assert(supported(insert) && kNotSupported);
    return drawnItemsInside(generateOf(document, record, insert), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Insert& insert,
                  const BBox& rectangle) {
    assert(supported(insert) && kNotSupported);
    return drawnItemsIntersect(generateOf(document, record, insert), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           [[maybe_unused]] const Insert& insert, const SnapModes& /*modes*/,
                           std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(insert) && kNotSupported);
}

TransformOutcome transformedOf(const Document& /*document*/, const Insert& insert,
                               const Similarity& /*similarity*/, MirrorText /*mirrorText*/) {
    assert(supported(insert) && kNotSupported);
    return {Entity{insert}, {}};
}

std::vector<Vec2> gripPointsOf([[maybe_unused]] const Insert& insert) {
    assert(supported(insert) && kNotSupported);
    return {};
}

GripResult moveGripOf(const Document& /*document*/, const Insert& insert, std::size_t /*index*/,
                      const Vec2& /*target*/) {
    assert(supported(insert) && kNotSupported);
    return {GripOutcome::Invalid, Entity{insert}, {}};
}

}  // namespace tg::core
