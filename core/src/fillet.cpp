// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/fillet.hpp"

#include "solid_space/geom2d/tangency.hpp"
#include "solid_space/robust/point.hpp"
#include "solid_space_bridge.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/curve_parts.hpp"
#include "tg/core/intersections.hpp"
#include "tg/core/line.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

namespace g = bridge::g;

/// The largest radius solid-space takes (docs/adr/0035, point 23): its domain
/// reaches kMaxCoordinate, included.
constexpr double kMaxRadius = solid_space::robust::kMaxCoordinate;

/// The input of fillet and tangentCircleNear that is the radius
/// (tangency.hpp): an outOfDomain diagnosis on it, with a radius within the
/// domain, is a construction beyond the domain.
constexpr std::size_t kRadiusInput = 2;

/// The entities FILLET takes as two distinct curves (docs/adr/0035, point 13).
bool takesFillet(const Entity& entity) {
    return std::holds_alternative<Line>(entity) || std::holds_alternative<Arc>(entity) ||
           std::holds_alternative<Circle>(entity);
}

CornerResult withOutcome(CornerOutcome outcome) {
    CornerResult result;
    result.outcome = outcome;
    return result;
}

CornerOutcome outcomeOf(g::TangencyOutcome outcome) {
    switch (outcome) {
        case g::TangencyOutcome::found:
            return CornerOutcome::Done;
        case g::TangencyOutcome::none:
            return CornerOutcome::None;
        case g::TangencyOutcome::infinitelyMany:
            return CornerOutcome::InfinitelyMany;
        case g::TangencyOutcome::ambiguousPick:
            return CornerOutcome::AmbiguousPick;
        case g::TangencyOutcome::radiusTooLarge:
            return CornerOutcome::RadiusTooLarge;
    }
    return CornerOutcome::None;
}

/// A diagnosis of fillet or tangentCircleNear (docs/adr/0035, points 13, 14
/// and 23): outOfDomain on the radius is a construction beyond the domain,
/// since a radius beyond it never reaches solid-space; anything else is data
/// solid-space refuses.
CornerOutcome outcomeOf(const g::Diagnosis& diagnosis) {
    if (diagnosis.code == g::DiagnosisCode::outOfDomain && diagnosis.input == kRadiusInput) {
        return CornerOutcome::OutOfDomain;
    }
    return CornerOutcome::None;
}

Location locationOf(g::Location location) {
    switch (location) {
        case g::Location::beforeStart:
            return Location::BeforeStart;
        case g::Location::atStart:
            return Location::AtStart;
        case g::Location::inside:
            return Location::Inside;
        case g::Location::atEnd:
            return Location::AtEnd;
        case g::Location::afterEnd:
            return Location::AfterEnd;
    }
    return Location::Inside;
}

CurvePoint curvePointOf(const g::TangencyPoint& tangency) {
    return CurvePoint{
        bridge::fromPoint(tangency.point),
        CurvePosition{0, locationOf(tangency.position.location), tangency.position.station}};
}

/// Fillet::arc as an Arc of Tecnigrafo, counter-clockwise: with a negative
/// sweep the start and the end swap. std::nullopt when there is no arc to
/// add: radius 0, a bulge of 0 (the returned points of tangency coincide,
/// tangency.hpp, Fillet), or an arc the rule finds degenerate
/// (docs/adr/0036, point 5), including one whose two angles come out equal.
std::optional<Arc> filletArc(const g::Fillet& fillet, const ToleranceRule& rule) {
    if (fillet.bulge == 0.0) {
        return std::nullopt;
    }
    const g::Arc& arc = fillet.arc;
    double start = arc.startAngle;
    double end = arc.startAngle + arc.sweep;
    if (arc.sweep < 0.0) {
        std::swap(start, end);
    }
    Arc result{bridge::fromPoint(arc.center), arc.radius, normalizeAngle(start),
               normalizeAngle(end)};
    if (result.startAngle == result.endAngle || isDegenerate(Entity{result}, rule)) {
        return std::nullopt;
    }
    return result;
}

