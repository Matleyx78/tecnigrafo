// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "document_session.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/core/snap_points.hpp"
#include "tg/edit/document_file.hpp"
#include "tg/edit/editor.hpp"
#include "theme.hpp"
#include "ui_builder.hpp"

#include <QHash>
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
class DocumentTabs;
class LayerPanel;
class PropertiesPanel;
class RecentFiles;
class StartPage;

/// The window draws its own frame, with the quick access bar in its title
/// row (docs/adr/0039, point 4), except under Wayland, where it keeps the
/// frame of the system and the quick access bar goes under the title.
///
/// The window owns a session for each open document (docs/adr/0041, point
/// 2), shown in a tab of its own, one of them active, or none while the
/// Start page is (docs/adr/0041, point 9; docs/tappe/0.7.5/P29). The program
/// opens on the Start page; NEW and OPEN, in the runner of any session or
/// from the Start page, add a session and activate it (the window is the
/// host of their documents, cmd::DocumentHost). View, command line,
/// the panels Layers and Properties, Undo and Redo, the drawing aids, title
/// and status follow the active session (activate()); the listener of every
/// session updates the interface only while that session is active.
class MainWindow final : public SARibbonMainWindow {
    Q_OBJECT

public:
    /// What the window shows when it opens.
    enum class InitialDrawing : std::uint8_t {
        /// The sample drawing of sample_drawing.hpp, active: the gallery of
        /// screenshots and the tests.
        Sample,
        /// An empty drawing, active: the control mode starts from it
        /// (docs/adr/0011, level 2).
        Empty,
        /// No document: the Start page, as the program opens, like AutoCAD
        /// (docs/DECISIONI.md, 2026-11-07).
        None,
    };

    Q_DISABLE_COPY_MOVE(MainWindow)

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
    /// is lost (closeDocument() asks). View, command line, panels and
    /// listener let it go first; if it was active, the session after it
    /// becomes active, or else the one before it, or none: the Start page
    /// (decision B4 of docs/DECISIONI.md). Precondition: `session` is one
    /// of sessions().
    void closeSession(DocumentSession& session);

    /// What the cross of the tab of `session` does (docs/tappe/0.7.5/P29,
    /// point 4): a command in progress ends first, as with Esc; then, if
    /// the document is modified, the question to save it, with the document
    /// shown. Cancel, or a save cancelled or failed, leaves it open; the
    /// document active before stays active, unless it was `session` and
    /// `session` is closed (closeSession()). True if it is closed.
    /// Precondition: `session` is one of sessions().
    bool closeDocument(DocumentSession& session);

    /// Shows the Start page: no session is active (docs/adr/0041, point 9).
    /// The session left keeps its command and what was typed, as with
    /// activate().
    void showStartPage();

    /// The row of the tabs of the documents.
    [[nodiscard]] DocumentTabs* documentTabs() const { return m_documentTabs; }

    /// The Start page, shown while no session is active.
    [[nodiscard]] StartPage* startPage() const { return m_startPage; }

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
    /// Connects what the tabs and the Start page ask.
    void connectDocumentTabs();

    /// The action `id` of builtin_actions.hpp; it must exist.
    [[nodiscard]] QAction* registeredAction(const char* id) const;

    /// A status bar button with the icon of the checkable `action`, no text,
    /// that mirrors the action both ways (docs/adr/0039, point 1, area 10).
    QToolButton* createModeButton(QAction* action, const QString& name);

    /// Hands the runner of the active session the drawing aids chosen in
    /// the Tools menu.
    void applyDrawingAids();

    /// Makes `session` active; nullptr for none, the Start page (activate()).
    void switchTo(DocumentSession* session);
    /// Before `session` closes: cancels its command, if any, and asks
    /// whether to save it if it is modified, showing it. False if the user
    /// cancels, or saving fails.
    bool confirmClose(DocumentSession& session);
    /// closeDocument(); when the user keeps `session` open, `keepShown`
    /// leaves it active, as closing the window does, instead of the session
    /// active before.
    bool closeAsked(DocumentSession& session, bool keepShown);
    /// Shows the Start page in place of the drawing and the command line,
    /// or the other way round.
    void setStartPageShown(bool shown);
    /// Takes the actions that need a document away while the Start page is
    /// shown, and gives them back as they were (ActionSpec::requiresDocument).
    void setDocumentActionsAvailable(bool available);
    /// The tabs show the sessions, in their order, and the active one.
    void refreshTabs();
    /// The Start page shows the recent files.
    void refreshStartPage();
    /// Runs the action `id` if it is enabled, as its shortcut would.
    void triggerAction(const char* id);
    /// The runner that the actions with a command and the file dialogs use:
    /// the active session's, or with the Start page a runner of the window
    /// whose document is never shown, so that NEW and OPEN work there too.
    [[nodiscard]] cmd::CommandRunner& commandRunner();
    /// What NEW and OPEN do through the host: a session for `document`,
    /// with `file`, after the others, made active. A file read by OPEN goes
    /// to the recent files.
    DocumentSession& addDocument(core::Document document, edit::DocumentFile file);
    /// The settings of the window that every runner gets: file access,
    /// description check, script sequences, the host of the documents.
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

    /// The host of the documents for the runners (cmd::DocumentHost).
    class Host;

    // Declared in dependency order: the sessions borrow the registry, the
    // file access and the host, so they are destroyed first. The sample
    // drawing goes straight into the document of the first session, without
    // a transaction, so it cannot be undone.
    cmd::CommandRegistry m_registry;
    std::unique_ptr<io::FileAccess> m_diskFiles;
    // The file access the runners get: m_diskFiles, setFileAccess()'s, or
    // nullptr in a script session.
    io::FileAccess* m_files{nullptr};
    cmd::CommandRunner::DescriptionCheck m_descriptionCheck;
    bool m_scriptSequences{false};
    std::unique_ptr<Host> m_host;
    // The runner of the Start page (commandRunner()): its document is never
    // shown, nor in sessions(), and never changes.
    std::unique_ptr<cmd::DrawingSession> m_startSession;
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
    // The actions declared with ActionSpec::requiresDocument, and, while the
    // Start page is shown, whether each was enabled before it.
    QList<QAction*> m_documentActions;
    QHash<QAction*, bool> m_documentActionStates;
    bool m_documentActionsAvailable{true};
    bool m_destroying{false};
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
    DocumentTabs* m_documentTabs{nullptr};
    StartPage* m_startPage{nullptr};
    QWidget* m_drawingArea{nullptr};
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
