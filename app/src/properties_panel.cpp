// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "properties_panel.hpp"

#include "cad_view.hpp"
#include "property_widgets.hpp"
#include "quick_select_dialog.hpp"
#include "tg/core/math.hpp"
#include "theme.hpp"

#include <QAction>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPalette>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <QVariant>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

namespace tg::app {
namespace {

// The side of the icons beside the controls, in pixels.
constexpr int kIconSide = 16;
// The width of the names in the grid, as in the reference image
// (docs/adr/0039/proprieta.svg).
constexpr int kNameColumnWidth = 124;
// How near the line between names and values a press starts a drag, and how
// narrow the drag leaves either column, in pixels.
constexpr int kDividerGrip = 3;
constexpr int kMinColumnWidth = 40;
// The space around the controls of the panel and between them.
constexpr int kMargin = 4;
constexpr int kSpacing = 2;
// The combos of the controls one under the other, in the Properties group
// of the ribbon, as wide as in the reference image (docs/adr/0039, area 3),
// with room for longer names in other languages; the icons keep this space
// from them.
constexpr int kStackedComboWidth = 170;
constexpr int kStackedIconSpace = 6;

// The name of the transaction of every change ("Undo Properties").
const char* const kTransactionName = "Properties";

// docs/ICONE.md: the icons of the properties (0.6-P19).
const char* const kColorIcon = "property-color";
const char* const kLinetypeIcon = "property-linetype";
const char* const kLineweightIcon = "property-lineweight";
// The funnel of QSELECT: the filter by type has no icon of its own yet.
const char* const kFilterIcon = "quick-select";

using TypeNames = std::array<std::string_view, std::variant_size_v<core::Entity>>;

template <std::size_t... Index>
TypeNames typeNamesOf(std::index_sequence<Index...> /*indices*/) {
    return {core::entityTypeName(core::Entity{std::in_place_index<Index>})...};
}

std::string_view dxfType(const core::Entity& entity) {
    return core::entityTypeName(entity);
}

/// `text` between double quotes, as core reads the tag and the value of an
/// attribute (core::withOwnProperty, PT24; docs/verifica/rapporto.md): '"'
/// and '\' after a '\', a control character as \u{XX}.
std::string reportQuoted(std::string_view text) {
    constexpr std::string_view kHexDigits = "0123456789ABCDEF";
    constexpr unsigned char kFirstPrintable = 0x20;
    constexpr unsigned char kDelete = 0x7F;
    constexpr unsigned kNibble = 4U;
    constexpr unsigned kLowNibble = 0x0FU;
    std::string out = "\"";
    for (const char c : text) {
        const auto byte = static_cast<unsigned char>(c);
        if (c == '"' || c == '\\') {
            out += '\\';
            out += c;
        } else if (byte < kFirstPrintable || byte == kDelete) {
            out += "\\u{";
            out += kHexDigits[byte >> kNibble];
            out += kHexDigits[byte & kLowNibble];
            out += '}';
        } else {
            out += c;
        }
    }
    out += '"';
    return out;
}

/// The number of `text`, independent of the locale, or nullopt.
std::optional<double> numberOf(const QString& text) {
    QString trimmed = text.trimmed();
    // An angle may keep the degree sign it is shown without.
    if (trimmed.endsWith(QChar(u'\u00B0'))) {
        trimmed.chop(1);
    }
    bool ok = false;
    const double value = trimmed.toDouble(&ok);
    if (!ok) {
        return std::nullopt;
    }
    return value;
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
        label->setPixmap(themedIcon(name, themeOf(palette)).pixmap(kIconSide, kIconSide));
    }
}

/// A button of the head of the panel: an icon, no frame at rest.
QToolButton* headButton(const char* name, QWidget* parent) {
    auto* button = new QToolButton(parent);
    button->setObjectName(QString::fromLatin1(name));
    button->setAutoRaise(true);
    button->setIconSize(QSize(kIconSide, kIconSide));
    return button;
}

/// Drags the line between the names and the values, which the grid shows
/// without a header (docs/adr/0039, area 8), so that a longer name or value
/// can be given more room.
class ColumnDivider final : public QObject {
public:
    ColumnDivider(QTreeView* grid, std::function<void(int)> resized)
        : QObject(grid), m_grid(grid), m_viewport(grid->viewport()), m_resized(std::move(resized)) {
        m_viewport->setMouseTracking(true);
        m_viewport->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        // The viewport kept from the start: the events of the viewport that
        // the grid sends while it is destroyed must not call the grid.
        QWidget* viewport = m_viewport;
        if (watched != viewport) {
            return false;
        }
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                const auto* mouse = dynamic_cast<const QMouseEvent*>(event);
                if (mouse != nullptr && mouse->button() == Qt::LeftButton &&
                    near(mouse->position().x())) {
                    m_dragging = true;
                    return true;
                }
                return false;
            }
            case QEvent::MouseMove: {
                const auto* mouse = dynamic_cast<const QMouseEvent*>(event);
                if (mouse == nullptr) {
                    return false;
                }
                const int x = qRound(mouse->position().x());
                if (m_dragging) {
                    const int width =
                        std::clamp(x, kMinColumnWidth,
                                   std::max(kMinColumnWidth, viewport->width() - kMinColumnWidth));
                    m_grid->setColumnWidth(PropertyGridModel::kNameColumn, width);
                    m_resized(width);
                    return true;
                }
                if (near(x)) {
                    viewport->setCursor(Qt::SplitHCursor);
                } else {
                    viewport->unsetCursor();
                }
                return false;
            }
            case QEvent::MouseButtonRelease:
                if (m_dragging) {
                    m_dragging = false;
                    return true;
                }
                return false;
            case QEvent::Leave:
                if (!m_dragging) {
                    viewport->unsetCursor();
                }
                return false;
            default:
                return false;
        }
    }

