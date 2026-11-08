// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "property_widgets.hpp"

#include "color_dialog.hpp"
#include "layer_table_model.hpp"
#include "theme.hpp"

#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QTimer>
#include <gtest/gtest.h>

#include <string>
#include <type_traits>

namespace tg::app {
namespace {
void activate(QComboBox& combo, int index) {
    combo.setCurrentIndex(index);
    ASSERT_TRUE(
        QMetaObject::invokeMethod(&combo, "activated", Qt::DirectConnection, Q_ARG(int, index)));
}

template <typename Value>
Value storedValue(const QVariant& data) {
    if constexpr (std::is_same_v<Value, core::Color>) {
        return *static_cast<const core::Color*>(data.constData());
    } else {
        return data.value<Value>();
    }
}

template <typename Combo, typename Value>
void checkSelection(Combo& combo, Value value) {
    const QSignalSpy spy(&combo, &Combo::valueChosen);
    combo.setValue(value);
    EXPECT_TRUE(spy.isEmpty());
    EXPECT_EQ(storedValue<Value>(combo.currentData()), value);
    const int count = combo.count();
    combo.setVaries();
    combo.setVaries();
    EXPECT_EQ(combo.count(), count + 1);
    EXPECT_FALSE(combo.currentData().isValid());
    EXPECT_TRUE(spy.isEmpty());
    const int index = combo.findData(QVariant::fromValue(value));
    ASSERT_GE(index, 0);
    activate(combo, index);
    ASSERT_EQ(spy.size(), 1);
    EXPECT_EQ(storedValue<Value>(spy.at(0).at(0)), value);
    EXPECT_EQ(combo.count(), count);
    EXPECT_EQ(storedValue<Value>(combo.currentData()), value);
    combo.setVaries();
    combo.setValue(value);
    EXPECT_EQ(combo.count(), count);
    EXPECT_EQ(spy.size(), 1);
}

TEST(PropertyWidgets, ColoursNamesRecentsAndMixedSelection) {
    const auto document = core::Document::newDrawing();
    ColorCombo combo;
    combo.setDocument(&document);
    EXPECT_EQ(storedValue<core::Color>(combo.currentData()), document.current().color);
    EXPECT_EQ(combo.itemText(2), QStringLiteral("Red"));
    EXPECT_EQ(combo.itemText(8), QStringLiteral("White"));
    checkSelection(combo, core::Color::byLayer());
    checkSelection(combo, core::Color::byBlock());
    checkSelection(combo, core::Color::fromIndex(3));
    checkSelection(combo, core::Color::fromIndex(200));
    checkSelection(combo, core::Color::fromRgb({12, 34, 56}));
    const int count = combo.count();
    combo.setValue(core::Color::fromRgb({12, 34, 56}));
    EXPECT_EQ(combo.count(), count);
    EXPECT_EQ(document.current().color, core::Color::byLayer());
}

TEST(PropertyWidgets, ColourDialogAcceptAndCancel) {
    ColorCombo combo;
    const QSignalSpy spy(&combo, &ColorCombo::valueChosen);
    combo.setVaries();
    QTimer::singleShot(0, &combo, [&combo] {
        auto* dialog = combo.findChild<ColorDialog*>();
        ASSERT_NE(dialog, nullptr);
        dialog->reject();
    });
    activate(combo, combo.count() - 1);
    EXPECT_TRUE(spy.isEmpty());
    EXPECT_FALSE(combo.currentData().isValid());
    QTimer::singleShot(0, &combo, [&combo] {
        auto* dialog = combo.findChild<ColorDialog*>();
        ASSERT_NE(dialog, nullptr);
        dialog->accept();
    });
    activate(combo, combo.count() - 1);
    ASSERT_EQ(spy.size(), 1);
    EXPECT_EQ(storedValue<core::Color>(spy.at(0).at(0)), core::Color::byLayer());
}

TEST(PropertyWidgets, LayersShowAllStatesAndUseStableIds) {
    auto document = core::Document::newDrawing();
    core::Layer layer;
    layer.name = "Hidden locked";
    layer.color = core::Color::fromRgb({1, 2, 3});
    layer.linetype = document.continuousLinetype();
    layer.on = false;
    layer.frozen = true;
    layer.locked = true;
    layer.plot = false;
    const auto id = document.addLayer(layer);
    const auto current = document.current();
    LayerCombo combo;
    const QSignalSpy spy(&combo, &LayerCombo::valueChosen);
    combo.setDocument(&document);
    EXPECT_TRUE(spy.isEmpty());
    EXPECT_EQ(combo.count(), static_cast<int>(document.layers().size()));
    const int index = combo.findData(QVariant::fromValue(id));
    ASSERT_GE(index, 0);
    EXPECT_EQ(combo.itemText(index), QStringLiteral("Hidden locked"));
    const QString description = QStringLiteral("Hidden locked [Off, Frozen, Locked, No plot]");
    EXPECT_EQ(combo.itemData(index, Qt::ToolTipRole).toString(), description);
    EXPECT_EQ(combo.itemData(index, Qt::AccessibleTextRole).toString(), description);
    combo.setValue(id);
    EXPECT_EQ(combo.toolTip(), description);
    EXPECT_EQ(combo.accessibleName(), description);
    EXPECT_FALSE(combo.itemIcon(index).isNull());
    checkSelection(combo, id);
    EXPECT_EQ(document.current(), current);
    combo.setDocument(nullptr);
    EXPECT_EQ(combo.count(), 0);
}

TEST(PropertyWidgets, LayerIconsShowEveryStateCombinationInBothThemes) {
    auto document = core::Document::newDrawing();
    for (int states = 0; states < 8; ++states) {
        core::Layer layer;
        layer.name = "States" + std::to_string(states);
        layer.linetype = document.continuousLinetype();
        layer.on = (states & 1) != 0;
        layer.frozen = (states & 2) != 0;
        layer.locked = (states & 4) != 0;
        document.addLayer(layer);
    }
    LayerCombo combo;
    combo.setDocument(&document);
    const QSignalSpy spy(&combo, &LayerCombo::valueChosen);
    for (const Theme theme : {Theme::Light, Theme::Dark}) {
        QPalette palette = combo.palette();
        palette.setColor(QPalette::Window, themeTokens(theme).panel);
        palette.setColor(QPalette::Text, themeTokens(theme).text);
        combo.setPalette(palette);
        for (const auto& layer : document.layers()) {
            const int index = combo.findData(QVariant::fromValue(layer.id));
            ASSERT_GE(index, 0);
            const QImage actual = combo.itemIcon(index).pixmap(combo.iconSize(), 1.0).toImage();
            int x = 0;
            for (const LayerState state : {LayerState::On, LayerState::Frozen, LayerState::Locked}) {
                QPixmap expected(16, 16);
                expected.fill(Qt::transparent);
                QPainter painter(&expected);
                themedIcon(layerStateIconName(state, layerStateValue(layer, state)), theme)
                    .paint(&painter, QRect(0, 0, 16, 16));
                painter.end();
                EXPECT_EQ(actual.copy(x, 0, 16, 16), expected.toImage());
                x += 18;
            }
        }
    }
    EXPECT_TRUE(spy.isEmpty());
}

TEST(PropertyWidgets, LinetypesIncludeLoadedPatternsAndInheritedTypes) {
    auto document = core::Document::newDrawing();
    core::Linetype linetype;
    linetype.name = "Test pattern";
    linetype.pattern.elements = {2.0, -1.0, 0.0, -1.0};
    const auto id = document.addLinetype(linetype);
    const auto current = document.current();
    LinetypeCombo combo;
    combo.setDocument(&document);
    EXPECT_EQ(combo.count(), static_cast<int>(document.linetypes().size()));
    EXPECT_EQ(combo.currentData().value<core::LinetypeId>(), current.linetype);
    checkSelection(combo, id);
    checkSelection(combo, document.byLayerLinetype());
    checkSelection(combo, document.byBlockLinetype());
    const int index = combo.findData(QVariant::fromValue(id));
    EXPECT_FALSE(combo.itemIcon(index).isNull());
    EXPECT_EQ(document.current(), current);
    combo.setDocument(nullptr);
    EXPECT_EQ(combo.count(), 0);
}

TEST(PropertyWidgets, LineweightsContainEveryAllowedValueAndDefault) {
    const auto document = core::Document::newDrawing();
    LineweightCombo combo;
    combo.setDocument(&document);
    EXPECT_EQ(combo.currentData().toInt(), document.current().lineweight);
    EXPECT_EQ(combo.count(), static_cast<int>(core::lineweight::kAllowed.size()) + 3);
    checkSelection(combo, core::lineweight::kByLayer);
    checkSelection(combo, core::lineweight::kByBlock);
    checkSelection(combo, core::lineweight::kDefault);
    for (const int weight : core::lineweight::kAllowed) {
        checkSelection(combo, weight);
        EXPECT_FALSE(combo.itemIcon(combo.currentIndex()).isNull());
    }
    EXPECT_EQ(document.current().lineweight, core::lineweight::kByLayer);
}
}  // namespace
}  // namespace tg::app
