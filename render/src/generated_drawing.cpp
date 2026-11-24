// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "generated_drawing.hpp"

#include "linework.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/math.hpp"
#include "tg/core/text.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <optional>
#include <type_traits>
#include <variant>

namespace tg::render {

using core::Vec2;

namespace {

// As core::tessellate() of an arc: never more segments than this.
constexpr int kMaxEllipseSegments = 4096;

// Index 7 follows the theme; no other colour does, not even a white RGB
// (docs/adr/0024, point 5).
Color penColor(const core::EffectiveProperties& properties, const SceneStyle& style) {
    if (properties.colorIsSeven) {
        return style.entity;
    }
    return Color{properties.color.r, properties.color.g, properties.color.b};
}

// The sweep of the parameter from start to end, in (0, 2π]: an end equal to
// the start, modulo 2π, is the full ellipse.
double parameterSweep(const core::Ellipse& ellipse) {
    double sweep = std::fmod(ellipse.endParameter - ellipse.startParameter, core::kTwoPi);
    if (!(sweep > 0.0)) {
        sweep += core::kTwoPi;
    }
    return sweep;
}

class ShapeDrawer {
public:
    ShapeDrawer(const Color& fill, const Viewport& viewport, double tolerancePx, Painter& painter)
        : m_fill(fill), m_viewport(viewport), m_tolerancePx(tolerancePx), m_painter(painter) {}

    void operator()(const core::Polyline& polyline) {
        toScreen(polyline.tessellate(toleranceWorld()));
        m_painter.drawPolyline(m_screenPoints, polyline.isClosed());
    }

    void operator()(const core::Line& line) {
        const std::array points{m_viewport.worldToScreen(line.start),
                                m_viewport.worldToScreen(line.end)};
        m_painter.drawPolyline(points, false);
    }

    void operator()(const core::Circle& circle) {
        // A closed polyline: the last point, back on the start, is dropped.
        drawArc(circle.geometry(), true);
    }

    void operator()(const core::Arc& arc) { drawArc(arc.geometry(), false); }

    void operator()(const core::Ellipse& ellipse) {
        toScreen(ellipsePoints(ellipse, toleranceWorld()));
        m_painter.drawPolyline(m_screenPoints, isFullEllipse(ellipse));
    }

    void operator()(const core::Text& text) {
        const auto frame = core::textFrame(text);
        const double heightPx = frame.height * m_viewport.scale();
        if (heightPx < kMinTextHeightPx) {
            const auto box = core::textBox(text);
            toScreen({box.begin(), box.end()});
            m_painter.drawPolyline(m_screenPoints, true);
            return;
        }
        // The screen flips Y and the angle together: a rotation that is
        // counter-clockwise in the drawing is counter-clockwise as seen.
        m_painter.drawText(TextRun{
            core::displayText(text.value), m_viewport.worldToScreen(frame.origin), heightPx,
            frame.rotation, frame.widthFactor, text.obliqueAngle, text.mirroredX, text.mirroredY});
    }

    // Until PT9 brings the faces, a formatted piece is drawn as its Text and
    // a line of a formatted MTEXT as its Line.
    void operator()(const core::FormattedText& piece) { (*this)(piece.text); }

    void operator()(const core::TextLine& textLine) { (*this)(textLine.line); }

    void operator()(const core::Solid& solid) {
        // The figure is the quadrilateral v0, v1, v3, v2 (docs/adr/0044, point 7).
        const auto& v = solid.vertices;
        const std::array points{m_viewport.worldToScreen(v[0]), m_viewport.worldToScreen(v[1]),
                                m_viewport.worldToScreen(v[3]), m_viewport.worldToScreen(v[2])};
        m_painter.fillPolygon(points, m_fill);
    }

    void operator()(const core::FilledRegion& region) {
        // The loops are tessellated before the transform: a chord error of
        // tolerance / norm becomes at most the tolerance once mapped, the
        // Frobenius norm bounding how much the map stretches.
        const Vec2 xAxis = region.transform.applyToVector({1.0, 0.0});
        const Vec2 yAxis = region.transform.applyToVector({0.0, 1.0});
        const double norm = std::sqrt(xAxis.lengthSquared() + yAxis.lengthSquared());
        if (!std::isfinite(norm) || !(norm > 0.0)) {
            return;
        }
        const double tolerance = toleranceWorld() / norm;
        std::vector<std::vector<Vec2>> loops;
        loops.reserve(region.loops.size());
        for (const core::Polyline& loop : region.loops) {
            std::vector<Vec2> points = loop.tessellate(tolerance);
            if (points.empty()) {
                continue;
            }
            for (Vec2& point : points) {
                point = m_viewport.worldToScreen(region.transform.apply(point));
            }
            loops.push_back(std::move(points));
        }
        if (!loops.empty()) {
            m_painter.fillLoops(loops, m_fill);
        }
    }

private:
    [[nodiscard]] double toleranceWorld() const { return m_tolerancePx / m_viewport.scale(); }

    void toScreen(const std::vector<Vec2>& worldPoints) {
        m_screenPoints.clear();
        for (const Vec2& point : worldPoints) {
            m_screenPoints.push_back(m_viewport.worldToScreen(point));
        }
    }

