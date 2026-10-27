// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "layer_table_model.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"

#include <QDockWidget>
#include <QModelIndex>
#include <QPointer>
#include <QString>
#include <QWidget>

#include <functional>

class QAction;
class QEvent;
class QLabel;
class QPushButton;
class QTableView;
class QToolButton;

namespace tg::app {

class LayerCombo;

/// The layers of the drawing in a table that can be docked (docs/adr/0009,
/// point 3; docs/tappe/0.6/P22).
///
/// Every change is one transaction "Layer" of the editor, so U and Ctrl+Z
/// undo it. The rules of edit::layer_rules decide; a refused change shows
/// why, with the words of -LAYER, and changes nothing. Changes that come
/// from elsewhere (a command, undo, redo, NEW, OPEN) reach the panel when the
/// owner calls refresh().
///
/// The editor is borrowed and must outlive the panel.
class LayerPanel final : public QDockWidget {
    Q_OBJECT

public:
    /// Answers a question of the panel: true is yes.
    using Confirm = std::function<bool(const QString&)>;

    explicit LayerPanel(edit::Editor& editor, QWidget* parent = nullptr);
    ~LayerPanel() override;

    /// The actions that do what the buttons do, on the layer selected in the
    /// table. The panel enables and disables them. Each may be null.
    void setActions(QAction* newAction, QAction* deleteAction, QAction* currentAction);

    /// While a command runs the panel shows the layers but changes nothing.
    void setEditingEnabled(bool enabled);

    [[nodiscard]] bool editingEnabled() const { return m_editingEnabled; }

    /// Replaces the question boxes, which wait for the user. An empty
    /// function brings back the default.
    void setConfirm(Confirm answer);

    [[nodiscard]] const core::Document& document() const { return m_editor.document(); }

    /// Shows the document again: the rows, the cells and the controls.
    void refresh();

    /// Shows the panel, brings it forward and gives it the keyboard focus.
    void showAndFocus();

    /// A new layer called Layer1, Layer2... (the first free name), selected.
    bool newLayer();
    bool deleteLayer(core::LayerId id);
    bool deleteSelected();
    bool makeCurrent(core::LayerId id);
    bool makeSelectedCurrent();
    bool renameLayer(core::LayerId id, const QString& name);
    bool setState(core::LayerId id, LayerState state, bool value);
    bool setColor(core::LayerId id, core::Color color);
    bool setLinetype(core::LayerId id, core::LinetypeId linetype);
    bool setLineweight(core::LayerId id, int lineweight);

    /// The layer of the current row of the table, or the identifier 0.
    [[nodiscard]] core::LayerId selectedLayer() const;
    void selectLayer(core::LayerId id);

    /// The reason of the last refused change; empty after a change is made.
    [[nodiscard]] const QString& lastMessage() const { return m_lastMessage; }

Q_SIGNALS:
    /// A refused change: the same text goes to the history of the command line.
    void messageShown(const QString& text);

    /// After every refresh: the controls of the ribbon follow it.
    void refreshed();

    /// A change of the panel was committed: the window updates Undo and Redo.
    void changed();

protected:
    void changeEvent(QEvent* event) override;

private:
    /// Runs `change` in a transaction "Layer": committed if it returns true,
    /// rolled back if not. Refreshes the panel.
    [[nodiscard]] bool canEdit() const;
    bool run(const std::function<bool()>& change);
    /// Shows `text` as the reason of a refused change; returns false.
    bool refuse(const QString& text);
    bool confirm(const QString& text);
    void setMessage(const QString& text);
    void rebuildCellWidgets();
    void syncCellWidgets();
    void updateActions();
    void cellClicked(const QModelIndex& index);

    edit::Editor& m_editor;
    LayerTableModel* m_model;
    QTableView* m_table;
    QLabel* m_message;
    QPushButton* m_newButton;
    QPushButton* m_deleteButton;
    QPushButton* m_currentButton;
    QPointer<QAction> m_newAction;
    QPointer<QAction> m_deleteAction;
    QPointer<QAction> m_currentAction;
    Confirm m_confirm;
    QString m_lastMessage;
    bool m_editingEnabled{true};
};

/// The drop-down list of the layers and the buttons of the state of the
/// current layer, for the ribbon and the toolbar `Layers`. It follows the
/// panel and changes the document through it.
class LayerControls final : public QWidget {
    Q_OBJECT

public:
    /// The controls stop working if the panel is destroyed first.
    explicit LayerControls(LayerPanel& panel, QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;

private:
    void sync();
    void toggle(LayerState state);

    QPointer<LayerPanel> m_panel;
    LayerCombo* m_combo;
    QToolButton* m_on;
    QToolButton* m_frozen;
    QToolButton* m_locked;
};

}  // namespace tg::app
