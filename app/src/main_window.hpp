// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "document_session.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/edit/editor.hpp"
#include "theme.hpp"
#include "ui_builder.hpp"

#include <QList>
#include <QString>
#include <QStringList>

#include <SARibbon.h>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class QAction;
class QCloseEvent;
class QDockWidget;
class QEvent;
class QShowEvent;
class QMenu;
class QSettings;
class QLabel;
class QToolButton;

namespace tg::app {

class ActionRegistry;
class AutoHide;
class CadView;
class CommandLine;
class LayerPanel;
class PropertiesPanel;
class RecentFiles;

/// The window draws its own frame, with the quick access bar in its title
/// row (docs/adr/0039, point 4), except under Wayland, where it keeps the
/// frame of the system and the quick access bar goes under the title.
///
/// The window owns a session for each open document (docs/adr/0041, point
/// 2), one of them active; the program opens with one. View, command line,
/// the panels Layers and Properties, Undo and Redo, the drawing aids, title
/// and status follow the active session (activate()); the listener of every
/// session updates the interface only while that session is active.
class MainWindow final : public SARibbonMainWindow {
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

    /// Chooses the AutoCAD script sequences, for the runner of every session,
    /// without writing to the history.
    void setScriptSequences(bool on);

    /// Replaces the platform file picker; an empty answer means Cancel.
    using FileDialog = std::function<QString(cmd::FilePrompt, const QString&)>;
    void setFileDialog(FileDialog dialog);

    [[nodiscard]] CadView* view() const { return m_view; }

    [[nodiscard]] CommandLine* commandLine() const { return m_commandLine; }

    /// The document of the active session. Precondition: a session is
    /// active, as it always is for scripts of level 2, the report of the
    /// control mode and the tests; callers inside the window check
    /// activeSession() instead.
    [[nodiscard]] const core::Document& document() const;

    /// The undo history of the active session, read by the report of the
    /// control mode. Precondition: a session is active.
    [[nodiscard]] const edit::Editor& editor() const;

    /// The runner of the active session, read by the report of the control
    /// mode. Precondition: a session is active.
    [[nodiscard]] const cmd::CommandRunner& runner() const;

    /// File access for the commands of every session, not owned; it must
    /// outlive the window.
    void setFileAccess(io::FileAccess* files);

    /// The selection of the active session. Precondition: a session is active.
    [[nodiscard]] const cmd::SelectionSet& selection() const { return runner().selection(); }

    /// The drawing aids of the active session's runner. Precondition: a
    /// session is active.
    [[nodiscard]] const cmd::DrawingAids& drawingAids() const { return runner().drawingAids(); }

    /// The active session; nullptr when there is none.
    [[nodiscard]] DocumentSession* activeSession() const { return m_active; }

    /// Every session, in the order they were opened. The window never moves
    /// them: a session stays at its address until closeSession().
    [[nodiscard]] const std::vector<std::unique_ptr<DocumentSession>>& sessions() const {
        return m_sessions;
    }

    /// Opens a session for `document`, with an empty history and no file,
    /// and returns it, not active: activate() shows it.
    DocumentSession& openSession(core::Document document = core::Document::newDrawing());

    /// The contract of activation (docs/adr/0041, point 2; docs/tappe/0.7.5/
    /// P28): the view keeps the centre and scale of the session it leaves;
    /// view, command line, panels, Undo and Redo, the drawing aids (which
    /// stay settings of the window) and the listener follow `session`, whose
    /// view comes back; title and status show it. The runner of the session
    /// left keeps its command, halfway or not, and the text typed and not
    /// sent stays in that session. Precondition: `session` is one of
    /// sessions().
    void activate(DocumentSession& session);

    /// Closes `session` without asking anything: a closed modified drawing
    /// is lost (the question to save is the job of the tabs, P29). View,
    /// command line, panels and listener let it go first; if it was active,
    /// the session opened before it, or else after it, becomes active, or
    /// none. Precondition: `session` is one of sessions().
    void closeSession(DocumentSession& session);

    /// Sets all snap switches and the runner, without writing in the history.
    void setSnapModes(const core::SnapModes& modes);

    /// Sets polar tracking silently, as !polar does. Turning ortho off still
    /// goes through its switch, as before (docs/adr/0033, point 9).
    void setPolar(bool on);

    void setTheme(Theme theme);

    [[nodiscard]] Theme theme() const { return m_theme; }

    /// Panels must have a unique, stable, nonempty objectName. A docked
    /// panel can hide itself (docs/tappe/0.7.5/P18).
    void addPanel(QDockWidget* panel, Qt::DockWidgetArea area);

    /// The panels that hide themselves and their tabs.
    [[nodiscard]] AutoHide& autoHide() const { return *m_autoHide; }

    void setInterface(InterfaceKind kind);
    [[nodiscard]] InterfaceKind interfaceKind() const;

