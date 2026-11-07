// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "style.hpp"

#include "theme.hpp"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QImage>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOption>
#include <QTableView>
#include <gtest/gtest.h>

#include <cstdlib>

namespace tg::app {
namespace {

QApplication& application() {
    return *qobject_cast<QApplication*>(QApplication::instance());
}

// Antialiasing and the conversions of the grab round colours by a few units.
constexpr int kRounding = 8;

bool near(const QColor& pixel, const QColor& colour) {
    return pixel.alpha() == 255 && std::abs(pixel.red() - colour.red()) <= kRounding &&
           std::abs(pixel.green() - colour.green()) <= kRounding &&
           std::abs(pixel.blue() - colour.blue()) <= kRounding;
}

// The colour at a point in widget coordinates of an image from grab().
QColor pixelAt(const QImage& image, QPoint point) {
    const qreal ratio = image.devicePixelRatio();
    return image.pixelColor(QPoint(qRound(point.x() * ratio), qRound(point.y() * ratio)));
}

bool anyPixelNear(const QImage& image, const QRect& area, const QColor& colour) {
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            if (near(pixelAt(image, QPoint(x, y)), colour)) {
                return true;
            }
        }
    }
    return false;
}

TEST(StyleTest, SheetContainsTheColoursOfTheTokens) {
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        const ThemeTokens& tokens = themeTokens(theme);
        const QString sheet = styleSheet(tokens);
        for (const QColor* colour : {&tokens.chrome, &tokens.ribbon, &tokens.group, &tokens.tabs,
                                     &tokens.panel, &tokens.field, &tokens.border, &tokens.text,
                                     &tokens.textSecondary, &tokens.accent, &tokens.selection}) {
            EXPECT_TRUE(sheet.contains(colour->name(QColor::HexRgb), Qt::CaseInsensitive))
                << colour->name().toStdString();
        }
        // No placeholder is left without a colour.
        EXPECT_FALSE(sheet.contains(QStringLiteral("{text")));
        EXPECT_FALSE(sheet.contains(QStringLiteral("{border}")));
    }
}

TEST(StyleTest, SheetAndStyleChangeWithTheTheme) {
    const QString light = styleSheet(themeTokens(Theme::Light));
    const QString dark = styleSheet(themeTokens(Theme::Dark));
    EXPECT_NE(light, dark);
    for (const Theme theme : {Theme::Light, Theme::Dark, Theme::Light}) {
        applyTheme(application(), theme);
        EXPECT_EQ(application().styleSheet(), theme == Theme::Dark ? dark : light);
        const Style* style = installedStyle();
        ASSERT_NE(style, nullptr);
        ASSERT_NE(style->baseStyle(), nullptr);
        EXPECT_EQ(style->baseStyle()->objectName(), QStringLiteral("fusion"));
        EXPECT_EQ(style->tokens(), themeTokens(theme));
    }
}

TEST(StyleTest, ApplyingTheSameThemeAgainKeepsTheStyle) {
    applyTheme(application(), Theme::Dark);
    const Style* style = installedStyle();
    ASSERT_NE(style, nullptr);
    applyTheme(application(), Theme::Dark);
    EXPECT_EQ(installedStyle(), style);
    applyTheme(application(), Theme::Light);
    EXPECT_EQ(installedStyle()->tokens(), themeTokens(Theme::Light));
}

TEST(StyleTest, ScrollBarsAreThinAndRowsAreDense) {
    applyTheme(application(), Theme::Light);
    const Style* style = installedStyle();
    ASSERT_NE(style, nullptr);
    const QScrollBar bar(Qt::Vertical);
    EXPECT_EQ(style->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, &bar), 10);
    const QTableView table;
    EXPECT_EQ(style->pixelMetric(QStyle::PM_HeaderDefaultSectionSizeVertical, nullptr, &table), 22);
}

