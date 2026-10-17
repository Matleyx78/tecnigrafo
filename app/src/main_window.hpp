// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"

#include <QMainWindow>
#include <QString>

class QAction;
class QLabel;
class QToolButton;

namespace tg::app {

class CadView;
class CommandLine;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    [[nodiscard]] CadView* view() const { return m_view; }

    [[nodiscard]] CommandLine* commandLine() const { return m_commandLine; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] const cmd::SelectionSet& selection() const { return m_runner.selection(); }

    [[nodiscard]] const cmd::DrawingAids& drawingAids() const { return m_runner.drawingAids(); }

private:
    // Connected with the function-pointer syntax of QObject::connect, so no
    // "slots" section is needed.
    void showCursorPosition(double worldX, double worldY);
    void showScale(double pixelsPerUnit);
    void showAbout();

    void createMenus();
    void createToolsMenu();
    void createStatusBar();

    /// A status bar button that mirrors the checkable `action` both ways.
    QToolButton* createModeButton(QAction* action, const QString& text, const QString& name);

    /// Hands the runner the drawing aids chosen in the Tools menu.
    void applyDrawingAids();

    /// After any input or change: redraws the view and updates the prompt
    /// and the Edit actions.
    void refresh();
    void updateEditActions();

    // Declared in dependency order: each member is built from the ones above.
    // The sample drawing goes straight into the document, without a
    // transaction, so it cannot be undone.
    core::Document m_document;
    edit::Editor m_editor;
    cmd::CommandRegistry m_registry;
    cmd::CommandRunner m_runner;
    CadView* m_view{nullptr};
    CommandLine* m_commandLine{nullptr};
    QAction* m_undoAction{nullptr};
    QAction* m_redoAction{nullptr};
    QAction* m_eraseAction{nullptr};
    QAction* m_objectSnapAction{nullptr};
    QAction* m_orthoAction{nullptr};
    QAction* m_endpointAction{nullptr};
    QAction* m_midpointAction{nullptr};
    QAction* m_centerAction{nullptr};
    QLabel* m_cursorLabel{nullptr};
    QLabel* m_scaleLabel{nullptr};
};

}  // namespace tg::app
