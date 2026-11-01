// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

// Where curves meet and on which side a point lies, decided by solid-space,
// the kernel of Tecnigrafo (docs/adr/0035, point 1; docs/adr/0030). This is
// a facade: it only translates. Every "they meet", "where", "in which order"
// and "on which side" comes from solid-space; the facade never compares
// stations and adds no threshold of its own.

#include "tg/core/document.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace tg::core {

/// Which part of an entity takes part (docs/adr/0035, points 1 and 6).
enum class Extension : std::uint8_t {
    None,  ///< the entity as it is
    /// A line: its whole line. An arc: its whole circle. An open polyline: the
    /// line or circle of its first segment, for the points before its start,
    /// and of its last segment, for the points after its end; a segment that
    /// is exactly a point meets nothing, so these are the first and the last
    /// segment that are not. Circles, closed polylines and texts: as None.
    ///
    /// For an open polyline of more than one segment the shared stretches
    /// (SharedStretch, HostCuts::overlapping) are searched on its segments
    /// only, without the extension: solid-space extends every cutter of a call
    /// alike, and cutting a stretch found on the whole line of a middle
    /// segment back to the allowed part would need comparing stations
    /// (decision of the maintainer, #357).
    Extended,
};

/// Where a point lies on a segment of an entity, decided exactly by
/// solid-space: never deduced from stations.
enum class Location : std::uint8_t { BeforeStart, AtStart, Inside, AtEnd, AfterEnd };

/// A position on the curve of an entity.
struct CurvePosition {
    /// For a polyline, the index of the segment (Polyline::segment); else 0.
    std::size_t segment{0};
    Location location{Location::Inside};
    /// Arc length from the start of the segment (for an arc, counter-clockwise
    /// from its start angle; for a circle, from angle 0), as solid-space gives
    /// it: rounded, for information only, never to order or compare points.
    double station{0.0};
    friend bool operator==(const CurvePosition&, const CurvePosition&) = default;
};

/// How two curves meet at a point, decided exactly.
enum class Contact : std::uint8_t {
    Crossing,   ///< different tangent directions
    Touching,   ///< the same tangent line, different curves
    SameCurve,  ///< on the same line or circle, meeting only there
};

struct Crossing {
    Vec2 point;  ///< within the error solid-space declares
    Contact contact{Contact::Crossing};
    CurvePosition onA;
    CurvePosition onB;
};

/// A stretch two curves share, from its end nearer the start of A.
struct SharedStretch {
    bool whole{false};  ///< the whole line or circle (extended lines, equal circles)
    Vec2 from;
    Vec2 to;
    CurvePosition fromOnA, toOnA, fromOnB, toOnB;
};

struct Intersections {
    /// In the exact order along A; no two are the same exact point.
    std::vector<Crossing> crossings;
    /// In the order of their start along A.
    std::vector<SharedStretch> shared;
    /// False when solid-space refused some data (beyond its domain, 2^50):
    /// what it refused is left out (docs/adr/0035, point 23).
    bool complete{true};
};

/// Where the curves of `a` and `b` meet (docs/adr/0035, point 1). A text has no
/// curve: empty. A line of zero length, or a polyline segment that is exactly a
/// point, meets nothing.
///
/// A point where several segments of B meet A (a vertex of B, or a point
/// where B crosses itself) is listed once: at a vertex, on the segment that
/// follows it; otherwise on the segment of B with the lowest index.
[[nodiscard]] Intersections intersect(const Entity& a, Extension extendA, const Entity& b,
                                      Extension extendB);

/// Where a polyline meets itself: the common points of two segments that are
/// not consecutive, and those of two consecutive segments other than their
/// shared vertex. Both positions are on the polyline, onA before onB.
[[nodiscard]] Intersections selfIntersections(const Polyline& polyline);

/// A point of the host where one or more cutters meet it.
struct Cut {
    Vec2 point;
    CurvePosition onHost;
    /// The cutters through this exact point, by increasing index.
    std::vector<std::size_t> cutters;
};

struct HostCuts {
    /// In the exact order along the host; points of different cutters that are
    /// the same exact point form a single Cut.
    std::vector<Cut> cuts;
    /// Cutters that share a stretch with the host, by increasing index. They
    /// give no cuts (docs/adr/0035, point 4).
    std::vector<std::size_t> overlapping;
    bool complete{true};
};

/// The cuts that `cutters` make on `host`, as TRIM and EXTEND need them
/// (docs/adr/0035, points 4-6), from solid-space's intersectAlong. Texts among
/// the cutters cut nothing. With `hostCutsItself`, a polyline host also cuts
/// itself: every vertex is a cut, and so is every point of
/// selfIntersections(); such cuts may have no cutters.
[[nodiscard]] HostCuts cutsAlong(const Entity& host, Extension extendHost,
                                 std::span<const Entity> cutters, bool hostCutsItself = false);

enum class Side : std::uint8_t { Left, On, Right };

/// The side of `point` with respect to the line or circle of the entity
/// (solid-space side()), looking along the curve: for a circle and an arc,
/// left is inside. For a polyline, of its segment nearest to the point
/// (closestPoint; on a tie the first). std::nullopt for a text and for a curve
/// that is exactly a point (and for data solid-space refuses).
[[nodiscard]] std::optional<Side> sideOf(const Entity& entity, const Vec2& point);

/// The position of the point of the entity nearest to `point`, with
/// Tecnigrafo's projection (closestPoint; on a tie the first segment): Inside,
/// or AtStart or AtEnd when the nearest point is that end. For choosing among
/// cuts by a pick, not an exact decision (docs/adr/0035, point 5). A polyline
/// without segments gives AtStart of segment 0.
/// Precondition: not a Text.
[[nodiscard]] CurvePosition nearestPosition(const Entity& entity, const Vec2& point);

}  // namespace tg::core
