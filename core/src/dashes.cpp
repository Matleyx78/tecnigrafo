// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/dashes.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <type_traits>
#include <utility>

namespace tg::core {
namespace {

struct CurveSegment {
    Vec2 start;
    Vec2 end;
    std::optional<ArcGeometry> arc;
    double length;
};

constexpr double kBoundaryRoundoff = 8.0 * std::numeric_limits<double>::epsilon();

bool shorterThanPattern(double length, double patternLength) {
    return patternLength - length > kBoundaryRoundoff * std::max(length, patternLength);
}

CurveSegment arcSegment(const ArcGeometry& arc) {
    return {startPoint(arc), endPoint(arc), arc, arc.radius * std::abs(arc.sweep)};
}

std::vector<CurveSegment> segmentsOf(const Entity& entity) {
    return std::visit(
        [](const auto& shape) -> std::vector<CurveSegment> {
            using Shape = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<Shape, Polyline>) {
                std::vector<CurveSegment> segments;
                segments.reserve(shape.segmentCount());
                for (std::size_t i = 0; i < shape.segmentCount(); ++i) {
                    const auto segment = shape.segment(i);
                    std::optional<ArcGeometry> arc;
                    if (segment.isArc()) {
                        arc = arcFromBulge(segment.start, segment.end, segment.bulge);
                    }
                    segments.push_back({segment.start, segment.end, arc, segment.length()});
                }
                return segments;
            } else if constexpr (std::is_same_v<Shape, Line>) {
                return {{shape.start, shape.end, std::nullopt, shape.length()}};
            } else if constexpr (std::is_same_v<Shape, Text>) {
                return {};
            } else {
                return {arcSegment(shape.geometry())};
            }
        },
        entity);
}

double angleAtDistance(const CurveSegment& segment, double offset) {
    const auto& arc = *segment.arc;
    return arc.startAngle + std::copysign(offset / arc.radius, arc.sweep);
}

Vec2 pointAtDistance(const CurveSegment& segment, double offset) {
    // Retain exact vertices, including the last point of a bulged segment.
    if (offset <= 0.0) {
        return segment.start;
    }
    if (offset >= segment.length) {
        return segment.end;
    }
    if (segment.arc) {
        return pointAt(*segment.arc, angleAtDistance(segment, offset));
    }
    return segment.start + (segment.end - segment.start) * (offset / segment.length);
}

DashPiece stroke(const CurveSegment& segment, double first, double last) {
    if (!segment.arc) {
        return DashLine{pointAtDistance(segment, first), pointAtDistance(segment, last)};
    }
    double start = angleAtDistance(segment, first);
    double end = angleAtDistance(segment, last);
    if (segment.arc->sweep < 0.0) {
        std::swap(start, end);
    }
    return DashArc{segment.arc->center, segment.arc->radius, normalizeAngle(start),
                   normalizeAngle(end)};
}

DashPiece dotAtDistance(const CurveSegment& segment, double offset) {
    Vec2 direction;
    if (segment.arc) {
        const double angle = angleAtDistance(segment, offset);
        const double sign = std::copysign(1.0, segment.arc->sweep);
        direction = Vec2{-std::sin(angle), std::cos(angle)} * sign;
    } else {
        direction = (segment.end - segment.start) * (1.0 / segment.length);
    }
    return DashPoint{pointAtDistance(segment, offset), direction};
}

class DashBuilder {
public:
    DashBuilder(const LinetypePattern& pattern, double scale) : m_pattern(pattern), m_scale(scale) {
        reset();
    }

    void reset() {
        m_index = 0;
        m_remaining = std::abs(m_pattern.elements.front()) * m_scale;
    }

    bool append(DashPiece piece) {
        if (m_pieces.size() >= kMaxDashPieces) {
            return false;
        }
        m_pieces.push_back(std::move(piece));
        return true;
    }

    bool appendPattern(const CurveSegment& segment, bool includeEndpoint) {
        if (std::ranges::all_of(m_pattern.elements, [](double value) { return value < 0.0; })) {
            return true;
        }
        double offset = 0.0;
        while (offset < segment.length || includeEndpoint) {
            const double element = m_pattern.elements[m_index];
            // Exact classification of a pattern element, not a geometric test.
            if (element == 0.0) {
                if (!append(dotAtDistance(segment, offset))) {
                    return false;
                }
                advance();
                continue;
            }
            const double available = segment.length - offset;
            if (available <= 0.0) {
                break;
            }
            // Snap only roundoff at a boundary, so terminal dots are not lost.
            const bool atBoundary = std::abs(m_remaining - available) <=
                                    kBoundaryRoundoff * std::max(segment.length, m_remaining);
            const double step = atBoundary ? available : std::min(m_remaining, available);
            const double next = std::min(offset + step, segment.length);
            if (next <= offset) {
                return false;  // the next pattern boundary cannot be represented
            }
            if (element > 0.0 && next > offset && !append(stroke(segment, offset, next))) {
                return false;
            }
            if (!atBoundary && m_remaining > available) {
                m_remaining -= available;
                break;
            }
            offset = next;
            advance();
        }
        return true;
    }

    std::vector<DashPiece> takePieces() { return std::move(m_pieces); }

private:
    void advance() {
        m_index = (m_index + 1) % m_pattern.elements.size();
        m_remaining = std::abs(m_pattern.elements[m_index]) * m_scale;
    }

    const LinetypePattern& m_pattern;
    double m_scale;
    std::size_t m_index{0};
    double m_remaining{0.0};
    std::vector<DashPiece> m_pieces;
};

}  // namespace

std::optional<std::vector<DashPiece>> dashes(const Entity& entity, const LinetypePattern& pattern,
                                             double scale) {
    assert(std::isfinite(scale) && scale > 0.0);
    assert(std::ranges::all_of(pattern.elements, [](double v) { return std::isfinite(v); }));
    const double patternLength = pattern.length() * scale;
    if (pattern.continuous() || std::holds_alternative<Text>(entity) ||
        !std::isfinite(patternLength) || patternLength <= 0.0) {
        return std::nullopt;
    }
    // Underflow would turn a nonzero element into a non-advancing step.
    if (std::ranges::any_of(pattern.elements, [scale](double value) {
            return value != 0.0 && std::abs(value) * scale <= 0.0;
        })) {
        return std::nullopt;
    }
    auto segments = segmentsOf(entity);
    std::erase_if(segments, [](const CurveSegment& segment) { return segment.length <= 0.0; });
    const auto* polyline = std::get_if<Polyline>(&entity);
    const bool restart = polyline != nullptr && !polyline->continuousLinetype();
    double total = 0.0;
    bool hasPattern = false;
    for (const auto& segment : segments) {
        total += segment.length;
        hasPattern = hasPattern || !shorterThanPattern(segment.length, patternLength);
    }
    if (!std::isfinite(total) || segments.empty() ||
        (restart ? !hasPattern : shorterThanPattern(total, patternLength))) {
        return std::nullopt;
    }
    DashBuilder builder(pattern, scale);
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto& segment = segments[i];
        if (restart) {
            builder.reset();
        }
        if (restart && shorterThanPattern(segment.length, patternLength)) {
            if (!builder.append(stroke(segment, 0.0, segment.length))) {
                return std::nullopt;
            }
        } else if (!builder.appendPattern(segment, restart || i + 1 == segments.size())) {
            return std::nullopt;
        }
    }
    return builder.takePieces();
}

}  // namespace tg::core
