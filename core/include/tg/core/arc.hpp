// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/arc_geometry.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/vec2.hpp"

#include <optional>

namespace tg::core {

/// A circular arc (docs/adr/0012), as the DXF ARC entity: centre 10/20,
/// radius 40, start angle 50, end angle 51, always counter-clockwise from
/// start to end. Angles in radians, in [0, 2π), never equal (the DXF file
/// and the control-mode report write them in degrees). The DXF properties
/// that a plane drawing does not use (Z 30, thickness 39, extrusion
/// 210/220/230) have their default values (docs/adr/0012, point 2).
/// Invariant: radius > 0.
struct Arc {
    Vec2 center;
    double radius{0.0};
    double startAngle{0.0};
    double endAngle{0.0};

    /// The arc with its sweep, in (0, 2π).
    [[nodiscard]] ArcGeometry geometry() const;
    [[nodiscard]] double length() const;
    [[nodiscard]] BBox boundingBox() const;

    friend constexpr bool operator==(const Arc&, const Arc&) = default;
};

/// Arc from `start` through `middle` to `end`, stored counter-clockwise
/// as DXF wants, whatever the direction of the three points: a clockwise
/// arc has its end points swapped. std::nullopt if the points are collinear
/// by the criterion of docs/adr/0012 (coincident points included).
[[nodiscard]] std::optional<Arc> arcThroughPoints(const Vec2& start, const Vec2& middle,
                                                  const Vec2& end);

}  // namespace tg::core
