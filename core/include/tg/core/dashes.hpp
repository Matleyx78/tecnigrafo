// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/core/linetype_pattern.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

namespace tg::core {

struct DashLine {
    Vec2 start;
    Vec2 end;
};

struct DashArc {
    Vec2 center;
    double radius;
    double startAngle;
    double endAngle;
};  ///< Counter-clockwise, angles in radians.

struct DashPoint {
    Vec2 position;
    Vec2 direction;
};  ///< Unit tangent in the direction of travel.

using DashPiece = std::variant<DashLine, DashArc, DashPoint>;

/// More pieces than this and the entity is drawn continuous (ADR 0025, point 5).
inline constexpr std::size_t kMaxDashPieces = 100'000;

/// Pieces in drawing coordinates, in order along the entity (ADR 0025, point 4).
/// nullopt means continuous: an empty pattern, TEXT, a curve shorter than one
/// scaled pattern (every segment for a polyline without continuous generation),
/// or more than kMaxDashPieces. Zero-length and unrepresentable scaled patterns
/// also fall back to continuous. An empty vector means the curve falls in gaps.
/// At an internal vertex a dot uses the outgoing tangent; at the end, the incoming
/// tangent. Clockwise bulges produce counter-clockwise DashArcs with swapped ends.
/// Precondition: scale is finite and positive; pattern elements are finite.
[[nodiscard]] std::optional<std::vector<DashPiece>> dashes(const Entity& entity,
                                                        const LinetypePattern& pattern,
                                                        double scale);

}  // namespace tg::core