/// A fillet as solid-space builds it, with its arc in Tecnigrafo's form.
struct Solved {
    CornerOutcome outcome{CornerOutcome::None};
    g::Fillet fillet;        ///< with Done
    std::optional<Arc> arc;  ///< with Done: none for the corner
};

/// fillet of solid-space on two curves, with one radius.
Solved attempt(const g::Curve& first, const Vec2& pickFirst, const g::Curve& second,
               const Vec2& pickSecond, double radius, const ToleranceRule& rule) {
    auto made =
        g::fillet(first, bridge::toPoint(pickFirst), second, bridge::toPoint(pickSecond), radius);
    if (!made.hasValue()) {
        return Solved{outcomeOf(made.diagnosis()), {}, std::nullopt};
    }
    const g::Fillet fillet = std::move(made).value();
    if (fillet.outcome != g::TangencyOutcome::found) {
        return Solved{outcomeOf(fillet.outcome), {}, std::nullopt};
    }
    return Solved{CornerOutcome::Done, fillet, filletArc(fillet, rule)};
}

/// fillet of solid-space on two curves. A fillet arc that is not added
/// (filletArc) gives the result of radius 0, the corner (docs/adr/0036,
/// point 5). Precondition: radius within [0, kMaxRadius].
Solved solve(const g::Curve& first, const Vec2& pickFirst, const g::Curve& second,
             const Vec2& pickSecond, double radius, const ToleranceRule& rule) {
    Solved solved = attempt(first, pickFirst, second, pickSecond, radius, rule);
    if (solved.outcome == CornerOutcome::Done && !solved.arc && radius > 0.0) {
        solved = attempt(first, pickFirst, second, pickSecond, 0.0, rule);
    }
    return solved;
}

/// The part of a line or an arc that a fillet keeps (Fillet::keepFirst and
/// keepSecond): cut with partOf, or extended with extendTo, as the position
/// of the point of tangency says (docs/tappe/0.7/P19-raccordi-core.md, B2 of
/// #367). A circle is never cut.
PartResult keptPart(const Entity& entity, const g::TangencyPoint& tangency, g::KeptPart keep) {
    if (std::holds_alternative<Circle>(entity)) {
        return PartResult{PartOutcome::Done, entity};
    }
    const CurvePoint place = curvePointOf(tangency);
    const bool fromStart = keep == g::KeptPart::fromStart;
    switch (place.position.location) {
        case Location::AtStart:
        case Location::Inside:
        case Location::AtEnd:
            return fromStart ? partOf(entity, std::nullopt, place, PartCourse::Direct)
                             : partOf(entity, place, std::nullopt, PartCourse::Direct);
        case Location::AfterEnd:
            if (fromStart || std::holds_alternative<Arc>(entity)) {
                // For an arc AfterEnd is the rest of its circle: the start is
                // run backwards to reach it (extendTo).
                return extendTo(entity, fromStart ? EntityEnd::End : EntityEnd::Start, place);
            }
            break;
        case Location::BeforeStart:
            if (!fromStart) {
                return extendTo(entity, EntityEnd::Start, place);
            }
            break;
    }
    // A straight kept part wholly on the extension, between the point of
    // tangency and the kept end (KeptPart, tangency.hpp): only a line gets
    // here, and only without a fillet arc to continue.
    assert(std::holds_alternative<Line>(entity));
    const auto& line = std::get<Line>(entity);
    return PartResult{PartOutcome::Done,
                      fromStart ? Line{line.start, place.point} : Line{place.point, line.end}};
}

/// The entity as the fillet leaves it, with trim: std::nullopt in `entity`
/// when it is erased.
struct Kept {
    bool representable{true};
    std::optional<Entity> entity;
};

Kept keep(const Entity& entity, const g::TangencyPoint& tangency, g::KeptPart part,
          const ToleranceRule& rule) {
    PartResult kept = keptPart(entity, tangency, part);
    switch (kept.outcome) {
        case PartOutcome::Done:
            assert(kept.entity.has_value());
            if (isDegenerate(*kept.entity, rule)) {
                return Kept{true, std::nullopt};
            }
            return Kept{true, std::move(kept.entity)};
        case PartOutcome::Empty:
            return Kept{true, std::nullopt};
        case PartOutcome::NotRepresentable:
            break;
    }
    return Kept{false, std::nullopt};
}

