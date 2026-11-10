// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The active environment and the registries of the types of document
// (docs/adr/0041, points 1, 6 and 7; docs/tappe/0.7.5/P30), shown with an
// environment and a type of document that exist only here: module app knows
// nothing of them. The type of the test opens and saves files of its own,
// through the frame of the window (docs/tappe/0.7.5/P30b).

#include "environments.hpp"

#include "action_registry.hpp"
#include "command_line.hpp"
#include "document_session.hpp"
#include "document_types.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "shortcut_map.hpp"
#include "start_page.hpp"
#include "tg/cmd/command.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/core/document.hpp"
#include "tg/core/line.hpp"
#include "tg/io/io.hpp"

#include <QAction>
#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QList>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <gtest/gtest.h>

#include <SARibbon.h>
#include <memory>
#include <span>
#include <string>
#include <utility>

namespace tg::app {
namespace {

/// The environment, the type and the ribbon tab of the test.
constexpr const char* kTest = "test";

/// The extension of the test files, which only the type of the test opens.
constexpr const char* kTestExtension = "tgtest";

/// What the type of the test writes in its files, and reads back.
constexpr const char* kTestContents = "a document of the test\n";

/// The action and the command of the environment of the test.
constexpr const char* kTestAction = "test.draw";
constexpr const char* kTestCommand = "TESTDRAW";

/// The key of the test action: no built-in action has it.
constexpr Qt::Key kTestKey = Qt::Key_F7;

/// What the runner says of a command typed outside its environments
/// (docs/tappe/0.7.5/P27).
const QString kNotAvailable =
    QStringLiteral("Command \"LINE\" is not available in this environment.");

QString latin1(const char* text) {
    return QString::fromLatin1(text);
}

/// TESTDRAW draws a line from (0, 0) to (10, 0), one step of undo, and asks
/// nothing.
class TestDrawCommand final : public cmd::Command {
public:
    void start(cmd::CommandContext& context) override {
        context.editor.beginTransaction(kTestCommand);
        core::Line line;
        line.start = {0.0, 0.0};
        line.end = {10.0, 0.0};
        context.editor.add(line);
        context.editor.commit();
    }

