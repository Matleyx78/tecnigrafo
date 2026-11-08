// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/curve_parts.hpp"

#include "solid_space/robust/trigonometry.hpp"
#include "tg/core/math.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

double angleOf(const Vec2& point, const Vec2& center) {
    const Vec2 radial = point - center;
    return normalizeAngle(solid_space::robust::atan2(radial.y, radial.x));
}

PartResult done(Entity entity) {
    return {PartOutcome::Done, std::move(entity)};
}

PartResult collapsedArc(PartCourse course) {
    if (course == PartCourse::AcrossStart) {
        return {PartOutcome::NotRepresentable, std::nullopt};
    }
    return {};
}

// Project only a contradictory representation, never the exact places. The
// candidates implement the responsible's minimum angular correction (#487).
struct AngularPart {
    double first;
    double last;
};

double angularDistance(double first, double last) {
    return std::abs(std::remainder(first - last, kTwoPi));
}

AngularPart correctAngles(AngularPart rays, AngularPart bounds, double sweep, bool across = false) {
    const auto [first, last] = rays;
    const auto [originAngle, limitAngle] = bounds;
    const double length = std::abs(sweep);
    const auto before = [&](double from, double to) {
        // Compare rays before subtracting the origin: subtraction can erase
        // the very ulp inversion that this correction must handle (#487 A).
        const bool fromWrapped = sweep > 0.0 ? from < originAngle : from > originAngle;
        const bool toWrapped = sweep > 0.0 ? to < originAngle : to > originAngle;
        if (fromWrapped != toWrapped) {
            return !fromWrapped;
        }
        return sweep > 0.0 ? from < to : from > to;
    };
    const auto inside = [&](double angle) {
        return length == kTwoPi || !before(limitAngle, angle);
    };
    const auto coherent = [&](double from, double to) {
        return inside(from) && inside(to) && (across ? !before(from, to) : !before(to, from));
    };
    if (coherent(first, last)) {
        return {first, last};
    }
    AngularPart best{first, first};
    double cost = std::numeric_limits<double>::infinity();
    double bestFirst = limitAngle;
    double bestLast = limitAngle;
    const auto consider = [&](double from, double to) {
        if (!coherent(from, to)) {
            return;
        }
        const double correction = angularDistance(first, from) + angularDistance(last, to);
        // Exact comparisons of computed values, no geometric tolerance. Prefer
        // the original start, then the beginning of the retained part on ties.
        const bool sameStart = from == bestFirst && before(to, bestLast);
        const bool preferStart = before(from, bestFirst) || sameStart;
        if (correction < cost || (correction == cost && preferStart)) {
            best = {from, to};
            cost = correction;
            bestFirst = from;
            bestLast = to;
        }
    };
    // Endpoints are kept as angles as well as positions: 0 and 2*pi are
    // distinct sides of a circle's cut even though they draw the same ray.
    const std::array fromAngles{first, originAngle, limitAngle};
    const std::array toAngles{last, originAngle, limitAngle};
    for (const double from : fromAngles) {
        for (const double to : toAngles) {
            consider(from, to);
        }
    }
    consider(first, first);
    consider(last, last);
    return best;
}

PartResult arcPart(const Arc& arc, const std::optional<CurvePoint>& from,
                   const std::optional<CurvePoint>& to, PartCourse course, bool circle = false) {
    const double first = from ? angleOf(from->point, arc.center) : arc.startAngle;
    const double last = to ? angleOf(to->point, arc.center) : arc.endAngle;
    const double originalEnd = circle ? kTwoPi : arc.endAngle;
    const double sweep = circle ? kTwoPi : arc.geometry().sweep;
    const bool across = course == PartCourse::AcrossStart;
    const AngularPart corrected =
        correctAngles({first, last}, {arc.startAngle, originalEnd}, sweep, across);
    Arc result = arc;
    result.startAngle = normalizeAngle(corrected.first);
    result.endAngle = normalizeAngle(corrected.last);
    if (result.startAngle == result.endAngle) {
        return collapsedArc(course);
    }
    return done(result);
}

// The changed endpoint must lie in the complementary interval. Its two
// boundaries are the minimum extension and the unrepresentable full turn.
double correctedExtension(double angle, double originalStart, double originalEnd, double sweep) {
    const double complement = std::copysign(kTwoPi - std::abs(sweep), sweep);
    const AngularPart corrected =
        correctAngles({angle, angle}, {originalEnd, originalStart}, complement);
    // correctAngles resolves endpoint ties toward the complement's start.
    // For a start extension the original arc's start must win instead.
    if (angularDistance(angle, originalStart) == angularDistance(angle, originalEnd) &&
        normalizeAngle(std::copysign(1.0, sweep) * (angle - originalEnd)) >
            kTwoPi - std::abs(sweep)) {
        return originalStart;
    }
    return corrected.first;
}

