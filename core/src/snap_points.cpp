// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/snap_points.hpp"

#include "tg/core/arc_geometry.hpp"
#include "tg/core/math.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <set>
#include <tuple>

namespace tg::core {

namespace {

/// Builds one visitor out of several lambdas, one per entity type.
template <typename... Lambdas>
struct Overloaded : Lambdas... {
    using Lambdas::operator()...;
};

// Explicit deduction guide: aggregate CTAD is missing from Apple Clang 15.
template <typename... Lambdas>
Overloaded(Lambdas...) -> Overloaded<Lambdas...>;

/// The quadrants of the arc: the points of its circle at 0°, 90°, 180° and
/// 270° that fall on the arc, its end points included (docs/adr/0033, point 2).
void appendQuadrants(const ArcGeometry& arc, std::vector<SnapPoint>& out) {
    // The exact offsets, not cos/sin of multiples of π/2, which are off by
    // about 1e-16 of the radius (as in expandByAxisExtremes()).
    const std::array offsets{Vec2{arc.radius, 0.0}, Vec2{0.0, arc.radius}, Vec2{-arc.radius, 0.0},
                             Vec2{0.0, -arc.radius}};
    for (std::size_t quadrant = 0; quadrant < offsets.size(); ++quadrant) {
        if (containsAngle(arc, static_cast<double>(quadrant) * (kPi / 2.0))) {
            out.push_back({arc.center + offsets.at(quadrant), SnapKind::Quadrant});
        }
    }
}

/// The feet of the perpendiculars from `from` to the circle of `arc` that
/// fall on the arc (docs/adr/0033, point 4): the two points of the circle on
/// the line through the centre and `from`, the one on the side of `from`
/// first. Whether a foot falls on the arc is decided by containsAngle(), the
/// criterion of every other arc test: no new threshold.
void appendArcPerpendiculars(const ArcGeometry& arc, const Vec2& from,
                             std::vector<SnapPoint>& out) {
    const Vec2 radial = from - arc.center;
    const double length = radial.length();
    // Exact zero guards the division, as in closestPoint(): from the centre
    // every direction is perpendicular and none is chosen.
    if (!(length > 0.0)) {
        return;
    }
    const Vec2 offset = radial * (arc.radius / length);
    const double angle = std::atan2(radial.y, radial.x);
    if (containsAngle(arc, angle)) {
        out.push_back({arc.center + offset, SnapKind::Perpendicular});
    }
    if (containsAngle(arc, angle + kPi)) {
        out.push_back({arc.center - offset, SnapKind::Perpendicular});
    }
}

/// The foot of the perpendicular from `from` to a straight segment, if it
/// falls on the segment and not on its extension. The parameter along the
/// segment is compared with 0 and 1 as closestOnLine() of proximity.cpp does.
void appendLinePerpendicular(const PolylineSegment& segment, const Vec2& from,
                             std::vector<SnapPoint>& out) {
    const Vec2 delta = segment.end - segment.start;
    const double lengthSquared = delta.lengthSquared();
    // Exact zero guards the division (a segment can underflow to it).
    if (!(lengthSquared > 0.0)) {
        return;
    }
    const double parameter = dot(from - segment.start, delta) / lengthSquared;
    // Written positively so that a NaN is not on the segment.
    const bool onSegment = parameter >= 0.0 && parameter <= 1.0;
    if (!onSegment) {
        return;
    }
    // A foot at an end is that end, not its roundoff.
    Vec2 foot = segment.start + (delta * parameter);
    if (parameter <= 0.0) {
        foot = segment.start;
    } else if (parameter >= 1.0) {
        foot = segment.end;
    }
    out.push_back({foot, SnapKind::Perpendicular});
}

void appendPolylineSnapPoints(const Polyline& polyline, const SnapModes& modes,
                              std::vector<SnapPoint>& out, const std::optional<Vec2>& from) {
    if (polyline.vertexCount() == 1) {
        if (modes.endpoint) {
            out.push_back({polyline.vertices().front().position, SnapKind::Endpoint});
        }
        return;
    }
    // Deduplicate exact stored values, not nearby geometry: merging
    // by a drawing-unit tolerance would discard distinct snap targets.
    std::set<std::tuple<double, double, SnapKind>> seen;
    std::vector<SnapPoint> segmentPoints;
    for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
        segmentPoints.clear();
        appendSnapPoints(polyline.segment(i), modes, segmentPoints, from);
        for (const SnapPoint& snap : segmentPoints) {
            if (seen.emplace(snap.point.x, snap.point.y, snap.kind).second) {
                out.push_back(snap);
            }
        }
    }
}

}  // namespace

