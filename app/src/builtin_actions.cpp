// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "builtin_actions.hpp"

#include "action_registry.hpp"

#include <QAction>
#include <QCoreApplication>
#include <QKeySequence>
#include <QList>
#include <QString>

#include <utility>

namespace tg::app {

namespace {

/// Gives the texts of the actions their own translation context.
class BuiltinActions {
    Q_DECLARE_TR_FUNCTIONS(BuiltinActions)

public:
    static void add(ActionRegistry& registry);
};

ActionSpec makeSpec(const QString& id, const QString& text, const QString& menu,
                    const QString& description) {
    ActionSpec spec;
    spec.id = id;
    spec.text = text;
    spec.menu = menu;
    spec.description = description;
    if (id.startsWith(QStringLiteral("draw."))) {
        spec.ribbon = QStringLiteral("home/draw");
    } else if (id.startsWith(QStringLiteral("edit."))) {
        spec.ribbon = QStringLiteral("home/modify");
    } else if (id.startsWith(QStringLiteral("view."))) {
        spec.ribbon = QStringLiteral("view/navigate");
    } else if (id.startsWith(QStringLiteral("aids."))) {
        spec.ribbon = QStringLiteral("view/aids");
    } else if (id.startsWith(QStringLiteral("layer."))) {
        spec.ribbon = QStringLiteral("home/layers");
    }
    return spec;
}

/// `spec` with its icon: the name of a file of resources/icons without the
/// ".svg" (docs/ICONE.md).
ActionSpec withIcon(ActionSpec spec, const QString& icon) {
    spec.icon = icon;
    return spec;
}

/// A separator of `menu`.
void addSeparator(ActionRegistry& registry, const QString& id, const QString& menu) {
    ActionSpec spec;
    spec.id = id;
    spec.menu = menu;
    registry.add(std::move(spec));
}

/// Adds `spec` with all of `keys`: the first is the spec's default, the
/// others are the platform's alternatives (Qt's standard key bindings).
QAction* addWithKeys(ActionRegistry& registry, ActionSpec spec, const QList<QKeySequence>& keys) {
    spec.shortcut = keys.value(0);
    QAction* action = registry.add(std::move(spec));
    action->setShortcuts(keys);
    return action;
}

/// The platform's keys of `key`, plus `extra` when the platform lacks it.
QList<QKeySequence> keysWith(QKeySequence::StandardKey key, const QKeySequence& extra) {
    QList<QKeySequence> keys = QKeySequence::keyBindings(key);
    if (!extra.isEmpty() && !keys.contains(extra)) {
        keys.append(extra);
    }
    return keys;
}

/// An action that runs `command` as if typed in the command line.
ActionSpec commandSpec(const QString& id, const QString& text, const QString& menu,
                       const QString& description, const QString& command) {
    ActionSpec spec = makeSpec(id, text, menu, description);
    spec.command = command;
    return spec;
}

/// A switch of the drawing aids (docs/adr/0008).
ActionSpec switchSpec(const QString& id, const QString& text, const QString& menu,
                      const QString& description) {
    ActionSpec spec = makeSpec(id, text, menu, description);
    spec.checkable = true;
    return spec;
}

void BuiltinActions::add(ActionRegistry& registry) {
    const QString file = tr("&File");
    // Some Qt platforms (including offscreen) have no SaveAs binding: keep
    // the platform keys and always offer the CAD shortcut (docs/tappe/0.5).
    addWithKeys(registry,
                withIcon(commandSpec(QStringLiteral("file.new"), tr("&New"), file,
                                     tr("Starts a new drawing."), QStringLiteral("NEW")),
                         QStringLiteral("file-new")),
                keysWith(QKeySequence::New, {}));
    addWithKeys(registry,
                withIcon(commandSpec(QStringLiteral("file.open"), tr("&Open…"), file,
                                     tr("Opens a drawing."), QStringLiteral("OPEN")),
                         QStringLiteral("file-open")),
                keysWith(QKeySequence::Open, {}));
    addWithKeys(registry,
                withIcon(commandSpec(QStringLiteral("file.save"), tr("&Save"), file,
                                     tr("Saves the drawing."), QStringLiteral("QSAVE")),
                         QStringLiteral("file-save")),
                keysWith(QKeySequence::Save, {}));
    addWithKeys(
        registry,
        withIcon(commandSpec(QStringLiteral("file.saveas"), tr("Save &As…"), file,
                             tr("Saves the drawing with a new name."), QStringLiteral("SAVEAS")),
                 QStringLiteral("file-saveas")),
        keysWith(QKeySequence::SaveAs, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S)));
    // The Recent Files submenu: MainWindow gives this action its menu and
    // fills it with the run-time actions file.recent.1...
    registry.add(makeSpec(QStringLiteral("file.recent"), tr("Recent Files"), file,
                          tr("Opens a drawing used recently.")));
    addSeparator(registry, QStringLiteral("file.separator-quit"), file);
    ActionSpec quit =
        makeSpec(QStringLiteral("file.quit"), tr("&Quit"), file, tr("Closes Tecnigrafo."));
    quit.shortcut = QKeySequence(QKeySequence::Quit);
    registry.add(std::move(quit));

