// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "layer_panel.hpp"

#include "property_widgets.hpp"
#include "tg/edit/layer_rules.hpp"
#include "theme.hpp"

#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSize>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

#include <optional>
#include <string>
#include <utility>

namespace tg::app {
namespace {

// The side of the icons of the table, in pixels.
constexpr int kIconSide = 18;
// Rows are tall enough for the drop-down lists of the cells.
constexpr int kRowHeight = 28;
constexpr int kStatusWidth = 56;
constexpr int kNameWidth = 130;
constexpr int kStateWidth = 52;
constexpr int kColorWidth = 130;
constexpr int kLinetypeWidth = 170;
constexpr int kLineweightWidth = 110;
// The width that shows a layer name and its states in the drop-down list.
constexpr int kLayerComboWidth = 240;

// The name of the transaction of every change ("Undo Layer").
const char* const kTransactionName = "Layer";

constexpr QAbstractItemView::EditTriggers kNameEditTriggers =
    QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed;

int columnOf(LayerColumn column) {
    return static_cast<int>(column);
}

/// The first of Layer1, Layer2... that no layer has.
std::string freeName(const core::Document& document) {
    int number = 1;
    std::string name = "Layer1";
    while (document.findLayer(name) != nullptr) {
        ++number;
        name = "Layer" + std::to_string(number);
    }
    return name;
}

QString layerName(const core::Layer* layer) {
    return layer != nullptr ? QString::fromStdString(layer->name) : QString();
}

}  // namespace

LayerPanel::LayerPanel(edit::Editor& editor, QWidget* parent)
    : QDockWidget(tr("Layers"), parent),
      m_editor(editor),
      m_model(new LayerTableModel(editor.document(), this)),
      m_table(new QTableView(this)),
      m_message(new QLabel(this)),
      m_newButton(new QPushButton(tr("New"), this)),
      m_deleteButton(new QPushButton(tr("Delete"), this)),
      m_currentButton(new QPushButton(tr("Make Current"), this)) {
    setObjectName(QStringLiteral("layerPanel"));
    m_model->setPalette(palette());

    m_table->setObjectName(QStringLiteral("layerTable"));
    m_table->setModel(m_model);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(kNameEditTriggers);
    m_table->setIconSize(QSize(kIconSide, kIconSide));
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(kRowHeight);
    m_table->horizontalHeader()->setStretchLastSection(false);

    m_message->setObjectName(QStringLiteral("layerMessage"));
    m_message->setWordWrap(true);
    m_newButton->setObjectName(QStringLiteral("layerNew"));
    m_deleteButton->setObjectName(QStringLiteral("layerDelete"));
    m_currentButton->setObjectName(QStringLiteral("layerMakeCurrent"));

    auto* content = new QWidget(this);
    auto* layout = new QVBoxLayout(content);
    auto* buttons = new QHBoxLayout;
    buttons->addWidget(m_newButton);
    buttons->addWidget(m_deleteButton);
    buttons->addWidget(m_currentButton);
    buttons->addStretch(1);
    layout->addLayout(buttons);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_message);
    setWidget(content);

    connect(m_newButton, &QPushButton::clicked, this, [this] { newLayer(); });
    connect(m_deleteButton, &QPushButton::clicked, this, [this] { deleteSelected(); });
    connect(m_currentButton, &QPushButton::clicked, this, [this] { makeSelectedCurrent(); });
    connect(m_table, &QTableView::clicked, this,
            [this](const QModelIndex& index) { cellClicked(index); });
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this] { updateActions(); });
    connect(m_model, &QAbstractItemModel::modelReset, this, [this] { rebuildCellWidgets(); });
    // The delegate is still closing its editor when the name is committed:
    // the change waits for the event loop.
    connect(
        m_model, &LayerTableModel::renameRequested, this,
        [this](core::LayerId id, const QString& name) { renameLayer(id, name); },
        Qt::QueuedConnection);

    rebuildCellWidgets();
    updateActions();
}

LayerPanel::~LayerPanel() {
    // A window destroys its drawing before its children. The table and the
    // model are deleted after this destructor, and they report their own
    // teardown (a row that stops being current, a model that resets): the
    // panel must not answer with a look at the drawing that is gone.
    m_table->selectionModel()->disconnect(this);
    m_model->disconnect(this);
    m_table->disconnect(this);
}

