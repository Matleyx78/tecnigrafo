// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The own properties of ELLIPSE and SOLID
// (docs/tappe/0.8.1/PT22-proprieta-core.md, points 2 to 4).

#include "tg/core/arc_geometry.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/entity_properties.hpp"
#include "tg/core/math.hpp"
#include "tg/core/solid.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

constexpr double kQuarterTurn = kPi / 2.0;

PropertyChange refused(std::string_view reason) {
    return PropertyChange{std::nullopt, std::nullopt, std::string{reason}};
}

PropertyChange invalid() {
    return refused(kInvalidPropertyValue);
}

PropertyChange changed(Entity entity) {
    return PropertyChange{std::move(entity), std::nullopt, {}};
}

/// The number held by `value`, if it is one and finite.
std::optional<double> finiteNumber(const PropertyValue& value) {
    const auto* number = std::get_if<double>(&value);
    if (number == nullptr || !std::isfinite(*number)) {
        return std::nullopt;
    }
    return *number;
}

/// The point held by `value`, if it is one and finite.
std::optional<Vec2> finitePoint(const PropertyValue& value) {
    const auto* point = std::get_if<Vec2>(&value);
    if (point == nullptr || !std::isfinite(point->x) || !std::isfinite(point->y)) {
        return std::nullopt;
    }
    return *point;
}

bool isFinite(const Vec2& vector) {
    return std::isfinite(vector.x) && std::isfinite(vector.y);
}

/// `vector` without negative zeros, so that the report never writes -0.
Vec2 withoutNegativeZero(const Vec2& vector) {
    return {vector.x + 0.0, vector.y + 0.0};
}

// ---------------------------------------------------------------------------
// ELLIPSE

bool isWhole(const Ellipse& ellipse) {
    return parameterSpan(ellipse) == kTwoPi;
}

/// The half axes a and b.
struct HalfAxes {
    double major{0.0};
    double minor{0.0};
};

HalfAxes halfAxes(const Ellipse& ellipse) {
    const double major = ellipse.majorAxis.length();
    return {major, major * ellipse.ratio};
}

/// The true angle of parameter `t`, atan2(b · sin t, a · cos t), in the turn
/// of `t`: both lie in the same quadrant, so the angle differs from `t` by
/// less than a quarter turn plus a whole number of turns.
double trueAngle(const HalfAxes& axes, double t) {
    const SinCos direction = sinCos(t);
    const double angle = atan2(axes.minor * direction.sine, axes.major * direction.cosine);
    return angle + (kTwoPi * std::round((t - angle) / kTwoPi));
}

/// The parameter of the true angle `angle`, atan2(a · sin θ, b · cos θ), in
/// [0, 2π). Quarter turns are exact (cardinalSinCos).
double parameterOf(const HalfAxes& axes, double angle) {
    const SinCos direction = cardinalSinCos(angle);
    return normalizeAngle(atan2(axes.major * direction.sine, axes.minor * direction.cosine));
}

