// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "qt_painter.hpp"

#include "tg/core/text_metrics.hpp"

#include <QImage>
#include <QPainter>
#include <QRect>
#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <numbers>
#include <string>

namespace tg::app {
namespace {

QImage renderText(const render::TextRun& run) {
    QImage image(1200, 600, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    QtPainter backend(painter);
    backend.setPen({40, 120, 200, 255}, 0.0, render::LineStyle::Solid);
    backend.drawText(run);
    return image;
}

QRect inkBounds(const QImage& image) {
    QRect bounds;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            // Ignore the faint antialiasing fringe when measuring outlines.
            if (qAlpha(image.pixel(x, y)) >= 128) {
                bounds = bounds.united(QRect(x, y, 1, 1));
            }
        }
    }
    return bounds;
}

TEST(QtPainterText, CapitalHeightAndAdvanceAgreeWithCoreWithinOnePixel) {
    const render::TextRun single{U"H", {50.0, 250.0}, 100.0};
    const QRect capital = inkBounds(renderText(single));
    ASSERT_FALSE(capital.isEmpty());
    EXPECT_NEAR(capital.height(), 100.0, 1.0);
    EXPECT_NEAR(capital.bottom() + 1, single.origin.y, 1.0);
    for (const double widthFactor : {0.6, 1.0, 1.7}) {
        const render::TextRun run{U"HAV H", single.origin, 100.0, 0.0, widthFactor};
        const QRect line = inkBounds(renderText(run));
        render::TextRun last = single;
        last.widthFactor = widthFactor;
        const QRect lastInk = inkBounds(renderText(last));
        // Advances include side bearings, so measure the last H relative to
        // an isolated H rather than mistaking ink width for advance width.
        const double advance = core::textWidth(U"HAV ") * 100.0 * widthFactor;
        EXPECT_NEAR(line.right() - lastInk.right(), advance, 1.0);
        EXPECT_NEAR(line.height(), 100.0, 1.0);
    }
}

TEST(QtPainterText, PositiveQuarterTurnIsCounterClockwiseAroundTheBaselineOrigin) {
    const render::TextRun horizontal{U"HHHH", {500.0, 500.0}, 100.0};
    render::TextRun vertical = horizontal;
    vertical.angle = std::numbers::pi / 2.0;
    const QRect before = inkBounds(renderText(horizontal));
    const QRect after = inkBounds(renderText(vertical));
    ASSERT_FALSE(after.isEmpty());
    EXPECT_NEAR(after.width(), before.height(), 1.0);
    EXPECT_NEAR(after.height(), before.width(), 1.0);
    EXPECT_NEAR(after.left(), horizontal.origin.x - 100.0, 1.0);
    EXPECT_LT(after.bottom(), horizontal.origin.y);
}

TEST(QtPainterText, MissingCharactersUseTheSameNotdefGlyphWithoutSystemFallback) {
    const render::TextRun missing{U"\U0001F600", {50.0, 250.0}, 100.0};
    render::TextRun unassigned = missing;
    unassigned.text = U"\U0010FFFF";
    const QImage first = renderText(missing);
    ASSERT_FALSE(inkBounds(first).isEmpty());
    EXPECT_EQ(first, renderText(unassigned));
}

TEST(QtPainterText, DiameterCharactersUseTheCoreSubstitution) {
    const render::TextRun diameter{U"\u2300\u2205", {50.0, 250.0}, 100.0};
    render::TextRun replacement = diameter;
    replacement.text = U"\u00D8\u00D8";
    EXPECT_EQ(renderText(diameter), renderText(replacement));
}

TEST(QtPainterText, CharactersArePositionedSeparatelyWithoutKerningOrLigatures) {
    const render::TextRun run{U"AVffi\u0301\u03A9", {50.0, 250.0}, 100.0};
    QImage separate(1200, 600, QImage::Format_ARGB32_Premultiplied);
    separate.fill(Qt::transparent);
    QPainter painter(&separate);
    QtPainter backend(painter);
    backend.setPen({40, 120, 200, 255}, 0.0, render::LineStyle::Solid);
    double advance = 0.0;
    for (const auto code : run.text) {
        auto character = run;
        character.text = std::u32string(1, code);
        character.origin.x += advance * run.capHeightPx;
        backend.drawText(character);
        advance += core::advanceWidth(code);
    }
    painter.end();
    const QImage together = renderText(run);
    // Glyph offsets and translated origins can quantize to different
    // subpixel phases. Require the same ink within one device pixel.
    for (const bool reverse : {false, true}) {
        const auto& source = reverse ? separate : together;
        const auto& target = reverse ? together : separate;
        int unmatched = 0;
        const QRect bounds = inkBounds(source);
        for (int y = bounds.top(); y <= bounds.bottom(); ++y) {
            for (int x = bounds.left(); x <= bounds.right(); ++x) {
                if (qAlpha(source.pixel(x, y)) < 128) {
                    continue;
                }
                bool found = false;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        found = found || qAlpha(target.pixel(x + dx, y + dy)) >= 128;
                    }
                }
                if (!found) {
                    ++unmatched;
                }
            }
        }
        EXPECT_EQ(unmatched, 0);
    }
}