// ---------------------------------------------------------------------------
// Polylines

/// The width at `point` of a straight segment from `start` to `end` with the
/// given widths, interpolated by length (docs/adr/0031, point 9).
double widthAt(const Vec2& start, const Vec2& end, double startWidth, double endWidth,
               const Vec2& point) {
    const double length = distance(start, end);
    if (startWidth == endWidth || !(length > 0.0)) {
        return startWidth;
    }
    return startWidth + (endWidth - startWidth) * (distance(start, point) / length);
}

/// The common vertex of two segments of a polyline, given in either order:
/// `earlier` is the segment that ends there and `later` the one that leaves
/// it. std::nullopt when they are not consecutive.
struct Corner {
    std::size_t earlier{0};
    std::size_t later{0};
};

std::optional<Corner> cornerOf(const Polyline& polyline, std::size_t i, std::size_t j) {
    const std::size_t count = polyline.segmentCount();
    const auto follows = [&](std::size_t before, std::size_t after) {
        return before + 1 == after || (polyline.isClosed() && before + 1 == count && after == 0);
    };
    if (i == j) {
        return std::nullopt;
    }
    if (follows(i, j)) {
        return Corner{i, j};
    }
    if (follows(j, i)) {
        return Corner{j, i};
    }
    return std::nullopt;
}

/// The fillet at the vertex where segment `corner.earlier` ends, of the two
/// segments of `polyline` (filletSegments). `firstIsEarlier` says which pick
/// goes first to solid-space.
struct VertexFillet {
    CornerOutcome outcome{CornerOutcome::None};
    Polyline polyline;       ///< with Done and trim
    std::optional<Arc> arc;  ///< with Done: none for the corner
    /// With Done and trim: the index in `polyline` of the vertex that ended
    /// the later segment.
    std::size_t next{0};
};