void LayerPanel::setActions(QAction* newAction, QAction* deleteAction, QAction* currentAction) {
    m_newAction = newAction;
    m_deleteAction = deleteAction;
    m_currentAction = currentAction;
    if (newAction != nullptr) {
        connect(newAction, &QAction::triggered, this, [this] { newLayer(); });
    }
    if (deleteAction != nullptr) {
        connect(deleteAction, &QAction::triggered, this, [this] { deleteSelected(); });
    }
    if (currentAction != nullptr) {
        connect(currentAction, &QAction::triggered, this, [this] { makeSelectedCurrent(); });
    }
    updateActions();
}

void LayerPanel::setEditingEnabled(bool enabled) {
    // The window calls this after every input: do nothing if nothing changed.
    if (enabled == m_editingEnabled) {
        return;
    }
    m_editingEnabled = enabled;
    m_table->setEditTriggers(
        enabled ? kNameEditTriggers
                : QAbstractItemView::EditTriggers(QAbstractItemView::NoEditTriggers));
    syncCellWidgets();
    updateActions();
    Q_EMIT refreshed();
}

void LayerPanel::setConfirm(Confirm answer) {
    m_confirm = std::move(answer);
}

bool LayerPanel::canEdit() const {
    return m_editingEnabled && !m_editor.inTransaction();
}

bool LayerPanel::confirm(const QString& text) {
    if (m_confirm) {
        return m_confirm(text);
    }
    return QMessageBox::question(this, tr("Layers"), text, QMessageBox::Yes | QMessageBox::No,
                                 QMessageBox::No) == QMessageBox::Yes;
}

void LayerPanel::setMessage(const QString& text) {
    m_lastMessage = text;
    m_message->setText(text);
}

bool LayerPanel::refuse(const QString& text) {
    setMessage(text);
    Q_EMIT messageShown(text);
    // The cells that offered the refused value show the real one again.
    refresh();
    return false;
}

bool LayerPanel::run(const std::function<bool()>& change) {
    if (!canEdit()) {
        return false;
    }
    m_editor.beginTransaction(kTransactionName);
    const bool done = change();
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

void LayerPanel::refresh() {
    const core::LayerId selected = selectedLayer();
    m_model->refresh();
    syncCellWidgets();
    if (selected.value != 0 && m_model->rowOf(selected) >= 0 && selectedLayer() != selected) {
        selectLayer(selected);
    }
    updateActions();
    Q_EMIT refreshed();
}

void LayerPanel::showAndFocus() {
    show();
    raise();
    if (isFloating()) {
        activateWindow();
    }
    m_table->setFocus(Qt::OtherFocusReason);
}

core::LayerId LayerPanel::selectedLayer() const {
    const QModelIndex index = m_table->currentIndex();
    return index.isValid() ? m_model->layerAt(index.row()) : core::LayerId{};
}

void LayerPanel::selectLayer(core::LayerId id) {
    const int row = m_model->rowOf(id);
    if (row >= 0) {
        m_table->setCurrentIndex(m_model->index(row, columnOf(LayerColumn::Name)));
        m_table->selectRow(row);
    }
}

void LayerPanel::rebuildCellWidgets() {
    // The cells of the three properties hold the controls of the properties
    // panel (0.6-P21), each one for the layer of its row.
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const core::LayerId id = m_model->layerAt(row);
        auto* color = new ColorCombo(m_table->viewport());
        connect(color, &ColorCombo::valueChosen, this,
                [this, id](core::Color value) { setColor(id, value); });
        m_table->setIndexWidget(m_model->index(row, columnOf(LayerColumn::Color)), color);
        auto* linetype = new LinetypeCombo(m_table->viewport());
        connect(linetype, &LinetypeCombo::valueChosen, this,
                [this, id](core::LinetypeId value) { setLinetype(id, value); });
        m_table->setIndexWidget(m_model->index(row, columnOf(LayerColumn::Linetype)), linetype);
        auto* lineweight = new LineweightCombo(m_table->viewport());
        connect(lineweight, &LineweightCombo::valueChosen, this,
                [this, id](int value) { setLineweight(id, value); });
        m_table->setIndexWidget(m_model->index(row, columnOf(LayerColumn::Lineweight)), lineweight);
    }
    m_table->setColumnWidth(columnOf(LayerColumn::Status), kStatusWidth);
    m_table->setColumnWidth(columnOf(LayerColumn::Name), kNameWidth);
    m_table->setColumnWidth(columnOf(LayerColumn::On), kStateWidth);
    m_table->setColumnWidth(columnOf(LayerColumn::Frozen), kStateWidth);
    m_table->setColumnWidth(columnOf(LayerColumn::Locked), kStateWidth);
    m_table->setColumnWidth(columnOf(LayerColumn::Color), kColorWidth);
    m_table->setColumnWidth(columnOf(LayerColumn::Linetype), kLinetypeWidth);
    m_table->setColumnWidth(columnOf(LayerColumn::Lineweight), kLineweightWidth);
    m_table->setColumnWidth(columnOf(LayerColumn::Plot), kStateWidth);
    syncCellWidgets();
}

