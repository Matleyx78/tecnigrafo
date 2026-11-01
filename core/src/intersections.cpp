// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/intersections.hpp"

#include "solid_space/geom2d/intersection.hpp"
#include "solid_space/geom2d/side.hpp"
#include "solid_space_bridge.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/math.hpp"
#include "tg/core/proximity.hpp"

#include <algorithm>
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

namespace g = bridge::g;

constexpr std::size_t kNone = std::numeric_limits<std::size_t>::max();

// ---------------------------------------------------------------------------
// The curves of an entity, with the part of each that takes part

/// Which points off a piece belong to the entity (docs/adr/0035, point 1):
/// the points before its start, after its end, or neither.
struct Reach {
    bool before{false};
    bool after{false};

    [[nodiscard]] bool extended() const { return before || after; }
};

/// The pieces of an entity, in order, and how far each reaches.
struct EntityCurves {
    std::vector<g::Curve> pieces;
    std::vector<Reach> reach;  ///< as many as pieces
    /// For a polyline: segment i is followed by i + 1, and the last by 0 when
    /// closed. Other entities have a single piece.
    bool closed{false};
    /// Some pieces reach beyond themselves and others do not: an open
    /// polyline of more than one segment, extended.
    bool mixed{false};
    /// Pieces that are exactly a point: solid-space refuses them and they
    /// meet nothing (they do not separate their neighbours).
    std::vector<bool> isPoint;

    [[nodiscard]] std::size_t size() const { return pieces.size(); }

    [[nodiscard]] bool empty() const { return pieces.empty(); }

    /// The extension of piece i in a call where it is the host.
    [[nodiscard]] g::Extension extensionOf(std::size_t i) const {
        return reach[i].extended() ? g::Extension::extended : g::Extension::none;
    }

    /// The single extension of all the pieces in a call where they are the
    /// cutters: extended when any reaches beyond itself; the points that
    /// this adds to the others are filtered out by allows().
    [[nodiscard]] g::Extension cutterExtension() const {
        return std::ranges::any_of(reach, &Reach::extended) ? g::Extension::extended
                                                            : g::Extension::none;
    }

    /// Whether a point at `location` of piece i belongs to the entity.
    [[nodiscard]] bool allows(std::size_t i, g::Location location) const {
        switch (location) {
            case g::Location::beforeStart:
                return reach[i].before;
            case g::Location::afterEnd:
                return reach[i].after;
            case g::Location::atStart:
            case g::Location::inside:
            case g::Location::atEnd:
                return true;
        }
        return false;
    }

    /// The piece that follows piece i, skipping pieces that are exactly a
    /// point; kNone at the end of an open polyline.
    [[nodiscard]] std::size_t next(std::size_t i) const {
        for (std::size_t step = 1; step < size(); ++step) {
            const std::size_t j = i + step;
            if (j >= size() && !closed) {
                return kNone;
            }
            const std::size_t k = j % size();
            if (!isPoint[k]) {
                return k;
            }
        }
        return kNone;
    }

    /// The piece before piece i, in the same way.
    [[nodiscard]] std::size_t previous(std::size_t i) const {
        for (std::size_t step = 1; step < size(); ++step) {
            if (step > i && !closed) {
                return kNone;
            }
            const std::size_t k = (i + size() - step) % size();
            if (!isPoint[k]) {
                return k;
            }
        }
        return kNone;
    }
};

bool isExactlyAPoint(const g::Curve& curve) {
    const auto ends = g::endpoints(curve);
    return !ends.hasValue() && ends.diagnosis().code == g::DiagnosisCode::degenerateCurve;
}

