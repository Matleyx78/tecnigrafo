// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "main_window.hpp"

#include "action_registry.hpp"
#include "app_commands.hpp"
#include "auto_hide.hpp"
#include "builtin_actions.hpp"
#include "cad_view.hpp"
#include "command_line.hpp"
#include "document_tabs.hpp"
#include "dynamic_input_box.hpp"
#include "layer_panel.hpp"
#include "properties_panel.hpp"
#include "recent_files.hpp"
#include "sample_drawing.hpp"
#include "settings_dialog.hpp"
#include "shortcut_map.hpp"
#include "shortcuts_dialog.hpp"
#include "start_page.hpp"
#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/document_host.hpp"
#include "tg/cmd/drawing_session.hpp"
#include "tg/io/file_access.hpp"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QScopedValueRollback>
#include <QSettings>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSize>
#include <QStatusBar>
#include <QStringList>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::app {

namespace {

/// Whether `response` is an option of `prompt`, by its name or its
/// shortcut in any case, as CommandRunner reads a typed option.
bool offers(const cmd::Prompt& prompt, const QString& response) {
    const auto same = [&response](const std::string& text) {
        return !text.empty() &&
               response.compare(QString::fromStdString(text), Qt::CaseInsensitive) == 0;
    };
    return std::ranges::any_of(prompt.keywords, [&same](const cmd::Keyword& keyword) {
        return same(keyword.name) || same(keyword.shortcut);
    });
}

/// Observes successful writes, including SAVE copies that do not change
/// the document file or the editor's saved position. Serialization stays in cmd/io.
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

/// The icons of the status bar: the 16 px grid of docs/adr/0040, point 3.
constexpr int kStatusIconSize = 16;

/// The frame of the window (docs/adr/0039, point 4). SAFramelessHelper
/// moves a window without frame with QWidget::move() and global pointer
/// positions, which Wayland does not give a client: there such a window
/// could not be moved, nor resized from its left and top edges. Under
/// Wayland the window keeps the frame of the system, and SARibbon puts the
/// quick access bar in the row of the tabs, under the title.
SARibbonMainWindowStyles windowStyles() {
    const bool wayland = QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
    return SARibbonMainWindowStyleFlag::UseRibbonMenuBar |
           (wayland ? SARibbonMainWindowStyleFlag::UseNativeFrame
                    : SARibbonMainWindowStyleFlag::UseRibbonFrame);
}

enum class WindowGlyph : std::uint8_t { Minimize, Maximize, Restore, Close };

/// The window buttons are drawn like the buttons of the dock headers
/// (style.cpp): strokes of 1.25 px on the 16 px grid of docs/adr/0040,
/// point 3, in its central 7 px, at these sizes for high-DPI screens.
constexpr qreal kGlyphGrid = 16.0;
constexpr qreal kGlyphStroke = 1.25;
constexpr std::array kGlyphSizes{16, 20, 24, 32};

QPixmap windowGlyph(WindowGlyph glyph, int size, const QColor& colour) {
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(size / kGlyphGrid, size / kGlyphGrid);
    painter.setPen(QPen(colour, kGlyphStroke, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    painter.setBrush(Qt::NoBrush);
    switch (glyph) {
        case WindowGlyph::Minimize:
            painter.drawLine(QPointF(4.5, 8.5), QPointF(11.5, 8.5));
            break;
        case WindowGlyph::Maximize:
            painter.drawRect(QRectF(4.5, 4.5, 7.0, 7.0));
            break;
        case WindowGlyph::Restore: {
            // The window in front, and the corner of the one behind it.
            painter.drawRect(QRectF(4.5, 6.5, 5.0, 5.0));
            const std::array behind{QPointF(6.5, 6.5), QPointF(6.5, 4.5), QPointF(11.5, 4.5),
                                    QPointF(11.5, 9.5), QPointF(9.5, 9.5)};
            painter.drawPolyline(behind.data(), static_cast<int>(behind.size()));
            break;
        }
        case WindowGlyph::Close:
            painter.drawLine(QPointF(4.5, 4.5), QPointF(11.5, 11.5));
            painter.drawLine(QPointF(11.5, 4.5), QPointF(4.5, 11.5));
            break;
    }
    painter.end();
    return pixmap;
}

/// `glyph`, and `checked` for a checked button (the maximize button of a
/// maximized window).
QIcon windowButtonIcon(WindowGlyph glyph, WindowGlyph checked, const QColor& colour) {
    QIcon icon;
    for (const int size : kGlyphSizes) {
        icon.addPixmap(windowGlyph(glyph, size, colour), QIcon::Normal, QIcon::Off);
        icon.addPixmap(windowGlyph(checked, size, colour), QIcon::Normal, QIcon::On);
    }
    return icon;
}

/// The icon of the tab of a document of `type` (docs/ICONE.md).
QString documentIcon(const QString& type) {
    return type == QString::fromLatin1(DocumentSession::kDrawingType)
               ? QStringLiteral("doc-drawing")
               : QString{};
}

/// The tooltip of a switch of the status bar: its name and its key.
QString switchToolTip(const QAction& action) {
    const QString name = action.text().remove(QLatin1Char('&'));
    const QString key = action.shortcut().toString(QKeySequence::NativeText);
    return key.isEmpty() ? name : QStringLiteral("%1 (%2)").arg(name, key);
}

}  // namespace

/// The window as the host of the documents of NEW and OPEN: every session
/// it owns, in the order they were opened.
class MainWindow::Host final : public cmd::DocumentHost {
public:
    explicit Host(MainWindow& window) : m_window(window) {}

    cmd::DrawingSession& addDocument(core::Document document, edit::DocumentFile file) override {
        return m_window.addDocument(std::move(document), std::move(file)).drawing();
    }

    [[nodiscard]] cmd::DrawingSession* findOpen(std::string_view name) override {
        // A document without a file has none to be open, whatever its name.
        for (const auto& session : m_window.m_sessions) {
            if (session->file().hasFile() && session->file().name() == name) {
                return &session->drawing();
            }
        }
        return nullptr;
    }

    void activate(cmd::DrawingSession& session) override {
        const auto found = std::ranges::find_if(m_window.m_sessions, [&session](const auto& owned) {
            return &owned->drawing() == &session;
        });
        assert(found != m_window.m_sessions.end() && "the session belongs to another window");
        if (found != m_window.m_sessions.end()) {
            m_window.activate(**found);
        }
    }

    [[nodiscard]] std::vector<std::string> openNames() const override {
        std::vector<std::string> names;
        for (const auto& session : m_window.m_sessions) {
            if (!session->file().hasFile()) {
                names.push_back(session->file().name());
            }
        }
        return names;
    }

private:
    MainWindow& m_window;
};

MainWindow::MainWindow(InitialDrawing drawing, QWidget* parent)
    : SARibbonMainWindow(parent, windowStyles()),
      m_diskFiles(
          std::make_unique<DesktopFiles>([this](const QString& path) { addRecentFile(path); },
                                         [this](const QString& path) { m_pendingOpen = path; })),
      m_files(m_diskFiles.get()),
      m_host(std::make_unique<Host>(*this)),
      m_view(new CadView(this)),
      m_commandLine(new CommandLine(this)) {
    // SARibbonMainWindow gives the window the style sheet of its own theme,
    // and queues a second time for when the event loop runs. The colours of
    // the interface are the tokens of theme.cpp, in the application's style
    // sheet (docs/adr/0039, point 2), which a window's own style sheet would
    // override. Taking such a style sheet away from widgets already shown
    // leaves on each of them the palette it had at that moment, which no
    // later theme changes (dark panels with a light status bar): the queued
    // time is dropped before it runs (QTimer::singleShot(0) posts a call to
    // the window), and the first one, set before any widget was shown, goes
    // now.
    QCoreApplication::removePostedEvents(this, QEvent::MetaCall);
    setStyleSheet(QString{});
    cmd::registerBuiltinCommands(m_registry);
    // The program as distributed does not check descriptions; --script turns
    // the check on (docs/adr/0026, point 3, and setScriptMode()): the empty
    // function of m_descriptionCheck.
    // The runner of the Start page: no command line shows it, so its
    // messages ("Cannot open ...") go to the history and to the page.
    m_startSession = std::make_unique<cmd::DrawingSession>(m_registry, m_files);
    configureRunner(m_startSession->runner());
    m_startSession->runner().setMessageSink(
        [this](const std::string& text) { showStartMessage(QString::fromStdString(text)); });
    // The session the window opens with, if any; activated once the
    // interface that follows it exists.
    DocumentSession* first = nullptr;
    if (drawing != InitialDrawing::None) {
        first = &openSession(drawing == InitialDrawing::Sample ? makeSampleDrawing()
                                                               : core::Document::newDrawing());
    }
    updateDrawingState();
    resize(1100, 720);

    // The tabs of the documents under the ribbon (docs/adr/0039, point 1,
    // area 4), with both interfaces; the command line sits below the
    // drawing, as in AutoCAD. The Start page takes the place of both while
    // no document is active.
    auto* central = new QWidget(this);
    m_drawingArea = central;
    m_documentTabs = new DocumentTabs(central);
    m_startPage = new StartPage(central);
    m_startPage->addDocumentType(QString::fromLatin1(DocumentSession::kDrawingType), tr("Drawing"),
                                 QStringLiteral("doc-drawing"));
    m_startPage->hide();
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_documentTabs);
    layout->addWidget(m_startPage, 1);
    layout->addWidget(m_view, 1);
    layout->addWidget(m_commandLine);
    // The tabs of the hidden panels stand around the drawing and the
    // command line (docs/tappe/0.7.5/P18).
    m_autoHide = new AutoHide(*this, central, m_view);
    setCentralWidget(m_autoHide->frame());
    connect(m_autoHide, &AutoHide::changed, this, &MainWindow::saveUiSettings);
    m_commandLine->forwardKeysFrom(m_view);
    m_view->dynamicInputBox()->routeKeysFrom(
        m_view, m_commandLine->findChild<QLineEdit*>(QStringLiteral("input")));
    connect(m_view->dynamicInputBox(), &DynamicInputBox::textSubmitted, this,
            [this](const QString& text) {
                if (m_active == nullptr) {
                    return;
                }
                cmd::CommandRunner& runner = m_active->runner();
                const QString prompt = QString::fromStdString(runner.promptText());
                m_commandLine->appendToHistory(
                    text.isEmpty() ? prompt : QStringLiteral("%1 %2").arg(prompt, text));
                runner.submitText(text.toStdString());
                refresh();
            });

    createMenus();
    createStatusBar();

    connect(m_view, &CadView::cursorMoved, this, &MainWindow::showCursorPosition);
    connect(m_view, &CadView::scaleChanged, this, &MainWindow::showScale);

    connect(m_view, &CadView::inputHandled, this, &MainWindow::refresh);
    connect(m_commandLine, &CommandLine::inputHandled, this, [this] {
        refresh();
        // Return the keyboard to the canvas after a completed command-line
        // exchange; explicitly typing in the command line still works as usual.
        if (!m_scriptMode && m_dynamicInputAction->isChecked() && m_active != nullptr &&
            m_active->runner().isActive()) {
            m_view->setFocus(Qt::OtherFocusReason);
        }
    });
    connectDocumentTabs();
    if (first != nullptr) {
        activate(*first);
    } else {
        setDocumentActionsAvailable(false);
        setStartPageShown(true);
        updateEditActions();
        refreshTabs();
    }
    setDockOptions(dockOptions() | QMainWindow::AllowTabbedDocks | QMainWindow::AllowNestedDocks);
    setTheme(Theme::Light);
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
    // The actions that need a document, disabled with the Start page
    // (ActionSpec::requiresDocument). The run-time actions of the recent
    // files are not among them: their menu, file.recent, works there.
    for (QAction* action : m_actions->actions()) {
        if (!action->isSeparator() && m_actions->spec(action->objectName())->requiresDocument) {
            m_documentActions.append(action);
        }
    }

    // Actions with a command run it as if typed in the command line, which
    // shows it, then give it their responses; like typing, they wait for the
    // running command to end. An action with variants runs the one used
    // last (docs/tappe/0.7.5/P19).
    for (QAction* action : m_actions->actions()) {
        const ActionSpec* spec = m_actions->spec(action->objectName());
        if (spec->command.isEmpty()) {
            continue;
        }
        m_idleActions.append(action);
        if (!m_actions->variants(spec->id).isEmpty()) {
            connect(action, &QAction::triggered, this,
                    [this, id = spec->id] { m_actions->currentVariant(id)->trigger(); });
            continue;
        }
        connect(action, &QAction::triggered, this,
                [this, command = spec->command, responses = spec->responses] {
                    startCommand(command, responses);
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
        if (m_active != nullptr) {
            m_active->runner().submitText(kUndoCommand);
            refresh();
        }
    });
    m_redoAction = registeredAction("edit.redo");
    connect(m_redoAction, &QAction::triggered, this, [this] {
        if (m_active != nullptr) {
            m_active->runner().submitText(kRedoCommand);
            refresh();
        }
    });
    m_eraseAction = registeredAction("edit.erase");

    connect(registeredAction("view.zoom-extents"), &QAction::triggered, m_view,
            &CadView::zoomExtents);

    connect(registeredAction("tools.shortcuts"), &QAction::triggered, this, [this] {
        ensureRecentFiles();
        ShortcutsDialog dialog(*m_actions, *m_settings, this);
        dialog.exec();
    });
    connectDrawingAids();
    connect(registeredAction("tools.settings"), &QAction::triggered, this,
            &MainWindow::showSettings);
    connect(registeredAction("ui.theme"), &QAction::toggled, this,
            [this](bool dark) { setTheme(dark ? Theme::Dark : Theme::Light); });
    connect(registeredAction("ui.interface"), &QAction::toggled, this, [this](bool classic) {
        setInterface(classic ? InterfaceKind::Classic : InterfaceKind::Ribbon);
    });
    connect(registeredAction("aids.lwdisplay"), &QAction::toggled, this, [this](bool on) {
        if (m_active != nullptr) {
            m_active->editor().tables().setLwdisplay(on);
        }
        m_view->update();
    });

    connect(registeredAction("help.about"), &QAction::triggered, this, &MainWindow::showAbout);

    // The layers panel (0.6-P22). It exists before the interface is built:
    // the ribbon and the toolbar find it to place their layer controls.
    auto* layers = new LayerPanel(this);
    m_layerPanel = layers;
    layers->setActions(registeredAction("layer.new"), registeredAction("layer.delete"),
                       registeredAction("layer.current"));
    connect(registeredAction("layer.panel"), &QAction::triggered, layers,
            &LayerPanel::showAndFocus);
    connect(layers, &LayerPanel::messageShown, m_commandLine, &CommandLine::appendToHistory);
    connect(layers, &LayerPanel::changed, this, &MainWindow::refresh);
    addPanel(layers, Qt::RightDockWidgetArea);

    // The properties panel and QSELECT (0.6-P24). Like the layers panel, it
    // exists before the interface, which places its controls.
    auto* properties = new PropertiesPanel(this);
    m_propertiesPanel = properties;
    properties->setQuickSelectAction(registeredAction("edit.quick-select"));
    connect(registeredAction("properties.panel"), &QAction::triggered, properties,
            &PropertiesPanel::showAndFocus);
    connect(properties, &PropertiesPanel::messageShown, m_commandLine,
            &CommandLine::appendToHistory);
    connect(properties, &PropertiesPanel::changed, this, &MainWindow::refresh);
    addPanel(properties, Qt::RightDockWidgetArea);

    m_uiBuilder = std::make_unique<UiBuilder>(*this, *m_actions);
    registerAppCommands(m_registry, *this);
    setInterface(InterfaceKind::Ribbon);
}

void MainWindow::connectDrawingAids() {
    // The switches start from the settings of a new runner, the defaults of
    // docs/adr/0008: object snap on with every mode, ortho off.
    const cmd::DrawingAids aids{};
    m_objectSnapAction = registeredAction("aids.osnap");
    m_orthoAction = registeredAction("aids.ortho");
    m_dynamicInputAction = registeredAction("aids.dynmode");
    m_dynamicInputAction->setChecked(true);
    connect(m_dynamicInputAction, &QAction::toggled, this, [this](bool on) {
        m_view->dynamicInputBox()->setInputEnabled(on && !m_scriptMode);
        if (!m_scriptMode && !m_restoringUi) {
            const QString message = on ? tr("<Dynamic input on>") : tr("<Dynamic input off>");
            m_commandLine->appendToHistory(message);
        }
        saveUiSettings();
    });
    m_polarAction = registeredAction("aids.polar");
    m_endpointAction = registeredAction("aids.snap.endpoint");
    m_midpointAction = registeredAction("aids.snap.midpoint");
    m_centerAction = registeredAction("aids.snap.center");
    m_insertionAction = registeredAction("aids.snap.insertion");
    m_intersectionAction = registeredAction("aids.snap.intersection");
    m_perpendicularAction = registeredAction("aids.snap.perpendicular");
    m_tangentAction = registeredAction("aids.snap.tangent");
    m_quadrantAction = registeredAction("aids.snap.quadrant");
    m_objectSnapAction->setChecked(aids.objectSnap);
    m_orthoAction->setChecked(aids.ortho);
    m_polarAction->setChecked(aids.polar);
    m_endpointAction->setChecked(aids.snapModes.endpoint);
    m_midpointAction->setChecked(aids.snapModes.midpoint);
    m_centerAction->setChecked(aids.snapModes.center);
    m_insertionAction->setChecked(aids.snapModes.insertion);
    m_intersectionAction->setChecked(aids.snapModes.intersection);
    m_perpendicularAction->setChecked(aids.snapModes.perpendicular);
    m_tangentAction->setChecked(aids.snapModes.tangent);
    m_quadrantAction->setChecked(aids.snapModes.quadrant);

    // Connected after the initial states are set, so that starting the
    // program writes nothing in the history.
    for (QAction* action : m_actions->actions()) {
        const bool snapMode = action->objectName().startsWith(QStringLiteral("aids.snap."));
        if (snapMode || action == m_objectSnapAction) {
            connect(action, &QAction::toggled, this, &MainWindow::applyDrawingAids);
        }
    }
    // The switches are echoed in the history, as AutoCAD does.
    connect(m_objectSnapAction, &QAction::toggled, this, [this](bool on) {
        m_commandLine->appendToHistory(on ? tr("<Osnap on>") : tr("<Osnap off>"));
    });
    connect(m_orthoAction, &QAction::toggled, this, [this](bool on) {
        if (on) {
            m_polarAction->setChecked(false);
        }
        applyDrawingAids();
        m_commandLine->appendToHistory(on ? tr("<Ortho on>") : tr("<Ortho off>"));
    });
    connect(m_polarAction, &QAction::toggled, this, [this](bool on) {
        if (on) {
            m_orthoAction->setChecked(false);
        }
        applyDrawingAids();
        if (!m_silentPolar) {
            m_commandLine->appendToHistory(on ? tr("<Polar on>") : tr("<Polar off>"));
        }
    });
}

void MainWindow::connectDocumentTabs() {
    connect(m_documentTabs, &DocumentTabs::startRequested, this, &MainWindow::showStartPage);
    connect(m_documentTabs, &DocumentTabs::documentRequested, this, [this](int index) {
        if (index >= 0 && static_cast<std::size_t>(index) < m_sessions.size()) {
            activate(*m_sessions[static_cast<std::size_t>(index)]);
        }
    });
    // Closed after the click: the cross belongs to the tab that the close
    // takes away, and the question to save runs an event loop of its own.
    connect(m_documentTabs, &DocumentTabs::closeRequested, this, [this](int index) {
        if (index < 0 || static_cast<std::size_t>(index) >= m_sessions.size()) {
            return;
        }
        DocumentSession* const session = m_sessions[static_cast<std::size_t>(index)].get();
        QMetaObject::invokeMethod(
            this,
            [this, session] {
                const auto open = std::ranges::any_of(
                    m_sessions, [session](const auto& owned) { return owned.get() == session; });
                if (open) {
                    closeDocument(*session);
                }
            },
            Qt::QueuedConnection);
    });
    connect(m_documentTabs, &DocumentTabs::newRequested, this,
            [this] { triggerAction("file.new"); });
    connect(m_startPage, &StartPage::newRequested, this,
            [this](const QString& /*type*/) { triggerAction("file.new"); });
    connect(m_startPage, &StartPage::openRequested, this, [this] { triggerAction("file.open"); });
    connect(m_startPage, &StartPage::recentRequested, this, &MainWindow::openRecentFile);
}

void MainWindow::triggerAction(const char* id) {
    // QAction::trigger() runs a disabled action too.
    QAction* action = registeredAction(id);
    if (action->isEnabled()) {
        action->trigger();
    }
}

void MainWindow::setSnapModes(const core::SnapModes& modes) {
    m_endpointAction->setChecked(modes.endpoint);
    m_midpointAction->setChecked(modes.midpoint);
    m_centerAction->setChecked(modes.center);
    m_insertionAction->setChecked(modes.insertion);
    m_intersectionAction->setChecked(modes.intersection);
    m_perpendicularAction->setChecked(modes.perpendicular);
    m_tangentAction->setChecked(modes.tangent);
    m_quadrantAction->setChecked(modes.quadrant);
    applyDrawingAids();
}

void MainWindow::setPolar(bool on) {
    const QScopedValueRollback<bool> silence(m_silentPolar, true);
    m_polarAction->setChecked(on);
    if (on) {
        m_orthoAction->setChecked(false);
    }
    applyDrawingAids();
}

void MainWindow::applyDrawingAids() {
    cmd::DrawingAids aids;
    aids.objectSnap = m_objectSnapAction->isChecked();
    aids.snapModes.endpoint = m_endpointAction->isChecked();
    aids.snapModes.midpoint = m_midpointAction->isChecked();
    aids.snapModes.center = m_centerAction->isChecked();
    aids.snapModes.insertion = m_insertionAction->isChecked();
    aids.ortho = m_orthoAction->isChecked();
    aids.snapModes.intersection = m_intersectionAction->isChecked();
    aids.snapModes.perpendicular = m_perpendicularAction->isChecked();
    aids.snapModes.tangent = m_tangentAction->isChecked();
    aids.snapModes.quadrant = m_quadrantAction->isChecked();
    aids.polar = m_polarAction->isChecked();
    if (m_active != nullptr) {
        m_active->runner().setDrawingAids(aids);
    }
    m_view->dynamicInputBox()->refresh(false);
    m_view->update();  // the snap marker and the preview may have changed
}

QToolButton* MainWindow::createModeButton(QAction* action, const QString& name) {
    auto* button = new QToolButton(this);
    button->setObjectName(name);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setIconSize(QSize(kStatusIconSize, kStatusIconSize));
    button->setIcon(action->icon());
    button->setToolTip(switchToolTip(*action));
    button->setCheckable(true);
    button->setChecked(action->isChecked());
    button->setEnabled(action->isEnabled());
    button->setAutoRaise(true);
    // A click must not take the keyboard away from the drawing and the
    // command line.
    button->setFocusPolicy(Qt::NoFocus);
    // Both ways; setChecked() with the current state emits nothing, so the
    // two signals do not call each other forever.
    connect(button, &QToolButton::toggled, action, &QAction::setChecked);
    connect(action, &QAction::toggled, button, &QToolButton::setChecked);
    // The theme changes the icon, the shortcuts window the key, the Start
    // page whether it works.
    connect(action, &QAction::changed, button, [button, action] {
        button->setIcon(action->icon());
        button->setToolTip(switchToolTip(*action));
        button->setEnabled(action->isEnabled());
    });
    return button;
}

void MainWindow::setInterface(InterfaceKind kind) {
    QWidget* focus = focusWidget();
    QWidget* editingFocus = m_view;
    if (focus != nullptr && (focus == m_view || m_commandLine->isAncestorOf(focus))) {
        editingFocus = focus;
    }
    m_uiBuilder->show(kind);
    const QSignalBlocker blocker(registeredAction("ui.interface"));
    registeredAction("ui.interface")->setChecked(kind == InterfaceKind::Classic);
    // SARibbon requests focus during construction. Keep keyboard input in
    // the editor, including the first command typed after the window opens.
    editingFocus->setFocus(Qt::OtherFocusReason);
    if (m_ribbonButton != nullptr) {
        m_ribbonButton->setChecked(kind == InterfaceKind::Ribbon);
    }
    saveUiSettings();
}

InterfaceKind MainWindow::interfaceKind() const {
    return m_uiBuilder->current();
}

void MainWindow::createStatusBar() {
    // docs/adr/0039, point 1, area 10: the coordinates on the left; from the
    // right the settings, the scale, the ribbon, the switches in the order
    // of the reference (lineweights, object snap, polar, ortho), then the
    // place of the environment indicator (point 5).
    m_cursorLabel = new QLabel(this);
    m_cursorLabel->setMinimumWidth(260);
    statusBar()->addWidget(m_cursorLabel);
    // Empty in the drawing: the indicator comes with the environments
    // (docs/adr/0041).
    auto* environment = new QWidget(this);
    environment->setObjectName(QStringLiteral("environmentIndicator"));
    statusBar()->addPermanentWidget(environment);
    statusBar()->addPermanentWidget(createModeButton(m_orthoAction, QStringLiteral("orthoButton")));
    statusBar()->addPermanentWidget(createModeButton(m_polarAction, QStringLiteral("polarButton")));
    statusBar()->addPermanentWidget(
        createModeButton(m_objectSnapAction, QStringLiteral("osnapButton")));
    statusBar()->addPermanentWidget(
        createModeButton(registeredAction("aids.lwdisplay"), QStringLiteral("lwdisplayButton")));

    statusBar()->addPermanentWidget(
        createModeButton(m_dynamicInputAction, QStringLiteral("dynmodeButton")));

    // On while the ribbon is shown, which is when ui.interface is off: its
    // own switch, not a mirror of the action.
    QAction* interfaceSwitch = registeredAction("ui.interface");
    m_ribbonButton = new QToolButton(this);
    m_ribbonButton->setObjectName(QStringLiteral("ribbonButton"));
    m_ribbonButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_ribbonButton->setIconSize(QSize(kStatusIconSize, kStatusIconSize));
    m_ribbonButton->setIcon(interfaceSwitch->icon());
    m_ribbonButton->setToolTip(tr("Ribbon"));
    m_ribbonButton->setCheckable(true);
    m_ribbonButton->setChecked(interfaceKind() == InterfaceKind::Ribbon);
    m_ribbonButton->setAutoRaise(true);
    m_ribbonButton->setFocusPolicy(Qt::NoFocus);
    connect(m_ribbonButton, &QToolButton::clicked, this, [this] {
        const bool ribbon = interfaceKind() == InterfaceKind::Ribbon;
        setInterface(ribbon ? InterfaceKind::Classic : InterfaceKind::Ribbon);
    });
    connect(interfaceSwitch, &QAction::changed, m_ribbonButton,
            [this, interfaceSwitch] { m_ribbonButton->setIcon(interfaceSwitch->icon()); });
    statusBar()->addPermanentWidget(m_ribbonButton);

    m_scaleLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_scaleLabel);
    auto* settings = new QToolButton(this);
    settings->setObjectName(QStringLiteral("settingsButton"));
    settings->setDefaultAction(registeredAction("tools.settings"));
    settings->setToolButtonStyle(Qt::ToolButtonIconOnly);
    settings->setIconSize(QSize(kStatusIconSize, kStatusIconSize));
    settings->setAutoRaise(true);
    settings->setFocusPolicy(Qt::NoFocus);
    statusBar()->addPermanentWidget(settings);
    showCursorPosition(0.0, 0.0);
    showScale(m_view->viewport().scale());
}

void MainWindow::refresh() {
    // An input may be the user's answer that a variant waits for.
    giveResponses();
    // A click or Esc may have changed the selection the panel shows.
    if (m_propertiesPanel != nullptr) {
        m_propertiesPanel->refresh();
    }
    m_view->update();
    m_commandLine->refreshPrompt();
    m_view->dynamicInputBox()->refresh();
    updateDrawingState();
    updateEditActions();
    handleFilePrompts();
    m_pendingOpen.clear();
    updateDrawingState();
    updateEditActions();
}

void MainWindow::updateEditActions() {
    // Undo and redo wait for the running command to end (docs/tappe/0.2).
    // With the Start page, NEW and OPEN wait for the runner of the window.
    const bool idle = !commandRunner().isActive();
    const bool editing = idle && m_active != nullptr;
    for (QAction* action : m_idleActions) {
        action->setEnabled(idle);
    }
    m_recentMenu->setEnabled(idle && !m_scriptMode);
    m_recentAction->setEnabled(idle && !m_scriptMode);
    m_eraseAction->setEnabled(editing && !m_active->runner().selection().isEmpty());
    if (m_layerPanel != nullptr) {
        m_layerPanel->setEditingEnabled(editing);
    }
    if (m_propertiesPanel != nullptr) {
        m_propertiesPanel->setEditingEnabled(editing);
    }
    QString undoName;
    QString redoName;
    if (m_active != nullptr) {
        undoName = QString::fromStdString(m_active->editor().undoName());
        redoName = QString::fromStdString(m_active->editor().redoName());
    }
    m_undoAction->setEnabled(editing && m_active->editor().canUndo());
    m_redoAction->setEnabled(editing && m_active->editor().canRedo());
    m_undoAction->setText(undoName.isEmpty() ? tr("&Undo") : tr("&Undo %1").arg(undoName));
    m_redoAction->setText(redoName.isEmpty() ? tr("&Redo") : tr("&Redo %1").arg(redoName));
    // With the Start page the actions that need a document stay disabled,
    // whatever the lines above gave them.
    if (m_active == nullptr) {
        for (QAction* action : m_documentActions) {
            action->setEnabled(false);
        }
    }
}

cmd::CommandRunner& MainWindow::commandRunner() {
    return m_active != nullptr ? m_active->runner() : m_startSession->runner();
}

MainWindow::~MainWindow() {
    // Every widget and callback lets the sessions go before they are
    // destroyed with the members: the widgets outlive them, as children.
    // Nothing more is shown: no Start page, no tabs.
    m_destroying = true;
    switchTo(nullptr);
    for (const auto& session : m_sessions) {
        session->editor().setChangeListener({});
    }
}

const core::Document& MainWindow::document() const {
    assert(m_active != nullptr && "no active session");
    return m_active->document();
}

const edit::Editor& MainWindow::editor() const {
    assert(m_active != nullptr && "no active session");
    return m_active->editor();
}

const cmd::CommandRunner& MainWindow::runner() const {
    assert(m_active != nullptr && "no active session");
    return m_active->runner();
}

void MainWindow::setScriptSequences(bool on) {
    m_scriptSequences = on;
    m_startSession->runner().setScriptSequences(on);
    for (const auto& session : m_sessions) {
        session->runner().setScriptSequences(on);
    }
}

void MainWindow::setFileAccess(io::FileAccess* files) {
    m_externalFiles = true;
    m_files = files;
    m_startSession->runner().setFileAccess(files);
    for (const auto& session : m_sessions) {
        session->runner().setFileAccess(files);
    }
}

void MainWindow::configureRunner(cmd::CommandRunner& runner) const {
    runner.setFileAccess(m_files);
    runner.setDescriptionCheck(m_descriptionCheck);
    runner.setScriptSequences(m_scriptSequences);
    runner.setDocumentHost(m_host.get());
}

DocumentSession& MainWindow::openSession(core::Document document) {
    // A unique_ptr each: the vector may grow, the sessions stay where they are.
    auto& session = *m_sessions.emplace_back(
        std::make_unique<DocumentSession>(m_registry, m_files, std::move(document)));
    configureRunner(session.runner());
    refreshTabs();
    // The editor's listener runs inside commit(), while the command that
    // commits is still running: the inputs' own signals refresh again once
    // the runner is idle, so that Undo is enabled when the command ends.
    // A session that is not shown receives no input, but a change to it
    // must never reach the interface of another one.
    session.editor().setChangeListener([this, changed = &session] {
        if (changed == m_active) {
            documentChanged();
        }
    });
    return session;
}

void MainWindow::documentChanged() {
    // Called inside the runner: never ask another question reentrantly.
    updateDrawingState();
    m_view->update();
    // The panel shows what a command, undo or redo changed.
    if (m_layerPanel != nullptr) {
        m_layerPanel->refresh();
    }
    if (m_propertiesPanel != nullptr) {
        m_propertiesPanel->refresh();
    }
}

DocumentSession& MainWindow::addDocument(core::Document document, edit::DocumentFile file) {
    DocumentSession* left = m_active;
    DocumentSession& session = openSession(std::move(document));
    session.file() = std::move(file);
    if (left != nullptr) {
        session.runner().setPixelSize(left->runner().pixelSize());
        // A script sees the view it set with !view, as at level 1, where a
        // new document keeps the size of a pixel (docs/verifica/script.md):
        // the view of a new session is the one left, not fitted to it.
        if (m_scriptMode) {
            session.setViewState({m_view->viewport().center(), m_view->viewport().scale()});
        }
    }
    // Only a decoded OPEN adds a session with a file after reading bytes: a
    // failed read or parse adds none, and never reaches the recent files.
    // OPEN of a file already open reads nothing, and leaves them as they are.
    const edit::DocumentFile& added = session.file();
    if (!m_pendingOpen.isEmpty() && added.hasFile() &&
        m_pendingOpen == QString::fromStdString(added.name())) {
        addRecentFile(std::exchange(m_pendingOpen, {}));
    }
    activate(session);
    return session;
}

void MainWindow::activate(DocumentSession& session) {
    assert(std::ranges::any_of(m_sessions,
                               [&session](const auto& owned) { return owned.get() == &session; }) &&
           "the session belongs to another window");
    switchTo(&session);
}

void MainWindow::switchTo(DocumentSession* session) {
    if (session == m_active) {
        return;
    }
    if (session == nullptr) {
        setDocumentActionsAvailable(false);
    } else if (m_active == nullptr) {
        // The view is shown and laid out before it fits a new session.
        setStartPageShown(false);
        setDocumentActionsAvailable(true);
    }
    // The widgets save in the session they leave what is of that session
    // (the view, the text typed and not sent) and show what the new one
    // kept. The listener of a session acts only while it is m_active.
    m_active = session;
    m_view->setSession(session);
    m_commandLine->setSession(session);
    if (m_layerPanel != nullptr) {
        m_layerPanel->setSession(session);
    }
    if (m_propertiesPanel != nullptr) {
        m_propertiesPanel->setSession(session);
    }
    if (session == nullptr) {
        setStartPageShown(true);
    }
    if (m_actions != nullptr) {
        // The switches are the window's: the runner shown now gets them.
        applyDrawingAids();
    }
    updateDrawingState();
    if (m_actions != nullptr) {
        updateEditActions();
    }
}

void MainWindow::closeSession(DocumentSession& session) {
    const auto found = std::ranges::find_if(
        m_sessions, [&session](const auto& owned) { return owned.get() == &session; });
    assert(found != m_sessions.end() && "the session belongs to another window");
    if (m_active == &session) {
        // The tab on its right, or else on its left, or the Start page
        // (decision B4).
        DocumentSession* next = nullptr;
        if (std::next(found) != m_sessions.end()) {
            next = std::next(found)->get();
        } else if (found != m_sessions.begin()) {
            next = std::prev(found)->get();
        }
        switchTo(next);
    }
    session.editor().setChangeListener({});
    m_sessions.erase(found);
    refreshTabs();
}

bool MainWindow::closeDocument(DocumentSession& session) {
    return closeAsked(session, false);
}

bool MainWindow::closeAsked(DocumentSession& session, bool keepShown) {
    DocumentSession* const before = m_active;
    // It shows `session` if it asks anything.
    const bool closing = confirmClose(session);
    if (!closing && keepShown) {
        return false;
    }
    if (before != &session) {
        // Closing a tab that is not active does not change the active one.
        switchTo(before);
    }
    if (!closing) {
        refreshTabs();
        return false;
    }
    closeSession(session);
    return true;
}

void MainWindow::showStartPage() {
    switchTo(nullptr);
}

void MainWindow::setStartPageShown(bool shown) {
    if (m_startPage == nullptr || m_destroying) {
        return;
    }
    if (!shown) {
        m_startPage->clearMessages();
    }
    m_startPage->setVisible(shown);
    m_view->setVisible(!shown);
    m_commandLine->setVisible(!shown);
    // Laid out now: a session shown for the first time fits the view at
    // its real size (CadView::setSession).
    m_drawingArea->layout()->activate();
    if (shown) {
        refreshStartPage();
    } else if (!m_scriptMode && isActiveWindow()) {
        m_view->setFocus(Qt::OtherFocusReason);
    }
}

void MainWindow::setDocumentActionsAvailable(bool available) {
    if (m_actions == nullptr || m_destroying || available == m_documentActionsAvailable) {
        return;
    }
    m_documentActionsAvailable = available;
    if (!available) {
        m_documentActionStates.clear();
        for (QAction* action : m_documentActions) {
            m_documentActionStates.insert(action, action->isEnabled());
            action->setEnabled(false);
        }
        return;
    }
    // Back as they were; updateEditActions() and the panels then give the
    // ones that depend on the session their state.
    for (auto state = m_documentActionStates.cbegin(); state != m_documentActionStates.cend();
         ++state) {
        state.key()->setEnabled(state.value());
    }
    m_documentActionStates.clear();
}

void MainWindow::refreshTabs() {
    if (m_documentTabs == nullptr || m_destroying) {
        return;
    }
    QList<DocumentTabs::Document> documents;
    documents.reserve(static_cast<qsizetype>(m_sessions.size()));
    int active = -1;
    for (const auto& session : m_sessions) {
        const QString name = QString::fromStdString(session->file().name());
        const QFileInfo info(name);
        if (session.get() == m_active) {
            active = static_cast<int>(documents.size());
        }
        documents.append({info.fileName(),
                          session->file().hasFile() ? info.absoluteFilePath() : info.fileName(),
                          documentIcon(session->documentType()), session->editor().isModified()});
    }
    m_documentTabs->setDocuments(documents, active);
}

void MainWindow::showStartMessage(const QString& line) {
    m_commandLine->appendToHistory(line);
    // The command line is hidden with the Start page: the page shows it too.
    if (m_active == nullptr && m_startPage != nullptr && !m_destroying) {
        m_startPage->addMessage(line);
    }
}

void MainWindow::refreshStartPage() {
    // The settings are read only for a window on the screen, never by a
    // script session (createMenus()).
    if (m_startPage == nullptr || m_destroying || m_scriptMode || !isVisible() ||
        !m_startPage->isVisibleTo(this)) {
        return;
    }
    ensureRecentFiles();
    m_startPage->setRecentFiles(m_recentFiles->files());
}

void MainWindow::setScriptMode() {
    m_scriptMode = true;
    m_dynamicInputAction->setChecked(false);
    m_dynamicInputAction->setEnabled(false);
    m_view->dynamicInputBox()->setInputEnabled(false);
    // Nothing is hidden in a script session, and nothing is saved.
    m_autoHide->setEnabled(false);
    setInterface(InterfaceKind::Ribbon);
    setTheme(Theme::Light);
    // At level 2 a prompt missing from its command's description is written
    // in the history: the report differs from any expected one, so the case
    // fails and prints the message among its differences (docs/adr/0026).
    m_descriptionCheck = [this](const std::string& message) {
        m_commandLine->appendToHistory(QString::fromStdString(message));
    };
    if (!m_externalFiles) {
        m_files = nullptr;
    }
    configureRunner(m_startSession->runner());
    for (const auto& session : m_sessions) {
        configureRunner(session->runner());
    }
    m_recentMenu->setEnabled(false);
    m_recentAction->setEnabled(false);
}

void MainWindow::setFileDialog(FileDialog dialog) {
    m_fileDialog = std::move(dialog);
}

QString MainWindow::documentName() const {
    return QString::fromStdString(m_active != nullptr ? m_active->file().name()
                                                      : edit::DocumentFile{}.name());
}

void MainWindow::updateDrawingState() {
    refreshTabs();
    if (m_active == nullptr) {
        setWindowTitle(tr("Tecnigrafo"));
        setWindowModified(false);
        return;
    }
    if (m_actions != nullptr) {
        const QSignalBlocker blocker(registeredAction("aids.lwdisplay"));
        registeredAction("aids.lwdisplay")->setChecked(m_active->document().lwdisplay());
    }
    setWindowTitle(tr("%1[*] — Tecnigrafo").arg(QFileInfo(documentName()).fileName()));
    setWindowModified(m_active->editor().isModified());
}

void MainWindow::submitFileText(const QString& text) {
    cmd::CommandRunner& runner = commandRunner();
    m_commandLine->appendToHistory(
        QStringLiteral("%1 %2").arg(QString::fromStdString(runner.promptText()), text));
    runner.submitText(text.toStdString());
    m_commandLine->refreshPrompt();
}

void MainWindow::startCommand(const QString& command, const QStringList& responses) {
    // With the Start page, the runner of the window: only NEW and OPEN, the
    // actions that need no document, run there, and they have no responses.
    // What the page said about the last one goes.
    if (m_active == nullptr && m_startPage != nullptr) {
        m_startPage->clearMessages();
    }
    const cmd::CommandRunner* runner = &commandRunner();
    submitFileText(command);
    // NEW, or OPEN of a file already open, ends by making another session
    // active: the command has ended, and nothing is left to answer.
    if (m_active == nullptr || runner != &m_active->runner()) {
        return;
    }
    DocumentSession::ActionResponses& pending = m_active->actionResponses();
    pending.responses = responses;
    pending.waitingAt.reset();
    giveResponses();
}

void MainWindow::giveResponses() {
    if (m_active == nullptr) {
        return;
    }
    // They belong to the command of the active session's runner.
    const cmd::CommandRunner& runner = m_active->runner();
    DocumentSession::ActionResponses& pending = m_active->actionResponses();
    while (!pending.responses.isEmpty() && runner.isActive()) {
        const cmd::Prompt prompt = runner.prompt();
        if (pending.responses.first() == QString(kWaitForUser)) {
            // The user answers at this prompt; until it changes, the next
            // input decides.
            if (!pending.waitingAt.has_value()) {
                pending.waitingAt = prompt;
                return;
            }
            if (*pending.waitingAt == prompt) {
                return;
            }
            pending.waitingAt.reset();
            pending.responses.removeFirst();
            continue;
        }
        if (!offers(prompt, pending.responses.first())) {
            break;  // the command asks something else: the rest is dropped
        }
        submitFileText(pending.responses.takeFirst());
    }
    // Answered, or the command has ended: nothing waits any more.
    pending.responses.clear();
    pending.waitingAt.reset();
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
    refreshStartPage();
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
    if (commandRunner().isActive()) {
        return;
    }
    if (m_active == nullptr && m_startPage != nullptr) {
        m_startPage->clearMessages();
    }
    if (!QFileInfo::exists(path)) {
        showStartMessage(tr("Cannot open %1: file no longer exists.").arg(path));
        ensureRecentFiles();
        m_recentFiles->remove(path);
        refreshStartPage();
        return;
    }
    // Handed to the question for the file, instead of the dialog.
    m_pendingRecent = path;
    submitFileText(QStringLiteral("OPEN"));
    refresh();
}

QString MainWindow::dialogPath() {
    if (m_active != nullptr && m_active->file().hasFile()) {
        return QFileInfo(documentName()).absoluteFilePath();
    }
    ensureRecentFiles();
    const QString folder =
        m_settings->value(QStringLiteral("lastFileDirectory"), QDir::homePath()).toString();
    return QDir(folder).filePath(documentName());
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
    // The dialogs are modal: only the answer of OPEN, by adding or
    // activating a session, changes the active one, and ends the command of
    // this runner, which stays alive (the Start page's too).
    cmd::CommandRunner& runner = commandRunner();
    while (runner.prompt().file != cmd::FilePrompt::None) {
        const cmd::FilePrompt kind = runner.prompt().file;
        const bool recent = kind == cmd::FilePrompt::Open && !m_pendingRecent.isEmpty();
        const QString path =
            recent ? std::exchange(m_pendingRecent, {}) : chooseFile(kind, dialogPath());
        if (path.isEmpty()) {
            m_pendingRecent.clear();
            runner.submit(cmd::CancelInput{});
            break;
        }
        const QString absolutePath = QFileInfo(path).absoluteFilePath();
        ensureRecentFiles();
        m_settings->setValue(QStringLiteral("lastFileDirectory"),
                             QFileInfo(absolutePath).absolutePath());
        submitFileText(absolutePath);
        // The picker already confirmed replacement. Only answer this
        // two-keyword question.
        const cmd::Prompt next = runner.prompt();
        if (kind == cmd::FilePrompt::Save &&
            next.keywords == std::vector<cmd::Keyword>{{"Yes", "Y"}, {"No", "N"}}) {
            submitFileText(QStringLiteral("Yes"));
        }
    }
    if (!runner.isActive()) {
        m_pendingRecent.clear();
    }
    m_commandLine->refreshPrompt();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_scriptMode) {
        event->accept();
        return;
    }
    // The documents close one by one, in the order of the tabs, as the cross
    // of each (decision B5; docs/adr/0041, point 3). A Cancel, or a save
    // cancelled or failed, stops there: that document, shown, and the
    // following ones stay open, the ones closed stay closed, and so does
    // the window.
    while (!m_sessions.empty()) {
        if (!closeAsked(*m_sessions.front(), true)) {
            event->ignore();
            return;
        }
    }
    saveUiSettings();
    event->accept();
}

