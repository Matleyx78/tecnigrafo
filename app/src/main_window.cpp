// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "main_window.hpp"

#include "action_registry.hpp"
#include "builtin_actions.hpp"
#include "cad_view.hpp"
#include "command_line.hpp"
#include "recent_files.hpp"
#include "sample_drawing.hpp"
#include "tg/cmd/builtin_commands.hpp"
#include "tg/io/file_access.hpp"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QScopedValueRollback>
#include <QSettings>
#include <QStatusBar>
#include <QStringList>
#include <QToolButton>
#include <QVBoxLayout>

#include <cassert>
#include <string>
#include <string_view>
#include <utility>

namespace tg::app {

namespace {

/// Observes successful writes, including SAVE copies that do not change
/// the editor's file name or saved position. Serialization stays in cmd/io.
class DesktopFiles final : public io::FileAccess {
public:
    DesktopFiles(std::function<void(const QString&)> saved,
                 std::function<void(const QString&)> read)
        : m_saved(std::move(saved)), m_read(std::move(read)) {}

    [[nodiscard]] io::Result<std::string> read(std::string_view name) override {
        auto result = m_disk.read(name);
        if (result.ok()) {
            m_read(QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size())));
        }
        return result;
    }

    [[nodiscard]] std::optional<io::Error> write(std::string_view name,
                                                 std::string_view bytes) override {
        const auto error = m_disk.write(name, bytes);
        if (!error) {
            m_saved(QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size())));
        }
        return error;
    }

    [[nodiscard]] bool exists(std::string_view name) const override { return m_disk.exists(name); }

private:
    io::DiskFiles m_disk;
    std::function<void(const QString&)> m_saved;
    std::function<void(const QString&)> m_read;
};

// Decimal places of the coordinate read-out.
constexpr int kCoordinateDecimals = 4;

// The runner's own commands, used by the Edit menu so that Ctrl+Z behaves
// exactly like typing U (message in the history, Enter repeats it).
constexpr const char* kUndoCommand = "UNDO";
constexpr const char* kRedoCommand = "REDO";

}  // namespace

MainWindow::MainWindow(InitialDrawing drawing, QWidget* parent)
    : QMainWindow(parent),
      m_document(drawing == InitialDrawing::Sample ? makeSampleDrawing()
                                                   : core::Document::newDrawing()),
      m_editor(m_document),
      m_runner(m_registry, m_editor),
      m_view(new CadView(m_document, m_runner, this)),
      m_commandLine(new CommandLine(m_runner, this)) {
    cmd::registerBuiltinCommands(m_registry);
    // The program as distributed does not check descriptions; --script turns
    // the check on (docs/adr/0026, point 3, and setScriptMode()).
    m_runner.setDescriptionCheck({});
    m_diskFiles =
        std::make_unique<DesktopFiles>([this](const QString& path) { addRecentFile(path); },
                                       [this](const QString& path) { m_pendingOpen = path; });
    m_runner.setFileAccess(m_diskFiles.get());
    updateDrawingState();
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
    m_editor.setChangeListener([this] {
        // Called inside the runner: never ask another question reentrantly.
        updateDrawingState();
        m_view->update();
        // Only a decoded OPEN replaces the document after reading bytes.
        // A failed parse, undo or a no-op edit must not reorder recent files.
        if (!m_pendingOpen.isEmpty() &&
            m_pendingOpen == QString::fromStdString(m_editor.fileName()) && m_editor.hasFile() &&
            !m_editor.isModified()) {
            addRecentFile(std::exchange(m_pendingOpen, {}));
        }
    });
    connect(m_view, &CadView::inputHandled, this, &MainWindow::refresh);
    connect(m_commandLine, &CommandLine::inputHandled, this, &MainWindow::refresh);
    updateEditActions();
}

QAction* MainWindow::registeredAction(const char* id) const {
    QAction* action = m_actions->action(QString::fromLatin1(id));
    assert(action != nullptr && "action not declared in builtin_actions.cpp");
    return action;
}

void MainWindow::createMenus() {
    // Every action comes from the registry (docs/adr/0009, point 1); the
    // window only connects what the actions do.
    m_actions = new ActionRegistry(this);
    addBuiltinActions(*m_actions);

    // Actions with a command run it as if typed in the command line, which
    // shows it; like typing, they wait for the running command to end.
    for (QAction* action : m_actions->actions()) {
        const QString command = m_actions->spec(action->objectName())->command;
        if (command.isEmpty()) {
            continue;
        }
        m_idleActions.append(action);
        connect(action, &QAction::triggered, this, [this, command] {
            submitFileText(command);
            refresh();
        });
    }

    m_recentMenu = new QMenu(tr("Recent Files"), this);
    m_recentMenu->setObjectName(QStringLiteral("recentMenu"));
    m_recentAction = registeredAction("file.recent");
    m_recentAction->setMenu(m_recentMenu);
    // Delay even reading settings until real UI use: ScriptSession marks
    // the window as scripted after construction, before sending any input.
    connect(m_recentMenu, &QMenu::aboutToShow, this, &MainWindow::populateRecentMenu);
    connect(registeredAction("file.quit"), &QAction::triggered, this, &QWidget::close);

    m_undoAction = registeredAction("edit.undo");
    connect(m_undoAction, &QAction::triggered, this, [this] {
        m_runner.submitText(kUndoCommand);
        refresh();
    });
    m_redoAction = registeredAction("edit.redo");
    connect(m_redoAction, &QAction::triggered, this, [this] {
        m_runner.submitText(kRedoCommand);
        refresh();
    });
    m_eraseAction = registeredAction("edit.erase");

    connect(registeredAction("view.zoom-extents"), &QAction::triggered, m_view,
            &CadView::zoomExtents);

    connectDrawingAids();

    connect(registeredAction("help.about"), &QAction::triggered, this, &MainWindow::showAbout);

    m_actions->buildMenus(menuBar());
}

