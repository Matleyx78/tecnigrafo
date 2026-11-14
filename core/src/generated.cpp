// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/generated.hpp"

#include "solid_space/robust/trigonometry.hpp"
#include "tg/core/attribute_definition.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/insert.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/proximity.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kInfinity = std::numeric_limits<double>::infinity();

/// The five entities of before the 0.8, which answer by themselves.
template <typename T>
constexpr bool kAnswersByItself =
    std::is_same_v<T, Polyline> || std::is_same_v<T, Line> || std::is_same_v<T, Circle> ||
    std::is_same_v<T, Arc> || std::is_same_v<T, Text>;

/// The shape types that are also entities and answer as shapes by
/// themselves, without the document (solid.hpp, ellipse.hpp).
template <typename T>
constexpr bool kShapeOfItsOwn = std::is_same_v<T, Solid> || std::is_same_v<T, Ellipse>;

bool isIdentity(const Transform2D& transform) {
    return transform.apply(Vec2{}) == Vec2{} &&
           transform.applyToVector({1.0, 0.0}) == Vec2{1.0, 0.0} &&
           transform.applyToVector({0.0, 1.0}) == Vec2{0.0, 1.0};
}

/// The similarity equal to `transform`, or std::nullopt when its linear part
/// is not exactly a rotation and a uniform scale, possibly with a reflection.
std::optional<Similarity> similarityOf(const Transform2D& transform) {
    const Vec2 xAxis = transform.applyToVector({1.0, 0.0});
    const Vec2 yAxis = transform.applyToVector({0.0, 1.0});
    const bool reflects = transform.determinant() < 0.0;
    const bool similar = reflects ? (yAxis.x == xAxis.y && yAxis.y == -xAxis.x)
                                  : (yAxis.x == -xAxis.y && yAxis.y == xAxis.x);
    if (!similar) {
        return std::nullopt;
    }
    const std::optional<Similarity> scaling =
        Similarity::scaling(std::sqrt(xAxis.lengthSquared()), Vec2{});
    if (!scaling) {
        return std::nullopt;
    }
    // p' = t + R * s * F * p: F, the reflection across the X axis, first.
    Similarity result = Similarity::translation(transform.apply(Vec2{})) *
                        Similarity::rotation(solid_space::robust::atan2(xAxis.y, xAxis.x), Vec2{}) *
                        *scaling;
    if (reflects) {
        const std::optional<Similarity> acrossX = Similarity::reflection(Vec2{}, Vec2{1.0, 0.0});
        assert(acrossX.has_value());
        if (acrossX) {
            result = result * *acrossX;
        }
    }
    return result;
}

/// An entity of a shape type as the shape.
GeneratedShape shapeOf(const Entity& entity) {
    return std::visit(
        [](const auto& value) -> GeneratedShape {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_constructible_v<GeneratedShape, const T&>) {
                return value;
            } else {
                // Never: transformed() keeps the type of a shape.
                return Polyline{};
            }
        },
        entity);
}

// --- Curves: the shapes other than a filled region ---------------------------

double curveDistance(const GeneratedShape& shape, const Vec2& point) {
    return std::visit(
        [&](const auto& value) -> double {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, FilledRegion>) {
                return kInfinity;  // never asked: see shapeDistance()
            } else if constexpr (kShapeOfItsOwn<T>) {
                return distanceOf(value, point);
            } else {
                return distance(Entity{value}, point);
            }
        },
        shape);
}

bool curveIsInside(const GeneratedShape& shape, const BBox& rectangle) {
    return std::visit(
        [&](const auto& value) -> bool {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, FilledRegion>) {
                return false;  // never asked: see shapeIsInside()
            } else if constexpr (kShapeOfItsOwn<T>) {
                return isInsideOf(value, rectangle);
            } else {
                return isInside(Entity{value}, rectangle);
            }
        },
        shape);
}

bool curveIntersects(const GeneratedShape& shape, const BBox& rectangle) {
    return std::visit(
        [&](const auto& value) -> bool {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, FilledRegion>) {
                return false;  // never asked: see shapeIntersects()
            } else if constexpr (kShapeOfItsOwn<T>) {
                return intersectsOf(value, rectangle);
            } else {
                return intersects(Entity{value}, rectangle);
            }
        },
        shape);
}

// --- Filled regions ----------------------------------------------------------

