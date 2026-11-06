// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/fillet.hpp"

#include <variant>

namespace tg::core {

struct ChamferDistances {
    double first{0.0};
    double second{0.0};
};

struct ChamferAngle {
    double length{0.0};
    double angle{0.0};  ///< on the first line, in radians
};

using ChamferSpec = std::variant<ChamferDistances, ChamferAngle>;

/// Chamfer of two lines. Other entities give NotAllowed. Parallel lines give
/// None, a corner beyond solid-space's domain gives OutOfDomain, and a pick
/// projected onto the corner gives AmbiguousPick. A cut beyond the kept end
/// gives DistanceTooLarge. With Angle, an oversized first length takes
/// precedence over an absent second cut (P21, #525).
/// For Angle, let L be the first length, a the reduced turn, b the opening
/// minus a, and s = L*sin(a)/sin(b) the second distance. Its first-order
/// error estimate is a few ulps of s plus
/// L*abs(cos(a)/sin(b))*da + abs(s*cos(b)/sin(b))*db,
/// where da and db are the angle rounding errors, including the construction
/// of the opening from the directions. Near parallelism this estimate is
/// ill-conditioned. At the kept end within the construction error (including
/// rounding of the kept length), the part may be erased or the result may be
/// DistanceTooLarge (P21, #508).
/// With trim off, both lines stay as they are and any new line goes in added.
/// Precondition for all three functions: spec is finite, lengths and distances >= 0.
[[nodiscard]] CornerResult chamfer(const Entity& first, const Vec2& pickFirst, const Entity& second,
                                   const Vec2& pickSecond, const ChamferSpec& spec, bool trim,
                                   const ToleranceRule& rule);

/// Chamfer of two consecutive straight segments of one polyline. With trim
/// the chamfer is built into the polyline; without, the line goes in added.
[[nodiscard]] CornerResult chamferSegments(const Polyline& polyline, const Vec2& pickFirst,
                                           const Vec2& pickSecond, const ChamferSpec& spec,
                                           bool trim, const ToleranceRule& rule);

/// Visit original corners in order, on the updated polyline with trim or on
/// the original segments without it. Unsuccessful corners count in tooShort.
[[nodiscard]] PolylineCorners chamferPolyline(const Polyline& polyline, const ChamferSpec& spec,
                                              bool trim, const ToleranceRule& rule);

}  // namespace tg::core