void MainWindow::connectDrawingAids() {
    // The switches start from the runner's settings, the defaults of
    // docs/adr/0008: object snap on with every mode, ortho off.
    const cmd::DrawingAids& aids = m_runner.drawingAids();
    m_objectSnapAction = registeredAction("aids.osnap");
    m_orthoAction = registeredAction("aids.ortho");
    m_endpointAction = registeredAction("aids.snap.endpoint");
    m_midpointAction = registeredAction("aids.snap.midpoint");
    m_centerAction = registeredAction("aids.snap.center");
    m_insertionAction = registeredAction("aids.snap.insertion");
    m_objectSnapAction->setChecked(aids.objectSnap);
    m_orthoAction->setChecked(aids.ortho);
    m_endpointAction->setChecked(aids.snapModes.endpoint);
    m_midpointAction->setChecked(aids.snapModes.midpoint);
    m_centerAction->setChecked(aids.snapModes.center);
    m_insertionAction->setChecked(aids.snapModes.insertion);

    // Connected after the initial states are set, so that starting the
    // program writes nothing in the history.
    for (QAction* action : {m_objectSnapAction, m_orthoAction, m_endpointAction, m_midpointAction,
                            m_centerAction, m_insertionAction}) {
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
    aids.snapModes.insertion = m_insertionAction->isChecked();
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
    updateDrawingState();
    updateEditActions();
    handleFilePrompts();
    m_pendingOpen.clear();
    updateDrawingState();
    updateEditActions();
}

void MainWindow::updateEditActions() {
    // Undo and redo wait for the running command to end (docs/tappe/0.2).
    const bool idle = !m_runner.isActive();
    for (QAction* action : m_idleActions) {
        action->setEnabled(idle);
    }
    m_recentMenu->setEnabled(idle && !m_scriptMode);
    m_recentAction->setEnabled(idle && !m_scriptMode);
    m_eraseAction->setEnabled(idle && !m_runner.selection().isEmpty());
    const QString undoName = QString::fromStdString(m_editor.undoName());
    const QString redoName = QString::fromStdString(m_editor.redoName());
    m_undoAction->setEnabled(idle && m_editor.canUndo());
    m_redoAction->setEnabled(idle && m_editor.canRedo());
    m_undoAction->setText(undoName.isEmpty() ? tr("&Undo") : tr("&Undo %1").arg(undoName));
    m_redoAction->setText(redoName.isEmpty() ? tr("&Redo") : tr("&Redo %1").arg(redoName));
}

MainWindow::~MainWindow() = default;

void MainWindow::setScriptMode() {
    m_scriptMode = true;
    // At level 2 a prompt missing from its command's description is written
    // in the history: the report differs from any expected one, so the case
    // fails and prints the message among its differences (docs/adr/0026).
    m_runner.setDescriptionCheck([this](const std::string& message) {
        m_commandLine->appendToHistory(QString::fromStdString(message));
    });
    if (!m_externalFiles) {
        m_runner.setFileAccess(nullptr);
    }
    m_recentMenu->setEnabled(false);
    m_recentAction->setEnabled(false);
}

void MainWindow::setFileDialog(FileDialog dialog) {
    m_fileDialog = std::move(dialog);
}

void MainWindow::updateDrawingState() {
    setWindowTitle(tr("%1[*] — Tecnigrafo")
                       .arg(QFileInfo(QString::fromStdString(m_editor.fileName())).fileName()));
    setWindowModified(m_editor.isModified());
}

void MainWindow::submitFileText(const QString& text) {
    m_commandLine->appendToHistory(
        QStringLiteral("%1 %2").arg(QString::fromStdString(m_runner.promptText()), text));
    m_runner.submitText(text.toStdString());
    m_commandLine->refreshPrompt();
}

void MainWindow::ensureRecentFiles() {
    if (!m_settings) {
        m_settings = std::make_unique<QSettings>();
        m_recentFiles = std::make_unique<RecentFiles>(*m_settings);
    }
}

void MainWindow::addRecentFile(const QString& path) {
    if (m_scriptMode) {
        return;
    }
    ensureRecentFiles();
    m_recentFiles->add(QFileInfo(path).absoluteFilePath());
}

void MainWindow::populateRecentMenu() {
    m_recentMenu->clear();
    if (m_scriptMode) {
        return;
    }
    ensureRecentFiles();
    const QStringList files = m_recentFiles->files();
    const QList<QAction*> actions =
        m_actions->dynamicActions(QStringLiteral("file.recent"), static_cast<int>(files.size()));
    for (qsizetype i = 0; i < files.size(); ++i) {
        QAction* action = actions[i];
        const QString& path = files[i];
        action->setText(path);
        // The registry reuses its actions: drop the file of the last time.
        disconnect(action, &QAction::triggered, this, nullptr);
        connect(action, &QAction::triggered, this, [this, path] { openRecentFile(path); });
        m_recentMenu->addAction(action);
    }
}

void MainWindow::openRecentFile(const QString& path) {
    if (m_runner.isActive()) {
        return;
    }
    if (!QFileInfo::exists(path)) {
        m_commandLine->appendToHistory(tr("Cannot open %1: file no longer exists.").arg(path));
        m_recentFiles->remove(path);
        return;
    }
    // Keep the selection through a possible unsaved-changes/save exchange.
    m_pendingRecent = path;
    submitFileText(QStringLiteral("OPEN"));
    refresh();
}

QString MainWindow::dialogPath() {
    if (m_editor.hasFile()) {
        return QFileInfo(QString::fromStdString(m_editor.fileName())).absoluteFilePath();
    }
    ensureRecentFiles();
    const QString folder =
        m_settings->value(QStringLiteral("lastFileDirectory"), QDir::homePath()).toString();
    return QDir(folder).filePath(QString::fromStdString(m_editor.fileName()));
}

QString MainWindow::chooseFile(cmd::FilePrompt prompt, const QString& path) {
    if (m_fileDialog) {
        return m_fileDialog(prompt, path);
    }
    const QString filters = tr("Tecnigrafo drawing (*.cadx);;DXF (*.dxf)");
    if (prompt == cmd::FilePrompt::Open) {
        return QFileDialog::getOpenFileName(this, tr("Open drawing"), path, filters);
    }
    QFileDialog dialog(this, tr("Save drawing"), path, filters);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setDefaultSuffix(QStringLiteral("cadx"));
    connect(&dialog, &QFileDialog::filterSelected, &dialog, [&dialog](const QString& filter) {
        dialog.setDefaultSuffix(filter.contains(QStringLiteral("*.dxf")) ? QStringLiteral("dxf")
                                                                         : QStringLiteral("cadx"));
    });
    return dialog.exec() == QDialog::Accepted ? dialog.selectedFiles().value(0) : QString{};
}

void MainWindow::handleFilePrompts() {
    if (m_scriptMode || m_handlingFilePrompt) {
        return;
    }
    const QScopedValueRollback<bool> guard(m_handlingFilePrompt, true);
    while (m_runner.prompt().file != cmd::FilePrompt::None) {
        const cmd::FilePrompt kind = m_runner.prompt().file;
        const bool recent = kind == cmd::FilePrompt::Open && !m_pendingRecent.isEmpty();
        const QString path =
            recent ? std::exchange(m_pendingRecent, {}) : chooseFile(kind, dialogPath());
        if (path.isEmpty()) {
            m_pendingRecent.clear();
            m_runner.submit(cmd::CancelInput{});
            break;
        }
        const QString absolutePath = QFileInfo(path).absoluteFilePath();
        ensureRecentFiles();
        m_settings->setValue(QStringLiteral("lastFileDirectory"),
                             QFileInfo(absolutePath).absolutePath());
        submitFileText(absolutePath);
        // The picker already confirmed replacement. Only answer this
        // two-keyword question, never a NEW/OPEN unsaved-changes question.
        const cmd::Prompt next = m_runner.prompt();
        if (kind == cmd::FilePrompt::Save &&
            next.keywords == std::vector<cmd::Keyword>{{"Yes", "Y"}, {"No", "N"}}) {
            submitFileText(QStringLiteral("Yes"));
        }
    }
    if (!m_runner.isActive()) {
        m_pendingRecent.clear();
    }
    m_commandLine->refreshPrompt();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_scriptMode) {
        event->accept();
        return;
    }
    // Finish/cancel the current transaction before checking the dirty flag:
    // Esc preserves completed LINE/PLINE work, just like the command line.
    if (m_runner.isActive()) {
        m_runner.submit(cmd::CancelInput{});
        refresh();
    }
    if (!m_editor.isModified()) {
        event->accept();
        return;
    }
    const auto choice = QMessageBox::question(
        this, tr("Tecnigrafo"),
        tr("Save changes to %1?")
            .arg(QFileInfo(QString::fromStdString(m_editor.fileName())).fileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (choice == QMessageBox::Cancel) {
        event->ignore();
        return;
    }
    if (choice == QMessageBox::Save) {
        submitFileText(QStringLiteral("QSAVE"));
        refresh();
        if (m_editor.isModified()) {
            event->ignore();
            return;
        }
    }
    event->accept();
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
