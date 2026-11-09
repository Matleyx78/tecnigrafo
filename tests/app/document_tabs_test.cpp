// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The documents in tabs and the Start page (docs/adr/0041, points 2, 3 and
// 9; docs/tappe/0.7.5/P29).

#include "document_tabs.hpp"

#include "action_registry.hpp"
#include "cad_view.hpp"
#include "command_line.hpp"
#include "document_session.hpp"
#include "dynamic_input_box.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "start_page.hpp"
#include "tg/core/document.hpp"
#include "tg/core/line.hpp"
#include "tg/io/io.hpp"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <variant>

namespace tg::app {
namespace {

class DocumentTabsTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_directory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("TecnigrafoP29Test"));
        QCoreApplication::setApplicationName(QFileInfo(m_directory.path()).fileName());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
        m_window = std::make_unique<MainWindow>(MainWindow::InitialDrawing::None);
        m_window->setFileDialog([this](cmd::FilePrompt kind, const QString& /*path*/) {
            m_dialogKinds.append(kind);
            return m_fileAnswers.isEmpty() ? QString{} : m_fileAnswers.takeFirst();
        });
        // Answers the questions to save, one at a time, in their own loop.
        QObject::connect(&m_boxes, &QTimer::timeout, &m_boxes, [this] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (box != nullptr && !m_boxAnswers.isEmpty()) {
                m_asked.append(box->text());
                box->button(m_boxAnswers.takeFirst())->click();
            }
        });
        m_boxes.start(10);
        m_window->show();
        m_window->activateWindow();
        ASSERT_TRUE(QTest::qWaitForWindowActive(m_window.get()));
    }

    void TearDown() override {
        m_boxes.stop();
        m_window.reset();
        QSettings::setDefaultFormat(m_oldFormat);
        QCoreApplication::setOrganizationName(m_oldOrganization);
        QCoreApplication::setApplicationName(m_oldApplication);
    }

    [[nodiscard]] MainWindow* window() const { return m_window.get(); }

    [[nodiscard]] DocumentTabs* tabs() const { return m_window->documentTabs(); }

    [[nodiscard]] QTabBar* bar() const { return tabs()->tabBar(); }

    [[nodiscard]] QLineEdit* input() const {
        return m_window->commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    }

    [[nodiscard]] DynamicInputBox* box() const { return m_window->view()->dynamicInputBox(); }

    [[nodiscard]] QLineEdit* field(int index) const {
        return box()->findChild<QLineEdit*>(QStringLiteral("dynamicField%1").arg(index));
    }

    [[nodiscard]] QAction* action(const char* id) const {
        return m_window->actionRegistry().action(QString::fromLatin1(id));
    }

    [[nodiscard]] QString history() const {
        return m_window->findChild<QPlainTextEdit*>(QStringLiteral("history"))->toPlainText();
    }

    /// The name of the file of the session `index`, in the order of the tabs.
    [[nodiscard]] std::string name(std::size_t index) const {
        return m_window->sessions().at(index)->file().name();
    }

    /// The name of the file of the active session, or "" with the Start page.
    [[nodiscard]] std::string activeName() const {
        const DocumentSession* active = m_window->activeSession();
        return active != nullptr ? active->file().name() : std::string{};
    }

    /// A click on `+`.
    void plus() const { tabs()->newButton()->click(); }

    /// A click on the tab `tab`: 0 is Start, i + 1 the document i.
    void clickTab(int tab) const {
        events::click(bar(), Qt::LeftButton, bar()->tabRect(tab).center());
    }

    /// A click on the cross of the tab of the document `index`, and the
    /// questions it asks answered with `answers`.
    void closeTab(int index, const QList<QMessageBox::StandardButton>& answers = {}) {
        m_boxAnswers = answers;
        auto* cross = qobject_cast<QAbstractButton*>(tabs()->closeButton(index));
        ASSERT_NE(cross, nullptr);
        cross->click();
        // The tab closes after the click.
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
    }

    /// Closes the window answering `answers`; true if it closes.
    bool closeWindow(const QList<QMessageBox::StandardButton>& answers) {
        m_boxAnswers = answers;
        QCloseEvent event;
        QCoreApplication::sendEvent(m_window.get(), &event);
        return event.isAccepted();
    }

    /// `text` typed in the command line, then Enter.
    void enter(const std::string& text) const {
        events::type(input(), text);
        events::press(input(), Qt::Key_Return);
    }

    /// A LINE from (0, 0) to (10, 0): the active drawing is modified.
    void line() const {
        enter("LINE");
        enter("0,0");
        enter("10,0");
        enter("");
    }

    /// The keys of `key` pressed on `widget`, as a keyboard does.
    static void shortcut(QWidget* widget, QKeySequence::StandardKey key) {
        const auto combination = QKeySequence(key)[0];
        events::press(widget, combination.key(), combination.keyboardModifiers());
        QKeyEvent release(QEvent::KeyRelease, combination.key(), combination.keyboardModifiers());
        QCoreApplication::sendEvent(widget, &release);
        QCoreApplication::processEvents();
    }

    /// A drawing file `file` in the folder of the test.
    [[nodiscard]] QString drawingFile(const char* file) const {
        QString path = m_directory.filePath(QString::fromLatin1(file));
        const auto bytes = io::writeCadx(core::Document::newDrawing());
        QFile out(path);
        if (!bytes.ok() || !out.open(QIODevice::WriteOnly)) {
            return {};
        }
        out.write(bytes.value().data(), static_cast<qint64>(bytes.value().size()));
        return path;
    }

    [[nodiscard]] QStringList& fileAnswers() { return m_fileAnswers; }

    [[nodiscard]] const QList<cmd::FilePrompt>& dialogKinds() const { return m_dialogKinds; }

    [[nodiscard]] const QStringList& asked() const { return m_asked; }