VertexFillet filletVertex(const Polyline& polyline, Corner corner, const Vec2& pickEarlier,
                          const Vec2& pickLater, bool earlierFirst, double radius, bool trim,
                          const ToleranceRule& rule) {
    const PolylineSegment earlier = polyline.segment(corner.earlier);
    const PolylineSegment later = polyline.segment(corner.later);
    if (earlier.isArc() || later.isArc()) {
        // FILLET on the arcs of a polyline is left out (docs/adr/0035).
        return VertexFillet{CornerOutcome::NotAllowed, {}, std::nullopt, 0};
    }
    const g::Curve earlierCurve{bridge::pieceOf(earlier)};
    const g::Curve laterCurve{bridge::pieceOf(later)};
    const Solved solved =
        earlierFirst ? solve(earlierCurve, pickEarlier, laterCurve, pickLater, radius, rule)
                     : solve(laterCurve, pickLater, earlierCurve, pickEarlier, radius, rule);
    if (solved.outcome != CornerOutcome::Done) {
        return VertexFillet{solved.outcome, {}, std::nullopt, 0};
    }
    const g::Fillet& fillet = solved.fillet;
    const g::TangencyPoint& onEarlier = earlierFirst ? fillet.onFirst : fillet.onSecond;
    const g::TangencyPoint& onLater = earlierFirst ? fillet.onSecond : fillet.onFirst;
    const g::KeptPart keepEarlier = earlierFirst ? fillet.keepFirst : fillet.keepSecond;
    const g::KeptPart keepLater = earlierFirst ? fillet.keepSecond : fillet.keepFirst;
    if (keepEarlier != g::KeptPart::fromStart || keepLater != g::KeptPart::toEnd) {
        // The fillet does not join the two segments at their common vertex:
        // one polyline cannot hold it.
        return VertexFillet{CornerOutcome::None, {}, std::nullopt, 0};
    }
    if (!trim) {
        return VertexFillet{CornerOutcome::Done, {}, solved.arc, 0};
    }

    std::vector<PolylineVertex> vertices(polyline.vertices().begin(), polyline.vertices().end());
    const std::size_t start = corner.earlier;                      // start of `earlier`
    const std::size_t vertex = corner.later;                       // the common vertex
    const std::size_t end = (corner.later + 1) % vertices.size();  // end of `later`
    const PolylineVertex original = vertices[vertex];

    std::vector<PolylineVertex> inserted;
    if (!solved.arc) {
        // The corner: a single point, with no arc (radius 0, or a degenerate
        // fillet arc).
        const Vec2 point = bridge::fromPoint(fillet.center);
        vertices[start].endWidth = widthAt(earlier.start, earlier.end, vertices[start].startWidth,
                                           vertices[start].endWidth, point);
        PolylineVertex cornerVertex = original;
        cornerVertex.position = point;
        cornerVertex.startWidth =
            widthAt(later.start, later.end, original.startWidth, original.endWidth, point);
        inserted.push_back(cornerVertex);
    } else {
        const Vec2 first = bridge::fromPoint(onEarlier.point);
        const Vec2 second = bridge::fromPoint(onLater.point);
        const double firstWidth = widthAt(earlier.start, earlier.end, vertices[start].startWidth,
                                          vertices[start].endWidth, first);
        const double secondWidth =
            widthAt(later.start, later.end, original.startWidth, original.endWidth, second);
        // Fillet::bulge runs from onFirst to onSecond: from the later
        // segment to the earlier one when the later came first.
        const double bulge = earlierFirst ? fillet.bulge : -fillet.bulge;
        const PolylineVertex arcStart{first, bulge, firstWidth, secondWidth};
        const PolylineVertex lineStart{second, original.bulge, secondWidth, original.endWidth};
        // A kept segment the rule finds degenerate is not created
        // (docs/adr/0036, point 5): its far vertex takes the point of
        // tangency, and the one inserted for it is not needed.
        if (isDegenerate(PolylineSegment{earlier.start, first, 0.0}, rule)) {
            vertices[start] = arcStart;
        } else {
            vertices[start].endWidth = firstWidth;
            inserted.push_back(arcStart);
        }
        if (isDegenerate(PolylineSegment{second, later.end, 0.0}, rule)) {
            vertices[end].position = second;
        } else {
            inserted.push_back(lineStart);
        }
    }
    const auto at = vertices.begin() + static_cast<std::ptrdiff_t>(vertex);
    vertices.erase(at);
    vertices.insert(vertices.begin() + static_cast<std::ptrdiff_t>(vertex), inserted.begin(),
                    inserted.end());
    const std::size_t next = end > vertex ? end - 1 + inserted.size() : end;

    Polyline result{std::move(vertices), polyline.isClosed()};
    result.setConstantWidth(polyline.constantWidth());
    result.setContinuousLinetype(polyline.continuousLinetype());
    return VertexFillet{CornerOutcome::Done, std::move(result), solved.arc, next};
}

// ---------------------------------------------------------------------------
// Tangent circles and points

/// The curve of an entity for CIRCLE Ttr: for a polyline, its segment
/// nearest the pick (docs/adr/0035, point 14).
std::optional<g::Curve> tangencyCurveOf(const Entity& entity, const Vec2& pick) {
    if (const auto* polyline = std::get_if<Polyline>(&entity)) {
        if (polyline->segmentCount() == 0) {
            return std::nullopt;
        }
        const std::size_t index = nearestPosition(entity, pick).segment;
        return g::Curve{bridge::pieceOf(polyline->segment(index))};
    }
    return bridge::curveOf(entity);
}

/// The tangent points from `point` to one curve, on the curve as given.
/// Empty for a straight curve and for data solid-space refuses.
std::vector<g::TangencyPoint> tangentsTo(const Vec2& point, const g::Curve& curve) {
    auto tangents = g::tangentsFrom(bridge::toPoint(point), curve, g::Extension::none);
    if (!tangents.hasValue()) {
        return {};
    }
    return std::move(tangents).value().points;
}

}  // namespace

