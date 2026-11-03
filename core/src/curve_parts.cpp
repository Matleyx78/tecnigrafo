// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/curve_parts.hpp"

#include "solid_space/robust/trigonometry.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
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

PartResult arcPart(const Arc& arc, const std::optional<CurvePoint>& from,
                   const std::optional<CurvePoint>& to, PartCourse course) {
    Arc result = arc;
    if (from) {
        result.startAngle = angleOf(from->point, arc.center);
    }
    if (to) {
        result.endAngle = angleOf(to->point, arc.center);
    }
    // Equality here tests the representation, not whether the exact places
    // coincide: only the caller can decide that, with PartCourse (#359).
    if (result.startAngle == result.endAngle) {
        return collapsedArc(course);
    }
    return done(result);
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
        if (polyline.isClosed()) {
            index %= polyline.segmentCount();
        }
    }
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

// Rebuild a cut segment on its original circle, with the original direction.
// Equal rounded ends/angles describe the short, zero-sweep part (B1).
double cutBulge(const PolylineSegment& original, const Vec2& from, const Vec2& to) {
    if (!original.isArc() || from == to) {
        return 0.0;
    }
    const ArcGeometry arc = arcFromBulge(original.start, original.end, original.bulge);
    const double start = angleOf(from, arc.center);
    const double end = angleOf(to, arc.center);
    const double sweep =
        original.bulge > 0.0 ? normalizeAngle(end - start) : -normalizeAngle(start - end);
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
        PolylineVertex next = vertexAt(cutEnd ? index : index + 1);
        if (cutEnd || (to && index + 1 == segmentEnd)) {
            next.position = to->point;
        }
        if (cutStart || cutEnd) {
            PolylineVertex& vertex = vertices.back();
            const PolylineVertex& original = polyline.vertices()[originalIndex];
            vertex.bulge = cutBulge(segment, vertex.position, next.position);
            const double startFraction = cutStart ? fractionAt(segment, *from) : 0.0;
            const double endFraction = cutEnd ? fractionAt(segment, *to) : 1.0;
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
        const double firstAngle = angleOf(first, arc.center);
        const double lastAngle = angleOf(last, arc.center);
        if (first == last || firstAngle == lastAngle) {
            return {PartOutcome::NotRepresentable, std::nullopt};
        }
        bulge = cutBulge(segment, first, last);
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
        return arcPart(Arc{circle->center, circle->radius, 0.0, 0.0}, from, to, course);
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
        const std::optional<CurvePoint> place{to};
        PartResult result = arcPart(*arc, start ? place : std::nullopt,
                                    start ? std::nullopt : place, PartCourse::Direct);
        if (result.outcome == PartOutcome::Empty) {
            result.outcome = PartOutcome::NotRepresentable;
        }
        return result;
    }
    assert(std::holds_alternative<Polyline>(entity));
    const Polyline& polyline = std::get<Polyline>(entity);
    assert(to.position.segment < polyline.segmentCount());
    auto expectedLocation = Location::AfterEnd;
    if (start && !polyline.segment(to.position.segment).isArc()) {
        expectedLocation = Location::BeforeStart;
    }
    assert(to.position.location == expectedLocation);
    return extendPolyline(polyline, end, to);
}

}  // namespace tg::core