private:
    const QString m_oldOrganization{QCoreApplication::organizationName()};
    const QString m_oldApplication{QCoreApplication::applicationName()};
    const QSettings::Format m_oldFormat{QSettings::defaultFormat()};
    QTemporaryDir m_directory;
    std::unique_ptr<MainWindow> m_window;
    QTimer m_boxes;
    QList<QMessageBox::StandardButton> m_boxAnswers;
    QStringList m_asked;
    QStringList m_fileAnswers;
    QList<cmd::FilePrompt> m_dialogKinds;
};

TEST_F(DocumentTabsTest, TheProgramStartsOnTheStartPageAndPlusAddsDrawings) {
    EXPECT_TRUE(window()->sessions().empty());
    EXPECT_EQ(window()->activeSession(), nullptr);
    EXPECT_TRUE(window()->startPage()->isVisible());
    EXPECT_FALSE(window()->view()->isVisible());
    EXPECT_FALSE(window()->commandLine()->isVisible());
    EXPECT_EQ(window()->windowTitle(), QStringLiteral("Tecnigrafo"));
    ASSERT_EQ(bar()->count(), 1);
    EXPECT_EQ(bar()->tabText(0), QStringLiteral("Start"));
    EXPECT_EQ(bar()->currentIndex(), 0);
    // Start is not a document: it has no cross.
    EXPECT_EQ(tabs()->closeButton(-1), nullptr);

    plus();
    plus();
    ASSERT_EQ(window()->sessions().size(), 2U);
    EXPECT_EQ(name(0), "Drawing1.cadx");
    EXPECT_EQ(name(1), "Drawing2.cadx");
    ASSERT_EQ(bar()->count(), 3);
    EXPECT_EQ(bar()->tabText(1), QStringLiteral("Drawing1.cadx"));
    EXPECT_EQ(bar()->tabText(2), QStringLiteral("Drawing2.cadx"));
    EXPECT_FALSE(bar()->tabIcon(1).isNull());
    EXPECT_EQ(window()->activeSession(), window()->sessions().back().get());
    EXPECT_EQ(bar()->currentIndex(), 2);
    EXPECT_TRUE(window()->view()->isVisible());
    EXPECT_FALSE(window()->startPage()->isVisible());
    EXPECT_EQ(window()->windowTitle(), QStringLiteral("Drawing2.cadx[*] — Tecnigrafo"));

    clickTab(1);
    EXPECT_EQ(window()->activeSession(), window()->sessions().front().get());
    EXPECT_EQ(window()->view()->session(), window()->sessions().front().get());
    EXPECT_EQ(bar()->currentIndex(), 1);

    clickTab(0);
    EXPECT_EQ(window()->activeSession(), nullptr);
    EXPECT_TRUE(window()->startPage()->isVisible());
    EXPECT_EQ(window()->windowTitle(), QStringLiteral("Tecnigrafo"));
    EXPECT_EQ(window()->sessions().size(), 2U);

    // The New button of the Start page adds a drawing as `+` does.
    auto* newDrawing =
        window()->startPage()->findChild<QToolButton*>(QStringLiteral("startNew.drawing"));
    ASSERT_NE(newDrawing, nullptr);
    newDrawing->click();
    ASSERT_EQ(window()->sessions().size(), 3U);
    EXPECT_EQ(activeName(), "Drawing3.cadx");
    EXPECT_EQ(bar()->currentIndex(), 3);

    // The menu of the documents lists Start and every document.
    auto* menu = tabs()->menuButton()->menu();
    ASSERT_NE(menu, nullptr);
    QMetaObject::invokeMethod(menu, "aboutToShow", Qt::DirectConnection);
    ASSERT_EQ(menu->actions().size(), 4);
    EXPECT_TRUE(menu->actions().at(3)->isChecked());
    menu->actions().at(1)->trigger();
    EXPECT_EQ(activeName(), "Drawing1.cadx");
}