void LayerPanel::syncCellWidgets() {
    const core::Document& doc = document();
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const core::Layer* layer = doc.layer(m_model->layerAt(row));
        if (layer == nullptr) {
            continue;
        }
        // The setters of the controls never emit valueChosen.
        auto* color = qobject_cast<ColorCombo*>(
            m_table->indexWidget(m_model->index(row, columnOf(LayerColumn::Color))));
        if (color != nullptr) {
            color->setValue(layer->color);
            color->setEnabled(m_editingEnabled);
        }
        auto* linetype = qobject_cast<LinetypeCombo*>(
            m_table->indexWidget(m_model->index(row, columnOf(LayerColumn::Linetype))));
        if (linetype != nullptr) {
            linetype->setDocument(&doc);
            linetype->setValue(layer->linetype);
            linetype->setEnabled(m_editingEnabled);
        }
        auto* lineweight = qobject_cast<LineweightCombo*>(
            m_table->indexWidget(m_model->index(row, columnOf(LayerColumn::Lineweight))));
        if (lineweight != nullptr) {
            lineweight->setDocument(&doc);
            lineweight->setValue(layer->lineweight);
            lineweight->setEnabled(m_editingEnabled);
        }
    }
}

void LayerPanel::updateActions() {
    // No row is selected while the window closes: the document may be gone.
    const core::LayerId id = selectedLayer();
    const bool selected = id.value != 0 && document().layer(id) != nullptr;
    const bool canNew = m_editingEnabled;
    const bool canAct = m_editingEnabled && selected;
    m_newButton->setEnabled(canNew);
    m_deleteButton->setEnabled(canAct);
    m_currentButton->setEnabled(canAct);
    if (!m_newAction.isNull()) {
        m_newAction->setEnabled(canNew);
    }
    if (!m_deleteAction.isNull()) {
        m_deleteAction->setEnabled(canAct);
    }
    if (!m_currentAction.isNull()) {
        m_currentAction->setEnabled(canAct);
    }
}

void LayerPanel::cellClicked(const QModelIndex& index) {
    if (!index.isValid() || !m_editingEnabled) {
        return;
    }
    const core::LayerId id = m_model->layerAt(index.row());
    const core::Layer* layer = document().layer(id);
    if (layer == nullptr) {
        return;
    }
    // The values are read before the call: a change invalidates the layer.
    switch (static_cast<LayerColumn>(index.column())) {
        case LayerColumn::Status:
            makeCurrent(id);
            break;
        case LayerColumn::On:
            setState(id, LayerState::On, !layer->on);
            break;
        case LayerColumn::Frozen:
            setState(id, LayerState::Frozen, !layer->frozen);
            break;
        case LayerColumn::Locked:
            setState(id, LayerState::Locked, !layer->locked);
            break;
        case LayerColumn::Plot:
            setState(id, LayerState::Plot, !layer->plot);
            break;
        case LayerColumn::Name:
        case LayerColumn::Color:
        case LayerColumn::Linetype:
        case LayerColumn::Lineweight:
        case LayerColumn::Count:
            break;
    }
}

bool LayerPanel::newLayer() {
    if (!canEdit()) {
        return false;
    }
    core::Layer layer;
    layer.name = freeName(document());
    layer.linetype = document().continuousLinetype();
    std::optional<core::LayerId> created;
    const bool done = run([&] {
        created = m_editor.addLayer(layer);
        return created.has_value();
    });
    if (done && created.has_value()) {
        selectLayer(*created);
    }
    return done;
}

bool LayerPanel::deleteLayer(core::LayerId id) {
    if (!canEdit()) {
        return false;
    }
    const QString name = layerName(document().layer(id));
    switch (edit::checkRemove(document(), id)) {
        case edit::LayerProblem::NotFound:
            return refuse(tr("Layer not found."));
        case edit::LayerProblem::LayerZero:
            return refuse(tr("Cannot delete layer \"0\"."));
        case edit::LayerProblem::Current:
            return refuse(tr("Cannot delete layer \"%1\".  It is the CURRENT layer.").arg(name));
        case edit::LayerProblem::InUse:
            return refuse(tr("Cannot delete layer \"%1\".  It is in use.").arg(name));
        case edit::LayerProblem::None:
        case edit::LayerProblem::InvalidName:
        case edit::LayerProblem::NameInUse:
            break;
    }
    return run([&] { return m_editor.removeLayer(id); });
}

