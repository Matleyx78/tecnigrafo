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

}  // namespace tg::core
