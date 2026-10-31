// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/grips.hpp"

#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"

#include <cassert>
#include <cmath>
#include <optional>
#include <type_traits>
#include <utility>

namespace tg::core {
namespace {

Vec2 midpoint(const PolylineSegment& segment) {
    if (segment.isArc()) {
        const auto arc = arcFromBulge(segment.start, segment.end, segment.bulge);
        return pointAt(arc, arc.startAngle + arc.sweep / 2.0);
    }
    return (segment.start + segment.end) * 0.5;
}

bool twoPointText(const Text& text) {
    return text.horizontal == TextHorizontal::Aligned || text.horizontal == TextHorizontal::Fit;
}

std::optional<Polyline> movedPoly(const Polyline& input, std::size_t index, const Vec2& target) {
    std::vector<PolylineVertex> vertices(input.vertices().begin(), input.vertices().end());
    const std::size_t vertex = index / 2;
    const bool segmentGrip = index % 2 != 0;
    const bool curvedGrip = segmentGrip && input.segment(vertex).isArc();
    const std::size_t next = (vertex + 1) % vertices.size();
    if (!segmentGrip) {
        vertices[vertex].position = target;
    } else if (!curvedGrip) {
        const Vec2 delta = target - midpoint(input.segment(vertex));
        vertices[vertex].position += delta;
        vertices[next].position += delta;
    }
    for (std::size_t i = 0; i < input.segmentCount(); ++i) {
        const auto old = input.segment(i);
        const std::size_t end = (i + 1) % vertices.size();
        const Vec2 startPointNew = vertices[i].position;
        const Vec2 endPointNew = vertices[end].position;
        const bool changed =
            distance(startPointNew, old.start) > 0.0 || distance(endPointNew, old.end) > 0.0;
        if (!changed && !(curvedGrip && i == vertex)) {
            continue;
        }
        if (!(distance(startPointNew, endPointNew) > 0.0)) {
            return std::nullopt;
        }
        if (old.isArc()) {
            const Vec2 middle = curvedGrip && i == vertex ? target : midpoint(old);
            const auto arc = arcThroughPoints(startPointNew, middle, endPointNew);
            if (!arc) {
                return std::nullopt;
            }
            // arcThroughPoints stores CCW geometry; retain the segment's
            // traversal direction, including a possible reversal at the grip.
            const double direction = cross(middle - startPointNew, endPointNew - startPointNew);
            const double sweep = arc->geometry().sweep;
            vertices[i].bulge = std::tan((direction > 0.0 ? sweep : -sweep) / 4.0);
        }
    }
    Polyline result{std::move(vertices), input.isClosed()};
    result.setConstantWidth(input.constantWidth());
    result.setContinuousLinetype(input.continuousLinetype());
    return result;
}

}  // namespace

std::vector<Vec2> gripPoints(const Entity& entity) {
    return std::visit(
        [](const auto& value) -> std::vector<Vec2> {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, Line>) {
                return {value.start, value.end, (value.start + value.end) * 0.5};
            } else if constexpr (std::is_same_v<T, Arc>) {
                const auto arc = value.geometry();
                return {startPoint(arc), endPoint(arc),
                        pointAt(arc, arc.startAngle + arc.sweep / 2.0), value.center};
            } else if constexpr (std::is_same_v<T, Circle>) {
                const auto c = value.center;
                const double r = value.radius;
                return {c, c + Vec2{r, 0.0}, c + Vec2{0.0, r}, c + Vec2{-r, 0.0},
                        c + Vec2{0.0, -r}};
            } else if constexpr (std::is_same_v<T, Text>) {
                if (twoPointText(value)) {
                    return {value.position, value.alignmentPoint};
                }
                return {textFrame(value).insertion};
            } else {
                std::vector<Vec2> points;
                for (std::size_t i = 0; i < value.vertexCount(); ++i) {
                    points.push_back(value.vertices()[i].position);
                    if (i < value.segmentCount()) {
                        points.push_back(midpoint(value.segment(i)));
                    }
                }
                return points;
            }
        },
        entity);
}

GripResult moveGrip(const Entity& entity, std::size_t index, const Vec2& target) {
    const auto points = gripPoints(entity);
    assert(index < points.size());
    const GripResult invalid{GripOutcome::Invalid, entity};
    if (!std::isfinite(target.x) || !std::isfinite(target.y)) {
        return invalid;
    }
    const auto translate = [&] {
        return GripResult{GripOutcome::Moved,
                          transformed(entity, Similarity::translation(target - points[index]))};
    };
    return std::visit(
        [&](const auto& value) -> GripResult {
            using T = std::decay_t<decltype(value)>;
            auto result = value;
            if constexpr (std::is_same_v<T, Line>) {
                if (index == 2) {
                    return translate();
                }
                (index == 0 ? result.start : result.end) = target;
                if (!(distance(result.start, result.end) > 0.0)) {
                    return invalid;
                }
            } else if constexpr (std::is_same_v<T, Circle>) {
                if (index == 0) {
                    return translate();
                }
                result.radius = distance(value.center, target);
                if (!(result.radius > 0.0) || !std::isfinite(result.radius)) {
                    return invalid;
                }
            } else if constexpr (std::is_same_v<T, Arc>) {
                if (index == 3) {
                    return translate();
                }
                auto moved = points;
                moved[index] = target;
                const auto arc = arcThroughPoints(moved[0], moved[2], moved[1]);
                if (!arc) {
                    return invalid;
                }
                result = *arc;
            } else if constexpr (std::is_same_v<T, Text>) {
                if (index == 0) {
                    return translate();
                }
                result.alignmentPoint = target;
                if (!(distance(result.position, target) > 0.0)) {
                    return invalid;
                }
                const auto frame = textFrame(result);
                result.rotation = frame.rotation;
                if (result.horizontal == TextHorizontal::Aligned) {
                    result.height = frame.height;
                } else {
                    result.widthFactor = frame.widthFactor;
                }
            } else {
                const auto polyline = movedPoly(value, index, target);
                if (!polyline) {
                    return invalid;
                }
                result = *polyline;
            }
            return {GripOutcome::Moved, std::move(result)};
        },
        entity);
}

}  // namespace tg::core
