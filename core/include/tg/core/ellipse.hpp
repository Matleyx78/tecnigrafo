// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace tg::core {

class Document;
class Similarity;
struct EntityRecord;
struct GeneratedItem;
struct GripResult;
struct SnapModes;
struct SnapPoint;
struct TransformOutcome;
enum class MirrorText : std::uint8_t;

/// DXF ELLIPSE (docs/adr/0052). Angles and parameters in radians.
struct Ellipse {
    Vec2 center;                 ///< 10
    Vec2 majorAxis;              ///< 11, relative to the center
    double ratio{1.0};           ///< 40, minor over major axis, in (0, 1]
    double startParameter{0.0};  ///< 41
    double endParameter{0.0};    ///< 42; 2*pi with a start of 0 is the full ellipse

    friend bool operator==(const Ellipse&, const Ellipse&) = default;
};

/// The bounding box: it does not depend on the document.
[[nodiscard]] BBox boundingBox(const Ellipse& ellipse);

// Ellipse is also a shape of the generated geometry (generated.hpp): the
// questions on a shape, which do not depend on the document.

/// Distance from `point`, as distance(const Entity&, const Vec2&).
[[nodiscard]] double distanceOf(const Ellipse& ellipse, const Vec2& point);

/// Window selection, as isInside(const Entity&, const BBox&).
[[nodiscard]] bool isInsideOf(const Ellipse& ellipse, const BBox& rectangle);

/// Crossing selection, as intersects(const Entity&, const BBox&).
[[nodiscard]] bool intersectsOf(const Ellipse& ellipse, const BBox& rectangle);

/// Appends the enabled snap points, as appendSnapPoints(const Entity&, …).
void appendSnapPointsOf(const Ellipse& ellipse, const SnapModes& modes, std::vector<SnapPoint>& out,
                        const std::optional<Vec2>& from);

// The functions of Ellipse that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// Each one but supported() has the precondition supported(ellipse): until the
// package of Ellipse is merged, no reader and no command makes one, and the
// precondition is checked.

/// False until the package of Ellipse is merged (docs/adr/0044, point 1).
[[nodiscard]] bool supported(const Ellipse& ellipse);

/// Every shape of the entity, in drawing coordinates, with its effective
/// properties, except those on frozen layers. A shape that is not drawn (on a
/// layer that is off, an invisible attribute) has properties.visible false:
/// generate() keeps only the drawn shapes, extent() measures them all
/// (docs/adr/0044, point 2).
[[nodiscard]] std::vector<GeneratedItem> generateOf(const Document& document,
                                                    const EntityRecord& record,
                                                    const Ellipse& ellipse);

/// The extent registered in the spatial index: the bounding box of every
/// shape of generateOf(), drawn or not.
[[nodiscard]] BBox extentOf(const Document& document, const EntityRecord& record,
                            const Ellipse& ellipse);

/// The distance from `point`: the least distance of the drawn shapes,
/// infinity without any.
[[nodiscard]] double distanceOf(const Document& document, const EntityRecord& record,
                                const Ellipse& ellipse, const Vec2& point);

/// Window selection: every drawn shape inside `rectangle`, and at least one.
[[nodiscard]] bool isInsideOf(const Document& document, const EntityRecord& record,
                              const Ellipse& ellipse, const BBox& rectangle);

/// Crossing selection: a drawn shape that crosses `rectangle`.
[[nodiscard]] bool intersectsOf(const Document& document, const EntityRecord& record,
                                const Ellipse& ellipse, const BBox& rectangle);

/// Appends the snap points that none of its shapes gives (as the insertion
/// point of a block reference): none.
void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record,
                           const Ellipse& ellipse, const SnapModes& modes,
                           std::vector<SnapPoint>& out, const std::optional<Vec2>& from);

/// The entity transformed by `similarity`, or a refusal (docs/adr/0044).
[[nodiscard]] TransformOutcome transformedOf(const Document& document, const Ellipse& ellipse,
                                             const Similarity& similarity, MirrorText mirrorText);

/// The grips, in order.
[[nodiscard]] std::vector<Vec2> gripPointsOf(const Ellipse& ellipse);

/// Moves grip `index` to `target`. Precondition: index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const Ellipse& ellipse,
                                    std::size_t index, const Vec2& target);

}  // namespace tg::core
