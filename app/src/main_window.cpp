// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "main_window.hpp"

#include "cad_view.hpp"
#include "command_line.hpp"
#include "sample_drawing.hpp"
#include "tg/cmd/builtin_commands.hpp"

#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace tg::app {

namespace {

// Decimal places of the coordinate read-out.
constexpr int kCoordinateDecimals = 4;

// The runner's own commands, used by the Edit menu so that Ctrl+Z behaves
// exactly like typing U (message in the history, Enter repeats it).
constexpr const char* kUndoCommand = "UNDO";
constexpr const char* kRedoCommand = "REDO";

/// The platform's redo keys, plus Ctrl+Y that AutoCAD users expect
/// everywhere (CommandLine recognises the same keys).
QList<QKeySequence> redoShortcuts() {
    QList<QKeySequence> keys = QKeySequence::keyBindings(QKeySequence::Redo);
    const QKeySequence ctrlY(Qt::CTRL | Qt::Key_Y);
    if (!keys.contains(ctrlY)) {
        keys.append(ctrlY);
    }
    return keys;
}

/// A checkable action of `menu`, named `name` for the tests.
QAction* addCheckableAction(QMenu* menu, const QString& text, const QString& name, bool checked) {
    QAction* action = menu->addAction(text);
    action->setObjectName(name);
    action->setCheckable(true);
    action->setChecked(checked);
    return action;
}

}  // namespace

MainWindow::MainWindow(InitialDrawing drawing, QWidget* parent)
    : QMainWindow(parent),
      m_document(drawing == InitialDrawing::Sample ? makeSampleDrawing() : core::Document{}),
      m_editor(m_document),
      m_runner(m_registry, m_editor),
      m_view(new CadView(m_document, m_runner, this)),
      m_commandLine(new CommandLine(m_runner, this)) {
    cmd::registerBuiltinCommands(m_registry);
    setWindowTitle(QStringLiteral("Tecnigrafo"));
    resize(1100, 720);

    // The command line sits below the drawing, as in AutoCAD.
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_view, 1);
    layout->addWidget(m_commandLine);
    setCentralWidget(central);
    m_commandLine->forwardKeysFrom(m_view);

    createMenus();
    createStatusBar();

    connect(m_view, &CadView::cursorMoved, this, &MainWindow::showCursorPosition);
    connect(m_view, &CadView::scaleChanged, this, &MainWindow::showScale);

    // The editor's listener runs inside commit(), while the command that
    // commits is still running: the inputs' own signals refresh again once
    // the runner is idle, so that Undo is enabled when the command ends.
    m_editor.setChangeListener([this] { refresh(); });
    connect(m_view, &CadView::inputHandled, this, &MainWindow::refresh);
    connect(m_commandLine, &CommandLine::inputHandled, this, &MainWindow::refresh);
    updateEditActions();
}

