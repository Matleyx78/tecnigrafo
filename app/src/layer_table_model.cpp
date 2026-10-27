// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "layer_table_model.hpp"

#include "theme.hpp"

#include <QIcon>
#include <QVariant>

#include <cstddef>
#include <utility>

namespace tg::app {
namespace {

constexpr int kColumnCount = static_cast<int>(LayerColumn::Count);

// The mark of the current layer in the status column (docs/ICONE.md).
const char* const kCurrentIcon = "layer-current";

QVariant iconData(const QString& name, const QPalette& palette) {
    if (name.isEmpty()) {
        return {};
    }
    return QVariant::fromValue(themedIcon(name, palette));
}

QVariant centered() {
    return static_cast<int>(Qt::AlignCenter);
}

QString stateText(LayerState state, bool value) {
    switch (state) {
        case LayerState::On:
            return value ? LayerTableModel::tr("On") : LayerTableModel::tr("Off");
        case LayerState::Frozen:
            return value ? LayerTableModel::tr("Frozen") : LayerTableModel::tr("Thawed");
        case LayerState::Locked:
            return value ? LayerTableModel::tr("Locked") : LayerTableModel::tr("Unlocked");
        case LayerState::Plot:
            return value ? LayerTableModel::tr("Plot") : LayerTableModel::tr("No plot");
    }
    return {};
}

}  // namespace

bool layerStateValue(const core::Layer& layer, LayerState state) {
    switch (state) {
        case LayerState::On:
            return layer.on;
        case LayerState::Frozen:
            return layer.frozen;
        case LayerState::Locked:
            return layer.locked;
        case LayerState::Plot:
            return layer.plot;
    }
    return false;
}

QString layerStateIconName(LayerState state, bool value) {
    switch (state) {
        case LayerState::On:
            return QString::fromLatin1(value ? "layer-on" : "layer-off");
        case LayerState::Frozen:
            return QString::fromLatin1(value ? "layer-frozen" : "layer-thawed");
        case LayerState::Locked:
            return QString::fromLatin1(value ? "layer-locked" : "layer-unlocked");
        case LayerState::Plot:
            break;
    }
    return {};
}

QStringList layerPanelIconNames() {
    return {QString::fromLatin1(kCurrentIcon),    QString::fromLatin1("layer-on"),
            QString::fromLatin1("layer-off"),     QString::fromLatin1("layer-frozen"),
            QString::fromLatin1("layer-thawed"),  QString::fromLatin1("layer-locked"),
            QString::fromLatin1("layer-unlocked")};
}

LayerTableModel::LayerTableModel(const core::Document& document, QObject* parent)
    : QAbstractTableModel(parent), m_document(document) {
    for (const core::Layer& layer : m_document.layers()) {
        m_ids.push_back(layer.id);
    }
}

int LayerTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_ids.size());
}

int LayerTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : kColumnCount;
}

QVariant LayerTableModel::stateData(int role, LayerState state, bool value) const {
    if (role == Qt::DecorationRole) {
        return iconData(layerStateIconName(state, value), m_palette);
    }
    if (role == Qt::ToolTipRole) {
        return stateText(state, value);
    }
    if (role == Qt::TextAlignmentRole) {
        return centered();
    }
    if (role == Qt::CheckStateRole && state == LayerState::Plot) {
        return static_cast<int>(value ? Qt::Checked : Qt::Unchecked);
    }
    return {};
}

