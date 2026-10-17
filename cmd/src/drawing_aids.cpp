// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/drawing_aids.hpp"

#include <cmath>
#include <limits>

namespace tg::cmd {
namespace {

// Relative tolerance on the two offsets, the same as core's proximity code:
// a pointer on the diagonal whose offsets differ only by rounding (pointer
// and last point with fractional coordinates) still counts as a tie, and a
// tie goes to the horizontal, as docs/adr/0008 prescribes.
constexpr double kRoundoff = 32.0 * std::numeric_limits<double>::epsilon();

}  // namespace

core::Vec2 orthoPoint(const core::Vec2& lastPoint, const core::Vec2& pointer) {
    const double dx = std::abs(pointer.x - lastPoint.x);
    const double dy = std::abs(pointer.y - lastPoint.y);
    if (dx >= dy - (kRoundoff * dy)) {
        return {pointer.x, lastPoint.y};
    }
    return {lastPoint.x, pointer.y};
}

}  // namespace tg::cmd
