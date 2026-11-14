// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "document_session.hpp"
#include "property_grid.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/cmd/quick_select.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"

#include <QDockWidget>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QWidget>

#include <array>
#include <functional>
#include <string>
#include <vector>

class QAction;
class QComboBox;
class QEvent;
class QLabel;
class QModelIndex;
class QToolButton;
class QTreeView;

namespace tg::app {

class ColorCombo;
class LayerCombo;
class LinetypeCombo;
class LineweightCombo;

/// Every icon the properties panel and its controls draw that no action
/// names: those of colour, linetype and lineweight (docs/ICONE.md).
[[nodiscard]] QStringList propertiesPanelIconNames();

/// The properties of the selection in a panel that can be docked
/// (docs/adr/0023, points 2–4; docs/tappe/0.6/P24).
///
/// At the top, the type of the selection with a filter by type and two
/// buttons: Filter, which opens the list of the filter by type, and Quick
/// Select, the action of setQuickSelectAction(). Below, a grid of two columns
/// (docs/adr/0039, point 1, area 8; PropertyGridModel): the group General
/// (colour, layer, linetype, linetype scale, lineweight), whose values change
/// in their cells, and, for a single entity, the group Geometry, read only,
/// with the browsing of the vertices of a polyline. A click on a group's row
/// closes or opens it. Without a selection the panel shows and changes the
/// current values, as in AutoCAD.
///
/// Every change is one transaction "Properties" of the editor, so U and
/// Ctrl+Z undo it. Entities on locked layers are never among those shown or
/// changed. Changes that come from elsewhere reach the panel when the owner
/// calls refresh().
///
/// The panel shows the session of setSession(), the active one
/// (docs/adr/0041, point 2): its document, its selection and its editor; or
/// the editor and the runner of the constructor that takes them. Without
/// either it shows the values of an empty document and changes nothing.
class PropertiesPanel final : public QDockWidget {
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(PropertiesPanel)

    /// A panel without a session, until setSession().
    explicit PropertiesPanel(QWidget* parent = nullptr);

    /// A panel working on `editor` and `runner` alone, without sessions, as
    /// the widget tests use it. Both must outlive the panel.
    PropertiesPanel(edit::Editor& editor, cmd::CommandRunner& runner, QWidget* parent = nullptr);

    ~PropertiesPanel() override;

    /// Shows and changes `session` from now on; nullptr for none. The
    /// session is borrowed: the owner calls setSession() again, with another
    /// session or nullptr, before destroying it.
    void setSession(DocumentSession* session);

    [[nodiscard]] DocumentSession* session() const { return m_session; }

    /// The action that opens the QSELECT window on the selection. The panel
    /// enables and disables it. May be null.
    void setQuickSelectAction(QAction* action);

    /// While a command runs the panel shows the properties but changes nothing.
    void setEditingEnabled(bool enabled);

    /// False also without a session.
    [[nodiscard]] bool editingEnabled() const { return m_editingEnabled && m_editor != nullptr; }

    /// The document shown; without a session, an empty document.
    [[nodiscard]] const core::Document& document() const {
        return m_editor != nullptr ? m_editor->document() : m_noDocument;
    }

    /// Reads the selection and the document again and shows them.
    void refresh();

    /// Shows the panel, brings it forward and gives it the keyboard focus.
    void showAndFocus();

    /// The selection shown: the runner's, without entities on locked layers.
    [[nodiscard]] const std::vector<core::EntityId>& selection() const { return m_selection; }

    /// The selection as it was at the last refresh while no command ran.
    [[nodiscard]] const std::vector<core::EntityId>& idleSelection() const {
        return m_idleSelection;
    }

    /// The entities that the panel shows and changes: the selection, limited
    /// to the type of the filter. Empty without a selection.
    [[nodiscard]] std::vector<core::EntityId> targets() const;

    [[nodiscard]] const ShownProperties& shown() const { return m_shown; }

    /// The texts of the filter by type, as the drop-down list shows them.
    [[nodiscard]] QStringList typeLabels() const;

    /// Limits the panel to the selected entities of DXF type `type` ("LINE");
    /// empty for all of them. False, changing nothing, if none has that type.
    bool setTypeFilter(const std::string& type);

    [[nodiscard]] const std::string& typeFilter() const { return m_typeFilter; }

    /// The rows of the group Geometry: name and value. Empty unless the
    /// panel shows a single entity.
    [[nodiscard]] const QList<QPair<QString, QString>>& geometry() const {
        return m_model->geometry();
    }

    /// The model of the grid: General, Geometry and the current vertex.
    [[nodiscard]] PropertyGridModel& grid() { return *m_model; }