QVariant LayerTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= static_cast<int>(m_ids.size()) ||
        index.column() >= kColumnCount) {
        return {};
    }
    const core::LayerId id = m_ids[static_cast<std::size_t>(index.row())];
    if (role == kLayerIdRole) {
        return QVariant::fromValue(id);
    }
    // The row may outlive the layer between a change and refresh().
    const core::Layer* layer = m_document.layer(id);
    if (layer == nullptr) {
        return {};
    }
    const bool display = role == Qt::DisplayRole || role == Qt::EditRole;
    switch (static_cast<LayerColumn>(index.column())) {
        case LayerColumn::Status: {
            if (id != m_document.current().layer) {
                return role == Qt::TextAlignmentRole ? centered() : QVariant();
            }
            if (role == Qt::DecorationRole) {
                return iconData(QString::fromLatin1(kCurrentIcon), m_palette);
            }
            if (role == Qt::ToolTipRole) {
                return tr("Current layer");
            }
            return role == Qt::TextAlignmentRole ? centered() : QVariant();
        }
        case LayerColumn::Name:
            return display ? QVariant(QString::fromStdString(layer->name)) : QVariant();
        case LayerColumn::On:
            return stateData(role, LayerState::On, layer->on);
        case LayerColumn::Frozen:
            return stateData(role, LayerState::Frozen, layer->frozen);
        case LayerColumn::Locked:
            return stateData(role, LayerState::Locked, layer->locked);
        case LayerColumn::Color:
            return role == Qt::DisplayRole
                       ? QVariant(QString::fromStdString(core::formatColor(layer->color)))
                       : QVariant();
        case LayerColumn::Linetype: {
            const core::Linetype* linetype = m_document.linetype(layer->linetype);
            return role == Qt::DisplayRole && linetype != nullptr
                       ? QVariant(QString::fromStdString(linetype->name))
                       : QVariant();
        }
        case LayerColumn::Lineweight:
            return role == Qt::DisplayRole
                       ? QVariant(QString::fromStdString(core::formatLineweight(layer->lineweight)))
                       : QVariant();
        case LayerColumn::Plot:
            return stateData(role, LayerState::Plot, layer->plot);
        case LayerColumn::Count:
            break;
    }
    return {};
}

QVariant LayerTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole || section < 0 ||
        section >= kColumnCount) {
        return {};
    }
    switch (static_cast<LayerColumn>(section)) {
        case LayerColumn::Status:
            return tr("Current");
        case LayerColumn::Name:
            return tr("Name");
        case LayerColumn::On:
            return tr("On");
        case LayerColumn::Frozen:
            return tr("Frozen");
        case LayerColumn::Locked:
            return tr("Locked");
        case LayerColumn::Color:
            return tr("Colour");
        case LayerColumn::Linetype:
            return tr("Linetype");
        case LayerColumn::Lineweight:
            return tr("Lineweight");
        case LayerColumn::Plot:
            return tr("Plot");
        case LayerColumn::Count:
            break;
    }
    return {};
}

Qt::ItemFlags LayerTableModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    // Layer 0 is never renamed (docs/adr/0023, point 1).
    if (index.column() == static_cast<int>(LayerColumn::Name) &&
        layerAt(index.row()) != m_document.layerZero()) {
        result |= Qt::ItemIsEditable;
    }
    return result;
}

bool LayerTableModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role != Qt::EditRole || !index.isValid() ||
        index.column() != static_cast<int>(LayerColumn::Name)) {
        return false;
    }
    Q_EMIT renameRequested(layerAt(index.row()), value.toString());
    return false;
}

void LayerTableModel::notifyAll() {
    if (m_ids.empty()) {
        return;
    }
    const int last = static_cast<int>(m_ids.size()) - 1;
    Q_EMIT dataChanged(index(0, 0), index(last, kColumnCount - 1));
}

void LayerTableModel::refresh() {
    std::vector<core::LayerId> ids;
    for (const core::Layer& layer : m_document.layers()) {
        ids.push_back(layer.id);
    }
    if (ids == m_ids) {
        notifyAll();
        return;
    }
    beginResetModel();
    m_ids = std::move(ids);
    endResetModel();
}

void LayerTableModel::setPalette(const QPalette& palette) {
    m_palette = palette;
    notifyAll();
}

core::LayerId LayerTableModel::layerAt(int row) const {
    if (row < 0 || row >= static_cast<int>(m_ids.size())) {
        return {};
    }
    return m_ids[static_cast<std::size_t>(row)];
}

int LayerTableModel::rowOf(core::LayerId id) const {
    for (std::size_t row = 0; row < m_ids.size(); ++row) {
        if (m_ids[row] == id) {
            return static_cast<int>(row);
        }
    }
    return -1;
}

}  // namespace tg::app
