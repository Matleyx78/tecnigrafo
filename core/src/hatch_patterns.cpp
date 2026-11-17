// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/hatch_patterns.hpp"

#include "tg/core/math.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace tg::core {
namespace {

/// A line family as a .pat file gives it: angle in degrees, origin and
/// offset in drawing units. The three patterns have no dashes: continuous
/// lines.
struct PatFamily {
    double angleDegrees{0.0};
    Vec2 origin;
    Vec2 offset;
};

// acad.pat (inches): the step between two lines is 1/8 inch.
constexpr double kInchStep = 0.125;
// acadiso.pat (millimetres): the same step, 3.175 mm.
constexpr double kMetricStep = 3.175;
constexpr double kAnsi31Degrees = 45.0;
constexpr double kNetSecondDegrees = 90.0;

constexpr std::array kAnsi31Inch{PatFamily{kAnsi31Degrees, {0.0, 0.0}, {0.0, kInchStep}}};
constexpr std::array kAnsi31Metric{PatFamily{kAnsi31Degrees, {0.0, 0.0}, {0.0, kMetricStep}}};
constexpr std::array kNetInch{PatFamily{0.0, {0.0, 0.0}, {0.0, kInchStep}},
                              PatFamily{kNetSecondDegrees, {0.0, 0.0}, {0.0, kInchStep}}};
constexpr std::array kNetMetric{PatFamily{0.0, {0.0, 0.0}, {0.0, kMetricStep}},
                                PatFamily{kNetSecondDegrees, {0.0, 0.0}, {0.0, kMetricStep}}};

/// ASCII comparison without case: pattern names are case-insensitive in
/// AutoCAD.
bool sameName(std::string_view a, std::string_view b) {
    const auto upper = [](char c) {
        return c >= 'a' && c <= 'z' ? static_cast<char>(c - 'a' + 'A') : c;
    };
    return std::ranges::equal(a, b, [&](char x, char y) { return upper(x) == upper(y); });
}

/// The families of a predefined pattern; none for SOLID and unknown names.
std::span<const PatFamily> familiesOf(std::string_view name, bool metric) {
    if (sameName(name, "ANSI31")) {
        return metric ? std::span<const PatFamily>{kAnsi31Metric}
                      : std::span<const PatFamily>{kAnsi31Inch};
    }
    if (sameName(name, "NET")) {
        return metric ? std::span<const PatFamily>{kNetMetric}
                      : std::span<const PatFamily>{kNetInch};
    }
    return {};
}

}  // namespace

std::vector<PatternLine> predefinedPatternLines(std::string_view name, double scale, double angle,
                                                int measurement) {
    const std::span<const PatFamily> families = familiesOf(name, measurement == kMetricMeasurement);
    const SinCos rotation = cardinalSinCos(angle);
    std::vector<PatternLine> lines;
    lines.reserve(families.size());
    for (const PatFamily& family : families) {
        const double lineAngle = degreesToRadians(family.angleDegrees) + angle;
        const SinCos d = cardinalSinCos(lineAngle);
        const Vec2 along{d.cosine, d.sine};
        const Vec2 normal{-d.sine, d.cosine};
        const Vec2 origin{(family.origin.x * rotation.cosine) - (family.origin.y * rotation.sine),
                          (family.origin.x * rotation.sine) + (family.origin.y * rotation.cosine)};
        lines.push_back(
            PatternLine{lineAngle,
                        origin * scale,
                        ((along * family.offset.x) + (normal * family.offset.y)) * scale,
                        {}});
    }
    return lines;
}

}  // namespace tg::core
