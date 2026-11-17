// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/text_affine.hpp"

#include "tg/core/document.hpp"
#include "tg/core/math.hpp"
#include "tg/core/text_metrics.hpp"

#include <cassert>
#include <cmath>
#include <optional>
#include <variant>

namespace tg::core {

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
                        Similarity::rotation(atan2(xAxis.y, xAxis.x), Vec2{}) * *scaling;
    if (reflects) {
        const std::optional<Similarity> acrossX = Similarity::reflection(Vec2{}, Vec2{1.0, 0.0});
        assert(acrossX.has_value());
        if (acrossX) {
            result = result * *acrossX;
        }
    }
    return result;
}

Text transformedText(const Text& text, const Transform2D& transform) {
    if (const std::optional<Similarity> similarity = similarityOf(transform)) {
        return std::get<Text>(transformed(Entity{text}, *similarity, MirrorText::Mirrored));
    }
    assert(transform.determinant() != 0.0);
    // The local frame of the text (textFrame(), docs/adr/0022): a point
    // (u, v) of the characters, in heights, maps to
    // origin + R(r)·[[sx, sx·t], [0, sy]]·(u·h·f, v·h), with sx and sy the
    // signs of the flags and t = tan(oblique). A quarter turn is exact.
    const TextFrame frame = textFrame(text);
    const SinCos direction = cardinalSinCos(frame.rotation);
    const Vec2 along{direction.cosine, direction.sine};
    const double backward = text.mirroredX ? -1.0 : 1.0;
    const double upsideDown = text.mirroredY ? -1.0 : 1.0;
    const Vec2 xAxis = along * backward;
    const Vec2 yAxis = (xAxis * tan(text.obliqueAngle)) + (along.perpendicular() * upsideDown);
    // The two columns of the mapped frame, B·[[f·h, t·h], [0, h]] up to the
    // signs of the flags.
    const Vec2 baseline = transform.applyToVector(xAxis) * (frame.height * frame.widthFactor);
    const Vec2 upward = transform.applyToVector(yAxis) * frame.height;

    Text result = text;
    const bool reflects = transform.determinant() < 0.0;
    result.mirroredX = text.mirroredX != reflects;
    const double newBackward = result.mirroredX ? -1.0 : 1.0;
    // R(θ)·(a·h·f, 0) is the baseline, read along the new backward flag;
    // R(−θ) applied to the second column gives (b + a·t, c)·h, with signs.
    const double width = baseline.length();
    const Vec2 unit = baseline * (newBackward / width);
    const double height = upsideDown * cross(unit, upward);
    const double slant = newBackward * dot(unit, upward);
    result.rotation = normalizeAngle(atan2(unit.y, unit.x));
    result.height = height;
    result.widthFactor = width / height;
    result.obliqueAngle = atan2(slant, height);

    result.position = transform.apply(text.position);
    result.alignmentPoint = transform.apply(text.alignmentPoint);
    const bool twoPoints =
        text.horizontal == TextHorizontal::Aligned || text.horizontal == TextHorizontal::Fit;
    const bool fitted = twoPoints && distance(text.position, text.alignmentPoint) > 0.0 &&
                        textWidth(displayText(text.value)) > 0.0;
    if (fitted && reflects) {
        // The text runs from the first point along the direction of the
        // second, or away from it with the backward flag: toggling the flag
        // puts the second point on the other side (ADR 0031, point 3).
        result.alignmentPoint = result.position + (result.position - result.alignmentPoint);
    }
    return result;
}

}  // namespace tg::core