EntityCurves curvesOf(const Entity& entity, Extension extension) {
    EntityCurves curves;
    const bool extend = extension == Extension::Extended;
    if (const auto* polyline = std::get_if<Polyline>(&entity)) {
        for (const g::PolylineSegment& piece : bridge::piecesOf(*polyline)) {
            curves.pieces.emplace_back(piece);
        }
        curves.reach.resize(curves.pieces.size());
        curves.closed = polyline->isClosed();
    } else if (std::optional<g::Curve> curve = bridge::curveOf(entity)) {
        const bool isCircle = std::holds_alternative<g::Circle>(*curve);
        curves.pieces.push_back(*curve);
        curves.reach.push_back(extend && !isCircle ? Reach{true, true} : Reach{});
    }
    curves.isPoint.reserve(curves.size());
    for (const g::Curve& piece : curves.pieces) {
        curves.isPoint.push_back(isExactlyAPoint(piece));
    }
    if (std::holds_alternative<Polyline>(entity) && extend && !curves.closed) {
        // The first and the last segment that are not exactly a point: a
        // piece that is a point meets nothing, so it cannot carry the
        // extension. Before the start of the first: its line, or the rest of
        // its circle (afterEnd in solid-space); after the end of the last.
        std::vector<std::size_t> segments;
        for (std::size_t i = 0; i < curves.size(); ++i) {
            if (!curves.isPoint[i]) {
                segments.push_back(i);
            }
        }
        if (!segments.empty()) {
            const std::size_t first = segments.front();
            if (std::get<g::PolylineSegment>(curves.pieces[first]).bulge == 0.0) {
                curves.reach[first].before = true;
            } else {
                curves.reach[first].after = true;
            }
            curves.reach[segments.back()].after = true;
            curves.mixed = segments.size() > 1;
        }
    }
    return curves;
}

// ---------------------------------------------------------------------------
// From the answers of solid-space to the types of Tecnigrafo

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

Contact contactOf(g::Contact contact) {
    switch (contact) {
        case g::Contact::crossing:
            return Contact::Crossing;
        case g::Contact::touching:
            return Contact::Touching;
        case g::Contact::sameCurve:
            return Contact::SameCurve;
    }
    return Contact::Crossing;
}

CurvePosition positionOf(std::size_t segment, const g::CurvePosition& position) {
    return CurvePosition{segment, locationOf(position.location), position.station};
}

/// Clears `complete` for what solid-space refused or left out: data beyond
/// its domain. A piece that is exactly a point meets nothing and costs
/// nothing.
void noteRefusals(const g::HostIntersections& found, bool& complete) {
    if (found.omittedBeyondDomain > 0) {
        complete = false;
    }
    for (const g::SkippedCutter& skipped : found.skipped) {
        if (skipped.diagnosis.code != g::DiagnosisCode::degenerateCurve) {
            complete = false;
        }
    }
}

// ---------------------------------------------------------------------------
// One call of intersectAlong for each piece of the host

/// What solid-space answered for one piece of the host.
struct HostRun {
    bool answered{false};
    std::vector<g::Cut> cuts;
    std::vector<g::CutterOverlap> overlaps;
};

/// Calls intersectAlong for each piece of `host` against `cutters`, a piece
/// at a time and in order (docs/adr/0035, point 1). `cuttersOf(i)` gives the
/// cutters for host piece i and `cutterExtension` their extension. The
/// shared stretches come from a second call without extensions when either
/// side is mixed (see Extension::Extended).
template <typename CuttersOf>
std::vector<HostRun> runAlong(const EntityCurves& host, CuttersOf cuttersOf,
                              g::Extension cutterExtension, bool cuttersMixed, bool& complete) {
    std::vector<HostRun> runs(host.size());
    for (std::size_t i = 0; i < host.size(); ++i) {
        if (host.isPoint[i]) {
            continue;  // a piece that is exactly a point meets nothing
        }
        const std::vector<g::Curve>& cutters = cuttersOf(i);
        const g::Extension hostExtension = host.extensionOf(i);
        auto found = g::intersectAlong(host.pieces[i], hostExtension, cutters, cutterExtension);
        if (!found.hasValue()) {
            complete = false;  // beyond the domain of solid-space, or invalid
            continue;
        }
        noteRefusals(found.value(), complete);
        HostRun& run = runs[i];
        run.answered = true;
        run.cuts = found.value().cuts;
        if (!host.mixed && !cuttersMixed) {
            run.overlaps = found.value().overlaps;
            continue;
        }
        auto onSegments =
            g::intersectAlong(host.pieces[i], host.mixed ? g::Extension::none : hostExtension,
                              cutters, cuttersMixed ? g::Extension::none : cutterExtension);
        if (onSegments.hasValue()) {
            run.overlaps = onSegments.value().overlaps;
        }
    }
    return runs;
}

