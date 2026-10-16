// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/render/painter.hpp"

#include <QPolygonF>

class QPainter;

namespace tg::app {

/// render::Painter backend implemented with QPainter.
class QtPainter final : public render::Painter {
public:
    explicit QtPainter(QPainter& painter);

    void clear(const render::Color& background) override;
    void setPen(const render::Color& color, double widthPx) override;
    void drawLine(const core::Vec2& from, const core::Vec2& to) override;
    void drawPolyline(std::span<const core::Vec2> points, bool closed) override;

private:
    QPainter& m_painter;
    QPolygonF m_buffer;  // reused between calls to avoid allocations
};

}  // namespace tg::app