/// The side of `point` with respect to the line from `start` to `end`: the
/// sign of cross(end - start, point - start), positive on the left. It is
/// computed from the endpoints in a fixed order, so the reversed segment gives
/// exactly the opposite value. A point on the line goes where the ray test of
/// crossSegment() puts it: moved right (+X), then up (+Y), so that neither an
/// edge through it nor a vertex at its height counts.
double sideOf(const Vec2& start, const Vec2& end, const Vec2& point) {
    const bool inOrder = start.x < end.x || (start.x == end.x && start.y < end.y);
    double side = inOrder ? cross(end - start, point - start) : -cross(start - end, point - end);
    if (side == 0.0) {
        const Vec2 chord = end - start;
        side = chord.y != 0.0 ? -chord.y : chord.x;
    }
    return side;
}

/// Flips `inside` when the ray from `point` towards +X crosses the segment
/// from `start` to `end` with `bulge`: once for its chord, once more when the
/// point lies between the chord and the arc. Both decisions use the same side
/// of the chord (sideOf()), whichever the direction of the loop. `bulge` is 0
/// for a segment that PolylineSegment::isArc() takes as straight
/// (docs/adr/0036, point 7).
void crossSegment(const Vec2& start, const Vec2& end, double bulge, const Vec2& point,
                  bool& inside) {
    const double pointSide = sideOf(start, end, point);
    if ((start.y > point.y) != (end.y > point.y)) {
        // The chord crosses the height of the point: to its right when the
        // point is left of a chord going up, or right of one going down.
        const bool up = end.y > start.y;
        if (up ? pointSide > 0.0 : pointSide < 0.0) {
            inside = !inside;
        }
    }
    if (bulge == 0.0) {
        return;
    }
    const ArcGeometry arc = arcFromBulge(start, end, bulge);
    const Vec2 chord = end - start;
    // A positive bulge runs counter-clockwise: the arc lies right of the chord.
    const Vec2 middle = ((start + end) * 0.5) + (Vec2{chord.y, -chord.x} * (bulge * 0.5));
    const double arcSide = cross(chord, middle - start);
    if (distance(point, arc.center) < arc.radius &&
        ((pointSide > 0.0 && arcSide > 0.0) || (pointSide < 0.0 && arcSide < 0.0))) {
        inside = !inside;
    }
}

/// Even-odd parity of `point` for one loop, closed even when its flag is not.
bool loopEncloses(const Polyline& loop, const Vec2& point) {
    bool inside = false;
    for (std::size_t i = 0; i < loop.segmentCount(); ++i) {
        const PolylineSegment segment = loop.segment(i);
        crossSegment(segment.start, segment.end, segment.isArc() ? segment.bulge : 0.0, point,
                     inside);
    }
    if (!loop.isClosed() && loop.vertexCount() > 1) {
        crossSegment(loop.vertices().back().position, loop.vertices().front().position, 0.0, point,
                     inside);
    }
    return inside;
}

/// True where the region is filled: the even-odd rule over all its loops.
bool regionFilledAt(const FilledRegion& region, const Vec2& point) {
    const std::optional<Transform2D> inverse = region.transform.inverted();
    if (!inverse) {
        return false;
    }
    const Vec2 local = inverse->apply(point);
    bool filled = false;
    for (const Polyline& loop : region.loops) {
        if (loopEncloses(loop, local)) {
            filled = !filled;
        }
    }
    return filled;
}

/// The boundary of a region, in drawing coordinates: curves only, never a
/// filled region.
std::vector<GeneratedShape> regionBoundary(const FilledRegion& region) {
    std::vector<GeneratedShape> boundary;
    const bool identity = isIdentity(region.transform);
    for (const Polyline& loop : region.loops) {
        if (identity) {
            boundary.emplace_back(loop);
            continue;
        }
        for (GeneratedShape& piece : transformedShape(loop, region.transform)) {
            boundary.push_back(std::move(piece));
        }
    }
    return boundary;
}

BBox regionBoundingBox(const FilledRegion& region) {
    BBox box;
    const bool identity = isIdentity(region.transform);
    for (const Polyline& loop : region.loops) {
        const BBox own = loop.boundingBox();
        if (own.isEmpty()) {
            continue;
        }
        if (identity) {
            box.expand(own);
            continue;
        }
        const std::array corners{own.min(), Vec2{own.max().x, own.min().y}, own.max(),
                                 Vec2{own.min().x, own.max().y}};
        for (const Vec2& corner : corners) {
            box.expand(region.transform.apply(corner));
        }
    }
    return box;
}

