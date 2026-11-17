// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/similarity.hpp"
#include "tg/core/text.hpp"
#include "tg/core/transform2d.hpp"

#include <optional>

namespace tg::core {

/// The similarity equal to `transform`, or std::nullopt when its linear part
/// is not exactly a rotation and a uniform scale, possibly with a reflection.
[[nodiscard]] std::optional<Similarity> similarityOf(const Transform2D& transform);

/// The exact image of `text` under the affine map `transform`
/// (docs/adr/0044, point 2; docs/tappe/0.8/PT17-attributi-core.md, point 2).
///
/// A similarity (similarityOf()) goes through transformed() with MIRRTEXT 1
/// (MirrorText::Mirrored). Any other map is decomposed on the base of the
/// text (PT4, P20): B = A·R(r), the linear part A composed with the rotation
/// r the text is drawn with (textFrame()), is R(θ)·[[a, b], [0, c]] with
/// c > 0 (QR); when det B < 0 the text is first reflected in its own X
/// (the backward flag toggled, as MIRRTEXT 1). Then the rotation is θ, the
/// height is multiplied by c, the width factor by a/c, the oblique angle has
/// tan(o') = (a·tan(o) + b)/c, and the points are mapped by `transform`.
/// The decomposition is that of the whole local frame of the text, its
/// flags included, so that every point of the characters maps exactly: with
/// the backward flag (set before or toggled now) b counts with the opposite
/// sign. Height, width factor and rotation are those the text is drawn with:
/// for Aligned and Fit they come from the two mapped points. A reflection
/// of an Aligned or Fit text that is not degenerate puts the second point
/// on the other side of the first, as transformed() does, because the
/// backward flag runs the text away from it (ADR 0031, point 3).
///
/// Precondition: the linear part of `transform` is not singular.
[[nodiscard]] Text transformedText(const Text& text, const Transform2D& transform);

}  // namespace tg::core
