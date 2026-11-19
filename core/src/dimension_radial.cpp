// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The graphics of radius and diameter dimensions
// (docs/tappe/0.8/PT24-quote-angolari-radiali.md, points 2 and 3), with the
// rules that the cases of RT24 take from AutoCAD for small circles.

#include "dimension_graphics.hpp"
#include "tg/core/arc_geometry.hpp"

#include <cmath>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace tg::core::dimension_graphics {
namespace {

bool finite(const Vec2& point) {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

/// The center mark (PT24, point 2; PT5, point B16): with DIMCEN > 0 a cross
/// of two segments 2·DIMCEN long; with DIMCEN < 0 the cross 2·|DIMCEN| long
/// and the center lines, from |DIMCEN| past the cross to |DIMCEN| past the
/// circle, when they have a length; nothing with DIMCEN 0.
void centerMark(Builder& builder, const Vec2& center, double radius, double mark) {
    if (mark == 0.0) {
        return;
    }
    const double size = std::fabs(mark);
    const Vec2 x{1.0, 0.0};
    const Vec2 y{0.0, 1.0};
    builder.line(center - (x * size), center + (x * size), Part::DimensionLine);
    builder.line(center - (y * size), center + (y * size), Part::DimensionLine);
    const double from = 2.0 * size;
    const double to = radius + size;
    if (mark > 0.0 || to <= from) {
        return;
    }
    for (const Vec2& axis : {x, -x, y, -y}) {
        builder.line(center + (axis * from), center + (axis * to), Part::DimensionLine);
    }
}

}  // namespace

std::optional<Dimension> regeneratedRadial(const Document& document, Dimension dimension,
                                           const DimVariables& variables, std::string* reason) {
    const DimensionDefinition& definition = dimension.definition;
    const DimensionPresentation& presentation = dimension.presentation;
    const auto refuse = [&](std::string_view why) -> std::optional<Dimension> {
        if (reason != nullptr) {
            *reason = why;
        }
        return std::nullopt;
    };
    const bool diameter = definition.kind == DimensionKind::Diameter;
    // point15 is on the curve; linePoint is the center of a radius and the
    // opposite point of a diameter.
    const Vec2 point15 = definition.point15;
    const Vec2 linePoint = definition.linePoint;
    const Vec2 t = presentation.textPoint;
    if (!finite(point15) || !finite(linePoint) || !finite(t) ||
        (presentation.textRotation && !std::isfinite(*presentation.textRotation))) {
        return refuse(kDimensionNotRegenerable);
    }
    if (point15 == linePoint) {
        return refuse("Points coincide.");
    }
    const Vec2 c = diameter ? (point15 + linePoint) * 0.5 : linePoint;
    const double measurement = distance(point15, linePoint);
    const double radius = diameter ? measurement / 2.0 : measurement;
    if (!diameter && t == c) {
        return refuse(kDimensionNotRegenerable);  // a text in the center (PT24, point 2)
    }
    // The text on the curve counts as inside (PT5, point B16).
    const bool inside = distance(t, c) <= radius;
    // The end the text belongs to: for a diameter the point nearer the text,
    // point15 at equal distance (PT5, point B16); the other is `far`.
    const bool nearIs15 = !diameter || distance(t, point15) <= distance(t, linePoint);
    const Vec2 near = nearIs15 ? point15 : linePoint;
    const Vec2 far = nearIs15 ? linePoint : point15;
    const Vec2 u = (near - c) * (1.0 / radius);  // from the center to `near`
    // DIMSD1 takes the half of point15 and its arrow, DIMSD2 that of
    // linePoint: a radius has only the half of point15 (PT5, point B11).
    const bool suppressNear = nearIs15 ? variables.suppressDim1 : variables.suppressDim2;
    const bool suppressFar = nearIs15 ? variables.suppressDim2 : variables.suppressDim1;
    const double arrow = variables.arrowSize;
    const double gap = variables.gap;
    const Vec2 arrowUp = readingDirection(u).perpendicular();
    // A small circle, where the arrows do not fit in the radius, with the
    // text outside follows the AutoCAD drawings of the cases of RT24 (PT24,
    // "Cosa fare"); the other dimensions follow points 2 and 3 of PT24.
    const bool small = radius < arrow && !inside;

    // The text and the direction of the line it goes with.
    const std::string value =
        dimensionText(variables, definition.kind, measurement, presentation.textOverride);
    const bool hasText = !value.empty();
    MText text;
    text.height = variables.textHeight;
    text.style = variables.textStyle;
    text.value = value;
    const Vec2 lineDirection = (inside || small) ? u : (t - near) * (1.0 / distance(t, near));
    const bool horizontal =
        inside ? variables.textInsideHorizontal : variables.textOutsideHorizontal;
    const bool alongLine = !presentation.textRotation && !horizontal;
    const Vec2 reading = readingDirection(lineDirection);
    if (presentation.textRotation) {
        text.rotation = normalizeAngle(*presentation.textRotation);
    } else {
        text.rotation = horizontal ? 0.0 : angleOf(reading);
    }
    // The text in its point (PT24, point 2). In a small circle a text placed
    // by the user is a point of the dimension line, and the text goes above
    // it with DIMTAD (handle 223 of the cases of RT24); otherwise the point is
    // the middle of the text (handles 1FB and 20D).
    if (small && presentation.userTextPosition && alongLine && variables.textAbove != 0) {
        text.attachment = MTextAttachment::BottomCenter;
        text.insertion = t + (reading.perpendicular() * gap);
    } else {
        text.attachment = MTextAttachment::MiddleCenter;
        text.insertion = t;
    }

    Builder builder{document, variables};
    if (small) {
        // As AutoCAD (handles 1FB, 20D and 223 of the cases of RT24): the
        // line goes from the arrow base towards the text, along the radius,
        // to DIMGAP past the text taken as long on the far side of the text
        // point as on the near one, where it starts DIMASZ + DIMGAP past the
        // arrow base (the automatic place of AutoCAD). With the text in front
        // of the point the arrow comes from outside and the line also goes
        // inside, from the center (DIMTOFL 1 of ISO-25); a diameter also has
        // the arrow of the far point outside, with its piece of line. With
        // the text behind the center (a radius) the arrow comes from inside,
        // through the center. No center mark.
        const bool front = dot(t - c, u) >= 0.0;
        const Vec2 d = front ? u : -u;
        const Vec2 base = near + (d * arrow);
        const double reach = (2.0 * dot(t - base, d)) - arrow;
        if (!suppressNear && reach > 0.0) {
            builder.line(base + (d * reach), base, Part::DimensionLine);
        }
        if (diameter && !suppressFar) {
            builder.line(far - (u * arrow), far - (u * (2.0 * arrow)), Part::DimensionLine);
        }
        if (hasText) {
            builder.text(std::move(text));
        }
        if (front) {
            if (diameter) {
                const Vec2 to15 = nearIs15 ? near : far;
                const bool suppress2 = variables.suppressDim2;
                const bool suppress1 = variables.suppressDim1;
                if (!suppress1 || !suppress2) {
                    builder.line(suppress2 ? c : linePoint, suppress1 ? c : to15,
                                 Part::DimensionLine);
                }
            } else if (!suppressNear) {
                builder.line(c, near, Part::DimensionLine);
            }
        }
        if (diameter && !suppressFar && nearIs15) {
            builder.arrow(far, far - (u * arrow), arrowUp);
        }
        if (!suppressNear) {
            builder.arrow(near, base, arrowUp);
        }
        if (diameter && !suppressFar && !nearIs15) {
            builder.arrow(far, far - (u * arrow), arrowUp);
        }
    } else {
        centerMark(builder, c, radius, variables.centerMark);
        // Outside: the line from the near point to the text, the arrow at it
        // pointing at the center from outside. Inside: the line across the
        // curve, from the center for a radius, the arrows pointing outwards.
        if (!inside && !suppressNear) {
            builder.line(near, t, Part::DimensionLine);
        }
        if (hasText) {
            builder.text(std::move(text));
        }
        if (inside) {
            if (diameter) {
                const bool suppress1 = variables.suppressDim1;
                const bool suppress2 = variables.suppressDim2;
                if (!suppress1 || !suppress2) {
                    builder.line(suppress2 ? c : linePoint, suppress1 ? c : point15,
                                 Part::DimensionLine);
                }
                if (!suppress2) {
                    builder.arrow(linePoint,
                                  linePoint + ((point15 - linePoint) * (arrow / measurement)),
                                  arrowUp);
                }
                if (!suppress1) {
                    builder.arrow(point15,
                                  point15 + ((linePoint - point15) * (arrow / measurement)),
                                  arrowUp);
                }
            } else if (!suppressNear) {
                builder.line(c, near, Part::DimensionLine);
                builder.arrow(near, near - (u * arrow), arrowUp);
            }
        } else if (!suppressNear) {
            builder.arrow(near, near + (u * arrow), arrowUp);
        }
    }

    dimension.value.measurement = measurement;
    dimension.graphics = builder.take();
    dimension.rawGraphics.clear();
    return dimension;
}

}  // namespace tg::core::dimension_graphics
