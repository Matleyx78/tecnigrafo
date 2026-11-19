// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/generated.hpp"
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

/// The four cases of the fit (docs/adr/0047, point 4), in their order.
enum class Fit : std::uint8_t { BothInside, TextInside, ArrowsInside, BothOutside };

/// The case of the fit for a dimension line of length `length` (for an
/// angular dimension, the length of the arc), a text that takes `textExtent`
/// along it and two arrows of `arrow`. The thresholds come from the narrow
/// dimensions of AutoCAD in the cases of RT23 (they contradict the starting
/// hypotheses of PT23, point 4, and the cases prevail): with L = 8, an arrow
/// of 2.5 and the text 8 both stay inside although L < t + 2·DIMGAP + 2a,
/// and with L = 3 the text stays inside although L < t + 2·DIMGAP. The rule
/// that gives every case has no DIMGAP: both inside when L >= t + 2a, the
/// text when L >= t, the arrows when L >= 2a.
[[nodiscard]] Fit fitOf(double length, double textExtent, double arrow);

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

    /// An ARC of the dimension line, counter-clockwise from `startAngle` to
    /// `endAngle` (radians, brought into [0, 2π)); nothing when they are
    /// equal.
    void arc(const Vec2& center, double radius, double startAngle, double endAngle);

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

/// Angular dimensions (dimension_angular.cpp, PT24, point 1), as
/// regeneratedLinear().
[[nodiscard]] std::optional<Dimension> regeneratedAngular(const Document& document,
                                                          Dimension dimension,
                                                          const DimVariables& variables,
                                                          std::string* reason);

/// Radius and diameter dimensions (dimension_radial.cpp, PT24, points 2 and
/// 3), as regeneratedLinear().
[[nodiscard]] std::optional<Dimension> regeneratedRadial(const Document& document,
                                                         Dimension dimension,
                                                         const DimVariables& variables,
                                                         std::string* reason);

}  // namespace tg::core::dimension_graphics

namespace tg::core {

/// The items of generate(document, record) for `dimension`, the entity of
/// `record`, in the same order, without the rows of the MTEXT of its
/// graphics: a row gives no snap point of its own, only the MTEXT its own
/// points (appendOwnSnapPointsOf()), as outside a dimension (docs/adr/0046,
/// point 2; PT23, point 7).
[[nodiscard]] std::vector<GeneratedItem> snapItemsOf(const Document& document,
                                                     const EntityRecord& record,
                                                     const Dimension& dimension);

/// A drawn MTEXT of the graphics of a dimension: its record, with the
/// properties it is drawn with, and its drawn rows.
struct DrawnMText {
    EntityRecord record;
    std::vector<GeneratedItem> rows;
};

/// The MTEXTs of the graphics of `dimension`, the entity of `record`, with at
/// least one drawn row, in drawing order. Within the aperture a row leads to
/// the points of its MTEXT, as outside a dimension (docs/adr/0046, point 2;
/// PT13, point 6; PT23, point 7).
[[nodiscard]] std::vector<DrawnMText> drawnMTextsOf(const Document& document,
                                                    const EntityRecord& record,
                                                    const Dimension& dimension);

}  // namespace tg::core
