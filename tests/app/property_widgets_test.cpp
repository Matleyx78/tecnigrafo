// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "property_widgets.hpp"

#include "color_dialog.hpp"

#include <QSignalSpy>
#include <QTimer>
#include <gtest/gtest.h>

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
    QSignalSpy spy(&combo, &Combo::valueChosen);
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
    QSignalSpy spy(&combo, &ColorCombo::valueChosen);
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
    QSignalSpy spy(&combo, &LayerCombo::valueChosen);
    combo.setDocument(&document);
    EXPECT_TRUE(spy.isEmpty());
    EXPECT_EQ(combo.count(), static_cast<int>(document.layers().size()));
    const int index = combo.findData(QVariant::fromValue(id));
    ASSERT_GE(index, 0);
    EXPECT_TRUE(combo.itemText(index).contains(QStringLiteral("Off")));
    EXPECT_TRUE(combo.itemText(index).contains(QStringLiteral("Frozen")));
    EXPECT_TRUE(combo.itemText(index).contains(QStringLiteral("Locked")));
    EXPECT_TRUE(combo.itemText(index).contains(QStringLiteral("No plot")));
    EXPECT_FALSE(combo.itemIcon(index).isNull());
    checkSelection(combo, id);
    EXPECT_EQ(document.current(), current);
    combo.setDocument(nullptr);
    EXPECT_EQ(combo.count(), 0);
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
