// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/circle.hpp"

#include "tg/core/math.hpp"

#include <cmath>

namespace tg::core {

ArcGeometry Circle::geometry() const {
    return ArcGeometry{center, radius, 0.0, kTwoPi};
}

double Circle::length() const {
    return kTwoPi * radius;
}

BBox Circle::boundingBox() const {
    const Vec2 half{radius, radius};
    return {center - half, center + half};
}

std::optional<Circle> circleThroughPoints(const Vec2& a, const Vec2& b, const Vec2& c) {
    if (areCollinear(a, b, c)) {
        return std::nullopt;
    }
    // Circumcentre relative to `a`: the coordinates stay small even far
    // from the origin, and the collinearity test above keeps the
    // determinant away from zero.
    const Vec2 ab = b - a;
    const Vec2 ac = c - a;
    const double determinant = 2.0 * cross(ab, ac);
    const double abSquared = ab.lengthSquared();
    const double acSquared = ac.lengthSquared();
    const Vec2 offset{((ac.y * abSquared) - (ab.y * acSquared)) / determinant,
                      ((ab.x * acSquared) - (ac.x * abSquared)) / determinant};
    return Circle{a + offset, offset.length()};
}

}  // namespace tg::core
