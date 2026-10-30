// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace tg::core {

enum class SnapKind : std::uint8_t {
    Endpoint,
    Midpoint,
    Center,
    Insertion,
    Intersection,
    Perpendicular,
    Tangent,
    Quadrant
};

/// Which object snaps are enabled (docs/adr/0008, docs/adr/0013,
/// docs/adr/0033, point 6).
struct SnapModes {
    bool endpoint{true};
    bool midpoint{true};
    bool center{true};
    bool insertion{true};
    bool intersection{true};  // docs/adr/0033, point 6
    bool perpendicular{false};
    bool tangent{false};
    bool quadrant{false};
    friend constexpr bool operator==(const SnapModes&, const SnapModes&) = default;
};

struct SnapPoint {
    Vec2 point;
    SnapKind kind{SnapKind::Endpoint};
    friend constexpr bool operator==(const SnapPoint&, const SnapPoint&) = default;
};

/// Appends start/end, halfway along the curve, arc centre, the quadrants of
/// an arc and the feet of the perpendiculars from `from`, as enabled. A
/// zero-length segment contributes only its Endpoint. `from` is the base
/// point of the running command (docs/adr/0033, point 4): without it
/// Perpendicular gives nothing. Intersection and Tangent give nothing yet.
void appendSnapPoints(const PolylineSegment& segment, const SnapModes& modes,
                      std::vector<SnapPoint>& out, std::optional<Vec2> from = std::nullopt);

/// Appends the enabled snap points of an entity (docs/adr/0008, docs/adr/0012,
/// docs/adr/0013, docs/adr/0033):
/// - Polyline: the points of its segments in segment order, each (position,
///   kind) once; existing output does not participate in deduplication, and
///   coincident points of different kinds retain their snap semantics;
/// - Line: start, end and midpoint (only the start if they coincide);
/// - Arc: start, end, midpoint halfway along the arc, centre;
/// - Circle: only the centre;
/// - Text: only the insertion point, its position.
/// Quadrant adds, for circles, arcs and the arcs of polylines, the points at
/// 0, 90, 180 and 270 degrees of the circle, in this order, those within the
/// arc included (containsAngle(): an end point on a quadrant counts, as in
/// AutoCAD). Perpendicular adds the feet of the perpendiculars from `from`
/// to the curve, only where they fall on it: on a straight segment, the
/// foot; on a circle or an arc, the two points of the circle on the line
/// through its centre and `from`, the nearer one first.
void appendSnapPoints(const Entity& entity, const SnapModes& modes, std::vector<SnapPoint>& out,
                      std::optional<Vec2> from = std::nullopt);

}  // namespace tg::core
