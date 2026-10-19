// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/vec2.hpp"

#include <vector>

/// Geometry shared by every circular curve of the model: the arc segments of
/// polylines, Arc and Circle (docs/adr/0012, point 3). A circle is an
/// ArcGeometry with a sweep of 2π.
namespace tg::core {

/// A circular arc: the points of the circle (center, radius) from
/// `startAngle` through a signed `sweep`. Angles in radians,
/// counter-clockwise from +X.
struct ArcGeometry {
    Vec2 center;
    double radius{0.0};
    double startAngle{0.0};
    double sweep{0.0};  ///< > 0 counter-clockwise, < 0 clockwise; |sweep| <= 2π

    friend constexpr bool operator==(const ArcGeometry&, const ArcGeometry&) = default;
};

/// `radians` brought into [0, 2π).
[[nodiscard]] double normalizeAngle(double radians);

/// The point of the arc's circle at `angle` (radians).
[[nodiscard]] Vec2 pointAt(const ArcGeometry& arc, double angle);

/// The first and the last point of the arc, computed from its angles.
[[nodiscard]] Vec2 startPoint(const ArcGeometry& arc);
[[nodiscard]] Vec2 endPoint(const ArcGeometry& arc);

/// True if the direction `angle` (radians, any value) falls within the swept
/// interval, with an allowance for the roundoff of computing the angle of a
/// point (atan2). A sweep of 2π contains every angle.
[[nodiscard]] bool containsAngle(const ArcGeometry& arc, double angle);

/// Exact bounding box: the end points and the axis-aligned extremes that the
/// arc passes through.
[[nodiscard]] BBox boundingBox(const ArcGeometry& arc);

/// Grows `box` by the axis-aligned extremes (0°, 90°, 180°, 270°) that the
/// arc passes through, not by its end points: a polyline segment adds its
/// exact vertices itself.
void expandByAxisExtremes(const ArcGeometry& arc, BBox& box);

/// Appends the points approximating the arc to `out`, excluding the start
/// point and including the end point. `maxChordError` is the largest
/// allowed distance between the arc and its approximation.
void tessellate(const ArcGeometry& arc, double maxChordError, std::vector<Vec2>& out);

/// The point of the arc nearest to `point`. Outside the swept interval, the
/// nearer end point; from the centre, where every point is as near, the start
/// point. Ties between the end points prefer the start point.
[[nodiscard]] Vec2 closestPoint(const ArcGeometry& arc, const Vec2& point);

/// True if the arc touches the rectangle, borders included: an end point
/// inside, or a crossing of one of its sides. An arc that surrounds the
/// rectangle without touching it does not intersect it.
[[nodiscard]] bool intersects(const ArcGeometry& arc, const BBox& rectangle);

/// True if the three points are collinear by the criterion of docs/adr/0012,
/// point 4: |(b − a) × (c − a)| <= 1e-9 · L², L the longest side. Coincident
/// points are collinear.
[[nodiscard]] bool areCollinear(const Vec2& a, const Vec2& b, const Vec2& c);

}  // namespace tg::core
