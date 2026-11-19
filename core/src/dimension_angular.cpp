// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The graphics of angular dimensions between two lines
// (docs/tappe/0.8/PT24-quote-angolari-radiali.md, point 1): the fit, the
// arrows and the text of linear dimensions (PT23, point 4), measured along
// the arc.

#include "dimension_graphics.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/intersections.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"

#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace tg::core::dimension_graphics {
namespace {

/// The reason of parallel or coincident lines (PT5, point B13).
constexpr std::string_view kLinesAreParallel = "Lines are parallel.";

bool finite(const Vec2& point) {
    return std::isfinite(point.x) && std::isfinite(point.y);
}

/// +1 on the left of the line, -1 on the right, 0 on it.
int signOf(Side side) {
    switch (side) {
        case Side::Left:
            return 1;
        case Side::Right:
            return -1;
        case Side::On:
            break;
    }
    return 0;
}

Vec2 unit(const Vec2& vector) {
    return vector * (1.0 / vector.length());
}

/// The angle that point16 selects among the four the two lines make.
struct Sector {
    Vec2 vertex;
    Vec2 first;            ///< unit, the ray along the first line
    Vec2 second;           ///< unit, the ray along the second line
    bool fromFirst{true};  ///< the arc goes counter-clockwise from `first`
};

/// The sector of `definition`, or the reason why there is none.
std::optional<Sector> sectorOf(const DimensionDefinition& definition, std::string& reason) {
    const Vec2 p13 = definition.point13;
    const Vec2 p14 = definition.point14;
    const Vec2 linePoint = definition.linePoint;
    const Vec2 p15 = definition.point15;
    const Vec2 p16 = definition.point16;
    reason = kDimensionNotRegenerable;
    if (p13 == p14 || linePoint == p15) {
        return std::nullopt;
    }
    const Entity line1{Line{p13, p14}};
    const Entity line2{Line{linePoint, p15}};
    const Intersections meeting = intersect(line1, Extension::Extended, line2, Extension::Extended);
    if (!meeting.complete) {
        return std::nullopt;
    }
    if (meeting.crossings.size() != 1 || !meeting.shared.empty()) {
        reason = kLinesAreParallel;
        return std::nullopt;
    }
    const Vec2 d1 = unit(p14 - p13);
    const Vec2 d2 = unit(p15 - linePoint);
    const double turn = cross(d1, d2);
    if (turn == 0.0) {
        reason = kLinesAreParallel;
        return std::nullopt;
    }
    const int k = turn > 0.0 ? 1 : -1;
    // The sides of point16 with respect to the two lines, decided by
    // solid-space, not with respect to the vertex built from them (PT24,
    // point 1). With point16 - vertex = α·d1 + β·d2, the side of the first
    // line is the sign of β·k and that of the second the sign of -α·k.
    const std::optional<Side> side1 = sideOf(line1, p16);
    const std::optional<Side> side2 = sideOf(line2, p16);
    if (!side1 || !side2) {
        return std::nullopt;
    }
    int alpha = -signOf(*side2) * k;
    int beta = signOf(*side1) * k;
    if (alpha == 0 && beta == 0) {
        return std::nullopt;  // point16 is the vertex (PT5, point B13)
    }
    // A point on a ray belongs to the angle that comes after it
    // counter-clockwise (PT5, point B13): the other ray is the one
    // counter-clockwise from it.
    if (beta == 0) {
        beta = alpha * k;
    } else if (alpha == 0) {
        alpha = -beta * k;
    }
    Sector sector;
    sector.vertex = meeting.crossings.front().point;
    sector.first = d1 * static_cast<double>(alpha);
    sector.second = d2 * static_cast<double>(beta);
    // cross(first, second) has the sign of α·β·k.
    sector.fromFirst = alpha * beta * k > 0;
    return sector;
}

}  // namespace