void MainWindow::createMenus() {
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QAction* quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));
    m_undoAction = editMenu->addAction(tr("&Undo"));
    m_undoAction->setObjectName(QStringLiteral("undoAction"));
    m_undoAction->setShortcuts(QKeySequence::Undo);
    connect(m_undoAction, &QAction::triggered, this, [this] {
        m_runner.submitText(kUndoCommand);
        refresh();
    });
    m_redoAction = editMenu->addAction(tr("&Redo"));
    m_redoAction->setObjectName(QStringLiteral("redoAction"));
    m_redoAction->setShortcuts(redoShortcuts());
    connect(m_redoAction, &QAction::triggered, this, [this] {
        m_runner.submitText(kRedoCommand);
        refresh();
    });

    editMenu->addSeparator();
    m_eraseAction = editMenu->addAction(tr("&Erase"));
    m_eraseAction->setObjectName(QStringLiteral("eraseAction"));
    m_eraseAction->setShortcut(QKeySequence::Delete);
    connect(m_eraseAction, &QAction::triggered, this, [this] {
        m_runner.submitText("ERASE");
        refresh();
    });

    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));
    QAction* extentsAction = viewMenu->addAction(tr("Zoom &Extents"));
    extentsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E));
    connect(extentsAction, &QAction::triggered, m_view, &CadView::zoomExtents);

    createToolsMenu();

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction* aboutAction = helpMenu->addAction(tr("&About Tecnigrafo"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::createToolsMenu() {
    // The menu starts from the runner's settings, the defaults of
    // docs/adr/0008: object snap on with every mode, ortho off.
    const cmd::DrawingAids& aids = m_runner.drawingAids();
    QMenu* toolsMenu = menuBar()->addMenu(tr("&Tools"));

    // F3 and F8 are AutoCAD's keys. On laptops and on macOS they may need
    // Fn: the same switches are in the status bar (docs/tappe/0.3).
    m_objectSnapAction = addCheckableAction(toolsMenu, tr("Object &Snap"),
                                            QStringLiteral("objectSnapAction"), aids.objectSnap);
    m_objectSnapAction->setShortcut(QKeySequence(Qt::Key_F3));
    m_orthoAction =
        addCheckableAction(toolsMenu, tr("&Ortho"), QStringLiteral("orthoAction"), aids.ortho);
    m_orthoAction->setShortcut(QKeySequence(Qt::Key_F8));

    QMenu* modesMenu = toolsMenu->addMenu(tr("Object Snap &Modes"));
    m_endpointAction = addCheckableAction(
        modesMenu, tr("&Endpoint"), QStringLiteral("endpointSnapAction"), aids.snapModes.endpoint);
    m_midpointAction = addCheckableAction(
        modesMenu, tr("&Midpoint"), QStringLiteral("midpointSnapAction"), aids.snapModes.midpoint);
    m_centerAction = addCheckableAction(modesMenu, tr("&Center"),
                                        QStringLiteral("centerSnapAction"), aids.snapModes.center);

    // Connected after the initial states are set, so that starting the
    // program writes nothing in the history.
    for (QAction* action :
         {m_objectSnapAction, m_orthoAction, m_endpointAction, m_midpointAction, m_centerAction}) {
        connect(action, &QAction::toggled, this, &MainWindow::applyDrawingAids);
    }
    // The switches are echoed in the history, as AutoCAD does.
    connect(m_objectSnapAction, &QAction::toggled, this, [this](bool on) {
        m_commandLine->appendToHistory(on ? tr("<Osnap on>") : tr("<Osnap off>"));
    });
    connect(m_orthoAction, &QAction::toggled, this, [this](bool on) {
        m_commandLine->appendToHistory(on ? tr("<Ortho on>") : tr("<Ortho off>"));
    });
}

void MainWindow::applyDrawingAids() {
    cmd::DrawingAids aids;
    aids.objectSnap = m_objectSnapAction->isChecked();
    aids.snapModes.endpoint = m_endpointAction->isChecked();
    aids.snapModes.midpoint = m_midpointAction->isChecked();
    aids.snapModes.center = m_centerAction->isChecked();
    aids.ortho = m_orthoAction->isChecked();
    m_runner.setDrawingAids(aids);
    m_view->update();  // the snap marker and the preview may have changed
}

QToolButton* MainWindow::createModeButton(QAction* action, const QString& text,
                                          const QString& name) {
    auto* button = new QToolButton(this);
    button->setObjectName(name);
    button->setText(text);
    button->setToolTip(
        QStringLiteral("%1 (%2)").arg(action->text().remove(QLatin1Char('&')),
                                      action->shortcut().toString(QKeySequence::NativeText)));
    button->setCheckable(true);
    button->setChecked(action->isChecked());
    button->setAutoRaise(true);
    // A click must not take the keyboard away from the drawing and the
    // command line.
    button->setFocusPolicy(Qt::NoFocus);
    // Both ways; setChecked() with the current state emits nothing, so the
    // two signals do not call each other forever.
    connect(button, &QToolButton::toggled, action, &QAction::setChecked);
    connect(action, &QAction::toggled, button, &QToolButton::setChecked);
    return button;
}

void MainWindow::createStatusBar() {
    // In AutoCAD's order: ORTHO before OSNAP.
    statusBar()->addPermanentWidget(
        createModeButton(m_orthoAction, tr("ORTHO"), QStringLiteral("orthoButton")));
    statusBar()->addPermanentWidget(
        createModeButton(m_objectSnapAction, tr("OSNAP"), QStringLiteral("osnapButton")));
    m_cursorLabel = new QLabel(this);
    m_scaleLabel = new QLabel(this);
    m_cursorLabel->setMinimumWidth(260);
    statusBar()->addPermanentWidget(m_scaleLabel);
    statusBar()->addPermanentWidget(m_cursorLabel);
    showCursorPosition(0.0, 0.0);
    showScale(m_view->viewport().scale());
}

void MainWindow::refresh() {
    m_view->update();
    m_commandLine->refreshPrompt();
    updateEditActions();
}

void MainWindow::updateEditActions() {
    // Undo and redo wait for the running command to end (docs/tappe/0.2).
    const bool idle = !m_runner.isActive();
    m_eraseAction->setEnabled(idle && !m_runner.selection().isEmpty());
    const QString undoName = QString::fromStdString(m_editor.undoName());
    const QString redoName = QString::fromStdString(m_editor.redoName());
    m_undoAction->setEnabled(idle && m_editor.canUndo());
    m_redoAction->setEnabled(idle && m_editor.canRedo());
    m_undoAction->setText(undoName.isEmpty() ? tr("&Undo") : tr("&Undo %1").arg(undoName));
    m_redoAction->setText(redoName.isEmpty() ? tr("&Redo") : tr("&Redo %1").arg(redoName));
}

void MainWindow::showCursorPosition(double worldX, double worldY) {
    m_cursorLabel->setText(tr("X: %1   Y: %2")
                               .arg(worldX, 0, 'f', kCoordinateDecimals)
                               .arg(worldY, 0, 'f', kCoordinateDecimals));
}

void MainWindow::showScale(double pixelsPerUnit) {
    m_scaleLabel->setText(tr("%1 px/unit").arg(pixelsPerUnit, 0, 'g', 4));
}

void MainWindow::showAbout() {
    QMessageBox::about(this, tr("About Tecnigrafo"),
                       tr("<h3>Tecnigrafo %1</h3>"
                          "<p>Free 2D/3D CAD for Windows, Linux and macOS.</p>"
                          "<p>Released under the GNU General Public License v3 or later.</p>")
                           .arg(QApplication::applicationVersion()));
}

}  // namespace tg::app
