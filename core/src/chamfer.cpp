// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/chamfer.hpp"

#include "solid_space/geom2d/intersection.hpp"
#include "solid_space_bridge.hpp"
#include "tg/core/curve_parts.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

namespace g = solid_space::geom2d;

CornerResult withOutcome(CornerOutcome outcome) {
    return {outcome, {}, {}, {}};
}

[[maybe_unused]] bool valid(const ChamferSpec& spec) {
    if (const auto* distances = std::get_if<ChamferDistances>(&spec)) {
        return std::isfinite(distances->first) && distances->first >= 0.0 &&
               std::isfinite(distances->second) && distances->second >= 0.0;
    }
    const auto& angle = std::get<ChamferAngle>(spec);
    return std::isfinite(angle.length) && angle.length >= 0.0 && std::isfinite(angle.angle);
}

struct Ray {
    Vec2 direction;
    Vec2 end;
    double length;
    bool keepsStart;
};

std::optional<Ray> keptRay(const Line& line, const Vec2& corner, const Vec2& pick,
                           g::Location location) {
    const Vec2 delta = line.end - line.start;
    const double length = delta.length();
    assert(length > 0.0);
    const Vec2 direction{delta.x / length, delta.y / length};
    // As in TRIM, the projected pick chooses a piece. It does not reclassify
    // the exact intersection locations supplied by the kernel.
    const double projected = dot(pick - corner, direction);
    if (projected == 0.0) {
        return std::nullopt;
    }
    const bool keepsStart = projected < 0.0;
    const Vec2 end = keepsStart ? line.start : line.end;
    const Vec2 keptDirection = keepsStart ? -direction : direction;
    const auto keptEnd = keepsStart ? g::Location::beforeStart : g::Location::afterEnd;
    const bool passesKeptEnd = location == keptEnd;
    const double available = distance(end, corner);
    return Ray{keptDirection, end, passesKeptEnd ? -available : available, keepsStart};
}

struct Cuts {
    CornerOutcome outcome{CornerOutcome::None};
    Vec2 first;
    Vec2 second;
    bool cornerOnly{false};
};

Vec2 cutPoint(const Ray& ray, const Vec2& corner, double length) {
    return length == ray.length ? ray.end : corner + ray.direction * length;
}

Cuts solve(const Vec2& corner, const Ray& first, const Ray& second, const ChamferSpec& spec) {
    ChamferDistances distances;
    if (const auto* pair = std::get_if<ChamferDistances>(&spec)) {
        distances = *pair;
        if ((distances.first == 0.0) != (distances.second == 0.0)) {
            return {};
        }
    } else {
        const auto& angle = std::get<ChamferAngle>(spec);
        if (angle.length > first.length) {
            return {CornerOutcome::DistanceTooLarge, {}, {}, false};
        }
        const Vec2 inward = -first.direction;
        const double opening =
            std::atan2(std::abs(cross(inward, second.direction)), dot(inward, second.direction));
        // A half turn gives the same carrying line. The sine rule computes
        // the second distance without constructing a far-away intersection.
        const double turn = std::remainder(angle.angle, kPi);
        const double crossing = std::remainder(opening - turn, kPi);
        if (crossing == 0.0 || turn == 0.0 || angle.length == 0.0) {
            return {};
        }
        const double secondLength = angle.length * (std::sin(turn) / std::sin(opening - turn));
        if (!(secondLength > 0.0)) {
            return {};
        }
        distances = {angle.length, secondLength};
    }

    if (distances.first > first.length || distances.second > second.length) {
        return {CornerOutcome::DistanceTooLarge, {}, {}, false};
    }
    return {CornerOutcome::Done, cutPoint(first, corner, distances.first),
            cutPoint(second, corner, distances.second),
            distances.first == 0.0 && distances.second == 0.0};
}

Location cutLocation(const Line& line, const Vec2& cut) {
    if (cut == line.start) {
        return Location::AtStart;
    }
    if (cut == line.end) {
        return Location::AtEnd;
    }
    const Vec2 direction = line.end - line.start;
    if (dot(cut - line.start, direction) < 0.0) {
        return Location::BeforeStart;
    }
    if (dot(cut - line.end, direction) > 0.0) {
        return Location::AfterEnd;
    }
    return Location::Inside;
}

