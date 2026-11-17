// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/solid.hpp"

#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

namespace tg::core {
namespace {

Polyline outline(const Solid& solid) {
    const auto& v = solid.vertices;
    return Polyline{{{v[0]}, {v[1]}, {v[3]}, {v[2]}}, true};
}

GeneratedShape region(const Solid& solid) {
    return FilledRegion{{outline(solid)}, Transform2D{}};
}

}  // namespace

BBox boundingBox(const Solid& solid) {
    return outline(solid).boundingBox();
}

double distanceOf(const Solid& solid, const Vec2& point) {
    return shapeDistance(region(solid), point);
}

bool isInsideOf(const Solid& solid, const BBox& rectangle) {
    return shapeIsInside(region(solid), rectangle);
}

bool intersectsOf(const Solid& solid, const BBox& rectangle) {
    return shapeIntersects(region(solid), rectangle);
}

void appendSnapPointsOf(const Solid& solid, const SnapModes& modes, std::vector<SnapPoint>& out,
                        const std::optional<Vec2>& /*from*/) {
    if (!modes.endpoint) {
        return;
    }
    std::set<std::tuple<double, double>> seen;
    for (const Vec2& vertex : solid.vertices) {
        if (seen.emplace(vertex.x, vertex.y).second) {
            out.push_back({vertex, SnapKind::Endpoint});
        }
    }
}

bool supported(const Solid& /*solid*/) {
    return true;
}

std::vector<GeneratedItem> generateOf(const Document& document, const EntityRecord& record,
                                      const Solid& solid) {
    return {{solid, effectiveProperties(document, record)}};
}

BBox extentOf(const Document& document, const EntityRecord& record, const Solid& solid) {
    return extentOfItems(generateOf(document, record, solid));
}

double distanceOf(const Document& document, const EntityRecord& record, const Solid& solid,
                  const Vec2& point) {
    return distanceToDrawnItems(generateOf(document, record, solid), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Solid& solid,
                const BBox& rectangle) {
    return drawnItemsInside(generateOf(document, record, solid), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Solid& solid,
                  const BBox& rectangle) {
    return drawnItemsIntersect(generateOf(document, record, solid), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           const Solid& /*solid*/, const SnapModes& /*modes*/,
                           std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {}

TransformOutcome transformedOf(const Document& /*document*/, const Solid& solid,
                               const Similarity& similarity, MirrorText /*mirrorText*/) {
    Solid result = solid;
    for (Vec2& vertex : result.vertices) {
        vertex = similarity.apply(vertex);
    }
    return {Entity{result}, {}};
}

std::vector<Vec2> gripPointsOf(const Solid& solid) {
    std::vector<Vec2> result;
    for (const Vec2& vertex : solid.vertices) {
        if (std::ranges::find(result, vertex) == result.end()) {
            result.push_back(vertex);
        }
    }
    return result;
}

GripResult moveGripOf(const Document& /*document*/, const Solid& solid, std::size_t index,
                      const Vec2& target) {
    const std::vector<Vec2> grips = gripPointsOf(solid);
    assert(index < grips.size());
    Solid result = solid;
    const Vec2 selected = grips[index];
    for (Vec2& vertex : result.vertices) {
        if (vertex == selected) {
            vertex = target;
        }
    }
    if (std::ranges::all_of(result.vertices, [&](const Vec2& vertex) {
            return vertex == result.vertices.front();
        })) {
        return {GripOutcome::Invalid, Entity{solid}, {}};
    }
    return {GripOutcome::Moved, Entity{result}, {}};
}

}  // namespace tg::core
