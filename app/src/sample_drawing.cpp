// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "sample_drawing.hpp"

#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"

#include <array>
#include <cmath>

namespace tg::app {

namespace {

using core::Polyline;
using core::Vec2;

// Bulge of a 90 degree counter-clockwise arc: tan(90deg / 4).
const double kQuarterBulge = std::tan(core::degreesToRadians(90.0) / 4.0);

// Bulge of a 180 degree counter-clockwise arc.
constexpr double kHalfBulge = 1.0;

Polyline roundedRectangle(double width, double height, double radius) {
    Polyline outline;
    outline.addVertex({radius, 0.0});
    outline.addVertex({width - radius, 0.0}, kQuarterBulge);
    outline.addVertex({width, radius});
    outline.addVertex({width, height - radius}, kQuarterBulge);
    outline.addVertex({width - radius, height});
    outline.addVertex({radius, height}, kQuarterBulge);
    outline.addVertex({0.0, height - radius});
    outline.addVertex({0.0, radius}, kQuarterBulge);
    outline.setClosed(true);
    return outline;
}

Polyline circle(const Vec2& center, double radius) {
    Polyline result;
    result.addVertex(center - Vec2{radius, 0.0}, kHalfBulge);
    result.addVertex(center + Vec2{radius, 0.0}, kHalfBulge);
    result.setClosed(true);
    return result;
}

Polyline slot(const Vec2& from, const Vec2& to, double width) {
    const double half = width / 2.0;
    Polyline result;
    result.addVertex({from.x, from.y - half});
    result.addVertex({to.x, to.y - half}, kHalfBulge);
    result.addVertex({to.x, to.y + half});
    result.addVertex({from.x, from.y + half}, kHalfBulge);
    result.setClosed(true);
    return result;
}

}  // namespace

core::Document makeSampleDrawing() {
    constexpr double kWidth = 120.0;
    constexpr double kHeight = 80.0;
    constexpr double kCornerRadius = 10.0;
    constexpr double kHoleRadius = 4.0;
    constexpr double kHoleInset = 15.0;

    core::Document document;
    document.add(roundedRectangle(kWidth, kHeight, kCornerRadius));
    document.add(slot({40.0, kHeight / 2.0}, {80.0, kHeight / 2.0}, 16.0));

    const std::array<Vec2, 4> holeCenters{{
        {kHoleInset, kHoleInset},
        {kWidth - kHoleInset, kHoleInset},
        {kWidth - kHoleInset, kHeight - kHoleInset},
        {kHoleInset, kHeight - kHoleInset},
    }};
    for (const Vec2& center : holeCenters) {
        document.add(circle(center, kHoleRadius));
    }
    return document;
}

}  // namespace tg::app
