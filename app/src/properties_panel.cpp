// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "properties_panel.hpp"

#include "property_widgets.hpp"
#include "quick_select_dialog.hpp"
#include "tg/cmd/value_format.hpp"
#include "theme.hpp"

#include <QAction>
#include <QComboBox>
#include <QEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <QVariant>

#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <utility>
#include <variant>

namespace tg::app {
namespace {

// The side of the icons beside the controls, in pixels.
constexpr int kIconSide = 16;
// The width that shows a layer name and its states in the drop-down list.
constexpr int kComboWidth = 160;

// The name of the transaction of every change ("Undo Properties").
const char* const kTransactionName = "Properties";

// What a value shared by no two of the entities shows, as in AutoCAD.
const char* const kVaries = "*VARIES*";

// docs/ICONE.md: the icons of the properties (0.6-P19).
const char* const kColorIcon = "property-color";
const char* const kLinetypeIcon = "property-linetype";
const char* const kLineweightIcon = "property-lineweight";

// The DXF types of core::Entity, in the order of its alternatives.
constexpr std::array<std::string_view, std::variant_size_v<core::Entity>> kDxfTypes{
    "LWPOLYLINE", "LINE", "CIRCLE", "ARC", "TEXT"};

std::string_view dxfType(const core::Entity& entity) {
    return kDxfTypes.at(entity.index());
}

QString typeName(std::string_view type) {
    if (type == "LWPOLYLINE") {
        return PropertiesPanel::tr("Polyline");
    }
    if (type == "LINE") {
        return PropertiesPanel::tr("Line");
    }
    if (type == "CIRCLE") {
        return PropertiesPanel::tr("Circle");
    }
    if (type == "ARC") {
        return PropertiesPanel::tr("Arc");
    }
    return PropertiesPanel::tr("Text");
}

QString lengthText(double value) {
    return QString::fromStdString(cmd::formatLength(value));
}

QString angleText(double radians) {
    return QString::fromStdString(cmd::formatAngle(radians));
}

/// The value of `field` that every entity of `ids` has, or std::nullopt if
/// they differ. An entity the document lacks is not counted.
template <typename Value>
std::optional<Value> sharedValue(const core::Document& document,
                                 const std::vector<core::EntityId>& ids,
                                 Value core::EntityProperties::*field) {
    std::optional<Value> shared;
    for (const core::EntityId id : ids) {
        const core::EntityRecord* record = document.find(id);
        if (record == nullptr) {
            continue;
        }
        const Value& value = record->properties.*field;
        if (!shared) {
            shared = value;
        } else if (!(*shared == value)) {
            return std::nullopt;
        }
    }
    return shared;
}

/// A label that shows the icon `name` beside a control.
QLabel* iconLabel(const QString& name, QWidget* parent, QList<QPair<QLabel*, QString>>& icons) {
    auto* label = new QLabel(parent);
    label->setObjectName(name);
    icons.append({label, name});
    return label;
}

void drawIcons(const QList<QPair<QLabel*, QString>>& icons, const QPalette& palette) {
    for (const auto& [label, name] : icons) {
        label->setPixmap(themedIcon(name, palette).pixmap(kIconSide, kIconSide));
    }
}

/// `control` with the icon `name` before it, in one row.
QWidget* withIcon(QWidget* control, const QString& name, QWidget* parent,
                  QList<QPair<QLabel*, QString>>& icons) {
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(iconLabel(name, row, icons));
    layout->addWidget(control, 1);
    return row;
}

/// The rows of the section Geometry of `entity`: its main coordinates, with
/// the names of AutoCAD's panel.
QList<QPair<QString, QString>> geometryRows(const core::Entity& entity) {
    QList<QPair<QString, QString>> rows;
    const auto add = [&rows](const QString& name, const QString& value) {
        rows.append({name, value});
    };
    if (const auto* line = std::get_if<core::Line>(&entity)) {
        add(PropertiesPanel::tr("Start X"), lengthText(line->start.x));
        add(PropertiesPanel::tr("Start Y"), lengthText(line->start.y));
        add(PropertiesPanel::tr("End X"), lengthText(line->end.x));
        add(PropertiesPanel::tr("End Y"), lengthText(line->end.y));
        add(PropertiesPanel::tr("Length"), lengthText(line->length()));
    } else if (const auto* circle = std::get_if<core::Circle>(&entity)) {
        add(PropertiesPanel::tr("Center X"), lengthText(circle->center.x));
        add(PropertiesPanel::tr("Center Y"), lengthText(circle->center.y));
        add(PropertiesPanel::tr("Radius"), lengthText(circle->radius));
    } else if (const auto* arc = std::get_if<core::Arc>(&entity)) {
        add(PropertiesPanel::tr("Center X"), lengthText(arc->center.x));
        add(PropertiesPanel::tr("Center Y"), lengthText(arc->center.y));
        add(PropertiesPanel::tr("Radius"), lengthText(arc->radius));
        add(PropertiesPanel::tr("Start angle"), angleText(arc->startAngle));
        add(PropertiesPanel::tr("End angle"), angleText(arc->endAngle));
    } else if (const auto* polyline = std::get_if<core::Polyline>(&entity)) {
        add(PropertiesPanel::tr("Vertices"),
            QString::number(static_cast<qulonglong>(polyline->vertexCount())));
        if (!polyline->vertices().empty()) {
            const core::Vec2 first = polyline->vertices().front().position;
            add(PropertiesPanel::tr("Vertex 1 X"), lengthText(first.x));
            add(PropertiesPanel::tr("Vertex 1 Y"), lengthText(first.y));
        }
        add(PropertiesPanel::tr("Closed"),
            polyline->isClosed() ? PropertiesPanel::tr("Yes") : PropertiesPanel::tr("No"));
        add(PropertiesPanel::tr("Length"), lengthText(polyline->length()));
    } else if (const auto* text = std::get_if<core::Text>(&entity)) {
        add(PropertiesPanel::tr("Position X"), lengthText(text->position.x));
        add(PropertiesPanel::tr("Position Y"), lengthText(text->position.y));
        add(PropertiesPanel::tr("Height"), lengthText(text->height));
        add(PropertiesPanel::tr("Contents"), QString::fromStdString(text->value));
    }
    return rows;
}

}  // namespace

QStringList propertiesPanelIconNames() {
    return {QString::fromLatin1(kColorIcon), QString::fromLatin1(kLinetypeIcon),
            QString::fromLatin1(kLineweightIcon)};
}

PropertiesPanel::PropertiesPanel(edit::Editor& editor, cmd::CommandRunner& runner, QWidget* parent)
    : QDockWidget(tr("Properties"), parent),
      m_editor(editor),
      m_runner(runner),
      m_types(new QComboBox(this)),
      m_general(new QGroupBox(tr("General"), this)),
      m_color(new ColorCombo(this)),
      m_layer(new LayerCombo(this)),
      m_linetype(new LinetypeCombo(this)),
      m_linetypeScale(new QLineEdit(this)),
      m_lineweight(new LineweightCombo(this)),
      m_geometryBox(new QGroupBox(tr("Geometry"), this)),
      m_geometryForm(new QFormLayout(m_geometryBox)),
      m_message(new QLabel(this)) {
    setObjectName(QStringLiteral("propertiesPanel"));

    m_types->setObjectName(QStringLiteral("propertiesType"));
    m_general->setObjectName(QStringLiteral("propertiesGeneral"));
    m_color->setObjectName(QStringLiteral("propertiesColor"));
    m_layer->setObjectName(QStringLiteral("propertiesLayer"));
    m_linetype->setObjectName(QStringLiteral("propertiesLinetype"));
    m_linetypeScale->setObjectName(QStringLiteral("propertiesLinetypeScale"));
    m_lineweight->setObjectName(QStringLiteral("propertiesLineweight"));
    m_geometryBox->setObjectName(QStringLiteral("propertiesGeometry"));
    m_message->setObjectName(QStringLiteral("propertiesMessage"));
    m_message->setWordWrap(true);
    m_layer->setMinimumWidth(kComboWidth);

    auto* general = new QFormLayout(m_general);
    general->addRow(tr("Color"),
                    withIcon(m_color, QString::fromLatin1(kColorIcon), m_general, m_icons));
    general->addRow(tr("Layer"), m_layer);
    general->addRow(tr("Linetype"),
                    withIcon(m_linetype, QString::fromLatin1(kLinetypeIcon), m_general, m_icons));
    general->addRow(tr("Linetype scale"), m_linetypeScale);
    general->addRow(tr("Lineweight"), withIcon(m_lineweight, QString::fromLatin1(kLineweightIcon),
                                               m_general, m_icons));

    auto* content = new QWidget(this);
    auto* layout = new QVBoxLayout(content);
    layout->addWidget(m_types);
    layout->addWidget(m_general);
    layout->addWidget(m_geometryBox);
    layout->addWidget(m_message);
    layout->addStretch(1);
    setWidget(content);

    // Only the user's choices change the drawing: the setters of the
    // controls emit nothing (0.6-P21), and the filter listens to activated.
    connect(m_types, &QComboBox::activated, this, [this](int index) {
        setTypeFilter(m_types->itemData(index).toString().toStdString());
    });
    connect(m_color, &ColorCombo::valueChosen, this,
            [this](core::Color value) { setColor(value); });
    connect(m_layer, &LayerCombo::valueChosen, this,
            [this](core::LayerId value) { setLayer(value); });
    connect(m_linetype, &LinetypeCombo::valueChosen, this,
            [this](core::LinetypeId value) { setLinetype(value); });
    connect(m_lineweight, &LineweightCombo::valueChosen, this,
            [this](int value) { setLineweight(value); });
    connect(m_linetypeScale, &QLineEdit::editingFinished, this, [this] {
        if (!m_linetypeScale->isModified()) {
            return;
        }
        m_linetypeScale->setModified(false);
        bool ok = false;
        const double value = m_linetypeScale->text().trimmed().toDouble(&ok);
        if (!ok) {
            refuse(tr("Requires a positive number."));
            return;
        }
        setLinetypeScale(value);
    });

    updateIcons();
    refresh();
}

PropertiesPanel::~PropertiesPanel() {
    // A window destroys its drawing and its runner before its children: a
    // control that reports its own teardown (a line edit losing the focus)
    // must not reach a drawing that is gone.
    m_linetypeScale->disconnect(this);
    m_types->disconnect(this);
}

void PropertiesPanel::setQuickSelectAction(QAction* action) {
    m_quickSelectAction = action;
    if (action != nullptr) {
        connect(action, &QAction::triggered, this, [this] { showQuickSelect(m_idleSelection); });
        // The window destroys its actions before its panels.
        connect(action, &QObject::destroyed, this, [this] { m_quickSelectAction = nullptr; });
    }
    updateEnabled();
}

void PropertiesPanel::setEditingEnabled(bool enabled) {
    // The window calls this after every input: do nothing if nothing changed.
    if (enabled == m_editingEnabled) {
        return;
    }
    m_editingEnabled = enabled;
    updateEnabled();
    Q_EMIT refreshed();
}

bool PropertiesPanel::canEdit() const {
    return m_editingEnabled && !m_editor.inTransaction() && !m_runner.isActive();
}

void PropertiesPanel::setMessage(const QString& text) {
    m_lastMessage = text;
    m_message->setText(text);
}

bool PropertiesPanel::refuse(const QString& text) {
    setMessage(text);
    Q_EMIT messageShown(text);
    // The controls that offered the refused value show the real one again.
    refresh();
    return false;
}

void PropertiesPanel::refresh() {
    readSelection();
    showTypes();
    readShown();
    showValues();
    showGeometry();
    updateEnabled();
    Q_EMIT refreshed();
}

void PropertiesPanel::showAndFocus() {
    show();
    raise();
    if (isFloating()) {
        activateWindow();
    }
    m_types->setFocus(Qt::OtherFocusReason);
}

void PropertiesPanel::readSelection() {
    const core::Document& doc = document();
    m_selection.clear();
    for (const core::EntityId id : m_runner.selection().ids()) {
        const core::EntityRecord* record = doc.find(id);
        if (record == nullptr) {
            continue;
        }
        // The runner never selects them, but a layer may be locked later.
        const core::Layer* layer = doc.layer(record->properties.layer);
        if (layer != nullptr && layer->locked) {
            continue;
        }
        m_selection.push_back(id);
    }
    if (!m_runner.isActive()) {
        m_idleSelection = m_selection;
    }
}

std::vector<core::EntityId> PropertiesPanel::targets() const {
    if (m_typeFilter.empty()) {
        return m_selection;
    }
    std::vector<core::EntityId> result;
    for (const core::EntityId id : m_selection) {
        const core::EntityRecord* record = document().find(id);
        if (record != nullptr && dxfType(record->entity) == m_typeFilter) {
            result.push_back(id);
        }
    }
    return result;
}

void PropertiesPanel::showTypes() {
    // How many selected entities each type has, in the order of core::Entity.
    std::array<std::size_t, kDxfTypes.size()> counts{};
    for (const core::EntityId id : m_selection) {
        if (const core::EntityRecord* record = document().find(id)) {
            ++counts.at(record->entity.index());
        }
    }
    std::size_t kinds = 0;
    bool filterFound = false;
    for (std::size_t index = 0; index < counts.size(); ++index) {
        if (counts.at(index) > 0) {
            ++kinds;
            filterFound = filterFound || kDxfTypes.at(index) == m_typeFilter;
        }
    }
    if (kinds < 2 || !filterFound) {
        m_typeFilter.clear();
    }

    const QSignalBlocker blocker(m_types);
    m_types->clear();
    const auto label = [](const QString& name, std::size_t count) {
        return PropertiesPanel::tr("%1 (%2)").arg(name,
                                                  QString::number(static_cast<qulonglong>(count)));
    };
    if (m_selection.empty()) {
        m_types->addItem(tr("No selection"), QString());
    } else if (kinds > 1) {
        m_types->addItem(label(tr("All"), m_selection.size()), QString());
    }
    for (std::size_t index = 0; index < counts.size(); ++index) {
        if (counts.at(index) == 0) {
            continue;
        }
        const std::string_view type = kDxfTypes.at(index);
        const QString name = typeName(type);
        const QString text = counts.at(index) == 1 ? name : label(name, counts.at(index));
        m_types->addItem(text,
                         QString::fromLatin1(type.data(), static_cast<qsizetype>(type.size())));
    }
    const int current =
        m_typeFilter.empty() ? 0 : m_types->findData(QString::fromStdString(m_typeFilter));
    m_types->setCurrentIndex(current < 0 ? 0 : current);
}

QStringList PropertiesPanel::typeLabels() const {
    QStringList labels;
    for (int index = 0; index < m_types->count(); ++index) {
        labels.append(m_types->itemText(index));
    }
    return labels;
}

bool PropertiesPanel::setTypeFilter(const std::string& type) {
    if (!type.empty()) {
        bool found = false;
        for (const core::EntityId id : m_selection) {
            const core::EntityRecord* record = document().find(id);
            found = found || (record != nullptr && dxfType(record->entity) == type);
        }
        if (!found) {
            return false;
        }
    }
    m_typeFilter = type;
    refresh();
    return true;
}

void PropertiesPanel::readShown() {
    const core::Document& doc = document();
    if (m_selection.empty()) {
        const core::CurrentProperties& current = doc.current();
        m_shown = {current.color, current.layer, current.linetype, current.linetypeScale,
                   current.lineweight};
        return;
    }
    const std::vector<core::EntityId> ids = targets();
    m_shown.color = sharedValue(doc, ids, &core::EntityProperties::color);
    m_shown.layer = sharedValue(doc, ids, &core::EntityProperties::layer);
    m_shown.linetype = sharedValue(doc, ids, &core::EntityProperties::linetype);
    m_shown.linetypeScale = sharedValue(doc, ids, &core::EntityProperties::linetypeScale);
    m_shown.lineweight = sharedValue(doc, ids, &core::EntityProperties::lineweight);
}

void PropertiesPanel::showValues() {
    const core::Document& doc = document();
    // The setters of the controls never emit valueChosen.
    if (m_shown.color) {
        m_color->setValue(*m_shown.color);
    } else {
        m_color->setVaries();
    }
    m_layer->setDocument(&doc);
    if (m_shown.layer) {
        m_layer->setValue(*m_shown.layer);
    } else {
        m_layer->setVaries();
    }
    m_linetype->setDocument(&doc);
    if (m_shown.linetype) {
        m_linetype->setValue(*m_shown.linetype);
    } else {
        m_linetype->setVaries();
    }
    m_lineweight->setDocument(&doc);
    if (m_shown.lineweight) {
        m_lineweight->setValue(*m_shown.lineweight);
    } else {
        m_lineweight->setVaries();
    }
    const QSignalBlocker blocker(m_linetypeScale);
    m_linetypeScale->setText(m_shown.linetypeScale ? lengthText(*m_shown.linetypeScale)
                                                   : QString::fromLatin1(kVaries));
    m_linetypeScale->setModified(false);
}

void PropertiesPanel::showGeometry() {
    m_geometry.clear();
    const std::vector<core::EntityId> ids = targets();
    if (ids.size() == 1) {
        if (const core::EntityRecord* record = document().find(ids.front())) {
            m_geometry = geometryRows(record->entity);
        }
    }
    while (m_geometryForm->rowCount() > 0) {
        m_geometryForm->removeRow(0);
    }
    for (const auto& [name, value] : m_geometry) {
        auto* field = new QLabel(value, m_geometryBox);
        field->setTextInteractionFlags(Qt::TextSelectableByMouse);
        m_geometryForm->addRow(name, field);
    }
    m_geometryBox->setVisible(!m_geometry.isEmpty());
}

void PropertiesPanel::updateEnabled() {
    const bool editable = m_editingEnabled && !m_runner.isActive();
    m_types->setEnabled(m_types->count() > 1);
    m_general->setEnabled(editable);
    if (m_quickSelectAction != nullptr) {
        m_quickSelectAction->setEnabled(editable);
    }
}

void PropertiesPanel::updateIcons() {
    drawIcons(m_icons, palette());
}

bool PropertiesPanel::change(const std::function<void(core::EntityProperties&)>& edit) {
    if (!canEdit()) {
        return false;
    }
    const bool hasSelection = !m_selection.empty();
    const std::vector<core::EntityId> ids = targets();
    m_editor.beginTransaction(kTransactionName);
    bool done = false;
    if (!hasSelection) {
        core::CurrentProperties current = document().current();
        edit(current);
        done = m_editor.setCurrent(current);
    } else {
        for (const core::EntityId id : ids) {
            const core::EntityRecord* record = document().find(id);
            if (record == nullptr) {
                continue;
            }
            core::EntityProperties properties = record->properties;  // a copy: the record changes
            edit(properties);
            done = m_editor.setProperties(id, properties) || done;
        }
    }
    if (done) {
        m_editor.commit();
        setMessage({});
    } else {
        m_editor.rollback();
    }
    // The owner's listener refreshes too; this keeps a panel without one right.
    refresh();
    if (done) {
        Q_EMIT changed();
    }
    return done;
}

bool PropertiesPanel::setColor(core::Color color) {
    return change([color](core::EntityProperties& properties) { properties.color = color; });
}

bool PropertiesPanel::setLayer(core::LayerId layer) {
    if (!canEdit()) {
        return false;
    }
    const core::Layer* record = document().layer(layer);
    if (record == nullptr) {
        return refuse(tr("Layer not found."));
    }
    // Entities may go to a frozen layer, as with CHPROP; the current layer may not.
    if (m_selection.empty() && record->frozen) {
        return refuse(tr("Cannot set current to a frozen layer."));
    }
    return change([layer](core::EntityProperties& properties) { properties.layer = layer; });
}

bool PropertiesPanel::setLinetype(core::LinetypeId linetype) {
    if (!canEdit()) {
        return false;
    }
    if (document().linetype(linetype) == nullptr) {
        return refuse(tr("Linetype not found."));
    }
    return change(
        [linetype](core::EntityProperties& properties) { properties.linetype = linetype; });
}

bool PropertiesPanel::setLinetypeScale(double scale) {
    if (!canEdit()) {
        return false;
    }
    if (!std::isfinite(scale) || scale <= 0.0) {
        return refuse(tr("Requires a positive number."));
    }
    return change(
        [scale](core::EntityProperties& properties) { properties.linetypeScale = scale; });
}

bool PropertiesPanel::setLineweight(int lineweight) {
    if (!canEdit()) {
        return false;
    }
    if (!core::isAllowedLineweight(lineweight)) {
        return refuse(tr("Requires a valid lineweight."));
    }
    return change(
        [lineweight](core::EntityProperties& properties) { properties.lineweight = lineweight; });
}

void PropertiesPanel::showQuickSelect(std::vector<core::EntityId> current) {
    if (m_editor.inTransaction()) {
        return;
    }
    auto* dialog = new QuickSelectDialog(document(), current, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::accepted, this,
            [this, dialog] { applyQuickSelect(dialog->criteria(), dialog->within()); });
    // A typed QSELECT starts a command, which clears the selection (ADR
    // 0008): Cancel gives it back, as the window found it.
    connect(dialog, &QDialog::rejected, this, [this, previous = std::move(current)] {
        if (!previous.empty() && m_runner.selection().isEmpty() &&
            m_runner.replaceSelection(previous)) {
            refresh();
            Q_EMIT changed();
        }
    });
    dialog->open();
}

bool PropertiesPanel::applyQuickSelect(const cmd::QuickSelectCriteria& criteria,
                                       const std::vector<core::EntityId>& within) {
    if (m_runner.isActive()) {
        return false;
    }
    std::vector<core::EntityId> found = cmd::quickSelect(document(), within, criteria);
    const auto count = static_cast<qulonglong>(found.size());
    if (!m_runner.replaceSelection(std::move(found))) {
        return false;
    }
    const QString text = tr("%1 item(s) selected.").arg(count);
    setMessage(text);
    Q_EMIT messageShown(text);
    refresh();
    Q_EMIT changed();
    return true;
}

void PropertiesPanel::changeEvent(QEvent* event) {
    QDockWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        updateIcons();
    }
}

