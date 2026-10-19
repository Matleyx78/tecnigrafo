// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/document_query.hpp"

#include "tg/core/polyline.hpp"
#include "tg/core/proximity.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <variant>

namespace tg::core {
namespace {

// proximity.cpp accepts contacts within a roundoff allowance relative to the
// coordinates, while the spatial index compares bounding boxes exactly.
// Growing the search area by the same relative allowance keeps such
// borderline entities among the candidates; the exact geometry still decides.
constexpr double kRoundoff = 32.0 * std::numeric_limits<double>::epsilon();

/// `area` grown by the roundoff allowance. An empty area stays empty.
BBox searchArea(const BBox& area) {
    if (area.isEmpty()) {
        return area;
    }
    const double scale = std::max({std::abs(area.min().x), std::abs(area.min().y),
                                   std::abs(area.max().x), std::abs(area.max().y)});
    const double margin = kRoundoff * scale;
    const Vec2 grow{margin, margin};
    return {area.min() - grow, area.max() + grow};
}

/// The square of half side `halfSide` centred on `center`: it contains every
/// point within `halfSide` of `center`.
BBox squareAround(const Vec2& center, double halfSide) {
    const Vec2 half{halfSide, halfSide};
    return {center - half, center + half};
}

std::vector<EntityId> candidates(const Document& document, const BBox& area) {
    std::vector<EntityId> ids;
    document.query(searchArea(area), ids);
    return ids;
}

const Entity& entityOf(const Document& document, EntityId id) {
    const EntityRecord* record = document.find(id);
    assert(record != nullptr && "the spatial index only holds entities of the document");
    return record->entity;
}

/// Rank of a snap kind on equal distance: an endpoint beats a midpoint.
int tieRank(SnapKind kind) {
    return kind == SnapKind::Endpoint ? 0 : 1;
}

/// A candidate answer, ordered by distance, then rank, then highest id.
/// Ties are exact: any difference in distance decides. On a complete tie the
/// candidate found first stays, so within an entity the earliest segment wins.
struct Candidate {
    double distance{std::numeric_limits<double>::infinity()};
    int rank{0};
    std::uint64_t id{0};

    [[nodiscard]] bool beats(const Candidate& other) const {
        if (distance != other.distance) {
            return distance < other.distance;
        }
        if (rank != other.rank) {
            return rank < other.rank;
        }
        return id > other.id;
    }
};

/// The segments that findSnap() considers: a single vertex counts as a
/// zero-length segment.
template <typename Visit>
void forEachSnapSegment(const Polyline& polyline, const Visit& visit) {
    if (polyline.vertexCount() == 1) {
        const Vec2 point = polyline.vertices().front().position;
        visit(PolylineSegment{point, point});
        return;
    }
    for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
        visit(polyline.segment(i));
    }
}

}  // namespace

std::optional<EntityId> pickEntity(const Document& document, const Vec2& point, double tolerance) {
    assert(tolerance >= 0.0);
    std::optional<EntityId> picked;
    Candidate best;
    for (const EntityId id : candidates(document, squareAround(point, tolerance))) {
        const Candidate candidate{distance(entityOf(document, id), point), 0, id.value};
        if (candidate.distance <= tolerance && (!picked || candidate.beats(best))) {
            picked = id;
            best = candidate;
        }
    }
    return picked;
}

std::vector<EntityId> selectInRectangle(const Document& document, const BBox& rectangle,
                                        RectangleMode mode) {
    std::vector<EntityId> selected;
    if (rectangle.isEmpty()) {
        return selected;
    }
    for (const EntityId id : candidates(document, rectangle)) {
        const Entity& entity = entityOf(document, id);
        const bool inside = isInside(entity, rectangle);
        if (inside || (mode == RectangleMode::Crossing && intersects(entity, rectangle))) {
            selected.push_back(id);
        }
    }
    std::ranges::sort(selected, {}, &EntityId::value);
    return selected;
}

std::optional<SnapPoint> findSnap(const Document& document, const Vec2& pointer, double aperture,
                                  const SnapModes& modes) {
    assert(aperture >= 0.0);
    if (!modes.endpoint && !modes.midpoint && !modes.center) {
        return std::nullopt;
    }
    std::optional<SnapPoint> point;
    Candidate bestPoint;
    std::optional<SnapPoint> center;
    Candidate bestCenter;
    std::vector<SnapPoint> curvePoints;

    // One curve within the aperture: a polyline segment, or a whole line,
    // circle or arc, at `curveDistance` from the pointer.
    const auto consider = [&](double curveDistance, std::uint64_t id) {
        for (const SnapPoint& snap : curvePoints) {
            if (snap.kind == SnapKind::Center) {
                // The centre is reached by passing over its curve.
                const Candidate candidate{curveDistance, 0, id};
                if (!center || candidate.beats(bestCenter)) {
                    center = snap;
                    bestCenter = candidate;
                }
                continue;
            }
            const Candidate candidate{distance(snap.point, pointer), tieRank(snap.kind), id};
            if (candidate.distance <= aperture && (!point || candidate.beats(bestPoint))) {
                point = snap;
                bestPoint = candidate;
            }
        }
    };

    for (const EntityId id : candidates(document, squareAround(pointer, aperture))) {
        const Entity& entity = entityOf(document, id);
        if (const auto* polyline = std::get_if<Polyline>(&entity)) {
            forEachSnapSegment(*polyline, [&](const PolylineSegment& segment) {
                const double segmentDistance = distance(segment, pointer);
                if (segmentDistance > aperture) {
                    return;
                }
                curvePoints.clear();
                appendSnapPoints(segment, modes, curvePoints);
                consider(segmentDistance, id.value);
            });
            continue;
        }
        // Lines, circles and arcs are a single curve each.
        const double curveDistance = distance(entity, pointer);
        if (curveDistance > aperture) {
            continue;
        }
        curvePoints.clear();
        appendSnapPoints(entity, modes, curvePoints);
        consider(curveDistance, id.value);
    }
    return point ? point : center;
}

}  // namespace tg::core
