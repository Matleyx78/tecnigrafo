// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "document_session.hpp"
#include "layer_table_model.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"

#include <QDockWidget>
#include <QModelIndex>
#include <QString>
#include <QWidget>

#include <cstddef>
#include <functional>
#include <vector>

class QAction;
class QEvent;
class QLabel;
class QLineEdit;
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
/// The panel shows the session of setSession(), the active one
/// (docs/adr/0041, point 2), or the editor of the constructor that takes
/// one. Without either it shows no layers and changes nothing.
class LayerPanel final : public QDockWidget {
    Q_OBJECT

public:
    /// Answers a question of the panel: true is yes.
    using Confirm = std::function<bool(const QString&)>;

    Q_DISABLE_COPY_MOVE(LayerPanel)

    /// A panel without a session, until setSession().
    explicit LayerPanel(QWidget* parent = nullptr);

    /// A panel working on `editor` alone, without sessions, as the widget
    /// tests use it. The editor must outlive the panel.
    explicit LayerPanel(edit::Editor& editor, QWidget* parent = nullptr);

    ~LayerPanel() override;

    /// Shows and changes `session` from now on; nullptr for none. The
    /// session is borrowed: the owner calls setSession() again, with another
    /// session or nullptr, before destroying it.
    void setSession(DocumentSession* session);

    [[nodiscard]] DocumentSession* session() const { return m_session; }

    /// The actions that do what the buttons do, on the layer selected in the
    /// table. The panel enables and disables them. Each may be null.
    void setActions(QAction* newAction, QAction* deleteAction, QAction* currentAction);

    /// While a command runs the panel shows the layers but changes nothing.
    void setEditingEnabled(bool enabled);

    /// False also without a session.
    [[nodiscard]] bool editingEnabled() const { return m_editingEnabled && m_editor != nullptr; }

    /// Replaces the question boxes, which wait for the user. An empty
    /// function brings back the default.
    void setConfirm(Confirm answer);

    /// The document shown; without a session, an empty document.
    [[nodiscard]] const core::Document& document() const {
        return m_editor != nullptr ? m_editor->document() : m_noDocument;
    }

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
    /// The editor shown. Precondition: there is one.
    [[nodiscard]] edit::Editor& editor() const;
    /// Shows and changes `editor` from now on, nullptr for none: a new model
    /// for its document.
    void attachEditor(edit::Editor* editor);
    /// Gives the table m_model, made for document(), and connects it.
    void connectModel();
    /// Runs `change` in a transaction "Layer": committed if it returns true,
    /// rolled back if not. Refreshes the panel.
    [[nodiscard]] bool canEdit() const;
    bool run(const std::function<bool()>& change);
    /// Shows `text` as the reason of a refused change; returns false.
    bool refuse(const QString& text);
    bool confirm(const QString& text);
    void setMessage(const QString& text);
    void rebuildCellWidgets();
    void fitCellColumns();
    void syncCellWidgets();
    void updateActions();
    void applyFilter();
    void updateButtonIcons();
    void cellClicked(const QModelIndex& index);
    /// The next syncCellWidgets() sets every control again.
    void invalidateCells();

    // Shown without a session; declared first, since m_model is made for it.
    core::Document m_noDocument;
    DocumentSession* m_session{nullptr};
    // The editor of m_session, or of the constructor; nullptr for none.
    edit::Editor* m_editor{nullptr};
    LayerTableModel* m_model;
    QTableView* m_table;
    QLabel* m_message;
    QToolButton* m_newButton;
    QToolButton* m_deleteButton;
    QToolButton* m_currentButton;
    QLineEdit* m_search;
    QAction* m_newAction{nullptr};
    QAction* m_deleteAction{nullptr};
    QAction* m_currentAction{nullptr};

    /// What the controls of a row show now, so that a refresh sets only the
    /// ones that changed: building a drop-down list is the costly part.
    struct CellValues {
        core::Color color{core::Color::byLayer()};
        core::LinetypeId linetype;
        int lineweight{0};
        bool enabled{false};
        bool valid{false};
    };

    std::vector<CellValues> m_cells;
    std::size_t m_linetypeCount{0};
    int m_lwdefault{0};
    // The columns have their first widths: later fits only widen them.
    bool m_columnsFitted{false};
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

    LayerPanel* m_panel;
    LayerCombo* m_combo;
    QToolButton* m_on;
    QToolButton* m_frozen;
    QToolButton* m_locked;
};

}  // namespace tg::app
