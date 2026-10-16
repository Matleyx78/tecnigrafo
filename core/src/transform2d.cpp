// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/transform2d.hpp"

#include <cmath>

namespace tg::core {

Transform2D Transform2D::rotation(double radians) {
    const double cosA = std::cos(radians);
    const double sinA = std::sin(radians);
    return {cosA, sinA, -sinA, cosA, 0.0, 0.0};
}

std::optional<Transform2D> Transform2D::inverted() const {
    const double det = determinant();
    if (det == 0.0 || !std::isfinite(det)) {
        return std::nullopt;
    }
    const double inv = 1.0 / det;
    const double a = m_d * inv;
    const double b = -m_b * inv;
    const double c = -m_c * inv;
    const double d = m_a * inv;
    const double e = -((a * m_e) + (c * m_f));
    const double f = -((b * m_e) + (d * m_f));
    return Transform2D{a, b, c, d, e, f};
}

}  // namespace tg::core
