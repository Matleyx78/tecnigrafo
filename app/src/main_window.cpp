// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "main_window.hpp"

#include "cad_view.hpp"
#include "command_line.hpp"
#include "sample_drawing.hpp"

#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>

namespace tg::app {

namespace {

// Decimal places of the coordinate read-out.
constexpr int kCoordinateDecimals = 4;

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      m_document(makeSampleDrawing()),
      m_editor(m_document),
      m_runner(m_registry, m_editor),
      m_view(new CadView(m_document, this)),
      m_commandLine(new CommandLine(m_runner, this)) {
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
}

void MainWindow::createMenus() {
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
    QAction* quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

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
