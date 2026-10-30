// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/core/transform2d.hpp"

#include <cstdint>
#include <optional>

namespace tg::core {

/// A plane similarity p' = s * R(theta) * F * p + t (ADR 0031, point 1).
class Similarity {
public:
    Similarity() = default;
    [[nodiscard]] static Similarity translation(const Vec2& offset);
    [[nodiscard]] static Similarity rotation(double radians, const Vec2& center);
    /// std::nullopt unless factor is finite and strictly positive.
    [[nodiscard]] static std::optional<Similarity> scaling(double factor, const Vec2& center);
    /// Reflection across a,b; std::nullopt for coincident or nonfinite points.
    [[nodiscard]] static std::optional<Similarity> reflection(const Vec2& a, const Vec2& b);

    [[nodiscard]] Vec2 apply(const Vec2& p) const;
    [[nodiscard]] double scale() const;
    [[nodiscard]] double rotationAngle() const;
    [[nodiscard]] bool reflects() const;
    [[nodiscard]] Transform2D matrix() const;

    /// (a * b).apply(p) == a.apply(b.apply(p)).
    friend Similarity operator*(const Similarity& a, const Similarity& b);

private:
    double m_scale{1.0};
    double m_angle{0.0};
    bool m_reflects{false};
    Vec2 m_offset;
};

/// MIRRTEXT 0 and 1 (ADR 0031, point 3).
enum class MirrorText : std::uint8_t { Readable, Mirrored };

/// Transform geometry without changing any drawing record or its metadata.
[[nodiscard]] Entity transformed(const Entity& entity, const Similarity& similarity,
                                 MirrorText mirrorText = MirrorText::Readable);

}  // namespace tg::core
