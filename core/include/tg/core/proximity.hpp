// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

namespace tg::core {

/// Point of segment nearest to point. For an arc, only its swept interval
/// participates, never the rest of its circle. Ties prefer the start point.
[[nodiscard]] Vec2 closestPoint(const PolylineSegment& segment, const Vec2& point);

[[nodiscard]] double distance(const PolylineSegment& segment, const Vec2& point);

/// Distance to the entity's curve, not its interior. A single vertex is a
/// point; an empty polyline has infinite distance.
[[nodiscard]] double distance(const Entity& entity, const Vec2& point);

/// Window selection, borders included. An empty entity or rectangle is false.
[[nodiscard]] bool isInside(const Entity& entity, const BBox& rectangle);

/// Crossing selection, borders included. A closed curve surrounding the
/// rectangle without touching it is false. Point/line rectangles are supported.
[[nodiscard]] bool intersects(const Entity& entity, const BBox& rectangle);

}  // namespace tg::core