    /// Each changes the targets, or the current value without a selection.
    /// False if nothing changed, or the change was refused (lastMessage()).
    bool setColor(core::Color color);
    bool setLayer(core::LayerId layer);
    bool setLinetype(core::LinetypeId linetype);
    bool setLinetypeScale(double scale);
    bool setLineweight(int lineweight);

    /// Opens the QSELECT window, which searches within `current` if the user
    /// chooses so. Cancel gives the runner `current` back as its selection.
    void showQuickSelect(std::vector<core::EntityId> current);

    /// Selects what cmd::quickSelect finds and writes "<n> item(s)
    /// selected." in the history. False while a command runs.
    bool applyQuickSelect(const cmd::QuickSelectCriteria& criteria,
                          const std::vector<core::EntityId>& within);

    /// The reason of the last refused change, or the last message.
    [[nodiscard]] const QString& lastMessage() const { return m_lastMessage; }

Q_SIGNALS:
    /// A message for the history of the command line.
    void messageShown(const QString& text);

    /// After every refresh: the controls of the ribbon follow it.
    void refreshed();

    /// The panel changed the drawing or the selection: the window updates.
    void changed();

    /// setSession() is about to show another session: what reads the one
    /// shown now lets it go.
    void sessionAboutToChange();

protected:
    void changeEvent(QEvent* event) override;

private:
    /// The editor and the runner shown. Precondition: there are some.
    [[nodiscard]] edit::Editor& editor() const;
    [[nodiscard]] cmd::CommandRunner& runner() const;
    /// True while the runner shown runs a command.
    [[nodiscard]] bool commandRunning() const;
    /// Shows and changes `editor` with `runner` from now on, both nullptr
    /// for none: a new model for its document.
    void attach(edit::Editor* editor, cmd::CommandRunner* runner);
    /// Gives the grid m_model, made for document(), and connects it.
    void connectModel();
    [[nodiscard]] bool canEdit() const;
    /// Gives every target, or the current values, the properties that
    /// `edit` makes of their own, in one transaction "Properties".
    bool change(const std::function<void(core::EntityProperties&)>& edit);
    bool refuse(const QString& text);
    void setMessage(const QString& text);
    void readSelection();
    void showTypes();
    void readShown();
    /// Gives the open editors of the grid the values shown.
    void showEditors();
    void showGeometry();
    /// Opens or closes the groups as the user left them.
    void showGroups();
    void toggleGroup(const QModelIndex& index);
    void enterLinetypeScale(const QString& text);
    void updateIcons();
    void updateEnabled();

    // Shown without a session; declared first, since m_model is made for it.
    core::Document m_noDocument;
    DocumentSession* m_session{nullptr};
    // Those of m_session, or of the constructor; nullptr for none.
    edit::Editor* m_editor{nullptr};
    cmd::CommandRunner* m_runner{nullptr};
    QComboBox* m_types;
    QToolButton* m_filter;
    QToolButton* m_quickSelect;
    PropertyGridModel* m_model;
    PropertyGridDelegate* m_delegate;
    QTreeView* m_grid;
    QLabel* m_message;
    // The width of the names, which the user can drag; a new model keeps it.
    int m_nameWidth;
    QAction* m_quickSelectAction{nullptr};

    std::vector<core::EntityId> m_selection;
    std::vector<core::EntityId> m_idleSelection;
    std::string m_typeFilter;
    ShownProperties m_shown;
    /// The groups the user closed, by PropertyGridModel group.
    std::array<bool, PropertyGridModel::kGroupCount> m_closed{};
    QString m_lastMessage;
    bool m_editingEnabled{true};
};

/// The drop-down lists of colour, linetype and lineweight, for the ribbon
/// and the toolbar `Properties`. They follow the panel and change the
/// document through it: the selection, or the current values without one.
class PropertyControls final : public QWidget {
    Q_OBJECT

public:
    /// The controls stop working if the panel is destroyed first. Side by
    /// side (Qt::Horizontal, for a toolbar) or one under the other, each
    /// with its icon on the left (Qt::Vertical, for a group of the ribbon,
    /// docs/adr/0039, point 1, area 3).
    explicit PropertyControls(PropertiesPanel& panel, Qt::Orientation orientation = Qt::Horizontal,
                              QWidget* parent = nullptr);

protected:
    void changeEvent(QEvent* event) override;

private:
    void sync();
    void updateIcons();

    PropertiesPanel* m_panel;
    ColorCombo* m_color;
    LinetypeCombo* m_linetype;
    LineweightCombo* m_lineweight;
    QList<QPair<QLabel*, QString>> m_icons;
};

}  // namespace tg::app
