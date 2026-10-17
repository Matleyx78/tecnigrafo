// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/snap_points.hpp"
#include "tg/core/vec2.hpp"

namespace tg::cmd {

/// Object snap and ortho settings (docs/adr/0008). The defaults are those of
/// a new session: object snap on with every mode, ortho off.
struct DrawingAids {
    bool objectSnap{true};  ///< F3: all object snaps on or off
    core::SnapModes snapModes;
    bool ortho{false};  ///< F8
    friend bool operator==(const DrawingAids&, const DrawingAids&) = default;
};

/// `pointer` moved onto the horizontal or the vertical through `lastPoint`,
/// whichever is nearer to the direction of the pointer (horizontal when
/// the two are equally near).
[[nodiscard]] core::Vec2 orthoPoint(const core::Vec2& lastPoint, const core::Vec2& pointer);

}  // namespace tg::cmd
