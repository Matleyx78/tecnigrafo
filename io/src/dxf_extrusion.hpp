// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/entity_extras.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

/// Quota, thickness and extrusion of the entities: the groups kept as read
/// and the conversion of extrusion (0,0,-1) (docs/adr/0018, point 5).
namespace tg::io {

/// The direction of extrusion of an entity.
enum class Extrusion : std::uint8_t {
    Up,    ///< (0,0,1): the object coordinate system is the world one
    Down,  ///< (0,0,-1): the world one seen from below, x -> -x
    Other  ///< any other direction: the entity stays unhandled
};

/// (0,0,1) or (0,0,-1) when x and y are within 1e-12 of zero and z is
/// positive or negative; anything else is Other.
[[nodiscard]] Extrusion classifyExtrusion(double x, double y, double z);

/// The extrusion written by the groups 210/220/230 of an entity (its groups
/// after group 0), outside the 102 groups and before the XDATA. A missing
/// component is the one of (0,0,1); a value that is not a number is Other.
[[nodiscard]] Extrusion entityExtrusion(const core::RawGroups& groups);

/// A point between the object coordinate system of extrusion (0,0,-1) and
/// the world one. By the arbitrary axis algorithm the object X axis is
/// (0,1,0) x (0,0,-1) = (-1,0,0) and its Y axis (0,0,-1) x (-1,0,0) =
/// (0,1,0): x changes sign, y does not. The conversion is its own inverse.
[[nodiscard]] core::Vec2 flipX(const core::Vec2& point);

/// `value` with its sign changed, without producing -0.
[[nodiscard]] double negate(double value);

/// `degrees` brought into [0, 360).
[[nodiscard]] double normalizeDegrees(double degrees);

/// The direction `degrees` after x -> -x: 180 - degrees, in [0, 360). Its
/// own inverse.
[[nodiscard]] double flipDirectionDegrees(double degrees);

/// The anchor of every placement group (30, 31, 38, 39, 210, 220, 230) of an
/// entity, its groups after group 0, in the order they are read: the group
/// that precedes it in the entity subclasses. Splits the groups as
/// splitEntityGroups() does, so it pairs with EntityExtras::placement.
[[nodiscard]] std::vector<core::GroupAnchor> readPlacementAnchors(const core::RawGroups& groups);

/// Moves the placement groups that a codec wrote in `out`, from index
/// `from` (its first subclass marker) to the XDATA, after the anchors of
/// `extras`. Leaves `out` as it is when the extras have no anchors or one
/// of them is no longer there (the model changed): the codec's positions.
void restorePlacement(const core::EntityExtras& extras, std::size_t from, core::RawGroups& out);

}  // namespace tg::io
