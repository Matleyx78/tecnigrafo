// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <numbers>

namespace tg::core {

inline constexpr double kPi = std::numbers::pi;
inline constexpr double kTwoPi = 2.0 * std::numbers::pi;

[[nodiscard]] constexpr double degreesToRadians(double degrees) {
    return degrees * kPi / 180.0;
}

[[nodiscard]] constexpr double radiansToDegrees(double radians) {
    return radians * 180.0 / kPi;
}

/// Sine and cosine of an angle in radians, the same bits on every
/// platform (solid_space::robust::sinCos, docs/adr/0044, point 8).
struct SinCos {
    double sine{0.0};
    double cosine{1.0};
};

/// Precondition: finite angle. Angles beyond robust's range are first reduced
/// with std::fmod by 2*pi.
[[nodiscard]] SinCos sinCos(double angle);
[[nodiscard]] double sin(double angle);  ///< sinCos(angle).sine
[[nodiscard]] double cos(double angle);  ///< sinCos(angle).cosine

/// robust::atan2; precondition: finite arguments.
[[nodiscard]] double atan2(double y, double x);
/// atan2(x, 1.0); precondition: finite x.
[[nodiscard]] double atan(double x);
/// Below this magnitude a component of cardinalSinCos is taken as 0:
/// above the rounding left by a quarter turn given as a double (at most
/// about 2^-50 for angles up to a few turns), far below the relative
/// tolerance 2^-42 of solid_space::robust::TolerancePolicy.
inline constexpr double kCardinalSnap = 0x1p-48;
/// sinCos(angle), except that a component below kCardinalSnap in
/// magnitude becomes exactly 0 and the other exactly +1 or -1, with its
/// sign: quarter turns are exact (docs/DECISIONI.md, 2026-11-12, "Quarti
/// di giro esatti"). For rotations and directions, not for evaluating
/// sine and cosine as numbers.
///
/// Band (solid-space ADR 0013, point 4): an angle within kCardinalSnap of
/// a quarter turn is taken as that quarter turn. A point at distance r
/// from the centre then moves by at most about r * 2^-48, so the matrix
/// and the angle kept by a Similarity, a composition and its two
/// applications, a reflection and the axis given by two points agree
/// within the tolerance, not bit for bit. Beyond a few tens of radians a
/// quarter turn may not be recognised: the result is then sinCos. Angles
/// given in degrees stay within that range when they enter through
/// reducedDegreesToRadians.
[[nodiscard]] SinCos cardinalSinCos(double angle);
/// A finite angle given in degrees, reduced to [0, 360) with std::fmod (exact;
/// a negative remainder gets 360 added, and a result that rounds to 360
/// or is -0 becomes +0), then converted with degreesToRadians: whatever
/// its number of turns, a multiple of 90 degrees stays a quarter turn for
/// cardinalSinCos (docs/DECISIONI.md, 2026-11-12, "Angoli in gradi
/// ridotti a un giro").
[[nodiscard]] double reducedDegreesToRadians(double degrees);

}  // namespace tg::core
