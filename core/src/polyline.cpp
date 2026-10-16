// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/polyline.hpp"

#include "tg/core/math.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <utility>

namespace tg::core {

namespace {

// Below this magnitude a bulge is treated as a straight segment.
constexpr double kBulgeEpsilon = 1e-12;

// Hard cap on the number of points generated for a single arc.
constexpr int kMaxArcSegments = 4096;

// Normalizes an angle to [0, 2*pi).
double normalizeAngle(double radians) {
    double result = std::fmod(radians, kTwoPi);
    if (result < 0.0) {
        result += kTwoPi;
    }
    return result;
}

// True if `angle` lies on the arc that starts at `start` and sweeps `sweep`.
bool angleOnArc(double angle, double start, double sweep) {
    const double offset =
        sweep >= 0.0 ? normalizeAngle(angle - start) : normalizeAngle(start - angle);
    return offset <= std::abs(sweep);
}

}  // namespace

ArcGeometry arcFromBulge(const Vec2& start, const Vec2& end, double bulge) {
    const Vec2 chordVector = end - start;
    const double chord = chordVector.length();
    assert(chord > 0.0 && std::abs(bulge) > kBulgeEpsilon);

    // Signed sagitta and signed radius; see docs/adr/0004 for the derivation.
    const double sagitta = bulge * chord / 2.0;
    const double signedRadius = chord * ((bulge * bulge) + 1.0) / (4.0 * bulge);

    const Vec2 normal = chordVector.perpendicular() * (1.0 / chord);
    const Vec2 midpoint = (start + end) * 0.5;
    const Vec2 center = midpoint + normal * (signedRadius - sagitta);

    const Vec2 fromCenter = start - center;
    return ArcGeometry{center, std::abs(signedRadius), std::atan2(fromCenter.y, fromCenter.x),
                       4.0 * std::atan(bulge)};
}

bool PolylineSegment::isArc() const {
    // Exact comparison on purpose: it only guards the division by the chord
    // length in arcFromBulge(), it does not decide a geometric fact.
    return std::abs(bulge) > kBulgeEpsilon && start != end;
}

double PolylineSegment::length() const {
    if (!isArc()) {
        return distance(start, end);
    }
    const ArcGeometry arc = arcFromBulge(start, end, bulge);
    return arc.radius * std::abs(arc.sweep);
}

BBox PolylineSegment::boundingBox() const {
    BBox box(start, end);
    if (!isArc()) {
        return box;
    }
    // An arc reaches beyond its end points only at its axis-aligned extremes.
    const ArcGeometry arc = arcFromBulge(start, end, bulge);
    for (int quadrant = 0; quadrant < 4; ++quadrant) {
        const double angle = quadrant * (kPi / 2.0);
        if (angleOnArc(angle, arc.startAngle, arc.sweep)) {
            box.expand(arc.center + Vec2{std::cos(angle), std::sin(angle)} * arc.radius);
        }
    }
    return box;
}

void PolylineSegment::tessellate(double maxChordError, std::vector<Vec2>& out) const {
    if (!isArc()) {
        out.push_back(end);
        return;
    }
    const ArcGeometry arc = arcFromBulge(start, end, bulge);

    // Largest angular step whose chord stays within maxChordError of the arc.
    const double tolerance = std::clamp(maxChordError, 1e-12, arc.radius);
    const double maxStep = 2.0 * std::acos(1.0 - (tolerance / arc.radius));
    const double steps = std::ceil(std::abs(arc.sweep) / std::max(maxStep, 1e-9));
    const int count = std::clamp(static_cast<int>(steps), 1, kMaxArcSegments);

    for (int i = 1; i < count; ++i) {
        const double angle = arc.startAngle + (arc.sweep * i / count);
        out.push_back(arc.center + Vec2{std::cos(angle), std::sin(angle)} * arc.radius);
    }
    out.push_back(end);  // exact end point, no accumulated rounding
}

Polyline::Polyline(std::vector<PolylineVertex> vertices, bool closed)
    : m_vertices(std::move(vertices)), m_closed(closed) {}

void Polyline::addVertex(const Vec2& position, double bulge) {
    m_vertices.push_back(PolylineVertex{position, bulge});
}

std::size_t Polyline::segmentCount() const {
    const std::size_t count = m_vertices.size();
    if (count < 2) {
        return 0;
    }
    return m_closed ? count : count - 1;
}

PolylineSegment Polyline::segment(std::size_t index) const {
    assert(index < segmentCount());
    const PolylineVertex& from = m_vertices[index];
    const PolylineVertex& to = m_vertices[(index + 1) % m_vertices.size()];
    return PolylineSegment{from.position, to.position, from.bulge};
}

double Polyline::length() const {
    double total = 0.0;
    for (std::size_t i = 0; i < segmentCount(); ++i) {
        total += segment(i).length();
    }
    return total;
}

BBox Polyline::boundingBox() const {
    BBox box;
    for (const PolylineVertex& vertex : m_vertices) {
        box.expand(vertex.position);
    }
    for (std::size_t i = 0; i < segmentCount(); ++i) {
        box.expand(segment(i).boundingBox());
    }
    return box;
}

std::vector<Vec2> Polyline::tessellate(double maxChordError) const {
    std::vector<Vec2> points;
    if (m_vertices.empty()) {
        return points;
    }
    points.reserve(m_vertices.size());
    points.push_back(m_vertices.front().position);
    for (std::size_t i = 0; i < segmentCount(); ++i) {
        segment(i).tessellate(maxChordError, points);
    }
    if (m_closed && points.size() > 1) {
        points.pop_back();  // the closing segment ends on the first point
    }
    return points;
}

}  // namespace tg::core
