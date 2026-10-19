// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/snap_points.hpp"

#include "tg/core/arc_geometry.hpp"

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

void appendPolylineSnapPoints(const Polyline& polyline, const SnapModes& modes,
                              std::vector<SnapPoint>& out) {
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
        appendSnapPoints(polyline.segment(i), modes, segmentPoints);
        for (const SnapPoint& snap : segmentPoints) {
            if (seen.emplace(snap.point.x, snap.point.y, snap.kind).second) {
                out.push_back(snap);
            }
        }
    }
}

}  // namespace

void appendSnapPoints(const PolylineSegment& segment, const SnapModes& modes,
                      std::vector<SnapPoint>& out) {
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
    if (modes.center && segment.isArc()) {
        out.push_back(
            {arcFromBulge(segment.start, segment.end, segment.bulge).center, SnapKind::Center});
    }
}

void appendSnapPoints(const Entity& entity, const SnapModes& modes, std::vector<SnapPoint>& out) {
    std::visit(
        Overloaded{
            [&](const Polyline& polyline) { appendPolylineSnapPoints(polyline, modes, out); },
            [&](const Line& line) {
                appendSnapPoints(PolylineSegment{line.start, line.end}, modes, out);
            },
            [&](const Circle& circle) {
                if (modes.center) {
                    out.push_back({circle.center, SnapKind::Center});
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
            },
        },
        entity);
}

}  // namespace tg::core