double regionDistance(const FilledRegion& region, const Vec2& point) {
    if (regionFilledAt(region, point)) {
        return 0.0;
    }
    double nearest = kInfinity;
    for (const GeneratedShape& piece : regionBoundary(region)) {
        nearest = std::min(nearest, curveDistance(piece, point));
    }
    return nearest;
}

bool regionInside(const FilledRegion& region, const BBox& rectangle) {
    const std::vector<GeneratedShape> boundary = regionBoundary(region);
    return !boundary.empty() && std::ranges::all_of(boundary, [&](const GeneratedShape& piece) {
        return curveIsInside(piece, rectangle);
    });
}

bool regionIntersects(const FilledRegion& region, const BBox& rectangle) {
    if (rectangle.isEmpty()) {
        return false;
    }
    const std::vector<GeneratedShape> boundary = regionBoundary(region);
    return std::ranges::any_of(
               boundary,
               [&](const GeneratedShape& piece) { return curveIntersects(piece, rectangle); }) ||
           regionFilledAt(region, rectangle.min());
}

}  // namespace

std::vector<GeneratedShape> transformedShape(const GeneratedShape& shape,
                                             const Transform2D& transform) {
    const std::optional<Similarity> similarity = similarityOf(transform);
    if (!similarity) {
        // The other affine maps: docs/tappe/0.8, PT48, PT17 and PT16.
        return {};
    }
    if (const auto* region = std::get_if<FilledRegion>(&shape)) {
        return {FilledRegion{region->loops, transform * region->transform}};
    }
    const std::optional<Entity> entity = shapeEntity(shape);
    assert(entity.has_value());
    return {shapeOf(transformed(*entity, *similarity, MirrorText::Mirrored))};
}

std::vector<GeneratedItem> generate(const Document& document, const EntityRecord& record) {
    assert(isSupported(record.entity));
    return std::visit(
        [&](const auto& value) -> std::vector<GeneratedItem> {
            using T = std::decay_t<decltype(value)>;
            if constexpr (kAnswersByItself<T>) {
                if (!isVisible(document, record)) {
                    return {};
                }
                return {GeneratedItem{value, effectiveProperties(document, record)}};
            } else {
                std::vector<GeneratedItem> items = generateOf(document, record, value);
                std::erase_if(items,
                              [](const GeneratedItem& item) { return !item.properties.visible; });
                return items;
            }
        },
        record.entity);
}

BBox extent(const Document& document, const EntityRecord& record) {
    assert(isSupported(record.entity));
    return std::visit(
        [&](const auto& value) -> BBox {
            if constexpr (kAnswersByItself<std::decay_t<decltype(value)>>) {
                return value.boundingBox();
            } else {
                return extentOf(document, record, value);
            }
        },
        record.entity);
}

double distance(const Document& document, const EntityRecord& record, const Vec2& point) {
    return std::visit(
        [&](const auto& value) -> double {
            if constexpr (kAnswersByItself<std::decay_t<decltype(value)>>) {
                return distance(record.entity, point);
            } else {
                return distanceOf(document, record, value, point);
            }
        },
        record.entity);
}

bool isInside(const Document& document, const EntityRecord& record, const BBox& rectangle) {
    return std::visit(
        [&](const auto& value) -> bool {
            if constexpr (kAnswersByItself<std::decay_t<decltype(value)>>) {
                return isInside(record.entity, rectangle);
            } else {
                return isInsideOf(document, record, value, rectangle);
            }
        },
        record.entity);
}

bool intersects(const Document& document, const EntityRecord& record, const BBox& rectangle) {
    return std::visit(
        [&](const auto& value) -> bool {
            if constexpr (kAnswersByItself<std::decay_t<decltype(value)>>) {
                return intersects(record.entity, rectangle);
            } else {
                return intersectsOf(document, record, value, rectangle);
            }
        },
        record.entity);
}

