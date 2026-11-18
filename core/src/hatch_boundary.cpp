// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/hatch_boundary.hpp"

#include "solid_space/geom2d/boundary.hpp"
#include "solid_space/geom2d/region.hpp"
#include "solid_space_bridge.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/entity_record.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/tolerance.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {
namespace g = solid_space::geom2d;

// The flags of a boundary path, DXF group 92 (docs/verifica/rapporto.md).
constexpr std::uint32_t kExternal = 1U;
constexpr std::uint32_t kPolylinePath = 2U;
constexpr std::uint32_t kDerived = 4U;
constexpr std::uint32_t kOutermost = 16U;

/// The candidate curves of a detection, each with the entity it comes from.
struct Candidates {
    std::vector<g::Curve> curves;
    std::vector<EntityId> owners;  ///< as many as curves
};

Candidates candidatesOf(const Document& document) {
    Candidates candidates;
    for (const EntityRecord& record : document.entities()) {
        if (!isVisible(document, record)) {
            continue;
        }
        if (const auto* polyline = std::get_if<Polyline>(&record.entity)) {
            for (const g::PolylineSegment& piece : bridge::piecesOf(*polyline)) {
                candidates.curves.emplace_back(piece);
                candidates.owners.push_back(record.id);
            }
        } else if (std::optional<g::Curve> curve = bridge::curveOf(record.entity)) {
            // curveOf gives a curve for a Line, an Arc and a Circle only.
            candidates.curves.push_back(*curve);
            candidates.owners.push_back(record.id);
        }
    }
    return candidates;
}

/// A loop of solid-space as a loop of a hatch, with the same doubles.
HatchLoop hatchLoopOf(const g::Loop& loop, std::size_t depth, std::uint32_t flags) {
    HatchLoop hatchLoop;
    if (depth == 0) {
        flags |= kExternal;
    } else if (depth == 1) {
        flags |= kOutermost;
    }
    if (const auto* circle = std::get_if<g::Circle>(&loop)) {
        hatchLoop.edges.push_back(
            HatchEdge{Arc{bridge::fromPoint(circle->center), circle->radius, 0.0, kTwoPi}});
    } else {
        const auto& polyline = std::get<g::Polyline>(loop);
        std::vector<PolylineVertex> vertices;
        vertices.reserve(polyline.vertices.size());
        for (const g::PolylineVertex& vertex : polyline.vertices) {
            vertices.push_back(PolylineVertex{bridge::fromPoint(vertex.position), vertex.bulge});
        }
        hatchLoop.polyline = Polyline{std::move(vertices), true};
        flags |= kPolylinePath;
    }
    hatchLoop.flags = flags;
    return hatchLoop;
}

/// The contour of a chosen entity: a Circle or a closed Polyline.
std::optional<g::Loop> contourOf(const Document& document, EntityId id) {
    const EntityRecord* record = document.find(id);
    if (record == nullptr) {
        return std::nullopt;
    }
    if (const auto* circle = std::get_if<Circle>(&record->entity)) {
        return g::Loop{g::Circle{bridge::toPoint(circle->center), circle->radius}};
    }
    if (const auto* polyline = std::get_if<Polyline>(&record->entity);
        polyline != nullptr && polyline->isClosed()) {
        return g::Loop{bridge::closedPolylineOf(*polyline)};
    }
    return std::nullopt;
}

}  // namespace

DetectedBoundary detectHatchBoundary(const Document& document, const Vec2& picked) {
    assert(std::isfinite(picked.x) && std::isfinite(picked.y));
    const Candidates candidates = candidatesOf(document);
    const auto result = g::detectBoundary(bridge::toPoint(picked), candidates.curves,
                                          bridge::policyOf(ToleranceRule::of(document)));
    DetectedBoundary boundary;
    if (!result.hasValue()) {
        // A finite point fails only beyond the domain (boundary.hpp).
        assert(result.diagnosis().code == g::DiagnosisCode::outOfDomain);
        boundary.complete = false;
        return boundary;
    }
    const g::DetectedBoundary& found = result.value();
    boundary.complete = found.skipped.empty();
    switch (found.outcome) {
        case g::BoundaryOutcome::enclosed:
            boundary.outcome = BoundaryOutcome::Enclosed;
            boundary.loops.reserve(found.loops.size());
            for (const g::RegionLoop& loop : found.loops) {
                boundary.loops.push_back(hatchLoopOf(loop.loop, loop.depth, kDerived));
            }
            break;
        case g::BoundaryOutcome::onCurve:
            boundary.outcome = BoundaryOutcome::OnCurve;
            boundary.nearEntity = candidates.owners.at(found.nearCurve);
            break;
        case g::BoundaryOutcome::notEnclosed:
            boundary.outcome = BoundaryOutcome::NotEnclosed;
            boundary.openEnds.reserve(found.openEnds.size());
            for (const g::OpenEnd& end : found.openEnds) {
                boundary.openEnds.push_back(bridge::fromPoint(end.point));
            }
            break;
    }
    return boundary;
}

ChosenContours chosenHatchContours(const Document& document, std::span<const EntityId> chosen) {
    // contourIndex[i]: the index in `contours` of the contour of chosen[i].
    std::vector<g::Loop> contours;
    std::vector<std::optional<std::size_t>> contourIndex;
    contourIndex.reserve(chosen.size());
    for (const EntityId id : chosen) {
        std::optional<g::Loop> contour = contourOf(document, id);
        if (contour) {
            contourIndex.emplace_back(contours.size());
            contours.push_back(std::move(*contour));
        } else {
            contourIndex.emplace_back(std::nullopt);
        }
    }
    const g::ContourNesting nesting = g::nestContours(contours);
    assert(nesting.depth.size() == contours.size());
    ChosenContours result;
    for (std::size_t i = 0; i < chosen.size(); ++i) {
        const std::optional<std::size_t> index = contourIndex[i];
        if (!index) {
            result.leftOut.push_back(chosen[i]);
            continue;
        }
        const std::optional<std::size_t> depth = nesting.depth.at(*index);
        if (!depth) {
            result.leftOut.push_back(chosen[i]);
            continue;
        }
        result.loops.push_back(hatchLoopOf(contours[*index], *depth, 0U));
        result.taken.push_back(chosen[i]);
    }
    return result;
}

}  // namespace tg::core
