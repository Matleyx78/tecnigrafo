// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The graphics of linear and aligned dimensions (docs/tappe/0.8/PT23,
// point 4, with the rules the cases of RT23 take from AutoCAD).

#include "dimension_graphics.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/math.hpp"

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace tg::core::dimension_graphics {
namespace {

/// The four cases of the fit (docs/adr/0047, point 4), in their order.
enum class Fit : std::uint8_t { BothInside, TextInside, ArrowsInside, BothOutside };

/// The thresholds of the fit, from the narrow dimensions of AutoCAD in the
/// cases of RT23 (they contradict the starting hypotheses of PT23, point 4,
/// and the cases prevail): with L = 8, an arrow of 2.5 and the text 8 both
/// stay inside although L < t + 2·DIMGAP + 2a, and with L = 3 the text stays
/// inside although L < t + 2·DIMGAP. The rule that gives every case has no
/// DIMGAP: both inside when L >= t + 2a, the text when L >= t, the arrows when
/// L >= 2a.
Fit fitOf(double length, double textExtent, double arrow) {
    if (textExtent + (2.0 * arrow) <= length) {
        return Fit::BothInside;
    }
    if (textExtent <= length) {
        return Fit::TextInside;
    }
    if (2.0 * arrow <= length) {
        return Fit::ArrowsInside;
    }
    return Fit::BothOutside;
}

bool finite(const Vec2& point) {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

}  // namespace

std::optional<Dimension> regeneratedLinear(const Document& document, Dimension dimension,
                                           const DimVariables& variables, std::string* reason) {
    const DimensionDefinition& definition = dimension.definition;
    DimensionPresentation& presentation = dimension.presentation;
    const Vec2 p1 = definition.point13;
    const Vec2 p2 = definition.point14;
    const Vec2 linePoint = definition.linePoint;
    if (!finite(p1) || !finite(p2) || !finite(linePoint) || !std::isfinite(definition.rotation) ||
        !finite(presentation.textPoint) ||
        (presentation.textRotation && !std::isfinite(*presentation.textRotation))) {
        if (reason != nullptr) {
            *reason = kDimensionNotRegenerable;
        }
        return std::nullopt;
    }

    // Direction u, measurement and the feet f1, f2 on the dimension line.
    Vec2 u;
    double measurement = 0.0;
    if (definition.kind == DimensionKind::Linear) {
        const SinCos direction = cardinalSinCos(definition.rotation);
        u = {direction.cosine, direction.sine};
        measurement = std::fabs(dot(p2 - p1, u));
    } else {
        const Vec2 delta = p2 - p1;
        measurement = delta.length();
        if (measurement == 0.0) {
            if (reason != nullptr) {
                *reason = "Points coincide.";
            }
            return std::nullopt;
        }
        u = delta * (1.0 / measurement);
    }
    const Vec2 f1 = linePoint + (u * dot(p1 - linePoint, u));
    const Vec2 f2 = linePoint + (u * dot(p2 - linePoint, u));
    // From f1 to f2; u when they coincide (PT5, point B08).
    const Vec2 w = dot(p2 - p1, u) < 0.0 ? -u : u;
    const double length = measurement;
    const Vec2 middle = (f1 + f2) * 0.5;
    const Vec2 reading = readingDirection(u);
    const Vec2 up = reading.perpendicular();
    const double lineAngle = angleOf(reading);

    const double arrow = variables.arrowSize;
    const double gap = variables.gap;
    Builder builder{document, variables};

    // Extension lines.
    const auto extension = [&](const Vec2& origin, const Vec2& foot, bool suppressed) {
        if (suppressed || origin == foot) {
            return;
        }
        const Vec2 delta = foot - origin;
        const Vec2 e = delta * (1.0 / delta.length());
        builder.line(origin + (e * variables.extOffset), foot + (e * variables.extExtension),
                     Part::ExtensionLine);
    };
    extension(p1, f1, variables.suppressExt1);
    extension(p2, f2, variables.suppressExt2);

    // The text and its extent along the dimension line.
    const std::string value =
        dimensionText(variables, definition.kind, measurement, presentation.textOverride);
    const bool hasText = !value.empty();
    MText text;
    text.height = variables.textHeight;
    text.style = variables.textStyle;
    text.value = value;
    const TextSize size = hasText ? textSize(document, text) : TextSize{};
    // The style decides only the automatic angle; an explicit one always
    // holds (PT23, point 4).
    const auto angleFor = [&](bool horizontal) {
        if (presentation.textRotation) {
            return *presentation.textRotation;
        }
        return horizontal ? 0.0 : lineAngle;
    };
    const auto alongLine = [&](bool horizontal) {
        return !presentation.textRotation && !horizontal;
    };
    const auto extentOf = [&](bool horizontal) {
        if (alongLine(horizontal)) {
            return size.width;
        }
        const SinCos direction = cardinalSinCos(angleFor(horizontal));
        const Vec2 x{direction.cosine, direction.sine};
        return (std::fabs(dot(x, w)) * size.width) + (std::fabs(cross(x, w)) * size.height);
    };

    // Inside or outside (docs/adr/0047, point 4): with two arrows, before the
    // suppressions (PT23, point 4).
    const Fit fit = fitOf(length, extentOf(variables.textInsideHorizontal), arrow);
    const bool arrowsInside = fit == Fit::BothInside || fit == Fit::ArrowsInside;
    const bool textInside = fit == Fit::BothInside || fit == Fit::TextInside;
    const bool horizontal =
        textInside ? variables.textInsideHorizontal : variables.textOutsideHorizontal;
    const double extent = extentOf(horizontal);
    const bool above = alongLine(horizontal) && variables.textAbove != 0;
    text.rotation = normalizeAngle(angleFor(horizontal));

    // Where the text goes. `breaks`: the dimension line stops DIMGAP before
    // the text, on both sides; `beyond`: how far past f2 the dimension line
    // goes on the side of f2 when the arrows are outside or the text is.
    // The text point is the middle of the text as it is laid out (PT5, point
    // B10): above the line, the rows go from the last baseline, in the
    // insertion point, up by their whole height.
    const auto middleAbove = [&] {
        return text.insertion + (up * (size.height / 2.0));
    };
    bool breaks = false;
    double beyond = 2.0 * arrow;
    if (presentation.userTextPosition) {
        text.attachment = MTextAttachment::MiddleCenter;
        text.insertion = presentation.textPoint;
        if (fit == Fit::ArrowsInside) {
            beyond = 0.0;
        }
    } else if (textInside) {
        if (above) {
            text.attachment = MTextAttachment::BottomCenter;
            text.insertion = middle + (up * gap);
            presentation.textPoint = middleAbove();
        } else {
            text.attachment = MTextAttachment::MiddleCenter;
            text.insertion = middle;
            presentation.textPoint = middle;
            breaks = hasText;
        }
    } else {
        // Past f2, on the dimension line extended (cases of RT23): the near
        // border of the text 2·DIMASZ + DIMGAP past f2 with the arrows
        // outside (an arrow and its piece of line), DIMASZ + DIMGAP with the
        // arrows inside.
        const double nearBorder = (fit == Fit::BothOutside ? 2.0 * arrow : arrow) + gap;
        const Vec2 center = f2 + (w * (nearBorder + (extent / 2.0)));
        if (above) {
            text.attachment = MTextAttachment::BottomCenter;
            text.insertion = center + (up * gap);
            presentation.textPoint = middleAbove();
            beyond = nearBorder + extent + gap;
        } else {
            text.attachment = MTextAttachment::MiddleCenter;
            text.insertion = center;
            presentation.textPoint = center;
            beyond = nearBorder - gap;
        }
    }

    // The dimension line: the pieces outside first, then the one between
    // the extension lines (DIMTOFL 1 of ISO-25), from arrow base to arrow
    // base when the arrows are inside.
    const bool suppress1 = variables.suppressDim1;
    const bool suppress2 = variables.suppressDim2;
    if (!arrowsInside) {
        if (!suppress1) {
            builder.line(f1 - (w * arrow), f1 - (w * (2.0 * arrow)), Part::DimensionLine);
        }
        if (!suppress2) {
            builder.line(f2 + (w * arrow), f2 + (w * beyond), Part::DimensionLine);
        }
    } else if (!textInside && !suppress2) {
        builder.line(f2, f2 + (w * beyond), Part::DimensionLine);
    }
    const Vec2 start = arrowsInside ? f1 + (w * arrow) : f1;
    const Vec2 end = arrowsInside ? f2 - (w * arrow) : f2;
    // A piece from `from` towards `to` along `direction`, if it goes that way.
    const auto piece = [&](const Vec2& from, const Vec2& to, const Vec2& direction) {
        if (dot(to - from, direction) > 0.0) {
            builder.line(from, to, Part::DimensionLine);
        }
    };
    if (breaks) {
        const double half = (extent / 2.0) + gap;
        if (!suppress1) {
            piece(start, middle - (w * half), w);
        }
        if (!suppress2) {
            piece(end, middle + (w * half), -w);
        }
    } else if (!suppress1 || !suppress2) {
        piece(suppress1 ? middle : start, suppress2 ? middle : end, w);
    }

    // Arrows, pointing at the extension lines from inside or from outside.
    if (!suppress1) {
        builder.arrow(f1, arrowsInside ? f1 + (w * arrow) : f1 - (w * arrow), up);
    }
    if (!suppress2) {
        builder.arrow(f2, arrowsInside ? f2 - (w * arrow) : f2 + (w * arrow), up);
    }

    if (hasText) {
        builder.text(std::move(text));
    }
    dimension.value.measurement = measurement;
    dimension.graphics = builder.take();
    dimension.rawGraphics.clear();
    return dimension;
}

}  // namespace tg::core::dimension_graphics
