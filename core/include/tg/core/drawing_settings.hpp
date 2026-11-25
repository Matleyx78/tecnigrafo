// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/vec2.hpp"

#include <cmath>
#include <cstdint>

namespace tg::core {

/// The grid, the snap to the grid and the UCS icon of a drawing, as the
/// *ACTIVE record of the DXF VPORT table holds them (docs/adr/0055, point 1).
/// The defaults are those of a new drawing (acadiso.dwt).
struct GridSettings {
    /// The highest GRIDMAJOR.
    static constexpr int kMaxGridMajor = 100;

    bool gridOn{true};  ///< GRIDMODE, VPORT 76
    /// GRIDUNIT, VPORT 15 and 25, each >= 0. An axis at 0 takes the snap
    /// spacing of that axis (0,0 both); the value is kept as written
    /// (docs/DECISIONI.md, 2026-11-22, "Revisione delle interfacce PT5").
    Vec2 gridUnit{10, 10};
    bool snapOn{false};     ///< SNAPMODE, VPORT 75
    Vec2 snapUnit{10, 10};  ///< SNAPUNIT, VPORT 14 and 24, each > 0
    Vec2 snapBase{0, 0};    ///< SNAPBASE, VPORT 13 and 23
    int gridDisplay{3};     ///< GRIDDISPLAY, VPORT 60: bits 2 and 4 used, the others kept
    int gridMajor{5};       ///< GRIDMAJOR, VPORT 61, 1 to kMaxGridMajor
    int ucsIcon{3};         ///< UCSICON, VPORT 74: bit 1 on, bit 2 at the origin
    double snapAngle{0};    ///< SNAPANG, VPORT 50, in degrees: kept, without effect
    int snapStyle{0};       ///< SNAPSTYL, VPORT 77: kept, without effect
    int snapIsoPair{0};     ///< SNAPISOPAIR, VPORT 78: kept, without effect

    friend bool operator==(const GridSettings&, const GridSettings&) = default;
};

/// True when every value of `grid` is in its range: finite numbers, the
/// grid spacing >= 0, the snap spacing > 0, GRIDMAJOR from 1 to 100.
[[nodiscard]] inline bool isValid(const GridSettings& grid) {
    const auto finite = [](const Vec2& point) {
        return std::isfinite(point.x) && std::isfinite(point.y);
    };
    return finite(grid.gridUnit) && finite(grid.snapUnit) && finite(grid.snapBase) &&
           std::isfinite(grid.snapAngle) && grid.gridUnit.x >= 0.0 && grid.gridUnit.y >= 0.0 &&
           grid.snapUnit.x > 0.0 && grid.snapUnit.y > 0.0 && grid.gridMajor >= 1 &&
           grid.gridMajor <= GridSettings::kMaxGridMajor;
}

/// ATTDISP, the display of the attributes of the inserts: $ATTMODE 0, 1
/// and 2 (docs/adr/0056, point 1). Normal draws the attributes that are not
/// invisible, On all of them, Off none.
enum class AttributeDisplay : std::uint8_t { Off, Normal, On };

}  // namespace tg::core