std::optional<Dimension> regeneratedAngular(const Document& document, Dimension dimension,
                                            const DimVariables& variables, std::string* reason) {
    const DimensionDefinition& definition = dimension.definition;
    DimensionPresentation& presentation = dimension.presentation;
    const auto refuse = [&](std::string_view why) -> std::optional<Dimension> {
        if (reason != nullptr) {
            *reason = why;
        }
        return std::nullopt;
    };
    if (!finite(definition.point13) || !finite(definition.point14) ||
        !finite(definition.linePoint) || !finite(definition.point15) ||
        !finite(definition.point16) || !finite(presentation.textPoint) ||
        (presentation.textRotation && !std::isfinite(*presentation.textRotation))) {
        return refuse(kDimensionNotRegenerable);
    }
    std::string why;
    const std::optional<Sector> found = sectorOf(definition, why);
    if (!found) {
        return refuse(why);
    }
    const Sector& sector = *found;
    const Vec2 v = sector.vertex;
    const double radius = distance(v, definition.point16);
    if (radius == 0.0 || !std::isfinite(radius)) {
        return refuse(kDimensionNotRegenerable);
    }
    const Vec2 start = sector.fromFirst ? sector.first : sector.second;
    const Vec2 end = sector.fromFirst ? sector.second : sector.first;
    const double measurement = atan2(cross(start, end), dot(start, end));
    const double startAngle = angleOf(start);
    const double endAngle = angleOf(end);

    // The ends of the arc on the two rays, and the tangent at each end that
    // goes into the arc.
    const Vec2 end1 = v + (sector.first * radius);
    const Vec2 end2 = v + (sector.second * radius);
    const Vec2 into1 =
        sector.fromFirst ? sector.first.perpendicular() : -sector.first.perpendicular();
    const Vec2 into2 =
        sector.fromFirst ? -sector.second.perpendicular() : sector.second.perpendicular();
    // The middle of the arc and its tangent (the sweep is below π).
    const Vec2 bisector = unit(start + end);
    const Vec2 middle = v + (bisector * radius);
    const Vec2 tangent = bisector.perpendicular();
    const double length = radius * measurement;

    const double arrow = variables.arrowSize;
    const double gap = variables.gap;
    Builder builder{document, variables};

    // Extension lines: only when the arc lies beyond the line, from its end
    // nearer the arc (plus DIMEXO) to the arc (plus DIMEXE), along the ray.
    const auto extension = [&](const Vec2& ray, const Vec2& a, const Vec2& b, bool suppressed) {
        const double nearer = std::fmax(dot(a - v, ray), dot(b - v, ray));
        const double from = nearer + variables.extOffset;
        const double to = radius + variables.extExtension;
        if (suppressed || radius <= nearer || to <= from) {
            return;
        }
        builder.line(v + (ray * from), v + (ray * to), Part::ExtensionLine);
    };
    extension(sector.first, definition.point13, definition.point14, variables.suppressExt1);
    extension(sector.second, definition.linePoint, definition.point15, variables.suppressExt2);

    // The text, in degrees (dimensionText()), and its extent along a
    // direction.
    const std::string value = dimensionText(
        variables, definition.kind, radiansToDegrees(measurement), presentation.textOverride);
    const bool hasText = !value.empty();
    MText text;
    text.height = variables.textHeight;
    text.style = variables.textStyle;
    text.value = value;
    const TextSize size = hasText ? textSize(document, text) : TextSize{};
    const auto alongLine = [&](bool horizontal) {
        return !presentation.textRotation && !horizontal;
    };
    // The automatic angle reads along `direction`; an explicit one always
    // holds (PT23, point 4).
    const auto angleFor = [&](bool horizontal, const Vec2& direction) {
        if (presentation.textRotation) {
            return *presentation.textRotation;
        }
        return horizontal ? 0.0 : angleOf(readingDirection(direction));
    };
    const auto extentOf = [&](bool horizontal, const Vec2& direction) {
        if (alongLine(horizontal)) {
            return size.width;
        }
        const SinCos rotation = cardinalSinCos(angleFor(horizontal, direction));
        const Vec2 x{rotation.cosine, rotation.sine};
        return (std::fabs(dot(x, direction)) * size.width) +
               (std::fabs(cross(x, direction)) * size.height);
    };

    // Inside or outside, along the arc (PT5, point B15), with two arrows and
    // before the suppressions (PT23, point 4).
    const Fit fit = fitOf(length, extentOf(variables.textInsideHorizontal, tangent), arrow);
    const bool arrowsInside = fit == Fit::BothInside || fit == Fit::ArrowsInside;
    const bool textInside = fit == Fit::BothInside || fit == Fit::TextInside;
    const bool horizontal =
        textInside ? variables.textInsideHorizontal : variables.textOutsideHorizontal;
    // Outside, the text goes past the end on the second ray, along the
    // tangent there, as it goes past f2 for a linear dimension.
    const Vec2 out2 = -into2;
    const Vec2 direction = textInside ? tangent : out2;
    const double extent = extentOf(horizontal, direction);
    const bool above = alongLine(horizontal) && variables.textAbove != 0;
    const Vec2 up = readingDirection(direction).perpendicular();
    text.rotation = normalizeAngle(angleFor(horizontal, direction));

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
        const double nearBorder = (fit == Fit::BothOutside ? 2.0 * arrow : arrow) + gap;
        const Vec2 center = end2 + (out2 * (nearBorder + (extent / 2.0)));
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

    // The pieces of the dimension line outside the arc, along the tangents.
    const bool suppress1 = variables.suppressDim1;
    const bool suppress2 = variables.suppressDim2;
    if (!arrowsInside) {
        if (!suppress1) {
            builder.line(end1 - (into1 * arrow), end1 - (into1 * (2.0 * arrow)),
                         Part::DimensionLine);
        }
        if (!suppress2) {
            builder.line(end2 + (out2 * arrow), end2 + (out2 * beyond), Part::DimensionLine);
        }
    } else if (!textInside && !suppress2) {
        builder.line(end2, end2 + (out2 * beyond), Part::DimensionLine);
    }

    // The arc from one ray to the other, broken around a centered text; each
    // suppression takes the half on the side of its ray.
    const bool suppressStart = sector.fromFirst ? suppress1 : suppress2;
    const bool suppressEnd = sector.fromFirst ? suppress2 : suppress1;
    const double half = measurement / 2.0;
    const auto piece = [&](double from, double to) {
        if (from < to) {
            builder.arc(v, radius, startAngle + from, startAngle + to);
        }
    };
    if (breaks) {
        const double opening = ((extent / 2.0) + gap) / radius;
        if (!suppressStart) {
            piece(0.0, half - opening);
        }
        if (!suppressEnd) {
            piece(half + opening, measurement);
        }
    } else if (!suppressStart && !suppressEnd) {
        builder.arc(v, radius, startAngle, endAngle);
    } else if (!suppressStart) {
        piece(0.0, half);
    } else if (!suppressEnd) {
        piece(half, measurement);
    }

    // Arrows tangent to the arc at its ends, from inside or from outside.
    if (!suppress1) {
        builder.arrow(end1, end1 + (into1 * (arrowsInside ? arrow : -arrow)), sector.first);
    }
    if (!suppress2) {
        builder.arrow(end2, end2 + (into2 * (arrowsInside ? arrow : -arrow)), sector.second);
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
