// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <QAbstractItemModel>
#include <QList>
#include <QPair>
#include <QPalette>
#include <QRect>
#include <QString>
#include <QStyledItemDelegate>

#include <cstdint>
#include <optional>

class QEvent;
class QPainter;

namespace tg::app {

/// The values the panel shows: the one every entity shares, or nothing when
/// they differ (*VARIES*). With no selection, the current values.
struct ShownProperties {
    std::optional<core::Color> color;
    std::optional<core::LayerId> layer;
    std::optional<core::LinetypeId> linetype;
    std::optional<double> linetypeScale;
    std::optional<int> lineweight;
};

/// The rows of the group General, in their order.
enum class PropertyField : std::uint8_t { Color, Layer, Linetype, LinetypeScale, Lineweight };

/// The grid of the panel Properties (docs/adr/0039, point 1, area 8): two
/// columns, name and value, under two groups, General and Geometry.
///
/// General has a row per PropertyField, with the values of setShown(); its
/// values are editable while setEditable(true), through
/// PropertyGridDelegate. Geometry lists, read only, the coordinates of the
/// entity of setEntity(), with the names of AutoCAD's panel, and is empty
/// without one. For a polyline it browses the vertices: Current Vertex,
/// Vertex X and Vertex Y of the vertex chosen with nextVertex() and
/// previousVertex(), which stop at the first and the last vertex.
///
/// The group rows are the top level; the rows of a group are its children.
/// The document is borrowed and must outlive the model.
class PropertyGridModel final : public QAbstractItemModel {
    Q_OBJECT

public:
    static constexpr int kGeneralGroup = 0;
    static constexpr int kGeometryGroup = 1;
    static constexpr int kGroupCount = 2;
    static constexpr int kNameColumn = 0;
    static constexpr int kValueColumn = 1;
    static constexpr int kColumnCount = 2;

    /// True on the row of a group.
    static constexpr int kGroupRole = Qt::UserRole + 1;
    /// The PropertyField of a row of General, as an int; invalid elsewhere.
    static constexpr int kFieldRole = Qt::UserRole + 2;
    /// On the value of Current Vertex only: whether there is a vertex before
    /// (kPreviousVertexRole) or after (kNextVertexRole) the current one.
    static constexpr int kPreviousVertexRole = Qt::UserRole + 3;
    static constexpr int kNextVertexRole = Qt::UserRole + 4;

    explicit PropertyGridModel(const core::Document& document, QObject* parent = nullptr);

    using QObject::parent;

    [[nodiscard]] const core::Document& document() const { return *m_document; }

    /// The palette the samples of colour, linetype and lineweight are drawn
    /// for.
    void setPalette(const QPalette& palette);

    /// Shows `shown` in General. Every value is reported changed, so that an
    /// open editor shows it again even when it did not change.
    void setShown(const ShownProperties& shown);

    [[nodiscard]] const ShownProperties& shown() const { return m_shown; }

    void setEditable(bool editable);

    [[nodiscard]] bool editable() const { return m_editable; }

    /// The entity Geometry shows, or none. The same entity again keeps the
    /// current vertex, moved to the last one if the polyline lost vertices;
    /// another entity starts from the first.
    void setEntity(const core::EntityRecord* record);

    /// The rows of Geometry: name and value. Empty without an entity.
    [[nodiscard]] const QList<QPair<QString, QString>>& geometry() const { return m_geometry; }

    /// The vertices of the polyline shown; 0 for any other entity.
    [[nodiscard]] int vertexCount() const;

    /// The vertex Current Vertex shows, from 0.
    [[nodiscard]] int currentVertex() const { return m_vertex; }

    /// Each shows another vertex. False, changing nothing, past the first or
    /// the last vertex, or without a polyline.
    bool setCurrentVertex(int vertex);
    bool nextVertex();
    bool previousVertex();

    /// The row of `group` (kGeneralGroup, kGeometryGroup).
    [[nodiscard]] QModelIndex groupIndex(int group) const;
    /// The value of the row `field` of General.
    [[nodiscard]] QModelIndex valueIndex(PropertyField field) const;

    [[nodiscard]] QModelIndex index(int row, int column,
                                    const QModelIndex& parent = {}) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& child) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    /// Only the linetype scale, as Qt::EditRole text, while editable: the
    /// model stores nothing and reports it with linetypeScaleEntered(), and
    /// the panel changes the document and shows the result (setShown).
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

Q_SIGNALS:
    /// The text the user committed as linetype scale, unchecked.
    void linetypeScaleEntered(const QString& text);

private:
    [[nodiscard]] QVariant generalData(PropertyField field, int column, int role) const;
    [[nodiscard]] QVariant geometryData(int row, int column, int role) const;
    [[nodiscard]] QString valueText(PropertyField field) const;
    [[nodiscard]] QVariant valueIcon(PropertyField field) const;
    /// Recomputes the rows of Geometry and tells the views.
    void showGeometry();

    const core::Document* m_document;
    QPalette m_palette;
    ShownProperties m_shown;
    bool m_editable{true};
    std::optional<core::Entity> m_entity;
    core::EntityId m_entityId;
    int m_vertex{0};
    /// The row of Current Vertex in Geometry, or -1.
    int m_vertexRow{-1};
    QList<QPair<QString, QString>> m_geometry;
};

/// Draws PropertyGridModel as in docs/adr/0039 (point 1, area 8; point 2):
/// the groups on surface.group with a chevron, the names in text.secondary,
/// the values in text, thin lines between the rows and the columns, rows of
/// 22 px. It opens the editors of General, the controls of the panel of
/// 0.6-P24: ColorCombo, LayerCombo, LinetypeCombo, LineweightCombo and a
/// line edit for the linetype scale.
///
/// The combos report the user's choice with the signals below as soon as it
/// is made; the linetype scale goes through the commit of the view (Enter,
/// Tab, the focus leaving the cell) to PropertyGridModel::setData, so Esc
/// drops what was typed. Either way the panel changes the document and shows
/// the result again (setShown). The arrows of Current Vertex report a click
/// with a signal too.
class PropertyGridDelegate final : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit PropertyGridDelegate(QObject* parent = nullptr);

    /// Where the arrows of Current Vertex are, in its value cell `cell`.
    [[nodiscard]] static QRect previousArrow(const QRect& cell);
    [[nodiscard]] static QRect nextArrow(const QRect& cell);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const override;
    [[nodiscard]] QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                                        const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;

protected:
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
                     const QModelIndex& index) override;

Q_SIGNALS:
    void colorChosen(tg::core::Color value);
    void layerChosen(tg::core::LayerId value);
    void linetypeChosen(tg::core::LinetypeId value);
    void lineweightChosen(int value);
    void previousVertexRequested();
    void nextVertexRequested();
};

}  // namespace tg::app
