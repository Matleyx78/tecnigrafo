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
#include <string_view>
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

/// The text the definition shows when it stands in the drawing: its text
/// with the tag as value (docs/adr/0045, point 6). Geometry, selection, snap
/// and grips are those of this text.
[[nodiscard]] Text shownText(const AttributeDefinition& definition);

/// True when `text` is valid as a TEXT: finite numbers, height > 0 and
/// width factor > 0 (text.hpp). The rule of the ATTDEF and of the ATTRIB.
[[nodiscard]] bool validAttributeText(const Text& text);

/// True when `tag` is a valid tag of an ATTDEF or ATTRIB: not empty and
/// without blanks (space, tab, line feed, …), as AutoCAD asks.
[[nodiscard]] bool validAttributeTag(std::string_view tag);

/// The bounding box of shownText(): it does not depend on the document.
[[nodiscard]] BBox boundingBox(const AttributeDefinition& definition);

// The functions of AttributeDefinition that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// Each one but supported() has the precondition supported(definition), which is checked.
// In the drawing an ATTDEF behaves as the TEXT shownText(); inside a block it is not
// drawn (insert.hpp).

/// True when the text is valid as a TEXT (validAttributeText()) and the tag
/// is valid (validAttributeTag()) (docs/tappe/0.8/PT17-attributi-core.md,
/// point 1).
[[nodiscard]] bool supported(const AttributeDefinition& definition);

/// Every shape of the entity, in drawing coordinates, with its effective
/// properties, except those on frozen layers. A shape that is not drawn (on a
/// layer that is off, an invisible attribute) has properties.visible false:
/// generate() keeps only the drawn shapes, extent() measures them all
/// (docs/adr/0044, point 2).
///
/// The text shownText(), drawn even with the Invisible flag and whatever
/// Document::attributeDisplay(), as in AutoCAD (docs/adr/0056, point 1).
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
/// point of a block reference): none, the text gives them.
void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record,
                           const AttributeDefinition& definition, const SnapModes& modes,
                           std::vector<SnapPoint>& out, const std::optional<Vec2>& from);

/// The entity transformed by `similarity` as the text shownText() with
/// `mirrorText` (ADR 0031, points 2 and 3), the default value, tag, prompt
/// and flags kept. Never refused.
[[nodiscard]] TransformOutcome transformedOf(const Document& document,
                                             const AttributeDefinition& definition,
                                             const Similarity& similarity, MirrorText mirrorText);

/// The grips, in order: those of the text shownText().
[[nodiscard]] std::vector<Vec2> gripPointsOf(const AttributeDefinition& definition);

/// Moves grip `index` to `target` as for the text shownText(), the default
/// value, tag, prompt and flags kept. Precondition:
/// index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const AttributeDefinition& definition,
                                    std::size_t index, const Vec2& target);

}  // namespace tg::core