    cmd::Status handle(cmd::CommandContext& /*context*/, const cmd::Input& /*input*/) override {
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override { return {}; }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }
};

/// What the window asked of the type of the test.
struct TypeCalls {
    QList<SaveMode> saves;  ///< DocumentType::save, in order
    QStringList saved;      ///< the paths of the saves
    QStringList opened;     ///< DocumentType::open
};

/// The environment `test`, with its ribbon tab, its action and its command,
/// and the type `test`, which hosts only it: what a second environment adds
/// to the window, through the registries alone. The type writes its files
/// with kTestContents, and records in `calls`, if any, what it is asked.
void addTestEnvironmentWith(const MainWindow::Registries& registries, TypeCalls* calls) {
    registries.environments.add(
        {latin1(kTest), QStringLiteral("Test"), QStringLiteral("help-about"), {latin1(kTest)}});

    ActionSpec draw;
    draw.id = latin1(kTestAction);
    draw.text = QStringLiteral("Test &Draw");
    draw.description = QStringLiteral("Draws the line of the test.");
    draw.shortcut = QKeySequence(kTestKey);
    draw.command = latin1(kTestCommand);
    draw.environments = {latin1(kTest)};
    draw.menu = QStringLiteral("&Test");
    draw.ribbon = QStringLiteral("test/tools");
    registries.actions.add(std::move(draw));

    cmd::CommandDescription description;
    description.name = kTestCommand;
    description.summary = "Draws the line of the test.";
    description.environments = {kTest};
    registries.commands.add(std::move(description),
                            [] { return std::make_unique<TestDrawCommand>(); });

    const cmd::CommandRegistry* commands = &registries.commands;
    DocumentType type;
    type.id = latin1(kTest);
    type.name = QStringLiteral("Test");
    type.icon = QStringLiteral("doc-drawing");
    type.extensions = {latin1(kTestExtension)};
    type.environments = {latin1(kTest)};
    type.create = [commands] {
        return std::make_unique<DocumentSession>(*commands, nullptr, core::Document::newDrawing(),
                                                 latin1(kTest));
    };
    type.open = [commands, calls](
                    const QString& path,
                    const HistoryMessage& message) -> std::unique_ptr<DocumentSession> {
        if (calls != nullptr) {
            calls->opened.append(path);
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly) || file.readAll() != QByteArray(kTestContents)) {
            message(QStringLiteral("Cannot open %1: not a document of the test.").arg(path));
            return nullptr;
        }
        auto session = std::make_unique<DocumentSession>(
            *commands, nullptr, core::Document::newDrawing(), latin1(kTest));
        session->file().setSaved(path.toStdString());
        return session;
    };
    type.save = [calls](DocumentSession& session, SaveMode mode, const QString& path,
                        const HistoryMessage& message) {
        if (calls != nullptr) {
            calls->saves.append(mode);
            calls->saved.append(path);
        }
        QFile file(path);
        const QByteArray contents(kTestContents);
        if (!file.open(QIODevice::WriteOnly) || file.write(contents) != contents.size()) {
            message(QStringLiteral("Cannot save %1.").arg(path));
            return false;
        }
        session.editor().markSaved();
        session.file().setSaved(path.toStdString());
        return true;
    };
    registries.types.add(std::move(type));
}

/// The environment and the type of the test, recording nothing.
void addTestEnvironment(const MainWindow::Registries& registries) {
    addTestEnvironmentWith(registries, nullptr);
}

class EnvironmentsTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_directory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("TecnigrafoP30Test"));
        QCoreApplication::setApplicationName(QFileInfo(m_directory.path()).fileName());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
        m_window = std::make_unique<MainWindow>(MainWindow::InitialDrawing::Empty, nullptr,
                                                [this](const MainWindow::Registries& registries) {
                                                    addTestEnvironmentWith(registries, &m_calls);
                                                });
        m_window->setFileDialog([this](cmd::FilePrompt kind, const QString& /*path*/) {
            m_dialogs.append(kind);
            return m_answers.isEmpty() ? QString{} : m_answers.takeFirst();
        });
        m_window->show();
        m_window->activateWindow();
        ASSERT_TRUE(QTest::qWaitForWindowActive(m_window.get()));
        m_drawing = m_window->activeSession();
        ASSERT_NE(m_drawing, nullptr);
        const DocumentType* type = m_window->documentTypes().find(latin1(kTest));
        ASSERT_NE(type, nullptr);
        m_test = &m_window->openSession(*type);
    }

    void TearDown() override {
        // Destruction avoids the questions to save of close().
        m_window.reset();
        QSettings::setDefaultFormat(m_oldFormat);
        QCoreApplication::setOrganizationName(m_oldOrganization);
        QCoreApplication::setApplicationName(m_oldApplication);
    }

    [[nodiscard]] MainWindow& window() const { return *m_window; }

    /// The drawing the window opened with.
    [[nodiscard]] DocumentSession& drawing() const { return *m_drawing; }

    /// The document of the type of the test, opened after it.
    [[nodiscard]] DocumentSession& test() const { return *m_test; }

    [[nodiscard]] QAction* action(const char* id) const {
        return m_window->actionRegistry().action(latin1(id));
    }

    [[nodiscard]] QLineEdit* input() const {
        return m_window->commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    }

    [[nodiscard]] QString history() const {
        return m_window->findChild<QPlainTextEdit*>(QStringLiteral("history"))->toPlainText();
    }

    [[nodiscard]] QToolButton* indicator() const {
        return m_window->findChild<QToolButton*>(QStringLiteral("environmentIndicator"));
    }

    [[nodiscard]] SARibbonBar* ribbon() const { return m_window->ribbonBar(); }

    /// Whether the ribbon shows the tab `name` ("home", "test").
    [[nodiscard]] bool hasTab(const char* name) const {
        const SARibbonCategory* category = ribbon()->categoryByObjectName(latin1(name));
        return category != nullptr && ribbon()->isCategoryVisible(category);
    }

    /// Whether the ribbon shows the group `path` ("home/draw").
    [[nodiscard]] bool hasGroup(const char* path) const {
        const auto* panel = ribbon()->findChild<SARibbonPanel*>(latin1(path));
        const SARibbonCategory* category =
            ribbon()->categoryByObjectName(latin1(path).section(QLatin1Char('/'), 0, 0));
        return panel != nullptr && !panel->isHidden() && category != nullptr &&
               ribbon()->isCategoryVisible(category);
    }

    /// The toolbar `path` of the classic interface ("home/draw").
    [[nodiscard]] QToolBar* toolbar(const char* path) const {
        return m_window->findChild<QToolBar*>(latin1(path));
    }

    /// Whether the bar of the menus on show has the menu `title` ("&Draw").
    [[nodiscard]] bool hasMenu(const char* title) const {
        const auto* bar = m_window->findChild<QMenuBar*>(QStringLiteral("classicMenuBar"));
        if (bar == nullptr) {
            return false;
        }
        for (const QAction* item : bar->actions()) {
            if (item->text() == latin1(title)) {
                return item->isVisible();
            }
        }
        return false;
    }

    /// `text` typed in the command line, then Enter.
    void enter(const std::string& text) const {
        events::type(input(), text);
        events::press(input(), Qt::Key_Return);
    }

    /// The keys `sequence` pressed on `widget`, as a keyboard does.
    static void shortcut(QWidget* widget, const QKeySequence& sequence) {
        const auto combination = sequence[0];
        events::press(widget, combination.key(), combination.keyboardModifiers());
        QKeyEvent release(QEvent::KeyRelease, combination.key(), combination.keyboardModifiers());
        QCoreApplication::sendEvent(widget, &release);
        QCoreApplication::processEvents();
    }

    /// The file dialogs shown, by kind.
    [[nodiscard]] const QList<cmd::FilePrompt>& dialogs() const { return m_dialogs; }

    /// What the next file dialogs answer, in order; none left: Cancel.
    [[nodiscard]] QStringList& answers() { return m_answers; }

    /// What the window asked of the type of the test.
    [[nodiscard]] const TypeCalls& calls() const { return m_calls; }

    /// The path of `name` in the folder of the test.
    [[nodiscard]] QString path(const char* name) const {
        return m_directory.filePath(QString::fromLatin1(name));
    }

    /// How many lines of the history are `line`.
    [[nodiscard]] qsizetype historyLines(const QString& line) const {
        return history().split(QLatin1Char('\n')).count(line);
    }

    /// A drawing of one line saved as `name`, in the folder of the test.
    [[nodiscard]] QString drawingFile(const char* name) const {
        core::Document document = core::Document::newDrawing();
        core::Line line;
        line.end = {10.0, 0.0};
        document.add(line);
        const auto bytes = io::writeCadx(document);
        const QString file = path(name);
        QFile out(file);
        if (!bytes.ok() || !out.open(QIODevice::WriteOnly)) {
            return {};
        }
        out.write(bytes.value().data(), static_cast<qint64>(bytes.value().size()));
        return file;
    }

