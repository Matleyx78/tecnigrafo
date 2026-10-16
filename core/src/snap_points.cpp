// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/snap_points.hpp"

#include <numeric>
#include <set>
#include <tuple>

namespace tg::core {

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
        [&](const Polyline& polyline) {
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
        },
        entity);
}

}  // namespace tg::core
