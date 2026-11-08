// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "property_grid.hpp"

#include "property_widgets.hpp"
#include "tg/cmd/value_format.hpp"
#include "theme.hpp"

#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QStyle>
#include <QTreeView>
#include <QVariant>

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>
#include <variant>

namespace tg::app {
namespace {

/// docs/adr/0039, point 3: rows of 20-22 px in the grids.
constexpr int kRowHeight = 22;
/// Where the names and the group titles start: right of the chevron of the
/// groups, as in the reference image (docs/adr/0039/proprieta.svg).
constexpr int kNameIndent = 21;
/// Where the chevron of a group is centred, from the left of the row.
constexpr int kChevronCenter = 11;
/// Half width and height of the chevron.
constexpr qreal kChevronHalfWidth = 3.0;
constexpr qreal kChevronHalfHeight = 1.5;
/// Space between a cell's border and its text or sample.
constexpr int kCellPadding = 7;
/// Space between a sample and the text after it.
constexpr int kIconSpacing = 6;
/// The samples of the values: a colour swatch, a dash pattern, a lineweight.
constexpr int kSampleWidth = 24;
constexpr int kSampleHeight = 10;
/// The square around each arrow of Current Vertex, and the space after the
/// last one, as in the reference image.
constexpr int kArrowSide = 18;
constexpr int kArrowMargin = 4;
/// Half height of an arrow, and how far it points.
constexpr qreal kArrowHalfHeight = 4.5;
constexpr qreal kArrowDepth = 2.5;
/// The 16 px icon stroke of docs/adr/0040, point 3.
constexpr qreal kStrokeWidth = 1.25;

// The names of the objects of the editors: the tests and the panel's former
// controls know them (0.6-P24).
const char* const kColorEditor = "propertiesColor";
const char* const kLayerEditor = "propertiesLayer";
const char* const kLinetypeEditor = "propertiesLinetype";
const char* const kLinetypeScaleEditor = "propertiesLinetypeScale";
const char* const kLineweightEditor = "propertiesLineweight";

// What a value shared by no two of the entities shows, as in AutoCAD.
const char* const kVaries = "*VARIES*";

constexpr std::array kFields{PropertyField::Color, PropertyField::Layer, PropertyField::Linetype,
                             PropertyField::LinetypeScale, PropertyField::Lineweight};

// A child row keeps its group in the internal id: the group plus one, so
// that 0 marks a group row.
constexpr quintptr kGroupRowId = 0;

QString lengthText(double value) {
    return QString::fromStdString(cmd::formatLength(value));
}

QString angleText(double radians) {
    return QString::fromStdString(cmd::formatAngle(radians));
}

QString fieldName(PropertyField field) {
    switch (field) {
        case PropertyField::Color:
            return PropertyGridModel::tr("Color");
        case PropertyField::Layer:
            return PropertyGridModel::tr("Layer");
        case PropertyField::Linetype:
            return PropertyGridModel::tr("Linetype");
        case PropertyField::LinetypeScale:
            return PropertyGridModel::tr("Linetype scale");
        case PropertyField::Lineweight:
            return PropertyGridModel::tr("Lineweight");
    }
    return {};
}

struct GeometryRows {
    QList<QPair<QString, QString>> rows;
    /// The row of Current Vertex, or -1.
    int vertexRow{-1};
};

/// The rows of the group Geometry of `entity`: its main coordinates, with
/// the names of AutoCAD's panel; for a polyline, the vertex `vertex`.
GeometryRows geometryRows(const core::Entity& entity, int vertex) {
    GeometryRows result;
    const auto add = [&result](const QString& name, const QString& value) {
        result.rows.append({name, value});
    };
    if (const auto* line = std::get_if<core::Line>(&entity)) {
        add(PropertyGridModel::tr("Start X"), lengthText(line->start.x));
        add(PropertyGridModel::tr("Start Y"), lengthText(line->start.y));
        add(PropertyGridModel::tr("End X"), lengthText(line->end.x));
        add(PropertyGridModel::tr("End Y"), lengthText(line->end.y));
        add(PropertyGridModel::tr("Length"), lengthText(line->length()));
    } else if (const auto* circle = std::get_if<core::Circle>(&entity)) {
        add(PropertyGridModel::tr("Center X"), lengthText(circle->center.x));
        add(PropertyGridModel::tr("Center Y"), lengthText(circle->center.y));
        add(PropertyGridModel::tr("Radius"), lengthText(circle->radius));
    } else if (const auto* arc = std::get_if<core::Arc>(&entity)) {
        add(PropertyGridModel::tr("Center X"), lengthText(arc->center.x));
        add(PropertyGridModel::tr("Center Y"), lengthText(arc->center.y));
        add(PropertyGridModel::tr("Radius"), lengthText(arc->radius));
        add(PropertyGridModel::tr("Start angle"), angleText(arc->startAngle));
        add(PropertyGridModel::tr("End angle"), angleText(arc->endAngle));
    } else if (const auto* polyline = std::get_if<core::Polyline>(&entity)) {
        add(PropertyGridModel::tr("Vertices"),
            QString::number(static_cast<qulonglong>(polyline->vertexCount())));
        if (!polyline->vertices().empty()) {
            // The area is not shown: core does not compute it (0.7.5-P14).
            const core::Vec2 position =
                polyline->vertices().subspan(static_cast<std::size_t>(vertex), 1).front().position;
            result.vertexRow = static_cast<int>(result.rows.size());
            add(PropertyGridModel::tr("Current Vertex"), QString::number(vertex + 1));
            add(PropertyGridModel::tr("Vertex X"), lengthText(position.x));
            add(PropertyGridModel::tr("Vertex Y"), lengthText(position.y));
        }
        add(PropertyGridModel::tr("Closed"),
            polyline->isClosed() ? PropertyGridModel::tr("Yes") : PropertyGridModel::tr("No"));
        add(PropertyGridModel::tr("Length"), lengthText(polyline->length()));
    } else if (const auto* text = std::get_if<core::Text>(&entity)) {
        add(PropertyGridModel::tr("Position X"), lengthText(text->position.x));
        add(PropertyGridModel::tr("Position Y"), lengthText(text->position.y));
        add(PropertyGridModel::tr("Height"), lengthText(text->height));
        add(PropertyGridModel::tr("Contents"), QString::fromStdString(text->value));
    }
    return result;
}

/// The chevron of a group: pointing down when open, right when closed.
void drawChevron(QPainter& painter, const QPointF& center, bool open, const QColor& color) {
    QPainterPath path;
    if (open) {
        path.moveTo(center.x() - kChevronHalfWidth, center.y() - kChevronHalfHeight);
        path.lineTo(center.x(), center.y() + kChevronHalfHeight);
        path.lineTo(center.x() + kChevronHalfWidth, center.y() - kChevronHalfHeight);
    } else {
        path.moveTo(center.x() - kChevronHalfHeight, center.y() - kChevronHalfWidth);
        path.lineTo(center.x() + kChevronHalfHeight, center.y());
        path.lineTo(center.x() - kChevronHalfHeight, center.y() + kChevronHalfWidth);
    }
    painter.setPen(QPen(color, kStrokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

/// An arrow of Current Vertex in `box`, pointing left or right.
void drawArrow(QPainter& painter, const QRect& box, bool left, const QColor& color) {
    const QPointF center = QRectF(box).center();
    const qreal tip = left ? -kArrowDepth : kArrowDepth;
    QPainterPath path;
    path.moveTo(center.x() - tip, center.y() - kArrowHalfHeight);
    path.lineTo(center.x() + tip, center.y());
    path.lineTo(center.x() - tip, center.y() + kArrowHalfHeight);
    painter.setPen(QPen(color, kStrokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

/// The row of a group: its title, bold, after the chevron.
void paintGroup(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) {
    const ThemeTokens& tokens = themeTokens(themeOf(option.palette));
    painter->fillRect(option.rect, tokens.group);
    // The view decides whether the group is open: the item's state does not
    // say it reliably.
    const auto* view = qobject_cast<const QTreeView*>(option.widget);
    const bool open = view == nullptr || view->isExpanded(index);
    drawChevron(*painter,
                QPointF(option.rect.left() + kChevronCenter, option.rect.center().y() + 0.5), open,
                tokens.text);
    QFont font = option.font;
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(tokens.text);
    const QRect text = option.rect.adjusted(kNameIndent, 0, -kCellPadding, 0);
    painter->drawText(
        text, Qt::AlignLeft | Qt::AlignVCenter,
        QFontMetrics(font).elidedText(index.data().toString(), Qt::ElideRight, text.width()));
    painter->setPen(tokens.border);
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
}

}  // namespace

PropertyGridModel::PropertyGridModel(const core::Document& document, QObject* parent)
    : QAbstractItemModel(parent), m_document(&document) {}

void PropertyGridModel::setPalette(const QPalette& palette) {
    m_palette = palette;
    Q_EMIT dataChanged(valueIndex(kFields.front()), valueIndex(kFields.back()),
                       {Qt::DecorationRole});
}

void PropertyGridModel::setShown(const ShownProperties& shown) {
    m_shown = shown;
    // One cell at a time: a view gives an open editor the new value only for
    // a change of a single cell.
    for (const PropertyField field : kFields) {
        const QModelIndex value = valueIndex(field);
        Q_EMIT dataChanged(value, value);
    }
}

void PropertyGridModel::setEditable(bool editable) {
    if (editable == m_editable) {
        return;
    }
    m_editable = editable;
    Q_EMIT dataChanged(valueIndex(kFields.front()), valueIndex(kFields.back()));
}

void PropertyGridModel::setEntity(const core::EntityRecord* record) {
    const bool same = record != nullptr && m_entity.has_value() && record->id == m_entityId;
    if (record != nullptr) {
        m_entity = record->entity;
        m_entityId = record->id;
    } else {
        m_entity.reset();
        m_entityId = {};
    }
    if (!same) {
        m_vertex = 0;
    }
    m_vertex = std::max(0, std::min(m_vertex, vertexCount() - 1));
    showGeometry();
}

int PropertyGridModel::vertexCount() const {
    if (!m_entity) {
        return 0;
    }
    const auto* polyline = std::get_if<core::Polyline>(&*m_entity);
    return polyline != nullptr ? static_cast<int>(polyline->vertexCount()) : 0;
}

bool PropertyGridModel::setCurrentVertex(int vertex) {
    if (vertex < 0 || vertex >= vertexCount() || vertex == m_vertex) {
        return false;
    }
    m_vertex = vertex;
    showGeometry();
    return true;
}

bool PropertyGridModel::nextVertex() {
    return setCurrentVertex(m_vertex + 1);
}

bool PropertyGridModel::previousVertex() {
    return setCurrentVertex(m_vertex - 1);
}

void PropertyGridModel::showGeometry() {
    GeometryRows next = m_entity ? geometryRows(*m_entity, m_vertex) : GeometryRows{};
    const QModelIndex group = groupIndex(kGeometryGroup);
    if (next.rows.size() == m_geometry.size()) {
        // Same rows: only the values change, and the view keeps its state.
        m_geometry = std::move(next.rows);
        m_vertexRow = next.vertexRow;
        if (!m_geometry.isEmpty()) {
            Q_EMIT dataChanged(index(0, kNameColumn, group),
                               index(static_cast<int>(m_geometry.size()) - 1, kValueColumn, group));
        }
        return;
    }
    if (!m_geometry.isEmpty()) {
        beginRemoveRows(group, 0, static_cast<int>(m_geometry.size()) - 1);
        m_geometry.clear();
        m_vertexRow = -1;
        endRemoveRows();
    }
    if (!next.rows.isEmpty()) {
        beginInsertRows(group, 0, static_cast<int>(next.rows.size()) - 1);
        m_geometry = std::move(next.rows);
        m_vertexRow = next.vertexRow;
        endInsertRows();
    }
}

QModelIndex PropertyGridModel::groupIndex(int group) const {
    return index(group, kNameColumn);
}

QModelIndex PropertyGridModel::valueIndex(PropertyField field) const {
    return index(static_cast<int>(field), kValueColumn, groupIndex(kGeneralGroup));
}

QModelIndex PropertyGridModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) {
        return {};
    }
    if (!parent.isValid()) {
        return createIndex(row, column, kGroupRowId);
    }
    return createIndex(row, column, static_cast<quintptr>(parent.row()) + 1);
}

QModelIndex PropertyGridModel::parent(const QModelIndex& child) const {
    if (!child.isValid() || child.internalId() == kGroupRowId) {
        return {};
    }
    return createIndex(static_cast<int>(child.internalId() - 1), kNameColumn, kGroupRowId);
}

int PropertyGridModel::rowCount(const QModelIndex& parent) const {
    if (!parent.isValid()) {
        return kGroupCount;
    }
    if (parent.internalId() != kGroupRowId || parent.column() != kNameColumn) {
        return 0;
    }
    return parent.row() == kGeneralGroup ? static_cast<int>(kFields.size())
                                         : static_cast<int>(m_geometry.size());
}

int PropertyGridModel::columnCount(const QModelIndex& /*parent*/) const {
    return kColumnCount;
}

QVariant PropertyGridModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) {
        return {};
    }
    if (index.internalId() == kGroupRowId) {
        if (role == kGroupRole) {
            return true;
        }
        if (role == Qt::DisplayRole && index.column() == kNameColumn) {
            return index.row() == kGeneralGroup ? tr("General") : tr("Geometry");
        }
        return {};
    }
    if (role == kGroupRole) {
        return false;
    }
    if (static_cast<int>(index.internalId() - 1) == kGeneralGroup) {
        return generalData(kFields.at(static_cast<std::size_t>(index.row())), index.column(), role);
    }
    return geometryData(index.row(), index.column(), role);
}

QVariant PropertyGridModel::generalData(PropertyField field, int column, int role) const {
    if (role == kFieldRole) {
        return static_cast<int>(field);
    }
    if (column == kNameColumn) {
        return role == Qt::DisplayRole ? QVariant(fieldName(field)) : QVariant();
    }
    switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
            return valueText(field);
        case Qt::DecorationRole:
            return valueIcon(field);
        default:
            return {};
    }
}

QVariant PropertyGridModel::geometryData(int row, int column, int role) const {
    const auto& [name, value] = m_geometry.at(row);
    if (role == Qt::DisplayRole) {
        return column == kNameColumn ? name : value;
    }
    if (row == m_vertexRow && column == kValueColumn) {
        if (role == kPreviousVertexRole) {
            return m_vertex > 0;
        }
        if (role == kNextVertexRole) {
            return m_vertex + 1 < vertexCount();
        }
    }
    return {};
}

QString PropertyGridModel::valueText(PropertyField field) const {
    const QString varies = QString::fromLatin1(kVaries);
    switch (field) {
        case PropertyField::Color:
            return m_shown.color ? colorName(*m_shown.color) : varies;
        case PropertyField::Layer: {
            if (!m_shown.layer) {
                return varies;
            }
            const core::Layer* layer = m_document->layer(*m_shown.layer);
            return layer != nullptr ? QString::fromStdString(layer->name) : QString();
        }
        case PropertyField::Linetype: {
            if (!m_shown.linetype) {
                return varies;
            }
            const core::Linetype* linetype = m_document->linetype(*m_shown.linetype);
            return linetype != nullptr ? QString::fromStdString(linetype->name) : QString();
        }
        case PropertyField::LinetypeScale:
            return m_shown.linetypeScale ? lengthText(*m_shown.linetypeScale) : varies;
        case PropertyField::Lineweight:
            return m_shown.lineweight ? lineweightName(*m_shown.lineweight) : varies;
    }
    return {};
}

QVariant PropertyGridModel::valueIcon(PropertyField field) const {
    switch (field) {
        case PropertyField::Color:
            if (m_shown.color) {
                return QVariant::fromValue(colorIcon(*m_shown.color, m_palette));
            }
            return {};
        case PropertyField::Linetype:
            if (m_shown.linetype) {
                if (const core::Linetype* linetype = m_document->linetype(*m_shown.linetype)) {
                    return QVariant::fromValue(linetypeIcon(linetype->pattern, m_palette));
                }
            }
            return {};
        case PropertyField::Lineweight:
            if (m_shown.lineweight) {
                // ByLayer and ByBlock have no width of their own, as in the
                // drop-down list; Default shows LWDEFAULT.
                const int weight = *m_shown.lineweight == core::lineweight::kDefault
                                       ? m_document->lwdefault()
                                       : *m_shown.lineweight;
                if (weight >= 0) {
                    return QVariant::fromValue(lineweightIcon(weight, m_palette));
                }
            }
            return {};
        case PropertyField::Layer:
        case PropertyField::LinetypeScale:
            return {};
    }
    return {};
}

Qt::ItemFlags PropertyGridModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    if (index.internalId() == kGroupRowId) {
        return Qt::ItemIsEnabled;
    }
    Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemNeverHasChildren;
    if (m_editable && index.column() == kValueColumn &&
        static_cast<int>(index.internalId() - 1) == kGeneralGroup) {
        result |= Qt::ItemIsEditable;
    }
    return result;
}

PropertyGridDelegate::PropertyGridDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

QRect PropertyGridDelegate::nextArrow(const QRect& cell) {
    return {cell.right() + 1 - kArrowMargin - kArrowSide, cell.top(), kArrowSide, cell.height()};
}

QRect PropertyGridDelegate::previousArrow(const QRect& cell) {
    return nextArrow(cell).translated(-kArrowSide, 0);
}

void PropertyGridDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
    painter->save();
    if (index.data(PropertyGridModel::kGroupRole).toBool()) {
        paintGroup(painter, option, index);
        painter->restore();
        return;
    }
    const ThemeTokens& tokens = themeTokens(themeOf(option.palette));
    const bool isName = index.column() == PropertyGridModel::kNameColumn;
    if (option.state.testFlag(QStyle::State_Selected)) {
        painter->fillRect(option.rect, tokens.selection);
    }

