// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_runner.hpp"
#include "tg/core/vec2.hpp"

#include <optional>
#include <span>

namespace tg::cmd::detail {

/// The polar ray the pointer is on: from `base` through `through`, which is
/// a different point.
struct PolarRay {
    core::Vec2 base;
    core::Vec2 through;
};

/// What the guides of `acquired` (oldest first) make of `pointer`
/// (docs/adr/0049, points 3-5; docs/tappe/0.8/PT34-otrack-cmd.md, point 3).
/// Only the guide fields of the result are set; Guide::None if the pointer
/// is not within `aperture` of any guide.
///
/// - Every acquired point gives the horizontal y = py and the vertical
///   x = px. A guide catches the pointer within `aperture` of it, edge
///   included.
/// - Two guides of different points that catch the pointer give their
///   exact crossing (the x of the vertical, the y of the horizontal). With
///   `polar`, a guide that catches the pointer gives its crossing with the
///   ray, as solid-space finds it on the extended lines, if it is on the
///   ray. Crossings win over single guides; the nearest crossing wins.
/// - Otherwise the nearest guide wins, and the pointer goes onto it.
///
/// Distances are compared squared, dx*dx + dy*dy in double, without roots.
/// Ties: between two guide crossings, the one using the horizontal of the
/// most recent point, then the vertical of the most recent point; a guide
/// crossing beats a crossing with the polar ray; between two crossings with
/// the polar ray, and between two single guides, the guide of the most
/// recent point; between the horizontal and the vertical of one point, the
/// horizontal. Precondition: aperture >= 0.
[[nodiscard]] CommandRunner::TrackingState trackPoint(std::span<const core::Vec2> acquired,
                                                      const core::Vec2& pointer, double aperture,
                                                      const std::optional<PolarRay>& polar);

/// The point at `distance` from the acquired point of the single guide of
/// `state` (Horizontal or Vertical), along it, towards the point the guide
/// gives (docs/adr/0049, point 6); std::nullopt with no single guide, or
/// when the guide gives its own acquired point, which shows no direction.
[[nodiscard]] std::optional<core::Vec2> pointAlongGuide(const CommandRunner::TrackingState& state,
                                                        double distance);

}  // namespace tg::cmd::detail
