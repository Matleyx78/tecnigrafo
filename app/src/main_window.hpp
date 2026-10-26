// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"

#include <QList>
#include <QMainWindow>
#include <QString>

#include <cstdint>
#include <functional>
#include <memory>

class QAction;
class QCloseEvent;
class QMenu;
class QSettings;
class QLabel;
class QToolButton;

namespace tg::app {

class ActionRegistry;
class CadView;
class CommandLine;
class RecentFiles;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    /// What the window shows when it opens.
    enum class InitialDrawing : std::uint8_t {
        /// The sample drawing of sample_drawing.hpp, as the program opens.
        Sample,
        /// Nothing: the control mode starts from an empty drawing
        /// (docs/adr/0011, level 2).
        Empty,
    };

    explicit MainWindow(InitialDrawing drawing = InitialDrawing::Sample, QWidget* parent = nullptr);

    ~MainWindow() override;

    /// Script sessions never show dialogs or access application settings.
    void setScriptMode();

    /// Replaces the platform file picker; an empty answer means Cancel.
    using FileDialog = std::function<QString(cmd::FilePrompt, const QString&)>;
    void setFileDialog(FileDialog dialog);

    [[nodiscard]] CadView* view() const { return m_view; }

    [[nodiscard]] CommandLine* commandLine() const { return m_commandLine; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    /// The drawing's undo history, read by the report of the control mode.
    [[nodiscard]] const edit::Editor& editor() const { return m_editor; }

    /// The runner of the commands, read by the report of the control mode.
    [[nodiscard]] const cmd::CommandRunner& runner() const { return m_runner; }

    /// File access for the commands, not owned; it must outlive the window.
    void setFileAccess(io::FileAccess* files) {
        m_externalFiles = true;
        m_runner.setFileAccess(files);
    }

    [[nodiscard]] const cmd::SelectionSet& selection() const { return m_runner.selection(); }

    [[nodiscard]] const cmd::DrawingAids& drawingAids() const { return m_runner.drawingAids(); }

    /// Every action of the window (docs/adr/0009, point 1).
    [[nodiscard]] const ActionRegistry& actionRegistry() const { return *m_actions; }

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    // Connected with the function-pointer syntax of QObject::connect, so no
    // "slots" section is needed.
    void showCursorPosition(double worldX, double worldY);
    void showScale(double pixelsPerUnit);
    void showAbout();

    /// Declares the actions in the registry, connects them and builds the
    /// menus from it.
    void createMenus();
    void connectDrawingAids();
    void createStatusBar();

    /// The action `id` of builtin_actions.hpp; it must exist.
    [[nodiscard]] QAction* registeredAction(const char* id) const;

    /// A status bar button that mirrors the checkable `action` both ways.
    QToolButton* createModeButton(QAction* action, const QString& text, const QString& name);

    /// Hands the runner the drawing aids chosen in the Tools menu.
    void applyDrawingAids();

    /// After any input or change: redraws the view and updates the prompt
    /// and the Edit actions.
    void refresh();
    void updateEditActions();
    void updateDrawingState();
    void submitFileText(const QString& text);
    void handleFilePrompts();
    void ensureRecentFiles();
    void addRecentFile(const QString& path);
    void populateRecentMenu();
    void openRecentFile(const QString& path);
    [[nodiscard]] QString dialogPath();
    [[nodiscard]] QString chooseFile(cmd::FilePrompt prompt, const QString& path);

    // Declared in dependency order: each member is built from the ones above.
    // The sample drawing goes straight into the document, without a
    // transaction, so it cannot be undone.
    core::Document m_document;
    edit::Editor m_editor;
    cmd::CommandRegistry m_registry;
    cmd::CommandRunner m_runner;
    std::unique_ptr<io::FileAccess> m_diskFiles;
    std::unique_ptr<QSettings> m_settings;
    std::unique_ptr<RecentFiles> m_recentFiles;
    FileDialog m_fileDialog;
    QString m_pendingRecent;
    QString m_pendingOpen;
    // The actions that wait for the running command to end: those with a
    // command, as typing does.
    QList<QAction*> m_idleActions;
    ActionRegistry* m_actions{nullptr};
    QMenu* m_recentMenu{nullptr};
    QAction* m_recentAction{nullptr};
    bool m_scriptMode{false};
    bool m_externalFiles{false};
    bool m_handlingFilePrompt{false};
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
    QAction* m_insertionAction{nullptr};
    QLabel* m_cursorLabel{nullptr};
    QLabel* m_scaleLabel{nullptr};
};

}  // namespace tg::app