    QRect content = option.rect.adjusted(isName ? kNameIndent : kCellPadding, 0, -kCellPadding, 0);
    const QVariant previous = index.data(PropertyGridModel::kPreviousVertexRole);
    if (previous.isValid()) {
        const bool hasNext = index.data(PropertyGridModel::kNextVertexRole).toBool();
        const QRect before = previousArrow(option.rect);
        drawArrow(*painter, before, true, previous.toBool() ? tokens.text : tokens.border);
        drawArrow(*painter, nextArrow(option.rect), false, hasNext ? tokens.text : tokens.border);
        content.setRight(before.left() - kIconSpacing);
    }
    const auto icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        const QRect sample(content.left(), content.center().y() - kSampleHeight / 2 + 1,
                           kSampleWidth, kSampleHeight);
        icon.paint(painter, sample, Qt::AlignCenter);
        content.setLeft(sample.right() + 1 + kIconSpacing);
    }
    painter->setFont(option.font);
    painter->setPen(isName ? tokens.textSecondary : tokens.text);
    painter->drawText(
        content, Qt::AlignLeft | Qt::AlignVCenter,
        option.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight, content.width()));

    painter->setPen(tokens.border);
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    if (!isName) {
        painter->drawLine(option.rect.topLeft(), option.rect.bottomLeft());
    }
    painter->restore();
}

