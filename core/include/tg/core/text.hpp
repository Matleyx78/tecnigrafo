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

/// DXF TEXT (one line). Every property of the DXF entity is kept, even
/// those that tappa 0.4 does not draw yet (docs/adr/0013).
///
/// The box, the drawing and the selection use position, height, rotation
/// and width factor only: oblique angle, mirroring and alignments other than
/// left on the baseline are kept but ignored until the DXF import of tappa
/// 0.5. The DXF properties that a plane drawing does not use (Z 30,
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

/// The four corners of the text box, counter-clockwise from the bottom
/// left of the first character (docs/adr/0013, point 3): in the system of
/// the text, from 0 to width · height · widthFactor along the baseline and
/// from descent · height to ascent · height across it, rotated about
/// `position`. The width is that of displayText(value) in the bundled font.
[[nodiscard]] std::array<Vec2, 4> textBox(const Text& text);

/// `value` decoded from UTF-8, with %%c, %%d, %%p and %%% converted to ⌀, °,
/// ± and % (docs/adr/0013, point 5), in either case. Any other sequence,
/// incomplete ones included (a final "%%", "%%x"), stays as it is written.
[[nodiscard]] std::u32string displayText(std::string_view value);

}  // namespace tg::core
