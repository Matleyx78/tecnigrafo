// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/arc.hpp"

#include "tg/core/circle.hpp"
#include "tg/core/math.hpp"

#include <cmath>

namespace tg::core {
namespace {

double angleOf(const Vec2& vector) {
    return normalizeAngle(std::atan2(vector.y, vector.x));
}

}  // namespace

ArcGeometry Arc::geometry() const {
    double sweep = endAngle - startAngle;
    if (sweep <= 0.0) {
        sweep += kTwoPi;  // counter-clockwise across 0
    }
    return ArcGeometry{center, radius, startAngle, sweep};
}

double Arc::length() const {
    return radius * geometry().sweep;
}

BBox Arc::boundingBox() const {
    return core::boundingBox(geometry());
}

std::optional<Arc> arcThroughPoints(const Vec2& start, const Vec2& middle, const Vec2& end) {
    const std::optional<Circle> circle = circleThroughPoints(start, middle, end);
    if (!circle) {
        return std::nullopt;
    }
    const double first = angleOf(start - circle->center);
    const double last = angleOf(end - circle->center);
    // Counter-clockwise points keep their order; clockwise ones are swapped,
    // because a DXF arc always runs counter-clockwise (docs/adr/0012).
    const bool counterClockwise = cross(middle - start, end - start) > 0.0;
    return Arc{circle->center, circle->radius, counterClockwise ? first : last,
               counterClockwise ? last : first};
}

}  // namespace tg::core
