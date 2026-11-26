// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

// The boundaries of a hatch that the user does not draw (docs/adr/0048,
// points 5 and 6; docs/tappe/0.8/PT43-contorni-dei-retini.md): the contour
// and islands found around a picked point, and the contours chosen as
// objects with which lies inside which. solid-space decides; this facade
// picks the candidate curves and turns solid-space's loops into the loops of
// a hatch.
//
// The loops carry the flags of DXF group 92 (docs/verifica/rapporto.md, row
// HATCH): `polyline` (2) for a polyline path, none for an edge path;
// `external` (1) for depth 0, `outermost` (16) for depth 1; `derived` (4)
// for the loops detected from a point, not for the chosen ones. A whole
// circle is an edge path with one Arc edge from 0 to kTwoPi; any other loop
// is a polyline path: a closed Polyline with solid-space's vertices and
// bulges, widths 0.

#include "tg/core/document.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace tg::core {

/// How the detection around a picked point ended (solid-space's
/// BoundaryOutcome, boundary.hpp).
enum class BoundaryOutcome : std::uint8_t {
    Enclosed,     ///< `loops` holds the contour and its islands
    OnCurve,      ///< the point lies on a curve: `nearEntity` says which
    NotEnclosed,  ///< no closed contour around the point: `openEnds` may help
};

struct DetectedBoundary {
    BoundaryOutcome outcome{BoundaryOutcome::NotEnclosed};
    /// With Enclosed: the contour first, then the islands, in solid-space's
    /// order (boundary.hpp, DetectedBoundary::loops), as loops of a hatch with
    /// their flags (point 4). Empty otherwise.
    std::vector<HatchLoop> loops;
    /// Entities whose curves form `loops`, once each, in first-use order.
    std::vector<EntityId> sources;
    /// With OnCurve: the entity that owns the curve solid-space names.
    std::optional<EntityId> nearEntity;
    /// With NotEnclosed: the free ends solid-space gives, in its order.
    std::vector<Vec2> openEnds;
    /// False when solid-space left out a candidate curve, or the picked point
    /// (beyond its domain, docs/adr/0035, point 23).
    bool complete{true};
};

/// The boundary of a hatch from a point picked inside it (docs/adr/0048,
/// points 5 and 6): solid-space's detectBoundary over the candidate curves
/// of `document`, with the tolerance rule of the drawing.
///
/// The candidates are the Line, Arc, Circle and Polyline entities, in the
/// order of Document::entities(), on layers that are on and not frozen
/// (locked layers count): a polyline gives one curve per segment. Ellipses
/// and the generated shapes of inserts, dimensions and texts are not
/// candidates. A point beyond solid-space's domain gives NotEnclosed with no
/// free end, and `complete` false.
/// Precondition: finite coordinates.
[[nodiscard]] DetectedBoundary detectHatchBoundary(const Document& document, const Vec2& picked);

struct ChosenContours {
    /// One loop per entity taken, in the order of `chosen`, with its flags
    /// (point 4).
    std::vector<HatchLoop> loops;
    /// The entities that gave `loops`, in the same order.
    std::vector<EntityId> taken;
    /// The entities of `chosen` that give no loop, in their order: not a
    /// circle nor a closed polyline, not in the document, or left out by
    /// solid-space (nestContours, skipped).
    std::vector<EntityId> leftOut;
};

/// The contours chosen with Select objects (docs/adr/0048, point 6) and which
/// lies inside which (solid-space's nestContours). A Circle is taken as its
/// circle, a closed Polyline as its closed polyline; the layer of the entity
/// does not matter. Two contours in conflict add nothing to each other's
/// depth, so both stay in the hatch, with the even-odd rule.
[[nodiscard]] ChosenContours chosenHatchContours(const Document& document,
                                                 std::span<const EntityId> chosen);

}  // namespace tg::core
