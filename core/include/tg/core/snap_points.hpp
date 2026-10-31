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
/// docs/adr/0033, point 6). Perpendicular, tangent and quadrant start off.
struct SnapModes {
    bool endpoint{true};
    bool midpoint{true};
    bool center{true};
    bool insertion{true};
    bool intersection{true};  ///< no snap point is given yet: package P15 of 0.7
    bool perpendicular{false};
    bool tangent{false};  ///< no snap point is given yet: package P19 of 0.7
    bool quadrant{false};
    friend constexpr bool operator==(const SnapModes&, const SnapModes&) = default;
};

struct SnapPoint {
    Vec2 point;
    SnapKind kind{SnapKind::Endpoint};
    friend constexpr bool operator==(const SnapPoint&, const SnapPoint&) = default;
};

/// Appends start/end, halfway along the curve, arc centre, and the quadrants
/// of an arc, as enabled. A zero-length segment contributes only its
/// Endpoint.
///
/// `from` is the base point of the running command (docs/adr/0033, point 4):
/// only with it, and with `modes.perpendicular`, the foot of the
/// perpendicular from `from` to the segment is appended too, if it falls on
/// the segment and not on its extension.
void appendSnapPoints(const PolylineSegment& segment, const SnapModes& modes,
                      std::vector<SnapPoint>& out, const std::optional<Vec2>& from = std::nullopt);

/// Appends the enabled snap points of an entity (docs/adr/0008, docs/adr/0012,
/// docs/adr/0013, docs/adr/0033):
/// - Polyline: the points of its segments in segment order, each (position,
///   kind) once; existing output does not participate in deduplication, and
///   coincident points of different kinds retain their snap semantics;
/// - Line: start, end and midpoint (only the start if they coincide);
/// - Arc: start, end, midpoint halfway along the arc, centre, and the
///   quadrants inside the arc (the 0°, 90°, 180° and 270° points of its
///   circle, end points included when they are one of them);
/// - Circle: the centre and the four quadrants;
/// - Text: only the insertion point, its position.
///
/// The Perpendicular points need `from` (see the segment overload). A line
/// has one, at most; a circle or arc has the two points of its circle on the
/// line through the centre and `from` that fall on it, none if `from` is the
/// centre, where every direction is perpendicular; a text has none.
/// Intersection and Tangent points are not appended yet.
void appendSnapPoints(const Entity& entity, const SnapModes& modes, std::vector<SnapPoint>& out,
                      const std::optional<Vec2>& from = std::nullopt);

}  // namespace tg::core