private:
    [[nodiscard]] bool near(double x) const {
        const int line = m_grid->columnWidth(PropertyGridModel::kNameColumn);
        return std::abs(qRound(x) - line) <= kDividerGrip;
    }

    QTreeView* m_grid;
    QWidget* m_viewport;
    std::function<void(int)> m_resized;
    bool m_dragging{false};
};

}  // namespace

const TypeNames& entityTypeNames() {
    static const TypeNames kNames =
        typeNamesOf(std::make_index_sequence<std::tuple_size_v<TypeNames>>{});
    return kNames;
}

QString entityTypeLabel(std::string_view type) {
    struct Label {
        std::string_view type;
        const char* label;
    };

    // As AutoCAD names them in its panel and in QSELECT.
    static constexpr std::array kLabels{
        Label{"LWPOLYLINE", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Polyline")},
        Label{"LINE", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Line")},
        Label{"CIRCLE", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Circle")},
        Label{"ARC", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Arc")},
        Label{"TEXT", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Text")},
        Label{"SOLID", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Solid")},
        Label{"ELLIPSE", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Ellipse")},
        Label{"MTEXT", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "MText")},
        Label{"ATTDEF", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Attribute Definition")},
        Label{"INSERT", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Block Reference")},
        Label{"DIMENSION", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Dimension")},
        Label{"HATCH", QT_TRANSLATE_NOOP("tg::app::PropertiesPanel", "Hatch")},
    };
    for (const Label& label : kLabels) {
        if (label.type == type) {
            return PropertiesPanel::tr(label.label);
        }
    }
    return {};
}

QStringList propertiesPanelIconNames() {
    return {QString::fromLatin1(kColorIcon), QString::fromLatin1(kLinetypeIcon),
            QString::fromLatin1(kLineweightIcon)};
}

PropertiesPanel::PropertiesPanel(QWidget* parent)
    : QDockWidget(tr("Properties"), parent),
      m_types(new QComboBox(this)),
      m_filter(headButton("propertiesFilter", this)),
      m_quickSelect(headButton("propertiesQuickSelect", this)),
      m_model(new PropertyGridModel(m_noDocument, this)),
      m_delegate(new PropertyGridDelegate(this)),
      m_grid(new QTreeView(this)),
      m_editMText(new QToolButton(m_grid->viewport())),
      m_message(new QLabel(this)),
      m_nameWidth(kNameColumnWidth) {
    setObjectName(QStringLiteral("propertiesPanel"));

    m_types->setObjectName(QStringLiteral("propertiesType"));
    m_filter->setToolTip(tr("Filter by type"));
    m_filter->setAccessibleName(m_filter->toolTip());
    // Without an action the panel offers no quick selection.
    m_quickSelect->hide();
    m_message->setObjectName(QStringLiteral("propertiesMessage"));
    m_message->setWordWrap(true);

    m_grid->setObjectName(QStringLiteral("propertiesGrid"));
    m_grid->setItemDelegate(m_delegate);
    m_grid->header()->hide();
    m_grid->header()->setStretchLastSection(true);
    // The delegate draws the chevrons of the groups, and a click on a group
    // opens or closes it (toggleGroup): no column of branches on the left.
    m_grid->setRootIsDecorated(false);
    m_grid->setIndentation(0);
    m_grid->setItemsExpandable(false);
    m_grid->setUniformRowHeights(true);
    m_grid->setFrameShape(QFrame::NoFrame);
    m_grid->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
    // A click on a value opens its editor, as in AutoCAD.
    m_grid->setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked |
                            QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_editMText->setObjectName(QStringLiteral("editMTextButton"));
    m_editMText->setText(QStringLiteral("…"));
    m_editMText->setToolTip(tr("Open text editor"));
    m_editMText->hide();
    connect(m_editMText, &QToolButton::clicked, this, [this] {
        const std::vector<core::EntityId> ids = targets();
        if (ids.size() != 1) {
            return;
        }
        if (QWidget* owner = parentWidget()) {
            if (auto* view = owner->findChild<CadView*>()) {
                view->editMText(ids.front());
            }
        }
    });

    auto* head = new QHBoxLayout;
    head->setSpacing(kSpacing);
    head->addWidget(m_types, 1);
    head->addWidget(m_filter);
    head->addWidget(m_quickSelect);

    auto* content = new QWidget(this);
    auto* layout = new QVBoxLayout(content);
    layout->setContentsMargins(kMargin, kMargin, kMargin, kMargin);
    layout->setSpacing(kMargin);
    layout->addLayout(head);
    layout->addWidget(m_grid, 1);
    layout->addWidget(m_message);
    // The panel scrolls rather than impose its size on the window: a window
    // with both panels docked still fits a small screen.
    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("propertiesScroll"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(content);
    setWidget(scroll);

    // Only the user's choices change the drawing: the setters of the
    // controls emit nothing (0.6-P21), and the filter listens to activated.
    connect(m_types, &QComboBox::activated, this, [this](int index) {
        setTypeFilter(m_types->itemData(index).toString().toStdString());
    });
    connect(m_filter, &QToolButton::clicked, m_types, &QComboBox::showPopup);
    connect(m_delegate, &PropertyGridDelegate::colorChosen, this,
            [this](core::Color value) { setColor(value); });
    connect(m_delegate, &PropertyGridDelegate::layerChosen, this,
            [this](core::LayerId value) { setLayer(value); });
    connect(m_delegate, &PropertyGridDelegate::linetypeChosen, this,
            [this](core::LinetypeId value) { setLinetype(value); });
    connect(m_delegate, &PropertyGridDelegate::lineweightChosen, this,
            [this](int value) { setLineweight(value); });
    connect(m_delegate, &PropertyGridDelegate::ownValueChosen, this,
            &PropertiesPanel::enterOwnValue);
    connect(m_grid, &QTreeView::clicked, this, &PropertiesPanel::toggleGroup);
    new ColumnDivider(m_grid, [this](int width) { m_nameWidth = width; });
    connectModel();
    const auto browseVertex = [this](bool next) {
        const bool moved = next ? m_model->nextVertex() : m_model->previousVertex();
        const std::vector<core::EntityId> ids = targets();
        if (!moved || ids.size() != 1) {
            return;
        }
        const int vertex = m_model->currentVertex();
        emit currentVertexChanged(ids.front(), vertex);
        if (QWidget* owner = parentWidget()) {
            if (auto* view = owner->findChild<CadView*>()) {
                view->setCurrentVertex(ids.front(), vertex);
            }
        }
    };
    connect(m_delegate, &PropertyGridDelegate::previousVertexRequested, this,
            [browseVertex] { browseVertex(false); });
    connect(m_delegate, &PropertyGridDelegate::nextVertexRequested, this,
            [browseVertex] { browseVertex(true); });

    updateIcons();
    refresh();
}

void PropertiesPanel::connectModel() {
    m_model->setPalette(palette());
    // A new model resets the widths of the columns and the rows that span
    // them: both are set again for every model.
    m_grid->setModel(m_model);
    m_grid->setColumnWidth(PropertyGridModel::kNameColumn, m_nameWidth);
    for (int group = 0; group < PropertyGridModel::kGroupCount; ++group) {
        m_grid->setFirstColumnSpanned(group, {}, true);
    }
    connect(m_model, &PropertyGridModel::linetypeScaleEntered, this,
            &PropertiesPanel::enterLinetypeScale);
    connect(m_model, &PropertyGridModel::ownValueEntered, this, &PropertiesPanel::enterOwnValue);
}

PropertiesPanel::PropertiesPanel(edit::Editor& editor, cmd::CommandRunner& runner, QWidget* parent)
    : PropertiesPanel(parent) {
    attach(&editor, &runner);
}

void PropertiesPanel::setSession(DocumentSession* session) {
    if (session == m_session) {
        return;
    }
    Q_EMIT sessionAboutToChange();
    m_session = session;
    attach(session != nullptr ? &session->editor() : nullptr,
           session != nullptr ? &session->runner() : nullptr);
}

void PropertiesPanel::attach(edit::Editor* editor, cmd::CommandRunner* runner) {
    m_editor = editor;
    m_runner = runner;
    // The model reads its document while the grid paints and whenever an
    // index asks for its data: one for each document. The old one goes now,
    // not later: the session it shows may be destroyed before the event loop
    // runs, and its persistent indexes must not reach it (docs/tappe/0.7.5/
    // P28, "Durata dei riferimenti"). Setting the new model first closes an
    // open editor without committing it.
    const std::unique_ptr<PropertyGridModel> old(
        std::exchange(m_model, new PropertyGridModel(document(), this)));
    old->disconnect(this);
    m_delegate->disconnect(old.get());
    connectModel();
    m_typeFilter.clear();
    m_idleSelection.clear();
    setMessage({});
    refresh();
}

edit::Editor& PropertiesPanel::editor() const {
    assert(m_editor != nullptr && "the panel changes a document only with an editor");
    return *m_editor;
}

cmd::CommandRunner& PropertiesPanel::runner() const {
    assert(m_runner != nullptr && "the panel changes a selection only with a runner");
    return *m_runner;
}

bool PropertiesPanel::commandRunning() const {
    return m_runner != nullptr && m_runner->isActive();
}

PropertiesPanel::~PropertiesPanel() {
    // A window destroys its drawing and its runner before its children: an
    // editor that reports its own teardown (a line edit losing the focus)
    // must not reach a drawing that is gone.
    m_delegate->disconnect(this);
    m_types->disconnect(this);
}

void PropertiesPanel::setQuickSelectAction(QAction* action) {
    m_quickSelectAction = action;
    m_quickSelect->setDefaultAction(action);
    m_quickSelect->setVisible(action != nullptr);
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
    return m_editingEnabled && m_editor != nullptr && !m_editor->inTransaction() &&
           !commandRunning();
}

void PropertiesPanel::enterLinetypeScale(const QString& text) {
    bool ok = false;
    const double value = text.toDouble(&ok);
    if (!ok) {
        refuse(tr("Requires a positive number."));
        return;
    }
    setLinetypeScale(value);
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
    m_model->setShown(m_shown);
    showEditors();
    showGeometry();
    showOwnEditors();
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
    if (m_runner == nullptr) {
        m_idleSelection.clear();
        return;
    }
    for (const core::EntityId id : m_runner->selection().ids()) {
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
    if (!commandRunning()) {
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
    const TypeNames& types = entityTypeNames();
    std::array<std::size_t, std::tuple_size_v<TypeNames>> counts{};
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
            filterFound = filterFound || types.at(index) == m_typeFilter;
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
        const std::string_view type = types.at(index);
        const QString name = entityTypeLabel(type);
        // Always with the number of the objects, as in the reference
        // (docs/adr/0039, area 8): "Polyline (1)".
        m_types->addItem(label(name, counts.at(index)),
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

void PropertiesPanel::showEditors() {
    // A tree view with a layout pending ignores the changes of the data, so
    // an open editor would keep a value refused or changed elsewhere.
    for (const PropertyField field :
         {PropertyField::Color, PropertyField::Layer, PropertyField::Linetype,
          PropertyField::LinetypeScale, PropertyField::Lineweight}) {
        const QModelIndex index = m_model->valueIndex(field);
        if (QWidget* editor = m_grid->indexWidget(index)) {
            m_delegate->setEditorData(editor, index);
        }
    }
}

void PropertiesPanel::showGeometry() {
    const std::vector<core::EntityId> ids = targets();
    m_model->setEntity(ids.size() == 1 ? document().find(ids.front()) : nullptr);
    for (const int group :
         {PropertyGridModel::kGeometryGroup, PropertyGridModel::kAttributesGroup}) {
        const bool hidden = m_model->rows(group).empty();
        // Hiding a row lays the grid out again: only when it changes.
        if (m_grid->isRowHidden(group, {}) != hidden) {
            m_grid->setRowHidden(group, {}, hidden);
        }
    }
    showGroups();
    m_editMText->hide();
    if (ids.size() == 1) {
        const core::EntityRecord* record = document().find(ids.front());
        if (record != nullptr && std::holds_alternative<core::MText>(record->entity)) {
            const int row = m_model->rowNamed(PropertyGridModel::kGeometryGroup, tr("Contents"));
            if (row >= 0) {
                const QRect cell = m_grid->visualRect(
                    m_model->rowValueIndex(PropertyGridModel::kGeometryGroup, row));
                const int side = std::max(18, cell.height());
                m_editMText->setGeometry(cell.right() - side + 1, cell.top(), side, side);
                m_editMText->show();
                m_editMText->raise();
            }
        }
    }
}

void PropertiesPanel::showOwnEditors() {
    // As showEditors(): an open editor shows a value refused or changed
    // elsewhere again.
    for (const int group :
         {PropertyGridModel::kGeometryGroup, PropertyGridModel::kAttributesGroup}) {
        const int count = static_cast<int>(m_model->rows(group).size());
        for (int row = 0; row < count; ++row) {
            const QModelIndex index = m_model->rowValueIndex(group, row);
            if (QWidget* editor = m_grid->indexWidget(index)) {
                m_delegate->setEditorData(editor, index);
            }
        }
    }
}

void PropertiesPanel::showGroups() {
    for (int group = 0; group < PropertyGridModel::kGroupCount; ++group) {
        m_grid->setExpanded(m_model->groupIndex(group),
                            !m_closed.at(static_cast<std::size_t>(group)));
    }
}

void PropertiesPanel::toggleGroup(const QModelIndex& index) {
    if (!index.data(PropertyGridModel::kGroupRole).toBool()) {
        return;
    }
    bool& closed = m_closed.at(static_cast<std::size_t>(index.row()));
    closed = !closed;
    showGroups();
}

void PropertiesPanel::updateEnabled() {
    const bool editable = m_editingEnabled && m_editor != nullptr && !commandRunning();
    m_types->setEnabled(m_types->count() > 1);
    m_filter->setEnabled(m_types->isEnabled());
    m_model->setEditable(editable);
    // An open editor closes when the current cell moves. What was typed in
    // it is dropped, as with Esc: moving commits it, and nothing may change
    // now.
    const QModelIndex current = m_grid->currentIndex();
    QWidget* open = current.isValid() ? m_grid->indexWidget(current) : nullptr;
    if (!editable && open != nullptr) {
        if (auto* typed = qobject_cast<QLineEdit*>(open)) {
            typed->setModified(false);
        }
        m_grid->setCurrentIndex(m_model->groupIndex(PropertyGridModel::kGeneralGroup));
    }
    if (m_quickSelectAction != nullptr) {
        m_quickSelectAction->setEnabled(editable);
    }
}

void PropertiesPanel::updateIcons() {
    m_filter->setIcon(themedIcon(QString::fromLatin1(kFilterIcon), themeOf(palette())));
    m_model->setPalette(palette());
}

bool PropertiesPanel::change(const std::function<void(core::EntityProperties&)>& edit) {
    if (!canEdit()) {
        return false;
    }
    const bool hasSelection = !m_selection.empty();
    const std::vector<core::EntityId> ids = targets();
    editor().beginTransaction(kTransactionName);
    bool done = false;
    if (!hasSelection) {
        core::CurrentProperties current = document().current();
        edit(current);
        done = editor().tables().setCurrent(current);
    } else {
        for (const core::EntityId id : ids) {
            const core::EntityRecord* record = document().find(id);
            if (record == nullptr) {
                continue;
            }
            core::EntityProperties properties = record->properties;  // a copy: the record changes
            edit(properties);
            done = editor().setProperties(id, properties) || done;
        }
    }
    if (done) {
        editor().commit();
        setMessage({});
    } else {
        editor().rollback();
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

bool PropertiesPanel::setOwnProperty(std::string_view name, const core::PropertyValue& value) {
    if (!canEdit()) {
        return false;
    }
    const std::vector<core::EntityId> ids = targets();
    if (ids.size() != 1) {
        return false;
    }
    const core::EntityId id = ids.front();
    const core::EntityRecord* record = document().find(id);
    if (record == nullptr) {
        return false;
    }
    core::PropertyChange change = core::withOwnProperty(document(), *record, name, value);
    if (!change.entity) {
        return refuse(QString::fromStdString(change.refusal));
    }
    editor().beginTransaction(kTransactionName);
    // With the extras, entity and extras are one change of the transaction
    // (#927, [bloccante] 6).
    const bool done =
        change.extras ? editor().replace(id, std::move(*change.entity), std::move(*change.extras))
                      : editor().replace(id, std::move(*change.entity));
    if (done) {
        editor().commit();
        setMessage({});
    } else {
        editor().rollback();
    }
    refresh();
    if (done) {
        Q_EMIT changed();
    }
    return done;
}

bool PropertiesPanel::setAttributeValue(const std::string& tag, const std::string& value) {
    return setOwnProperty("attribute", reportQuoted(tag) + ":" + reportQuoted(value));
}

bool PropertiesPanel::enterOwnValue(int group, int row, const QString& text) {
    const OwnPropertyRow* shown = m_model->row(group, row);
    if (shown == nullptr || !shown->editable()) {
        return false;
    }
    // A copy: the change shows the rows again.
    const OwnPropertyRow entered = *shown;
    if (!entered.tag.empty()) {
        return setAttributeValue(entered.tag, text.toStdString());
    }
    const auto invalid = [this] {
        return refuse(
            QString::fromUtf8(core::kInvalidPropertyValue.data(),
                              static_cast<qsizetype>(core::kInvalidPropertyValue.size())));
    };
    switch (entered.kind) {
        case core::PropertyKind::Number:
        case core::PropertyKind::Distance: {
            const std::optional<double> number = numberOf(text);
            return number ? setOwnProperty(entered.property, *number) : invalid();
        }
        case core::PropertyKind::Angle: {
            const std::optional<double> degrees = numberOf(text);
            return degrees
                       ? setOwnProperty(entered.property, core::reducedDegreesToRadians(*degrees))
                       : invalid();
        }
        case core::PropertyKind::Point: {
            const std::optional<double> number = numberOf(text);
            const std::vector<core::EntityId> ids = targets();
            const core::EntityRecord* record =
                ids.size() == 1 ? document().find(ids.front()) : nullptr;
            if (!number || record == nullptr) {
                return invalid();
            }
            const std::vector<core::OwnProperty> properties =
                core::ownProperties(document(), *record);
            const auto found =
                std::ranges::find(properties, entered.property, &core::OwnProperty::name);
            const auto* point =
                found != properties.end() ? std::get_if<core::Vec2>(&found->value) : nullptr;
            if (point == nullptr) {
                return invalid();
            }
            core::Vec2 changedPoint = *point;
            if (entered.component == 0) {
                changedPoint.x = *number;
            } else {
                changedPoint.y = *number;
            }
            return setOwnProperty(entered.property, changedPoint);
        }
        case core::PropertyKind::Text:
        case core::PropertyKind::Choice:
            return setOwnProperty(entered.property, text.toStdString());
        case core::PropertyKind::Flag:
            if (!entered.choices.contains(text)) {
                return invalid();
            }
            return setOwnProperty(entered.property, text == entered.choices.front());
    }
    return false;
}

void PropertiesPanel::showQuickSelect(std::vector<core::EntityId> current) {
    if (m_editor == nullptr || m_editor->inTransaction()) {
        return;
    }
    auto* dialog = new QuickSelectDialog(document(), current, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    // The window reads the document of the session it was opened for: it
    // closes before the panel shows another session, which may then go. As
    // Cancel does, it gives that session its selection back, quietly: the
    // window is changing session.
    connect(this, &PropertiesPanel::sessionAboutToChange, dialog, [this, dialog, current] {
        const QSignalBlocker blocker(dialog);
        dialog->close();
        if (!current.empty() && m_runner != nullptr && runner().selection().isEmpty()) {
            static_cast<void>(runner().replaceSelection(current));
        }
    });
    connect(dialog, &QDialog::accepted, this,
            [this, dialog] { applyQuickSelect(dialog->criteria(), dialog->within()); });
    // A typed QSELECT starts a command, which clears the selection (ADR
    // 0008): Cancel gives it back, as the window found it.
    connect(dialog, &QDialog::rejected, this, [this, previous = std::move(current)] {
        if (!previous.empty() && m_runner != nullptr && runner().selection().isEmpty() &&
            runner().replaceSelection(previous)) {
            refresh();
            Q_EMIT changed();
        }
    });
    dialog->open();
}

bool PropertiesPanel::applyQuickSelect(const cmd::QuickSelectCriteria& criteria,
                                       const std::vector<core::EntityId>& within) {
    if (m_runner == nullptr || commandRunning()) {
        return false;
    }
    std::vector<core::EntityId> found = cmd::quickSelect(document(), within, criteria);
    const auto count = static_cast<qulonglong>(found.size());
    if (!runner().replaceSelection(std::move(found))) {
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

PropertyControls::PropertyControls(PropertiesPanel& panel, Qt::Orientation orientation,
                                   QWidget* parent)
    : QWidget(parent),
      m_panel(&panel),
      m_color(new ColorCombo(this)),
      m_linetype(new LinetypeCombo(this)),
      m_lineweight(new LineweightCombo(this)) {
    setObjectName(QStringLiteral("propertyControls"));
    auto* layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    m_color->setObjectName(QStringLiteral("propertyColorCombo"));
    m_linetype->setObjectName(QStringLiteral("propertyLinetypeCombo"));
    m_lineweight->setObjectName(QStringLiteral("propertyLineweightCombo"));
    const std::array<std::pair<QComboBox*, const char*>, 3> controls{
        {{m_color, kColorIcon}, {m_linetype, kLinetypeIcon}, {m_lineweight, kLineweightIcon}}};
    int place = 0;
    for (const auto& [combo, icon] : controls) {
        // A click must not take the keyboard away from the drawing and the
        // command line.
        combo->setFocusPolicy(Qt::NoFocus);
        QLabel* label = iconLabel(QString::fromLatin1(icon), this, m_icons);
        if (orientation == Qt::Vertical) {
            combo->setMinimumWidth(kStackedComboWidth);
            layout->addWidget(label, place, 0);
            layout->addWidget(combo, place, 1);
        } else {
            layout->addWidget(label, 0, 2 * place);
            layout->addWidget(combo, 0, (2 * place) + 1);
        }
        ++place;
    }
    if (orientation == Qt::Vertical) {
        layout->setHorizontalSpacing(kStackedIconSpace);
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