QSize PropertyGridDelegate::sizeHint(const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const {
    return {QStyledItemDelegate::sizeHint(option, index).width(), kRowHeight};
}

QWidget* PropertyGridDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/,
                                            const QModelIndex& index) const {
    const QVariant field = index.data(PropertyGridModel::kFieldRole);
    if (!field.isValid() || index.column() != PropertyGridModel::kValueColumn) {
        return nullptr;
    }
    switch (static_cast<PropertyField>(field.toInt())) {
        case PropertyField::Color: {
            auto* editor = new ColorCombo(parent);
            editor->setObjectName(QString::fromLatin1(kColorEditor));
            connect(editor, &ColorCombo::valueChosen, this, &PropertyGridDelegate::colorChosen);
            return editor;
        }
        case PropertyField::Layer: {
            auto* editor = new LayerCombo(parent);
            editor->setObjectName(QString::fromLatin1(kLayerEditor));
            connect(editor, &LayerCombo::valueChosen, this, &PropertyGridDelegate::layerChosen);
            return editor;
        }
        case PropertyField::Linetype: {
            auto* editor = new LinetypeCombo(parent);
            editor->setObjectName(QString::fromLatin1(kLinetypeEditor));
            connect(editor, &LinetypeCombo::valueChosen, this,
                    &PropertyGridDelegate::linetypeChosen);
            return editor;
        }
        case PropertyField::LinetypeScale: {
            auto* editor = new QLineEdit(parent);
            editor->setObjectName(QString::fromLatin1(kLinetypeScaleEditor));
            connect(editor, &QLineEdit::editingFinished, this,
                    &PropertyGridDelegate::finishLinetypeScale);
            return editor;
        }
        case PropertyField::Lineweight: {
            auto* editor = new LineweightCombo(parent);
            editor->setObjectName(QString::fromLatin1(kLineweightEditor));
            connect(editor, &LineweightCombo::valueChosen, this,
                    &PropertyGridDelegate::lineweightChosen);
            return editor;
        }
    }
    return nullptr;
}