    void drawArc(const core::ArcGeometry& arc, bool closed) {
        m_worldPoints.clear();
        m_worldPoints.push_back(core::startPoint(arc));
        core::tessellate(arc, toleranceWorld(), m_worldPoints);
        if (closed) {
            m_worldPoints.pop_back();
        }
        toScreen(m_worldPoints);
        m_painter.drawPolyline(m_screenPoints, closed);
    }

    const Color& m_fill;
    const Viewport& m_viewport;
    double m_tolerancePx;
    Painter& m_painter;
    std::vector<Vec2> m_worldPoints;   // reused buffer
    std::vector<Vec2> m_screenPoints;  // reused buffer
};

// The curve that core::dashes() takes for `shape`: the entity of the same
// type, an ellipse as the polyline of its points with the pattern running
// across its vertices; none for a text and for the fills. A line of a
// formatted MTEXT is its Line.
std::optional<core::Entity> dashedCurve(const core::GeneratedShape& shape, double toleranceWorld) {
    if (const auto* ellipse = std::get_if<core::Ellipse>(&shape)) {
        std::vector<core::PolylineVertex> vertices;
        for (const Vec2& point : ellipsePoints(*ellipse, toleranceWorld)) {
            vertices.push_back(core::PolylineVertex{point});
        }
        core::Polyline polyline(std::move(vertices), isFullEllipse(*ellipse));
        polyline.setContinuousLinetype(true);
        return core::Entity{std::move(polyline)};
    }
    if (std::holds_alternative<core::Text>(shape) ||
        std::holds_alternative<core::FormattedText>(shape) ||
        std::holds_alternative<core::Solid>(shape) ||
        std::holds_alternative<core::FilledRegion>(shape)) {
        return std::nullopt;
    }
    return core::shapeEntity(shape);
}

}  // namespace

bool isFullEllipse(const core::Ellipse& ellipse) {
    return parameterSweep(ellipse) >= core::kTwoPi;
}

std::vector<Vec2> ellipsePoints(const core::Ellipse& ellipse, double maxChordError) {
    assert(maxChordError > 0.0 && "a tessellation needs a positive tolerance");
    std::vector<Vec2> points;
    const double major = std::sqrt(ellipse.majorAxis.lengthSquared());
    if (!std::isfinite(major) || !(major > 0.0)) {
        return points;
    }
    // The angular step of an arc as large as the major axis: the ellipse is
    // that circle squeezed along the minor axis, which only shortens the
    // distance between a chord and the curve.
    const double tolerance = std::clamp(maxChordError, 1e-12, major);
    const double maxStep = 2.0 * core::acos(1.0 - (tolerance / major));
    const double sweep = parameterSweep(ellipse);
    const double steps = std::ceil(sweep / std::max(maxStep, 1e-9));
    const int count = std::clamp(static_cast<int>(steps), 1, kMaxEllipseSegments);
    const bool full = isFullEllipse(ellipse);
    // The point of parameter t is center + cos t * a + sin t * b, b being a
    // turned by +90 degrees and scaled by the ratio (docs/adr/0052, point 1).
    const Vec2 a = ellipse.majorAxis;
    const Vec2 b = Vec2{-a.y, a.x} * ellipse.ratio;
    const int last = full ? count - 1 : count;
    points.reserve(static_cast<std::size_t>(last) + 1);
    for (int i = 0; i <= last; ++i) {
        const double t = ellipse.startParameter + (sweep * i / count);
        const auto direction = core::sinCos(t);
        points.push_back(ellipse.center + (a * direction.cosine) + (b * direction.sine));
    }
    return points;
}

void drawShape(const core::GeneratedShape& shape, const Color& fill, const Viewport& viewport,
               double tessellationTolerancePx, Painter& painter) {
    ShapeDrawer drawer(fill, viewport, tessellationTolerancePx, painter);
    std::visit(drawer, shape);
}

void drawGeneratedItem(const core::GeneratedItem& item, bool lwdisplay, const Viewport& viewport,
                       const SceneStyle& style, Painter& painter) {
    const Color color = penColor(item.properties, style);
    const core::GeneratedShape& shape = item.shape;
    if (std::holds_alternative<core::Solid>(shape) ||
        std::holds_alternative<core::FilledRegion>(shape)) {
        drawShape(shape, color, viewport, style.tessellationTolerancePx, painter);
        return;
    }
    if (std::holds_alternative<core::Text>(shape) ||
        std::holds_alternative<core::FormattedText>(shape)) {
        // A text has neither lineweight nor linetype; its tiny box, if it
        // comes to that, is a plain cosmetic outline.
        painter.setPen(color, 0.0, LineStyle::Solid);
        drawShape(shape, color, viewport, style.tessellationTolerancePx, painter);
        return;
    }
    const double widthPx =
        penWidthPx(item.properties.lineweight, lwdisplay, style.devicePixelRatio);
    painter.setPen(color, widthPx, LineStyle::Solid);
    const double toleranceWorld = style.tessellationTolerancePx / viewport.scale();
    if (const std::optional<core::Entity> curve = dashedCurve(shape, toleranceWorld)) {
        if (const auto pieces = dashPieces(*curve, item.properties, viewport.scale())) {
            drawDashPieces(*pieces, viewport, style.tessellationTolerancePx, painter);
            return;
        }
    }
    drawShape(shape, color, viewport, style.tessellationTolerancePx, painter);
}

}  // namespace tg::render