TEST_F(DocumentTabsTest, TextTypedAndNotSentComesBackFromTheStartPage) {
    plus();
    enter("LINE");
    enter("0,0");
    events::type(input(), "10,20");
    clickTab(0);
    ASSERT_EQ(window()->activeSession(), nullptr);
    clickTab(1);
    ASSERT_NE(window()->activeSession(), nullptr);
    EXPECT_EQ(input()->text(), QStringLiteral("10,20"));
    events::press(input(), Qt::Key_Return);
    ASSERT_EQ(window()->document().entityCount(), 1U);
    const auto made = std::get<core::Line>(window()->document().entities().front().entity);
    EXPECT_EQ(made.end, (core::Vec2{10.0, 20.0}));
}

TEST_F(DocumentTabsTest, ValuesOfTheDynamicInputComeBackFromTheStartPage) {
    plus();
    ASSERT_TRUE(action("aids.dynmode")->isChecked());
    enter("LINE");
    enter("0,0");
    ASSERT_TRUE(box()->isVisible());
    events::type(window()->view(), "5");
    events::press(window()->view(), Qt::Key_Tab);
    events::type(window()->view(), "9");
    ASSERT_EQ(field(0)->text(), QStringLiteral("5"));
    ASSERT_EQ(field(1)->text(), QStringLiteral("9"));

    clickTab(0);
    EXPECT_FALSE(box()->isVisible());
    clickTab(1);
    ASSERT_TRUE(box()->isVisible());
    EXPECT_EQ(field(0)->text(), QStringLiteral("5"));
    EXPECT_EQ(field(1)->text(), QStringLiteral("9"));
    events::press(window()->view(), Qt::Key_Return);
    EXPECT_EQ(window()->document().entityCount(), 1U);
}

TEST_F(DocumentTabsTest, OnTheStartPageOnlyTheActionsWithoutADocumentWork) {
    const ActionRegistry& registry = window()->actionRegistry();
    for (const char* id : {"file.new", "file.open", "file.recent", "file.quit", "tools.settings",
                           "tools.shortcuts", "ui.interface", "ui.theme", "help.about"}) {
        SCOPED_TRACE(id);
        EXPECT_FALSE(registry.spec(QString::fromLatin1(id))->requiresDocument);
        EXPECT_TRUE(action(id)->isEnabled());
    }
    for (const char* id : {"edit.undo", "edit.redo", "file.save", "file.saveas", "draw.line",
                           "edit.erase", "view.zoom-extents", "aids.osnap", "layer.panel"}) {
        SCOPED_TRACE(id);
        EXPECT_TRUE(registry.spec(QString::fromLatin1(id))->requiresDocument);
        EXPECT_FALSE(action(id)->isEnabled());
    }

    // Ctrl+Z does nothing; Ctrl+O shows the dialog; Ctrl+N opens a drawing.
    const QString before = history();
    shortcut(window()->startPage(), QKeySequence::Undo);
    EXPECT_EQ(history(), before);
    shortcut(window()->startPage(), QKeySequence::Open);
    EXPECT_EQ(dialogKinds(), QList<cmd::FilePrompt>{cmd::FilePrompt::Open});
    EXPECT_TRUE(window()->sessions().empty());
    EXPECT_EQ(window()->activeSession(), nullptr);
    shortcut(window()->startPage(), QKeySequence::New);
    ASSERT_EQ(window()->sessions().size(), 1U);
    ASSERT_NE(window()->activeSession(), nullptr);

    // With a document they work again.
    EXPECT_TRUE(action("file.save")->isEnabled());
    EXPECT_TRUE(action("draw.line")->isEnabled());
    EXPECT_TRUE(action("aids.osnap")->isEnabled());
    line();
    EXPECT_TRUE(action("edit.undo")->isEnabled());
    clickTab(0);
    EXPECT_FALSE(action("edit.undo")->isEnabled());
    EXPECT_FALSE(action("file.save")->isEnabled());
    EXPECT_TRUE(action("file.new")->isEnabled());
    clickTab(1);
    EXPECT_TRUE(action("edit.undo")->isEnabled());
    EXPECT_TRUE(action("file.save")->isEnabled());
    shortcut(input(), QKeySequence::Undo);
    EXPECT_TRUE(window()->document().isEmpty());
}

