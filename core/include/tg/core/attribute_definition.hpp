// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/text.hpp"
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

/// DXF group 70 of ATTDEF and ATTRIB (docs/adr/0045, point 2).
struct AttributeFlags {
    bool invisible{false};  ///< bit 1
    bool constant{false};   ///< bit 2
    bool verify{false};     ///< bit 4
    bool preset{false};     ///< bit 8

    friend bool operator==(const AttributeFlags&, const AttributeFlags&) = default;
};

/// DXF ATTDEF (docs/adr/0045, point 2): a text with a tag and a prompt.
struct AttributeDefinition {
    Text text;           ///< text.value is the default value (1)
    std::string tag;     ///< 2
    std::string prompt;  ///< 3
    AttributeFlags flags;

    friend bool operator==(const AttributeDefinition&, const AttributeDefinition&) = default;
};

/// The bounding box: it does not depend on the document.
[[nodiscard]] BBox boundingBox(const AttributeDefinition& definition);

// The functions of AttributeDefinition that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// Each one but supported() has the precondition supported(definition): until the
// package of AttributeDefinition is merged, no reader and no command makes one, and the
// precondition is checked.

/// False until the package of AttributeDefinition is merged (docs/adr/0044, point 1).
[[nodiscard]] bool supported(const AttributeDefinition& definition);

/// Every shape of the entity, in drawing coordinates, with its effective
/// properties, except those on frozen layers. A shape that is not drawn (on a
/// layer that is off, an invisible attribute) has properties.visible false:
/// generate() keeps only the drawn shapes, extent() measures them all
/// (docs/adr/0044, point 2).
[[nodiscard]] std::vector<GeneratedItem> generateOf(const Document& document,
                                                    const EntityRecord& record,
                                                    const AttributeDefinition& definition);

/// The extent registered in the spatial index: the bounding box of every
/// shape of generateOf(), drawn or not.
[[nodiscard]] BBox extentOf(const Document& document, const EntityRecord& record,
                            const AttributeDefinition& definition);

/// The distance from `point`: the least distance of the drawn shapes,
/// infinity without any.
[[nodiscard]] double distanceOf(const Document& document, const EntityRecord& record,
                                const AttributeDefinition& definition, const Vec2& point);

/// Window selection: every drawn shape inside `rectangle`, and at least one.
[[nodiscard]] bool isInsideOf(const Document& document, const EntityRecord& record,
                              const AttributeDefinition& definition, const BBox& rectangle);

/// Crossing selection: a drawn shape that crosses `rectangle`.
[[nodiscard]] bool intersectsOf(const Document& document, const EntityRecord& record,
                                const AttributeDefinition& definition, const BBox& rectangle);

/// Appends the snap points that none of its shapes gives (as the insertion
/// point of a block reference): none.
void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record,
                           const AttributeDefinition& definition, const SnapModes& modes,
                           std::vector<SnapPoint>& out, const std::optional<Vec2>& from);

/// The entity transformed by `similarity`, or a refusal (docs/adr/0044).
[[nodiscard]] TransformOutcome transformedOf(const Document& document,
                                             const AttributeDefinition& definition,
                                             const Similarity& similarity, MirrorText mirrorText);

/// The grips, in order.
[[nodiscard]] std::vector<Vec2> gripPointsOf(const AttributeDefinition& definition);

/// Moves grip `index` to `target`. Precondition: index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const AttributeDefinition& definition,
                                    std::size_t index, const Vec2& target);

}  // namespace tg::core
