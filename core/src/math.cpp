// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/math.hpp"

#include "solid_space/robust/elementary.hpp"

#include <cmath>
#include <limits>

namespace tg::core {
namespace robust = solid_space::robust;

namespace {
double reducedTrigAngle(double angle) {
    if (std::abs(angle) > robust::kMaxTrigArgument) {
        return std::fmod(angle, kTwoPi);
    }
    return angle;
}
}  // namespace

SinCos sinCos(double angle) {
    angle = reducedTrigAngle(angle);
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

double tan(double angle) {
    return robust::tan(reducedTrigAngle(angle));
}

double acos(double x) {
    return robust::acos(x);
}

double hypot(double x, double y) {
    if (std::isinf(x) || std::isinf(y)) {
        return std::numeric_limits<double>::infinity();
    }
    if (std::isnan(x) || std::isnan(y)) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (std::abs(x) > robust::kMaxHypotArgument || std::abs(y) > robust::kMaxHypotArgument) {
        return 2.0 * robust::hypot(x / 2.0, y / 2.0);
    }
    return robust::hypot(x, y);
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
