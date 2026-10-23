// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/vec2.hpp"

#include <cstdint>
#include <span>
#include <string>

namespace tg::render {

struct Color {
    std::uint8_t r{0};
    std::uint8_t g{0};
    std::uint8_t b{0};
    std::uint8_t a{255};

    friend constexpr bool operator==(const Color&, const Color&) = default;
};

enum class LineStyle : std::uint8_t { Solid, Dashed };

/// One line of text placed on the screen.
struct TextRun {
    std::u32string text;      ///< Control codes already converted.
    core::Vec2 origin;        ///< Start of the baseline, screen pixels.
    double capHeightPx{0.0};  ///< Height of capital letters.
    double angle{0.0};        ///< Radians, counter-clockwise as seen on screen.
    double widthFactor{1.0};  ///< The used width factor.
    double oblique{0.0};      ///< Radians, positive leans right (DXF 51).
    bool backward{false};     ///< Mirrored along the baseline (DXF 71, bit 2).
    bool upsideDown{false};   ///< Mirrored across the baseline (DXF 71, bit 4).
};

/// Abstract drawing backend. Every coordinate is in screen pixels (origin
/// top-left, Y down). The scene renderer talks only to this interface, so
/// the backend (QPainter today, OpenGL/Vulkan tomorrow) can be replaced
/// without touching the drawing logic.
class Painter {
public:
    Painter() = default;
    Painter(const Painter&) = delete;
    Painter& operator=(const Painter&) = delete;
    Painter(Painter&&) = delete;
    Painter& operator=(Painter&&) = delete;
    virtual ~Painter() = default;

    virtual void clear(const Color& background) = 0;

    /// Pen used by the following draw calls. A width of 0 means "cosmetic",
    /// i.e. always one device pixel regardless of zoom.
    virtual void setPen(const Color& color, double widthPx, LineStyle style) = 0;

    /// Fills a screen-space polygon, including alpha, without an outline.
    virtual void fillPolygon(std::span<const core::Vec2> points, const Color& color) = 0;

    /// Draws run with the bundled font in the current pen colour.
    virtual void drawText(const TextRun& run) = 0;

    virtual void drawLine(const core::Vec2& from, const core::Vec2& to) = 0;
    virtual void drawPolyline(std::span<const core::Vec2> points, bool closed) = 0;
};

}  // namespace tg::render
