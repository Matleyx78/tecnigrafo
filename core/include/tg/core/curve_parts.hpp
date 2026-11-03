// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/intersections.hpp"

#include <cstdint>
#include <optional>

namespace tg::core {

/// A place on the curve of an entity, as intersect(), cutsAlong() and the
/// constructions of solid-space give it: the point and its position.
struct CurvePoint {
    Vec2 point;
    CurvePosition position;
};

/// How the part runs from `from` to `to`. The caller knows it exactly, from
/// the order of HostCuts::cuts or from a construction; partOf never deduces it
/// from points or stations, which are rounded (#359).
enum class PartCourse : std::uint8_t {
    Direct,       ///< `from` comes before `to`, without passing the start
    AcrossStart,  ///< a circle or a closed polyline, passing its start
                  ///< (angle 0, vertex 0)
    SamePlace,    ///< `from` and `to` are the same place: nothing in between
};

enum class PartOutcome : std::uint8_t {
    Done,              ///< `entity` holds the part
    Empty,             ///< nothing is left: the part does not stay
    NotRepresentable,  ///< the exact part exists, but the model cannot hold it
};

struct PartResult {
    PartOutcome outcome{PartOutcome::Empty};
    std::optional<Entity> entity;  ///< with Done
};

/// The part of `entity` from `from` to `to`, each std::nullopt for its start
/// or its end (docs/adr/0035, point 5):
/// - Line: from `from` to `to`;
/// - Arc: counter-clockwise from `from` to `to`;
/// - Circle: the arc counter-clockwise from `from` to `to` (both required);
/// - Polyline: the vertices between the two places, with the cut segments
///   ending at the given points; a closed polyline becomes open, running from
///   `from` to `to`, across its vertex 0 with AcrossStart (both required).
/// Every supplied boundary point replaces the endpoint or vertex exactly,
/// including AtStart/AtEnd at an existing vertex. Supplied arc endpoints are
/// also rebuilt, even at AtStart/AtEnd; omitted endpoints keep their angles.
/// New arc angles come from solid_space::robust::atan2
/// of (point - centre), brought into [0, 2π) with normalizeAngle: the ends of
/// an arc lie within the error, not exactly on the given points. A cut
/// polyline arc gets the bulge of its new included angle; the widths of a cut
/// polyline segment are interpolated by length (docs/adr/0031, point 9), with
/// stations measured from its start in either direction. An interior end cut
/// copies the following original vertex's bulge and widths to the final,
/// non-drawing vertex, replacing only its position.
/// Every other property of the entity stays.
/// Empty with SamePlace, and when the part is a single arc (an Arc, a part of
/// a Circle, or a part of a polyline within one curved segment) whose two
/// angles, or two vertices, come out equal while the exact part is the short
/// one (Direct, on one segment); NotRepresentable when they come out equal
/// and the exact part is the long one (AcrossStart): it never becomes a
/// circle. Inverted computed angles follow the same rule, with exact
/// comparisons and no tolerance: for an Arc or a curved segment, a cut sweep
/// larger than the original; for a Circle, start > end with Direct or start
/// < end with AcrossStart. Within several polyline segments a collapsed or
/// inverted cut arc has bulge 0 and retains its supplied boundary points.
/// Precondition: not a Text; no place is BeforeStart or AfterEnd (extendTo
/// takes those); with Direct, `from` does not come after `to` in the order of
/// cutsAlong; AcrossStart only for a circle or a closed polyline, with `to`
/// not after `from`. A polyline vertex is AtStart of the following segment
/// (vertex 0 is segment 0, also on a closed polyline); only the final vertex
/// of an open polyline is AtEnd of its last segment.
[[nodiscard]] PartResult partOf(const Entity& entity, const std::optional<CurvePoint>& from,
                                const std::optional<CurvePoint>& to, PartCourse course);

enum class EntityEnd : std::uint8_t { Start, End };

/// `entity` with its end `end` brought to `to`, beyond that end (EXTEND, and
/// FILLET and CHAMFER when the point is BeforeStart or AfterEnd; docs/adr/0035,
/// point 6):
/// - Line: along its line, `to` BeforeStart for the start, AfterEnd for the end;
/// - Arc: along its circle, `to` AfterEnd for both ends: for the start the
///   circle is run backwards from the start;
/// - open Polyline: its first or last segment that is not exactly a point
///   (as Extension::Extended, P14), the same way as a line or an arc; the
///   vertices of the segments exactly a point between that segment and the
///   end take the new point and keep all their other data (docs/adr/0006).
/// The new point of a line and of a polyline is `to`, exactly; a new arc
/// angle comes as in partOf. NotRepresentable when the new angle of an arc,
/// or the new vertex of a curved polyline segment, comes out equal to the
/// other end of that arc or segment, or passes it: the computed directed
/// sweep is smaller than the original, compared exactly without tolerance.
/// The exact extension would close into a circle.
/// Precondition: a Line, an Arc or an open Polyline; `to` is on the segment
/// above, BeforeStart or AfterEnd as said.
[[nodiscard]] PartResult extendTo(const Entity& entity, EntityEnd end, const CurvePoint& to);

}  // namespace tg::core
