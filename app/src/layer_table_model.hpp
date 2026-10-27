// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <QAbstractTableModel>
#include <QPalette>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <vector>

namespace tg::app {

/// The columns of the layers table, in the order of docs/tappe/0.6/P22.
enum class LayerColumn : int {
    Status,
    Name,
    On,
    Frozen,
    Locked,
    Color,
    Linetype,
    Lineweight,
    Plot,
    Count
};

/// The yes/no states of a layer.
enum class LayerState : std::uint8_t { On, Frozen, Locked, Plot };

/// The value of `state` in `layer`.
[[nodiscard]] bool layerStateValue(const core::Layer& layer, LayerState state);

/// Name, under :/tg/icons/, of the icon that shows `state` when it is `value`:
/// the eye of "on", the snowflake of "frozen", the padlock of "locked". Empty
/// for Plot, which is a check box.
[[nodiscard]] QString layerStateIconName(LayerState state, bool value);

/// Every icon the layers panel draws that no action names: the states and
/// the mark of the current layer (docs/ICONE.md).
[[nodiscard]] QStringList layerPanelIconNames();

/// The layers of a document as a table; read only. A change of the document
/// is made by LayerPanel, which asks the editor for it, and then calls
/// refresh(). The model shows a copy of the layers taken by refresh(): it
/// never reads the document while the view asks for the data, so a view that
/// outlives the document (a window closing) cannot reach freed layers.
class LayerTableModel final : public QAbstractTableModel {
    Q_OBJECT

public:
    /// The core::LayerId of the row, for every column.
    static constexpr int kLayerIdRole = Qt::UserRole + 1;

    /// The document is borrowed and must outlive the model.
    explicit LayerTableModel(const core::Document& document, QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role = Qt::DisplayRole) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;

    /// Only the name can be edited, and it never changes the document: it
    /// asks for the change with renameRequested() and returns false.
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    /// Follows the document. Rows are added or removed with a reset of the
    /// model; when only values changed, with dataChanged() and no reset, so
    /// that the widgets of the cells stay.
    void refresh();

    /// The palette the icons are drawn with.
    void setPalette(const QPalette& palette);

    /// The layer of `row`, or the identifier 0 if there is no such row.
    [[nodiscard]] core::LayerId layerAt(int row) const;

    /// The row of `id`, or -1.
    [[nodiscard]] int rowOf(core::LayerId id) const;

Q_SIGNALS:
    void renameRequested(tg::core::LayerId id, const QString& name);

private:
    /// What the table shows of one layer.
    struct Row {
        core::LayerId id;
        QString name;
        QString color;
        QString linetype;
        QString lineweight;
        bool on{true};
        bool frozen{false};
        bool locked{false};
        bool plot{true};
        bool current{false};
        bool zero{false};
    };

    [[nodiscard]] std::vector<Row> snapshot() const;
    [[nodiscard]] QVariant stateData(int role, LayerState state, bool value) const;
    void notifyAll();

    const core::Document& m_document;
    std::vector<Row> m_rows;
    QPalette m_palette;
};

}  // namespace tg::app