TEST(StyleTest, ScrollBarHandleFollowsTheValueWithoutArrows) {
    applyTheme(application(), Theme::Light);
    const Style* style = installedStyle();
    ASSERT_NE(style, nullptr);
    QStyleOptionSlider option;
    option.rect = QRect(0, 0, 10, 200);
    option.orientation = Qt::Vertical;
    option.minimum = 0;
    option.maximum = 100;
    option.pageStep = 100;
    option.direction = Qt::LeftToRight;
    for (const auto line : {QStyle::SC_ScrollBarAddLine, QStyle::SC_ScrollBarSubLine}) {
        EXPECT_TRUE(style->subControlRect(QStyle::CC_ScrollBar, &option, line).isEmpty());
    }
    EXPECT_EQ(style->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarGroove),
              option.rect);
    // A page as long as the range: the handle is half the bar.
    option.sliderPosition = 0;
    EXPECT_EQ(style->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider),
              QRect(0, 0, 10, 100));
    option.sliderPosition = 100;
    EXPECT_EQ(style->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSlider),
              QRect(0, 100, 10, 100));
    EXPECT_EQ(style->subControlRect(QStyle::CC_ScrollBar, &option, QStyle::SC_ScrollBarSubPage),
              QRect(0, 0, 10, 100));
}

TEST(StyleTest, ComboArrowIsDrawnInTheTextColour) {
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        applyTheme(application(), theme);
        const Style* style = installedStyle();
        ASSERT_NE(style, nullptr);
        const QComboBox combo;
        QImage image(16, 16, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QStyleOption option;
        option.rect = image.rect();
        option.state = QStyle::State_Enabled;
        QPainter painter(&image);
        style->drawPrimitive(QStyle::PE_IndicatorArrowDown, &option, &painter, &combo);
        painter.end();
        // The most opaque pixel of the chevron has the colour of the text,
        // up to the rounding of antialiasing.
        QColor strongest(Qt::transparent);
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor pixel = image.pixelColor(x, y);
                if (pixel.alpha() > strongest.alpha()) {
                    strongest = pixel;
                }
            }
        }
        const bool found =
            strongest.alpha() > 128 && near(QColor(strongest.rgb()), themeTokens(theme).text);
        EXPECT_TRUE(found);
    }
    applyTheme(application(), Theme::Light);
}

// The application style is the style sheet style wrapping Style: real
// widgets must still reach the proxy and the tokens through it.
TEST(StyleTest, RealWidgetsReachTheProxyThroughTheStyleSheet) {
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        applyTheme(application(), theme);
        const ThemeTokens& tokens = themeTokens(theme);

        // A page as long as the range: the handle fills the upper half,
        // in the border colour, on the field colour, with no arrows.
        QScrollBar bar(Qt::Vertical);
        bar.setRange(0, 100);
        bar.setPageStep(100);
        bar.setValue(0);
        bar.resize(10, 200);
        const QImage barImage = bar.grab().toImage();
        EXPECT_TRUE(near(pixelAt(barImage, QPoint(5, 50)), tokens.border));
        EXPECT_TRUE(near(pixelAt(barImage, QPoint(5, 150)), tokens.field));
        EXPECT_TRUE(near(pixelAt(barImage, QPoint(5, 198)), tokens.field));

        // A combo box: field colour inside, border at the edge, and the
        // arrow in the text colour inside the drop-down area on the right.
        QComboBox combo;
        combo.addItem(QStringLiteral("A"));
        combo.resize(120, 24);
        const QImage comboImage = combo.grab().toImage();
        EXPECT_TRUE(near(pixelAt(comboImage, QPoint(60, 12)), tokens.field));
        EXPECT_TRUE(near(pixelAt(comboImage, QPoint(60, 0)), tokens.border));
        EXPECT_TRUE(anyPixelNear(comboImage, QRect(100, 2, 18, 20), tokens.text));
    }
    applyTheme(application(), Theme::Light);
}

}  // namespace
}  // namespace tg::app
