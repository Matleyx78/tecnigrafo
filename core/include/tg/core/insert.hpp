// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/entity_extras.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/layer.hpp"
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

/// DXF ATTRIB, owned by its Insert (docs/adr/0045, point 2): it has its own
/// handle, extras and properties, and leaves and comes back with the insert.
struct Attribute {
    EntityId id;                  ///< its handle
    Text text;                    ///< text.value is the value (1)
    std::string tag;              ///< 2
    bool invisible{false};        ///< 70 bit 1
    EntityExtras extras;          ///< what the model does not represent
    EntityProperties properties;  ///< layer, colour, linetype, …

    friend bool operator==(const Attribute&, const Attribute&) = default;
};

/// DXF INSERT (docs/adr/0045, points 2 and 4). Angles in radians.
struct Insert {
    std::string block;                  ///< 2, the name of the block definition
    Vec2 insertion;                     ///< 10
    double scaleX{1.0};                 ///< 41
    double scaleY{1.0};                 ///< 42
    double rotation{0.0};               ///< 50
    std::vector<Attribute> attributes;  ///< the ATTRIB that follow it, in order

    friend bool operator==(const Insert&, const Insert&) = default;
};

/// True when `record` depends on the block definition `name`: an insert of
/// it, or of a block that contains it at any depth. Document::replaceBlock()
/// re-indexes the entities for which it is true. False for now: the package
/// of Insert fills it.
[[nodiscard]] bool dependsOnBlock(const Document& document, const EntityRecord& record,
                                  std::string_view name);

// The functions of Insert that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// Each one but supported() has the precondition supported(insert): until the
// package of Insert is merged, no reader and no command makes one, and the
// precondition is checked.

/// False until the package of Insert is merged (docs/adr/0044, point 1).
[[nodiscard]] bool supported(const Insert& insert);

/// Every shape of the entity, in drawing coordinates, with its effective
/// properties, except those on frozen layers. A shape that is not drawn (on a
/// layer that is off, an invisible attribute) has properties.visible false:
/// generate() keeps only the drawn shapes, extent() measures them all
/// (docs/adr/0044, point 2).
[[nodiscard]] std::vector<GeneratedItem> generateOf(const Document& document,
                                                    const EntityRecord& record,
                                                    const Insert& insert);

/// The extent registered in the spatial index: the bounding box of every
/// shape of generateOf(), drawn or not.
[[nodiscard]] BBox extentOf(const Document& document, const EntityRecord& record,
                            const Insert& insert);

/// The distance from `point`: the least distance of the drawn shapes,
/// infinity without any.
[[nodiscard]] double distanceOf(const Document& document, const EntityRecord& record,
                                const Insert& insert, const Vec2& point);

/// Window selection: every drawn shape inside `rectangle`, and at least one.
[[nodiscard]] bool isInsideOf(const Document& document, const EntityRecord& record,
                              const Insert& insert, const BBox& rectangle);

/// Crossing selection: a drawn shape that crosses `rectangle`.
[[nodiscard]] bool intersectsOf(const Document& document, const EntityRecord& record,
                                const Insert& insert, const BBox& rectangle);

/// Appends the snap points that none of its shapes gives (as the insertion
/// point of a block reference): none.
void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record,
                           const Insert& insert, const SnapModes& modes,
                           std::vector<SnapPoint>& out, const std::optional<Vec2>& from);

/// The entity transformed by `similarity`, or a refusal (docs/adr/0044).
[[nodiscard]] TransformOutcome transformedOf(const Document& document, const Insert& insert,
                                             const Similarity& similarity, MirrorText mirrorText);

/// The grips, in order.
[[nodiscard]] std::vector<Vec2> gripPointsOf(const Insert& insert);

/// Moves grip `index` to `target`. Precondition: index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const Insert& insert,
                                    std::size_t index, const Vec2& target);

}  // namespace tg::core