private:
    QTemporaryDir m_directory;
    const QString m_oldOrganization{QCoreApplication::organizationName()};
    const QString m_oldApplication{QCoreApplication::applicationName()};
    const QSettings::Format m_oldFormat{QSettings::defaultFormat()};
    std::unique_ptr<MainWindow> m_window;
    DocumentSession* m_drawing{nullptr};
    DocumentSession* m_test{nullptr};
    QList<cmd::FilePrompt> m_dialogs;
    QStringList m_answers;
    TypeCalls m_calls;
};

TEST(DocumentTypesTest, TheProgramHasTheDrawingInDrawing2d) {
    const cmd::CommandRegistry commands{};
    DocumentTypeRegistry types;
    EnvironmentRegistry environments;
    registerBuiltinDocumentTypes(types, environments, commands);

    ASSERT_EQ(environments.list().size(), 1);
    const Environment* drawing2d = environments.find(QStringLiteral("drawing2d"));
    ASSERT_NE(drawing2d, nullptr);
    EXPECT_EQ(drawing2d->name, QStringLiteral("Drawing"));
    EXPECT_EQ(drawing2d->icon, QStringLiteral("env-drawing2d"));
    EXPECT_TRUE(drawing2d->contextualRibbon.isEmpty());

    ASSERT_EQ(types.list().size(), 1);
    const DocumentType* drawing = types.find(latin1(DocumentSession::kDrawingType));
    ASSERT_NE(drawing, nullptr);
    EXPECT_EQ(drawing->environments, QStringList{QStringLiteral("drawing2d")});
    EXPECT_EQ(drawing->extensions, (QStringList{QStringLiteral("cadx"), QStringLiteral("dxf")}));
    EXPECT_EQ(types.findByExtension(QStringLiteral("DXF")), drawing);
    EXPECT_EQ(types.findByExtension(QStringLiteral("cadx")), drawing);
    EXPECT_EQ(types.findByExtension(latin1(kTestExtension)), nullptr);
    const std::unique_ptr<DocumentSession> created = drawing->create();
    ASSERT_NE(created, nullptr);
    EXPECT_EQ(created->documentType(), drawing->id);
    EXPECT_TRUE(created->document().isEmpty());
    EXPECT_FALSE(created->file().hasFile());
}

TEST_F(EnvironmentsTest, WithOnlyTheDrawingEveryBuiltInActionIsOffered) {
    // The window as it was before the environments: nothing hidden.
    EXPECT_EQ(drawing().environment(), QStringLiteral("drawing2d"));
    EXPECT_EQ(drawing().runner().environment(), "drawing2d");
    for (QAction* item : window().actionRegistry().actions()) {
        const ActionSpec* spec = window().actionRegistry().spec(item->objectName());
        SCOPED_TRACE(item->objectName().toStdString());
        EXPECT_EQ(item->isVisible(), item->objectName() != latin1(kTestAction));
        EXPECT_TRUE(availableIn(*spec, QStringLiteral("drawing2d")) ||
                    item->objectName() == latin1(kTestAction));
    }
    EXPECT_TRUE(hasTab("home"));
    EXPECT_TRUE(hasTab("view"));
    EXPECT_FALSE(hasTab(kTest));
    EXPECT_TRUE(hasGroup("home/draw"));
    EXPECT_FALSE(hasGroup("test/tools"));
}