CornerResult fillet(const Entity& first, const Vec2& pickFirst, const Entity& second,
                    const Vec2& pickSecond, double radius, bool trim, const ToleranceRule& rule) {
    assert(std::isfinite(radius) && radius >= 0.0);
    if (!takesFillet(first) || !takesFillet(second)) {
        return withOutcome(CornerOutcome::NotAllowed);
    }
    if (radius > kMaxRadius) {
        return withOutcome(CornerOutcome::None);
    }
    const std::optional<g::Curve> firstCurve = bridge::curveOf(first);
    const std::optional<g::Curve> secondCurve = bridge::curveOf(second);
    assert(firstCurve && secondCurve);
    const Solved solved = solve(*firstCurve, pickFirst, *secondCurve, pickSecond, radius, rule);
    if (solved.outcome != CornerOutcome::Done) {
        return withOutcome(solved.outcome);
    }
    CornerResult result = withOutcome(CornerOutcome::Done);
    if (solved.arc) {
        result.added = *solved.arc;
    }
    if (!trim) {
        result.first = first;
        result.second = second;
        return result;
    }
    const g::Fillet& made = solved.fillet;
    Kept keptFirst = keep(first, made.onFirst, made.keepFirst, rule);
    Kept keptSecond = keep(second, made.onSecond, made.keepSecond, rule);
    if (!keptFirst.representable || !keptSecond.representable) {
        // A kept part the model cannot hold: the operation does not succeed
        // and no entity changes (B1 of #464).
        return withOutcome(CornerOutcome::None);
    }
    result.first = std::move(keptFirst.entity);
    result.second = std::move(keptSecond.entity);
    return result;
}

CornerResult filletSegments(const Polyline& polyline, const Vec2& pickFirst, const Vec2& pickSecond,
                            double radius, bool trim, const ToleranceRule& rule) {
    assert(std::isfinite(radius) && radius >= 0.0);
    if (polyline.segmentCount() < 2) {
        return withOutcome(CornerOutcome::NotAllowed);
    }
    const Entity entity{polyline};
    const std::size_t i = nearestPosition(entity, pickFirst).segment;
    const std::size_t j = nearestPosition(entity, pickSecond).segment;
    const std::optional<Corner> corner = cornerOf(polyline, i, j);
    if (!corner) {
        return withOutcome(CornerOutcome::NotAllowed);
    }
    if (polyline.segment(i).isArc() || polyline.segment(j).isArc()) {
        // FILLET on the arcs of a polyline is left out (docs/adr/0035).
        return withOutcome(CornerOutcome::NotAllowed);
    }
    if (radius > kMaxRadius) {
        return withOutcome(CornerOutcome::None);
    }
    const bool earlierFirst = corner->earlier == i;
    const Vec2& pickEarlier = earlierFirst ? pickFirst : pickSecond;
    const Vec2& pickLater = earlierFirst ? pickSecond : pickFirst;
    VertexFillet made =
        filletVertex(polyline, *corner, pickEarlier, pickLater, earlierFirst, radius, trim, rule);
    if (made.outcome != CornerOutcome::Done) {
        return withOutcome(made.outcome);
    }
    CornerResult result = withOutcome(CornerOutcome::Done);
    if (trim) {
        result.first = std::move(made.polyline);
    } else {
        result.first = polyline;
        if (made.arc) {
            result.added = *made.arc;
        }
    }
    return result;
}

PolylineCorners filletPolyline(const Polyline& polyline, double radius, bool trim,
                               const ToleranceRule& rule) {
    assert(std::isfinite(radius) && radius >= 0.0);
    PolylineCorners corners;
    corners.polyline = polyline;
    const std::size_t vertexCount = polyline.vertexCount();
    if (polyline.segmentCount() < 2) {
        return corners;
    }
    // The original vertices in order (B15): for an open polyline the inner
    // ones, for a closed one from vertex 0, the closing vertex.
    const std::size_t first = polyline.isClosed() ? 0 : 1;
    const std::size_t last = polyline.isClosed() ? vertexCount : vertexCount - 1;
    // Where the original vertex being visited is now, on the polyline as the
    // vertices before it left it (with trim), or on the original one.
    std::size_t current = first;
    for (std::size_t original = first; original < last; ++original) {
        const Polyline& judged = trim ? corners.polyline : polyline;
        const std::size_t at = trim ? current : original;
        const std::size_t count = judged.segmentCount();
        const Corner corner{(at + count - 1) % count, at};
        const PolylineSegment earlier = judged.segment(corner.earlier);
        const PolylineSegment later = judged.segment(corner.later);
        VertexFillet made =
            radius > kMaxRadius
                ? VertexFillet{CornerOutcome::None, {}, std::nullopt, 0}
                : filletVertex(judged, corner, earlier.start, later.end, true, radius, trim, rule);
        if (made.outcome != CornerOutcome::Done) {
            ++corners.tooShort;
            current = at + 1;
            continue;
        }
        ++corners.done;
        if (trim) {
            corners.polyline = std::move(made.polyline);
            current = made.next;
        } else if (made.arc) {
            corners.added.emplace_back(*made.arc);
        }
    }
    return corners;
}