/// A vertex of the host lies at the end of one piece and at the start of the
/// piece that follows: it is kept once, at the start of the one that follows
/// (docs/adr/0035, point 1; for a closed polyline, the start of piece 0).
void keepVerticesOnce(const EntityCurves& host, std::vector<HostRun>& runs) {
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const std::size_t following = host.next(i);
        if (!runs[i].answered || following == kNone || !runs[following].answered) {
            continue;
        }
        const bool startsThere = std::ranges::any_of(runs[following].cuts, [](const g::Cut& cut) {
            return cut.onHost.location == g::Location::atStart;
        });
        if (startsThere) {
            std::erase_if(runs[i].cuts, [](const g::Cut& cut) {
                return cut.onHost.location == g::Location::atEnd;
            });
        }
    }
}

/// The contacts of a cut that belong to the cutter entity, with a vertex of
/// it kept once, on the piece that follows it.
std::vector<g::CutterContact> contactsOn(const EntityCurves& cutter, const g::Cut& cut) {
    std::vector<g::CutterContact> kept;
    for (const g::CutterContact& contact : cut.cutters) {
        if (cutter.allows(contact.cutter, contact.onCutter.location)) {
            kept.push_back(contact);
        }
    }
    // Decided on the whole list first, then removed: the test reads the
    // other contacts.
    std::vector<bool> drop(kept.size(), false);
    for (std::size_t k = 0; k < kept.size(); ++k) {
        if (kept[k].onCutter.location != g::Location::atEnd) {
            continue;
        }
        const std::size_t following = cutter.next(kept[k].cutter);
        drop[k] = std::ranges::any_of(kept, [following](const g::CutterContact& other) {
            return other.cutter == following && other.onCutter.location == g::Location::atStart;
        });
    }
    std::vector<g::CutterContact> once;
    for (std::size_t k = 0; k < kept.size(); ++k) {
        if (!drop[k]) {
            once.push_back(kept[k]);
        }
    }
    return once;
}

SharedStretch stretchOf(std::size_t hostPiece, std::size_t cutterPiece, const g::Overlap& overlap) {
    SharedStretch stretch;
    stretch.whole = overlap.whole;
    stretch.from = bridge::fromPoint(overlap.from.point);
    stretch.to = bridge::fromPoint(overlap.to.point);
    stretch.fromOnA = positionOf(hostPiece, overlap.from.onFirst);
    stretch.toOnA = positionOf(hostPiece, overlap.to.onFirst);
    stretch.fromOnB = positionOf(cutterPiece, overlap.from.onSecond);
    stretch.toOnB = positionOf(cutterPiece, overlap.to.onSecond);
    return stretch;
}

// ---------------------------------------------------------------------------
// Side and nearest point

Side sideFrom(g::Side side) {
    switch (side) {
        case g::Side::left:
            return Side::Left;
        case g::Side::on:
            return Side::On;
        case g::Side::right:
            return Side::Right;
    }
    return Side::On;
}

std::optional<Side> sideOfCurve(const g::Curve& curve, const Vec2& point) {
    const auto side = g::side(curve, bridge::toPoint(point));
    if (!side.hasValue()) {
        return std::nullopt;  // exactly a point, or data beyond the domain
    }
    return sideFrom(side.value());
}

/// The polyline segment nearest to `point`, with Tecnigrafo's projection;
/// on a tie the first. Precondition: the polyline has a segment.
std::size_t nearestSegment(const Polyline& polyline, const Vec2& point) {
    assert(polyline.segmentCount() > 0);
    std::size_t nearest = 0;
    double best = distance(polyline.segment(0), point);
    for (std::size_t i = 1; i < polyline.segmentCount(); ++i) {
        const double candidate = distance(polyline.segment(i), point);
        if (candidate < best) {
            best = candidate;
            nearest = i;
        }
    }
    return nearest;
}

