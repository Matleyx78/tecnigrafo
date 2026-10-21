// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

namespace tg::core {

/// Point of segment nearest to point. For an arc, only its swept interval
/// participates, never the rest of its circle. Ties prefer the start point.
[[nodiscard]] Vec2 closestPoint(const PolylineSegment& segment, const Vec2& point);

[[nodiscard]] double distance(const PolylineSegment& segment, const Vec2& point);

/// Point of the text box (textBox()) nearest to point. The box is a region:
/// a point inside it, borders included, is its own nearest point.
[[nodiscard]] Vec2 closestPoint(const Text& text, const Vec2& point);

/// Distance to the entity's curve, not its interior. A single vertex is a
/// point; an empty polyline has infinite distance. A text is a region
/// (docs/adr/0013, point 6): the distance is to its box, 0 inside it.
[[nodiscard]] double distance(const Entity& entity, const Vec2& point);

/// Window selection, borders included. An empty entity or rectangle is false.
[[nodiscard]] bool isInside(const Entity& entity, const BBox& rectangle);

/// Crossing selection, borders included. A closed curve surrounding the
/// rectangle without touching it is false, while a text box surrounding it is
/// true, because a text is a region. Point/line rectangles are supported.
[[nodiscard]] bool intersects(const Entity& entity, const BBox& rectangle);

}  // namespace tg::core
