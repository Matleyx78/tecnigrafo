// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/offset.hpp"

#include "solid_space/geom2d/offset.hpp"
#include "solid_space_bridge.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/intersections.hpp"
#include "tg/core/line.hpp"
#include "tg/core/polyline.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

namespace g = solid_space::geom2d;

OffsetResult withOutcome(OffsetOutcome outcome) {
    return OffsetResult{outcome, std::nullopt, false};
}

/// The entity of the curve offsetCurve gives for `input`: the same type as
/// the input (curveOf gives a Segment for a Line, a Circle for a Circle, an
/// Arc for an Arc, and offsetCurve keeps the kind of curve).
Entity entityOf(const Entity& input, const g::Curve& curve) {
    if (std::holds_alternative<Line>(input)) {
        const auto& segment = std::get<g::Segment>(curve);
        return Line{bridge::fromPoint(segment.start), bridge::fromPoint(segment.end)};
    }
    if (std::holds_alternative<Circle>(input)) {
        const auto& circle = std::get<g::Circle>(curve);
        return Circle{bridge::fromPoint(circle.center), circle.radius};
    }
    // The same start and end angles as the input, bit for bit: offsetCurve
    // keeps the start angle and the sweep of the arc, so start + sweep would
    // only add a rounding (docs/tappe/0.7/P17-curva-parallela.md).
    const auto& inputArc = std::get<Arc>(input);
    const auto& arc = std::get<g::Arc>(curve);
    return Arc{bridge::fromPoint(arc.center), arc.radius, inputArc.startAngle, inputArc.endAngle};
}

OffsetResult offsetCurveEntity(const Entity& entity, double signedDistance) {
    const std::optional<g::Curve> curve = bridge::curveOf(entity);
    assert(curve.has_value());
    const auto offset = g::offsetCurve(*curve, signedDistance);
    if (!offset.hasValue()) {
        return withOutcome(OffsetOutcome::Failed);
    }
    const g::CurveOffset& result = offset.value();
    switch (result.outcome) {
        case g::OffsetOutcome::done:
            return OffsetResult{OffsetOutcome::Done, entityOf(entity, result.curve), false};
        case g::OffsetOutcome::vanished:
            return withOutcome(OffsetOutcome::Vanished);
        case g::OffsetOutcome::failed:
            return withOutcome(OffsetOutcome::Failed);
    }
    return withOutcome(OffsetOutcome::Failed);
}

/// The polyline of solid-space for `polyline`, with the positions and bulges
/// of its vertices; a segment that PolylineSegment::isArc() considers straight
/// passes with bulge 0, as in bridge::piecesOf (docs/adr/0035, point 1).
g::Polyline polylineCurveOf(const Polyline& polyline) {
    g::Polyline curve;
    curve.closed = polyline.isClosed();
    curve.vertices.reserve(polyline.vertexCount());
    for (const PolylineVertex& vertex : polyline.vertices()) {
        curve.vertices.push_back(g::PolylineVertex{bridge::toPoint(vertex.position), vertex.bulge});
    }
    for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
        if (!polyline.segment(i).isArc()) {
            curve.vertices[i].bulge = 0.0;
        }
    }
    return curve;
}

/// The copy of `input` with the vertices of `offset` (docs/adr/0035, point 11).
/// Segment i of the copy starts at vertex i and comes from origins[i].
/// Widths come from `origins`, one entry per segment of the copy, never from
/// the position of a vertex: pieces of the input may have vanished or been
/// removed, and joint arcs come in between.
Polyline polylineOf(const Polyline& input, const g::PolylineOffset& offset) {
    const std::span<const PolylineVertex> source = input.vertices();
    std::vector<PolylineVertex> vertices;
    vertices.reserve(offset.polyline.vertices.size());
    for (const g::PolylineVertex& vertex : offset.polyline.vertices) {
        // Widths 0: a joint arc and the last vertex of an open copy.
        vertices.push_back(PolylineVertex{bridge::fromPoint(vertex.position), vertex.bulge});
    }
    for (std::size_t i = 0; i < offset.origins.size(); ++i) {
        const g::OffsetPiece& piece = offset.origins[i];
        if (piece.origin == g::PieceOrigin::source) {
            assert(i < vertices.size() && piece.index < input.segmentCount());
            vertices[i].startWidth = source[piece.index].startWidth;
            vertices[i].endWidth = source[piece.index].endWidth;
        }
    }
    Polyline result{std::move(vertices), offset.polyline.closed};
    assert(offset.origins.size() == result.segmentCount());
    result.setConstantWidth(input.constantWidth());
    result.setContinuousLinetype(input.continuousLinetype());
    return result;
}

OffsetResult offsetPolylineEntity(const Polyline& polyline, double signedDistance,
                                  const ToleranceRule& rule) {
    const auto offset =
        g::offsetPolyline(polylineCurveOf(polyline), signedDistance, bridge::policyOf(rule));
    if (!offset.hasValue()) {
        return withOutcome(OffsetOutcome::Failed);
    }
    const g::PolylineOffset& result = offset.value();
    switch (result.outcome) {
        case g::OffsetOutcome::done:
            return OffsetResult{OffsetOutcome::Done, Entity{polylineOf(polyline, result)},
                                result.simple};
        case g::OffsetOutcome::vanished:
            return withOutcome(OffsetOutcome::Vanished);
        case g::OffsetOutcome::failed:
            return withOutcome(OffsetOutcome::Failed);
    }
    return withOutcome(OffsetOutcome::Failed);
}

}  // namespace

OffsetResult offsetEntity(const Entity& entity, double distance, const Vec2& sidePoint,
                          const ToleranceRule& rule) {
    assert(!std::holds_alternative<Text>(entity));
    assert(std::isfinite(distance) && distance > 0.0);
    const std::optional<Side> side = sideOf(entity, sidePoint);
    if (!side) {
        // No side for a curve that is not a text: a curve that is exactly a
        // point, a polyline without segments or with only such segments, or
        // data solid-space refuses (decision B5 of #464).
        return withOutcome(OffsetOutcome::Failed);
    }
    if (*side == Side::On) {
        return withOutcome(OffsetOutcome::NoSide);
    }
    // offset.hpp of solid-space: a positive distance is on the left.
    const double signedDistance = *side == Side::Left ? distance : -distance;
    if (const auto* polyline = std::get_if<Polyline>(&entity)) {
        return offsetPolylineEntity(*polyline, signedDistance, rule);
    }
    return offsetCurveEntity(entity, signedDistance);
}

}  // namespace tg::core