Location cornerLocation(g::Location location) {
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

PartResult keptPart(const Line& line, const Ray& ray, const Vec2& cut, Location location) {
    const CurvePoint point{cut, {0, location, distance(line.start, cut)}};
    if (location == Location::BeforeStart) {
        return extendTo(line, EntityEnd::Start, point);
    }
    if (location == Location::AfterEnd) {
        return extendTo(line, EntityEnd::End, point);
    }
    const PartCourse course = cut == ray.end ? PartCourse::SamePlace : PartCourse::Direct;
    return ray.keepsStart ? partOf(line, std::nullopt, point, course)
                          : partOf(line, point, std::nullopt, course);
}

bool degenerate(const Vec2& first, const Vec2& second, const ToleranceRule& rule) {
    return isDegenerate(PolylineSegment{first, second, 0.0}, rule);
}

struct VertexCut {
    CornerOutcome outcome{CornerOutcome::None};
    std::optional<Line> added;
    std::size_t next{0};
};

double widthAt(const PolylineVertex& vertex, const Line& line, const Vec2& point) {
    const double fraction = std::clamp(distance(line.start, point) / line.length(), 0.0, 1.0);
    return std::lerp(vertex.startWidth, vertex.endWidth, fraction);
}

std::size_t insertCuts(std::vector<PolylineVertex>& vertices, std::size_t vertex,
                       const Line& earlier, const Line& later, const Line& added,
                       const ToleranceRule& rule) {
    const std::size_t start = (vertex + vertices.size() - 1) % vertices.size();
    const std::size_t end = (vertex + 1) % vertices.size();
    const double firstWidth = widthAt(vertices[start], earlier, added.start);
    const double secondWidth = widthAt(vertices[vertex], later, added.end);
    const PolylineVertex chamferStart{added.start, 0.0, firstWidth, secondWidth};
    PolylineVertex outgoing = vertices[vertex];
    outgoing.position = added.end;
    outgoing.startWidth = secondWidth;

    std::vector<PolylineVertex> inserted;
    if (degenerate(earlier.start, added.start, rule)) {
        vertices[start] = chamferStart;
    } else {
        vertices[start].endWidth = firstWidth;
        inserted.push_back(chamferStart);
    }
    if (degenerate(added.end, later.end, rule)) {
        vertices[end].position = added.end;
    } else {
        inserted.push_back(outgoing);
    }
    const auto at = static_cast<std::ptrdiff_t>(vertex);
    vertices.erase(vertices.begin() + at);
    vertices.insert(vertices.begin() + at, inserted.begin(), inserted.end());
    return end > vertex ? end - 1 + inserted.size() : end;
}

VertexCut cutVertex(Polyline& polyline, std::size_t vertex, bool earlierFirst,
                    const Vec2& pickEarlier, const Vec2& pickLater, const ChamferSpec& spec,
                    bool trim, const ToleranceRule& rule) {
    const std::size_t count = polyline.segmentCount();
    const auto in = polyline.segment((vertex + count - 1) % count);
    const auto out = polyline.segment(vertex);
    if (in.isArc() || out.isArc()) {
        return {CornerOutcome::NotAllowed, {}, 0};
    }
    const Line earlier{in.start, in.end};
    const Line later{out.start, out.end};
    const Entity first = earlierFirst ? earlier : later;
    const Entity second = earlierFirst ? later : earlier;
    const auto result = chamfer(first, earlierFirst ? pickEarlier : pickLater, second,
                                earlierFirst ? pickLater : pickEarlier, spec, false, rule);
    if (result.outcome != CornerOutcome::Done) {
        return {result.outcome, {}, 0};
    }
    if (!result.added) {
        return {CornerOutcome::Done, {}, (vertex + 1) % polyline.vertexCount()};
    }
    const auto& line = std::get<Line>(*result.added);
    if (!trim) {
        return {CornerOutcome::Done, line, 0};
    }
    // An off-segment pick can select an extension beyond the shared vertex.
    // In-place replacement needs both cuts on the original sides of the vertex.
    if (dot(pickEarlier - in.end, in.start - in.end) <= 0.0 ||
        dot(pickLater - out.start, out.end - out.start) <= 0.0) {
        return {};
    }
    std::vector<PolylineVertex> vertices(polyline.vertices().begin(), polyline.vertices().end());
    const Line ordered = earlierFirst ? line : Line{line.end, line.start};
    const std::size_t next = insertCuts(vertices, vertex, earlier, later, ordered, rule);
    Polyline updated{std::move(vertices), polyline.isClosed()};
    updated.setConstantWidth(polyline.constantWidth());
    updated.setContinuousLinetype(polyline.continuousLinetype());
    polyline = std::move(updated);
    return {CornerOutcome::Done, {}, next};
}

}  // namespace

