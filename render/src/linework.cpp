// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "linework.hpp"

#include "tg/core/arc_geometry.hpp"
#include "tg/core/math.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <variant>

namespace tg::render {

using core::Vec2;

namespace {

class DashDrawer {
public:
    DashDrawer(const Viewport& viewport, double tessellationTolerancePx, Painter& painter)
        : m_viewport(viewport), m_tolerancePx(tessellationTolerancePx), m_painter(painter) {}

    void operator()(const core::DashLine& line) {
        m_painter.drawLine(m_viewport.worldToScreen(line.start),
                           m_viewport.worldToScreen(line.end));
    }

    void operator()(const core::DashArc& dash) {
        // core normalizes both angles into [0, 2π) and runs counter-clockwise
        // from the first: an end below the start means the dash crosses 0.
        double sweep = dash.endAngle - dash.startAngle;
        if (sweep < 0.0) {
            sweep += core::kTwoPi;
        }
        const core::ArcGeometry arc{dash.center, dash.radius, dash.startAngle, sweep};
        const double toleranceWorld = m_tolerancePx / m_viewport.scale();
        m_worldPoints.clear();
        m_worldPoints.push_back(core::startPoint(arc));
        core::tessellate(arc, toleranceWorld, m_worldPoints);
        m_screenPoints.clear();
        for (const Vec2& point : m_worldPoints) {
            m_screenPoints.push_back(m_viewport.worldToScreen(point));
        }
        m_painter.drawPolyline(m_screenPoints, false);
    }

    void operator()(const core::DashPoint& dot) {
        // The direction is a unit vector in drawing units: one pixel is
        // 1 / scale of them, whatever the zoom.
        const Vec2 tip = dot.position + dot.direction * (kDotLengthPx / m_viewport.scale());
        m_painter.drawLine(m_viewport.worldToScreen(dot.position), m_viewport.worldToScreen(tip));
    }

private:
    const Viewport& m_viewport;
    double m_tolerancePx;
    Painter& m_painter;
    std::vector<Vec2> m_worldPoints;   // reused buffer
    std::vector<Vec2> m_screenPoints;  // reused buffer
};

}  // namespace

double penWidthPx(int lineweight, bool lwdisplay, double devicePixelRatio) {
    assert(lineweight >= 0 && "a lineweight must be resolved before it is drawn");
    assert(devicePixelRatio > 0.0 && "the device pixel ratio must be positive");
    if (!lwdisplay) {
        return 0.0;
    }
    constexpr double kHundredthsPerMillimetre = 100.0;
    const double millimetres = static_cast<double>(lineweight) / kHundredthsPerMillimetre;
    const double pixels = millimetres * kPixelsPerInch / kMillimetresPerInch * devicePixelRatio;
    return std::max(1.0, std::round(pixels));
}

std::optional<std::vector<core::DashPiece>> dashPieces(const core::Entity& entity,
                                                       const core::EffectiveProperties& properties,
                                                       double pixelsPerUnit) {
    assert(pixelsPerUnit > 0.0 && "a view needs a positive scale");
    if (std::holds_alternative<core::Text>(entity)) {
        return std::nullopt;
    }
    const core::LinetypePattern& pattern = properties.linetype->pattern;
    const double scale = properties.linetypeScale;
    if (pattern.continuous() || !std::isfinite(scale) || scale <= 0.0) {
        return std::nullopt;
    }
    const double patternPx = pattern.length() * scale * pixelsPerUnit;
    // Not finite covers a pattern with a non-finite element, which core::dashes()
    // does not accept.
    if (!std::isfinite(patternPx) || patternPx < kMinPatternLengthPx) {
        return std::nullopt;
    }
    return core::dashes(entity, pattern, scale);
}

void drawDashPieces(std::span<const core::DashPiece> pieces, const Viewport& viewport,
                    double tessellationTolerancePx, Painter& painter) {
    DashDrawer drawer(viewport, tessellationTolerancePx, painter);
    for (const core::DashPiece& piece : pieces) {
        std::visit(drawer, piece);
    }
}

}  // namespace tg::render
