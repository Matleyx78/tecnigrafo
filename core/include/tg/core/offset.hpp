// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

// The parallel curve of an entity, as OFFSET makes it (docs/adr/0035, points
// 10-12), decided and built by solid-space (solid_space/geom2d/offset.hpp,
// its ADR 0012). This is a facade: it only translates. Which side, whether
// the copy vanishes, where the joints are and whether the result is valid
// all come from solid-space; Tecnigrafo adds no threshold of its own and
// promises no more than solid-space does ("What is guaranteed" in
// offset.hpp of solid-space).

#include "tg/core/document.hpp"
#include "tg/core/tolerance.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <optional>

namespace tg::core {

enum class OffsetOutcome : std::uint8_t {
    Done,      ///< `entity` holds the copy
    Vanished,  ///< nothing is left at that distance
    Failed,    ///< solid-space could not make the copy (its outcome failed)
    NoSide,    ///< the side point lies exactly on the line or circle
};

struct OffsetResult {
    OffsetOutcome outcome{OffsetOutcome::Failed};
    std::optional<Entity> entity;  ///< with Done: the same type as the input
    /// With Done for a polyline: solid-space's PolylineOffset::simple.
    bool simple{false};
};

/// OFFSET (docs/adr/0035, points 10-12): the curve at `distance` (> 0) from
/// `entity`, on the side of `sidePoint`.
///
/// The side is sideOf(entity, sidePoint): Left gives the signed distance
/// +distance, Right -distance, for a polyline along the direction of its
/// vertices (the side is the one of its segment nearest to the point). On
/// gives NoSide; no side at all (a curve that is exactly a point, a polyline
/// without segments or with only segments that are exactly a point, data
/// solid-space refuses) gives Failed. In neither case is solid-space asked
/// for the copy.
///
/// - Line, arc, circle: offsetCurve. The copy has the curve solid-space gives;
///   an arc keeps the start and end angles of the input, bit for bit, with
///   the new radius.
/// - Polyline: offsetPolyline with the rule `rule`. The copy is open or closed
///   as the input, with the positions and bulges solid-space gives. Each
///   segment that comes from a segment of the input (PieceOrigin::source)
///   takes its start and end widths; a joint arc (PieceOrigin::gap) has
///   widths 0. The last vertex of an open copy has widths 0. Constant width,
///   linetype generation and every other property stay those of the input.
///
/// vanished gives Vanished, failed (whatever the reason) and any diagnosis of
/// solid-space give Failed.
/// Precondition: not a Text; distance finite and > 0.
[[nodiscard]] OffsetResult offsetEntity(const Entity& entity, double distance,
                                        const Vec2& sidePoint, const ToleranceRule& rule);

}  // namespace tg::core