TEST(QtPainterText, TinyTextUsesOnlyItsRotatedBoundingRectangle) {
    const render::TextRun run{U"HHHHHHHHHHHHHHHHHHHH", {50.0, 250.0}, 1.5, 0.4, 2.0};
    QImage expected(1200, 600, QImage::Format_ARGB32_Premultiplied);
    expected.fill(Qt::transparent);
    QPainter painter(&expected);
    QtPainter backend(painter);
    backend.setPen({40, 120, 200, 255}, 0.0, render::LineStyle::Solid);
    const auto& metrics = core::fontMetrics();
    painter.translate(run.origin.x, run.origin.y);
    painter.rotate(-run.angle * 180.0 / std::numbers::pi);
    painter.drawRect(QRectF(0.0, -metrics.ascent * run.capHeightPx,
                            core::textWidth(run.text) * run.capHeightPx * run.widthFactor,
                            (metrics.ascent - metrics.descent) * run.capHeightPx));
    painter.end();
    EXPECT_EQ(renderText(run), expected);
}

TEST(QtPainterText, RestoresPainterStateAndUsesCurrentPenColour) {
    QImage image(1200, 600, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.translate(3.0, 7.0);
    painter.setBrush(Qt::green);
    QtPainter backend(painter);
    backend.setPen({255, 0, 0, 255}, 0.0, render::LineStyle::Solid);
    const auto transform = painter.transform();
    const auto pen = painter.pen();
    const auto brush = painter.brush();
    const auto hints = painter.renderHints();
    backend.drawText({U"H", {200.0, 250.0}, 100.0, 0.3, 1.5});
    EXPECT_EQ(painter.transform(), transform);
    EXPECT_EQ(painter.pen(), pen);
    EXPECT_EQ(painter.brush(), brush);
    EXPECT_EQ(painter.renderHints(), hints);
    painter.end();
    const QRect ink = inkBounds(image);
    ASSERT_FALSE(ink.isEmpty());
    bool foundRed = false;
    for (int y = ink.top(); y <= ink.bottom(); ++y) {
        for (int x = ink.left(); x <= ink.right(); ++x) {
            const auto color = image.pixelColor(x, y);
            if (color.alpha() > 128) {
                EXPECT_EQ(color.red(), 255);
                EXPECT_EQ(color.green(), 0);
                EXPECT_EQ(color.blue(), 0);
                foundRed = true;
            }
        }
    }
    EXPECT_TRUE(foundRed);
}

TEST(QtPainterText, EmptyOrInvalidRunsDrawNothing) {
    const auto empty = renderText({});
    EXPECT_TRUE(inkBounds(empty).isEmpty());
    const double nan = std::numeric_limits<double>::quiet_NaN();
    for (const double height : {0.0, -1.0, nan}) {
        EXPECT_EQ(renderText({U"H", {50.0, 250.0}, height}), empty);
    }
    EXPECT_EQ(renderText({U"H", {50.0, 250.0}, 100.0, nan}), empty);
    EXPECT_EQ(renderText({U"H", {50.0, 250.0}, 100.0, 0.0, 0.0}), empty);
}

}  // namespace
}  // namespace tg::app
