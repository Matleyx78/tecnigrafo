// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/arc.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/line.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
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

/// DXF group 75 (docs/adr/0048, point 1).
enum class IslandStyle : std::uint8_t { Normal, Outer, Ignore };

/// An edge of a boundary path; a full circle is an Arc.
struct HatchEdge {
    std::variant<Line, Arc> curve;

    friend bool operator==(const HatchEdge&, const HatchEdge&) = default;
};

/// A boundary path: a polyline, or a list of edges.
struct HatchLoop {
    std::uint32_t flags{0};            ///< 92
    std::optional<Polyline> polyline;  ///< a polyline path, otherwise the edges
    std::vector<HatchEdge> edges;

    friend bool operator==(const HatchLoop&, const HatchLoop&) = default;
};

/// A line family of a pattern, in drawing coordinates (docs/adr/0048).
struct PatternLine {
    double angle{0.0};           ///< 53, radians
    Vec2 origin;                 ///< 43, 44
    Vec2 offset;                 ///< 45, 46
    std::vector<double> dashes;  ///< 49

    friend bool operator==(const PatternLine&, const PatternLine&) = default;
};

/// DXF HATCH (docs/adr/0048, point 1). Angles in radians.
struct Hatch {
    std::string pattern;                       ///< 2
    bool solid{false};                         ///< 70
    IslandStyle islands{IslandStyle::Normal};  ///< 75
    int patternType{1};                        ///< 76
    double angle{0.0};                         ///< 52
    double scale{1.0};                         ///< 41
    bool doubled{false};                       ///< 77
    std::vector<PatternLine> patternLines;
    std::vector<Vec2> seeds;  ///< 10, 20 after 98
    std::vector<HatchLoop> loops;

    friend bool operator==(const Hatch&, const Hatch&) = default;
};

// The functions of Hatch that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// Each one but supported() has the precondition supported(hatch): until the
// package of Hatch is merged, no reader and no command makes one, and the
// precondition is checked.

/// False until the package of Hatch is merged (docs/adr/0044, point 1).
[[nodiscard]] bool supported(const Hatch& hatch);

/// Every shape of the entity, in drawing coordinates, with its effective
/// properties, except those on frozen layers. A shape that is not drawn (on a
/// layer that is off, an invisible attribute) has properties.visible false:
/// generate() keeps only the drawn shapes, extent() measures them all
/// (docs/adr/0044, point 2).
[[nodiscard]] std::vector<GeneratedItem> generateOf(const Document& document,
                                                    const EntityRecord& record, const Hatch& hatch);

/// The extent registered in the spatial index: the bounding box of every
/// shape of generateOf(), drawn or not.
[[nodiscard]] BBox extentOf(const Document& document, const EntityRecord& record,
                            const Hatch& hatch);

/// The distance from `point`: the least distance of the drawn shapes,
/// infinity without any.
[[nodiscard]] double distanceOf(const Document& document, const EntityRecord& record,
                                const Hatch& hatch, const Vec2& point);

/// Window selection: every drawn shape inside `rectangle`, and at least one.
[[nodiscard]] bool isInsideOf(const Document& document, const EntityRecord& record,
                              const Hatch& hatch, const BBox& rectangle);

/// Crossing selection: a drawn shape that crosses `rectangle`.
[[nodiscard]] bool intersectsOf(const Document& document, const EntityRecord& record,
                                const Hatch& hatch, const BBox& rectangle);

/// Appends the snap points that none of its shapes gives (as the insertion
/// point of a block reference): none.
void appendOwnSnapPointsOf(const Document& document, const EntityRecord& record, const Hatch& hatch,
                           const SnapModes& modes, std::vector<SnapPoint>& out,
                           const std::optional<Vec2>& from);

/// The entity transformed by `similarity`, or a refusal (docs/adr/0044).
[[nodiscard]] TransformOutcome transformedOf(const Document& document, const Hatch& hatch,
                                             const Similarity& similarity, MirrorText mirrorText);

/// The grips, in order.
[[nodiscard]] std::vector<Vec2> gripPointsOf(const Hatch& hatch);

/// Moves grip `index` to `target`. Precondition: index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const Hatch& hatch, std::size_t index,
                                    const Vec2& target);

}  // namespace tg::core
