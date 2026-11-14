// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/mtext.hpp"

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
[[maybe_unused]] constexpr const char* kNotSupported = "MText is not supported yet";
}  // namespace

bool supported(const MText& /*mtext*/) {
    // The package of MText makes it true (docs/adr/0044, point 1).
    return false;
}

std::vector<GeneratedItem> generateOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                      [[maybe_unused]] const MText& mtext) {
    assert(supported(mtext) && kNotSupported);
    return {};
}

BBox extentOf(const Document& document, const EntityRecord& record, const MText& mtext) {
    assert(supported(mtext) && kNotSupported);
    return extentOfItems(generateOf(document, record, mtext));
}

double distanceOf(const Document& document, const EntityRecord& record, const MText& mtext,
                  const Vec2& point) {
    assert(supported(mtext) && kNotSupported);
    return distanceToDrawnItems(generateOf(document, record, mtext), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const MText& mtext,
                const BBox& rectangle) {
    assert(supported(mtext) && kNotSupported);
    return drawnItemsInside(generateOf(document, record, mtext), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const MText& mtext,
                  const BBox& rectangle) {
    assert(supported(mtext) && kNotSupported);
    return drawnItemsIntersect(generateOf(document, record, mtext), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           [[maybe_unused]] const MText& mtext, const SnapModes& /*modes*/,
                           std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(mtext) && kNotSupported);
}

TransformOutcome transformedOf(const Document& /*document*/, const MText& mtext,
                               const Similarity& /*similarity*/, MirrorText /*mirrorText*/) {
    assert(supported(mtext) && kNotSupported);
    return {Entity{mtext}, {}};
}

std::vector<Vec2> gripPointsOf([[maybe_unused]] const MText& mtext) {
    assert(supported(mtext) && kNotSupported);
    return {};
}

GripResult moveGripOf(const Document& /*document*/, const MText& mtext, std::size_t /*index*/,
                      const Vec2& /*target*/) {
    assert(supported(mtext) && kNotSupported);
    return {GripOutcome::Invalid, Entity{mtext}, {}};
}

}  // namespace tg::core
