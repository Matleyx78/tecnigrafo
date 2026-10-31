// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/drawing_aids.hpp"

#include "tg/core/math.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace tg::cmd {
namespace {

// Relative tolerance on the two offsets, the same as core's proximity code:
// a pointer on the diagonal whose offsets differ only by rounding (pointer
// and last point with fractional coordinates) still counts as a tie, and a
// tie goes to the horizontal, as docs/adr/0008 prescribes.
constexpr double kRoundoff = 32.0 * std::numeric_limits<double>::epsilon();

// The unit vector at `angle`, exact on the axes: the rays at 90, 180 and 270
// degrees keep the coordinate of the base point that they do not move along,
// as ortho does, instead of being off by the rounding of cos and sin.
core::Vec2 direction(double angle) {
    const double quarters = angle / (core::kPi / 2.0);
    const double nearest = std::round(quarters);
    if (std::abs(quarters - nearest) <= kRoundoff * std::max(1.0, std::abs(nearest))) {
        constexpr std::array<core::Vec2, 4> kAxes{core::Vec2{1.0, 0.0}, core::Vec2{0.0, 1.0},
                                                  core::Vec2{-1.0, 0.0}, core::Vec2{0.0, -1.0}};
        const auto quarter = static_cast<std::int64_t>(nearest);
        return kAxes.at(static_cast<std::size_t>(((quarter % 4) + 4) % 4));
    }
    return {std::cos(angle), std::sin(angle)};
}

}  // namespace

core::Vec2 orthoPoint(const core::Vec2& lastPoint, const core::Vec2& pointer) {
    const double dx = std::abs(pointer.x - lastPoint.x);
    const double dy = std::abs(pointer.y - lastPoint.y);
    if (dx >= dy - (kRoundoff * dy)) {
        return {pointer.x, lastPoint.y};
    }
    return {lastPoint.x, pointer.y};
}

std::optional<core::Vec2> polarPoint(const core::Vec2& base, const core::Vec2& pointer,
                                     double increment, double tolerance) {
    assert(increment > 0.0);
    assert(tolerance >= 0.0);
    const core::Vec2 offset = pointer - base;
    if (offset == core::Vec2{}) {
        return base;
    }
    // The largest |k| of a ray: pi / increment, with the rounding of the
    // division forgiven, so that 180 / 45 gives the rays at +-180 degrees.
    const double turnsToHalf = core::kPi / increment;
    const double lastRay = std::floor(turnsToHalf + (kRoundoff * turnsToHalf));
    const double angle = std::atan2(offset.y, offset.x);  // in [-pi, pi]

    std::optional<core::Vec2> best;
    double bestDistance = std::numeric_limits<double>::infinity();
    // The two rays around the pointer, and the ones across +-pi, which are
    // near the pointer when it points almost along the negative X axis.
    // Clockwise first, so that it keeps a tie.
    for (const double shift : {0.0, -core::kTwoPi, core::kTwoPi}) {
        const double turns = (angle + shift) / increment;
        for (const double ray : {std::floor(turns), std::ceil(turns)}) {
            if (std::abs(ray) > lastRay) {
                continue;
            }
            const core::Vec2 unit = direction(ray * increment);
            const double along = core::dot(offset, unit);
            const core::Vec2 foot = along > 0.0 ? base + (unit * along) : base;
            const double distance = (pointer - foot).length();
            if (distance <= tolerance && distance < bestDistance) {
                best = foot;
                bestDistance = distance;
            }
        }
    }
    return best;
}

}  // namespace tg::cmd