void checkPlace(const CurvePoint& place) {
    assert(place.position.location != Location::BeforeStart &&
           place.position.location != Location::AfterEnd);
    (void)place;
}

// An endpoint is a vertex, or a cut inside the segment beginning at index.
// Only exact segment indices and Location determine the traversal.
struct Boundary {
    std::size_t index{0};
    bool inside{false};
};

Boundary boundaryOf(const Polyline& polyline, const CurvePoint& place) {
    assert(place.position.segment < polyline.segmentCount());
    std::size_t index = place.position.segment;
    if (place.position.location == Location::AtEnd) {
        ++index;
        // Vertices belong to the following segment, except the open end.
        assert(!polyline.isClosed() && index == polyline.segmentCount());
    }
    (void)polyline;  // used only by the assertions, absent in Release
    return {index, place.position.location == Location::Inside};
}

// The station supplies a length for width interpolation only. It never
// decides which places coincide, their order or which segments remain.
double fractionAt(const PolylineSegment& segment, const CurvePoint& place) {
    if (place.position.location == Location::AtStart) {
        return 0.0;
    }
    if (place.position.location == Location::AtEnd) {
        return 1.0;
    }
    const double length = segment.length();
    assert(length > 0.0);
    return place.position.station / length;
}

// Rebuild on the original circle in traversal order, in either direction.
double cutBulge(const PolylineSegment& original, const Vec2& from, const Vec2& to) {
    if (!original.isArc() || from == to) {
        return 0.0;
    }
    const ArcGeometry arc = arcFromBulge(original.start, original.end, original.bulge);
    const double originalStart = normalizeAngle(arc.startAngle);
    const double originalEnd = normalizeAngle(arc.startAngle + arc.sweep);
    const AngularPart rays{angleOf(from, arc.center), angleOf(to, arc.center)};
    const auto corrected = correctAngles(rays, {originalStart, originalEnd}, arc.sweep);
    const double forward = normalizeAngle(corrected.last - corrected.first);
    const double backward = normalizeAngle(corrected.first - corrected.last);
    const double sweep = original.bulge > 0.0 ? forward : -backward;
    return std::tan(sweep / 4.0);
}

Polyline withVertices(const Polyline& source, std::vector<PolylineVertex> vertices) {
    Polyline result{std::move(vertices)};
    result.setConstantWidth(source.constantWidth());
    result.setContinuousLinetype(source.continuousLinetype());
    return result;
}

PartResult polylinePart(const Polyline& polyline, const std::optional<CurvePoint>& from,
                        const std::optional<CurvePoint>& to, PartCourse course) {
    assert(!polyline.isClosed() || (from && to));
    if (polyline.segmentCount() == 0) {
        assert(!from && !to);
        return done(polyline);
    }
    const Boundary first = from ? boundaryOf(polyline, *from) : Boundary{};
    const Boundary last = to ? boundaryOf(polyline, *to) : Boundary{polyline.segmentCount()};
    const std::size_t endIndex =
        last.index + (course == PartCourse::AcrossStart ? polyline.segmentCount() : 0);
    assert(first.index <= endIndex);
    const auto vertexAt = [&](std::size_t index) {
        return polyline.vertices()[index % polyline.vertexCount()];
    };
    std::vector<PolylineVertex> vertices;
    vertices.push_back(vertexAt(first.index));
    if (from) {
        vertices.back().position = from->point;
    }
    const std::size_t segmentEnd = endIndex + (last.inside ? 1 : 0);
    for (std::size_t index = first.index; index < segmentEnd; ++index) {
        const std::size_t originalIndex = index % polyline.segmentCount();
        const PolylineSegment segment = polyline.segment(originalIndex);
        const bool cutStart = index == first.index && first.inside;
        const bool cutEnd = index == endIndex && last.inside;
        PolylineVertex next = vertexAt(index + 1);
        if (to && (cutEnd || index + 1 == segmentEnd)) {
            next.position = to->point;
        }
        if (cutStart || cutEnd) {
            PolylineVertex& vertex = vertices.back();
            const PolylineVertex& original = polyline.vertices()[originalIndex];
            vertex.bulge = cutBulge(segment, vertex.position, next.position);
            const double startFraction = from && cutStart ? fractionAt(segment, *from) : 0.0;
            const double endFraction = to && cutEnd ? fractionAt(segment, *to) : 1.0;
            vertex.startWidth = std::lerp(original.startWidth, original.endWidth, startFraction);
            vertex.endWidth = std::lerp(original.startWidth, original.endWidth, endFraction);
            if (segment.isArc() && vertex.bulge == 0.0 && segmentEnd == first.index + 1) {
                return collapsedArc(course);
            }
        }
        vertices.push_back(next);
    }
    return done(withVertices(polyline, std::move(vertices)));
}

