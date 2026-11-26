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

/// What the guides of `acquisitions` (oldest first) make of `pointer`
/// (docs/adr/0049, points 3-5; docs/adr/0057, point 4;
/// docs/tappe/0.8/PT34-otrack-cmd.md, point 3;
/// docs/tappe/0.8.1/PT31-prolungamento-parallelo.md, point 2). Only the guide
/// fields of the result are set; Guide::None if the pointer is not within
/// `aperture` of any guide.
///
/// - The guides: every point gives the horizontal y = py and the vertical
///   x = px; every extension its ray beyond the end of a line or straight
///   segment, or the part of the circle of an arc beyond its end; every
///   direction, with a `base` point, the line through `base` with that
///   direction. Within one acquisition they come in this order, which also
///   decides ties: extension, horizontal, vertical, parallel.
/// - A guide catches the pointer within `aperture` of it, edge included: the
///   horizontal and the vertical by |dy| and |dx|; a straight guide by the
///   foot of the perpendicular, at a squared distance within aperture²,
///   never behind the end of an extension; an arc extension when the pointer
///   is not its centre, its distance from the centre differs from the radius
///   by at most `aperture` and its direction falls in the part beyond the
///   end (core::containsAngle()). The point is the foot, or for an arc the
///   point of the circle along the radius through the pointer.
/// - Two guides of different acquisitions that catch the pointer give their
///   crossing: a horizontal and a vertical the exact one (the x of the
///   vertical, the y of the horizontal); other straight guides the one
///   solid-space finds on the extended lines (core::Contact::Crossing), not
///   behind the end of an extension; an arc extension and a straight guide
///   those on the part of the circle beyond the end; two arcs none. With
///   `polar`, a guide that catches the pointer gives its crossings with the
///   ray in the same way, if they are on the ray. Crossings win over single
///   guides; the nearest crossing wins; a crossing with the polar ray wins
///   only if it is nearer than every crossing of two guides.
/// - Otherwise the nearest guide wins, and the pointer goes onto it.
///
/// Distances are compared squared, dx*dx + dy*dy in double, without roots
/// (docs/REGOLE-NUMERICHE.md, section 3, point 1). Ties (section 4,
/// "Tracciamento degli snap"):
/// - two crossings of horizontals and verticals of points only: the one
///   using the horizontal of the most recent point, then the vertical of the
///   most recent point (point 3);
/// - two other crossings: the one whose more recent acquisition is the more
///   recent; then the one whose other acquisition is; then by their guides,
///   acquisition by acquisition from the more recent, in the order above;
///   with the same two guides (an arc extension crossing a line twice), the
///   first along the extension from its end (point 6);
/// - two crossings with the polar ray, and two single guides: the guide of
///   the more recent acquisition, then the order above, then the first along
///   an arc extension (points 1, 2, 5 and 6).
/// Precondition: aperture >= 0.
[[nodiscard]] CommandRunner::TrackingState trackPoint(
    std::span<const CommandRunner::TrackingState::Acquisition> acquisitions,
    const std::optional<core::Vec2>& base, const core::Vec2& pointer, double aperture,
    const std::optional<PolarRay>& polar);

/// The point at `distance` along the single guide of `state`
/// (docs/adr/0049, point 6; docs/adr/0057, point 4); std::nullopt with no
/// single guide, or where the guide gives no point:
/// - horizontal, vertical, parallel: from the point acquired, or the base
///   point, towards the point the guide gives; none when that is the point
///   itself, which shows no direction;
/// - extension of a line or straight segment: from its end, along the ray;
///   none for a negative distance, which falls behind the end;
/// - extension of an arc: along the arc, at the angle distance / radius
///   from its end, with core::sinCos(); none for a negative distance or an
///   angle beyond the part of the circle beyond the end (the other end
///   included). 0 gives the end itself.
[[nodiscard]] std::optional<core::Vec2> pointAlongGuide(const CommandRunner::TrackingState& state,
                                                        double distance);

}  // namespace tg::cmd::detail