TEST(DocumentTabsWindowTest, ScriptsStartFromAnEmptyActiveDrawing) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    ASSERT_EQ(window.sessions().size(), 1U);
    EXPECT_EQ(window.activeSession(), window.sessions().front().get());
    EXPECT_TRUE(window.document().isEmpty());
    EXPECT_FALSE(window.startPage()->isVisibleTo(&window));
    EXPECT_TRUE(window.view()->isVisibleTo(&window));
    const QTabBar* bar = window.documentTabs()->tabBar();
    ASSERT_EQ(bar->count(), 2);
    EXPECT_EQ(bar->currentIndex(), 1);
    EXPECT_EQ(bar->tabText(1), QStringLiteral("Drawing1.cadx"));
    EXPECT_TRUE(window.actionRegistry().action(QStringLiteral("draw.line"))->isEnabled());
}

TEST_F(DocumentTabsTest, AModifiedTabAsksBeforeItCloses) {
    plus();
    line();
    EXPECT_EQ(bar()->tabText(1), QStringLiteral("Drawing1.cadx*"));
    closeTab(0, {QMessageBox::Cancel});
    EXPECT_EQ(asked(), QStringList{QStringLiteral("Save changes to Drawing1.cadx?")});
    ASSERT_EQ(window()->sessions().size(), 1U);
    EXPECT_TRUE(window()->editor().isModified());
    EXPECT_EQ(bar()->count(), 2);

    closeTab(0, {QMessageBox::Discard});
    EXPECT_TRUE(window()->sessions().empty());
    EXPECT_EQ(window()->activeSession(), nullptr);
    EXPECT_TRUE(window()->startPage()->isVisible());
    EXPECT_EQ(bar()->count(), 1);
    EXPECT_EQ(bar()->currentIndex(), 0);
}

TEST_F(DocumentTabsTest, ACommandInProgressEndsBeforeTheTabCloses) {
    plus();
    enter("LINE");
    enter("0,0");
    enter("10,0");
    ASSERT_TRUE(window()->runner().isActive());
    // Esc keeps the segment drawn: the drawing is modified, and asked for.
    closeTab(0, {QMessageBox::Discard});
    EXPECT_EQ(asked(), QStringList{QStringLiteral("Save changes to Drawing1.cadx?")});
    EXPECT_TRUE(window()->sessions().empty());
}

TEST_F(DocumentTabsTest, ClosingATabActivatesTheOneOnItsRightThenOnItsLeft) {
    plus();
    plus();
    plus();
    clickTab(2);
    ASSERT_EQ(activeName(), "Drawing2.cadx");
    closeTab(1);
    EXPECT_EQ(activeName(), "Drawing3.cadx");
    closeTab(1);
    EXPECT_EQ(activeName(), "Drawing1.cadx");
    closeTab(0);
    EXPECT_EQ(window()->activeSession(), nullptr);
    EXPECT_TRUE(window()->startPage()->isVisible());

    // Closing a tab that is not active leaves the active one, even when it
    // asks to save.
    plus();
    plus();
    plus();
    clickTab(3);
    line();
    clickTab(1);
    ASSERT_EQ(activeName(), "Drawing1.cadx");
    closeTab(2, {QMessageBox::Discard});
    EXPECT_EQ(asked(), QStringList{QStringLiteral("Save changes to Drawing3.cadx?")});
    ASSERT_EQ(window()->sessions().size(), 2U);
    EXPECT_EQ(activeName(), "Drawing1.cadx");
    EXPECT_EQ(bar()->currentIndex(), 1);
}