void appendSnapPoints(const Document& document, const EntityRecord& record, const SnapModes& modes,
                      std::vector<SnapPoint>& out, const std::optional<Vec2>& from) {
    std::visit(
        [&](const auto& value) {
            if constexpr (kAnswersByItself<std::decay_t<decltype(value)>>) {
                appendSnapPoints(record.entity, modes, out, from);
            } else {
                for (const GeneratedItem& item : generate(document, record)) {
                    appendShapeSnapPoints(item.shape, modes, out, from);
                }
                appendOwnSnapPointsOf(document, record, value, modes, out, from);
            }
        },
        record.entity);
}

TransformOutcome transformedIn(const Document& document, const Entity& entity,
                               const Similarity& similarity, MirrorText mirrorText) {
    assert(isSupported(entity));
    return std::visit(
        [&](const auto& value) -> TransformOutcome {
            if constexpr (kAnswersByItself<std::decay_t<decltype(value)>>) {
                return {transformed(entity, similarity, mirrorText), {}};
            } else {
                return transformedOf(document, value, similarity, mirrorText);
            }
        },
        entity);
}

GripResult moveGripIn(const Document& document, const Entity& entity, std::size_t index,
                      const Vec2& target) {
    assert(isSupported(entity));
    return std::visit(
        [&](const auto& value) -> GripResult {
            if constexpr (kAnswersByItself<std::decay_t<decltype(value)>>) {
                return moveGrip(entity, index, target);
            } else {
                return moveGripOf(document, value, index, target);
            }
        },
        entity);
}

BBox shapeBoundingBox(const GeneratedShape& shape) {
    return std::visit(
        [](const auto& value) -> BBox {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, FilledRegion>) {
                return regionBoundingBox(value);
            } else if constexpr (kShapeOfItsOwn<T>) {
                return boundingBox(value);
            } else {
                return value.boundingBox();
            }
        },
        shape);
}

double shapeDistance(const GeneratedShape& shape, const Vec2& point) {
    if (const auto* region = std::get_if<FilledRegion>(&shape)) {
        return regionDistance(*region, point);
    }
    return curveDistance(shape, point);
}

bool shapeIsInside(const GeneratedShape& shape, const BBox& rectangle) {
    if (const auto* region = std::get_if<FilledRegion>(&shape)) {
        return regionInside(*region, rectangle);
    }
    return curveIsInside(shape, rectangle);
}

bool shapeIntersects(const GeneratedShape& shape, const BBox& rectangle) {
    if (const auto* region = std::get_if<FilledRegion>(&shape)) {
        return regionIntersects(*region, rectangle);
    }
    return curveIntersects(shape, rectangle);
}

void appendShapeSnapPoints(const GeneratedShape& shape, const SnapModes& modes,
                           std::vector<SnapPoint>& out, const std::optional<Vec2>& from) {
    std::visit(
        [&](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, FilledRegion>) {
                return;
            } else if constexpr (kShapeOfItsOwn<T>) {
                appendSnapPointsOf(value, modes, out, from);
            } else {
                appendSnapPoints(Entity{value}, modes, out, from);
            }
        },
        shape);
}

std::optional<Entity> shapeEntity(const GeneratedShape& shape) {
    return std::visit(
        [](const auto& value) -> std::optional<Entity> {
            if constexpr (std::is_same_v<std::decay_t<decltype(value)>, FilledRegion>) {
                return std::nullopt;
            } else {
                return Entity{value};
            }
        },
        shape);
}

BBox extentOfItems(std::span<const GeneratedItem> items) {
    BBox box;
    for (const GeneratedItem& item : items) {
        box.expand(shapeBoundingBox(item.shape));
    }
    return box;
}

double distanceToDrawnItems(std::span<const GeneratedItem> items, const Vec2& point) {
    double nearest = kInfinity;
    for (const GeneratedItem& item : items) {
        if (item.properties.visible) {
            nearest = std::min(nearest, shapeDistance(item.shape, point));
        }
    }
    return nearest;
}

bool drawnItemsInside(std::span<const GeneratedItem> items, const BBox& rectangle) {
    bool any = false;
    for (const GeneratedItem& item : items) {
        if (!item.properties.visible) {
            continue;
        }
        if (!shapeIsInside(item.shape, rectangle)) {
            return false;
        }
        any = true;
    }
    return any;
}

bool drawnItemsIntersect(std::span<const GeneratedItem> items, const BBox& rectangle) {
    return std::ranges::any_of(items, [&](const GeneratedItem& item) {
        return item.properties.visible && shapeIntersects(item.shape, rectangle);
    });
}

}  // namespace tg::core