TEST_F(EnvironmentsTest, ATabOfTheTestTypeActivatesTheTestEnvironment) {
    window().activate(test());
    EXPECT_EQ(test().environment(), latin1(kTest));
    EXPECT_EQ(test().runner().environment(), kTest);
    ASSERT_NE(window().activeEnvironment(), nullptr);
    EXPECT_EQ(window().activeEnvironment()->id, latin1(kTest));

    // The ribbon has the contextual tab, at the right of the fixed ones, and
    // not Draw.
    EXPECT_TRUE(hasTab(kTest));
    EXPECT_TRUE(hasGroup("test/tools"));
    EXPECT_FALSE(hasGroup("home/draw"));
    EXPECT_FALSE(hasTab("view"));
    EXPECT_GT(ribbon()->categoryIndex(ribbon()->categoryByObjectName(latin1(kTest))),
              ribbon()->categoryIndex(ribbon()->categoryByObjectName(QStringLiteral("home"))));
    EXPECT_TRUE(action(kTestAction)->isVisible());
    EXPECT_FALSE(action("draw.line")->isVisible());
    EXPECT_FALSE(action("draw.line")->isEnabled());

    // LINE typed does not start; the command of the test does.
    enter("LINE");
    EXPECT_TRUE(history().split(QLatin1Char('\n')).contains(kNotAvailable));
    EXPECT_FALSE(test().runner().isActive());
    enter(kTestCommand);
    EXPECT_FALSE(test().runner().isActive());
    EXPECT_EQ(test().document().entityCount(), 1U);
    EXPECT_TRUE(drawing().document().isEmpty());

    // The indicator names the environment.
    ASSERT_NE(indicator(), nullptr);
    EXPECT_TRUE(indicator()->isVisibleTo(&window()));
    EXPECT_EQ(indicator()->text(), QStringLiteral("Test"));
    EXPECT_FALSE(indicator()->icon().isNull());
}

TEST_F(EnvironmentsTest, BackToTheDrawingRibbonMenusAndCommandsAreTheDrawingsAgain) {
    window().activate(test());
    window().activate(drawing());
    EXPECT_FALSE(hasTab(kTest));
    EXPECT_TRUE(hasTab("view"));
    EXPECT_TRUE(hasGroup("home/draw"));
    EXPECT_TRUE(action("draw.line")->isVisible());
    EXPECT_TRUE(action("draw.line")->isEnabled());
    EXPECT_FALSE(action(kTestAction)->isVisible());
    EXPECT_EQ(indicator()->text(), QStringLiteral("Drawing"));

    // The classic menus too, without rebuilding the window.
    window().setInterface(InterfaceKind::Classic);
    EXPECT_TRUE(hasMenu("&Draw"));
    EXPECT_FALSE(hasMenu("&Test"));
    window().activate(test());
    EXPECT_FALSE(hasMenu("&Draw"));
    EXPECT_TRUE(hasMenu("&Test"));
    EXPECT_TRUE(hasMenu("&File"));
    window().activate(drawing());
    EXPECT_TRUE(hasMenu("&Draw"));
    EXPECT_FALSE(hasMenu("&Test"));
    window().setInterface(InterfaceKind::Ribbon);

    // LINE typed starts again.
    enter("LINE");
    EXPECT_TRUE(drawing().runner().isActive());
    EXPECT_FALSE(history().split(QLatin1Char('\n')).contains(kNotAvailable));
    events::press(input(), Qt::Key_Escape);
    EXPECT_FALSE(drawing().runner().isActive());
}

