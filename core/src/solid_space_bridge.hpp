// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

// The translation between the entities of Tecnigrafo and the curves of
// solid-space (docs/adr/0035, point 1). Internal to core: no header outside
// core/src includes it, so that no other module sees solid-space.

#include "solid_space/geom2d/curves.hpp"
#include "solid_space/robust/tolerance.hpp"
#include "tg/core/document.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/tolerance.hpp"

#include <optional>
#include <vector>

namespace tg::core::bridge {
namespace g = solid_space::geom2d;

/// The curve of a Line, Arc or Circle (docs/adr/0035, point 1). An Arc gets
/// sweep = endAngle - startAngle, plus g::kFullTurn when not positive,
/// computed in double in this order. std::nullopt for a Polyline or a Text.
[[nodiscard]] std::optional<g::Curve> curveOf(const Entity& entity);

/// The piece of one polyline segment: its bulge, or 0 when
/// PolylineSegment::isArc() is false (docs/adr/0036, point 7).
[[nodiscard]] g::PolylineSegment pieceOf(const PolylineSegment& segment);

/// The pieces of a polyline (g::pieces), one per Polyline::segment(), in the
/// same order. A segment whose PolylineSegment::isArc() is false gets bulge 0
/// (docs/adr/0036, point 7).
[[nodiscard]] std::vector<g::PolylineSegment> piecesOf(const Polyline& polyline);

/// solid-space's tolerance rule for a ToleranceRule.
[[nodiscard]] solid_space::robust::TolerancePolicy policyOf(const ToleranceRule& rule);

/// The point of solid-space for a point of Tecnigrafo, and back.
[[nodiscard]] g::Point2 toPoint(const Vec2& point);
[[nodiscard]] Vec2 fromPoint(const g::Point2& point);

}  // namespace tg::core::bridge
