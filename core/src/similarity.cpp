// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/similarity.hpp"

#include "tg/core/math.hpp"
#include "tg/core/text_metrics.hpp"

#include <array>
#include <cmath>
#include <type_traits>
#include <utility>
#include <vector>

namespace tg::core {
namespace {

Vec2 boxCenter(const std::array<Vec2, 4>& box) {
    Vec2 center;
    for (const Vec2& corner : box) {
        center += corner * 0.25;
    }
    return center;
}

Text transformedText(const Text& text, const Similarity& similarity, MirrorText mirrorText) {
    Text result = text;
    result.position = similarity.apply(text.position);
    result.alignmentPoint = similarity.apply(text.alignmentPoint);
    result.height *= similarity.scale();
    if (!similarity.reflects()) {
        result.rotation = normalizeAngle(text.rotation + similarity.rotationAngle());
        return result;
    }

    const double alpha = normalizeAngle(similarity.rotationAngle() - textFrame(text).rotation);
    if (mirrorText == MirrorText::Mirrored) {
        result.rotation = normalizeAngle(alpha + kPi);
        result.mirroredX = !text.mirroredX;
    } else {
        const bool reverse = alpha > kPi / 2.0 && alpha <= 3.0 * kPi / 2.0;
        result.rotation = normalizeAngle(reverse ? alpha - kPi : alpha);
        result.obliqueAngle = -text.obliqueAngle;
    }

    const bool baseline =
        text.horizontal == TextHorizontal::Aligned || text.horizontal == TextHorizontal::Fit;
    const double length = distance(text.position, text.alignmentPoint);
    const bool coincident = !(length > 0.0);
    const bool fitted = baseline && !coincident && textWidth(displayText(text.value)) > 0.0;
    if (fitted) {
        result.alignmentPoint =
            result.position + Vec2{std::cos(result.rotation), std::sin(result.rotation)} *
                                  (similarity.scale() * length);
    }

    // The center determines placement for every justification, without a
    // glyph-dependent measurement or a new geometric tolerance (ADR 0031).
    const Vec2 shift = similarity.apply(boxCenter(textBox(text))) - boxCenter(textBox(result));
    if (baseline) {
        result.position += shift;
        if (fitted || coincident) {
            result.alignmentPoint += shift;
        }
    } else if (text.horizontal == TextHorizontal::Left && text.vertical == TextVertical::Baseline) {
        result.position += shift;
    } else {
        result.alignmentPoint += shift;
    }
    return result;
}

}  // namespace

Similarity Similarity::translation(const Vec2& offset) {
    Similarity result;
    result.m_offset = offset;
    return result;
}

Similarity Similarity::rotation(double radians, const Vec2& center) {
    Similarity result;
    result.m_angle = normalizeAngle(radians);
    result.m_offset = center - result.apply(center);
    return result;
}

std::optional<Similarity> Similarity::scaling(double factor, const Vec2& center) {
    if (!std::isfinite(factor) || !(factor > 0.0)) {
        return std::nullopt;
    }
    Similarity result;
    result.m_scale = factor;
    result.m_offset = center - center * factor;
    return result;
}

std::optional<Similarity> Similarity::reflection(const Vec2& a, const Vec2& b) {
    const Vec2 delta = b - a;
    const bool finite = std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(b.x) &&
                        std::isfinite(b.y);
    if (!finite || !(delta.length() > 0.0)) {
        return std::nullopt;
    }
    Similarity result;
    result.m_angle = normalizeAngle(2.0 * std::atan2(delta.y, delta.x));
    result.m_reflects = true;
    result.m_offset = a - result.apply(a);
    return result;
}

Vec2 Similarity::apply(const Vec2& p) const {
    return matrix().apply(p);
}

double Similarity::scale() const {
    return m_scale;
}

double Similarity::rotationAngle() const {
    return m_angle;
}

bool Similarity::reflects() const {
    return m_reflects;
}

Transform2D Similarity::matrix() const {
    const double cosine = m_scale * std::cos(m_angle);
    const double sine = m_scale * std::sin(m_angle);
    const double xFromY = m_reflects ? sine : -sine;
    const double yFromY = m_reflects ? -cosine : cosine;
    return {cosine, sine, xFromY, yFromY, m_offset.x, m_offset.y};
}

Similarity operator*(const Similarity& a, const Similarity& b) {
    Similarity result;
    result.m_scale = a.m_scale * b.m_scale;
    result.m_angle = normalizeAngle(a.m_angle + (a.m_reflects ? -b.m_angle : b.m_angle));
    result.m_reflects = a.m_reflects != b.m_reflects;
    result.m_offset = a.apply(b.m_offset);
    return result;
}

Entity transformed(const Entity& entity, const Similarity& similarity, MirrorText mirrorText) {
    return std::visit(
        [&](const auto& shape) -> Entity {
            using Shape = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<Shape, Text>) {
                return transformedText(shape, similarity, mirrorText);
            } else if constexpr (std::is_same_v<Shape, Polyline>) {
                std::vector<PolylineVertex> vertices;
                vertices.reserve(shape.vertexCount());
                for (const PolylineVertex& vertex : shape.vertices()) {
                    vertices.push_back({similarity.apply(vertex.position),
                                        similarity.reflects() ? -vertex.bulge : vertex.bulge,
                                        vertex.startWidth * similarity.scale(),
                                        vertex.endWidth * similarity.scale()});
                }
                Polyline result{std::move(vertices), shape.isClosed()};
                result.setConstantWidth(shape.constantWidth() * similarity.scale());
                result.setContinuousLinetype(shape.continuousLinetype());
                return result;
            } else if constexpr (std::is_same_v<Shape, Line>) {
                return Line{similarity.apply(shape.start), similarity.apply(shape.end)};
            } else {
                Shape result = shape;
                result.center = similarity.apply(shape.center);
                result.radius *= similarity.scale();
                if constexpr (std::is_same_v<Shape, Arc>) {
                    const double sign = similarity.reflects() ? -1.0 : 1.0;
                    result.startAngle =
                        normalizeAngle(similarity.rotationAngle() + sign * shape.startAngle);
                    result.endAngle =
                        normalizeAngle(similarity.rotationAngle() + sign * shape.endAngle);
                    if (similarity.reflects()) {
                        std::swap(result.startAngle, result.endAngle);
                    }
                    // A rotation can round two adjacent floating-point angles
                    // to one value. Keep an arc, never an accidental full circle.
                    if (!(std::abs(result.endAngle - result.startAngle) > 0.0)) {
                        if (shape.geometry().sweep > kPi) {
                            result.endAngle = std::nextafter(
                                result.startAngle > 0.0 ? result.startAngle : kTwoPi, 0.0);
                        } else {
                            result.endAngle = std::nextafter(result.startAngle, kTwoPi);
                            if (result.endAngle >= kTwoPi) {
                                result.endAngle = 0.0;
                            }
                        }
                    }
                }
                return result;
            }
        },
        entity);
}

}  // namespace tg::core