/// The position of `nearest`, a point of `arc` found by closestPoint.
CurvePosition positionOnArc(std::size_t segment, const ArcGeometry& arc, const Vec2& nearest) {
    if (nearest == startPoint(arc) && std::abs(arc.sweep) < kTwoPi) {
        return CurvePosition{segment, Location::AtStart, 0.0};
    }
    const double length = arc.radius * std::abs(arc.sweep);
    if (nearest == endPoint(arc) && std::abs(arc.sweep) < kTwoPi) {
        return CurvePosition{segment, Location::AtEnd, length};
    }
    const Vec2 radial = nearest - arc.center;
    const double angle = std::atan2(radial.y, radial.x);
    const double turned = arc.sweep >= 0.0 ? normalizeAngle(angle - arc.startAngle)
                                           : normalizeAngle(arc.startAngle - angle);
    return CurvePosition{segment, Location::Inside, std::min(arc.radius * turned, length)};
}

CurvePosition positionOnSegment(std::size_t index, const PolylineSegment& segment,
                                const Vec2& point) {
    const Vec2 nearest = closestPoint(segment, point);
    if (segment.isArc()) {
        return positionOnArc(index, arcFromBulge(segment.start, segment.end, segment.bulge),
                             nearest);
    }
    if (nearest == segment.start) {
        return CurvePosition{index, Location::AtStart, 0.0};
    }
    if (nearest == segment.end) {
        return CurvePosition{index, Location::AtEnd, distance(segment.start, segment.end)};
    }
    return CurvePosition{index, Location::Inside, distance(segment.start, nearest)};
}

}  // namespace

// ---------------------------------------------------------------------------

Intersections intersect(const Entity& a, Extension extendA, const Entity& b, Extension extendB) {
    Intersections result;
    const EntityCurves first = curvesOf(a, extendA);
    const EntityCurves second = curvesOf(b, extendB);
    if (first.empty() || second.empty()) {
        return result;
    }
    std::vector<HostRun> runs = runAlong(
        first, [&second](std::size_t) -> const std::vector<g::Curve>& { return second.pieces; },
        second.cutterExtension(), second.mixed, result.complete);
    keepVerticesOnce(first, runs);
    for (std::size_t i = 0; i < runs.size(); ++i) {
        for (const g::Cut& cut : runs[i].cuts) {
            if (!first.allows(i, cut.onHost.location)) {
                continue;
            }
            const std::vector<g::CutterContact> contacts = contactsOn(second, cut);
            if (contacts.empty()) {
                continue;
            }
            // One crossing per exact point: the contacts are by increasing
            // index, and a vertex of B is already on the piece that follows.
            const g::CutterContact& contact = contacts.front();
            result.crossings.push_back(
                Crossing{bridge::fromPoint(cut.point), contactOf(contact.contact),
                         positionOf(i, cut.onHost), positionOf(contact.cutter, contact.onCutter)});
        }
        for (const g::CutterOverlap& overlap : runs[i].overlaps) {
            result.shared.push_back(stretchOf(i, overlap.cutter, overlap.overlap));
        }
    }
    return result;
}