void appendSnapPoints(const PolylineSegment& segment, const SnapModes& modes,
                      std::vector<SnapPoint>& out, const std::optional<Vec2>& from) {
    if (modes.endpoint) {
        out.push_back({segment.start, SnapKind::Endpoint});
    }
    // Exact identity preserves arbitrarily short segments, as in isArc().
    if (segment.start == segment.end) {
        return;
    }
    if (modes.endpoint) {
        out.push_back({segment.end, SnapKind::Endpoint});
    }
    if (modes.midpoint) {
        const Vec2 chordMidpoint{std::midpoint(segment.start.x, segment.end.x),
                                 std::midpoint(segment.start.y, segment.end.y)};
        // The signed sagitta reaches halfway along the swept arc, including
        // major arcs. This avoids subtracting large radii for shallow arcs.
        const Vec2 midpoint = segment.isArc()
                                  ? chordMidpoint - (segment.end - segment.start).perpendicular() *
                                                        (segment.bulge * 0.5)
                                  : chordMidpoint;
        out.push_back({midpoint, SnapKind::Midpoint});
    }
    const bool perpendicular = modes.perpendicular && from.has_value();
    if (!segment.isArc()) {
        if (perpendicular) {
            appendLinePerpendicular(segment, *from, out);
        }
        return;
    }
    if (!modes.center && !modes.quadrant && !perpendicular) {
        return;
    }
    const ArcGeometry arc = arcFromBulge(segment.start, segment.end, segment.bulge);
    if (modes.center) {
        out.push_back({arc.center, SnapKind::Center});
    }
    if (modes.quadrant) {
        appendQuadrants(arc, out);
    }
    if (perpendicular) {
        appendArcPerpendiculars(arc, *from, out);
    }
}

void appendSnapPoints(const Entity& entity, const SnapModes& modes, std::vector<SnapPoint>& out,
                      const std::optional<Vec2>& from) {
    std::visit(
        Overloaded{
            [&](const Polyline& polyline) { appendPolylineSnapPoints(polyline, modes, out, from); },
            [&](const Line& line) {
                appendSnapPoints(PolylineSegment{line.start, line.end}, modes, out, from);
            },
            [&](const Circle& circle) {
                const ArcGeometry geometry = circle.geometry();
                if (modes.center) {
                    out.push_back({circle.center, SnapKind::Center});
                }
                if (modes.quadrant) {
                    appendQuadrants(geometry, out);
                }
                if (modes.perpendicular && from.has_value()) {
                    appendArcPerpendiculars(geometry, *from, out);
                }
            },
            [&](const Arc& arc) {
                const ArcGeometry geometry = arc.geometry();
                if (modes.endpoint) {
                    out.push_back({startPoint(geometry), SnapKind::Endpoint});
                    out.push_back({endPoint(geometry), SnapKind::Endpoint});
                }
                if (modes.midpoint) {
                    out.push_back({pointAt(geometry, geometry.startAngle + (geometry.sweep / 2.0)),
                                   SnapKind::Midpoint});
                }
                if (modes.center) {
                    out.push_back({arc.center, SnapKind::Center});
                }
                if (modes.quadrant) {
                    appendQuadrants(geometry, out);
                }
                if (modes.perpendicular && from.has_value()) {
                    appendArcPerpendiculars(geometry, *from, out);
                }
            },
            [&](const Text& text) {
                if (modes.insertion) {
                    out.push_back({textFrame(text).insertion, SnapKind::Insertion});
                }
            },
        },
        entity);
}

}  // namespace tg::core