TEST_F(EnvironmentsTest, AToolbarTheUserHidStaysHiddenAcrossTheTabs) {
    window().setInterface(InterfaceKind::Classic);
    // Qt shows the new toolbars a moment later.
    QCoreApplication::processEvents();
    QToolBar* draw = toolbar("home/draw");
    QToolBar* modify = toolbar("home/modify");
    ASSERT_NE(draw, nullptr);
    ASSERT_NE(modify, nullptr);
    EXPECT_FALSE(draw->isHidden());
    // As QMainWindow's menu of the toolbars does.
    draw->toggleViewAction()->trigger();
    ASSERT_TRUE(draw->isHidden());

    // Another drawing: the same environment.
    DocumentSession& other = window().openSession();
    window().activate(other);
    EXPECT_TRUE(draw->isHidden());
    EXPECT_FALSE(modify->isHidden());
    // The test environment hides the other one too, and gives it back; a
    // new interface does the same.
    window().activate(test());
    EXPECT_TRUE(draw->isHidden());
    EXPECT_TRUE(modify->isHidden());
    window().setInterface(InterfaceKind::Ribbon);
    window().setInterface(InterfaceKind::Classic);
    QCoreApplication::processEvents();
    EXPECT_TRUE(toolbar("home/modify")->isHidden());
    window().setInterface(InterfaceKind::Ribbon);
    window().activate(drawing());
    window().setInterface(InterfaceKind::Classic);
    QCoreApplication::processEvents();
    draw = toolbar("home/draw");
    modify = toolbar("home/modify");
    ASSERT_FALSE(draw->isHidden());
    draw->toggleViewAction()->trigger();
    window().activate(test());
    EXPECT_TRUE(draw->isHidden());
    EXPECT_TRUE(modify->isHidden());
    window().activate(drawing());
    EXPECT_TRUE(draw->isHidden());
    EXPECT_FALSE(modify->isHidden());
    // So does the Start page.
    window().showStartPage();
    EXPECT_TRUE(modify->isHidden());
    window().activate(other);
    EXPECT_TRUE(draw->isHidden());
    EXPECT_FALSE(modify->isHidden());

    // Shown again by the user, it follows the environment.
    draw->toggleViewAction()->trigger();
    EXPECT_FALSE(draw->isHidden());
    window().activate(test());
    EXPECT_TRUE(draw->isHidden());
    window().activate(drawing());
    EXPECT_FALSE(draw->isHidden());
}

TEST_F(EnvironmentsTest, ARestoredLayoutKeepsTheToolbarsAsTheUserLeftThem) {
    window().setInterface(InterfaceKind::Classic);
    QCoreApplication::processEvents();
    ASSERT_NE(toolbar("home/draw"), nullptr);
    toolbar("home/draw")->toggleViewAction()->trigger();
    // Closed from a tab of the test, where the toolbars of the drawing are
    // all hidden by the environment; closing ends on the Start page.
    window().activate(test());
    ASSERT_TRUE(window().close());

    MainWindow restored(MainWindow::InitialDrawing::Empty, nullptr, addTestEnvironment);
    restored.show();
    QCoreApplication::processEvents();
    ASSERT_EQ(restored.interfaceKind(), InterfaceKind::Classic);
    auto* draw = restored.findChild<QToolBar*>(QStringLiteral("home/draw"));
    auto* modify = restored.findChild<QToolBar*>(QStringLiteral("home/modify"));
    ASSERT_NE(draw, nullptr);
    ASSERT_NE(modify, nullptr);
    EXPECT_TRUE(draw->isHidden());
    EXPECT_FALSE(modify->isHidden());

    // A drawing opened after the restore keeps them as they are.
    DocumentSession& first = *restored.activeSession();
    DocumentSession& other = restored.openSession();
    restored.activate(other);
    EXPECT_TRUE(draw->isHidden());
    EXPECT_FALSE(modify->isHidden());
    restored.activate(restored.openSession(*restored.documentTypes().find(latin1(kTest))));
    EXPECT_TRUE(modify->isHidden());
    restored.activate(first);
    EXPECT_TRUE(draw->isHidden());
    EXPECT_FALSE(modify->isHidden());
}

TEST_F(EnvironmentsTest, TheChosenRibbonTabStaysWhileItIsShown) {
    ribbon()->raiseCategory(ribbon()->categoryByObjectName(QStringLiteral("home")));
    window().activate(test());
    EXPECT_EQ(ribbon()->currentIndex(),
              ribbon()->categoryIndex(ribbon()->categoryByObjectName(QStringLiteral("home"))));
    ribbon()->raiseCategory(ribbon()->categoryByObjectName(latin1(kTest)));
    // The tab of the test goes away with its environment: a tab still shown
    // is chosen.
    window().activate(drawing());
    EXPECT_EQ(ribbon()->currentIndex(),
              ribbon()->categoryIndex(ribbon()->categoryByObjectName(QStringLiteral("home"))));
}

TEST_F(EnvironmentsTest, AShortcutOfTheTestEnvironmentDoesNothingInTheDrawing) {
    const QString before = history();
    shortcut(input(), QKeySequence(kTestKey));
    EXPECT_TRUE(drawing().document().isEmpty());
    EXPECT_TRUE(test().document().isEmpty());
    EXPECT_EQ(history(), before);

    // In its own environment it works.
    window().activate(test());
    shortcut(input(), QKeySequence(kTestKey));
    EXPECT_EQ(test().document().entityCount(), 1U);
}