PartResult extendPolyline(const Polyline& polyline, EntityEnd end, const CurvePoint& to) {
    assert(!polyline.isClosed());
    const std::size_t index = to.position.segment;
    assert(index < polyline.segmentCount());
    const PolylineSegment segment = polyline.segment(index);
    assert(segment.start != segment.end);
    const bool start = end == EntityEnd::Start;
    const Vec2 first = start ? to.point : segment.start;
    const Vec2 last = start ? segment.end : to.point;
    double bulge = segment.bulge;
    if (segment.isArc()) {
        const ArcGeometry arc = arcFromBulge(segment.start, segment.end, segment.bulge);
        const double originalStart = normalizeAngle(arc.startAngle);
        const double originalEnd = normalizeAngle(arc.startAngle + arc.sweep);
        const double received = angleOf(to.point, arc.center);
        const double angle = correctedExtension(received, originalStart, originalEnd, arc.sweep);
        const double firstAngle = start ? angle : originalStart;
        const double lastAngle = start ? originalEnd : angle;
        if (first == last || firstAngle == lastAngle) {
            return {PartOutcome::NotRepresentable, std::nullopt};
        }
        const double sweep = segment.bulge > 0.0 ? normalizeAngle(lastAngle - firstAngle)
                                                 : -normalizeAngle(firstAngle - lastAngle);
        bulge = std::tan(sweep / 4.0);
    }
    std::vector<PolylineVertex> vertices{polyline.vertices().begin(), polyline.vertices().end()};
    // The caller's exact position identifies the first/last non-point
    // segment, just as Extension::Extended does. Preserve the skipped data.
    if (start) {
        for (std::size_t i = 0; i <= index; ++i) {
            assert(i == index || polyline.segment(i).start == polyline.segment(i).end);
            vertices[i].position = to.point;
        }
    } else {
        for (std::size_t i = index + 1; i < vertices.size(); ++i) {
            assert(i == index + 1 || polyline.segment(i - 1).start == polyline.segment(i - 1).end);
            vertices[i].position = to.point;
        }
    }
    PolylineVertex& vertex = vertices[index];
    vertex.bulge = bulge;
    return done(withVertices(polyline, std::move(vertices)));
}

}  // namespace

PartResult partOf(const Entity& entity, const std::optional<CurvePoint>& from,
                  const std::optional<CurvePoint>& to, PartCourse course) {
    assert(!std::holds_alternative<Text>(entity));
    if (from) {
        checkPlace(*from);
    }
    if (to) {
        checkPlace(*to);
    }
    assert(course != PartCourse::AcrossStart || std::holds_alternative<Circle>(entity) ||
           (std::holds_alternative<Polyline>(entity) && std::get<Polyline>(entity).isClosed()));
    if (course == PartCourse::SamePlace) {
        return {};
    }
    if (const auto* line = std::get_if<Line>(&entity)) {
        return done(Line{from ? from->point : line->start, to ? to->point : line->end});
    }
    if (const auto* arc = std::get_if<Arc>(&entity)) {
        return arcPart(*arc, from, to, course);
    }
    if (const auto* circle = std::get_if<Circle>(&entity)) {
        assert(from && to);
        return arcPart(Arc{circle->center, circle->radius, 0.0, 0.0}, from, to, course, true);
    }
    return polylinePart(std::get<Polyline>(entity), from, to, course);
}

PartResult extendTo(const Entity& entity, EntityEnd end, const CurvePoint& to) {
    const bool start = end == EntityEnd::Start;
    if (const auto* line = std::get_if<Line>(&entity)) {
        assert(to.position.location == (start ? Location::BeforeStart : Location::AfterEnd));
        return done(Line{start ? to.point : line->start, start ? line->end : to.point});
    }
    if (const auto* arc = std::get_if<Arc>(&entity)) {
        assert(to.position.location == Location::AfterEnd);
        Arc result = *arc;
        if (start) {
            result.startAngle = correctedExtension(angleOf(to.point, arc->center), arc->startAngle,
                                                   arc->endAngle, arc->geometry().sweep);
        } else {
            result.endAngle = correctedExtension(angleOf(to.point, arc->center), arc->startAngle,
                                                 arc->endAngle, arc->geometry().sweep);
        }
        if (result.startAngle == result.endAngle) {
            return {PartOutcome::NotRepresentable, std::nullopt};
        }
        return done(result);
    }
    assert(std::holds_alternative<Polyline>(entity));
    const auto& polyline = std::get<Polyline>(entity);
    assert(to.position.segment < polyline.segmentCount());
    auto expectedLocation = Location::AfterEnd;
    if (start && !polyline.segment(to.position.segment).isArc()) {
        expectedLocation = Location::BeforeStart;
    }
    assert(to.position.location == expectedLocation);
    (void)expectedLocation;  // used only by the assertion, absent in Release
    return extendPolyline(polyline, end, to);
}

}  // namespace tg::core
