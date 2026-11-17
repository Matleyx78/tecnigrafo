// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/hatch.hpp"

#include "solid_space/robust/point.hpp"
#include "solid_space_bridge.hpp"
#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/tolerance.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {
// The message of the precondition supported() (docs/adr/0044, point 1);
// unused in Release, where assertions are absent.
[[maybe_unused]] constexpr const char* kNotSupported = "this Hatch is not supported";

/// An edge of a loop as the loop runs it (docs/tappe/0.8/PT30-retini-core.md,
/// point 3): the point it starts from, the point it ends at, and its bulge,
/// negative for an arc run clockwise.
struct RunEdge {
    Vec2 start;
    Vec2 end;
    double bulge{0.0};
};

/// The edge as it is written: a line from its first point to its second, an
/// arc from its start angle to its end angle, with the end points and the
/// bulge of toPolyline().
RunEdge writtenRun(const HatchEdge& edge) {
    if (const auto* line = std::get_if<Line>(&edge.curve)) {
        return {line->start, line->end, 0.0};
    }
    const Polyline polyline = toPolyline(std::get<Arc>(edge.curve));
    const auto vertices = polyline.vertices();
    return {vertices[0].position, vertices[1].position, vertices[0].bulge};
}

RunEdge reversedRun(const RunEdge& run) {
    return {run.end, run.start, -run.bulge};
}

/// The distance from `point` to the nearer end of `run`.
double distanceToEnds(const Vec2& point, const RunEdge& run) {
    return std::min(distance(point, run.start), distance(point, run.end));
}

/// The edges of a loop as the loop runs them (point 3): the first ends at its
/// end nearer to the second edge, every other one starts from its end nearer
/// to where the previous one ends; on a tie, the direction it is written in.
/// A single edge runs as it is written.
std::vector<RunEdge> runEdges(const std::vector<HatchEdge>& edges) {
    std::vector<RunEdge> runs;
    runs.reserve(edges.size());
    for (const HatchEdge& edge : edges) {
        runs.push_back(writtenRun(edge));
    }
    if (runs.size() < 2) {
        return runs;
    }
    if (distanceToEnds(runs[0].start, runs[1]) < distanceToEnds(runs[0].end, runs[1])) {
        runs[0] = reversedRun(runs[0]);
    }
    for (std::size_t i = 1; i < runs.size(); ++i) {
        const Vec2 previous = runs[i - 1].end;
        if (distance(runs[i].end, previous) < distance(runs[i].start, previous)) {
            runs[i] = reversedRun(runs[i]);
        }
    }
    return runs;
}

/// True for an arc of a whole turn: the circle= of the report.
bool isWholeTurn(const HatchEdge& edge) {
    const auto* arc = std::get_if<Arc>(&edge.curve);
    return arc != nullptr && arc->geometry().sweep == kTwoPi;
}

/// The closed polyline of an edge loop (point 3).
Polyline edgePolyline(const std::vector<HatchEdge>& edges) {
    // A half circle has bulge tan(pi / 4) = 1.
    constexpr double kHalfTurnBulge = 1.0;
    const std::vector<RunEdge> runs = runEdges(edges);
    std::vector<PolylineVertex> vertices;
    if (runs.size() == 1) {
        const RunEdge& run = runs.front();
        if (isWholeTurn(edges.front())) {
            const Vec2 center = std::get<Arc>(edges.front().curve).center;
            vertices = {{run.start, kHalfTurnBulge}, {(center * 2.0) - run.start, kHalfTurnBulge}};
        } else {
            vertices = {{run.start, run.bulge}, {run.end, 0.0}};
        }
        return Polyline{std::move(vertices), true};
    }
    vertices.reserve(runs.size());
    for (const RunEdge& run : runs) {
        vertices.push_back({run.start, run.bulge});
    }
    return Polyline{std::move(vertices), true};
}

/// The closed polyline of a loop: its own, or that of its edges.
Polyline loopPolyline(const HatchLoop& loop) {
    if (loop.polyline) {
        return *loop.polyline;
    }
    return edgePolyline(loop.edges);
}

/// True when solid-space accepts the data of the edge (docs/adr/0035,
/// point 23): finite values, within its domain.
bool inDomain(const HatchEdge& edge) {
    const std::optional<bridge::g::Curve> curve =
        std::visit([](const auto& value) { return bridge::curveOf(Entity{value}); }, edge.curve);
    return curve && bridge::g::magnitude(*curve).hasValue();
}

/// True when ToleranceRule::coincident() can compare the point: finite
/// coordinates of magnitude at most 2^50. An edge within the domain can have
/// an end beyond it (a centre near the limit and a large radius).
bool comparable(const Vec2& point) {
    const auto within = [](double value) {
        return std::isfinite(value) && std::abs(value) <= solid_space::robust::kMaxCoordinate;
    };
    return within(point.x) && within(point.y);
}

bool onFrozenLayer(const Document& document, const EntityRecord& record) {
    const Layer* layer = document.layer(record.properties.layer);
    return layer != nullptr && layer->frozen;
}

