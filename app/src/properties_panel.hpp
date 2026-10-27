// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

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

#include <functional>
#include <optional>
#include <string>
#include <vector>

class QAction;
class QComboBox;
class QEvent;
class QFormLayout;
class QGroupBox;
class QLabel;
class QLineEdit;

namespace tg::app {

class ColorCombo;
class LayerCombo;
class LinetypeCombo;
class LineweightCombo;

/// Every icon the properties panel and its controls draw that no action
/// names: those of colour, linetype and lineweight (docs/ICONE.md).
[[nodiscard]] QStringList propertiesPanelIconNames();

/// The values the panel shows: the one every entity shares, or nothing when
/// they differ (*VARIES*). With no selection, the current values.
struct ShownProperties {
    std::optional<core::Color> color;
    std::optional<core::LayerId> layer;
    std::optional<core::LinetypeId> linetype;
    std::optional<double> linetypeScale;
    std::optional<int> lineweight;
};

/// The properties of the selection in a panel that can be docked
/// (docs/adr/0023, points 2–4; docs/tappe/0.6/P24).
///
/// At the top, the type of the selection with a filter by type; then the
/// section General (colour, layer, linetype, linetype scale, lineweight) and,
/// for a single entity, the section Geometry, read only. Without a selection
/// the panel shows and changes the current values, as in AutoCAD.
///
/// Every change is one transaction "Properties" of the editor, so U and
/// Ctrl+Z undo it. Entities on locked layers are never among those shown or
/// changed. Changes that come from elsewhere reach the panel when the owner
/// calls refresh().
///
/// The editor and the runner are borrowed and must outlive the panel.
class PropertiesPanel final : public QDockWidget {
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(PropertiesPanel)

    PropertiesPanel(edit::Editor& editor, cmd::CommandRunner& runner, QWidget* parent = nullptr);
    ~PropertiesPanel() override;

    /// The action that opens the QSELECT window on the selection. The panel
    /// enables and disables it. May be null.
    void setQuickSelectAction(QAction* action);

    /// While a command runs the panel shows the properties but changes nothing.
    void setEditingEnabled(bool enabled);

    [[nodiscard]] bool editingEnabled() const { return m_editingEnabled; }

    [[nodiscard]] const core::Document& document() const { return m_editor.document(); }

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

    /// The rows of the section Geometry: name and value. Empty unless the
    /// panel shows a single entity.
    [[nodiscard]] const QList<QPair<QString, QString>>& geometry() const { return m_geometry; }

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

protected:
    void changeEvent(QEvent* event) override;

private:
    [[nodiscard]] bool canEdit() const;
    /// Gives every target, or the current values, the properties that
    /// `edit` makes of their own, in one transaction "Properties".
    bool change(const std::function<void(core::EntityProperties&)>& edit);
    bool refuse(const QString& text);
    void setMessage(const QString& text);
    void readSelection();
    void showTypes();
    void readShown();
    void showValues();
    void showGeometry();
    void updateIcons();
    void updateEnabled();

    edit::Editor& m_editor;
    cmd::CommandRunner& m_runner;
    QComboBox* m_types;
    QGroupBox* m_general;
    ColorCombo* m_color;
    LayerCombo* m_layer;
    LinetypeCombo* m_linetype;
    QLineEdit* m_linetypeScale;
    LineweightCombo* m_lineweight;
    QGroupBox* m_geometryBox;
    QFormLayout* m_geometryForm;
    QLabel* m_message;
    QList<QPair<QLabel*, QString>> m_icons;
    QAction* m_quickSelectAction{nullptr};

    std::vector<core::EntityId> m_selection;
    std::vector<core::EntityId> m_idleSelection;
    std::string m_typeFilter;
    ShownProperties m_shown;
    QList<QPair<QString, QString>> m_geometry;
    QString m_lastMessage;
    bool m_editingEnabled{true};
};

/// The drop-down lists of colour, linetype and lineweight, for the ribbon
/// and the toolbar `Properties`. They follow the panel and change the
/// document through it: the selection, or the current values without one.
class PropertyControls final : public QWidget {
    Q_OBJECT

public:
    /// The controls stop working if the panel is destroyed first.
    explicit PropertyControls(PropertiesPanel& panel, QWidget* parent = nullptr);

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
