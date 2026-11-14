// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/math.hpp"

#include "solid_space/robust/trigonometry.hpp"

#include <cmath>

namespace tg::core {
namespace robust = solid_space::robust;

SinCos sinCos(double angle) {
    if (std::abs(angle) > robust::kMaxTrigArgument) {
        angle = std::fmod(angle, kTwoPi);
    }
    const auto result = robust::sinCos(angle);
    return {result.sine, result.cosine};
}

double sin(double angle) {
    return sinCos(angle).sine;
}

double cos(double angle) {
    return sinCos(angle).cosine;
}

double atan2(double y, double x) {
    return robust::atan2(y, x);
}

double atan(double x) {
    return atan2(x, 1.0);
}

SinCos cardinalSinCos(double angle) {
    const auto result = sinCos(angle);
    if (std::abs(result.sine) < kCardinalSnap) {
        return {0.0, std::copysign(1.0, result.cosine)};
    }
    if (std::abs(result.cosine) < kCardinalSnap) {
        return {std::copysign(1.0, result.sine), 0.0};
    }
    return result;
}

double reducedDegreesToRadians(double degrees) {
    constexpr double kFullTurnDegrees = 360.0;
    double reduced = std::fmod(degrees, kFullTurnDegrees);
    if (reduced < 0.0) {
        reduced += kFullTurnDegrees;
    }
    return reduced > 0.0 && reduced < kFullTurnDegrees ? degreesToRadians(reduced) : 0.0;
}

}  // namespace tg::core
