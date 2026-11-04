// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/tolerance.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace tg::core {

/// The result of JOIN (docs/adr/0035, points 17-20). `source` is the source
/// entity as it becomes, when its type does not change; otherwise empty, and
/// `created` holds the new entity. `consumed` lists the indexes, into the
/// input, of the entities the result absorbs (the source excluded).
struct JoinResult {
    std::optional<Entity> source;
    std::optional<Entity> created;
    std::vector<std::size_t> consumed;
    /// Arcs of the source's circle that reach the whole circle (docs/adr/0035,
    /// point 17): `created` is the circle, which the command applies only if
    /// the user answers Yes to "Convert to circle?"; with No nothing joins.
    /// Never set with `close` of joinToSource, which asks nothing.
    bool formsCircle{false};
};

/// JOIN of `entities`, entities[0] the source, the others in the order they
/// were chosen: aligned lines into a line, when the source and every other
/// entity are lines aligned with the source; arcs of one circle into an arc
/// (or, reaching the whole circle, into a circle: `formsCircle`), when the
/// source and every other entity are arcs on the source's circle; otherwise a
/// chain of contiguous lines, arcs and polylines into a polyline.
/// std::nullopt when nothing joins.
[[nodiscard]] std::optional<JoinResult> join(std::span<const Entity> entities,
                                             const ToleranceRule& rule);

/// The script sequence (docs/adr/0035, point 19): only what the source's type
/// accepts (a line: aligned lines; an arc: arcs of its circle; a polyline:
/// contiguous lines, arcs and polylines). `close` (arcs only) closes the arc
/// into a circle.
[[nodiscard]] std::optional<JoinResult> joinToSource(std::span<const Entity> entities,
                                                     const ToleranceRule& rule, bool close = false);

}  // namespace tg::core
