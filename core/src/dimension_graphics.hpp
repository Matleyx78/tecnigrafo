// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/vec2.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/// What the kinds of dimension share to build their graphics
/// (docs/tappe/0.8/PT23-quote-lineari.md, point 4). Private to core.
namespace tg::core::dimension_graphics {

/// `variables` with DIMASZ, DIMEXO, DIMEXE, DIMGAP, DIMTXT and DIMCEN
/// multiplied by DIMSCALE (PT23, point 2). DIMSCALE 0 counts as 1, as
/// AutoCAD does in model space.
[[nodiscard]] DimVariables scaledVariables(DimVariables variables);

/// The direction in which a line with direction `direction` (a unit vector)
/// reads: `direction` when its angle is in (-90°, 90°], its opposite
/// otherwise.
[[nodiscard]] Vec2 readingDirection(const Vec2& direction);

/// The angle of a unit vector, in [0, 2π).
[[nodiscard]] double angleOf(const Vec2& direction);

/// The size of a text laid out as an MTEXT (PT13, point 3): the widest row
/// (0 for a text without a character other than a space) and the height
/// from the top of the first row to the baseline of the last one.
struct TextSize {
    double width{0.0};
    double height{0.0};
};

[[nodiscard]] TextSize textSize(const Document& document, const MText& mtext);

/// What a line of the graphics belongs to, for its colour and lineweight.
enum class Part : std::uint8_t { DimensionLine, ExtensionLine };

/// The records of the graphics of a dimension, in drawing order: on layer 0,
/// linetype ByBlock, with the colour and the lineweight of the variables.
class Builder {
public:
    Builder(const Document& document, const DimVariables& variables);

    /// A LINE from `start` to `end`; nothing when they coincide (ADR 0036,
    /// point 5: no piece of length zero).
    void line(const Vec2& start, const Vec2& end, Part part);

    /// A closed filled arrow (docs/adr/0047, point 4): a triangular SOLID with
    /// the tip in `tip`, the base in `base`, as wide as a third of the
    /// distance; its first vertex is on the side of `up`, a unit vector
    /// across the arrow.
    void arrow(const Vec2& tip, const Vec2& base, const Vec2& up);

    /// The text, with the colour DIMCLRT.
    void text(MText mtext);

    [[nodiscard]] std::vector<EntityRecord> take();

private:
    [[nodiscard]] EntityProperties properties(const Color& color, int lineweight) const;

    const Document* m_document;
    const DimVariables* m_variables;
    std::vector<EntityRecord> m_records;
};

/// Linear and aligned dimensions (dimension_linear.cpp): the measurement,
/// the text and the graphics of `dimension`, with the effective variables
/// already scaled; nullopt with the reason when the points do not give one.
[[nodiscard]] std::optional<Dimension> regeneratedLinear(const Document& document,
                                                         Dimension dimension,
                                                         const DimVariables& variables,
                                                         std::string* reason);

}  // namespace tg::core::dimension_graphics
