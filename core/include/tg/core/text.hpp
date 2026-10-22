// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/bbox.hpp"
#include "tg/core/vec2.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace tg::core {

/// DXF group 72, in the order of its values (0 to 5).
enum class TextHorizontal : std::uint8_t { Left, Center, Right, Aligned, Middle, Fit };

/// DXF group 73, in the order of its values (0 to 3).
enum class TextVertical : std::uint8_t { Baseline, Bottom, Middle, Top };

/// DXF TEXT (one line). Every property of the DXF entity is kept (docs/adr/0013).
/// Geometry uses all alignments, oblique angle and mirroring (docs/adr/0022).
/// The DXF properties that a plane drawing does not use (Z 30,
/// thickness 39, extrusion 210/220/230) have their default values, as for
/// the other entities (docs/adr/0012, point 2).
/// Invariants: height > 0, widthFactor > 0.
struct Text {
    Vec2 position;                                    ///< 10/20, first alignment point
    Vec2 alignmentPoint;                              ///< 11/21, second alignment point
    double height{2.5};                               ///< 40, height of capital letters, > 0
    double rotation{0.0};                             ///< 50, radians
    double widthFactor{1.0};                          ///< 41
    double obliqueAngle{0.0};                         ///< 51, radians
    std::string style{"Standard"};                    ///< 7
    bool mirroredX{false};                            ///< 71 bit 2
    bool mirroredY{false};                            ///< 71 bit 4
    TextHorizontal horizontal{TextHorizontal::Left};  ///< 72
    TextVertical vertical{TextVertical::Baseline};    ///< 73
    std::string value;                                ///< 1, UTF-8, DXF control codes kept

    /// The box of textBox(), aligned with the axes.
    [[nodiscard]] BBox boundingBox() const;
    friend bool operator==(const Text&, const Text&) = default;
};

/// Placement of the text system (docs/adr/0022, points 1-4): a point (x, y),
/// in drawing units from the start of the baseline, maps to
/// origin + x * xAxis + y * yAxis. Derived values never change stored properties.
struct TextFrame {
    Vec2 origin;
    Vec2 xAxis;               ///< R(rotation) * M * (1, 0)
    Vec2 yAxis;               ///< R(rotation) * M * (tan(oblique), 1)
    double height{0.0};       ///< used height (Aligned: derived)
    double widthFactor{0.0};  ///< used width factor (Fit: derived)
    double rotation{0.0};     ///< used rotation, radians (Aligned, Fit: P1 -> P2)
    Vec2 insertion;           ///< insertion snap point (docs/adr/0022, point 5)
};

[[nodiscard]] TextFrame textFrame(const Text& text);

/// The images of (0, d*h), (w, d*h), (w, a*h), (0, a*h), in that order
/// (docs/adr/0022, point 5), where w = textWidth(displayText(value)) * h * f,
/// a/d are the bundled font's ascent/descent and h/f are the used values.
/// The parallelogram is counter-clockwise without mirroring, clockwise with
/// exactly one mirroring. Empty text collapses to a segment.
[[nodiscard]] std::array<Vec2, 4> textBox(const Text& text);

/// `value` decoded from UTF-8, with %%c, %%d, %%p and %%% converted to ⌀, °,
/// ± and % (docs/adr/0013, point 5), in either case. Any other sequence,
/// incomplete ones included (a final "%%", "%%x"), stays as it is written.
[[nodiscard]] std::u32string displayText(std::string_view value);

}  // namespace tg::core
