// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "dxf_extrusion.hpp"

#include "dxf_entities.hpp"
#include "tg/io/dxf_groups.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>

namespace tg::io {
namespace {

// Components of an axial extrusion that count as zero (docs/adr/0018,
// point 5): files written in single precision or rotated by 180 degrees
// leave residues far below this.
constexpr double kAxialTolerance = 1e-12;

constexpr double kFullTurn = 360.0;
constexpr double kHalfTurn = 180.0;

constexpr std::array kPlacementCodes{30, 31, 38, 39, 210, 220, 230};

bool isPlacement(int code) {
    return std::ranges::find(kPlacementCodes, code) != kPlacementCodes.end();
}

/// Counts the groups of the entity subclasses by code, for the anchors.
class GroupCounter {
public:
    core::GroupAnchor count(int code) { return {code, ++m_counts[code]}; }

private:
    std::map<int, std::size_t> m_counts;
};

}  // namespace

Extrusion classifyExtrusion(double x, double y, double z) {
    if (!(std::abs(x) <= kAxialTolerance) || !(std::abs(y) <= kAxialTolerance)) {
        return Extrusion::Other;
    }
    if (z > 0.0) {
        return Extrusion::Up;
    }
    if (z < 0.0) {
        return Extrusion::Down;
    }
    return Extrusion::Other;
}

Extrusion entityExtrusion(const core::RawGroups& groups) {
    std::array<double, 3> direction{0, 0, 1};
    std::size_t depth = 0;
    for (const auto& group : groups) {
        const auto value = trimDxf(group.value);
        if (group.code == 1001) {
            break;  // XDATA belongs to applications, not to the placement.
        }
        if (group.code == 102) {
            if (value.starts_with('{')) {
                ++depth;
            } else if (value == "}" && depth != 0) {
                --depth;
            }
            continue;
        }
        if (depth != 0 || (group.code != 210 && group.code != 220 && group.code != 230)) {
            continue;
        }
        const auto number = parseNumber(value);
        if (!number) {
            return Extrusion::Other;
        }
        direction.at(static_cast<std::size_t>((group.code - 210) / 10)) = *number;
    }
    return classifyExtrusion(direction[0], direction[1], direction[2]);
}

core::Vec2 flipX(const core::Vec2& point) {
    return {negate(point.x), point.y};
}

double negate(double value) {
    // 0.0 - x is -x for every x but zero, which stays +0.
    return 0.0 - value;
}

double normalizeDegrees(double degrees) {
    double result = std::fmod(degrees, kFullTurn);
    if (result < 0.0) {
        result += kFullTurn;
    }
    // Adding 360 to a tiny negative value can round up to 360 itself.
    return result >= kFullTurn ? 0.0 : result;
}

double flipDirectionDegrees(double degrees) {
    return normalizeDegrees(kHalfTurn - degrees);
}

std::vector<core::GroupAnchor> readPlacementAnchors(const core::RawGroups& groups) {
    std::vector<core::GroupAnchor> anchors;
    GroupCounter counter;
    std::optional<core::GroupAnchor> previous;
    bool geometry = false;
    for (const auto& group : groups) {
        if (group.code == 1001) {
            break;
        }
        if (group.code == 100) {
            geometry = trimDxf(group.value) != "AcDbEntity";
        }
        if (!geometry) {
            continue;
        }
        if (isPlacement(group.code)) {
            anchors.push_back(previous.value_or(core::GroupAnchor{}));
        }
        previous = counter.count(group.code);
    }
    return anchors;
}

void restorePlacement(const core::EntityExtras& extras, std::size_t from, core::RawGroups& out) {
    const auto& placement = extras.placement;
    const auto& anchors = extras.placementAnchors;
    if (anchors.empty() || anchors.size() != placement.size() || from >= out.size()) {
        return;
    }
    const auto begin = out.begin() + static_cast<std::ptrdiff_t>(from);
    const auto end = std::ranges::find(begin, out.end(), 1001, &core::RawGroup::code);
    core::RawGroups result;
    std::vector<bool> placed(placement.size(), false);
    GroupCounter counter;
    // Every group emitted, model or placement, can be the anchor of the
    // next placement group: 210, 220 and 230 follow one another. A group
    // has one predecessor, so it anchors at most one placement group.
    const auto emit = [&](const core::RawGroup& first) {
        const core::RawGroup* current = &first;
        while (current != nullptr) {
            result.push_back(*current);
            const core::GroupAnchor anchor = counter.count(current->code);
            current = nullptr;
            for (std::size_t i = 0; i < placement.size(); ++i) {
                if (!placed[i] && anchors[i] == anchor) {
                    placed[i] = true;
                    current = &placement[i];
                    break;
                }
            }
        }
    };
    for (auto it = begin; it != end; ++it) {
        if (!isPlacement(it->code)) {
            emit(*it);
        }
    }
    if (std::ranges::find(placed, false) != placed.end()) {
        return;  // an anchor is gone: keep the codec's positions
    }
    const auto tail = static_cast<std::size_t>(end - out.begin());
    core::RawGroups rest(out.begin() + static_cast<std::ptrdiff_t>(tail), out.end());
    out.resize(from);
    out.insert(out.end(), result.begin(), result.end());
    out.insert(out.end(), rest.begin(), rest.end());
}

}  // namespace tg::io
