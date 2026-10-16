// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/vec2.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace tg::cmd {

/// A point typed on the command line, as read, not yet turned into a point
/// of the drawing (docs/adr/0005):
///
/// | Text    | Kind          | a        | b               |
/// |---------|---------------|----------|-----------------|
/// | `x,y`   | Absolute      | x        | y               |
/// | `@dx,dy`| Relative      | dx       | dy              |
/// | `d<a`   | PolarAbsolute | distance | angle, degrees  |
/// | `@d<a`  | PolarRelative | distance | angle, degrees  |
/// | `d`     | Distance      | distance | (unused)        |
///
/// Angles are counter-clockwise from +X, as everywhere in the drawing.
struct PointSpec {
    enum class Kind : std::uint8_t { Absolute, Relative, PolarAbsolute, PolarRelative, Distance };

    Kind kind{Kind::Absolute};
    double a{0.0};  ///< x, dx or distance
    double b{0.0};  ///< y, dy or angle in degrees (unused for Distance)

    friend constexpr bool operator==(const PointSpec&, const PointSpec&) = default;
};

/// Reads a typed coordinate. Leading and trailing white space is ignored;
/// none is allowed inside. Numbers have an optional sign, a decimal point
/// (never a comma) and an optional exponent ("1.5e3"). The result does not
/// depend on the locale of the system. Returns std::nullopt if `text` is not
/// a coordinate, or if a number is out of the range of double.
[[nodiscard]] std::optional<PointSpec> parseCoordinate(std::string_view text);

/// The point of the drawing that `spec` stands for. `lastPoint` is the
/// reference of the relative forms and of Distance; `pointer` gives the
/// direction of Distance, from `lastPoint`. Returns std::nullopt if a needed
/// datum is missing: no last point, or a pointer on the last point.
[[nodiscard]] std::optional<core::Vec2> resolve(const PointSpec& spec,
                                                std::optional<core::Vec2> lastPoint,
                                                core::Vec2 pointer);

}  // namespace tg::cmd
