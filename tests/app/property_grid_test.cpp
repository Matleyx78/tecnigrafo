// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "property_grid.hpp"

#include "property_widgets.hpp"

#include <QAbstractItemModel>
#include <QIcon>
#include <QLineEdit>
#include <QMetaObject>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPalette>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QStyleOptionViewItem>
#include <QWidget>
#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <utility>

namespace tg::app {
namespace {

using Model = PropertyGridModel;
using Rows = QList<QPair<QString, QString>>;

/// The model on a new drawing of its own.
class PropertyGridTest : public testing::Test {
protected:
    PropertyGridTest() : m_document(core::Document::newDrawing()), m_model(m_document) {
        m_model.setPalette(QPalette());
    }

    [[nodiscard]] Model& model() { return m_model; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    /// The texts of the column `which` of the rows of `group`.
    [[nodiscard]] QStringList column(int group, int which) {
        QStringList texts;
        const QModelIndex parent = m_model.groupIndex(group);
        for (int row = 0; row < m_model.rowCount(parent); ++row) {
            texts.append(m_model.index(row, which, parent).data().toString());
        }
        return texts;
    }

    [[nodiscard]] QString value(PropertyField field) {
        return m_model.valueIndex(field).data().toString();
    }

    /// The value of Current Vertex in Geometry.
    [[nodiscard]] QModelIndex vertexValue() {
        const QModelIndex group = m_model.groupIndex(Model::kGeometryGroup);
        for (int row = 0; row < m_model.rowCount(group); ++row) {
            const QModelIndex value = m_model.index(row, Model::kValueColumn, group);
            if (value.data(Model::kPreviousVertexRole).isValid()) {
                return value;
            }
        }
        return {};
    }

private:
    core::Document m_document;
    Model m_model;
};

core::EntityRecord record(std::uint64_t id, core::Entity entity) {
    core::EntityRecord result;
    result.id = core::EntityId{id};
    result.entity = std::move(entity);
    return result;
}

core::Polyline polyline(std::initializer_list<core::Vec2> points) {
    core::Polyline result;
    for (const core::Vec2& point : points) {
        result.addVertex(point);
    }
    return result;
}

TEST_F(PropertyGridTest, HasTheGroupsGeneralAndGeometry) {
    EXPECT_EQ(model().rowCount(), Model::kGroupCount);
    EXPECT_EQ(model().columnCount(), Model::kColumnCount);
    const QModelIndex general = model().groupIndex(Model::kGeneralGroup);
    const QModelIndex geometry = model().groupIndex(Model::kGeometryGroup);
    EXPECT_EQ(general.data().toString(), QStringLiteral("General"));
    EXPECT_EQ(geometry.data().toString(), QStringLiteral("Geometry"));
    EXPECT_TRUE(general.data(Model::kGroupRole).toBool());
    EXPECT_FALSE(model().flags(general).testFlag(Qt::ItemIsEditable));
    EXPECT_EQ(
        column(Model::kGeneralGroup, Model::kNameColumn),
        (QStringList{QStringLiteral("Color"), QStringLiteral("Layer"), QStringLiteral("Linetype"),
                     QStringLiteral("Linetype scale"), QStringLiteral("Lineweight")}));
    // Without an entity Geometry has no rows.
    EXPECT_EQ(model().rowCount(geometry), 0);
    EXPECT_TRUE(model().geometry().isEmpty());
    // The rows of a group have no children, and their parent is the group.
    const QModelIndex color = model().valueIndex(PropertyField::Color);
    EXPECT_EQ(model().parent(color), general);
    EXPECT_EQ(model().rowCount(color), 0);
    EXPECT_FALSE(color.data(Model::kGroupRole).toBool());
}

TEST_F(PropertyGridTest, GeneralShowsTheValuesAndVaries) {
    const core::CurrentProperties& current = document().current();
    model().setShown({current.color, current.layer, current.linetype, current.linetypeScale,
                      current.lineweight});
    EXPECT_EQ(value(PropertyField::Color), QStringLiteral("ByLayer"));
    EXPECT_EQ(value(PropertyField::Layer), QStringLiteral("0"));
    EXPECT_EQ(value(PropertyField::Linetype),
              QString::fromStdString(document().linetype(current.linetype)->name));
    EXPECT_EQ(value(PropertyField::LinetypeScale), QStringLiteral("1.0000"));
    EXPECT_EQ(value(PropertyField::Lineweight), QStringLiteral("ByLayer"));
    EXPECT_EQ(model().valueIndex(PropertyField::Layer).data(Model::kFieldRole).toInt(),
              static_cast<int>(PropertyField::Layer));

    // A colour and a lineweight of their own, with their samples.
    ShownProperties red = model().shown();
    red.color = core::Color::fromIndex(1);
    red.lineweight = 50;
    model().setShown(red);
    EXPECT_EQ(value(PropertyField::Color), QStringLiteral("Red"));
    EXPECT_EQ(value(PropertyField::Lineweight), QStringLiteral("0.50 mm"));
    EXPECT_FALSE(
        model().valueIndex(PropertyField::Color).data(Qt::DecorationRole).value<QIcon>().isNull());
    EXPECT_FALSE(model()
                     .valueIndex(PropertyField::Lineweight)
                     .data(Qt::DecorationRole)
                     .value<QIcon>()
                     .isNull());

    // Values the entities do not share.
    model().setShown({});
    for (const PropertyField field :
         {PropertyField::Color, PropertyField::Layer, PropertyField::Linetype,
          PropertyField::LinetypeScale, PropertyField::Lineweight}) {
        EXPECT_EQ(value(field), QStringLiteral("*VARIES*"));
    }
}

TEST_F(PropertyGridTest, OnlyTheValuesOfGeneralAreEditable) {
    const QModelIndex name =
        model().index(0, Model::kNameColumn, model().groupIndex(Model::kGeneralGroup));
    EXPECT_FALSE(model().flags(name).testFlag(Qt::ItemIsEditable));
    EXPECT_TRUE(
        model().flags(model().valueIndex(PropertyField::Color)).testFlag(Qt::ItemIsEditable));

    const core::EntityRecord line = record(257, core::Line{{0.0, 0.0}, {10.0, 5.0}});
    model().setEntity(&line);
    const QModelIndex geometry =
        model().index(0, Model::kValueColumn, model().groupIndex(Model::kGeometryGroup));
    EXPECT_FALSE(model().flags(geometry).testFlag(Qt::ItemIsEditable));

    model().setEditable(false);
    EXPECT_FALSE(model().editable());
    EXPECT_FALSE(
        model().flags(model().valueIndex(PropertyField::Color)).testFlag(Qt::ItemIsEditable));
}

TEST_F(PropertyGridTest, GeometryOfALineAndACircle) {
    const core::EntityRecord line = record(257, core::Line{{0.0, 0.0}, {10.0, 5.0}});
    model().setEntity(&line);
    const Rows lineRows{{QStringLiteral("Start X"), QStringLiteral("0.0000")},
                        {QStringLiteral("Start Y"), QStringLiteral("0.0000")},
                        {QStringLiteral("End X"), QStringLiteral("10.0000")},
                        {QStringLiteral("End Y"), QStringLiteral("5.0000")},
                        {QStringLiteral("Length"), QStringLiteral("11.1803")}};
    EXPECT_EQ(model().geometry(), lineRows);
    EXPECT_EQ(column(Model::kGeometryGroup, Model::kNameColumn).size(), 5);
    EXPECT_EQ(column(Model::kGeometryGroup, Model::kValueColumn).last(), QStringLiteral("11.1803"));
    EXPECT_EQ(model().vertexCount(), 0);
    EXPECT_FALSE(vertexValue().isValid());

    // Fewer rows: the view is told of the rows that go.
    const QSignalSpy removed(&model(), &QAbstractItemModel::rowsRemoved);
    const core::EntityRecord circle = record(258, core::Circle{{20.0, 0.0}, 2.0});
    model().setEntity(&circle);
    const Rows circleRows{{QStringLiteral("Center X"), QStringLiteral("20.0000")},
                          {QStringLiteral("Center Y"), QStringLiteral("0.0000")},
                          {QStringLiteral("Radius"), QStringLiteral("2.0000")}};
    EXPECT_EQ(model().geometry(), circleRows);
    EXPECT_EQ(removed.size(), 1);
    EXPECT_EQ(model().rowCount(model().groupIndex(Model::kGeometryGroup)), 3);

    model().setEntity(nullptr);
    EXPECT_TRUE(model().geometry().isEmpty());
    EXPECT_EQ(model().rowCount(model().groupIndex(Model::kGeometryGroup)), 0);
}

TEST_F(PropertyGridTest, APolylineBrowsesItsVerticesBetweenTheTwoEnds) {
    const core::EntityRecord plate = record(300, polyline({{0.0, 0.0}, {40.0, 0.0}, {40.0, 30.0}}));
    model().setEntity(&plate);
    EXPECT_EQ(model().vertexCount(), 3);
    EXPECT_EQ(model().currentVertex(), 0);
    const Rows first{{QStringLiteral("Vertices"), QStringLiteral("3")},
                     {QStringLiteral("Current Vertex"), QStringLiteral("1")},
                     {QStringLiteral("Vertex X"), QStringLiteral("0.0000")},
                     {QStringLiteral("Vertex Y"), QStringLiteral("0.0000")},
                     {QStringLiteral("Closed"), QStringLiteral("No")},
                     {QStringLiteral("Length"), QStringLiteral("70.0000")}};
    EXPECT_EQ(model().geometry(), first);

    // The first vertex: no way back.
    EXPECT_FALSE(vertexValue().data(Model::kPreviousVertexRole).toBool());
    EXPECT_TRUE(vertexValue().data(Model::kNextVertexRole).toBool());
    EXPECT_FALSE(model().previousVertex());
    EXPECT_EQ(model().currentVertex(), 0);

    ASSERT_TRUE(model().nextVertex());
    EXPECT_EQ(model().geometry().at(1).second, QStringLiteral("2"));
    EXPECT_EQ(model().geometry().at(2).second, QStringLiteral("40.0000"));
    EXPECT_EQ(model().geometry().at(3).second, QStringLiteral("0.0000"));
    EXPECT_TRUE(vertexValue().data(Model::kPreviousVertexRole).toBool());
    EXPECT_TRUE(vertexValue().data(Model::kNextVertexRole).toBool());

    // The last vertex: no way forward, and nothing changes.
    ASSERT_TRUE(model().nextVertex());
    EXPECT_EQ(model().geometry().at(1).second, QStringLiteral("3"));
    EXPECT_EQ(model().geometry().at(2).second, QStringLiteral("40.0000"));
    EXPECT_EQ(model().geometry().at(3).second, QStringLiteral("30.0000"));
    EXPECT_FALSE(vertexValue().data(Model::kNextVertexRole).toBool());
    EXPECT_FALSE(model().nextVertex());
    EXPECT_EQ(model().currentVertex(), 2);
    EXPECT_FALSE(model().setCurrentVertex(3));
    EXPECT_FALSE(model().setCurrentVertex(-1));

    ASSERT_TRUE(model().previousVertex());
    EXPECT_EQ(model().currentVertex(), 1);
    ASSERT_TRUE(model().setCurrentVertex(0));
    EXPECT_EQ(model().geometry(), first);
}

TEST_F(PropertyGridTest, TheSamePolylineKeepsItsVertexAnotherStartsAgain) {
    const core::EntityRecord plate = record(300, polyline({{0.0, 0.0}, {40.0, 0.0}, {40.0, 30.0}}));
    model().setEntity(&plate);
    ASSERT_TRUE(model().setCurrentVertex(2));

    // Shown again, as after a change of its properties.
    model().setEntity(&plate);
    EXPECT_EQ(model().currentVertex(), 2);

    // The same polyline with a vertex less: its last vertex.
    const core::EntityRecord shorter = record(300, polyline({{0.0, 0.0}, {40.0, 0.0}}));
    model().setEntity(&shorter);
    EXPECT_EQ(model().currentVertex(), 1);
    EXPECT_EQ(model().geometry().at(1).second, QStringLiteral("2"));

    const core::EntityRecord other = record(301, polyline({{5.0, 5.0}, {6.0, 5.0}}));
    model().setEntity(&other);
    EXPECT_EQ(model().currentVertex(), 0);
    EXPECT_EQ(model().geometry().at(2).second, QStringLiteral("5.0000"));
}

TEST_F(PropertyGridTest, TheArrowsOfCurrentVertexAskForAnotherVertex) {
    const core::EntityRecord plate = record(300, polyline({{0.0, 0.0}, {40.0, 0.0}}));
    model().setEntity(&plate);
    PropertyGridDelegate delegate;
    QAbstractItemDelegate& base = delegate;
    const QSignalSpy previous(&delegate, &PropertyGridDelegate::previousVertexRequested);
    const QSignalSpy next(&delegate, &PropertyGridDelegate::nextVertexRequested);

    QStyleOptionViewItem option;
    option.rect = QRect(124, 0, 200, 22);
    const QRect before = PropertyGridDelegate::previousArrow(option.rect);
    const QRect after = PropertyGridDelegate::nextArrow(option.rect);
    EXPECT_TRUE(option.rect.contains(before));
    EXPECT_TRUE(option.rect.contains(after));
    EXPECT_LT(before.right(), after.left());

    const auto click = [&](const QPoint& at, const QModelIndex& index) {
        QMouseEvent release(QEvent::MouseButtonRelease, QPointF(at), QPointF(at), Qt::LeftButton,
                            Qt::NoButton, Qt::NoModifier);
        return base.editorEvent(&release, &model(), option, index);
    };
    EXPECT_TRUE(click(after.center(), vertexValue()));
    EXPECT_EQ(next.size(), 1);
    EXPECT_TRUE(click(before.center(), vertexValue()));
    EXPECT_EQ(previous.size(), 1);
    // Beside the arrows, or on another row, a click is no request.
    EXPECT_FALSE(click(option.rect.topLeft() + QPoint(10, 10), vertexValue()));
    EXPECT_FALSE(click(after.center(), model().valueIndex(PropertyField::Color)));
    EXPECT_EQ(next.size() + previous.size(), 2);
}

TEST_F(PropertyGridTest, TheDelegateOpensTheEditorsOfGeneral) {
    const core::CurrentProperties& current = document().current();
    model().setShown({std::nullopt, current.layer, current.linetype, 2.5, current.lineweight});
    PropertyGridDelegate delegate;
    QWidget parent;
    const QStyleOptionViewItem option;
    const auto open = [&](PropertyField field) {
        const QModelIndex index = model().valueIndex(field);
        QWidget* editor = delegate.createEditor(&parent, option, index);
        if (editor != nullptr) {
            delegate.setEditorData(editor, index);
        }
        return std::unique_ptr<QWidget>(editor);
    };

    const auto color = open(PropertyField::Color);
    ASSERT_NE(qobject_cast<ColorCombo*>(color.get()), nullptr);
    EXPECT_EQ(color->objectName(), QStringLiteral("propertiesColor"));
    EXPECT_FALSE(qobject_cast<ColorCombo*>(color.get())->currentData().isValid())
        << "a colour that varies shows Varies";

    const auto layer = open(PropertyField::Layer);
    ASSERT_NE(qobject_cast<LayerCombo*>(layer.get()), nullptr);
    EXPECT_EQ(qobject_cast<LayerCombo*>(layer.get())->currentText(), QStringLiteral("0"));
    EXPECT_NE(qobject_cast<LinetypeCombo*>(open(PropertyField::Linetype).get()), nullptr);
    EXPECT_NE(qobject_cast<LineweightCombo*>(open(PropertyField::Lineweight).get()), nullptr);

    const auto scale = open(PropertyField::LinetypeScale);
    auto* edit = qobject_cast<QLineEdit*>(scale.get());
    ASSERT_NE(edit, nullptr);
    EXPECT_EQ(edit->text(), QStringLiteral("2.5000"));

    // A typed scale is reported once, as typed; an untouched one never.
    const QSignalSpy entered(&delegate, &PropertyGridDelegate::linetypeScaleEntered);
    ASSERT_TRUE(QMetaObject::invokeMethod(edit, "editingFinished", Qt::DirectConnection));
    EXPECT_EQ(entered.size(), 0);
    edit->setText(QStringLiteral(" 3 "));
    edit->setModified(true);
    ASSERT_TRUE(QMetaObject::invokeMethod(edit, "editingFinished", Qt::DirectConnection));
    ASSERT_EQ(entered.size(), 1);
    EXPECT_EQ(entered.at(0).at(0).toString(), QStringLiteral("3"));

    // Geometry and the names have no editor.
    const core::EntityRecord line = record(257, core::Line{{0.0, 0.0}, {10.0, 5.0}});
    model().setEntity(&line);
    EXPECT_EQ(delegate.createEditor(
                  &parent, option,
                  model().index(0, Model::kValueColumn, model().groupIndex(Model::kGeometryGroup))),
              nullptr);
    EXPECT_EQ(delegate.createEditor(
                  &parent, option,
                  model().index(0, Model::kNameColumn, model().groupIndex(Model::kGeneralGroup))),
              nullptr);
}

}  // namespace
}  // namespace tg::app
