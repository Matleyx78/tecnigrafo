// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/proximity.hpp"

#include "tg/core/arc_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <variant>

namespace tg::core {
namespace {

// Arithmetic roundoff allowance for circle/side roots and angle wrapping.
// Relative to the operands (radians for angles), not a drawing-unit pick
// tolerance. Arc/line classification still uses PolylineSegment::isArc(),
// including polyline.cpp's 1e-12 bulge threshold.
constexpr double kRoundoff = 32.0 * std::numeric_limits<double>::epsilon();

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

bool boxInside(const BBox& inner, const BBox& outer) {
    return !inner.isEmpty() && !outer.isEmpty() && outer.contains(inner.min()) &&
           outer.contains(inner.max());
}

Vec2 closestOnLine(const PolylineSegment& segment, const Vec2& point) {
    const Vec2 delta = segment.end - segment.start;
    const double length = delta.length();
    // Exact zero guards division, as in polyline.cpp. Do not collapse short
    // but nonzero segments with an absolute length tolerance.
    if (!(length > 0.0)) {
        return segment.start;
    }
    const Vec2 direction = delta * (1.0 / length);
    const double along = dot(point - segment.start, direction);
    if (along <= 0.0) {
        return segment.start;
    }
    if (along >= length) {
        return segment.end;
    }
    return segment.start + direction * along;
}

// A vertical rectangle side. Swap x/y at the call site for a horizontal side.
bool lineHitsSide(const Vec2& start, const Vec2& end, double coordinate, double lower,
                  double upper) {
    const double delta = end.x - start.x;
    if (!(std::abs(delta) > 0.0)) {
        return inRange(start.x, coordinate, coordinate) &&
               overlaps(std::min(start.y, end.y), std::max(start.y, end.y), lower, upper);
    }
    const double parameter = (coordinate - start.x) / delta;
    return inRange(parameter, 0.0, 1.0) &&
           inRange(start.y + parameter * (end.y - start.y), lower, upper);
}

bool segmentIntersects(const PolylineSegment& segment, const BBox& rectangle) {
    if (!boxesOverlap(segment.boundingBox(), rectangle)) {
        return false;
    }
    if (rectangle.contains(segment.start) || rectangle.contains(segment.end)) {
        return true;
    }
    const Vec2 low = rectangle.min();
    const Vec2 high = rectangle.max();
    if (!segment.isArc()) {
        const Vec2 swappedStart{segment.start.y, segment.start.x};
        const Vec2 swappedEnd{segment.end.y, segment.end.x};
        return lineHitsSide(segment.start, segment.end, low.x, low.y, high.y) ||
               lineHitsSide(segment.start, segment.end, high.x, low.y, high.y) ||
               lineHitsSide(swappedStart, swappedEnd, low.y, low.x, high.x) ||
               lineHitsSide(swappedStart, swappedEnd, high.y, low.x, high.x);
    }
    return intersects(arcFromBulge(segment.start, segment.end, segment.bulge), rectangle);
}

/// Builds one visitor out of several lambdas, one per entity type.
template <typename... Lambdas>
struct Overloaded : Lambdas... {
    using Lambdas::operator()...;
};

// Explicit deduction guide: aggregate CTAD is missing from Apple Clang 15.
template <typename... Lambdas>
Overloaded(Lambdas...) -> Overloaded<Lambdas...>;

double polylineDistance(const Polyline& polyline, const Vec2& point) {
    double nearest = std::numeric_limits<double>::infinity();
    if (polyline.vertexCount() == 1) {
        return distance(polyline.vertices().front().position, point);
    }
    for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
        nearest = std::min(nearest, distance(polyline.segment(i), point));
    }
    return nearest;
}

bool polylineIntersects(const Polyline& polyline, const BBox& rectangle) {
    const BBox bounds = polyline.boundingBox();
    if (!boxesOverlap(bounds, rectangle)) {
        return false;
    }
    if (boxInside(bounds, rectangle)) {
        return true;
    }
    for (const PolylineVertex& vertex : polyline.vertices()) {
        if (rectangle.contains(vertex.position)) {
            return true;
        }
    }
    for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
        if (segmentIntersects(polyline.segment(i), rectangle)) {
            return true;
        }
    }
    return false;
}

}  // namespace

Vec2 closestPoint(const PolylineSegment& segment, const Vec2& point) {
    if (!segment.isArc()) {
        return closestOnLine(segment, point);
    }
    const Vec2 endpoint = distance(segment.start, point) <= distance(segment.end, point)
                              ? segment.start
                              : segment.end;
    const ArcGeometry arc = arcFromBulge(segment.start, segment.end, segment.bulge);
    const Vec2 radial = point - arc.center;
    const double length = radial.length();
    if (!(length > 0.0) || !containsAngle(arc, std::atan2(radial.y, radial.x))) {
        return endpoint;
    }
    const Vec2 projected = arc.center + radial * (arc.radius / length);
    // Prefer exact endpoints when projection roundoff would move off them.
    return distance(projected, point) < distance(endpoint, point) ? projected : endpoint;
}

double distance(const PolylineSegment& segment, const Vec2& point) {
    return distance(closestPoint(segment, point), point);
}

double distance(const Entity& entity, const Vec2& point) {
    return std::visit(
        Overloaded{
            [&point](const Polyline& polyline) { return polylineDistance(polyline, point); },
            [&point](const Line& line) {
                return distance(PolylineSegment{line.start, line.end}, point);
            },
            [&point](const Circle& circle) {
                return std::abs(distance(circle.center, point) - circle.radius);
            },
            [&point](const Arc& arc) {
                return distance(closestPoint(arc.geometry(), point), point);
            },
        },
        entity);
}

bool isInside(const Entity& entity, const BBox& rectangle) {
    return boxInside(boundingBox(entity), rectangle);
}

bool intersects(const Entity& entity, const BBox& rectangle) {
    if (boxInside(boundingBox(entity), rectangle)) {
        return true;
    }
    return std::visit(
        Overloaded{
            [&rectangle](const Polyline& polyline) {
                return polylineIntersects(polyline, rectangle);
            },
            [&rectangle](const Line& line) {
                return segmentIntersects(PolylineSegment{line.start, line.end}, rectangle);
            },
            [&rectangle](const Circle& circle) { return intersects(circle.geometry(), rectangle); },
            [&rectangle](const Arc& arc) { return intersects(arc.geometry(), rectangle); },
        },
        entity);
}

}  // namespace tg::core