    const QString edit = tr("&Edit");
    // No command: undo and redo send the runner UNDO and REDO without the
    // echo of a typed command, so that Ctrl+Z writes only "Undone: ..." in
    // the history, as before the registry. MainWindow connects them.
    addWithKeys(registry,
                withIcon(makeSpec(QStringLiteral("edit.undo"), tr("&Undo"), edit,
                                  tr("Undoes the last change.")),
                         QStringLiteral("edit-undo")),
                keysWith(QKeySequence::Undo, {}));
    // Ctrl+Y as well, which AutoCAD users expect everywhere (CommandLine
    // recognises the same keys).
    addWithKeys(registry,
                withIcon(makeSpec(QStringLiteral("edit.redo"), tr("&Redo"), edit,
                                  tr("Redoes the last change undone.")),
                         QStringLiteral("edit-redo")),
                keysWith(QKeySequence::Redo, QKeySequence(Qt::CTRL | Qt::Key_Y)));
    addSeparator(registry, QStringLiteral("edit.separator-erase"), edit);
    ActionSpec erase = commandSpec(QStringLiteral("edit.erase"), tr("&Erase"), edit,
                                   tr("Removes the selected objects."), QStringLiteral("ERASE"));
    erase.icon = QStringLiteral("edit-erase");
    erase.shortcut = QKeySequence(QKeySequence::Delete);
    registry.add(std::move(erase));

    ActionSpec extents = makeSpec(QStringLiteral("view.zoom-extents"), tr("Zoom &Extents"),
                                  tr("&View"), tr("Shows the whole drawing."));
    extents.icon = QStringLiteral("view-zoom-extents");
    extents.shortcut = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E);
    registry.add(std::move(extents));

    // F3 and F8 are AutoCAD's keys. On laptops and on macOS they may need
    // Fn: the same switches are in the status bar (docs/tappe/0.3).
    const QString tools = tr("&Tools");
    ActionSpec osnap = switchSpec(QStringLiteral("aids.osnap"), tr("Object &Snap"), tools,
                                  tr("Turns object snap on or off."));
    osnap.icon = QStringLiteral("aids-osnap");
    osnap.shortcut = QKeySequence(Qt::Key_F3);
    registry.add(std::move(osnap));
    ActionSpec ortho =
        switchSpec(QStringLiteral("aids.ortho"), tr("&Ortho"), tools, tr("Turns ortho on or off."));
    ortho.icon = QStringLiteral("aids-ortho");
    ortho.shortcut = QKeySequence(Qt::Key_F8);
    registry.add(std::move(ortho));
    ActionSpec polar = switchSpec(QStringLiteral("aids.polar"), tr("&Polar Tracking"), tools,
                                  tr("Turns polar tracking on or off."));
    polar.icon = QStringLiteral("aids-polar");
    polar.shortcut = QKeySequence(Qt::Key_F10);
    registry.add(std::move(polar));
    registry.add(withIcon(switchSpec(QStringLiteral("aids.lwdisplay"), tr("Line&weight Display"),
                                     tools, tr("Shows lineweights in the drawing.")),
                          QStringLiteral("aids-lwdisplay")));
    registry.add(withIcon(makeSpec(QStringLiteral("tools.settings"), tr("&Settings…"), tools,
                                   tr("Chooses the interface and theme.")),
                          QStringLiteral("tools-settings")));
    registry.add(withIcon(makeSpec(QStringLiteral("tools.shortcuts"), tr("&Shortcuts…"), tools,
                                   tr("Customizes keyboard shortcuts.")),
                          QStringLiteral("tools-settings")));
    registry.add(withIcon(switchSpec(QStringLiteral("ui.theme"), tr("&Dark Theme"), tools,
                                     tr("Switches between light and dark themes.")),
                          QStringLiteral("ui-theme")));
    // The switch is between the ribbon and the classic menus: its icon is
    // the ribbon's (docs/ICONE.md).
    registry.add(withIcon(switchSpec(QStringLiteral("ui.interface"), tr("&Classic Interface"),
                                     tools, tr("Switches between ribbon and classic menus.")),
                          QStringLiteral("ui-ribbon")));
    // The layers panel (0.6-P22). No command: the actions work on the panel,
    // and LAYER (app_commands.cpp) only shows it.
    const QString layers = tools + QLatin1Char('/') + tr("&Layers");
    registry.add(withIcon(makeSpec(QStringLiteral("layer.panel"), tr("Layers &Panel"), layers,
                                   tr("Shows the layers panel.")),
                          QStringLiteral("layer-panel")));
    registry.add(withIcon(
        makeSpec(QStringLiteral("layer.new"), tr("&New Layer"), layers, tr("Adds a layer.")),
        QStringLiteral("layer-new")));
    registry.add(withIcon(makeSpec(QStringLiteral("layer.delete"), tr("&Delete Layer"), layers,
                                   tr("Deletes the selected layer.")),
                          QStringLiteral("layer-delete")));
    registry.add(withIcon(makeSpec(QStringLiteral("layer.current"), tr("Make &Current"), layers,
                                   tr("Makes the selected layer the current one.")),
                          QStringLiteral("layer-current")));
    // The properties panel and the quick selection (0.6-P24). No command:
    // the actions work on the panel, which keeps the selection; PROPERTIES
    // and QSELECT (app_commands.cpp) open the same.
    ActionSpec properties =
        withIcon(makeSpec(QStringLiteral("properties.panel"), tr("&Properties Panel"), tools,
                          tr("Shows the properties panel.")),
                 QStringLiteral("properties-panel"));
    properties.ribbon = QStringLiteral("home/properties");
    registry.add(std::move(properties));
    registry.add(withIcon(makeSpec(QStringLiteral("edit.quick-select"), tr("&Quick Select…"), tools,
                                   tr("Selects objects by type and property.")),
                          QStringLiteral("quick-select")));
    const QString modes = tools + QLatin1Char('/') + tr("Object Snap &Modes");
    registry.add(withIcon(switchSpec(QStringLiteral("aids.snap.endpoint"), tr("&Endpoint"), modes,
                                     tr("Snaps to the endpoints of lines, arcs and polylines.")),
                          QStringLiteral("aids-snap-endpoint")));
    registry.add(withIcon(switchSpec(QStringLiteral("aids.snap.midpoint"), tr("&Midpoint"), modes,
                                     tr("Snaps to the midpoints of lines, arcs and polylines.")),
                          QStringLiteral("aids-snap-midpoint")));
    registry.add(withIcon(switchSpec(QStringLiteral("aids.snap.center"), tr("&Center"), modes,
                                     tr("Snaps to the centers of circles and arcs.")),
                          QStringLiteral("aids-snap-center")));
    registry.add(withIcon(switchSpec(QStringLiteral("aids.snap.insertion"), tr("&Insertion"), modes,
                                     tr("Snaps to the insertion points of texts.")),
                          QStringLiteral("aids-snap-insertion")));