Intersections selfIntersections(const Polyline& polyline) {
    Intersections result;
    const EntityCurves curves = curvesOf(Entity{polyline}, Extension::None);
    if (curves.size() < 2) {
        return result;
    }
    // For host piece i the cutters are all the other pieces; others[i][k] is
    // the index in the polyline of cutter k.
    std::vector<std::vector<g::Curve>> cutters(curves.size());
    std::vector<std::vector<std::size_t>> others(curves.size());
    for (std::size_t i = 0; i < curves.size(); ++i) {
        for (std::size_t j = 0; j < curves.size(); ++j) {
            if (j != i) {
                cutters[i].push_back(curves.pieces[j]);
                others[i].push_back(j);
            }
        }
    }
    std::vector<HostRun> runs = runAlong(
        curves, [&cutters](std::size_t i) -> const std::vector<g::Curve>& { return cutters[i]; },
        g::Extension::none, false, result.complete);
    keepVerticesOnce(curves, runs);
    for (std::size_t i = 0; i < runs.size(); ++i) {
        const std::size_t previous = curves.previous(i);
        const std::size_t following = curves.next(i);
        for (const g::Cut& cut : runs[i].cuts) {
            // Renumber the contacts as segments of the polyline, then keep
            // those that come after piece i and are not the vertex it shares
            // with a neighbour; each other pair is found from its first piece.
            g::Cut renumbered = cut;
            for (g::CutterContact& contact : renumbered.cutters) {
                contact.cutter = others[i][contact.cutter];
            }
            std::vector<g::CutterContact> contacts = contactsOn(curves, renumbered);
            std::erase_if(contacts, [&](const g::CutterContact& contact) {
                const bool sharedVertex =
                    (cut.onHost.location == g::Location::atStart && contact.cutter == previous &&
                     contact.onCutter.location == g::Location::atEnd) ||
                    (cut.onHost.location == g::Location::atEnd && contact.cutter == following &&
                     contact.onCutter.location == g::Location::atStart);
                return sharedVertex || contact.cutter < i;
            });
            if (contacts.empty()) {
                continue;
            }
            const g::CutterContact& contact = contacts.front();
            result.crossings.push_back(
                Crossing{bridge::fromPoint(cut.point), contactOf(contact.contact),
                         positionOf(i, cut.onHost), positionOf(contact.cutter, contact.onCutter)});
        }
        for (const g::CutterOverlap& overlap : runs[i].overlaps) {
            const std::size_t j = others[i][overlap.cutter];
            if (j > i) {
                result.shared.push_back(stretchOf(i, j, overlap.overlap));
            }
        }
    }
    return result;
}

HostCuts cutsAlong(const Entity& host, Extension extendHost, std::span<const Entity> cutters,
                   bool hostCutsItself) {
    HostCuts result;
    const EntityCurves curves = curvesOf(host, extendHost);
    if (curves.empty()) {
        return result;
    }
    const bool selfCuts = hostCutsItself && std::holds_alternative<Polyline>(host);

    // The pieces of all the cutters, each with the entity it comes from.
    std::vector<g::Curve> cutterPieces;
    std::vector<std::size_t> owners;
    for (std::size_t e = 0; e < cutters.size(); ++e) {
        for (const g::Curve& piece : curvesOf(cutters[e], Extension::None).pieces) {
            cutterPieces.push_back(piece);
            owners.push_back(e);
        }
    }
    // For host piece i, the cutters followed by the other pieces of the host
    // when it cuts itself; ownerOf[i][k] is kNone for those.
    std::vector<std::vector<g::Curve>> callCutters(curves.size(), cutterPieces);
    std::vector<std::vector<std::size_t>> ownerOf(curves.size(), owners);
    if (selfCuts) {
        for (std::size_t i = 0; i < curves.size(); ++i) {
            for (std::size_t j = 0; j < curves.size(); ++j) {
                if (j != i) {
                    callCutters[i].push_back(curves.pieces[j]);
                    ownerOf[i].push_back(kNone);
                }
            }
        }
    }
    std::vector<HostRun> runs = runAlong(
        curves,
        [&callCutters](std::size_t i) -> const std::vector<g::Curve>& { return callCutters[i]; },
        g::Extension::none, false, result.complete);

    // Every vertex of a host that cuts itself is a cut, also the two ends of
    // an open polyline and the ends of the pieces next to one that is exactly
    // a point. A missing vertex goes where its location puts it: after the
    // points before the start, or before the points after the end.
    if (selfCuts) {
        for (std::size_t i = 0; i < runs.size(); ++i) {
            if (!runs[i].answered) {
                continue;
            }
            std::vector<g::Cut>& cuts = runs[i].cuts;
            const auto has = [&cuts](g::Location location) {
                return std::ranges::any_of(cuts, [location](const g::Cut& cut) {
                    return cut.onHost.location == location;
                });
            };
            const auto ends = g::endpoints(curves.pieces[i]);
            assert(ends.hasValue() && "an answered piece is a curve");
            if (!has(g::Location::atStart)) {
                const auto at = std::ranges::find_if(cuts, [](const g::Cut& cut) {
                    return cut.onHost.location != g::Location::beforeStart;
                });
                cuts.insert(at, g::Cut{ends.value().start, {g::Location::atStart, 0.0}, {}});
            }
            if (!has(g::Location::atEnd)) {
                const auto length = g::length(curves.pieces[i]);
                const double station = length.hasValue() ? length.value() : 0.0;
                const auto at = std::ranges::find_if(cuts, [](const g::Cut& cut) {
                    return cut.onHost.location == g::Location::afterEnd;
                });
                cuts.insert(at, g::Cut{ends.value().end, {g::Location::atEnd, station}, {}});
            }
        }
    }
    keepVerticesOnce(curves, runs);

    for (std::size_t i = 0; i < runs.size(); ++i) {
        for (const g::CutterOverlap& overlap : runs[i].overlaps) {
            const std::size_t owner = ownerOf[i][overlap.cutter];
            if (owner != kNone) {
                result.overlapping.push_back(owner);
            }
        }
    }
    std::ranges::sort(result.overlapping);
    const auto [last, end] = std::ranges::unique(result.overlapping);
    result.overlapping.erase(last, end);

    for (std::size_t i = 0; i < runs.size(); ++i) {
        for (const g::Cut& cut : runs[i].cuts) {
            const g::Location location = cut.onHost.location;
            if (!curves.allows(i, location)) {
                continue;
            }
            const bool onTheHost = location == g::Location::atStart ||
                                   location == g::Location::inside ||
                                   location == g::Location::atEnd;
            Cut found{bridge::fromPoint(cut.point), positionOf(i, cut.onHost), {}};
            bool selfCut = selfCuts && onTheHost &&
                           (location == g::Location::atStart || location == g::Location::atEnd);
            for (const g::CutterContact& contact : cut.cutters) {
                const std::size_t owner = ownerOf[i][contact.cutter];
                if (owner == kNone) {
                    selfCut = selfCut || onTheHost;
                } else if (!std::ranges::binary_search(result.overlapping, owner)) {
                    found.cutters.push_back(owner);
                }
            }
            std::ranges::sort(found.cutters);
            const auto [repeated, tail] = std::ranges::unique(found.cutters);
            found.cutters.erase(repeated, tail);
            if (!found.cutters.empty() || selfCut) {
                result.cuts.push_back(std::move(found));
            }
        }
    }
    return result;
}