TEST_F(DocumentTabsTest, ClosingTheWindowClosesTheTabsInOrderAndStopsAtCancel) {
    plus();
    plus();
    plus();
    clickTab(1);
    line();
    clickTab(3);
    line();
    clickTab(1);
    ASSERT_EQ(activeName(), "Drawing1.cadx");

    EXPECT_FALSE(closeWindow({QMessageBox::Discard, QMessageBox::Cancel}));
    EXPECT_EQ(asked(), (QStringList{QStringLiteral("Save changes to Drawing1.cadx?"),
                                    QStringLiteral("Save changes to Drawing3.cadx?")}));
    ASSERT_EQ(window()->sessions().size(), 1U);
    EXPECT_EQ(activeName(), "Drawing3.cadx");
    EXPECT_TRUE(window()->editor().isModified());
    EXPECT_EQ(bar()->count(), 2);

    EXPECT_TRUE(closeWindow({QMessageBox::Discard}));
    EXPECT_TRUE(window()->sessions().empty());
}

TEST_F(DocumentTabsTest, ClosingTheWindowWithNoForEachModifiedDocumentCloses) {
    plus();
    line();
    plus();
    plus();
    line();
    clickTab(1);
    EXPECT_TRUE(closeWindow({QMessageBox::Discard, QMessageBox::Discard}));
    EXPECT_EQ(asked().size(), 2);
    EXPECT_TRUE(window()->sessions().empty());
}

TEST_F(DocumentTabsTest, ASaveCancelledInTheDialogKeepsEveryDocumentOpen) {
    plus();
    line();
    plus();
    plus();
    line();
    clickTab(1);
    // Yes, then Cancel in the dialog of the file.
    EXPECT_FALSE(closeWindow({QMessageBox::Save}));
    EXPECT_EQ(dialogKinds(), QList<cmd::FilePrompt>{cmd::FilePrompt::Save});
    EXPECT_EQ(window()->sessions().size(), 3U);
    EXPECT_EQ(activeName(), "Drawing1.cadx");
    EXPECT_TRUE(window()->editor().isModified());
}

TEST_F(DocumentTabsTest, OpeningTheSameFileTwiceActivatesItsTab) {
    const QString path = drawingFile("plan.cadx");
    ASSERT_FALSE(path.isEmpty());
    fileAnswers() << path << path;
    action("file.open")->trigger();
    ASSERT_EQ(window()->sessions().size(), 1U);
    EXPECT_EQ(bar()->tabText(1), QStringLiteral("plan.cadx"));
    plus();
    ASSERT_EQ(window()->sessions().size(), 2U);
    ASSERT_EQ(bar()->currentIndex(), 2);
    action("file.open")->trigger();
    EXPECT_EQ(window()->sessions().size(), 2U);
    EXPECT_EQ(window()->activeSession(), window()->sessions().front().get());
    EXPECT_EQ(bar()->currentIndex(), 1);
    EXPECT_TRUE(history().contains(QStringLiteral("is already open")));
}

TEST_F(DocumentTabsTest, NewTypedAndOpenAfterADrawingAddTabs) {
    plus();
    enter("NEW");
    ASSERT_EQ(window()->sessions().size(), 2U);
    EXPECT_EQ(bar()->count(), 3);
    EXPECT_EQ(activeName(), "Drawing2.cadx");

    const QString path = drawingFile("other.cadx");
    ASSERT_FALSE(path.isEmpty());
    fileAnswers() << path;
    enter("OPEN");
    ASSERT_EQ(window()->sessions().size(), 3U);
    EXPECT_EQ(bar()->count(), 4);
    EXPECT_EQ(bar()->currentIndex(), 3);
    EXPECT_EQ(bar()->tabText(3), QStringLiteral("other.cadx"));
    EXPECT_EQ(name(1), "Drawing2.cadx");
}

TEST_F(DocumentTabsTest, TheStartPageListsTheRecentFilesAndOpensThem) {
    const QString path = drawingFile("recent.cadx");
    ASSERT_FALSE(path.isEmpty());
    fileAnswers() << path;
    action("file.open")->trigger();
    ASSERT_EQ(window()->sessions().size(), 1U);
    closeTab(0);
    ASSERT_TRUE(window()->sessions().empty());
    EXPECT_EQ(window()->startPage()->recentFiles(),
              QStringList{QFileInfo(path).absoluteFilePath()});
    Q_EMIT window() -> startPage()->recentRequested(QFileInfo(path).absoluteFilePath());
    ASSERT_EQ(window()->sessions().size(), 1U);
    EXPECT_EQ(bar()->tabText(1), QStringLiteral("recent.cadx"));
}

}  // namespace
}  // namespace tg::app
