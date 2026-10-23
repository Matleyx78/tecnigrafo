// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/render/scene_renderer.hpp"

#include "tg/core/text.hpp"

#include <gtest/gtest.h>

#include <array>
#include <numbers>
#include <vector>

namespace tg::render {
namespace {

class RecordingPainter final : public Painter {
public:
    std::vector<TextRun> runs;
    std::vector<std::vector<core::Vec2>> outlines;
    std::vector<bool> closed;

    void clear(const Color& /*background*/) override {}

    void setPen(const Color& /*color*/, double /*widthPx*/, LineStyle /*style*/) override {}

    void fillPolygon(std::span<const core::Vec2> /*points*/, const Color& /*color*/) override {}

    void drawText(const TextRun& run) override { runs.push_back(run); }

    void drawLine(const core::Vec2& /*from*/, const core::Vec2& /*to*/) override {}

    void drawPolyline(std::span<const core::Vec2> points, bool isClosed) override {
        outlines.emplace_back(points.begin(), points.end());
        closed.push_back(isClosed);
    }
};

Viewport textViewport() {
    Viewport viewport;
    viewport.setScreenSize(1000.0, 800.0);
    viewport.setCenter({10.0, 20.0});
    viewport.setScale(3.0);
    return viewport;
}

void expectPoint(const core::Vec2& actual, const core::Vec2& expected) {
    constexpr double kTolerance = 1e-10;
    EXPECT_NEAR(actual.x, expected.x, kTolerance);
    EXPECT_NEAR(actual.y, expected.y, kTolerance);
}

TEST(SceneRendererText, UsesTextFrameForEveryAlignmentAndTransformation) {
    const auto viewport = textViewport();
    core::Text base;
    base.position = {5.0, 7.0};
    base.alignmentPoint = {40.0, 30.0};
    base.height = 4.0;
    base.rotation = 0.4;
    base.widthFactor = 1.3;
    base.value = "H%%d";
    std::array<core::Text, 7> texts;
    texts.fill(base);
    texts.at(1).horizontal = core::TextHorizontal::Center;
    texts.at(1).vertical = core::TextVertical::Middle;
    texts.at(2).horizontal = core::TextHorizontal::Aligned;
    texts.at(3).horizontal = core::TextHorizontal::Fit;
    texts.at(4).obliqueAngle = std::numbers::pi / 6.0;
    texts.at(5).mirroredX = true;
    texts.at(6).mirroredY = true;
    for (const auto& text : texts) {
        RecordingPainter painter;
        const std::array<core::Entity, 1> entities{text};
        renderPreview(entities, viewport, {}, painter);
        ASSERT_EQ(painter.runs.size(), 1U);
        EXPECT_TRUE(painter.outlines.empty());
        const auto frame = core::textFrame(text);
        const auto& run = painter.runs.front();
        expectPoint(run.origin, viewport.worldToScreen(frame.origin));
        EXPECT_DOUBLE_EQ(run.capHeightPx, frame.height * viewport.scale());
        EXPECT_DOUBLE_EQ(run.angle, frame.rotation);
        EXPECT_DOUBLE_EQ(run.widthFactor, frame.widthFactor);
        EXPECT_DOUBLE_EQ(run.oblique, text.obliqueAngle);
        EXPECT_EQ(run.backward, text.mirroredX);
        EXPECT_EQ(run.upsideDown, text.mirroredY);
        EXPECT_EQ(run.text, U"H\u00b0");
    }
}

TEST(SceneRendererText, TinyTextUsesTransformedBoxAndDerivedHeight) {
    const auto viewport = textViewport();
    core::Text text;
    text.value = "HHHH";
    text.height = 10.0;
    text.position = {10.0, 20.0};
    // Aligned derives a tiny height despite the large stored height.
    text.alignmentPoint = {10.1, 20.1};
    text.horizontal = core::TextHorizontal::Aligned;
    text.obliqueAngle = 0.5;
    text.mirroredX = true;
    text.mirroredY = true;
    RecordingPainter painter;
    const std::array<core::Entity, 1> entities{text};
    renderPreview(entities, viewport, {}, painter);
    EXPECT_TRUE(painter.runs.empty());
    ASSERT_EQ(painter.outlines.size(), 1U);
    ASSERT_TRUE(painter.closed.front());
    const auto box = core::textBox(text);
    ASSERT_EQ(painter.outlines.front().size(), box.size());
    for (std::size_t i = 0; i < box.size(); ++i) {
        expectPoint(painter.outlines.front().at(i), viewport.worldToScreen(box.at(i)));
    }
}

}  // namespace
}  // namespace tg::render
