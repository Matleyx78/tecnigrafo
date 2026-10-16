// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <cmath>

namespace tg::core {

/// A point or vector in the 2D drawing plane, in drawing units.
///
/// The drawing plane uses a right-handed system: X grows to the right,
/// Y grows upwards (as in AutoCAD), angles are counter-clockwise from +X.
struct Vec2 {
    double x{0.0};
    double y{0.0};

    constexpr Vec2() = default;

    constexpr Vec2(double xValue, double yValue) : x(xValue), y(yValue) {}

    constexpr Vec2& operator+=(const Vec2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    constexpr Vec2& operator-=(const Vec2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    constexpr Vec2& operator*=(double factor) {
        x *= factor;
        y *= factor;
        return *this;
    }

    [[nodiscard]] double length() const { return std::hypot(x, y); }

    [[nodiscard]] constexpr double lengthSquared() const { return (x * x) + (y * y); }

    /// Vector rotated by +90 degrees (counter-clockwise).
    [[nodiscard]] constexpr Vec2 perpendicular() const { return {-y, x}; }

    friend constexpr bool operator==(const Vec2&, const Vec2&) = default;
};

[[nodiscard]] constexpr Vec2 operator+(Vec2 lhs, const Vec2& rhs) {
    lhs += rhs;
    return lhs;
}

[[nodiscard]] constexpr Vec2 operator-(Vec2 lhs, const Vec2& rhs) {
    lhs -= rhs;
    return lhs;
}

[[nodiscard]] constexpr Vec2 operator-(const Vec2& value) {
    return {-value.x, -value.y};
}

[[nodiscard]] constexpr Vec2 operator*(Vec2 lhs, double factor) {
    lhs *= factor;
    return lhs;
}

[[nodiscard]] constexpr Vec2 operator*(double factor, Vec2 rhs) {
    rhs *= factor;
    return rhs;
}

[[nodiscard]] constexpr double dot(const Vec2& a, const Vec2& b) {
    return (a.x * b.x) + (a.y * b.y);
}

/// Z component of the 3D cross product: > 0 when b is counter-clockwise from a.
[[nodiscard]] constexpr double cross(const Vec2& a, const Vec2& b) {
    return (a.x * b.y) - (a.y * b.x);
}

[[nodiscard]] inline double distance(const Vec2& a, const Vec2& b) {
    return (b - a).length();
}

[[nodiscard]] inline bool nearlyEqual(const Vec2& a, const Vec2& b, double tolerance) {
    return distance(a, b) <= tolerance;
}

}  // namespace tg::core
