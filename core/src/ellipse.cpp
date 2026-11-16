// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/ellipse.hpp"

#include "tg/core/arc.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/document.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/grips.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/snap_points.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

namespace tg::core {
namespace {

constexpr double kQuarterTurn = kPi / 2.0;

/// The centre and the four ends of the axes (gripPointsOf()); unused in
/// Release, where assertions are absent.
[[maybe_unused]] constexpr std::size_t kGripCount = 5;

/// The minor axis b: the major one turned by +90° and multiplied by the
/// ratio (docs/adr/0052, point 1).
Vec2 minorAxis(const Ellipse& ellipse) {
    return ellipse.majorAxis.perpendicular() * ellipse.ratio;
}

bool isWhole(const Ellipse& ellipse) {
    return parameterSpan(ellipse) == kTwoPi;
}

/// True when the parameter `t` lies on the curve, its end points included.
bool containsParameter(const Ellipse& ellipse, double t) {
    return normalizeAngle(t - ellipse.startParameter) <= parameterSpan(ellipse);
}

/// True when the angle of `vector` is in [0°, 180°).
bool pointsUpward(const Vec2& vector) {
    return vector.y > 0.0 || (vector.y == 0.0 && vector.x > 0.0);
}

/// `vector` without negative zeros, so that the report never writes -0.
Vec2 withoutNegativeZero(const Vec2& vector) {
    return {vector.x + 0.0, vector.y + 0.0};
}

/// True when the linear part of `m` is exactly a rotation and a uniform
/// scale, possibly with a reflection: the ratio does not change.
bool isSimilarity(const Transform2D& m) {
    const Vec2 xAxis = m.applyToVector({1.0, 0.0});
    const Vec2 yAxis = m.applyToVector({0.0, 1.0});
    return (yAxis.x == -xAxis.y && yAxis.y == xAxis.x) ||
           (yAxis.x == xAxis.y && yAxis.y == -xAxis.x);
}

/// Sets the parameters of an arc of span `span` from `start` to `end`,
/// both brought into [0, 2π). When a rounding makes them equal, the end
/// moves by one unit in the last place towards the side of the span, as
/// transformed() does for an Arc: an arc never becomes a full ellipse.
void setArcParameters(Ellipse& ellipse, double start, double end, double span) {
    ellipse.startParameter = normalizeAngle(start);
    ellipse.endParameter = normalizeAngle(end);
    if (ellipse.startParameter != ellipse.endParameter) {
        return;
    }
    const double from = ellipse.startParameter;
    if (span > kPi) {
        ellipse.endParameter = std::nextafter(from > 0.0 ? from : kTwoPi, 0.0);
    } else {
        ellipse.endParameter = std::nextafter(from, kTwoPi);
        if (ellipse.endParameter >= kTwoPi) {
            ellipse.endParameter = 0.0;
        }
    }
}

/// The ellipse with major axis `axis` (non-zero) and `ratio` in (0, 1], in
/// the direction of docs/tappe/0.8/PT48-ellisse.md, point 3 only for a
/// circle (`anyDirection` false keeps the direction of a non-circle), and
/// the parameters of `source` (in the frame of `axis`) shifted by `shift`,
/// or reversed around `shift` when `reverses`: t' = shift - t.
Ellipse withAxes(const Ellipse& source, const Vec2& center, const Vec2& axis, double ratio,
                 double shift, bool reverses, bool anyDirection) {
    Ellipse result{center, withoutNegativeZero(axis), ratio, 0.0, kTwoPi};
    double offset = shift;
    if (ratio == 1.0) {
        // A circle: the major axis along +X, parameters turned with it.
        offset += atan2(axis.y, axis.x);
        result.majorAxis = Vec2{axis.length(), 0.0};
    } else if (anyDirection && !pointsUpward(axis)) {
        // The angle of the major axis in [0°, 180°): the opposite end.
        result.majorAxis = withoutNegativeZero(Vec2{-axis.x, -axis.y});
        offset -= kPi;
    }
    if (isWhole(source)) {
        return result;
    }
    const double start = reverses ? offset - source.endParameter : offset + source.startParameter;
    const double end = reverses ? offset - source.startParameter : offset + source.endParameter;
    setArcParameters(result, start, end, parameterSpan(source));
    return result;
}

}  // namespace

double parameterSpan(const Ellipse& ellipse) {
    double span = std::fmod(ellipse.endParameter - ellipse.startParameter, kTwoPi);
    if (!(span > 0.0)) {
        span += kTwoPi;
    }
    return span;
}

Vec2 pointAt(const Ellipse& ellipse, double t) {
    const SinCos direction = sinCos(t);
    return ellipse.center + (ellipse.majorAxis * direction.cosine) +
           (minorAxis(ellipse) * direction.sine);
}

Ellipse transformed(const Ellipse& e, const Transform2D& m) {
    assert(supported(e));
    const Vec2 center = m.apply(e.center);
    // The conjugate half diameters of the image: the point of parameter t
    // is center + cos t · p + sin t · q.
    const Vec2 p = m.applyToVector(e.majorAxis);
    const Vec2 q = m.applyToVector(minorAxis(e));
    const bool reflects = m.determinant() < 0.0;
    Vec2 axis = p;
    double ratio = e.ratio;
    double theta = 0.0;  // the parameter of the old frame on the new major axis
    if (!isSimilarity(m)) {
        // |cos t · p + sin t · q|² is largest at 2θ = atan2(2 p·q, |p|² - |q|²):
        // the first right singular vector of [p q], the left one being the
        // major axis.
        theta = 0.5 * atan2(2.0 * dot(p, q), p.lengthSquared() - q.lengthSquared());
        const SinCos direction = cardinalSinCos(theta);
        axis = (p * direction.cosine) + (q * direction.sine);
        const Vec2 other = (q * direction.cosine) - (p * direction.sine);
        if (other.lengthSquared() > axis.lengthSquared()) {
            // Rounding near a circle: the other axis is the major one.
            axis = other;
            theta += kQuarterTurn;
        }
        const double squared = axis.lengthSquared();
        // The product of the half axes is |p × q|, the area is preserved up to
        // the determinant: the minor half axis is |p × q| / |axis|.
        ratio = squared > 0.0 ? std::min(std::abs(cross(p, q)) / squared, 1.0) : 0.0;
    }
    if (axis == Vec2{} || !(ratio > 0.0)) {
        return Ellipse{center, Vec2{}, 0.0, e.startParameter, e.endParameter};
    }
    // Without a reflection the point of old parameter t has t - θ on the new
    // axes; with one the minor axis turns the other way and it has θ - t.
    return reflects ? withAxes(e, center, axis, ratio, theta, true, true)
                    : withAxes(e, center, axis, ratio, -theta, false, true);
}

Ellipse asEllipse(const Circle& circle) {
    return Ellipse{circle.center, Vec2{circle.radius, 0.0}, 1.0, 0.0, kTwoPi};
}

Ellipse asEllipse(const Arc& arc) {
    return Ellipse{arc.center, Vec2{arc.radius, 0.0}, 1.0, arc.startAngle, arc.endAngle};
}

BBox boundingBox(const Ellipse& ellipse) {
    assert(supported(ellipse));
    const Vec2& a = ellipse.majorAxis;
    const Vec2 b = minorAxis(ellipse);
    const Vec2& center = ellipse.center;
    // x(t) = cx + ax cos t + bx sin t reaches cx ± |(ax, bx)| at
    // t = atan2(±bx, ±ax); the same for y.
    const double halfWidth = hypot(a.x, b.x);
    const double halfHeight = hypot(a.y, b.y);
    if (isWhole(ellipse)) {
        return BBox{center - Vec2{halfWidth, halfHeight}, center + Vec2{halfWidth, halfHeight}};
    }
    const BBox box{pointAt(ellipse, ellipse.startParameter),
                   pointAt(ellipse, ellipse.endParameter)};
    Vec2 low = box.min();
    Vec2 high = box.max();
    if (containsParameter(ellipse, atan2(b.x, a.x))) {
        high.x = std::max(high.x, center.x + halfWidth);
    }
    if (containsParameter(ellipse, atan2(-b.x, -a.x))) {
        low.x = std::min(low.x, center.x - halfWidth);
    }
    if (containsParameter(ellipse, atan2(b.y, a.y))) {
        high.y = std::max(high.y, center.y + halfHeight);
    }
    if (containsParameter(ellipse, atan2(-b.y, -a.y))) {
        low.y = std::min(low.y, center.y - halfHeight);
    }
    return BBox{low, high};
}

double distanceOf([[maybe_unused]] const Ellipse& ellipse, const Vec2& /*point*/) {
    assert(supported(ellipse));
    // The distance is a question for solid-space (docs/adr/0052, point 4).
    return std::numeric_limits<double>::infinity();
}

bool isInsideOf(const Ellipse& ellipse, const BBox& rectangle) {
    assert(supported(ellipse));
    if (rectangle.isEmpty()) {
        return false;
    }
    const BBox box = boundingBox(ellipse);
    return rectangle.contains(box.min()) && rectangle.contains(box.max());
}

bool intersectsOf([[maybe_unused]] const Ellipse& ellipse, const BBox& /*rectangle*/) {
    assert(supported(ellipse));
    // The crossing is a question for solid-space (docs/adr/0052, point 4).
    return false;
}

void appendSnapPointsOf(const Ellipse& ellipse, const SnapModes& modes, std::vector<SnapPoint>& out,
                        const std::optional<Vec2>& /*from*/) {
    assert(supported(ellipse));
    const bool whole = isWhole(ellipse);
    if (modes.endpoint && !whole) {
        out.push_back({pointAt(ellipse, ellipse.startParameter), SnapKind::Endpoint});
        out.push_back({pointAt(ellipse, ellipse.endParameter), SnapKind::Endpoint});
    }
    if (modes.center) {
        out.push_back({ellipse.center, SnapKind::Center});
    }
    if (modes.quadrant) {
        const Vec2 b = minorAxis(ellipse);
        const std::array ends{ellipse.majorAxis, b,
                              Vec2{-ellipse.majorAxis.x, -ellipse.majorAxis.y}, Vec2{-b.x, -b.y}};
        for (std::size_t i = 0; i < ends.size(); ++i) {
            if (whole || containsParameter(ellipse, static_cast<double>(i) * kQuarterTurn)) {
                out.push_back({ellipse.center + ends.at(i), SnapKind::Quadrant});
            }
        }
    }
}

bool supported(const Ellipse& ellipse) {
    return ellipse.majorAxis != Vec2{} && ellipse.ratio > 0.0 && ellipse.ratio <= 1.0 &&
           std::isfinite(ellipse.startParameter) && std::isfinite(ellipse.endParameter);
}

std::vector<GeneratedItem> generateOf(const Document& document, const EntityRecord& record,
                                      const Ellipse& ellipse) {
    assert(supported(ellipse));
    const Layer* layer = document.layer(record.properties.layer);
    if (layer != nullptr && layer->frozen) {
        return {};
    }
    return {GeneratedItem{ellipse, effectiveProperties(document, record)}};
}

BBox extentOf(const Document& document, const EntityRecord& record, const Ellipse& ellipse) {
    assert(supported(ellipse));
    return extentOfItems(generateOf(document, record, ellipse));
}

double distanceOf(const Document& document, const EntityRecord& record, const Ellipse& ellipse,
                  const Vec2& point) {
    assert(supported(ellipse));
    return distanceToDrawnItems(generateOf(document, record, ellipse), point);
}

bool isInsideOf(const Document& document, const EntityRecord& record, const Ellipse& ellipse,
                const BBox& rectangle) {
    assert(supported(ellipse));
    return drawnItemsInside(generateOf(document, record, ellipse), rectangle);
}

bool intersectsOf(const Document& document, const EntityRecord& record, const Ellipse& ellipse,
                  const BBox& rectangle) {
    assert(supported(ellipse));
    return drawnItemsIntersect(generateOf(document, record, ellipse), rectangle);
}

void appendOwnSnapPointsOf(const Document& /*document*/, const EntityRecord& /*record*/,
                           [[maybe_unused]] const Ellipse& ellipse, const SnapModes& /*modes*/,
                           std::vector<SnapPoint>& /*out*/, const std::optional<Vec2>& /*from*/) {
    assert(supported(ellipse));
}

TransformOutcome transformedOf(const Document& /*document*/, const Ellipse& ellipse,
                               const Similarity& similarity, MirrorText /*mirrorText*/) {
    assert(supported(ellipse));
    return {Entity{transformed(ellipse, similarity.matrix())}, {}};
}

std::vector<Vec2> gripPointsOf(const Ellipse& ellipse) {
    assert(supported(ellipse));
    const Vec2& a = ellipse.majorAxis;
    const Vec2 b = minorAxis(ellipse);
    const Vec2& center = ellipse.center;
    return {center, center + a, center + b, center - a, center - b};
}

GripResult moveGripOf(const Document& /*document*/, const Ellipse& ellipse, std::size_t index,
                      const Vec2& target) {
    assert(supported(ellipse));
    assert(index < kGripCount);
    GripResult invalid{GripOutcome::Invalid, Entity{ellipse}, {}};
    if (!std::isfinite(target.x) || !std::isfinite(target.y)) {
        return invalid;
    }
    if (index == 0) {
        Ellipse moved = ellipse;
        moved.center = target;
        return {GripOutcome::Moved, Entity{moved}, {}};
    }
    const double length = distance(ellipse.center, target);
    if (!(length > 0.0) || !std::isfinite(length)) {
        return invalid;  // a half axis of length 0 (docs/tappe/0.8, PT48, point 5)
    }
    const Vec2& a = ellipse.majorAxis;
    const double major = a.length();
    const double minor = major * ellipse.ratio;
    // The half axes along a and along b after the move.
    const bool majorGrip = index == 1 || index == 3;
    const double alongA = majorGrip ? length : major;
    const double alongB = majorGrip ? minor : length;
    if (alongA >= alongB) {
        const Ellipse moved = withAxes(ellipse, ellipse.center, a * (alongA / major),
                                       alongB / alongA, 0.0, false, false);
        return {GripOutcome::Moved, Entity{moved}, {}};
    }
    // The minor axis is now the longer one: the axes swap, and the point of
    // parameter t has t - 90° on the new axes.
    const Ellipse moved = withAxes(ellipse, ellipse.center, a.perpendicular() * (alongB / major),
                                   alongA / alongB, -kQuarterTurn, false, false);
    return {GripOutcome::Moved, Entity{moved}, {}};
}

}  // namespace tg::core