std::optional<Side> sideOf(const Entity& entity, const Vec2& point) {
    if (const auto* polyline = std::get_if<Polyline>(&entity)) {
        if (polyline->segmentCount() == 0) {
            return std::nullopt;
        }
        const PolylineSegment segment = polyline->segment(nearestSegment(*polyline, point));
        return sideOfCurve(g::Curve{bridge::pieceOf(segment)}, point);
    }
    const std::optional<g::Curve> curve = bridge::curveOf(entity);
    if (!curve) {
        return std::nullopt;
    }
    return sideOfCurve(*curve, point);
}

CurvePosition nearestPosition(const Entity& entity, const Vec2& point) {
    assert(!std::holds_alternative<Text>(entity));
    if (const auto* polyline = std::get_if<Polyline>(&entity)) {
        if (polyline->segmentCount() == 0) {
            return CurvePosition{0, Location::AtStart, 0.0};
        }
        const std::size_t index = nearestSegment(*polyline, point);
        return positionOnSegment(index, polyline->segment(index), point);
    }
    if (const auto* line = std::get_if<Line>(&entity)) {
        return positionOnSegment(0, PolylineSegment{line->start, line->end}, point);
    }
    if (const auto* arc = std::get_if<Arc>(&entity)) {
        const ArcGeometry geometry = arc->geometry();
        return positionOnArc(0, geometry, closestPoint(geometry, point));
    }
    const auto& circle = std::get<Circle>(entity);
    const Vec2 radial = point - circle.center;
    // Every point of a circle is inside it (solid-space, Location::inside);
    // the station runs counter-clockwise from angle 0.
    const double angle = radial == Vec2{} ? 0.0 : normalizeAngle(std::atan2(radial.y, radial.x));
    return CurvePosition{0, Location::Inside, circle.radius * angle};
}

}  // namespace tg::core