TEST_F(EnvironmentsTest, TheGlobalActionsWorkOnTheStartPage) {
    window().showStartPage();
    EXPECT_EQ(window().activeEnvironment(), nullptr);
    for (const char* id : {"file.new", "file.open", "file.recent", "file.quit", "tools.settings",
                           "tools.shortcuts", "ui.interface", "ui.theme", "help.about"}) {
        SCOPED_TRACE(id);
        EXPECT_TRUE(action(id)->isVisible());
        EXPECT_TRUE(action(id)->isEnabled());
    }
    // Undo is there, without a document to undo.
    EXPECT_TRUE(action("edit.undo")->isVisible());
    EXPECT_FALSE(action("edit.undo")->isEnabled());
    // Save and Save As are there too, without a document to save
    // (docs/tappe/0.7.5/P30b).
    for (const char* id : {"file.save", "file.saveas"}) {
        SCOPED_TRACE(id);
        EXPECT_TRUE(action(id)->isVisible());
        EXPECT_FALSE(action(id)->isEnabled());
    }
    for (const char* id : {"draw.line", "edit.erase", "aids.osnap", kTestAction}) {
        SCOPED_TRACE(id);
        EXPECT_FALSE(action(id)->isVisible());
        EXPECT_FALSE(action(id)->isEnabled());
    }
    EXPECT_FALSE(hasGroup("home/draw"));
    EXPECT_FALSE(hasTab(kTest));
    window().setInterface(InterfaceKind::Classic);
    EXPECT_TRUE(hasMenu("&File"));
    EXPECT_TRUE(hasMenu("&Tools"));
    EXPECT_FALSE(hasMenu("&Draw"));
    EXPECT_FALSE(hasMenu("&Test"));
    window().setInterface(InterfaceKind::Ribbon);

    // Ctrl+Z does nothing; Ctrl+O shows the dialog; Ctrl+N opens a drawing.
    const QString before = history();
    shortcut(window().startPage(), QKeySequence(QKeySequence::Undo));
    EXPECT_EQ(history(), before);
    shortcut(window().startPage(), QKeySequence(QKeySequence::Open));
    EXPECT_EQ(dialogs(), QList<cmd::FilePrompt>{cmd::FilePrompt::Open});
    EXPECT_EQ(window().activeSession(), nullptr);
    shortcut(window().startPage(), QKeySequence(QKeySequence::New));
    ASSERT_EQ(window().sessions().size(), 3U);
    ASSERT_NE(window().activeSession(), nullptr);
    EXPECT_EQ(window().activeSession()->documentType(), latin1(DocumentSession::kDrawingType));
    EXPECT_EQ(window().activeSession()->environment(), QStringLiteral("drawing2d"));
}

TEST_F(EnvironmentsTest, TheGlobalActionsAreInTheTestEnvironmentAndUndoItsCommand) {
    window().activate(test());
    for (const char* id : {"file.new", "file.open", "file.save", "file.saveas", "file.quit",
                           "edit.undo", "edit.redo", "tools.settings", "help.about"}) {
        SCOPED_TRACE(id);
        EXPECT_TRUE(action(id)->isVisible());
    }
    EXPECT_FALSE(hasGroup("home/draw"));
    EXPECT_FALSE(action("draw.line")->isVisible());

    enter(kTestCommand);
    ASSERT_EQ(test().document().entityCount(), 1U);
    EXPECT_TRUE(action("edit.undo")->isEnabled());
    action("edit.undo")->trigger();
    EXPECT_TRUE(test().document().isEmpty());
    EXPECT_TRUE(action("edit.redo")->isEnabled());
    shortcut(input(), QKeySequence(QKeySequence::Redo));
    EXPECT_EQ(test().document().entityCount(), 1U);
}

TEST_F(EnvironmentsTest, AGlobalShortcutConflictsWithTheActionsOfEveryEnvironment) {
    const ShortcutMap shortcuts(window().actionRegistry());
    const QKeySequence newKey = shortcuts.shortcut(QStringLiteral("file.new"));
    ASSERT_FALSE(newKey.isEmpty());
    // With an action of the drawing.
    EXPECT_EQ(shortcuts.conflict(QStringLiteral("file.new"), QKeySequence(Qt::Key_F3)),
              QStringLiteral("aids.osnap"));
    EXPECT_EQ(shortcuts.conflict(QStringLiteral("aids.osnap"), newKey), QStringLiteral("file.new"));
    // With the action of another environment.
    EXPECT_EQ(shortcuts.conflict(QStringLiteral("file.new"), QKeySequence(kTestKey)),
              latin1(kTestAction));
    EXPECT_EQ(shortcuts.conflict(latin1(kTestAction), newKey), QStringLiteral("file.new"));
    // Two environments do not conflict with each other.
    EXPECT_TRUE(shortcuts.conflict(latin1(kTestAction), QKeySequence(Qt::Key_F3)).isEmpty());
}