/// The parameters of an arc moved by `shift` and brought into [0, 2π). When
/// a rounding makes them equal, the end moves by one unit in the last place
/// towards the side of the span, as moveGripOf() does: an arc never becomes
/// a whole ellipse.
void shiftArcParameters(Ellipse& ellipse, double shift) {
    const double span = parameterSpan(ellipse);
    ellipse.startParameter = normalizeAngle(ellipse.startParameter + shift);
    ellipse.endParameter = normalizeAngle(ellipse.endParameter + shift);
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

/// `result`, whose parameters are those of `source` moved by `shift`, with
/// the major axis along +X when the ratio is 1 exactly (as moveGripOf()).
/// A whole ellipse keeps its parameters.
Ellipse withAxisRule(const Ellipse& source, Ellipse result, double shift) {
    if (result.ratio == 1.0) {
        shift += atan2(result.majorAxis.y, result.majorAxis.x);
        result.majorAxis = Vec2{result.majorAxis.length(), 0.0};
    }
    result.majorAxis = withoutNegativeZero(result.majorAxis);
    if (shift != 0.0 && !isWhole(source)) {
        shiftArcParameters(result, shift);
    }
    return result;
}

/// The ellipse with half axes `alongA` along the major axis and `alongB`
/// along the minor one, both greater than 0: the axes swap when `alongB` is
/// the longer (docs/REGOLE-NUMERICHE.md, section 2, point 7).
Ellipse withHalfAxes(const Ellipse& ellipse, double alongA, double alongB) {
    const Vec2& axis = ellipse.majorAxis;
    const double major = axis.length();
    Ellipse result = ellipse;
    if (alongA >= alongB) {
        if (alongA != major) {
            result.majorAxis = axis * (alongA / major);
        }
        result.ratio = alongB / alongA;
        return withAxisRule(ellipse, result, 0.0);
    }
    // The minor axis is the longer one: the point of parameter t has t - 90°
    // on the new axes.
    result.majorAxis = axis.perpendicular() * (alongB / major);
    result.ratio = alongA / alongB;
    return withAxisRule(ellipse, result, -kQuarterTurn);
}

/// The ellipse with the true angle `angle` at its start (or end): only that
/// parameter changes.
Ellipse withAngle(const Ellipse& ellipse, double angle, bool start) {
    Ellipse result = ellipse;
    double& parameter = start ? result.startParameter : result.endParameter;
    const double t = parameterOf(halfAxes(ellipse), angle);
    if (normalizeAngle(parameter) != t) {
        parameter = t;
    }
    return result;
}

/// A changed ellipse that the model supports, with finite center and axis.
PropertyChange checked(const Ellipse& ellipse) {
    if (!isFinite(ellipse.center) || !isFinite(ellipse.majorAxis) || !supported(ellipse)) {
        return invalid();
    }
    return changed(ellipse);
}

// ---------------------------------------------------------------------------
// SOLID

constexpr std::array<std::string_view, 4> kVertexNames{"vertex1", "vertex2", "vertex3", "vertex4"};

}  // namespace

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/,
                                         const EntityRecord& /*record*/, const Ellipse& ellipse) {
    const HalfAxes axes = halfAxes(ellipse);
    const bool whole = isWhole(ellipse);
    const double start = trueAngle(axes, ellipse.startParameter);
    const double end = whole ? start + kTwoPi : trueAngle(axes, ellipse.endParameter);
    double area = kPi * axes.major * axes.minor;
    if (!whole) {
        // The segment closed by the chord (docs/DECISIONI.md, 2026-11-22,
        // "Esiti della seconda seduta in AutoCAD della 0.8.1", point 3).
        const double span = parameterSpan(ellipse);
        area = axes.major * axes.minor / 2.0 * (span - sin(span));
    }
    return {
        {"center", PropertyKind::Point, ellipse.center, {}, false},
        {"major-radius", PropertyKind::Distance, axes.major, {}, false},
        {"minor-radius", PropertyKind::Distance, axes.minor, {}, false},
        {"ratio", PropertyKind::Number, ellipse.ratio, {}, false},
        {"start-angle", PropertyKind::Angle, start, {}, false},
        {"end-angle", PropertyKind::Angle, end, {}, false},
        {"~area", PropertyKind::Number, area, {}, true},
    };
}

PropertyChange withOwnPropertyOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                 const Ellipse& ellipse, std::string_view name,
                                 const PropertyValue& value) {
    if (name == "center") {
        const std::optional<Vec2> center = finitePoint(value);
        if (!center) {
            return invalid();
        }
        Ellipse moved = ellipse;
        moved.center = *center;
        return checked(moved);
    }
    const std::optional<double> number = finiteNumber(value);
    if (!number) {
        return invalid();
    }
    const HalfAxes axes = halfAxes(ellipse);
    if (name == "major-radius" || name == "minor-radius") {
        if (!(*number > 0.0)) {
            return invalid();
        }
        const bool major = name == "major-radius";
        return checked(
            withHalfAxes(ellipse, major ? *number : axes.major, major ? axes.minor : *number));
    }
    if (name == "ratio") {
        if (!(*number > 0.0) || *number > 1.0) {
            return invalid();
        }
        Ellipse result = ellipse;
        result.ratio = *number;
        return checked(withAxisRule(ellipse, result, 0.0));
    }
    // start-angle or end-angle: the only names left.
    return checked(withAngle(ellipse, *number, name == "start-angle"));
}

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/,
                                         const EntityRecord& /*record*/, const Solid& solid) {
    std::vector<OwnProperty> properties;
    properties.reserve(kVertexNames.size());
    for (std::size_t i = 0; i < kVertexNames.size(); ++i) {
        properties.push_back({std::string{kVertexNames.at(i)},
                              PropertyKind::Point,
                              solid.vertices.at(i),
                              {},
                              false});
    }
    return properties;
}

PropertyChange withOwnPropertyOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                 const Solid& solid, std::string_view name,
                                 const PropertyValue& value) {
    const std::optional<Vec2> point = finitePoint(value);
    if (!point) {
        return invalid();
    }
    Solid result = solid;
    for (std::size_t i = 0; i < kVertexNames.size(); ++i) {
        if (kVertexNames.at(i) == name) {
            result.vertices.at(i) = *point;
        }
    }
    constexpr std::size_t kThird = 2;
    constexpr std::size_t kFourth = 3;
    result.fourthVertexPresent =
        solid.fourthVertexPresent || result.vertices.at(kFourth) != result.vertices.at(kThird);
    return changed(result);
}

}  // namespace tg::core