void PropertyGridDelegate::finishLinetypeScale() {
    auto* editor = qobject_cast<QLineEdit*>(sender());
    // A line edit also finishes when it loses the focus or closes: only a
    // text the user changed is a value.
    if (editor == nullptr || !editor->isModified()) {
        return;
    }
    editor->setModified(false);
    Q_EMIT linetypeScaleEntered(editor->text().trimmed());
}

void PropertyGridDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    const auto* model = qobject_cast<const PropertyGridModel*>(index.model());
    if (model == nullptr) {
        return;
    }
    const ShownProperties& shown = model->shown();
    const core::Document& document = model->document();
    // The setters of the controls never emit valueChosen (0.6-P21).
    if (auto* color = qobject_cast<ColorCombo*>(editor)) {
        if (shown.color) {
            color->setValue(*shown.color);
        } else {
            color->setVaries();
        }
    } else if (auto* layer = qobject_cast<LayerCombo*>(editor)) {
        layer->setDocument(&document);
        if (shown.layer) {
            layer->setValue(*shown.layer);
        } else {
            layer->setVaries();
        }
    } else if (auto* linetype = qobject_cast<LinetypeCombo*>(editor)) {
        linetype->setDocument(&document);
        if (shown.linetype) {
            linetype->setValue(*shown.linetype);
        } else {
            linetype->setVaries();
        }
    } else if (auto* lineweight = qobject_cast<LineweightCombo*>(editor)) {
        lineweight->setDocument(&document);
        if (shown.lineweight) {
            lineweight->setValue(*shown.lineweight);
        } else {
            lineweight->setVaries();
        }
    } else if (auto* scale = qobject_cast<QLineEdit*>(editor)) {
        scale->setText(index.data().toString());
        scale->setModified(false);
    }
}

void PropertyGridDelegate::setModelData(QWidget* /*editor*/, QAbstractItemModel* /*model*/,
                                        const QModelIndex& /*index*/) const {
    // Nothing: the editors report the user's choice with the signals, and
    // the panel changes the document, which the model shows again.
}

void PropertyGridDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                                                const QModelIndex& /*index*/) const {
    editor->setGeometry(option.rect);
}

bool PropertyGridDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
                                       const QStyleOptionViewItem& option,
                                       const QModelIndex& index) {
    if (event->type() == QEvent::MouseButtonRelease &&
        index.data(PropertyGridModel::kPreviousVertexRole).isValid()) {
        const auto* mouse = dynamic_cast<const QMouseEvent*>(event);
        if (mouse != nullptr && mouse->button() == Qt::LeftButton) {
            const QPoint at = mouse->position().toPoint();
            if (previousArrow(option.rect).contains(at)) {
                Q_EMIT previousVertexRequested();
                return true;
            }
            if (nextArrow(option.rect).contains(at)) {
                Q_EMIT nextVertexRequested();
                return true;
            }
        }
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

}  // namespace tg::app
