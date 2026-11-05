// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

// Fillets, tangent circles and tangent points (docs/adr/0035, points 13 and
// 14), decided and built by solid-space (solid_space/geom2d/tangency.hpp, its
// ADR 0011). This is a facade: it only translates. Whether a fillet exists,
// on which side, where it touches each curve, the direction of its arc and
// the part of each curve it keeps all come from solid-space; Tecnigrafo adds
// no geometric decision of its own.

#include "tg/core/circle.hpp"
#include "tg/core/document.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/tolerance.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace tg::core {

/// How a corner construction (FILLET, CHAMFER) ended (docs/adr/0035).
enum class CornerOutcome : std::uint8_t {
    Done,
    None,            ///< it does not exist (solid-space: none)
    InfinitelyMany,  ///< the curves lie on one line or circle, or similar
    AmbiguousPick,   ///< a pick lies exactly on the other curve's line or circle
    /// FILLET only (fillet, filletSegments): the kept part would turn a line
    /// back past the end it keeps (radiusTooLarge, tangency.hpp, rule 5).
    RadiusTooLarge,
    /// CHAMFER only (chamfer, chamferSegments; P21): a distance, or the
    /// length, longer than the kept part of its line, or with an angle a
    /// second point beyond the kept part of the second line.
    DistanceTooLarge,
    NotAllowed,  ///< an entity the command does not take
    /// It exists, but a construction is beyond solid-space's domain: the
    /// outOfDomain diagnosis on the radius of fillet and tangentCircleNear
    /// (tangency.hpp), or for CHAMFER a corner that intersect() omits.
    OutOfDomain,
};

/// The result of FILLET and CHAMFER on two entities.
struct CornerResult {
    CornerOutcome outcome{CornerOutcome::None};
    /// With Done: the two entities as they become (unchanged with trim off,
    /// and a circle is never cut), and the new arc or line, none for a
    /// corner of radius 0. With trim on, an empty `first` or `second` means
    /// that entity is erased: its kept part is empty (partOf gives Empty) or
    /// degenerate for the rule of the document (docs/adr/0036, point 5). In
    /// the results on one polyline, `first` is the polyline, never empty, and
    /// `second` is always empty without meaning "erased".
    std::optional<Entity> first;
    std::optional<Entity> second;
    std::optional<Entity> added;
};

/// FILLET of two different entities, each a Line, an Arc or a Circle, picked
/// at the given points (docs/adr/0035, point 13). Radius 0 gives the corner.
/// Anything else:
/// NotAllowed. A radius beyond robust::kMaxCoordinate: None
/// (docs/adr/0035, point 23). Precondition: radius finite and >= 0.
///
/// With trim, each line and arc keeps the part that solid-space's fillet
/// gives (Fillet::keepFirst, Fillet::keepSecond): cut with partOf or extended
/// with extendTo, as the position of its point of tangency says. A line whose
/// kept part lies wholly on its extension (possible only without a fillet
/// arc, tangency.hpp, rule 5) keeps its kept end and gets the point of
/// tangency as the other end. A kept part that the model cannot hold
/// (NotRepresentable) gives None, and no entity changes. A fillet arc that
/// the rule finds degenerate, or whose bulge is 0, gives the result of
/// radius 0 (docs/adr/0036, point 5).
[[nodiscard]] CornerResult fillet(const Entity& first, const Vec2& pickFirst, const Entity& second,
                                  const Vec2& pickSecond, double radius, bool trim,
                                  const ToleranceRule& rule);

/// FILLET of two segments of one polyline, each the segment nearest its pick
/// (docs/adr/0035, point 13). With `trim`: `first` of the result is the new
/// polyline, with the curved segment in the corner, and `added` is empty.
/// Without: `first` is the polyline unchanged, and `added` the arc of the
/// corner (none for radius 0 or a degenerate arc, docs/adr/0036, point 5).
/// `second` is always empty. Two segments that are not consecutive (also
/// across the closing segment): NotAllowed.
///
/// Only straight segments are filleted: FILLET on the arcs of a polyline is
/// left out (docs/adr/0035, "Alternative considerate"), so a curved segment
/// gives NotAllowed. A radius beyond robust::kMaxCoordinate: None.
/// Precondition: radius finite and >= 0.
///
/// With `trim` the common vertex V of the two segments is replaced by the two
/// points of tangency, the first carrying Fillet::bulge (one point, the
/// corner, without an arc). The kept parts must be the part of the earlier
/// segment that reaches V and the part of the later one that leaves it, as
/// solid-space gives them for two consecutive segments; any other pair
/// cannot be one polyline and gives None. A kept segment that the rule finds
/// degenerate is not created: its far vertex takes the point of tangency
/// (docs/adr/0036, point 5). The widths of a cut segment are interpolated by
/// length; the new curved segment runs from the width of the earlier segment
/// at its point of tangency to that of the later one at its own.
[[nodiscard]] CornerResult filletSegments(const Polyline& polyline, const Vec2& pickFirst,
                                          const Vec2& pickSecond, double radius, bool trim,
                                          const ToleranceRule& rule);

/// FILLET Polyline: every original vertex in order (docs/adr/0035, point 13).
/// With `trim`, each vertex on the polyline as the vertices before it left
/// it, and `polyline` is the result. Without, the polyline does not change,
/// each vertex is judged on the original segments, and the new arcs go in
/// `added`, in the order of the visit.
///
/// Each vertex is filleted as filletSegments does on its two segments, with
/// the far ends of the two segments as the picks; any outcome other than
/// Done (a curved segment, a point of tangency outside a segment, no fillet,
/// RadiusTooLarge, OutOfDomain and the others) leaves the vertex as it is and
/// counts it in `tooShort` (docs/verifica/domande.md, "Raccordi e
/// tangenze"). So every visited vertex counts in `done` or in `tooShort`.
struct PolylineCorners {
    Polyline polyline;
    /// With trim off: the new arcs (for CHAMFER, lines). Empty with trim on.
    std::vector<Entity> added;
    std::size_t done{0};      ///< vertices filleted
    std::size_t tooShort{0};  ///< vertices whose segments are too short
};

[[nodiscard]] PolylineCorners filletPolyline(const Polyline& polyline, double radius, bool trim,
                                             const ToleranceRule& rule);

/// CIRCLE Ttr (docs/adr/0035, point 14): the circle of `radius` tangent to
/// both, nearest the picks; for a polyline its segment nearest the pick. A
/// text: NotAllowed. A radius beyond robust::kMaxCoordinate: None
/// (docs/adr/0035, point 23).
struct TangentCircleResult {
    CornerOutcome outcome{CornerOutcome::None};
    std::optional<Circle> circle;  ///< with Done
};

[[nodiscard]] TangentCircleResult circleTangentTangentRadius(const Entity& first,
                                                             const Vec2& pickFirst,
                                                             const Entity& second,
                                                             const Vec2& pickSecond, double radius);

/// The tangent points from `point` to a circle, an arc or the arcs of a
/// polyline, as tangentsFrom gives them with Extension::none (tangency.hpp);
/// none for a straight curve. For a polyline: by segment index, then in the
/// order tangentsFrom gives on the segment; a point AtEnd of a segment and
/// AtStart of the one that follows (for a closed polyline also the last and segment 0)
/// is listed once, on the segment that follows; a segment solid-space refuses,
/// or whose construction is beyond its domain, gives no points, and the
/// others keep theirs.
[[nodiscard]] std::vector<Vec2> tangentPoints(const Vec2& point, const Entity& entity);

}  // namespace tg::core
