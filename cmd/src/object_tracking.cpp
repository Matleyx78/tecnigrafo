// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "object_tracking.hpp"

#include "tg/core/document.hpp"
#include "tg/core/intersections.hpp"
#include "tg/core/line.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>

namespace tg::cmd::detail {
namespace {

using Guide = CommandRunner::TrackingState::Guide;

double squaredDistance(const core::Vec2& a, const core::Vec2& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return (dx * dx) + (dy * dy);
}

// The best candidate so far of one kind (crossing or single guide), with
// what its ties are decided by: the indices of the acquired points it uses.
struct Best {
    CommandRunner::TrackingState state;
    double squared{std::numeric_limits<double>::infinity()};
    std::size_t horizontal{0};  ///< of the horizontal guide, or of the only one
    std::size_t vertical{0};    ///< of the vertical guide of a guide crossing

    [[nodiscard]] bool found() const { return state.guide != Guide::None; }
};

// Where the polar ray crosses the horizontal (or the vertical) through
// `point`, if it does: solid-space decides on the extended lines, and a
// crossing behind the base of the ray is not on the ray.
std::optional<core::Vec2> polarCrossing(const PolarRay& ray, const core::Vec2& point,
                                        bool horizontal) {
    const core::Vec2 along = horizontal ? core::Vec2{1.0, 0.0} : core::Vec2{0.0, 1.0};
    const core::Entity rayLine{core::Line{ray.base, ray.through}};
    const core::Entity guideLine{core::Line{point, point + along}};
    const core::Intersections found =
        core::intersect(rayLine, core::Extension::Extended, guideLine, core::Extension::Extended);
    for (const core::Crossing& crossing : found.crossings) {
        if (crossing.contact == core::Contact::Crossing &&
            crossing.onA.location != core::Location::BeforeStart) {
            return crossing.point;
        }
    }
    return std::nullopt;
}

}  // namespace

CommandRunner::TrackingState trackPoint(std::span<const core::Vec2> acquired,
                                        const core::Vec2& pointer, double aperture,
                                        const std::optional<PolarRay>& polar) {
    assert(aperture >= 0.0);
    const auto catchesHorizontal = [&](const core::Vec2& point) {
        return std::abs(pointer.y - point.y) <= aperture;
    };
    const auto catchesVertical = [&](const core::Vec2& point) {
        return std::abs(pointer.x - point.x) <= aperture;
    };

    // Crossings of two guides of different points. Later horizontals, and
    // with the same horizontal later verticals, win a tie.
    Best crossing;
    for (std::size_t h = 0; h < acquired.size(); ++h) {
        if (!catchesHorizontal(acquired[h])) {
            continue;
        }
        for (std::size_t v = 0; v < acquired.size(); ++v) {
            if (v == h || !catchesVertical(acquired[v])) {
                continue;
            }
            const core::Vec2 point{acquired[v].x, acquired[h].y};
            const double squared = squaredDistance(point, pointer);
            const bool wins =
                !crossing.found() || squared < crossing.squared ||
                (squared == crossing.squared &&
                 (h > crossing.horizontal || (h == crossing.horizontal && v > crossing.vertical)));
            if (wins) {
                crossing = Best{{{}, point, Guide::Crossing, std::nullopt}, squared, h, v};
            }
        }
    }

    // Crossings of one guide with the polar ray: only nearer than a guide
    // crossing. Between them a later point wins a tie, and the horizontal,
    // tried first, keeps it against the vertical of the same point.
    if (polar) {
        Best withPolar;
        for (std::size_t index = 0; index < acquired.size(); ++index) {
            for (const bool horizontal : {true, false}) {
                const core::Vec2& from = acquired[index];
                if (!(horizontal ? catchesHorizontal(from) : catchesVertical(from))) {
                    continue;
                }
                const std::optional<core::Vec2> point = polarCrossing(*polar, from, horizontal);
                if (!point) {
                    continue;
                }
                const double squared = squaredDistance(*point, pointer);
                const bool wins = !withPolar.found() || squared < withPolar.squared ||
                                  (squared == withPolar.squared && index > withPolar.horizontal);
                if (wins) {
                    withPolar =
                        Best{{{}, *point, Guide::WithPolar, std::nullopt}, squared, index, index};
                }
            }
        }
        if (withPolar.found() && (!crossing.found() || withPolar.squared < crossing.squared)) {
            crossing = withPolar;
        }
    }
    if (crossing.found()) {
        return crossing.state;
    }

    // A single guide: the nearest; a later point wins a tie, and the
    // horizontal, tried first, keeps it against the vertical of its point.
    Best single;
    for (std::size_t index = 0; index < acquired.size(); ++index) {
        const core::Vec2& from = acquired[index];
        for (const bool horizontal : {true, false}) {
            if (!(horizontal ? catchesHorizontal(from) : catchesVertical(from))) {
                continue;
            }
            const core::Vec2 point =
                horizontal ? core::Vec2{pointer.x, from.y} : core::Vec2{from.x, pointer.y};
            const double squared = squaredDistance(point, pointer);
            const bool wins = !single.found() || squared < single.squared ||
                              (squared == single.squared && index > single.horizontal);
            if (wins) {
                single = Best{{{}, point, horizontal ? Guide::Horizontal : Guide::Vertical, from},
                              squared,
                              index,
                              index};
            }
        }
    }
    return single.state;
}

std::optional<core::Vec2> pointAlongGuide(const CommandRunner::TrackingState& state,
                                          double distance) {
    if ((state.guide != Guide::Horizontal && state.guide != Guide::Vertical) || !state.from ||
        !state.guidePoint) {
        return std::nullopt;
    }
    const core::Vec2 from = *state.from;
    const core::Vec2 towards = *state.guidePoint;
    // Exact along the guide: the coordinate it keeps is the acquired one.
    if (state.guide == Guide::Horizontal) {
        if (towards.x == from.x) {
            return std::nullopt;
        }
        return core::Vec2{from.x + (towards.x > from.x ? distance : -distance), from.y};
    }
    if (towards.y == from.y) {
        return std::nullopt;
    }
    return core::Vec2{from.x, from.y + (towards.y > from.y ? distance : -distance)};
}

}  // namespace tg::cmd::detail
