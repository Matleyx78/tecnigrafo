// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "quick_select_dialog.hpp"

#include "property_widgets.hpp"
#include "tg/cmd/quick_select.hpp"
#include "tg/edit/editor.hpp"

#include <QComboBox>
#include <QMetaObject>
#include <QRadioButton>
#include <QString>
#include <QVariant>
#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tg::app {
namespace {

using core::EntityId;
using Property = cmd::QuickSelectCriteria::Property;
using Operator = cmd::QuickSelectCriteria::Operator;

const core::Color kRed = core::Color::fromIndex(1);

void activate(QComboBox& combo, int index) {
    ASSERT_GE(index, 0);
    combo.setCurrentIndex(index);
    ASSERT_TRUE(
        QMetaObject::invokeMethod(&combo, "activated", Qt::DirectConnection, Q_ARG(int, index)));
}

template <typename Value>
void choose(QComboBox& combo, Value value) {
    activate(combo, combo.findData(QVariant::fromValue(value)));
}

/// A drawing with two layers: on 0 a red line and a circle, on Walls a line
/// with lineweight 0.50 and the linetype Continuous, and a circle.
class QuickSelectDialogTest : public testing::Test {
protected:
    QuickSelectDialogTest() : m_document(core::Document::newDrawing()), m_editor(m_document) {
        core::Layer layer;
        layer.name = "Walls";
        layer.linetype = m_document.continuousLinetype();
        m_editor.beginTransaction("Test");
        m_walls = m_editor.addLayer(layer).value_or(core::LayerId{});
        m_redLine = m_editor.add(core::Line{{0.0, 0.0}, {10.0, 0.0}});
        m_circle = m_editor.add(core::Circle{{20.0, 0.0}, 2.0});
        m_wallLine = m_editor.add(core::Line{{0.0, 10.0}, {10.0, 10.0}});
        m_wallCircle = m_editor.add(core::Circle{{20.0, 10.0}, 2.0});
        core::EntityProperties red = m_document.find(m_redLine)->properties;
        red.color = kRed;
        EXPECT_TRUE(m_editor.setProperties(m_redLine, red));
        core::EntityProperties wall = m_document.find(m_wallLine)->properties;
        wall.layer = m_walls;
        wall.lineweight = 50;
        wall.linetype = m_document.continuousLinetype();
        EXPECT_TRUE(m_editor.setProperties(m_wallLine, wall));
        core::EntityProperties onWalls = m_document.find(m_wallCircle)->properties;
        onWalls.layer = m_walls;
        EXPECT_TRUE(m_editor.setProperties(m_wallCircle, onWalls));
        m_editor.commit();
    }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    template <typename Widget>
    [[nodiscard]] static Widget* child(const QuickSelectDialog& dialog, const char* name) {
        auto* found = dialog.findChild<Widget*>(QString::fromLatin1(name));
        EXPECT_NE(found, nullptr) << name;
        return found;
    }

    static void chooseProperty(const QuickSelectDialog& dialog, Property property) {
        auto* combo = child<QComboBox>(dialog, "quickSelectProperty");
        activate(*combo, combo->findData(static_cast<int>(property)));
    }

    static void chooseType(const QuickSelectDialog& dialog, const char* type) {
        auto* combo = child<QComboBox>(dialog, "quickSelectType");
        activate(*combo, combo->findData(QString::fromLatin1(type)));
    }

    [[nodiscard]] core::LayerId walls() const { return m_walls; }

    [[nodiscard]] EntityId redLine() const { return m_redLine; }

    [[nodiscard]] EntityId circle() const { return m_circle; }

    [[nodiscard]] EntityId wallLine() const { return m_wallLine; }

