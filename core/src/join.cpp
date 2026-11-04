// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/join.hpp"

#include "solid_space_bridge.hpp"
#include "solid_space/geom2d/coincidence.hpp"
#include "solid_space/robust/predicates.hpp"
#include "solid_space/robust/trigonometry.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace tg::core {
namespace {

namespace g = bridge::g;
using Policy = solid_space::robust::TolerancePolicy;

struct Piece {
    PolylineVertex start;
    Vec2 end;
};

bool inDomain(Vec2 point) {
    using solid_space::robust::kMaxCoordinate;
    return std::abs(point.x) <= kMaxCoordinate && std::abs(point.y) <= kMaxCoordinate;
}

g::PolylineSegment curveOf(const Piece& piece) {
    return {bridge::toPoint(piece.start.position), bridge::toPoint(piece.end), piece.start.bulge};
}

// Use the exact direction tests in the Arc contract, rather than
// comparing the rounded endpoint positions.
bool isWholeCircle(const g::Arc& arc) {
    if (arc.sweep >= g::kFullTurn) {
        return true;
    }
    if (arc.sweep < g::kThreeQuarterTurn) {
        return false;
    }
    namespace robust = solid_space::robust;
    const auto first = robust::sinCos(arc.startAngle);
    const auto last = robust::sinCos(arc.startAngle + arc.sweep);
    const g::Point2 end{last.cosine, last.sine};
    return robust::orient2d({}, {first.cosine, first.sine}, end) != robust::Sign::negative &&
           robust::orient2d({}, {first.sine, -first.cosine}, end) != robust::Sign::negative;
}

std::optional<g::Endpoints> arcEnds(const g::Arc& arc) {
    const auto ends = g::endpoints(arc);
    if (!ends.hasValue() || isWholeCircle(arc)) {
        return std::nullopt;
    }
    const auto& result = ends.value();
    if (!inDomain(bridge::fromPoint(result.start)) || !inDomain(bridge::fromPoint(result.end))) {
        return std::nullopt;
    }
    return result;
}

std::vector<Piece> piecesOf(const Entity& entity, const Policy& policy) {
    std::vector<Piece> pieces;
    if (const auto* polyline = std::get_if<Polyline>(&entity)) {
        if (polyline->isClosed()) {
            return {};
        }
        for (std::size_t i = 0; i < polyline->segmentCount(); ++i) {
            const auto segment = polyline->segment(i);
            const auto curve = bridge::pieceOf(segment);
            const auto straight = g::isStraight(curve, policy);
            if (!straight.hasValue()) {
                return {};
            }
            if (segment.start == segment.end) {
                continue;
            }
            auto vertex = polyline->vertices()[i];
            vertex.bulge = straight.value() ? 0.0 : curve.bulge;
            if (vertex.startWidth == 0.0 && vertex.endWidth == 0.0) {
                vertex.startWidth = polyline->constantWidth();
                vertex.endWidth = polyline->constantWidth();
            }
            pieces.push_back({vertex, segment.end});
        }
    } else if (const auto* line = std::get_if<Line>(&entity)) {
        if (inDomain(line->start) && inDomain(line->end) && line->start != line->end) {
            pieces.push_back({PolylineVertex{line->start}, line->end});
        }
    } else if (std::holds_alternative<Arc>(entity)) {
        const auto curve = bridge::curveOf(entity);
        if (!curve) {
            return {};
        }
        const auto& arc = std::get<g::Arc>(*curve);
        if (const auto ends = arcEnds(arc)) {
            Piece piece{{bridge::fromPoint(ends->start), std::tan(arc.sweep / 4.0)},
                        bridge::fromPoint(ends->end)};
            const auto straight = g::isStraight(curveOf(piece), policy);
            if (straight.hasValue()) {
                if (straight.value()) {
                    piece.start.bulge = 0.0;
                }
                pieces.push_back(piece);
            }
        }
    }
    return pieces;
}

void reverse(std::vector<Piece>& pieces) {
    std::ranges::reverse(pieces);
    for (auto& piece : pieces) {
        std::swap(piece.start.position, piece.end);
        std::swap(piece.start.startWidth, piece.start.endWidth);
        piece.start.bulge = -piece.start.bulge;
    }
}

bool checkPieces(std::vector<Piece>& pieces) {
    std::erase_if(pieces, [](const Piece& piece) { return piece.start.position == piece.end; });
    return !pieces.empty() && std::ranges::all_of(pieces, [](const Piece& piece) {
        return g::magnitude(curveOf(piece)).hasValue();
    });
}

std::optional<JoinResult> joinChain(std::span<const Entity> entities, const ToleranceRule& rule,
                                    const Policy& policy) {
    auto chain = piecesOf(entities.front(), policy);
    if (!checkPieces(chain)) {
        return std::nullopt;
    }
    std::vector<std::vector<Piece>> candidates;
    candidates.reserve(entities.size() - 1);
    for (const auto& entity : entities.subspan(1)) {
        candidates.push_back(piecesOf(entity, policy));
    }

    JoinResult result;
    bool closed = false;
    for (const bool tail : {true, false}) {
        while (!closed) {
            bool added = false;
            const Vec2 free = tail ? chain.back().end : chain.front().start.position;
            for (std::size_t i = 0; i < candidates.size(); ++i) {
                const auto& candidate = candidates[i];
                if (candidate.empty()) {
                    continue;
                }
                const bool atStart = rule.coincident(free, candidate.front().start.position);
                if (!atStart && !rule.coincident(free, candidate.back().end)) {
                    continue;
                }
                auto oriented = candidate;
                if (atStart != tail) {
                    reverse(oriented);
                }
                auto joined = tail ? chain : oriented;
                const auto& following = tail ? oriented : chain;
                joined.back().end = following.front().start.position;
                joined.insert(joined.end(), following.begin(), following.end());

                const bool closes = rule.coincident(joined.front().start.position, joined.back().end);
                if (closes) {
                    joined.back().end = joined.front().start.position;
                }
                if (!checkPieces(joined)) {
                    continue;
                }
                closed = closes;
                chain = std::move(joined);
                candidates[i].clear();
                result.consumed.push_back(i + 1);
                added = true;
                break;
            }
            if (!added) {
                break;
            }
        }
    }

    if (result.consumed.empty()) {
        return std::nullopt;
    }
    std::vector<PolylineVertex> vertices;
    vertices.reserve(chain.size() + (closed ? 0 : 1));
    const double width = chain.front().start.startWidth;
    const bool constantWidth = std::ranges::all_of(chain, [width](const Piece& piece) {
        return piece.start.startWidth == width && piece.start.endWidth == width;
    });
    for (const auto& piece : chain) {
        vertices.push_back(piece.start);
    }
    if (!closed) {
        vertices.push_back(PolylineVertex{chain.back().end});
    }
    Polyline polyline{std::move(vertices), closed};
    if (constantWidth) {
        polyline.setConstantWidth(width);
    }
    if (const auto* source = std::get_if<Polyline>(&entities.front())) {
        polyline.setContinuousLinetype(source->continuousLinetype());
        result.source = std::move(polyline);
    } else {
        result.created = std::move(polyline);
    }
    return result;
}

std::optional<JoinResult> joinLines(std::span<const Entity> entities, const Policy& policy,
                                    bool sourceOnly) {
    const auto& line = std::get<Line>(entities.front());
    const g::Segment source{bridge::toPoint(line.start), bridge::toPoint(line.end)};
    std::vector<g::Segment> others;
    JoinResult result;
    for (std::size_t i = 1; i < entities.size(); ++i) {
        if (const auto* other = std::get_if<Line>(&entities[i])) {
            const g::Segment segment{bridge::toPoint(other->start), bridge::toPoint(other->end)};
            const auto aligned = g::areAligned(source, segment, policy);
            if (aligned.hasValue() && aligned.value()) {
                others.push_back(segment);
                result.consumed.push_back(i);
                continue;
            }
        }
        if (!sourceOnly) {
            return std::nullopt;
        }
    }
    if (others.empty()) {
        return std::nullopt;
    }
    const auto joined = g::joinAligned(source, others);
    if (!joined.hasValue()) {
        return std::nullopt;
    }
    result.source = Line{bridge::fromPoint(joined.value().start),
                         bridge::fromPoint(joined.value().end)};
    return result;
}

std::optional<JoinResult> joinArcs(std::span<const Entity> entities, const Policy& policy,
                                   bool sourceOnly, bool close) {
    const auto sourceCurve = bridge::curveOf(entities.front());
    if (!sourceCurve) {
        return std::nullopt;
    }
    const auto& source = std::get<Arc>(entities.front());
    const auto& arc = std::get<g::Arc>(*sourceCurve);
    if (!arcEnds(arc)) {
        return std::nullopt;
    }

    JoinResult result;
    double farthest = arc.sweep;
    for (std::size_t i = 1; i < entities.size(); ++i) {
        if (std::holds_alternative<Arc>(entities[i])) {
            const auto other = bridge::curveOf(entities[i]);
            if (other && arcEnds(std::get<g::Arc>(*other))) {
                const auto same = g::onSameCircle(*sourceCurve, *other, policy);
                if (same.hasValue() && same.value()) {
                    const auto& candidate = std::get<g::Arc>(*other);
                    const double start = normalizeAngle(candidate.startAngle - arc.startAngle);
                    farthest = std::max(farthest, start + candidate.sweep);
                    result.consumed.push_back(i);
                    continue;
                }
            }
        }
        if (!sourceOnly) {
            return std::nullopt;
        }
    }
    if (!close && result.consumed.empty()) {
        return std::nullopt;
    }
    const double end = normalizeAngle(arc.startAngle + farthest);
    if (close || farthest >= g::kFullTurn || end == arc.startAngle) {
        result.created = Circle{source.center, source.radius};
        result.formsCircle = !close;
    } else {
        result.source = Arc{source.center, source.radius, source.startAngle, end};
    }
    return result;
}

}  // namespace

std::optional<JoinResult> join(std::span<const Entity> entities, const ToleranceRule& rule) {
    if (entities.size() < 2) {
        return std::nullopt;
    }
    const auto policy = bridge::policyOf(rule);
    if (std::holds_alternative<Line>(entities.front())) {
        if (auto result = joinLines(entities, policy, false)) {
            return result;
        }
    } else if (std::holds_alternative<Arc>(entities.front())) {
        if (auto result = joinArcs(entities, policy, false, false)) {
            return result;
        }
    }
    return joinChain(entities, rule, policy);
}

std::optional<JoinResult> joinToSource(std::span<const Entity> entities, const ToleranceRule& rule,
                                       bool close) {
    if (entities.empty()) {
        return std::nullopt;
    }
    const auto policy = bridge::policyOf(rule);
    if (std::holds_alternative<Line>(entities.front())) {
        return joinLines(entities, policy, true);
    }
    if (std::holds_alternative<Arc>(entities.front())) {
        return joinArcs(entities, policy, true, close);
    }
    return joinChain(entities, rule, policy);
}

}  // namespace tg::core
