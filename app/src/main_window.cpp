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
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
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

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_document(makeSampleDrawing()),
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

    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction* aboutAction = helpMenu->addAction(tr("&About Tecnigrafo"));
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::createStatusBar() {
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
