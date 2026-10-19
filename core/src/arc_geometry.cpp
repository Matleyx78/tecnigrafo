// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/arc_geometry.hpp"

#include "tg/core/math.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace tg::core {
namespace {

// Hard cap on the number of points generated for a single arc.
constexpr int kMaxArcSegments = 4096;

// Arithmetic roundoff allowance for circle/side roots and angle wrapping.
// Relative to the operands (radians for angles), not a drawing-unit pick
// tolerance.
constexpr double kRoundoff = 32.0 * std::numeric_limits<double>::epsilon();

// Factor of the collinearity criterion of docs/adr/0012, point 4.
constexpr double kCollinearFactor = 1e-9;

bool inRange(double value, double lower, double upper) {
    const double tolerance =
        kRoundoff * std::max({std::abs(value), std::abs(lower), std::abs(upper)});
    return value >= lower - tolerance && value <= upper + tolerance;
}

bool overlaps(double lowerA, double upperA, double lowerB, double upperB) {
    const double tolerance = kRoundoff * std::max({std::abs(lowerA), std::abs(upperA),
                                                   std::abs(lowerB), std::abs(upperB)});
    return upperA >= lowerB - tolerance && upperB >= lowerA - tolerance;
}

bool boxesOverlap(const BBox& a, const BBox& b) {
    return !a.isEmpty() && !b.isEmpty() && overlaps(a.min().x, a.max().x, b.min().x, b.max().x) &&
           overlaps(a.min().y, a.max().y, b.min().y, b.max().y);
}

// True if `angle` lies on the arc, without any roundoff allowance: decides
// which axis-aligned extremes enlarge the bounding box.
bool sweepsThrough(const ArcGeometry& arc, double angle) {
    if (std::abs(arc.sweep) >= kTwoPi) {
        return true;
    }
    const double offset = arc.sweep >= 0.0 ? normalizeAngle(angle - arc.startAngle)
                                           : normalizeAngle(arc.startAngle - angle);
    return offset <= std::abs(arc.sweep);
}

bool containsPoint(const ArcGeometry& arc, const Vec2& point) {
    const Vec2 relative = point - arc.center;
    return containsAngle(arc, std::atan2(relative.y, relative.x));
}

// Where the arc crosses one side of a rectangle: the vertical side
// x = coordinate, lower <= y <= upper, or with `vertical` false the
// horizontal side y = coordinate, lower <= x <= upper.
bool hitsSide(const ArcGeometry& arc, bool vertical, double coordinate, double lower,
              double upper) {
    const double fixedCenter = vertical ? arc.center.x : arc.center.y;
    const double freeCenter = vertical ? arc.center.y : arc.center.x;
    const double offset = coordinate - fixedCenter;
    if (!inRange(offset, -arc.radius, arc.radius)) {
        return false;
    }
    // Factor r^2-d^2 to avoid cancellation near tangency. Clamp only the
    // roundoff accepted above, so a tangent has a single (repeated) root.
    const double root =
        std::sqrt(std::max(0.0, (arc.radius - std::abs(offset)) * (arc.radius + std::abs(offset))));
    const std::array roots{freeCenter - root, freeCenter + root};
    return std::ranges::any_of(roots, [&](double freeCoordinate) {
        if (!inRange(freeCoordinate, lower, upper)) {
            return false;
        }
        const Vec2 point =
            vertical ? Vec2{coordinate, freeCoordinate} : Vec2{freeCoordinate, coordinate};
        return containsPoint(arc, point);
    });
}

}  // namespace

double normalizeAngle(double radians) {
    double result = std::fmod(radians, kTwoPi);
    if (result < 0.0) {
        result += kTwoPi;
    }
    // Adding 2π to a tiny negative value can round up to 2π itself.
    if (result >= kTwoPi) {
        result = 0.0;
    }
    return result + 0.0;  // turns -0 into +0
}

Vec2 pointAt(const ArcGeometry& arc, double angle) {
    return arc.center + Vec2{std::cos(angle), std::sin(angle)} * arc.radius;
}

Vec2 startPoint(const ArcGeometry& arc) {
    return pointAt(arc, arc.startAngle);
}