bool MainWindow::confirmClose(DocumentSession& session) {
    // A command left halfway in a session not shown has its transaction
    // open too.
    if (session.runner().isActive() || session.editor().isModified()) {
        activate(session);
    }
    // Finish/cancel the current transaction before checking the dirty flag:
    // Esc preserves completed LINE/PLINE work, just like the command line.
    if (session.runner().isActive()) {
        session.runner().submit(cmd::CancelInput{});
        refresh();
    }
    if (!session.editor().isModified()) {
        return true;
    }
    const auto choice = QMessageBox::question(
        this, tr("Tecnigrafo"), tr("Save changes to %1?").arg(QFileInfo(documentName()).fileName()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (choice == QMessageBox::Cancel) {
        return false;
    }
    if (choice == QMessageBox::Save) {
        submitFileText(QStringLiteral("QSAVE"));
        refresh();
        return !session.editor().isModified();
    }
    return true;
}

void MainWindow::setTheme(Theme theme) {
    m_theme = theme;
    m_uiBuilder->setTheme(theme);
    auto* application = qobject_cast<QApplication*>(QApplication::instance());
    assert(application != nullptr);
    applyTheme(*application, theme);
    // Application palette notifications can be queued by the platform.
    // The window and its icons must already use the new colours now.
    setPalette(QApplication::palette());
    m_view->setStyle(sceneStyle(theme));
    m_view->dynamicInputBox()->refresh(false);
    const QSignalBlocker blocker(registeredAction("ui.theme"));
    registeredAction("ui.theme")->setChecked(theme == Theme::Dark);
    updateThemeIcons();
    updateWindowButtons();
    saveUiSettings();
}

void MainWindow::updateWindowButtons() {
    SARibbonSystemButtonBar* buttons = windowButtonBar();
    if (buttons == nullptr) {
        return;  // the frame of the system (windowStyles())
    }
    const QColor& colour = themeTokens(m_theme).text;
    if (QAbstractButton* button = buttons->minimizeButton()) {
        button->setIcon(windowButtonIcon(WindowGlyph::Minimize, WindowGlyph::Minimize, colour));
    }
    if (QAbstractButton* button = buttons->maximizeButton()) {
        button->setIcon(windowButtonIcon(WindowGlyph::Maximize, WindowGlyph::Restore, colour));
    }
    if (QAbstractButton* button = buttons->closeButton()) {
        button->setIcon(windowButtonIcon(WindowGlyph::Close, WindowGlyph::Close, colour));
    }
    // SARibbon checks the maximize button of a maximized window: there the
    // glyph changes, without the fill of a switch that is on.
    buttons->setStyleSheet(QStringLiteral(
        "QToolButton:checked { background: transparent; border-color: transparent; }"));
}

void MainWindow::updateThemeIcons() {
    for (QAction* action : m_actions->actions()) {
        const ActionSpec* spec = m_actions->spec(action->objectName());
        if (!spec->icon.isEmpty()) {
            action->setIcon(themedIcon(spec->icon, m_theme));
        }
    }
    if (m_documentTabs != nullptr) {
        m_documentTabs->setTheme(m_theme);
    }
    if (m_startPage != nullptr) {
        m_startPage->setTheme(m_theme);
    }
    // The tab of a hidden panel shows the icon of the action that opens it.
    if (auto* layers = findChild<LayerPanel*>(QStringLiteral("layerPanel")); layers != nullptr) {
        layers->setWindowIcon(registeredAction("layer.panel")->icon());
    }
    if (auto* properties = findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"));
        properties != nullptr) {
        properties->setWindowIcon(registeredAction("properties.panel")->icon());
    }
}

void MainWindow::addPanel(QDockWidget* panel, Qt::DockWidgetArea area) {
    assert(panel != nullptr && !panel->objectName().isEmpty());
    panel->setAllowedAreas(Qt::AllDockWidgetAreas);
    panel->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable |
                       QDockWidget::DockWidgetFloatable);
    // restoreDockWidget consumes the saved placeholder. Adding the dock
    // first would discard that placeholder, including its floating state.
    if (!m_uiSettingsReady || m_scriptMode || !restoreDockWidget(panel)) {
        addDockWidget(area, panel);
    }
    m_autoHide->addPanel(panel);
}

void MainWindow::showSettings() {
    if (m_scriptMode) {
        return;
    }
    SettingsDialog dialog(interfaceKind(), theme(), this);
    if (dialog.exec() == QDialog::Accepted) {
        setInterface(dialog.interfaceKind());
        setTheme(dialog.theme());
    }
}

void MainWindow::showEvent(QShowEvent* event) {
    // ScriptSession selects script mode after construction but before show.
    // Delaying the first read is essential: --script never opens QSettings.
    if (!m_uiSettingsReady && !m_scriptMode) {
        restoreUiSettings();
    }
    // On Windows SARibbonMainWindow sets the shadow of the frameless window.
    SARibbonMainWindow::showEvent(event);
    refreshStartPage();
}

void MainWindow::changeEvent(QEvent* event) {
    // The classic menu bar shows the title of a window without frame; the
    // ribbon reads the title when it paints.
    if (event->type() == QEvent::WindowTitleChange || event->type() == QEvent::ModifiedChange) {
        if (m_uiBuilder) {
            m_uiBuilder->updateTitle();
        }
        if (SARibbonBar* ribbon = ribbonBar()) {
            ribbon->update();
        }
    }
    SARibbonMainWindow::changeEvent(event);
}

void MainWindow::restoreUiSettings() {
    const QScopedValueRollback<bool> guard(m_restoringUi, true);
    ensureRecentFiles();
    ShortcutMap shortcuts(*m_actions);
    shortcuts.load(*m_settings);
    shortcuts.apply(*m_actions);
    setInterface(m_settings->value(QStringLiteral("ui/interface")).toString() ==
                         QStringLiteral("classic")
                     ? InterfaceKind::Classic
                     : InterfaceKind::Ribbon);
    setTheme(m_settings->value(QStringLiteral("ui/theme")).toString() == QStringLiteral("dark")
                 ? Theme::Dark
                 : Theme::Light);
    m_dynamicInputAction->setChecked(
        m_settings->value(QStringLiteral("ui/dynmode"), true).toBool());
    restoreGeometry(m_settings->value(QStringLiteral("ui/geometry")).toByteArray());
    restoreState(m_settings->value(QStringLiteral("ui/state")).toByteArray());
    // The hidden panels stay hidden in the place that restoreState() gave
    // them, where docking them again puts them back.
    m_autoHide->restoreState(m_settings->value(QStringLiteral("ui/autoHide")).toStringList());
    m_uiSettingsReady = true;
}

void MainWindow::saveUiSettings() {
    if (m_scriptMode || !m_uiSettingsReady || m_restoringUi) {
        return;
    }
    ensureRecentFiles();
    QString interfaceName = QStringLiteral("ribbon");
    if (interfaceKind() == InterfaceKind::Classic) {
        interfaceName = QStringLiteral("classic");
    }
    QString themeName = QStringLiteral("light");
    if (theme() == Theme::Dark) {
        themeName = QStringLiteral("dark");
    }
    m_settings->setValue(QStringLiteral("ui/dynmode"), m_dynamicInputAction->isChecked());
    m_settings->setValue(QStringLiteral("ui/interface"), interfaceName);
    m_settings->setValue(QStringLiteral("ui/theme"), themeName);
    m_settings->setValue(QStringLiteral("ui/geometry"), saveGeometry());
    m_settings->setValue(QStringLiteral("ui/state"), saveState());
    m_settings->setValue(QStringLiteral("ui/autoHide"), m_autoHide->saveState());
    m_settings->sync();
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
