// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
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

/// DXF group 71, in the order of its values (1 to 9).
enum class MTextAttachment : std::uint8_t {
    TopLeft = 1,
    TopCenter,
    TopRight,
    MiddleLeft,
    MiddleCenter,
    MiddleRight,
    BottomLeft,
    BottomCenter,
    BottomRight
};

/// DXF MTEXT (docs/adr/0046, points 1 and 2). Angles in radians.
struct MText {
    Vec2 insertion;                                        ///< 10
    double height{2.5};                                    ///< 40
    double width{0.0};                                     ///< 41, reference rectangle width
    MTextAttachment attachment{MTextAttachment::TopLeft};  ///< 71
    double rotation{0.0};                                  ///< 50 (or 11 as a direction)
    std::string style{"Standard"};                         ///< 7
    double lineSpacing{1.0};                               ///< 44
    bool exactSpacing{false};                              ///< 73 = 2
    bool directionByStyle{false};                          ///< 72 = 5
    std::string value;  ///< groups 3 and 1 joined, UTF-8, formatting codes kept

    friend bool operator==(const MText&, const MText&) = default;
};

// The functions of MText that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// Each one but supported() has the precondition supported(mtext): until the
// package of MText is merged, no reader and no command makes one, and the
// precondition is checked.

/// False until the package of MText is merged (docs/adr/0044, point 1).
[[nodiscard]] bool supported(const MText& mtext);

/// Every shape of the entity, in drawing coordinates, with its effective
/// properties, except those on frozen layers. A shape that is not drawn (on a
/// layer that is off, an invisible attribute) has properties.visible false:
/// generate() keeps only the drawn shapes, extent() measures them all
/// (docs/adr/0044, point 2).
[[nodiscard]] std::vector<GeneratedItem> generateOf(const Document& document,
                                                    const EntityRecord& record, const MText& mtext);

/// The extent registered in the spatial index: the bounding box of every
/// shape of generateOf(), drawn or not.
[[nodiscard]] BBox extentOf(const Document& document, const EntityRecord& record,
                            const MText& mtext);

/// The distance from `point`: the least distance of the drawn shapes,
/// infinity without any.
[[nodiscard]] double distanceOf(const Document& document, const EntityRecord& record,
                                const MText& mtext, const Vec2& point);

/// Window selection: every drawn shape inside `rectangle`, and at least one.
[[nodiscard]] bool isInsideOf(const Document& document, const EntityRecord& record,
                              const MText& mtext, const BBox& rectangle);

/// Crossing selection: a drawn shape that crosses `rectangle`.
[[nodiscard]] bool intersectsOf(const Document& document, const EntityRecord& record,
                                const MText& mtext, const BBox& rectangle);

/// Appends the snap points that none of its shapes gives (as the insertion
/// point of a block reference): none.
void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record, const MText& mtext,
                           const SnapModes& modes, std::vector<SnapPoint>& out,
                           const std::optional<Vec2>& from);

/// The entity transformed by `similarity`, or a refusal (docs/adr/0044).
[[nodiscard]] TransformOutcome transformedOf(const Document& document, const MText& mtext,
                                             const Similarity& similarity, MirrorText mirrorText);

/// The grips, in order.
[[nodiscard]] std::vector<Vec2> gripPointsOf(const MText& mtext);

/// Moves grip `index` to `target`. Precondition: index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const MText& mtext, std::size_t index,
                                    const Vec2& target);

}  // namespace tg::core
