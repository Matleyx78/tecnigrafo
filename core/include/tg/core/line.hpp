// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/vec2.hpp"

namespace tg::core {

/// A straight line between two points (docs/adr/0012), as the DXF LINE
/// entity: start 10/20, end 11/21. The DXF properties that a plane drawing
/// does not use (Z 30/31, thickness 39, extrusion 210/220/230) have their
/// default values (docs/adr/0012, point 2).
struct Line {
    Vec2 start;
    Vec2 end;

    [[nodiscard]] double length() const;
    [[nodiscard]] BBox boundingBox() const;

    friend constexpr bool operator==(const Line&, const Line&) = default;
};

}  // namespace tg::core