    registry.add(
        withIcon(switchSpec(QStringLiteral("aids.snap.intersection"), tr("&Intersection"), modes,
                            tr("Snaps to intersections of curves.")),
                 QStringLiteral("aids-snap-intersection")));
    registry.add(
        withIcon(switchSpec(QStringLiteral("aids.snap.perpendicular"), tr("&Perpendicular"), modes,
                            tr("Snaps perpendicular from the base point to curves.")),
                 QStringLiteral("aids-snap-perpendicular")));
    registry.add(withIcon(switchSpec(QStringLiteral("aids.snap.tangent"), tr("&Tangent"), modes,
                                     tr("Snaps tangent from the base point to curves.")),
                          QStringLiteral("aids-snap-tangent")));
    registry.add(withIcon(switchSpec(QStringLiteral("aids.snap.quadrant"), tr("&Quadrant"), modes,
                                     tr("Snaps to quadrants of circles and arcs.")),
                          QStringLiteral("aids-snap-quadrant")));

    const QString draw = tr("&Draw");
    registry.add(withIcon(commandSpec(QStringLiteral("draw.line"), tr("&Line"), draw,
                                      tr("Draws straight line segments."), QStringLiteral("LINE")),
                          QStringLiteral("draw-line")));
    registry.add(
        withIcon(commandSpec(QStringLiteral("draw.pline"), tr("&Polyline"), draw,
                             tr("Draws a polyline of lines and arcs."), QStringLiteral("PLINE")),
                 QStringLiteral("draw-pline")));
    registry.add(withIcon(commandSpec(QStringLiteral("draw.circle"), tr("&Circle"), draw,
                                      tr("Draws a circle."), QStringLiteral("CIRCLE")),
                          QStringLiteral("draw-circle")));
    registry.add(withIcon(commandSpec(QStringLiteral("draw.arc"), tr("&Arc"), draw,
                                      tr("Draws an arc."), QStringLiteral("ARC")),
                          QStringLiteral("draw-arc")));
    registry.add(withIcon(commandSpec(QStringLiteral("draw.text"), tr("&Text"), draw,
                                      tr("Writes single-line texts."), QStringLiteral("TEXT")),
                          QStringLiteral("draw-text")));

    registry.add(
        withIcon(makeSpec(QStringLiteral("help.about"), tr("&About Tecnigrafo"), tr("&Help"),
                          tr("Shows the version and the license of Tecnigrafo.")),
                 QStringLiteral("help-about")));
}

}  // namespace

void addBuiltinActions(ActionRegistry& registry) {
    BuiltinActions::add(registry);
}

}  // namespace tg::app