Vec2 endPoint(const ArcGeometry& arc) {
    return pointAt(arc, arc.startAngle + arc.sweep);
}

bool containsAngle(const ArcGeometry& arc, double angle) {
    if (std::abs(arc.sweep) >= kTwoPi) {
        return true;
    }
    const double directed = arc.sweep >= 0.0 ? angle - arc.startAngle : arc.startAngle - angle;
    double offset = std::fmod(directed, kTwoPi);
    if (offset < 0.0) {
        offset += kTwoPi;
    }
    // Values just below the start can wrap to 2*pi after atan2 rounding.
    return offset <= std::abs(arc.sweep) + kRoundoff || kTwoPi - offset <= kRoundoff;
}

void expandByAxisExtremes(const ArcGeometry& arc, BBox& box) {
    // The exact offsets, not cos/sin of multiples of π/2, which are off by
    // about 1e-16 of the radius.
    const std::array extremes{Vec2{arc.radius, 0.0}, Vec2{0.0, arc.radius}, Vec2{-arc.radius, 0.0},
                              Vec2{0.0, -arc.radius}};
    for (std::size_t quadrant = 0; quadrant < extremes.size(); ++quadrant) {
        if (sweepsThrough(arc, static_cast<double>(quadrant) * (kPi / 2.0))) {
            box.expand(arc.center + extremes.at(quadrant));
        }
    }
}

BBox boundingBox(const ArcGeometry& arc) {
    BBox box;
    // A full circle reaches every extreme: its start point adds nothing.
    if (std::abs(arc.sweep) < kTwoPi) {
        box.expand(startPoint(arc));
        box.expand(endPoint(arc));
    }
    expandByAxisExtremes(arc, box);
    return box;
}

void tessellate(const ArcGeometry& arc, double maxChordError, std::vector<Vec2>& out) {
    // Largest angular step whose chord stays within maxChordError of the arc.
    const double tolerance = std::clamp(maxChordError, 1e-12, arc.radius);
    const double maxStep = 2.0 * std::acos(1.0 - (tolerance / arc.radius));
    const double steps = std::ceil(std::abs(arc.sweep) / std::max(maxStep, 1e-9));
    const int count = std::clamp(static_cast<int>(steps), 1, kMaxArcSegments);

    for (int i = 1; i <= count; ++i) {
        out.push_back(pointAt(arc, arc.startAngle + (arc.sweep * i / count)));
    }
}

Vec2 closestPoint(const ArcGeometry& arc, const Vec2& point) {
    const Vec2 first = startPoint(arc);
    const Vec2 last = endPoint(arc);
    const Vec2 endpoint = distance(first, point) <= distance(last, point) ? first : last;
    const Vec2 radial = point - arc.center;
    const double length = radial.length();
    if (!(length > 0.0) || !containsPoint(arc, point)) {
        return endpoint;
    }
    const Vec2 projected = arc.center + radial * (arc.radius / length);
    // Prefer the end points when projection roundoff would move off them.
    return distance(projected, point) < distance(endpoint, point) ? projected : endpoint;
}

bool intersects(const ArcGeometry& arc, const BBox& rectangle) {
    if (!boxesOverlap(boundingBox(arc), rectangle)) {
        return false;
    }
    if (std::abs(arc.sweep) < kTwoPi &&
        (rectangle.contains(startPoint(arc)) || rectangle.contains(endPoint(arc)))) {
        return true;
    }
    const Vec2 low = rectangle.min();
    const Vec2 high = rectangle.max();
    return hitsSide(arc, true, low.x, low.y, high.y) ||
           hitsSide(arc, true, high.x, low.y, high.y) ||
           hitsSide(arc, false, low.y, low.x, high.x) ||
           hitsSide(arc, false, high.y, low.x, high.x);
}

bool areCollinear(const Vec2& a, const Vec2& b, const Vec2& c) {
    const Vec2 ab = b - a;
    const Vec2 ac = c - a;
    const double longestSquared =
        std::max({ab.lengthSquared(), ac.lengthSquared(), (c - b).lengthSquared()});
    return std::abs(cross(ab, ac)) <= kCollinearFactor * longestSquared;
}

}  // namespace tg::core
