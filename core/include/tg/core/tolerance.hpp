// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/vec2.hpp"

namespace tg::core {

/// The length in metres of the drawing unit INSUNITS `units` (docs/adr/0036,
/// point 1): the double nearest the exact value; 0, no unit, as millimetres.
/// Precondition: 0 <= units <= Document::kMaxUnits.
[[nodiscard]] double unitInMeters(int units);

/// solid-space's rule for coincidence and tolerance in the plane (its ADR
/// 0009), for the unit of a drawing (docs/adr/0036, point 2).
class ToleranceRule {
public:
    /// The rule of a drawing with INSUNITS `units`.
    /// Precondition: 0 <= units <= Document::kMaxUnits.
    explicit ToleranceRule(int units);
    /// The rule of `document`, for its units now.
    [[nodiscard]] static ToleranceRule of(const Document& document);
    [[nodiscard]] int units() const;
    /// epsilon(magnitude), in drawing units.
    /// Precondition: magnitude is finite, in [0, 2^50] (solid-space's domain).
    [[nodiscard]] double epsilon(double magnitude) const;
    /// Whether two points coincide under the rule (exact comparison).
    /// Precondition: every coordinate is finite, of magnitude at most 2^50.
    [[nodiscard]] bool coincident(const Vec2& a, const Vec2& b) const;

private:
    int m_units;
};

/// Whether the curve of a line, arc or circle is degenerate under the rule
/// (geom2d isDegenerate: every two of its points within epsilon). A polyline
/// and a text: false. Data that solid-space refuses (beyond its domain, 2^50)
/// are not judged degenerate, so that nothing is dropped on their account.
[[nodiscard]] bool isDegenerate(const Entity& entity, const ToleranceRule& rule);
/// The same for one polyline segment.
[[nodiscard]] bool isDegenerate(const PolylineSegment& segment, const ToleranceRule& rule);

}  // namespace tg::core