TEST_F(EnvironmentsTest, TheEnvironmentFollowsTheActiveSession) {
    EXPECT_EQ(window().activeEnvironment()->id, QStringLiteral("drawing2d"));
    window().activate(test());
    EXPECT_EQ(window().activeEnvironment()->id, latin1(kTest));
    window().activate(drawing());
    EXPECT_EQ(window().activeEnvironment()->id, QStringLiteral("drawing2d"));
    EXPECT_EQ(drawing().runner().environment(), "drawing2d");
    window().activate(test());
    EXPECT_EQ(window().activeEnvironment()->id, latin1(kTest));
    EXPECT_EQ(test().environment(), latin1(kTest));
    EXPECT_EQ(test().runner().environment(), kTest);
    // Through the Start page too.
    window().showStartPage();
    window().activate(test());
    EXPECT_EQ(window().activeEnvironment()->id, latin1(kTest));
    EXPECT_TRUE(hasTab(kTest));
}

TEST_F(EnvironmentsTest, TheIndicatorStaysOnTheStartPageAndNamesNoEnvironment) {
    window().showStartPage();
    ASSERT_NE(indicator(), nullptr);
    EXPECT_TRUE(indicator()->isVisibleTo(&window()));
    EXPECT_FALSE(indicator()->text().isEmpty());
    EXPECT_FALSE(indicator()->text().contains(QStringLiteral("Drawing")));
    EXPECT_FALSE(indicator()->text().contains(QStringLiteral("Test")));
    window().activate(drawing());
    EXPECT_EQ(indicator()->text(), QStringLiteral("Drawing"));
    EXPECT_FALSE(indicator()->icon().isNull());
    window().activate(test());
    EXPECT_EQ(indicator()->text(), QStringLiteral("Test"));
}

TEST_F(EnvironmentsTest, TheStartPageOffersEveryType) {
    window().showStartPage();
    auto* drawingButton = window().startPage()->findChild<QToolButton*>(
        QStringLiteral("startNew.") + latin1(DocumentSession::kDrawingType));
    auto* testButton =
        window().startPage()->findChild<QToolButton*>(QStringLiteral("startNew.") + latin1(kTest));
    ASSERT_NE(drawingButton, nullptr);
    ASSERT_NE(testButton, nullptr);
    testButton->click();
    ASSERT_NE(window().activeSession(), nullptr);
    EXPECT_EQ(window().activeSession()->documentType(), latin1(kTest));
    EXPECT_EQ(window().activeEnvironment()->id, latin1(kTest));
}

TEST_F(EnvironmentsTest, SaveAsAndSaveOfTheTestTypeGoThroughTheType) {
    // B6 of docs/DECISIONI.md: the type saves, the frame shows the dialog.
    window().activate(test());
    const QString file = path("a.tgtest");
    answers().append(file);
    action("file.saveas")->trigger();
    EXPECT_EQ(calls().saves, QList<SaveMode>{SaveMode::SaveAs});
    EXPECT_EQ(calls().saved, QStringList{file});
    EXPECT_EQ(dialogs(), QList<cmd::FilePrompt>{cmd::FilePrompt::Save});
    EXPECT_TRUE(QFileInfo::exists(file));
    EXPECT_EQ(QString::fromStdString(test().file().name()), file);
    EXPECT_FALSE(test().editor().isModified());
    EXPECT_EQ(window().windowTitle(), QStringLiteral("a.tgtest[*] — Tecnigrafo"));
    EXPECT_FALSE(window().isWindowModified());
    // No command of the drawing ran for it.
    EXPECT_FALSE(history().contains(QStringLiteral("not available in this environment")));
    EXPECT_FALSE(history().contains(QStringLiteral("SAVEAS")));

    enter(kTestCommand);
    ASSERT_TRUE(window().isWindowModified());
    action("file.save")->trigger();
    EXPECT_EQ(calls().saves, (QList<SaveMode>{SaveMode::SaveAs, SaveMode::Save}));
    EXPECT_EQ(calls().saved, (QStringList{file, file}));
    // Its own file: no dialog.
    EXPECT_EQ(dialogs().size(), 1);
    EXPECT_FALSE(window().isWindowModified());
    EXPECT_FALSE(history().contains(QStringLiteral("QSAVE")));
}

