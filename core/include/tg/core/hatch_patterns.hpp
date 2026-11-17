// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/hatch.hpp"

#include <string_view>
#include <vector>

/// The predefined patterns of new hatches (docs/adr/0048, point 2;
/// docs/tappe/0.8/PT30-retini-core.md, point 2): ANSI31, NET and SOLID, as
/// acadiso.pat defines them for a metric drawing and acad.pat for the others.
namespace tg::core {

/// DXF $MEASUREMENT of a metric drawing: acadiso.pat.
inline constexpr int kMetricMeasurement = 1;

/// The pattern lines of the predefined pattern `name` (ANSI31, NET or SOLID,
/// ASCII letters of any case) as AutoCAD writes them in a HATCH, for the
/// pattern scale `scale` and angle `angle` (radians, already reduced to one
/// turn by the caller with reducedDegreesToRadians) of a drawing with
/// $MEASUREMENT `measurement` (kMetricMeasurement: acadiso.pat; any other
/// value: acad.pat).
///
/// A family at angle a of the .pat file, with origin (x0, y0), offset
/// (dx, dy) and dashes, becomes a PatternLine with angle a + angle, origin
/// (x0, y0) rotated by `angle` and multiplied by `scale`, offset
/// scale * (dx * d + dy * n) with d = cardinalSinCos(a + angle) as (cosine,
/// sine) and n = (-d.sine, d.cosine), and dashes multiplied by `scale`. So
/// the step between two lines, offset . n, is scale * dy, and the shift
/// along a line, offset . d, is scale * dx. SOLID has no line, and neither
/// has a name that is not predefined.
[[nodiscard]] std::vector<PatternLine> predefinedPatternLines(std::string_view name, double scale,
                                                              double angle, int measurement);

}  // namespace tg::core
