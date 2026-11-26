// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "object_tracking.hpp"

#include "tg/core/arc.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/document.hpp"
#include "tg/core/intersections.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

namespace tg::cmd::detail {
namespace {

using State = CommandRunner::TrackingState;
using Acquisition = State::Acquisition;
using Guide = State::Guide;
using ActiveGuide = State::ActiveGuide;
using Kind = ActiveGuide::Kind;

double squaredDistance(const core::Vec2& a, const core::Vec2& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return (dx * dx) + (dy * dy);
}

/// The order of the guides of one acquisition on ties
/// (docs/REGOLE-NUMERICHE.md, section 4, "Tracciamento degli snap", points
/// 2 and 6): the extension, then the horizontal, the vertical, the parallel.
int rankOf(Kind kind) {
    switch (kind) {
        case Kind::Extension:
            return 0;
        case Kind::Horizontal:
            return 1;
        case Kind::Vertical:
            return 2;
        case Kind::Parallel:
            break;
    }
    return 3;
}

bool isPointGuide(const ActiveGuide& guide) {
    return guide.kind == Kind::Horizontal || guide.kind == Kind::Vertical;
}

/// A ray, not a whole line: a straight extension, or the polar ray.
bool isRay(const ActiveGuide& guide) {
    return guide.kind == Kind::Extension && !guide.arc;
}

/// The guides of the acquisitions, in their order and, within one, in the
/// order of rankOf().
std::vector<ActiveGuide> guidesOf(std::span<const Acquisition> acquisitions,
                                  const std::optional<core::Vec2>& base) {
    std::vector<ActiveGuide> guides;
    for (std::size_t index = 0; index < acquisitions.size(); ++index) {
        const Acquisition& acquisition = acquisitions[index];
        if (acquisition.extension) {
            guides.push_back({Kind::Extension, index, acquisition.extension->end,
                              acquisition.extension->direction, acquisition.extension->beyond});
        }
        if (acquisition.point) {
            guides.push_back({Kind::Horizontal, index, *acquisition.point, {1.0, 0.0}, {}});
            guides.push_back({Kind::Vertical, index, *acquisition.point, {0.0, 1.0}, {}});
        }
        if (acquisition.parallel && base) {
            const core::Vec2 direction = acquisition.parallel->end - acquisition.parallel->start;
            if (direction != core::Vec2{}) {
                guides.push_back({Kind::Parallel, index, *base, direction, {}});
            }
        }
    }
    return guides;
}

/// The point of `guide` the pointer goes to, if the guide catches it.
std::optional<core::Vec2> footOn(const ActiveGuide& guide, const core::Vec2& pointer,
                                 double aperture) {
    if (guide.kind == Kind::Horizontal) {
        if (std::abs(pointer.y - guide.origin.y) <= aperture) {
            return core::Vec2{pointer.x, guide.origin.y};
        }
        return std::nullopt;
    }
    if (guide.kind == Kind::Vertical) {
        if (std::abs(pointer.x - guide.origin.x) <= aperture) {
            return core::Vec2{guide.origin.x, pointer.y};
        }
        return std::nullopt;
    }
    if (guide.arc) {
        const core::Vec2 radial = pointer - guide.arc->center;
        const double length = core::hypot(radial.x, radial.y);
        if (length == 0.0 || std::abs(length - guide.arc->radius) > aperture ||
            !core::containsAngle(*guide.arc, core::atan2(radial.y, radial.x))) {
            return std::nullopt;
        }
        return guide.arc->center + (radial * (guide.arc->radius / length));
    }
    const core::Vec2& direction = guide.direction;
    const double squaredLength = core::dot(direction, direction);
    if (squaredLength == 0.0) {
        return std::nullopt;
    }
    const double parameter = core::dot(pointer - guide.origin, direction) / squaredLength;
    if (isRay(guide) && parameter < 0.0) {
        return std::nullopt;  // behind the end: over the curve, not beyond it
    }
    const core::Vec2 foot = guide.origin + (direction * parameter);
    if (squaredDistance(foot, pointer) > aperture * aperture) {
        return std::nullopt;
    }
    return foot;
}

/// The line of a straight guide, through `through` if given (the polar
/// ray, as the user sees it) or else one direction away from its origin.
core::Entity lineOf(const ActiveGuide& guide, const std::optional<core::Vec2>& through) {
    return core::Line{guide.origin, through.value_or(guide.origin + guide.direction)};
}

/// The part of the circle beyond the end, counter-clockwise as an ARC.
core::Entity arcOf(const core::ArcGeometry& beyond) {
    const double from = beyond.sweep > 0.0 ? beyond.startAngle : beyond.startAngle + beyond.sweep;
    const double to = beyond.sweep > 0.0 ? beyond.startAngle + beyond.sweep : beyond.startAngle;
    return core::Arc{beyond.center, beyond.radius, from, to};
}

/// How far `point`, on the circle, is along an arc extension from its end.
double alongArc(const core::ArcGeometry& beyond, const core::Vec2& point) {
    const double angle = core::atan2(point.y - beyond.center.y, point.x - beyond.center.x);
    return core::normalizeAngle(beyond.sweep > 0.0 ? angle - beyond.startAngle
                                                   : beyond.startAngle - angle);
}

/// A point where two guides meet, with how far it is along an arc
/// extension, if one takes part (0 otherwise).
struct Meeting {
    core::Vec2 point;
    double along{0.0};
};

/// Where two guides meet (see trackPoint()); `b` may be the polar ray, with
/// the point `bThrough` it passes through.
std::vector<Meeting> meetings(const ActiveGuide& a, const ActiveGuide& b,
                              const std::optional<core::Vec2>& bThrough = std::nullopt) {
    std::vector<Meeting> found;
    if (isPointGuide(a) && isPointGuide(b)) {
        if (a.kind == b.kind) {
            return found;  // parallel
        }
        const ActiveGuide& horizontal = a.kind == Kind::Horizontal ? a : b;
        const ActiveGuide& vertical = a.kind == Kind::Horizontal ? b : a;
        found.push_back({{vertical.origin.x, horizontal.origin.y}, 0.0});
        return found;
    }
    if (a.arc && b.arc) {
        return found;  // an arc extension crosses only straight guides
    }
    if (const std::optional<core::ArcGeometry>& arc = a.arc ? a.arc : b.arc) {
        const ActiveGuide& straight = a.arc ? b : a;
        const core::Intersections crossings = core::intersect(
            arcOf(*arc), core::Extension::None, lineOf(straight, a.arc ? bThrough : std::nullopt),
            core::Extension::Extended);
        for (const core::Crossing& crossing : crossings.crossings) {
            if (crossing.contact != core::Contact::Crossing ||
                (isRay(straight) && crossing.onB.location == core::Location::BeforeStart)) {
                continue;
            }
            found.push_back({crossing.point, alongArc(*arc, crossing.point)});
        }
        return found;
    }
    const core::Intersections crossings =
        core::intersect(lineOf(a, std::nullopt), core::Extension::Extended, lineOf(b, bThrough),
                        core::Extension::Extended);
    for (const core::Crossing& crossing : crossings.crossings) {
        if (crossing.contact != core::Contact::Crossing ||
            (isRay(a) && crossing.onA.location == core::Location::BeforeStart) ||
            (isRay(b) && crossing.onB.location == core::Location::BeforeStart)) {
            continue;
        }
        found.push_back({crossing.point, 0.0});
    }
    return found;
}

/// A candidate point: a crossing of two guides (`newer` and `older`, by
/// their acquisitions), a crossing of `newer` with the polar ray, or a
/// single guide `newer`; indices into the guides.
struct Candidate {
    core::Vec2 point;
    double squared{std::numeric_limits<double>::infinity()};
    std::size_t newer{0};
    std::optional<std::size_t> older;
    double along{0.0};
};

/// The ties of two crossings of two guides (see trackPoint()).
bool crossingBeats(const Candidate& candidate, const Candidate& best,
                   const std::vector<ActiveGuide>& guides) {
    if (candidate.squared != best.squared) {
        return candidate.squared < best.squared;
    }
    if (!candidate.older || !best.older) {
        return false;  // only crossings of two guides come here
    }
    const ActiveGuide& newer = guides[candidate.newer];
    const ActiveGuide& older = guides[*candidate.older];
    const ActiveGuide& bestNewer = guides[best.newer];
    const ActiveGuide& bestOlder = guides[*best.older];
    if (isPointGuide(newer) && isPointGuide(older) && isPointGuide(bestNewer) &&
        isPointGuide(bestOlder)) {
        // Points only: the horizontal of the most recent point, then its
        // vertical (docs/REGOLE-NUMERICHE.md, section 4, point 3).
        const auto horizontal = [](const ActiveGuide& one, const ActiveGuide& other) {
            return one.kind == Kind::Horizontal ? one.acquisition : other.acquisition;
        };
        const auto vertical = [](const ActiveGuide& one, const ActiveGuide& other) {
            return one.kind == Kind::Vertical ? one.acquisition : other.acquisition;
        };
        if (horizontal(newer, older) != horizontal(bestNewer, bestOlder)) {
            return horizontal(newer, older) > horizontal(bestNewer, bestOlder);
        }
        return vertical(newer, older) > vertical(bestNewer, bestOlder);
    }
    if (newer.acquisition != bestNewer.acquisition) {
        return newer.acquisition > bestNewer.acquisition;
    }
    if (older.acquisition != bestOlder.acquisition) {
        return older.acquisition > bestOlder.acquisition;
    }
    if (rankOf(newer.kind) != rankOf(bestNewer.kind)) {
        return rankOf(newer.kind) < rankOf(bestNewer.kind);
    }
    if (rankOf(older.kind) != rankOf(bestOlder.kind)) {
        return rankOf(older.kind) < rankOf(bestOlder.kind);
    }
    return candidate.along < best.along;
}

/// The ties of two single guides, and of two crossings with the polar ray.
bool guideBeats(const Candidate& candidate, const Candidate& best,
                const std::vector<ActiveGuide>& guides) {
    if (candidate.squared != best.squared) {
        return candidate.squared < best.squared;
    }
    const ActiveGuide& guide = guides[candidate.newer];
    const ActiveGuide& bestGuide = guides[best.newer];
    if (guide.acquisition != bestGuide.acquisition) {
        return guide.acquisition > bestGuide.acquisition;
    }
    if (rankOf(guide.kind) != rankOf(bestGuide.kind)) {
        return rankOf(guide.kind) < rankOf(bestGuide.kind);
    }
    return candidate.along < best.along;
}

Guide singleGuide(Kind kind) {
    switch (kind) {
        case Kind::Horizontal:
            return Guide::Horizontal;
        case Kind::Vertical:
            return Guide::Vertical;
        case Kind::Extension:
            return Guide::Extension;
        case Kind::Parallel:
            break;
    }
    return Guide::Parallel;
}

}  // namespace

CommandRunner::TrackingState trackPoint(std::span<const Acquisition> acquisitions,
                                        const std::optional<core::Vec2>& base,
                                        const core::Vec2& pointer, double aperture,
                                        const std::optional<PolarRay>& polar) {
    assert(aperture >= 0.0);
    const std::vector<ActiveGuide> guides = guidesOf(acquisitions, base);
    // The guides that catch the pointer, with the point each gives.
    std::vector<std::size_t> catching;
    std::vector<core::Vec2> feet;
    for (std::size_t index = 0; index < guides.size(); ++index) {
        if (const std::optional<core::Vec2> foot = footOn(guides[index], pointer, aperture)) {
            catching.push_back(index);
            feet.push_back(*foot);
        }
    }

    // Crossings of two guides of different acquisitions.
    std::optional<Candidate> crossing;
    for (std::size_t i = 0; i < catching.size(); ++i) {
        for (std::size_t j = i + 1; j < catching.size(); ++j) {
            const ActiveGuide& first = guides[catching[i]];
            const ActiveGuide& second = guides[catching[j]];
            if (first.acquisition == second.acquisition) {
                continue;
            }
            // Guides come in the order of their acquisitions.
            const std::size_t newer = catching[j];
            const std::size_t older = catching[i];
            for (const Meeting& meeting : meetings(first, second)) {
                const Candidate candidate{meeting.point, squaredDistance(meeting.point, pointer),
                                          newer, older, meeting.along};
                if (!crossing || crossingBeats(candidate, *crossing, guides)) {
                    crossing = candidate;
                }
            }
        }
    }

    // Crossings of one guide with the polar ray: only nearer than a guide
    // crossing (docs/REGOLE-NUMERICHE.md, section 4, point 4).
    if (polar) {
        const ActiveGuide ray{Kind::Extension, 0, polar->base, polar->through - polar->base, {}};
        std::optional<Candidate> withPolar;
        for (const std::size_t index : catching) {
            for (const Meeting& meeting : meetings(guides[index], ray, polar->through)) {
                const Candidate candidate{meeting.point, squaredDistance(meeting.point, pointer),
                                          index, std::nullopt, meeting.along};
                if (!withPolar || guideBeats(candidate, *withPolar, guides)) {
                    withPolar = candidate;
                }
            }
        }
        if (withPolar && (!crossing || withPolar->squared < crossing->squared)) {
            State state;
            state.guidePoint = withPolar->point;
            state.guide = Guide::WithPolar;
            state.guides = {guides[withPolar->newer]};
            return state;
        }
    }
    if (crossing) {
        State state;
        state.guidePoint = crossing->point;
        state.guide = Guide::Crossing;
        state.guides = {guides[crossing->newer], guides[*crossing->older]};
        return state;
    }

    // A single guide: the nearest.
    std::optional<Candidate> single;
    for (std::size_t k = 0; k < catching.size(); ++k) {
        const ActiveGuide& guide = guides[catching[k]];
        const double along = guide.arc ? alongArc(*guide.arc, feet[k]) : 0.0;
        const Candidate candidate{feet[k], squaredDistance(feet[k], pointer), catching[k],
                                  std::nullopt, along};
        if (!single || guideBeats(candidate, *single, guides)) {
            single = candidate;
        }
    }
    State state;
    if (single) {
        const ActiveGuide& guide = guides[single->newer];
        state.guidePoint = single->point;
        state.guide = singleGuide(guide.kind);
        state.from = guide.origin;
        state.guides = {guide};
    }
    return state;
}

std::optional<core::Vec2> pointAlongGuide(const CommandRunner::TrackingState& state,
                                          double distance) {
    if (state.guides.size() != 1 || !state.guidePoint ||
        (state.guide != Guide::Horizontal && state.guide != Guide::Vertical &&
         state.guide != Guide::Extension && state.guide != Guide::Parallel)) {
        return std::nullopt;
    }
    const ActiveGuide& guide = state.guides.front();
    const core::Vec2 from = guide.origin;
    const core::Vec2 towards = *state.guidePoint;
    switch (guide.kind) {
        // Exact along the guide: the coordinate it keeps is the acquired one.
        case Kind::Horizontal:
            if (towards.x == from.x) {
                return std::nullopt;
            }
            return core::Vec2{from.x + (towards.x > from.x ? distance : -distance), from.y};
        case Kind::Vertical:
            if (towards.y == from.y) {
                return std::nullopt;
            }
            return core::Vec2{from.x, from.y + (towards.y > from.y ? distance : -distance)};
        case Kind::Extension: {
            // A negative distance falls behind the end, off the guide.
            if (distance < 0.0) {
                return std::nullopt;
            }
            if (distance == 0.0) {
                return from;
            }
            if (guide.arc) {
                // Along the arc, at most up to its other end (docs/adr/0057,
                // point 4).
                const double angle = distance / guide.arc->radius;
                if (angle > std::abs(guide.arc->sweep)) {
                    return std::nullopt;
                }
                const core::SinCos turn =
                    core::sinCos(guide.arc->startAngle + (guide.arc->sweep > 0.0 ? angle : -angle));
                return guide.arc->center + (core::Vec2{turn.cosine, turn.sine} * guide.arc->radius);
            }
            const double length = core::hypot(guide.direction.x, guide.direction.y);
            return from + (guide.direction * (distance / length));
        }
        case Kind::Parallel: {
            const double side = core::dot(towards - from, guide.direction);
            if (side == 0.0) {
                return std::nullopt;
            }
            const double length = core::hypot(guide.direction.x, guide.direction.y);
            return from + (guide.direction * ((side > 0.0 ? distance : -distance) / length));
        }
    }
    return std::nullopt;
}

}  // namespace tg::cmd::detail
