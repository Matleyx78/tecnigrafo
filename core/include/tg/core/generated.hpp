// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/arc.hpp"
#include "tg/core/bbox.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/line.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/core/solid.hpp"
#include "tg/core/text.hpp"
#include "tg/core/transform2d.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

/// The generated geometry of an entity (docs/adr/0044, point 2): what it
/// draws, as shapes already in drawing coordinates, and the questions that
/// selection, object snap and the spatial index ask about it.
///
/// For the five entities of before the 0.8 (polyline, line, circle, arc and
/// text) every function gives what the functions of the entity give
/// (boundingBox, distance, …): generate() gives the entity itself. For the
/// other types they call the functions of the type (solid.hpp, insert.hpp, …).
namespace tg::core {

/// A filled region (docs/adr/0044, point 2): closed loops of segments and
/// arcs, filled with the even-odd rule, mapped to the drawing by `transform`.
struct FilledRegion {
    std::vector<Polyline> loops;  ///< closed, even-odd
    Transform2D transform;        ///< identity unless inside a non-uniform insert
};

using GeneratedShape =
    std::variant<Line, Arc, Circle, Ellipse, Polyline, Text, Solid, FilledRegion>;

struct GeneratedItem {
    GeneratedShape shape;            ///< in drawing coordinates (docs/adr/0044, point 2)
    EffectiveProperties properties;  ///< resolved
};

/// The exact image of a shape under an affine map (docs/adr/0044, point 2;
/// docs/adr/0052, point 2): circles and arcs become ellipses, a polyline with
/// arcs splits into lines and elliptical arcs, text is decomposed.
///
/// A similarity (exactly: the linear part is a rotation and a uniform scale,
/// possibly with a reflection) goes through transformed(), text mirrored as
/// MIRRTEXT 1 (MirrorText::Mirrored); a filled region keeps the map with it,
/// composed with its own. The other affine maps are realized by the packages
/// of the ellipse (circles, arcs, ellipses), of the attributes (text) and of
/// the blocks (the rest): until then the result is empty.
[[nodiscard]] std::vector<GeneratedShape> transformedShape(const GeneratedShape& shape,
                                                           const Transform2D& transform);

/// The shapes the entity draws, with the properties they are drawn with, in
/// drawing order. Nothing on a layer that is off or frozen, nor an invisible
/// attribute (docs/tappe/0.8/PT6-scheletro-core.md, point 3).
/// Precondition: isSupported(record.entity).
[[nodiscard]] std::vector<GeneratedItem> generate(const Document& document,
                                                  const EntityRecord& record);

/// The extent of the whole geometry of the entity, as AutoCAD's zoom extents:
/// with the shapes on layers that are off, not those on frozen layers. For
/// the five entities of before the 0.8, boundingBox(record.entity), whatever
/// its layer. Document registers it in the spatial index.
/// Precondition: isSupported(record.entity).
[[nodiscard]] BBox extent(const Document& document, const EntityRecord& record);

/// distance(const Entity&, const Vec2&), through the drawn shapes.
[[nodiscard]] double distance(const Document& document, const EntityRecord& record,
                              const Vec2& point);

/// isInside(const Entity&, const BBox&), through the drawn shapes.
[[nodiscard]] bool isInside(const Document& document, const EntityRecord& record,
                            const BBox& rectangle);

/// intersects(const Entity&, const BBox&), through the drawn shapes.
[[nodiscard]] bool intersects(const Document& document, const EntityRecord& record,
                              const BBox& rectangle);

/// appendSnapPoints(const Entity&, …): those of every drawn shape, in order,
/// then those of the entity that no shape gives (appendOwnSnapPointsOf()).
void appendSnapPoints(const Document& document, const EntityRecord& record, const SnapModes& modes,
                      std::vector<SnapPoint>& out, const std::optional<Vec2>& from);

/// The result of a transformation through the document: the entity, or a
/// refusal, the message that the command writes (docs/adr/0044).
struct TransformOutcome {
    std::optional<Entity> entity;
    std::string refusal;  ///< empty when entity has a value
};

/// transformed(), or the refusal of the type. The five entities of before the
/// 0.8 are never refused. Precondition: isSupported(entity).
[[nodiscard]] TransformOutcome transformedIn(const Document& document, const Entity& entity,
                                             const Similarity& similarity, MirrorText mirrorText);

/// moveGrip(), through the type; a grip that cannot apply is Invalid with a
/// non-empty GripResult::refusal. Preconditions: isSupported(entity), and
/// index is valid in gripPoints(entity).
[[nodiscard]] GripResult moveGripIn(const Document& document, const Entity& entity,
                                    std::size_t index, const Vec2& target);

// --- Questions on one shape, for the visits and the packages of the types ---

/// The bounding box of a shape. A filled region under a map other than the
/// identity gives the box of the mapped corners of its loops' boxes: it
/// contains the region, and is exact for a scaling along the axes.
[[nodiscard]] BBox shapeBoundingBox(const GeneratedShape& shape);

/// The distance from `point`: that of the entity of the same type; a filled
/// region is 0 where it is filled, otherwise the distance to its boundary.
[[nodiscard]] double shapeDistance(const GeneratedShape& shape, const Vec2& point);

/// Window selection, as for the entity of the same type; a filled region is
/// inside when its whole boundary is.
[[nodiscard]] bool shapeIsInside(const GeneratedShape& shape, const BBox& rectangle);

/// Crossing selection, as for the entity of the same type; a filled region
/// is crossed when its boundary is, or when the rectangle lies where it is
/// filled.
[[nodiscard]] bool shapeIntersects(const GeneratedShape& shape, const BBox& rectangle);

/// The snap points, as for the entity of the same type; none for a filled region.
void appendShapeSnapPoints(const GeneratedShape& shape, const SnapModes& modes,
                           std::vector<SnapPoint>& out, const std::optional<Vec2>& from);

/// The shape as an entity of the same type; std::nullopt for a filled region.
[[nodiscard]] std::optional<Entity> shapeEntity(const GeneratedShape& shape);

/// The box of every item, drawn or not: the default extentOf() of a type.
[[nodiscard]] BBox extentOfItems(std::span<const GeneratedItem> items);

/// The least distance of the drawn items, infinity without any.
[[nodiscard]] double distanceToDrawnItems(std::span<const GeneratedItem> items, const Vec2& point);

/// True when there is a drawn item and every drawn item is inside.
[[nodiscard]] bool drawnItemsInside(std::span<const GeneratedItem> items, const BBox& rectangle);

/// True when a drawn item crosses the rectangle.
[[nodiscard]] bool drawnItemsIntersect(std::span<const GeneratedItem> items, const BBox& rectangle);

}  // namespace tg::core
