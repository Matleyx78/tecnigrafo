// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/raw_drawing.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
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

/// The kind of a dimension, from DXF group 70 & 7.
enum class DimensionKind : std::uint8_t { Linear, Aligned, Angular, Diameter, Radius, Other };

/// What a dimension measures: the DXF definition points (docs/adr/0047, point 1).
struct DimensionDefinition {
    DimensionKind kind{DimensionKind::Linear};
    int dxfType{0};        ///< 70 & 7
    Vec2 linePoint;        ///< 10
    Vec2 point13;          ///< 13
    Vec2 point14;          ///< 14
    Vec2 point15;          ///< 15
    Vec2 point16;          ///< 16
    double rotation{0.0};  ///< 50, radians, linear only

    friend bool operator==(const DimensionDefinition&, const DimensionDefinition&) = default;
};

/// How a dimension shows (docs/adr/0047, points 1 and 3).
struct DimensionPresentation {
    std::string style{"ISO-25"};         ///< 3
    Vec2 textPoint;                      ///< 11, the middle of the text
    std::string textOverride;            ///< 1
    std::optional<double> textRotation;  ///< 53, radians; empty: automatic
    bool userTextPosition{false};        ///< 70 bit 128
    /// The essential overridden variables, by name (PT22 fixes the value type).
    std::vector<std::pair<std::string, std::string>> overrides;

    friend bool operator==(const DimensionPresentation&, const DimensionPresentation&) = default;
};

/// Who decides the value (docs/adr/0044, point 5): today always the measure.
enum class ValueSource : std::uint8_t { Measured };

struct DimensionValue {
    ValueSource source{ValueSource::Measured};
    double measurement{0.0};  ///< 42

    friend bool operator==(const DimensionValue&, const DimensionValue&) = default;
};

/// DXF DIMENSION (docs/adr/0044, point 5; docs/adr/0047). Its graphics, the
/// anonymous *D block, are part of the entity.
struct Dimension {
    DimensionDefinition definition;
    DimensionPresentation presentation;
    DimensionValue value;
    std::vector<EntityRecord> graphics;  ///< the *D block, in order
    std::vector<RawEntity> rawGraphics;  ///< what of it the model does not handle

    friend bool operator==(const Dimension&, const Dimension&) = default;
};

// The functions of Dimension that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// Each one but supported() has the precondition supported(dimension): until the
// package of Dimension is merged, no reader and no command makes one, and the
// precondition is checked.

/// False until the package of Dimension is merged (docs/adr/0044, point 1).
[[nodiscard]] bool supported(const Dimension& dimension);

/// Every shape of the entity, in drawing coordinates, with its effective
/// properties, except those on frozen layers. A shape that is not drawn (on a
/// layer that is off, an invisible attribute) has properties.visible false:
/// generate() keeps only the drawn shapes, extent() measures them all
/// (docs/adr/0044, point 2).
[[nodiscard]] std::vector<GeneratedItem> generateOf(const Document& document,
                                                    const EntityRecord& record,
                                                    const Dimension& dimension);

/// The extent registered in the spatial index: the bounding box of every
/// shape of generateOf(), drawn or not.
[[nodiscard]] BBox extentOf(const Document& document, const EntityRecord& record,
                            const Dimension& dimension);

/// The distance from `point`: the least distance of the drawn shapes,
/// infinity without any.
[[nodiscard]] double distanceOf(const Document& document, const EntityRecord& record,
                                const Dimension& dimension, const Vec2& point);

/// Window selection: every drawn shape inside `rectangle`, and at least one.
[[nodiscard]] bool isInsideOf(const Document& document, const EntityRecord& record,
                              const Dimension& dimension, const BBox& rectangle);

/// Crossing selection: a drawn shape that crosses `rectangle`.
[[nodiscard]] bool intersectsOf(const Document& document, const EntityRecord& record,
                                const Dimension& dimension, const BBox& rectangle);

/// Appends the snap points that none of its shapes gives (as the insertion
/// point of a block reference): none.
void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record,
                           const Dimension& dimension, const SnapModes& modes,
                           std::vector<SnapPoint>& out, const std::optional<Vec2>& from);

/// The entity transformed by `similarity`, or a refusal (docs/adr/0044).
[[nodiscard]] TransformOutcome transformedOf(const Document& document, const Dimension& dimension,
                                             const Similarity& similarity, MirrorText mirrorText);

/// The grips, in order.
[[nodiscard]] std::vector<Vec2> gripPointsOf(const Dimension& dimension);

/// Moves grip `index` to `target`. Precondition: index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const Dimension& dimension,
                                    std::size_t index, const Vec2& target);

}  // namespace tg::core
