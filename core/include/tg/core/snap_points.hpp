// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <cstdint>
#include <vector>

namespace tg::core {

enum class SnapKind : std::uint8_t { Endpoint, Midpoint, Center };

/// Which object snaps are enabled (docs/adr/0008).
struct SnapModes {
    bool endpoint{true};
    bool midpoint{true};
    bool center{true};
    friend constexpr bool operator==(const SnapModes&, const SnapModes&) = default;
};

struct SnapPoint {
    Vec2 point;
    SnapKind kind{SnapKind::Endpoint};
    friend constexpr bool operator==(const SnapPoint&, const SnapPoint&) = default;
};

/// Appends start/end, halfway along the curve, and arc centre, as enabled.
/// A zero-length segment contributes only its Endpoint.
void appendSnapPoints(const PolylineSegment& segment, const SnapModes& modes,
                      std::vector<SnapPoint>& out);

/// Appends the enabled snap points of an entity (docs/adr/0008, docs/adr/0012):
/// - Polyline: the points of its segments in segment order, each (position,
///   kind) once; existing output does not participate in deduplication, and
///   coincident points of different kinds retain their snap semantics;
/// - Line: start, end and midpoint (only the start if they coincide);
/// - Arc: start, end, midpoint halfway along the arc, centre;
/// - Circle: only the centre.
void appendSnapPoints(const Entity& entity, const SnapModes& modes, std::vector<SnapPoint>& out);

}  // namespace tg::core
