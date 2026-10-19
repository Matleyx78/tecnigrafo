// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/arc_geometry.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/vec2.hpp"

#include <optional>

namespace tg::core {

/// A full circle (docs/adr/0012), as the DXF CIRCLE entity: centre 10/20,
/// radius 40. The DXF properties that a plane drawing does not use (Z 30,
/// thickness 39, extrusion 210/220/230) have their default values
/// (docs/adr/0012, point 2). Invariant: radius > 0.
struct Circle {
    Vec2 center;
    double radius{0.0};

    /// The circle as an arc that starts at 0 and sweeps 2π.
    [[nodiscard]] ArcGeometry geometry() const;
    /// The circumference.
    [[nodiscard]] double length() const;
    [[nodiscard]] BBox boundingBox() const;

    friend constexpr bool operator==(const Circle&, const Circle&) = default;
};

/// Circle through three points, or std::nullopt if they are collinear by
/// the criterion of docs/adr/0012 (coincident points included).
[[nodiscard]] std::optional<Circle> circleThroughPoints(const Vec2& a, const Vec2& b,
                                                        const Vec2& c);

}  // namespace tg::core