PropertyControls::PropertyControls(PropertiesPanel& panel, QWidget* parent)
    : QWidget(parent),
      m_panel(&panel),
      m_color(new ColorCombo(this)),
      m_linetype(new LinetypeCombo(this)),
      m_lineweight(new LineweightCombo(this)) {
    setObjectName(QStringLiteral("propertyControls"));
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    m_color->setObjectName(QStringLiteral("propertyColorCombo"));
    m_linetype->setObjectName(QStringLiteral("propertyLinetypeCombo"));
    m_lineweight->setObjectName(QStringLiteral("propertyLineweightCombo"));
    const std::array<std::pair<QComboBox*, const char*>, 3> controls{
        {{m_color, kColorIcon}, {m_linetype, kLinetypeIcon}, {m_lineweight, kLineweightIcon}}};
    for (const auto& [combo, icon] : controls) {
        // A click must not take the keyboard away from the drawing and the
        // command line.
        combo->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(iconLabel(QString::fromLatin1(icon), this, m_icons));
        layout->addWidget(combo);
    }
    connect(m_color, &ColorCombo::valueChosen, this, [this](core::Color value) {
        if (m_panel != nullptr) {
            m_panel->setColor(value);
        }
    });
    connect(m_linetype, &LinetypeCombo::valueChosen, this, [this](core::LinetypeId value) {
        if (m_panel != nullptr) {
            m_panel->setLinetype(value);
        }
    });
    connect(m_lineweight, &LineweightCombo::valueChosen, this, [this](int value) {
        if (m_panel != nullptr) {
            m_panel->setLineweight(value);
        }
    });
    connect(&panel, &PropertiesPanel::refreshed, this, [this] { sync(); });
    connect(&panel, &QObject::destroyed, this, [this] { m_panel = nullptr; });
    updateIcons();
    sync();
}

void PropertyControls::sync() {
    if (m_panel == nullptr) {
        return;
    }
    const core::Document& doc = m_panel->document();
    const ShownProperties& shown = m_panel->shown();
    if (shown.color) {
        m_color->setValue(*shown.color);
    } else {
        m_color->setVaries();
    }
    m_linetype->setDocument(&doc);
    if (shown.linetype) {
        m_linetype->setValue(*shown.linetype);
    } else {
        m_linetype->setVaries();
    }
    m_lineweight->setDocument(&doc);
    if (shown.lineweight) {
        m_lineweight->setValue(*shown.lineweight);
    } else {
        m_lineweight->setVaries();
    }
    setEnabled(m_panel->editingEnabled());
}

void PropertyControls::updateIcons() {
    drawIcons(m_icons, palette());
}

void PropertyControls::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        updateIcons();
    }
}

}  // namespace tg::app
