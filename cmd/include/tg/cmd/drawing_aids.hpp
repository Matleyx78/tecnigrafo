// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/snap_points.hpp"
#include "tg/core/vec2.hpp"

#include <optional>

namespace tg::cmd {

/// Object snap, ortho and polar tracking settings (docs/adr/0008,
/// docs/adr/0033). The defaults are those of a new session: object snap on
/// with every mode, ortho and polar off.
struct DrawingAids {
    bool objectSnap{true};  ///< F3: all object snaps on or off
    core::SnapModes snapModes;
    bool ortho{false};  ///< F8
    /// F10. Ortho and polar are never both true: whoever switches one on
    /// switches the other off (the interface, the script directives). If
    /// both are true anyway, ortho is the one that applies.
    bool polar{false};
    friend bool operator==(const DrawingAids&, const DrawingAids&) = default;
};

/// `pointer` moved onto the horizontal or the vertical through `lastPoint`,
/// whichever is nearer to the direction of the pointer (horizontal when
/// the two are equally near).
[[nodiscard]] core::Vec2 orthoPoint(const core::Vec2& lastPoint, const core::Vec2& pointer);

/// The point on the polar ray from `base` nearest to `pointer`, among the
/// rays at the multiples of `increment` (radians), if `pointer` is within
/// `tolerance` of it; otherwise std::nullopt (docs/adr/0033, point 9).
///
/// The rays leave `base` at the angles k * increment, for every integer k
/// with |k * increment| <= pi, counter-clockwise from the X axis of the
/// drawing: with an increment that does not divide 360 degrees they are
/// symmetric about the X axis. The distance is measured from the ray, not
/// from its line, so the ray behind the pointer never wins. When two rays are
/// equally near, the one clockwise from the pointer wins. A pointer on `base`
/// gives `base`. Preconditions: increment > 0, tolerance >= 0.
[[nodiscard]] std::optional<core::Vec2> polarPoint(const core::Vec2& base,
                                                   const core::Vec2& pointer, double increment,
                                                   double tolerance);

}  // namespace tg::cmd