TangentCircleResult circleTangentTangentRadius(const Entity& first, const Vec2& pickFirst,
                                               const Entity& second, const Vec2& pickSecond,
                                               double radius) {
    if (std::holds_alternative<Text>(first) || std::holds_alternative<Text>(second)) {
        return TangentCircleResult{CornerOutcome::NotAllowed, std::nullopt};
    }
    if (radius > kMaxRadius) {
        return TangentCircleResult{CornerOutcome::None, std::nullopt};
    }
    const std::optional<g::Curve> firstCurve = tangencyCurveOf(first, pickFirst);
    const std::optional<g::Curve> secondCurve = tangencyCurveOf(second, pickSecond);
    if (!firstCurve || !secondCurve) {
        return TangentCircleResult{CornerOutcome::None, std::nullopt};
    }
    const auto chosen = g::tangentCircleNear(*firstCurve, bridge::toPoint(pickFirst), *secondCurve,
                                             bridge::toPoint(pickSecond), radius);
    if (!chosen.hasValue()) {
        return TangentCircleResult{outcomeOf(chosen.diagnosis()), std::nullopt};
    }
    const g::ChosenCircle& circle = chosen.value();
    if (circle.outcome != g::TangencyOutcome::found) {
        return TangentCircleResult{outcomeOf(circle.outcome), std::nullopt};
    }
    return TangentCircleResult{CornerOutcome::Done,
                               Circle{bridge::fromPoint(circle.circle.center), radius}};
}

std::vector<Vec2> tangentPoints(const Vec2& point, const Entity& entity) {
    std::vector<Vec2> points;
    const auto* polyline = std::get_if<Polyline>(&entity);
    if (polyline == nullptr) {
        if (const std::optional<g::Curve> curve = bridge::curveOf(entity)) {
            for (const g::TangencyPoint& tangency : tangentsTo(point, *curve)) {
                points.push_back(bridge::fromPoint(tangency.point));
            }
        }
        return points;
    }
    const std::vector<g::PolylineSegment> pieces = bridge::piecesOf(*polyline);
    std::vector<std::vector<g::TangencyPoint>> bySegment;
    bySegment.reserve(pieces.size());
    for (const g::PolylineSegment& piece : pieces) {
        bySegment.push_back(piece.bulge == 0.0 ? std::vector<g::TangencyPoint>{}
                                               : tangentsTo(point, g::Curve{piece}));
    }
    for (std::size_t i = 0; i < bySegment.size(); ++i) {
        // The segment that follows: a point at the end of this one that is
        // at the start of that one is their common vertex, listed there (B7).
        std::size_t following = bySegment.size();
        if (i + 1 < bySegment.size()) {
            following = i + 1;
        } else if (polyline->isClosed()) {
            following = 0;
        }
        const bool followerStartsHere =
            following < bySegment.size() && following != i &&
            std::ranges::any_of(bySegment[following], [](const g::TangencyPoint& tangency) {
                return tangency.position.location == g::Location::atStart;
            });
        for (const g::TangencyPoint& tangency : bySegment[i]) {
            if (followerStartsHere && tangency.position.location == g::Location::atEnd) {
                continue;
            }
            points.push_back(bridge::fromPoint(tangency.point));
        }
    }
    return points;
}

}  // namespace tg::core
