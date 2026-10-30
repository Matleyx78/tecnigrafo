// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/snap_points.hpp"

#include "tg/core/arc_geometry.hpp"

#include <array>
#include <cmath>
#include <numbers>
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

/// The quadrants of the circle of `arc` that lie within the arc, at 0, 90,
/// 180 and 270 degrees (docs/adr/0033, point 2). The points are the exact
/// sums of the centre and the radius, not cos/sin values, so that a
/// quadrant on an end point of the arc is that very point.
void appendQuadrants(const ArcGeometry& arc, std::vector<SnapPoint>& out) {
    const Vec2& center = arc.center;
    const double radius = arc.radius;
    const std::array<std::pair<double, Vec2>, 4> quadrants{{
        {0.0, {center.x + radius, center.y}},
        {std::numbers::pi / 2.0, {center.x, center.y + radius}},
        {std::numbers::pi, {center.x - radius, center.y}},
        {3.0 * std::numbers::pi / 2.0, {center.x, center.y - radius}},
    }};
    for (const auto& [angle, point] : quadrants) {
        if (containsAngle(arc, angle)) {
            out.push_back({point, SnapKind::Quadrant});
        }
    }
}

/// The foot of the perpendicular from `from` to a straight segment, if it
/// falls on the segment and not on its extension (docs/adr/0033, point 4).
/// The test is the one of closestPoint(): the projection along the segment
/// against 0 and its length, with no tolerance of its own.
void appendLineFoot(const PolylineSegment& segment, const Vec2& from, std::vector<SnapPoint>& out) {
    const Vec2 delta = segment.end - segment.start;
    const double length = delta.length();
    if (!(length > 0.0)) {
        return;
    }
    const Vec2 direction = delta * (1.0 / length);
    const double along = dot(from - segment.start, direction);
    if (along < 0.0 || along > length) {
        return;
    }
    // The end points stay exact when the foot is on one of them.
    Vec2 foot = segment.start + direction * along;
    if (along == 0.0) {
        foot = segment.start;
    } else if (along == length) {
        foot = segment.end;
    }
    out.push_back({foot, SnapKind::Perpendicular});
}

/// The feet of the perpendiculars from `from` to a circular arc: the points
/// of its circle on the line through the centre and `from`, if they fall
/// within the arc (containsAngle()). The nearer one comes first. From the
/// centre every direction is as good: there is no foot.
void appendArcFeet(const ArcGeometry& arc, const Vec2& from, std::vector<SnapPoint>& out) {
    const Vec2 radial = from - arc.center;
    const double length = radial.length();
    if (!(length > 0.0)) {
        return;
    }
    const Vec2 outward = radial * (arc.radius / length);
    if (containsAngle(arc, std::atan2(radial.y, radial.x))) {
        out.push_back({arc.center + outward, SnapKind::Perpendicular});
    }
    if (containsAngle(arc, std::atan2(-radial.y, -radial.x))) {
        out.push_back({arc.center - outward, SnapKind::Perpendicular});
    }
}

/// Quadrants and feet of the perpendiculars of a circle or an arc.
void appendCircularPoints(const ArcGeometry& arc, const SnapModes& modes,
                          const std::optional<Vec2>& from, std::vector<SnapPoint>& out) {
    if (modes.quadrant) {
        appendQuadrants(arc, out);
    }
    if (modes.perpendicular && from) {
        appendArcFeet(arc, *from, out);
    }
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
                      std::vector<SnapPoint>& out, std::optional<Vec2> from) {
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
    if (segment.isArc()) {
        const ArcGeometry geometry = arcFromBulge(segment.start, segment.end, segment.bulge);
        if (modes.center) {
            out.push_back({geometry.center, SnapKind::Center});
        }
        appendCircularPoints(geometry, modes, from, out);
    } else if (modes.perpendicular && from) {
        appendLineFoot(segment, *from, out);
    }
}

void appendSnapPoints(const Entity& entity, const SnapModes& modes, std::vector<SnapPoint>& out,
                      std::optional<Vec2> from) {
    std::visit(
        Overloaded{
            [&](const Polyline& polyline) { appendPolylineSnapPoints(polyline, modes, out, from); },
            [&](const Line& line) {
                appendSnapPoints(PolylineSegment{line.start, line.end}, modes, out, from);
            },
            [&](const Circle& circle) {
                if (modes.center) {
                    out.push_back({circle.center, SnapKind::Center});
                }
                appendCircularPoints(circle.geometry(), modes, from, out);
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
                appendCircularPoints(geometry, modes, from, out);
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