    [[nodiscard]] EntityId wallCircle() const { return m_wallCircle; }

private:
    core::Document m_document;
    edit::Editor m_editor;
    core::LayerId m_walls;
    EntityId m_redLine;
    EntityId m_circle;
    EntityId m_wallLine;
    EntityId m_wallCircle;
};

TEST_F(QuickSelectDialogTest, OneCriterionForEachProperty) {
    const QuickSelectDialog dialog(document(), {});
    EXPECT_EQ(dialog.objectName(), QStringLiteral("quickSelectDialog"));
    // Without a selection only the whole drawing can be searched.
    EXPECT_EQ(child<QComboBox>(dialog, "quickSelectWhere")->count(), 1);
    EXPECT_FALSE(dialog.searchesSelection());
    EXPECT_TRUE(dialog.within().empty());

    // Colour: the control of the colours chooses the value.
    chooseProperty(dialog, Property::Color);
    choose(*child<ColorCombo>(dialog, "quickSelectColor"), kRed);
    EXPECT_EQ(dialog.criteria().property, Property::Color);
    EXPECT_EQ(dialog.criteria().value, "1");
    EXPECT_EQ(dialog.result(), std::vector<EntityId>{redLine()});

    chooseProperty(dialog, Property::Layer);
    choose(*child<LayerCombo>(dialog, "quickSelectLayer"), walls());
    EXPECT_EQ(dialog.criteria().value, "Walls");
    EXPECT_EQ(dialog.result(), (std::vector<EntityId>{wallLine(), wallCircle()}));

    chooseProperty(dialog, Property::Linetype);
    choose(*child<LinetypeCombo>(dialog, "quickSelectLinetype"), document().continuousLinetype());
    EXPECT_EQ(dialog.result(), std::vector<EntityId>{wallLine()});

    chooseProperty(dialog, Property::Lineweight);
    choose(*child<LineweightCombo>(dialog, "quickSelectLineweight"), 50);
    EXPECT_EQ(dialog.result(), std::vector<EntityId>{wallLine()});

    // The operator ≠ and the type of object.
    auto* op = child<QComboBox>(dialog, "quickSelectOperator");
    activate(*op, op->findData(static_cast<int>(Operator::NotEqual)));
    EXPECT_EQ(dialog.criteria().op, Operator::NotEqual);
    EXPECT_EQ(dialog.result(), (std::vector<EntityId>{redLine(), circle(), wallCircle()}));
    chooseType(dialog, "CIRCLE");
    EXPECT_EQ(dialog.criteria().type, std::optional<std::string>("CIRCLE"));
    EXPECT_EQ(dialog.result(), (std::vector<EntityId>{circle(), wallCircle()}));

    // By type alone: no property, so no operator and no value.
    chooseProperty(dialog, Property::None);
    EXPECT_FALSE(op->isEnabled());
    EXPECT_EQ(dialog.criteria().op, Operator::Equal);
    chooseType(dialog, "LINE");
    EXPECT_EQ(dialog.result(), (std::vector<EntityId>{redLine(), wallLine()}));
    chooseProperty(dialog, Property::Layer);
    EXPECT_TRUE(op->isEnabled());
}

TEST_F(QuickSelectDialogTest, ExcludeTakesTheComplement) {
    const QuickSelectDialog dialog(document(), {});
    chooseProperty(dialog, Property::Layer);
    choose(*child<LayerCombo>(dialog, "quickSelectLayer"), walls());
    child<QRadioButton>(dialog, "quickSelectExclude")->setChecked(true);
    EXPECT_TRUE(dialog.criteria().exclude);
    EXPECT_EQ(dialog.result(), (std::vector<EntityId>{redLine(), circle()}));
    child<QRadioButton>(dialog, "quickSelectInclude")->setChecked(true);
    EXPECT_FALSE(dialog.criteria().exclude);
    EXPECT_EQ(dialog.result(), (std::vector<EntityId>{wallLine(), wallCircle()}));
}

TEST_F(QuickSelectDialogTest, SearchesWithinTheSelection) {
    const QuickSelectDialog dialog(document(), {redLine(), wallCircle()});
    // The selection is offered, and taken by default.
    auto* where = child<QComboBox>(dialog, "quickSelectWhere");
    EXPECT_EQ(where->count(), 2);
    EXPECT_TRUE(dialog.searchesSelection());
    EXPECT_EQ(dialog.within(), (std::vector<EntityId>{redLine(), wallCircle()}));

    chooseProperty(dialog, Property::None);
    chooseType(dialog, "CIRCLE");
    EXPECT_EQ(dialog.result(), std::vector<EntityId>{wallCircle()});
    child<QRadioButton>(dialog, "quickSelectExclude")->setChecked(true);
    EXPECT_EQ(dialog.result(), std::vector<EntityId>{redLine()});

    // The whole drawing.
    activate(*where, 0);
    EXPECT_FALSE(dialog.searchesSelection());
    EXPECT_EQ(dialog.result(), (std::vector<EntityId>{redLine(), wallLine()}));
}

}  // namespace
}  // namespace tg::app
