// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/vec2.hpp"

#include <optional>

namespace tg::core {

/// 2D affine transform:
///
///   x' = a*x + c*y + e
///   y' = b*x + d*y + f
///
/// Composition follows the mathematical convention: (A * B).apply(p) is
/// A.apply(B.apply(p)), i.e. B is applied first.
class Transform2D {
public:
    constexpr Transform2D() = default;

    constexpr Transform2D(double a, double b, double c, double d, double e, double f)
        : m_a(a), m_b(b), m_c(c), m_d(d), m_e(e), m_f(f) {}

    [[nodiscard]] static constexpr Transform2D identity() { return {}; }

    [[nodiscard]] static constexpr Transform2D translation(const Vec2& offset) {
        return {1.0, 0.0, 0.0, 1.0, offset.x, offset.y};
    }

    [[nodiscard]] static constexpr Transform2D scaling(double sx, double sy) {
        return {sx, 0.0, 0.0, sy, 0.0, 0.0};
    }

    [[nodiscard]] static Transform2D rotation(double radians);

    [[nodiscard]] constexpr Vec2 apply(const Vec2& p) const {
        return {(m_a * p.x) + (m_c * p.y) + m_e, (m_b * p.x) + (m_d * p.y) + m_f};
    }

    /// Applies only the linear part (no translation): for direction vectors.
    [[nodiscard]] constexpr Vec2 applyToVector(const Vec2& v) const {
        return {(m_a * v.x) + (m_c * v.y), (m_b * v.x) + (m_d * v.y)};
    }

    [[nodiscard]] constexpr double determinant() const { return (m_a * m_d) - (m_b * m_c); }

    /// Inverse transform, or std::nullopt when the transform is singular.
    [[nodiscard]] std::optional<Transform2D> inverted() const;

    friend constexpr Transform2D operator*(const Transform2D& lhs, const Transform2D& rhs) {
        return {(lhs.m_a * rhs.m_a) + (lhs.m_c * rhs.m_b),
                (lhs.m_b * rhs.m_a) + (lhs.m_d * rhs.m_b),
                (lhs.m_a * rhs.m_c) + (lhs.m_c * rhs.m_d),
                (lhs.m_b * rhs.m_c) + (lhs.m_d * rhs.m_d),
                (lhs.m_a * rhs.m_e) + (lhs.m_c * rhs.m_f) + lhs.m_e,
                (lhs.m_b * rhs.m_e) + (lhs.m_d * rhs.m_f) + lhs.m_f};
    }

private:
    double m_a{1.0};
    double m_b{0.0};
    double m_c{0.0};
    double m_d{1.0};
    double m_e{0.0};
    double m_f{0.0};
};

}  // namespace tg::core