CornerResult chamfer(const Entity& first, const Vec2& pickFirst, const Entity& second,
                     const Vec2& pickSecond, const ChamferSpec& spec, bool trim,
                     const ToleranceRule& rule) {
    assert(valid(spec));
    const auto* lineA = std::get_if<Line>(&first);
    const auto* lineB = std::get_if<Line>(&second);
    if (lineA == nullptr || lineB == nullptr) {
        return withOutcome(CornerOutcome::NotAllowed);
    }
    const auto a = bridge::curveOf(first);
    const auto b = bridge::curveOf(second);
    assert(a && b);
    auto found = g::intersect(*a, g::Extension::extended, *b, g::Extension::extended);
    if (!found.hasValue()) {
        return {};
    }
    const auto& intersection = found.value();
    if (intersection.points.empty()) {
        const bool beyondDomain = intersection.omittedBeyondDomain > 0;
        return withOutcome(beyondDomain ? CornerOutcome::OutOfDomain : CornerOutcome::None);
    }
    const auto& crossing = intersection.points.front();
    const Vec2 corner = bridge::fromPoint(crossing.point);
    const auto rayA = keptRay(*lineA, corner, pickFirst, crossing.onFirst.location);
    const auto rayB = keptRay(*lineB, corner, pickSecond, crossing.onSecond.location);
    if (!rayA || !rayB) {
        return withOutcome(CornerOutcome::AmbiguousPick);
    }
    const Cuts cuts = solve(corner, *rayA, *rayB, spec);
    if (cuts.outcome != CornerOutcome::Done) {
        return withOutcome(cuts.outcome);
    }
    CornerResult result{CornerOutcome::Done, first, second, {}};
    if (!cuts.cornerOnly && !degenerate(cuts.first, cuts.second, rule)) {
        result.added = Line{cuts.first, cuts.second};
    }
    if (trim) {
        const Location locationA = cuts.cornerOnly ? cornerLocation(crossing.onFirst.location)
                                                   : cutLocation(*lineA, cuts.first);
        const Location locationB = cuts.cornerOnly ? cornerLocation(crossing.onSecond.location)
                                                   : cutLocation(*lineB, cuts.second);
        const auto aPart = keptPart(*lineA, *rayA, cuts.first, locationA);
        const auto bPart = keptPart(*lineB, *rayB, cuts.second, locationB);
        if (aPart.outcome == PartOutcome::NotRepresentable ||
            bPart.outcome == PartOutcome::NotRepresentable) {
            return {};
        }
        result.first = aPart.entity;
        result.second = bPart.entity;
        if (result.first && isDegenerate(*result.first, rule)) {
            result.first.reset();
        }
        if (result.second && isDegenerate(*result.second, rule)) {
            result.second.reset();
        }
    }
    return result;
}

CornerResult chamferSegments(const Polyline& polyline, const Vec2& pickFirst,
                             const Vec2& pickSecond, const ChamferSpec& spec, bool trim,
                             const ToleranceRule& rule) {
    assert(valid(spec));
    const std::size_t count = polyline.segmentCount();
    if (count < 2) {
        return withOutcome(CornerOutcome::NotAllowed);
    }
    const Entity entity = polyline;
    const auto first = nearestPosition(entity, pickFirst).segment;
    const auto second = nearestPosition(entity, pickSecond).segment;
    const bool closed = polyline.isClosed();
    const bool earlierFirst = first + 1 == second || (closed && first + 1 == count && second == 0);
    const bool earlierSecond = second + 1 == first || (closed && second + 1 == count && first == 0);
    if (!earlierFirst && !earlierSecond) {
        return withOutcome(CornerOutcome::NotAllowed);
    }
    Polyline updated = polyline;
    const auto cut = cutVertex(updated, earlierFirst ? second : first, earlierFirst,
                               earlierFirst ? pickFirst : pickSecond,
                               earlierFirst ? pickSecond : pickFirst, spec, trim, rule);
    if (cut.outcome != CornerOutcome::Done) {
        return withOutcome(cut.outcome);
    }
    return {CornerOutcome::Done, std::move(updated), {}, cut.added};
}

PolylineCorners chamferPolyline(const Polyline& polyline, const ChamferSpec& spec, bool trim,
                                const ToleranceRule& rule) {
    assert(valid(spec));
    PolylineCorners result{polyline, {}, 0, 0};
    if (polyline.segmentCount() < 2) {
        return result;
    }
    const bool closed = polyline.isClosed();
    const std::size_t first = closed ? 0 : 1;
    const std::size_t end = closed ? polyline.vertexCount() : polyline.vertexCount() - 1;
    std::size_t current = first;
    for (std::size_t original = first; original < end; ++original) {
        const std::size_t count = result.polyline.segmentCount();
        if (count < 2) {
            result.tooShort += end - original;
            break;
        }
        const std::size_t at = trim ? current : original;
        const Vec2 pickEarlier = result.polyline.segment((at + count - 1) % count).start;
        const Vec2 pickLater = result.polyline.segment(at).end;
        const auto cut =
            cutVertex(result.polyline, at, true, pickEarlier, pickLater, spec, trim, rule);
        if (cut.outcome != CornerOutcome::Done) {
            ++result.tooShort;
            current = at + 1;
            continue;
        }
        ++result.done;
        current = cut.next;
        if (cut.added) {
            result.added.emplace_back(*cut.added);
        }
    }
    return result;
}

}  // namespace tg::core