TEST_F(EnvironmentsTest, OpenOfATestFileOpensATabOfTheTestTypeAndThenActivatesIt) {
    window().activate(test());
    const QString file = path("a.tgtest");
    answers().append(file);
    action("file.saveas")->trigger();
    ASSERT_TRUE(QFileInfo::exists(file));
    window().closeSession(test());
    ASSERT_EQ(window().sessions().size(), 1U);

    answers().append(file);
    action("file.open")->trigger();
    EXPECT_EQ(calls().opened, QStringList{file});
    ASSERT_EQ(window().sessions().size(), 2U);
    DocumentSession* opened = window().activeSession();
    ASSERT_EQ(opened, window().sessions().back().get());
    EXPECT_EQ(opened->documentType(), latin1(kTest));
    EXPECT_EQ(opened->environment(), latin1(kTest));
    EXPECT_EQ(opened->runner().environment(), kTest);
    ASSERT_NE(window().activeEnvironment(), nullptr);
    EXPECT_EQ(window().activeEnvironment()->id, latin1(kTest));
    EXPECT_EQ(QString::fromStdString(opened->file().name()), file);
    EXPECT_FALSE(opened->editor().isModified());

    // Again, from the drawing: the same tab, with the message, and the type
    // is not asked.
    window().activate(drawing());
    answers().append(file);
    action("file.open")->trigger();
    EXPECT_EQ(window().sessions().size(), 2U);
    EXPECT_EQ(window().activeSession(), opened);
    EXPECT_EQ(historyLines(QStringLiteral("%1 is already open.").arg(file)), 1);
    EXPECT_EQ(calls().opened, QStringList{file});
}

TEST_F(EnvironmentsTest, NewAndOpenOfADrawingFromTheTestTabRunNoCommandThere) {
    const QString plan = drawingFile("plan.cadx");
    ASSERT_FALSE(plan.isEmpty());
    window().activate(test());
    action("file.new")->trigger();
    ASSERT_EQ(window().sessions().size(), 3U);
    DocumentSession* created = window().activeSession();
    ASSERT_EQ(created, window().sessions().back().get());
    EXPECT_EQ(created->documentType(), latin1(DocumentSession::kDrawingType));
    EXPECT_EQ(created->environment(), QStringLiteral("drawing2d"));
    EXPECT_TRUE(created->document().isEmpty());

    window().activate(test());
    answers().append(plan);
    action("file.open")->trigger();
    ASSERT_EQ(window().sessions().size(), 4U);
    DocumentSession* opened = window().activeSession();
    ASSERT_EQ(opened, window().sessions().back().get());
    EXPECT_EQ(opened->documentType(), latin1(DocumentSession::kDrawingType));
    EXPECT_EQ(opened->environment(), QStringLiteral("drawing2d"));
    EXPECT_EQ(opened->document().entityCount(), 1U);
    EXPECT_EQ(QString::fromStdString(opened->file().name()), plan);

    // Nothing reached the runner of the test: no command, no message of P27.
    EXPECT_FALSE(test().runner().isActive());
    EXPECT_FALSE(test().editor().canUndo());
    EXPECT_FALSE(history().contains(QStringLiteral("not available in this environment")));
    EXPECT_TRUE(calls().opened.isEmpty());
    EXPECT_TRUE(calls().saves.isEmpty());
}

TEST_F(EnvironmentsTest, SaveAsOverADrawingOpenElsewhereIsRefusedWithoutCallingTheType) {
    const QString plan = drawingFile("plan.cadx");
    ASSERT_FALSE(plan.isEmpty());
    answers().append(plan);
    action("file.open")->trigger();
    ASSERT_EQ(window().sessions().size(), 3U);
    const QByteArray before = [&plan] {
        QFile file(plan);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
    }();

    window().activate(test());
    enter(kTestCommand);
    answers().append(plan);
    action("file.saveas")->trigger();
    EXPECT_EQ(
        historyLines(QStringLiteral("Cannot save %1: it is open in another document.").arg(plan)),
        1);
    EXPECT_TRUE(calls().saves.isEmpty());
    EXPECT_FALSE(test().file().hasFile());
    EXPECT_TRUE(test().editor().isModified());
    QFile file(plan);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(file.readAll(), before);
}

TEST_F(EnvironmentsTest, AFileOfAnExtensionNoTypeHasOpensNothing) {
    const QString file = path("notes.txt");
    QFile out(file);
    ASSERT_TRUE(out.open(QIODevice::WriteOnly));
    out.write("notes");
    out.close();
    answers().append(file);
    action("file.open")->trigger();
    EXPECT_EQ(window().sessions().size(), 2U);
    EXPECT_EQ(window().activeSession(), &drawing());
    EXPECT_TRUE(calls().opened.isEmpty());
    // "Cannot open <name>: …", with the extensions of every type.
    EXPECT_EQ(historyLines(QStringLiteral(
                               "Cannot open %1: only .cadx, .dxf and .tgtest files are supported.")
                               .arg(file)),
              1);
}

}  // namespace
}  // namespace tg::app
