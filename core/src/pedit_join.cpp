// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/pedit_join.hpp"

#include "solid_space/geom2d/coincidence.hpp"
#include "solid_space/geom2d/intersection.hpp"
#include "solid_space/robust/predicates.hpp"
#include "solid_space/robust/trigonometry.hpp"
#include "solid_space_bridge.hpp"
#include "tg/core/math.hpp"

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

namespace g = bridge::g;
using Policy = solid_space::robust::TolerancePolicy;

// One segment of the chain: the vertex it starts from (bulge and widths) and
// the point where it ends. As in join(), so that fuzz == 0 joins as it does.
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

// The exact direction tests of the Arc contract, as join() makes them.
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

std::vector<Piece> piecesOfPolyline(const Polyline& polyline, const Policy& policy) {
    std::vector<Piece> pieces;
    if (polyline.isClosed()) {
        return {};
    }
    for (std::size_t i = 0; i < polyline.segmentCount(); ++i) {
        const auto segment = polyline.segment(i);
        const auto curve = bridge::pieceOf(segment);
        const auto straight = g::isStraight(curve, policy);
        if (!straight.hasValue()) {
            return {};
        }
        if (segment.start == segment.end) {
            continue;
        }
        auto vertex = polyline.vertices()[i];
        vertex.bulge = straight.value() ? 0.0 : curve.bulge;
        if (vertex.startWidth == 0.0 && vertex.endWidth == 0.0) {
            vertex.startWidth = polyline.constantWidth();
            vertex.endWidth = polyline.constantWidth();
        }
        pieces.push_back({vertex, segment.end});
    }
    return pieces;
}

std::vector<Piece> piecesOf(const Entity& entity, const Policy& policy) {
    if (const auto* polyline = std::get_if<Polyline>(&entity)) {
        return piecesOfPolyline(*polyline, policy);
    }
    if (const auto* line = std::get_if<Line>(&entity)) {
        if (inDomain(line->start) && inDomain(line->end) && line->start != line->end) {
            return {{PolylineVertex{line->start}, line->end}};
        }
        return {};
    }
    if (std::holds_alternative<Arc>(entity)) {
        // The ends and the bulge of the arc as join() takes them, so that
        // fuzz == 0 gives the same polyline.
        const auto curve = bridge::curveOf(entity);
        if (!curve) {
            return {};
        }
        const auto& arc = std::get<g::Arc>(*curve);
        const auto ends = g::endpoints(arc);
        if (!ends.hasValue() || isWholeCircle(arc)) {
            return {};
        }
        const Vec2 start = bridge::fromPoint(ends.value().start);
        const Vec2 end = bridge::fromPoint(ends.value().end);
        if (!inDomain(start) || !inDomain(end)) {
            return {};
        }
        Piece piece{{start, tg::core::tan(arc.sweep / 4.0)}, end};
        const auto straight = g::isStraight(curveOf(piece), policy);
        if (!straight.hasValue()) {
            return {};
        }
        if (straight.value()) {
            piece.start.bulge = 0.0;
        }
        return {piece};
    }
    return {};
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

double gap(Vec2 a, Vec2 b) {
    return tg::core::hypot(b.x - a.x, b.y - a.y);
}

// The bulge of `piece` run from `from` to `to`, one of them its own end and
// the other the new point: on the circle of the piece, in its direction
// (docs/tappe/0.8/PT32, point 3). std::nullopt when nothing is left.
std::optional<double> movedBulge(const Piece& piece, Vec2 from, Vec2 to) {
    if (from == to) {
        return std::nullopt;
    }
    const PolylineSegment segment{piece.start.position, piece.end, piece.start.bulge};
    if (!segment.isArc()) {
        return 0.0;
    }
    const ArcGeometry arc = arcFromBulge(segment.start, segment.end, segment.bulge);
    const double first =
        normalizeAngle(tg::core::atan2(from.y - arc.center.y, from.x - arc.center.x));
    const double last = normalizeAngle(tg::core::atan2(to.y - arc.center.y, to.x - arc.center.x));
    if (first == last) {
        return std::nullopt;
    }
    const double sweep =
        segment.bulge > 0.0 ? normalizeAngle(last - first) : -normalizeAngle(first - last);
    return tg::core::tan(sweep / 4.0);
}

// Extend: `before` ends where `after` starts, at the isolated point of their
// extended curves nearest to the two ends. False when there is none, or a
// segment would shrink to nothing; then neither changes.
bool extendPair(Piece& before, Piece& after) {
    const auto meeting = g::intersect(curveOf(before), g::Extension::extended, curveOf(after),
                                      g::Extension::extended);
    if (!meeting.hasValue()) {
        return false;
    }
    const auto& found = meeting.value();
    if (!found.overlaps.empty() || found.points.empty()) {
        return false;
    }
    const auto sumOf = [&](Vec2 candidate) {
        return gap(candidate, before.end) + gap(candidate, after.start.position);
    };
    // On equal sums the first that solid-space gives.
    Vec2 point = bridge::fromPoint(found.points.front().point);
    double best = sumOf(point);
    for (const auto& meetingPoint : found.points) {
        const Vec2 candidate = bridge::fromPoint(meetingPoint.point);
        const double sum = sumOf(candidate);
        if (sum < best) {
            point = candidate;
            best = sum;
        }
    }
    const auto beforeBulge = movedBulge(before, before.start.position, point);
    const auto afterBulge = movedBulge(after, point, after.end);
    if (!beforeBulge || !afterBulge) {
        return false;
    }
    before.end = point;
    before.start.bulge = *beforeBulge;
    after.start.position = point;
    after.start.bulge = *afterBulge;
    return true;
}

// How one object attaches to a free end of the chain.
struct Attachment {
    std::size_t index{0};
    bool atStart{true};  // by the start of the object
    bool coincident{false};
    double distance{0.0};
};

// The objects that may attach to `free`, in the order they are tried.
std::vector<Attachment> ranking(Vec2 free, const std::vector<std::vector<Piece>>& candidates,
                                double fuzz, const ToleranceRule& rule) {
    std::vector<Attachment> coincident;
    std::vector<Attachment> near;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const auto& candidate = candidates[i];
        if (candidate.empty()) {
            continue;
        }
        const Vec2 first = candidate.front().start.position;
        const Vec2 last = candidate.back().end;
        if (rule.coincident(free, first) || rule.coincident(free, last)) {
            coincident.push_back({i, rule.coincident(free, first), true, 0.0});
            continue;
        }
        const double toFirst = gap(free, first);
        const double toLast = gap(free, last);
        const bool atStart = toFirst <= toLast;
        const double nearer = atStart ? toFirst : toLast;
        if (nearer <= fuzz) {
            near.push_back({i, atStart, false, nearer});
        }
    }
    std::ranges::stable_sort(near, {}, &Attachment::distance);
    coincident.insert(coincident.end(), near.begin(), near.end());
    return coincident;
}