/// The arc transformed; a whole turn stays one, from 0 to 2*pi, which
/// transformed() would otherwise turn into an arc just short of it.
Arc transformedArc(const Arc& arc, const Similarity& similarity) {
    if (arc.geometry().sweep == kTwoPi) {
        return Arc{similarity.apply(arc.center), arc.radius * similarity.scale(), 0.0, kTwoPi};
    }
    return std::get<Arc>(transformed(Entity{arc}, similarity));
}

/// The angle of the hatch or of a pattern line after `similarity` (point 4).
double transformedAngle(double angle, const Similarity& similarity) {
    return similarity.reflects() ? similarity.rotationAngle() - angle
                                 : angle + similarity.rotationAngle();
}

}  // namespace

bool supported(const Hatch& hatch) {
    return std::ranges::all_of(hatch.loops, [](const HatchLoop& loop) {
        if (loop.polyline) {
            return loop.polyline->isClosed() && loop.polyline->vertexCount() > 0;
        }
        return !loop.edges.empty();
    });
}

bool isClosed(const HatchLoop& loop, const ToleranceRule& rule) {
    if (loop.polyline) {
        return loop.polyline->isClosed();
    }
    if (loop.edges.empty() || !std::ranges::all_of(loop.edges, inDomain)) {
        return false;
    }
    const std::vector<RunEdge> runs = runEdges(loop.edges);
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const Vec2& end = runs[i].end;
        const Vec2& next = runs[(i + 1) % runs.size()].start;
        if (!comparable(end) || !comparable(next) || !rule.coincident(end, next)) {
            return false;
        }
    }
    return true;
}

std::vector<GeneratedItem> generateOf(const Document& document, const EntityRecord& record,
                                      const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    if (!hatch.solid || hatch.islands != IslandStyle::Normal || onFrozenLayer(document, record)) {
        // Patterns and the other island styles: docs/tappe/0.8, PT42 and PT43.
        return {};
    }
    FilledRegion region;
    region.loops.reserve(hatch.loops.size());
    for (const HatchLoop& loop : hatch.loops) {
        region.loops.push_back(loopPolyline(loop));
    }
    return {GeneratedItem{std::move(region), effectiveProperties(document, record)}};
}

BBox extentOf(const Document& document, const EntityRecord& record, const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    BBox box;
    if (onFrozenLayer(document, record)) {
        return box;
    }
    for (const HatchLoop& loop : hatch.loops) {
        box.expand(loopPolyline(loop).boundingBox());
    }
    return box;
}

double distanceOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                  const Vec2& point) {
    assert(supported(hatch) && kNotSupported);
    return distanceToDrawnItems(generateOf(document, record, hatch), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                const BBox& rectangle) {
    assert(supported(hatch) && kNotSupported);
    return drawnItemsInside(generateOf(document, record, hatch), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                  const BBox& rectangle) {
    assert(supported(hatch) && kNotSupported);
    return drawnItemsIntersect(generateOf(document, record, hatch), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           [[maybe_unused]] const Hatch& hatch, const SnapModes& /*modes*/,
                           std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(hatch) && kNotSupported);
}

TransformOutcome transformedOf(const Document& /*document*/, const Hatch& hatch,
                               const Similarity& similarity, MirrorText /*mirrorText*/) {
    assert(supported(hatch) && kNotSupported);
    const double factor = similarity.scale();
    const Transform2D linear = similarity.matrix();
    Hatch result = hatch;
    result.angle = transformedAngle(hatch.angle, similarity);
    result.scale = hatch.scale * factor;
    for (PatternLine& line : result.patternLines) {
        line.angle = transformedAngle(line.angle, similarity);
        line.origin = similarity.apply(line.origin);
        line.offset = linear.applyToVector(line.offset);
        for (double& dash : line.dashes) {
            dash *= factor;
        }
    }
    for (Vec2& seed : result.seeds) {
        seed = similarity.apply(seed);
    }
    for (HatchLoop& loop : result.loops) {
        if (loop.polyline) {
            loop.polyline = std::get<Polyline>(transformed(Entity{*loop.polyline}, similarity));
        }
        for (HatchEdge& edge : loop.edges) {
            if (auto* line = std::get_if<Line>(&edge.curve)) {
                *line = Line{similarity.apply(line->start), similarity.apply(line->end)};
            } else {
                edge.curve = transformedArc(std::get<Arc>(edge.curve), similarity);
            }
        }
    }
    return {Entity{std::move(result)}, {}};
}

std::vector<Vec2> gripPointsOf([[maybe_unused]] const Hatch& hatch) {
    assert(supported(hatch) && kNotSupported);
    return {};
}

GripResult moveGripOf(const Document& /*document*/, const Hatch& hatch, std::size_t /*index*/,
                      const Vec2& /*target*/) {
    // Never reached: a hatch has no grip, so no index is valid.
    assert(supported(hatch) && kNotSupported);
    return {GripOutcome::Invalid, Entity{hatch}, {}};
}

}  // namespace tg::core
