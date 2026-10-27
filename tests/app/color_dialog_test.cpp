// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "color_dialog.hpp"

#include "property_widgets.hpp"

#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <gtest/gtest.h>

namespace tg::app {
namespace {
TEST(ColorDialog, IndexGridAndNumberChooseAnAci) {
    ColorDialog dialog(core::Color::fromIndex(12), ColorDialog::Allow::All);
    EXPECT_EQ(dialog.color(), core::Color::fromIndex(12));
    for (int index = 1; index <= 255; ++index) {
        auto* button = dialog.findChild<QPushButton*>(QStringLiteral("aci%1").arg(index));
        ASSERT_NE(button, nullptr);
        button->click();
        EXPECT_EQ(dialog.color(), core::Color::fromIndex(index));
    }
    dialog.findChild<QSpinBox*>(QStringLiteral("colourIndex"))->setValue(42);
    EXPECT_EQ(dialog.color(), core::Color::fromIndex(42));
    dialog.accept();
    EXPECT_EQ(dialog.result(), QDialog::Accepted);
}

TEST(ColorDialog, RgbChannelsAndNearestIndex) {
    const core::Rgb initial{13, 57, 201};
    const ColorDialog dialog(core::Color::fromRgb(initial), ColorDialog::Allow::All);
    EXPECT_EQ(dialog.color(), core::Color::fromRgb(initial));
    EXPECT_EQ(dialog.findChild<QTabWidget*>()->currentIndex(), 1);
    dialog.findChild<QSpinBox*>(QStringLiteral("rgb0"))->setValue(255);
    dialog.findChild<QSpinBox*>(QStringLiteral("rgb1"))->setValue(128);
    dialog.findChild<QSpinBox*>(QStringLiteral("rgb2"))->setValue(0);
    const core::Rgb chosen{255, 128, 0};
    EXPECT_EQ(dialog.color(), core::Color::fromRgb(chosen));
    EXPECT_EQ(dialog.findChild<QLabel*>(QStringLiteral("nearestAci"))->text(),
              QString::number(core::nearestAci(chosen)));
    EXPECT_FALSE(dialog.findChild<QLabel*>(QStringLiteral("colourPreview"))->pixmap().isNull());
}

TEST(ColorDialog, InheritedColoursAreOfferedOnlyWhenAllowed) {
    const ColorDialog dialog(core::Color::byBlock(), ColorDialog::Allow::All);
    EXPECT_EQ(dialog.color(), core::Color::byBlock());
    dialog.findChild<QPushButton*>(QStringLiteral("byLayer"))->click();
    EXPECT_EQ(dialog.color(), core::Color::byLayer());
    dialog.findChild<QPushButton*>(QStringLiteral("byBlock"))->click();
    EXPECT_EQ(dialog.color(), core::Color::byBlock());
    const ColorDialog restricted(core::Color::byLayer(), ColorDialog::Allow::IndexAndRgb);
    EXPECT_EQ(restricted.findChild<QPushButton*>(QStringLiteral("byLayer")), nullptr);
    EXPECT_EQ(restricted.findChild<QPushButton*>(QStringLiteral("byBlock")), nullptr);
    EXPECT_EQ(restricted.color(), core::Color::fromIndex(7));
}

TEST(ColorDialog, SwitchingTabsSelectsTheCorrespondingColourRepresentation) {
    const ColorDialog dialog(core::Color::fromIndex(1), ColorDialog::Allow::All);
    auto* tabs = dialog.findChild<QTabWidget*>();
    tabs->setCurrentIndex(1);
    EXPECT_EQ(dialog.color(), core::Color::fromRgb(core::aciToRgb(1)));
    tabs->setCurrentIndex(0);
    EXPECT_EQ(dialog.color(), core::Color::fromIndex(1));
}

TEST(ColorDialog, IndexSevenSwatchContainsWhiteAndBlackOnEitherPalette) {
    for (const auto group : {QPalette::Active, QPalette::Disabled}) {
        QPalette palette;
        palette.setCurrentColorGroup(group);
        const auto image = colorIcon(core::Color::fromIndex(7), palette).pixmap(64, 18).toImage();
        EXPECT_EQ(image.pixelColor(8, 8), QColor(Qt::white));
        EXPECT_EQ(image.pixelColor(48, 8), QColor(Qt::black));
    }
}
}  // namespace
}  // namespace tg::app