// The chain with the object of `attachment` joined at its end (`tail`) or at
// its start, or std::nullopt when the pair does not join. `added` gets the
// segments that enter it.
std::optional<std::vector<Piece>> attach(const std::vector<Piece>& chain,
                                         const std::vector<Piece>& candidate,
                                         const Attachment& attachment, bool tail, JoinType type,
                                         std::size_t& added) {
    auto oriented = candidate;
    if (attachment.atStart != tail) {
        reverse(oriented);
    }
    std::vector<Piece> joined = tail ? chain : oriented;
    const auto& following = tail ? oriented : chain;
    const std::size_t meeting = joined.size();
    joined.insert(joined.end(), following.begin(), following.end());
    std::size_t bridges = 0;
    if (attachment.coincident) {
        joined[meeting - 1].end = joined[meeting].start.position;
    } else {
        bool extended = false;
        if (type != JoinType::Add) {
            extended = extendPair(joined[meeting - 1], joined[meeting]);
        }
        if (!extended) {
            if (type == JoinType::Extend) {
                return std::nullopt;
            }
            const Piece bridge{PolylineVertex{joined[meeting - 1].end},
                               joined[meeting].start.position};
            joined.insert(joined.begin() + static_cast<std::ptrdiff_t>(meeting), bridge);
            bridges = 1;
        }
    }
    added = oriented.size() + bridges;
    return joined;
}

}  // namespace

GapJoinResult joinWithGaps(const Polyline& source, std::span<const Entity> others, double fuzz,
                           JoinType type, const ToleranceRule& rule) {
    assert(std::isfinite(fuzz) && fuzz >= 0.0);
    GapJoinResult result{source, {}, 0};
    const auto policy = bridge::policyOf(rule);
    auto chain = piecesOfPolyline(source, policy);
    if (!checkPieces(chain)) {
        return result;
    }
    std::vector<std::vector<Piece>> candidates;
    candidates.reserve(others.size());
    for (const auto& entity : others) {
        candidates.push_back(piecesOf(entity, policy));
    }

    bool closed = false;
    bool tailJoined = false;
    for (const bool tail : {true, false}) {
        while (!closed) {
            const Vec2 free = tail ? chain.back().end : chain.front().start.position;
            bool joinedOne = false;
            for (const auto& attachment : ranking(free, candidates, fuzz, rule)) {
                std::size_t added = 0;
                auto joined =
                    attach(chain, candidates[attachment.index], attachment, tail, type, added);
                if (!joined) {
                    continue;
                }
                const Vec2 first = joined->front().start.position;
                const bool closes = rule.coincident(first, joined->back().end);
                if (closes) {
                    joined->back().end = first;
                }
                if (!checkPieces(*joined)) {
                    continue;
                }
                closed = closes;
                chain = std::move(*joined);
                candidates[attachment.index].clear();
                result.consumed.push_back(attachment.index);
                result.added += added;
                tailJoined = tailJoined || tail;
                joinedOne = true;
                break;
            }
            if (!joinedOne) {
                break;
            }
        }
    }

    if (result.consumed.empty()) {
        return result;
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
        // Nothing attached at the end: its vertex keeps its data, which belong
        // to no segment (docs/adr/0006).
        PolylineVertex last = tailJoined ? PolylineVertex{} : source.vertices().back();
        last.position = chain.back().end;
        vertices.push_back(last);
    }
    Polyline polyline{std::move(vertices), closed};
    if (constantWidth) {
        polyline.setConstantWidth(width);
    }
    polyline.setContinuousLinetype(source.continuousLinetype());
    result.polyline = std::move(polyline);
    return result;
}

}  // namespace tg::core
