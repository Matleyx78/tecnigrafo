// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/vec2.hpp"

#include <algorithm>
#include <limits>

namespace tg::core {

/// Axis-aligned bounding box. A default-constructed box is empty and
/// becomes valid as soon as a point is added.
class BBox {
public:
    constexpr BBox() = default;

    constexpr BBox(const Vec2& a, const Vec2& b)
        : m_min{std::min(a.x, b.x), std::min(a.y, b.y)},
          m_max{std::max(a.x, b.x), std::max(a.y, b.y)} {}

    [[nodiscard]] constexpr bool isEmpty() const { return m_min.x > m_max.x || m_min.y > m_max.y; }

    constexpr void expand(const Vec2& point) {
        m_min.x = std::min(m_min.x, point.x);
        m_min.y = std::min(m_min.y, point.y);
        m_max.x = std::max(m_max.x, point.x);
        m_max.y = std::max(m_max.y, point.y);
    }

    constexpr void expand(const BBox& other) {
        if (other.isEmpty()) {
            return;
        }
        expand(other.m_min);
        expand(other.m_max);
    }

    [[nodiscard]] constexpr const Vec2& min() const { return m_min; }

    [[nodiscard]] constexpr const Vec2& max() const { return m_max; }

    [[nodiscard]] constexpr double width() const { return isEmpty() ? 0.0 : m_max.x - m_min.x; }

    [[nodiscard]] constexpr double height() const { return isEmpty() ? 0.0 : m_max.y - m_min.y; }

    [[nodiscard]] constexpr Vec2 center() const { return (m_min + m_max) * 0.5; }

    [[nodiscard]] constexpr bool contains(const Vec2& point) const {
        return point.x >= m_min.x && point.x <= m_max.x && point.y >= m_min.y && point.y <= m_max.y;
    }

private:
    static constexpr double kInf = std::numeric_limits<double>::infinity();

    Vec2 m_min{kInf, kInf};
    Vec2 m_max{-kInf, -kInf};
};

}  // namespace tg::core