bool LayerPanel::deleteSelected() {
    return deleteLayer(selectedLayer());
}

bool LayerPanel::makeCurrent(core::LayerId id) {
    if (!canEdit()) {
        return false;
    }
    const core::Layer* layer = document().layer(id);
    if (layer == nullptr) {
        return refuse(tr("Layer not found."));
    }
    if (layer->frozen) {
        return refuse(tr("Cannot set current to a frozen layer."));
    }
    core::CurrentProperties current = document().current();
    current.layer = id;
    return run([&] { return m_editor.setCurrent(current); });
}

bool LayerPanel::makeSelectedCurrent() {
    return makeCurrent(selectedLayer());
}

bool LayerPanel::renameLayer(core::LayerId id, const QString& name) {
    if (!canEdit()) {
        return false;
    }
    const core::Layer* layer = document().layer(id);
    if (layer == nullptr) {
        return refuse(tr("Layer not found."));
    }
    const QString oldName = layerName(layer);
    const std::string text = name.toStdString();
    switch (edit::checkRename(document(), id, text)) {
        case edit::LayerProblem::NotFound:
            return refuse(tr("Layer not found."));
        case edit::LayerProblem::LayerZero:
            return refuse(tr("Cannot rename layer \"0\"."));
        case edit::LayerProblem::InvalidName:
            return refuse(tr("Invalid layer name."));
        case edit::LayerProblem::NameInUse:
            return refuse(tr("Layer \"%1\" already exists.").arg(name));
        case edit::LayerProblem::None:
        case edit::LayerProblem::Current:
        case edit::LayerProblem::InUse:
            break;
    }
    if (text == layer->name) {
        // The edit left the name as it was: show it again.
        refresh();
        return false;
    }
    // ADR 0023, point 5: said before, then done; the other data keeps the old name.
    if (edit::namedInOtherUnhandledData(document(), id) &&
        !confirm(tr("Layer \"%1\" is also named in data Tecnigrafo does not handle; that data "
                    "keeps the old name.\n\nRename the layer anyway?")
                     .arg(oldName))) {
        refresh();
        return false;
    }
    return run([&] { return m_editor.renameLayer(id, text); });
}

bool LayerPanel::setState(core::LayerId id, LayerState state, bool value) {
    if (!canEdit()) {
        return false;
    }
    const core::Layer* layer = document().layer(id);
    if (layer == nullptr) {
        return refuse(tr("Layer not found."));
    }
    core::Layer next = *layer;
    const QString name = layerName(layer);
    switch (state) {
        case LayerState::On:
            if (!value && layer->on && id == document().current().layer &&
                !confirm(tr("Really want layer %1 (the CURRENT layer) off?").arg(name))) {
                return false;
            }
            next.on = value;
            break;
        case LayerState::Frozen:
            if (value && edit::checkFreeze(document(), id) == edit::LayerProblem::Current) {
                return refuse(
                    tr("Cannot freeze layer \"%1\".  It is the CURRENT layer.").arg(name));
            }
            next.frozen = value;
            break;
        case LayerState::Locked:
            next.locked = value;
            break;
        case LayerState::Plot:
            next.plot = value;
            break;
    }
    return run([&] { return m_editor.changeLayer(next); });
}

bool LayerPanel::setColor(core::LayerId id, core::Color color) {
    if (!canEdit()) {
        return false;
    }
    const core::Layer* layer = document().layer(id);
    if (layer == nullptr) {
        return refuse(tr("Layer not found."));
    }
    // A layer has a colour of its own: never ByLayer nor ByBlock (ADR 0023).
    if (color.kind() == core::Color::Kind::ByLayer || color.kind() == core::Color::Kind::ByBlock) {
        return refuse(tr("Requires a color index from 1 to 255."));
    }
    core::Layer next = *layer;
    next.color = color;
    return run([&] { return m_editor.changeLayer(next); });
}

