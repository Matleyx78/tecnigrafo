// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/transform2d.hpp"
#include "tg/core/vec2.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace tg::core {

class Document;
class Similarity;
struct Arc;
struct Circle;
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

/// The sweep of the parameter from start to end, in (0, 2π]: an end equal to
/// the start modulo 2π (0 and 2π in particular) is the full ellipse, any
/// other pair an arc from start to end counter-clockwise.
/// Precondition: finite parameters.
[[nodiscard]] double parameterSpan(const Ellipse& ellipse);

/// The point of parameter `t`: centre + cos t · a + sin t · b, with a the
/// major axis and b the minor one, a turned by +90° and multiplied by the
/// ratio (docs/adr/0052, point 1), with core::sinCos. Precondition: finite t.
[[nodiscard]] Vec2 pointAt(const Ellipse& ellipse, double t);

/// The ellipse (or elliptical arc) that the affine map `m` makes of `e`,
/// exactly up to rounding: new axes from the singular value decomposition
/// of m·[a b], start and end parameters shifted to the new major axis; a
/// reflection reverses the direction of the parameters (start and end swap).
///
/// The axes of the result (docs/tappe/0.8/PT48-ellisse.md, point 3): with
/// ratio 1 exactly, a circle, the major axis lies along +X; otherwise its
/// angle is in [0°, 180°). A ratio that rounding puts above 1 swaps the axes.
/// The parameters of an arc are in [0, 2π) and never equal (a rounding that
/// would make them equal moves the end by one unit in the last place); a full
/// ellipse stays 0 and 2π. A singular `m` gives an ellipse that is not
/// supported() (ratio 0, or no major axis).
/// Precondition: supported(e).
[[nodiscard]] Ellipse transformed(const Ellipse& e, const Transform2D& m);

/// A circle as an ellipse with ratio 1: major axis (radius, 0), 0 and 2π.
[[nodiscard]] Ellipse asEllipse(const Circle& circle);

/// An arc as an ellipse with ratio 1: major axis (radius, 0), start and end
/// parameters equal to the start and end angles of the arc.
[[nodiscard]] Ellipse asEllipse(const Arc& arc);

/// The bounding box, exact: the end points of an arc and, where they lie on
/// it, the points where X or Y is extreme. It does not depend on the document.
[[nodiscard]] BBox boundingBox(const Ellipse& ellipse);

// Ellipse is also a shape of the generated geometry (generated.hpp): the
// questions on a shape, which do not depend on the document.

/// Distance from `point`, as distance(const Entity&, const Vec2&): until the
/// ellipse of solid-space's geom2d (docs/adr/0052, point 4; docs/tappe/0.8,
/// PT50) always infinity, so a click never picks an ellipse.
[[nodiscard]] double distanceOf(const Ellipse& ellipse, const Vec2& point);

/// Window selection, as isInside(const Entity&, const BBox&): true when the
/// bounding box lies in `rectangle`, borders included.
[[nodiscard]] bool isInsideOf(const Ellipse& ellipse, const BBox& rectangle);

/// Crossing selection, as intersects(const Entity&, const BBox&): until PT50
/// always false (docs/adr/0052, point 4).
[[nodiscard]] bool intersectsOf(const Ellipse& ellipse, const BBox& rectangle);

/// Appends the enabled snap points, as appendSnapPoints(const Entity&, …),
/// in this order: Endpoint at the start and at the end of an arc (none for a
/// full ellipse); Center; Quadrant at the ends of the axes that lie on the
/// curve, end points included, by parameter 0, 90°, 180°, 270°. Intersection
/// comes with PT50, Nearest after the 0.8; `from` is not used.
void appendSnapPointsOf(const Ellipse& ellipse, const SnapModes& modes, std::vector<SnapPoint>& out,
                        const std::optional<Vec2>& from);

// The functions of Ellipse that the common visits call, one line per type
// (docs/adr/0044, point 1; docs/tappe/0.8/PT6-scheletro-core.md, point 4).
// Each one but supported() has the precondition supported(ellipse), which is
// checked.

/// True when the major axis is not (0, 0), the ratio is in (0, 1] and the
/// parameters are finite (docs/tappe/0.8/PT48-ellisse.md, point 1).
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

/// The grips, in order: the centre, then the ends of the axes centre + a,
/// centre + b, centre - a, centre - b (a the major axis, b the minor one),
/// for a full ellipse and for an arc.
[[nodiscard]] std::vector<Vec2> gripPointsOf(const Ellipse& ellipse);

/// Moves grip `index` to `target` (docs/tappe/0.8/PT48-ellisse.md, point 5).
/// The centre moves the ellipse. The grip of an axis end sets the length of
/// that half axis to the distance from the centre to `target` and keeps the
/// direction of both axes; when the minor one becomes longer than the major
/// one, the axes swap (the new major axis keeps the direction of the old
/// minor one, the parameters are shifted by -90°), and with ratio 1 exactly
/// the major axis goes along +X as in transformed(). An arc keeps its end
/// points at the same parametric angles. A half axis of length 0 (or a target
/// that is not finite) is Invalid, without a refusal.
/// Precondition: index < gripPointsOf().size().
[[nodiscard]] GripResult moveGripOf(const Document& document, const Ellipse& ellipse,
                                    std::size_t index, const Vec2& target);

}  // namespace tg::core
