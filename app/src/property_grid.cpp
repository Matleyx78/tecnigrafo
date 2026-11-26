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
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
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
const char* const kOwnValueEditor = "propertiesOwnValue";
const char* const kChoiceEditor = "propertiesOwnChoice";

// What a value shared by no two of the entities shows, as in AutoCAD.
const char* const kVaries = "*VARIES*";

QString variesText() {
    return QString::fromLatin1(kVaries);
}

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

/// The label of the own property `name` (core::ownProperties), with the
/// names of AutoCAD's panel; the name itself for one this table lacks.
QString ownPropertyLabel(const core::Entity& entity, std::string_view name) {
    struct Label {
        std::string_view name;
        const char* label;
    };

    static constexpr std::array kLabels{
        Label{"center", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Center")},
        Label{"major-radius", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Major radius")},
        Label{"minor-radius", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Minor radius")},
        Label{"ratio", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Radius ratio")},
        Label{"start-angle", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Start angle")},
        Label{"end-angle", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "End angle")},
        Label{"~area", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Area")},
        Label{"pattern", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Pattern name")},
        Label{"scale", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Scale")},
        Label{"angle", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Angle")},
        Label{"origin", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Origin")},
        Label{"islands", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Island detection")},
        Label{"style", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Style")},
        Label{"text", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Text override")},
        Label{"text-rotation", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Text rotation")},
        Label{"~measurement", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Measurement")},
        Label{"value", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Contents")},
        Label{"attachment", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Justify")},
        Label{"width", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Width")},
        Label{"height", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Height")},
        Label{"rotation", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Rotation")},
        Label{"line-spacing", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Line space factor")},
        Label{"line-spacing-style",
              QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Line space style")},
        Label{"tag", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Tag")},
        Label{"prompt", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Prompt")},
        Label{"default", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Value")},
        Label{"invisible", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Invisible")},
        Label{"constant", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Constant")},
        Label{"verify", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Verify")},
        Label{"preset", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Preset")},
        Label{"lock-position", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Lock position")},
        Label{"block", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Name")},
        Label{"insertion", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Position")},
        Label{"scale-z", QT_TRANSLATE_NOOP("tg::app::PropertyGridModel", "Scale Z")},
    };
    // The style of a dimension is its dimension style.
    if (name == "style" && std::holds_alternative<core::Dimension>(entity)) {
        return PropertyGridModel::tr("Dim style");
    }
    for (const Label& label : kLabels) {
        if (label.name == name) {
            return PropertyGridModel::tr(label.label);
        }
    }
    return QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size()));
}

QString yesNo(bool value) {
    return value ? PropertyGridModel::tr("Yes") : PropertyGridModel::tr("No");
}

/// The text of the value of an own property that is not a point.
QString ownValueText(const core::OwnProperty& property) {
    return std::visit(
        [&property](const auto& value) -> QString {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, double>) {
                return property.kind == core::PropertyKind::Angle ? angleText(value)
                                                                  : lengthText(value);
            } else if constexpr (std::is_same_v<Value, core::Vec2>) {
                return lengthText(value.x) + QStringLiteral(", ") + lengthText(value.y);
            } else if constexpr (std::is_same_v<Value, std::string>) {
                return QString::fromStdString(value);
            } else {
                return yesNo(value);
            }
        },
        property.value);
}

struct GeometryRows {
    std::vector<OwnPropertyRow> rows;
    /// The row of Current Vertex, or -1.
    int vertexRow{-1};
    QString title;
    std::vector<OwnPropertyRow> attributes;
};

/// A row only shown.
OwnPropertyRow shownRow(const QString& name, const QString& value) {
    OwnPropertyRow row;
    row.name = name;
    row.value = value;
    return row;
}

/// The rows X and Y of the point property `property`, named `label` X and
/// `label` Y.
void addPointRows(std::vector<OwnPropertyRow>& rows, const core::OwnProperty& property,
                  const QString& label) {
    const auto* point = std::get_if<core::Vec2>(&property.value);
    if (point == nullptr) {
        return;
    }
    for (int component = 0; component < 2; ++component) {
        OwnPropertyRow row;
        row.name = label + (component == 0 ? QStringLiteral(" X") : QStringLiteral(" Y"));
        row.value = lengthText(component == 0 ? point->x : point->y);
        if (!property.readOnly) {
            row.property = property.name;
        }
        row.kind = property.kind;
        row.component = component;
        rows.push_back(std::move(row));
    }
}

/// The name of the attribute of an INSERT, which ownProperties() gives once
/// per tag (PT24).
constexpr std::string_view kAttributeProperty = "attribute";

/// The rows of the own properties of a type of the 0.8 (core::ownProperties).
GeometryRows ownRows(const core::Document& document, const core::EntityRecord& record, int vertex) {
    GeometryRows result;
    const core::Entity& entity = record.entity;
    if (std::holds_alternative<core::Hatch>(entity)) {
        result.title = PropertyGridModel::tr("Pattern");
    } else if (std::holds_alternative<core::MText>(entity) ||
               std::holds_alternative<core::AttributeDefinition>(entity) ||
               std::holds_alternative<core::Dimension>(entity)) {
        result.title = PropertyGridModel::tr("Text");
    }
    const std::vector<core::OwnProperty> properties = core::ownProperties(document, record);
    const auto* solid = std::get_if<core::Solid>(&entity);
    const std::string currentVertex = "vertex" + std::to_string(vertex + 1);
    for (const core::OwnProperty& property : properties) {
        if (property.name == kAttributeProperty) {
            continue;
        }
        if (solid != nullptr) {
            // The vertices, browsed as those of a polyline: only the current
            // one, where the first vertex property is.
            if (property.name == "vertex1") {
                result.vertexRow = static_cast<int>(result.rows.size());
                result.rows.push_back(
                    shownRow(PropertyGridModel::tr("Current Vertex"), QString::number(vertex + 1)));
            }
            if (property.name == currentVertex) {
                addPointRows(result.rows, property, PropertyGridModel::tr("Vertex"));
            }
            continue;
        }
        const QString label = ownPropertyLabel(entity, property.name);
        if (property.kind == core::PropertyKind::Point) {
            addPointRows(result.rows, property, label);
            continue;
        }
        OwnPropertyRow row = shownRow(label, ownValueText(property));
        if (!property.readOnly) {
            row.property = property.name;
        }
        row.kind = property.kind;
        if (property.kind == core::PropertyKind::Choice) {
            for (const std::string& choice : property.choices) {
                row.choices.append(QString::fromStdString(choice));
            }
        } else if (property.kind == core::PropertyKind::Flag) {
            row.choices = {yesNo(true), yesNo(false)};
        }
        result.rows.push_back(std::move(row));
    }
    if (const auto* insert = std::get_if<core::Insert>(&entity)) {
        // One row per tag, that of the first ATTRIB with it, as
        // ownProperties() gives them and withOwnProperty() changes them.
        for (std::size_t i = 0; i < insert->attributes.size(); ++i) {
            const core::Attribute& attribute = insert->attributes[i];
            const bool first =
                std::none_of(insert->attributes.begin(),
                             insert->attributes.begin() + static_cast<std::ptrdiff_t>(i),
                             [&attribute](const core::Attribute& before) {
                                 return core::sameTableName(before.tag, attribute.tag);
                             });
            if (!first) {
                continue;
            }
            OwnPropertyRow row = shownRow(QString::fromStdString(attribute.tag),
                                          QString::fromStdString(attribute.text.value));
            row.property = kAttributeProperty;
            row.kind = core::PropertyKind::Text;
            row.tag = attribute.tag;
            result.attributes.push_back(std::move(row));
        }
    }
    return result;
}

/// The rows of the group Geometry of `record`: for the types of before the
/// 0.8 its main coordinates, with the names of AutoCAD's panel, and for a
/// polyline the vertex `vertex`; for the others, its own properties.
GeometryRows geometryRows(const core::Document& document, const core::EntityRecord& record,
                          int vertex) {
    GeometryRows result;
    result.title = PropertyGridModel::tr("Geometry");
    const core::Entity& entity = record.entity;
    const auto add = [&result](const QString& name, const QString& value) {
        result.rows.push_back(shownRow(name, value));
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
        add(PropertyGridModel::tr("Closed"), yesNo(polyline->isClosed()));
        add(PropertyGridModel::tr("Length"), lengthText(polyline->length()));
    } else if (const auto* text = std::get_if<core::Text>(&entity)) {
        add(PropertyGridModel::tr("Position X"), lengthText(text->position.x));
        add(PropertyGridModel::tr("Position Y"), lengthText(text->position.y));
        add(PropertyGridModel::tr("Height"), lengthText(text->height));
        add(PropertyGridModel::tr("Contents"), QString::fromStdString(text->value));
    } else {
        GeometryRows own = ownRows(document, record, vertex);
        if (own.title.isEmpty()) {
            own.title = result.title;
        }
        return own;
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
    const bool same = record != nullptr && m_record.has_value() && record->id == m_record->id;
    if (record != nullptr) {
        m_record = *record;
    } else {
        m_record.reset();
    }
    if (!same) {
        m_vertex = 0;
    }
    m_vertex = std::max(0, std::min(m_vertex, vertexCount() - 1));
    showGeometry();
}

int PropertyGridModel::vertexCount() const {
    if (!m_record) {
        return 0;
    }
    if (const auto* polyline = std::get_if<core::Polyline>(&m_record->entity)) {
        return static_cast<int>(polyline->vertexCount());
    }
    if (const auto* solid = std::get_if<core::Solid>(&m_record->entity)) {
        return static_cast<int>(solid->vertices.size());
    }
    return 0;
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

const std::vector<OwnPropertyRow>& PropertyGridModel::rows(int group) const {
    return group == kAttributesGroup ? m_attributeRows : m_geometryRows;
}

const OwnPropertyRow* PropertyGridModel::row(int group, int row) const {
    if (group != kGeometryGroup && group != kAttributesGroup) {
        return nullptr;
    }
    const std::vector<OwnPropertyRow>& list = rows(group);
    if (row < 0 || row >= static_cast<int>(list.size())) {
        return nullptr;
    }
    return &list.at(static_cast<std::size_t>(row));
}

int PropertyGridModel::rowNamed(int group, const QString& name) const {
    if (group != kGeometryGroup && group != kAttributesGroup) {
        return -1;
    }
    const std::vector<OwnPropertyRow>& list = rows(group);
    for (std::size_t i = 0; i < list.size(); ++i) {
        if (list[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

QModelIndex PropertyGridModel::rowValueIndex(int group, int row) const {
    return index(row, kValueColumn, groupIndex(group));
}

void PropertyGridModel::showGeometry() {
    GeometryRows next = m_record ? geometryRows(*m_document, *m_record, m_vertex) : GeometryRows{};
    if (!next.title.isEmpty() && next.title != m_geometryTitle) {
        m_geometryTitle = next.title;
        const QModelIndex title = groupIndex(kGeometryGroup);
        Q_EMIT dataChanged(title, title);
    }
    m_vertexRow = next.vertexRow;
    m_geometry.clear();
    for (const OwnPropertyRow& row : next.rows) {
        m_geometry.append({row.name, row.value});
    }
    showRows(kGeometryGroup, std::move(next.rows));
    showRows(kAttributesGroup, std::move(next.attributes));
}

void PropertyGridModel::showRows(int group, std::vector<OwnPropertyRow> next) {
    std::vector<OwnPropertyRow>& current =
        group == kAttributesGroup ? m_attributeRows : m_geometryRows;
    const QModelIndex parent = groupIndex(group);
    const bool sameShape =
        next.size() == current.size() &&
        std::equal(next.begin(), next.end(), current.begin(),
                   [](const OwnPropertyRow& a, const OwnPropertyRow& b) {
                       return a.name == b.name && a.property == b.property && a.kind == b.kind;
                   });
    if (sameShape) {
        // Same rows: only the values change, and the view keeps its state.
        current = std::move(next);
        // One row at a time: a view gives an open editor the new value only
        // for a change of a single cell.
        for (int row = 0; row < static_cast<int>(current.size()); ++row) {
            Q_EMIT dataChanged(index(row, kNameColumn, parent), index(row, kValueColumn, parent));
        }
        return;
    }
    if (!current.empty()) {
        beginRemoveRows(parent, 0, static_cast<int>(current.size()) - 1);
        current.clear();
        endRemoveRows();
    }
    if (!next.empty()) {
        beginInsertRows(parent, 0, static_cast<int>(next.size()) - 1);
        current = std::move(next);
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
    if (parent.row() == kGeneralGroup) {
        return static_cast<int>(kFields.size());
    }
    return static_cast<int>(rows(parent.row()).size());
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
            switch (index.row()) {
                case kGeneralGroup:
                    return tr("General");
                case kGeometryGroup:
                    return m_geometryTitle.isEmpty() ? tr("Geometry") : m_geometryTitle;
                default:
                    return tr("Attributes");
            }
        }
        return {};
    }
    if (role == kGroupRole) {
        return false;
    }
    if (static_cast<int>(index.internalId() - 1) == kGeneralGroup) {
        return generalData(kFields.at(static_cast<std::size_t>(index.row())), index.column(), role);
    }
    return rowData(static_cast<int>(index.internalId() - 1), index.row(), index.column(), role);
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

QVariant PropertyGridModel::rowData(int group, int row, int column, int role) const {
    const OwnPropertyRow* shown = this->row(group, row);
    if (shown == nullptr) {
        return {};
    }
    if (role == Qt::DisplayRole) {
        return column == kNameColumn ? shown->name : shown->value;
    }
    if (column != kValueColumn) {
        return {};
    }
    if (role == Qt::ToolTipRole) {
        return shown->value;
    }
    if (group == kGeometryGroup && row == m_vertexRow) {
        if (role == kPreviousVertexRole) {
            return m_vertex > 0;
        }
        if (role == kNextVertexRole) {
            return m_vertex + 1 < vertexCount();
        }
    }
    if (shown->editable()) {
        if (role == kOwnKindRole) {
            return static_cast<int>(shown->kind);
        }
        if (role == kChoicesRole && !shown->choices.isEmpty()) {
            return shown->choices;
        }
    }
    return {};
}

QString PropertyGridModel::valueText(PropertyField field) const {
    switch (field) {
        case PropertyField::Color:
            return m_shown.color ? colorName(*m_shown.color) : variesText();
        case PropertyField::Layer: {
            if (!m_shown.layer) {
                return variesText();
            }
            const core::Layer* layer = m_document->layer(*m_shown.layer);
            return layer != nullptr ? QString::fromStdString(layer->name) : QString();
        }
        case PropertyField::Linetype: {
            if (!m_shown.linetype) {
                return variesText();
            }
            const core::Linetype* linetype = m_document->linetype(*m_shown.linetype);
            return linetype != nullptr ? QString::fromStdString(linetype->name) : QString();
        }
        case PropertyField::LinetypeScale:
            return m_shown.linetypeScale ? lengthText(*m_shown.linetypeScale) : variesText();
        case PropertyField::Lineweight:
            return m_shown.lineweight ? lineweightName(*m_shown.lineweight) : variesText();
    }
    return {};
}

QVariant PropertyGridModel::valueIcon(PropertyField field) const {
    switch (field) {
        case PropertyField::Color:
            if (m_shown.color) {
                return QVariant::fromValue(colorSampleIcon(*m_shown.color, m_palette));
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
    if (!m_editable || index.column() != kValueColumn) {
        return result;
    }
    const int group = static_cast<int>(index.internalId() - 1);
    const OwnPropertyRow* shown = row(group, index.row());
    if (group == kGeneralGroup || (shown != nullptr && shown->editable())) {
        result |= Qt::ItemIsEditable;
    }
    return result;
}

bool PropertyGridModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (role != Qt::EditRole || !m_editable || !index.isValid()) {
        return false;
    }
    if (index == valueIndex(PropertyField::LinetypeScale)) {
        Q_EMIT linetypeScaleEntered(value.toString());
        return true;
    }
    if (index.internalId() == kGroupRowId || index.column() != kValueColumn) {
        return false;
    }
    const int group = static_cast<int>(index.internalId() - 1);
    const OwnPropertyRow* shown = row(group, index.row());
    if (shown == nullptr || !shown->editable()) {
        return false;
    }
    Q_EMIT ownValueEntered(group, index.row(), value.toString());
    return true;
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
        // An arrow that leads nowhere is off, in the secondary text, but
        // still in sight (docs/adr/0039, area 8).
        drawArrow(*painter, before, true, previous.toBool() ? tokens.text : tokens.textSecondary);
        drawArrow(*painter, nextArrow(option.rect), false,
                  hasNext ? tokens.text : tokens.textSecondary);
        content.setRight(before.left() - kIconSpacing);
    }
    const auto icon = index.data(Qt::DecorationRole).value<QIcon>();
    if (!icon.isNull()) {
        // A square sample (the colour) keeps its size; a line is drawn in
        // the strip of the line samples (docs/adr/0039, area 8).
        const QSize own = icon.actualSize(QSize(kSampleWidth, kSampleWidth));
        const bool square = own.width() == own.height();
        const int height = square ? own.height() : kSampleHeight;
        const QRect sample(content.left(), content.center().y() - height / 2 + (square ? 0 : 1),
                           square ? own.width() : kSampleWidth, height);
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
    if (index.column() != PropertyGridModel::kValueColumn) {
        return nullptr;
    }
    const QVariant own = index.data(PropertyGridModel::kOwnKindRole);
    if (own.isValid()) {
        const QModelIndex group = index.parent();
        if (index.data(PropertyGridModel::kChoicesRole).isValid()) {
            auto* editor = new ChoiceCombo(group.row(), index.row(), parent);
            editor->setObjectName(QString::fromLatin1(kChoiceEditor));
            connect(editor, &ChoiceCombo::valueChosen, this, &PropertyGridDelegate::ownValueChosen);
            return editor;
        }
        auto* editor = new QLineEdit(parent);
        editor->setObjectName(QString::fromLatin1(kOwnValueEditor));
        return editor;
    }
    const QVariant field = index.data(PropertyGridModel::kFieldRole);
    if (!field.isValid()) {
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

void PropertyGridDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
    const auto* model = qobject_cast<const PropertyGridModel*>(index.model());
    if (model == nullptr) {
        return;
    }
    const ShownProperties& shown = model->shown();
    const core::Document& document = model->document();
    // The setters of the controls never emit valueChosen (0.6-P21).
    if (auto* choice = qobject_cast<ChoiceCombo*>(editor)) {
        choice->setValue(index.data(PropertyGridModel::kChoicesRole).toStringList(),
                         index.data().toString());
    } else if (auto* color = qobject_cast<ColorCombo*>(editor)) {
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
    } else if (auto* typed = qobject_cast<QLineEdit*>(editor)) {
        typed->setText(index.data().toString());
        typed->setModified(false);
    }
}

void PropertyGridDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                        const QModelIndex& index) const {
    // The view commits on Enter, Tab and the focus leaving the cell, never
    // on Esc. The combos have reported their choice already; the scale and
    // the own values are committed only if the user changed their text.
    auto* typed = qobject_cast<QLineEdit*>(editor);
    if (typed == nullptr || !typed->isModified()) {
        return;
    }
    typed->setModified(false);
    // Untrimmed: the spaces of a text are its own; a number ignores them.
    model->setData(index, typed->text(), Qt::EditRole);
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
