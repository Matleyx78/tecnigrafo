// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/document.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <optional>
#include <vector>

/// The questions that selection and object snap ask the drawing
/// (docs/adr/0007, docs/adr/0008). Each one takes its candidates from
/// Document::query() and then decides with the exact geometry of
/// proximity.hpp and snap_points.hpp, so the answers are the same as visiting
/// every entity. Layers that are off or frozen are ignored; locked layers
/// remain queryable.
namespace tg::core {

enum class RectangleMode : std::uint8_t { Window, Crossing };

/// The entity nearest to `point` within `tolerance` (drawing units), or
/// std::nullopt. At equal distance the highest id wins.
/// The distance is to the entity's curve, so a click inside a closed polyline
/// does not pick it; an empty polyline is never picked. A text is a region: a
/// click inside its box is at distance 0.
/// Precondition: tolerance >= 0.
[[nodiscard]] std::optional<EntityId> pickEntity(const Document& document, const Vec2& point,
                                                 double tolerance);

/// The entities selected by `rectangle`: Window takes those isInside(),
/// Crossing those isInside() or intersects(). Sorted by id.
/// An empty rectangle selects nothing.
[[nodiscard]] std::vector<EntityId> selectInRectangle(const Document& document,
                                                      const BBox& rectangle, RectangleMode mode);

/// The object snap for a pointer at `pointer` (docs/adr/0008, docs/adr/0033),
/// or std::nullopt. `aperture` is in drawing units.
///
/// Only segments within `aperture` of the pointer are considered; a polyline
/// with a single vertex counts as a zero-length segment, which offers only its
/// Endpoint. Among their enabled snap points within `aperture` of the pointer
/// (endpoints, midpoints, quadrants and perpendicular feet; the intersections
/// and tangent points are not given yet) the nearest to the pointer wins; at
/// equal distance the order is Endpoint, Intersection, Midpoint, Quadrant,
/// Perpendicular, Tangent (docs/adr/0033, point 5), then the highest id wins,
/// then the earliest segment. If there is none, the centre of the nearest
/// considered arc or the insertion point of the nearest considered text wins
/// (nearest by distance from the pointer to the arc or to the text box, not to
/// the snap point), with the same id and segment order on ties: a text is
/// considered when the pointer is within `aperture` of its box
/// (docs/adr/0013, point 6). With every mode disabled the result is always
/// std::nullopt.
///
/// `from` is the base point of the running command (Command::basePoint()):
/// the Perpendicular points start from it, and without it there are none. A
/// foot of a perpendicular competes only if it is within `aperture` of the
/// pointer, like every other point, not merely its curve.
/// Precondition: aperture >= 0.
[[nodiscard]] std::optional<SnapPoint> findSnap(const Document& document, const Vec2& pointer,
                                                double aperture, const SnapModes& modes,
                                                std::optional<Vec2> from = std::nullopt);

}  // namespace tg::core
