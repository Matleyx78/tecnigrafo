// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "solid_space_bridge.hpp"

#include "tg/core/math.hpp"

#include <cassert>
#include <cstddef>
#include <utility>
#include <variant>

namespace tg::core::bridge {
namespace {

// Keep the exact axis endpoints from #422: the ill-conditioned grip case
// grip/degeneri depends on them. The adjustment stays within the kernel's
// declared point error.
std::optional<Vec2> axisEndpoint(const ArcGeometry& arc, double angle) {
    const double normalized = normalizeAngle(angle);
    if (normalized == 0.0) {
        return Vec2{arc.center.x + arc.radius, arc.center.y};
    }
    if (normalized == kPi / 2.0) {
        return Vec2{arc.center.x, arc.center.y + arc.radius};
    }
    if (normalized == kPi) {
        return Vec2{arc.center.x - arc.radius, arc.center.y};
    }
    if (normalized == 3.0 * kPi / 2.0) {
        return Vec2{arc.center.x, arc.center.y - arc.radius};
    }
    return std::nullopt;
}

}  // namespace

g::Point2 toPoint(const Vec2& point) {
    return g::Point2{point.x, point.y};
}

Vec2 fromPoint(const g::Point2& point) {
    return Vec2{point.x, point.y};
}

std::optional<g::Curve> curveOf(const Entity& entity) {
    if (const auto* line = std::get_if<Line>(&entity)) {
        return g::Curve{g::Segment{toPoint(line->start), toPoint(line->end)}};
    }
    if (const auto* circle = std::get_if<Circle>(&entity)) {
        return g::Curve{g::Circle{toPoint(circle->center), circle->radius}};
    }
    if (const auto* arc = std::get_if<Arc>(&entity)) {
        double sweep = arc->endAngle - arc->startAngle;
        if (!(sweep > 0.0)) {
            sweep += g::kFullTurn;
        }
        return g::Curve{g::Arc{toPoint(arc->center), arc->radius, arc->startAngle, sweep}};
    }
    return std::nullopt;
}

std::pair<Vec2, Vec2> arcEndpoints(const Arc& arc) {
    const auto geometry = arc.geometry();
    const auto curve = curveOf(arc);
    assert(curve.has_value());  // An Arc always has a kernel curve.
    const auto ends = g::endpoints(*curve);
    Vec2 start = ends.hasValue() ? fromPoint(ends.value().start) : startPoint(geometry);
    Vec2 end = ends.hasValue() ? fromPoint(ends.value().end) : endPoint(geometry);
    start = axisEndpoint(geometry, geometry.startAngle).value_or(start);
    end = axisEndpoint(geometry, geometry.startAngle + geometry.sweep).value_or(end);
    return {start, end};
}

g::PolylineSegment pieceOf(const PolylineSegment& segment) {
    return g::PolylineSegment{toPoint(segment.start), toPoint(segment.end),
                              segment.isArc() ? segment.bulge : 0.0};
}

std::vector<g::PolylineSegment> piecesOf(const Polyline& polyline) {
    g::Polyline curve;
    curve.closed = polyline.isClosed();
    curve.vertices.reserve(polyline.vertexCount());
    for (const PolylineVertex& vertex : polyline.vertices()) {
        curve.vertices.push_back(g::PolylineVertex{toPoint(vertex.position), vertex.bulge});
    }
    std::vector<g::PolylineSegment> pieces = g::pieces(curve);
    assert(pieces.size() == polyline.segmentCount());
    for (std::size_t i = 0; i < pieces.size(); ++i) {
        if (!polyline.segment(i).isArc()) {
            pieces[i].bulge = 0.0;
        }
    }
    return pieces;
}

solid_space::robust::TolerancePolicy policyOf(const ToleranceRule& rule) {
    auto policy = solid_space::robust::TolerancePolicy::forUnit(unitInMeters(rule.units()));
    // forUnit gives a value for each of the 25 units of INSUNITS
    // (docs/tappe/0.7/P14-intersezioni.md): a diagnosis here is a bug.
    assert(policy.hasValue());
    return std::move(policy).value();
}

}  // namespace tg::core::bridge
