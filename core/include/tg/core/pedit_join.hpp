// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/tolerance.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace tg::core {

/// How PEDIT Join bridges a gap (docs/adr/0050, point 4; Jointype of AutoCAD).
enum class JoinType : std::uint8_t {
    /// The two facing segments are extended or shortened to where their
    /// extended curves meet.
    Extend,
    /// A straight segment of zero width between the two ends.
    Add,
    /// Extend when the extended curves meet at an isolated point, else Add.
    Both,
};

struct GapJoinResult {
    Polyline polyline;                  // the joined source
    std::vector<std::size_t> consumed;  // indices of `others` used
    std::size_t added{0};               // segments added, bridges included
};

/// Joins `others` (lines, arcs, open polylines) to the ends of `source`
/// (docs/adr/0050, point 4). fuzz == 0: the rule of join() (docs/adr/0035).
///
/// From the end of `source`, then from its start: at each step, among the
/// objects not used yet, first those with an end that coincides with that end
/// of the polyline under `rule` (in the order of `others`; they attach as in
/// join(), whatever `fuzz` and `type`), then those with an end at distance at
/// most `fuzz`, by increasing distance (ties: the first in `others`). An object
/// attaches by its nearer end, its start when both are as near, reversed if
/// needed. With Extend (and Both) the last segment of the polyline and the
/// first of the object, or the last of the object and the first of the
/// polyline at the start, end at the isolated point of their extended curves
/// (lines and whole circles, from solid-space) nearest to the two ends (least
/// sum of distances; ties: the first solid-space gives), used as it is: a
/// segment is extended or shortened, an arc keeps its centre and radius and
/// gets a new bulge, every segment keeps its widths. Without such a point (no
/// common point, or curves on one line or circle), or when a segment would
/// shrink to nothing, Extend does not join the pair and Both adds a bridge;
/// the next object of the ranking is tried. The steps go on until no object
/// joins at that end. When the free end comes to coincide with the other end
/// under `rule`, the polyline closes and the join ends.
///
/// Objects that do not join: closed polylines, other entity types, and lines,
/// arcs and segments that solid-space refuses or that are a whole circle.
/// An Entity has no elevation: leaving out the objects at an elevation other
/// than the source's is the caller's (the PEDIT command, PT33; decision of
/// 2026-11-15 on RT32).
///
/// `added` counts the segments of the objects and the bridges that enter the
/// polyline; `consumed` is in the order the objects attach. With nothing
/// consumed, `polyline` is `source` unchanged. Otherwise it is rebuilt from
/// the joined segments, as by join(): a constant width shared by all of them
/// becomes the constant width; the last vertex of `source` keeps its data
/// when nothing attaches at its end (docs/adr/0006); closure aside, the
/// linetype generation of `source` stays.
///
/// Precondition: `fuzz` is finite and not negative.
[[nodiscard]] GapJoinResult joinWithGaps(const Polyline& source, std::span<const Entity> others,
                                         double fuzz, JoinType type, const ToleranceRule& rule);

}  // namespace tg::core
