// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tg::core {

/// Grips in ADR 0034 order: line endpoints then midpoint; arc endpoints,
/// midpoint then centre; circle centre then quadrants; polyline vertices
/// interleaved with segment midpoints; text insertion then second alignment
/// point for Aligned and Fit. The types of the 0.8: gripPointsOf() of the type.
/// Precondition: isSupported(entity).
[[nodiscard]] std::vector<Vec2> gripPoints(const Entity& entity);

enum class GripOutcome : std::uint8_t { Moved, Invalid };

struct GripResult {
    GripOutcome outcome{GripOutcome::Invalid};
    Entity entity;  ///< The moved entity, or the unchanged input when invalid.
    /// Why the grip cannot apply, the message the command writes; empty when
    /// it is simply invalid (docs/adr/0044; moveGripIn() in generated.hpp).
    std::string refusal;
};

/// Move one grip according to ADR 0034, points 4 and 5. The types of the 0.8:
/// moveGripOf() of the type, without a document (moveGripIn() has one).
/// Preconditions: isSupported(entity); index is valid in gripPoints(entity).
[[nodiscard]] GripResult moveGrip(const Entity& entity, std::size_t index, const Vec2& target);

}  // namespace tg::core