    /// Every action of the window (docs/adr/0009, point 1).
    [[nodiscard]] const ActionRegistry& actionRegistry() const { return *m_actions; }

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    // Connected with the function-pointer syntax of QObject::connect, so no
    // "slots" section is needed.
    void showCursorPosition(double worldX, double worldY);
    void showScale(double pixelsPerUnit);
    void showAbout();

    /// Declares the actions in the registry, connects them and builds the
    /// menus from it.
    void createMenus();
    void showSettings();
    void restoreUiSettings();
    void saveUiSettings();
    void updateThemeIcons();
    /// Draws the glyphs of the window buttons of a window without frame,
    /// which SARibbon gives only through the style sheet it is denied.
    void updateWindowButtons();
    void connectDrawingAids();
    void createStatusBar();

    /// The action `id` of builtin_actions.hpp; it must exist.
    [[nodiscard]] QAction* registeredAction(const char* id) const;

    /// A status bar button with the icon of the checkable `action`, no text,
    /// that mirrors the action both ways (docs/adr/0039, point 1, area 10).
    QToolButton* createModeButton(QAction* action, const QString& name);

    /// Hands the runner of the active session the drawing aids chosen in
    /// the Tools menu.
    void applyDrawingAids();

    /// Makes `session` active; nullptr for none (activate()).
    void switchTo(DocumentSession* session);
    /// The settings of the window that every runner gets: file access,
    /// description check, script sequences.
    void configureRunner(cmd::CommandRunner& runner) const;
    /// What the listener of the active session does after a change.
    void documentChanged();
    /// The name of the file of the active session, or "Drawing1.cadx".
    [[nodiscard]] QString documentName() const;

    /// After any input or change: redraws the view and updates the prompt
    /// and the Edit actions.
    void refresh();
    void updateEditActions();
    void updateDrawingState();
    void submitFileText(const QString& text);
    /// Runs `command` as typed, then gives it `responses`
    /// (ActionSpec::responses, docs/tappe/0.7.5/P19).
    void startCommand(const QString& command, const QStringList& responses);
    /// Gives the running command the responses of its action still due:
    /// after an input, the ones that follow the user's answer.
    void giveResponses();
    void handleFilePrompts();
    void ensureRecentFiles();
    void addRecentFile(const QString& path);
    void populateRecentMenu();
    void openRecentFile(const QString& path);
    [[nodiscard]] QString dialogPath();
    [[nodiscard]] QString chooseFile(cmd::FilePrompt prompt, const QString& path);

    // Declared in dependency order: the sessions borrow the registry and the
    // file access, so they are destroyed first. The sample drawing goes
    // straight into the document of the first session, without a
    // transaction, so it cannot be undone.
    cmd::CommandRegistry m_registry;
    std::unique_ptr<io::FileAccess> m_diskFiles;
    // The file access the runners get: m_diskFiles, setFileAccess()'s, or
    // nullptr in a script session.
    io::FileAccess* m_files{nullptr};
    cmd::CommandRunner::DescriptionCheck m_descriptionCheck;
    bool m_scriptSequences{false};
    std::vector<std::unique_ptr<DocumentSession>> m_sessions;
    DocumentSession* m_active{nullptr};
    std::unique_ptr<QSettings> m_settings;
    std::unique_ptr<RecentFiles> m_recentFiles;
    FileDialog m_fileDialog;
    QString m_pendingRecent;
    QString m_pendingOpen;
    // The actions that wait for the running command to end: those with a
    // command, as typing does.
    QList<QAction*> m_idleActions;
    ActionRegistry* m_actions{nullptr};
    std::unique_ptr<UiBuilder> m_uiBuilder;
    QToolButton* m_ribbonButton{nullptr};
    QMenu* m_recentMenu{nullptr};
    QAction* m_recentAction{nullptr};
    Theme m_theme{Theme::Light};
    bool m_uiSettingsReady{false};
    bool m_restoringUi{false};
    bool m_scriptMode{false};
    bool m_silentPolar{false};
    bool m_externalFiles{false};
    bool m_handlingFilePrompt{false};
    CadView* m_view{nullptr};
    CommandLine* m_commandLine{nullptr};
    LayerPanel* m_layerPanel{nullptr};
    PropertiesPanel* m_propertiesPanel{nullptr};
    AutoHide* m_autoHide{nullptr};
    QAction* m_undoAction{nullptr};
    QAction* m_redoAction{nullptr};
    QAction* m_eraseAction{nullptr};
    QAction* m_objectSnapAction{nullptr};
    QAction* m_orthoAction{nullptr};
    QAction* m_dynamicInputAction{nullptr};
    QAction* m_polarAction{nullptr};
    QAction* m_endpointAction{nullptr};
    QAction* m_midpointAction{nullptr};
    QAction* m_centerAction{nullptr};
    QAction* m_insertionAction{nullptr};
    QAction* m_intersectionAction{nullptr};
    QAction* m_perpendicularAction{nullptr};
    QAction* m_tangentAction{nullptr};
    QAction* m_quadrantAction{nullptr};
    QLabel* m_cursorLabel{nullptr};
    QLabel* m_scaleLabel{nullptr};
};

}  // namespace tg::app