bool LayerPanel::setLinetype(core::LayerId id, core::LinetypeId linetype) {
    if (!canEdit()) {
        return false;
    }
    const core::Layer* layer = document().layer(id);
    if (layer == nullptr) {
        return refuse(tr("Layer not found."));
    }
    const core::Linetype* record = document().linetype(linetype);
    if (record == nullptr || linetype == document().byLayerLinetype() ||
        linetype == document().byBlockLinetype()) {
        const QString name = record != nullptr ? QString::fromStdString(record->name) : QString();
        return refuse(tr("Linetype \"%1\" is not loaded.").arg(name));
    }
    core::Layer next = *layer;
    next.linetype = linetype;
    return run([&] { return m_editor.changeLayer(next); });
}

bool LayerPanel::setLineweight(core::LayerId id, int lineweight) {
    if (!canEdit()) {
        return false;
    }
    const core::Layer* layer = document().layer(id);
    if (layer == nullptr) {
        return refuse(tr("Layer not found."));
    }
    // A layer has a lineweight of its own: never ByLayer nor ByBlock.
    if (lineweight == core::lineweight::kByLayer || lineweight == core::lineweight::kByBlock ||
        !core::isAllowedLineweight(lineweight)) {
        return refuse(tr("Requires a valid lineweight."));
    }
    core::Layer next = *layer;
    next.lineweight = lineweight;
    return run([&] { return m_editor.changeLayer(next); });
}

void LayerPanel::changeEvent(QEvent* event) {
    QDockWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange && m_model != nullptr) {
        m_model->setPalette(palette());
        syncCellWidgets();
    }
}

LayerControls::LayerControls(LayerPanel& panel, QWidget* parent)
    : QWidget(parent),
      m_panel(panel),
      m_combo(new LayerCombo(this)),
      m_on(new QToolButton(this)),
      m_frozen(new QToolButton(this)),
      m_locked(new QToolButton(this)) {
    setObjectName(QStringLiteral("layerControls"));
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    // A click must not take the keyboard away from the drawing and the
    // command line.
    m_combo->setObjectName(QStringLiteral("layerCombo"));
    m_combo->setFocusPolicy(Qt::NoFocus);
    m_combo->setMinimumWidth(kLayerComboWidth);
    layout->addWidget(m_combo);
    m_on->setObjectName(QStringLiteral("layerOnButton"));
    m_frozen->setObjectName(QStringLiteral("layerFrozenButton"));
    m_locked->setObjectName(QStringLiteral("layerLockedButton"));
    for (QToolButton* button : {m_on, m_frozen, m_locked}) {
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::NoFocus);
        layout->addWidget(button);
    }
    connect(m_combo, &LayerCombo::valueChosen, this,
            [this](core::LayerId id) { m_panel.makeCurrent(id); });
    connect(m_on, &QToolButton::clicked, this, [this] { toggle(LayerState::On); });
    connect(m_frozen, &QToolButton::clicked, this, [this] { toggle(LayerState::Frozen); });
    connect(m_locked, &QToolButton::clicked, this, [this] { toggle(LayerState::Locked); });
    connect(&m_panel, &LayerPanel::refreshed, this, [this] { sync(); });
    sync();
}

void LayerControls::sync() {
    const core::Document& doc = m_panel.document();
    m_combo->setDocument(&doc);
    const core::Layer* layer = doc.layer(doc.current().layer);
    const auto showState = [this, layer](QToolButton* button, LayerState state,
                                         const QString& turnOn, const QString& turnOff) {
        const bool value = layer != nullptr && layerStateValue(*layer, state);
        button->setIcon(themedIcon(layerStateIconName(state, value), palette()));
        button->setToolTip(value ? turnOff : turnOn);
    };
    showState(m_on, LayerState::On, tr("Turn the current layer on"),
              tr("Turn the current layer off"));
    showState(m_frozen, LayerState::Frozen, tr("Freeze the current layer"),
              tr("Thaw the current layer"));
    showState(m_locked, LayerState::Locked, tr("Lock the current layer"),
              tr("Unlock the current layer"));
    setEnabled(m_panel.editingEnabled());
}

void LayerControls::toggle(LayerState state) {
    const core::Document& doc = m_panel.document();
    const core::Layer* layer = doc.layer(doc.current().layer);
    if (layer == nullptr) {
        return;
    }
    // Both are read before the call: a change invalidates the layer.
    const core::LayerId id = layer->id;
    const bool next = !layerStateValue(*layer, state);
    m_panel.setState(id, state, next);
}

void LayerControls::changeEvent(QEvent* event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange) {
        sync();
    }
}

}  // namespace tg::app
