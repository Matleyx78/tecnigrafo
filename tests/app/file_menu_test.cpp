// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "command_line.hpp"
#include "document_session.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "recent_files.hpp"
#include "script_session.hpp"
#include "tg/io/file_access.hpp"
#include "tg/io/io.hpp"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QCloseEvent>
#include <QFile>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace tg::app {
namespace {

class FileMenuTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_directory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("TecnigrafoFileMenuTest"));
        QCoreApplication::setApplicationName(QFileInfo(m_directory.path()).fileName());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
        m_window = std::make_unique<MainWindow>(MainWindow::InitialDrawing::Empty);
        m_window->setFileDialog([this](cmd::FilePrompt kind, const QString& path) {
            m_dialogKinds.append(kind);
            m_dialogPaths.append(path);
            return m_answers.isEmpty() ? QString{} : m_answers.takeFirst();
        });
        m_window->show();
        m_window->activateWindow();
        ASSERT_TRUE(QTest::qWaitForWindowActive(m_window.get()));
    }

    void TearDown() override {
        m_window.reset();
        QSettings::setDefaultFormat(m_oldFormat);
        QCoreApplication::setOrganizationName(m_oldOrganization);
        QCoreApplication::setApplicationName(m_oldApplication);
    }

    void enter(const QString& text) {
        auto* input = m_window->commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
        input->setText(text);
        events::press(input, Qt::Key_Return);
    }

    void line() {
        enter(QStringLiteral("LINE"));
        enter(QStringLiteral("0,0"));
        enter(QStringLiteral("10,0"));
        enter({});
    }

    QAction* action(const char* name) {
        return m_window->findChild<QAction*>(QString::fromLatin1(name));
    }

    [[nodiscard]] QString history() const {
        return m_window->findChild<QPlainTextEdit*>(QStringLiteral("history"))->toPlainText();
    }

    [[nodiscard]] QString path(const char* name) const {
        return m_directory.filePath(QString::fromUtf8(name));
    }

    [[nodiscard]] static QStringList recent() {
        QSettings settings;
        return RecentFiles(settings).files();
    }

    [[nodiscard]] QMenu* recentMenu() const {
        auto* menu = m_window->findChild<QMenu*>(QStringLiteral("recentMenu"));
        QMetaObject::invokeMethod(menu, "aboutToShow", Qt::DirectConnection);
        return menu;
    }

    bool closeWith(QMessageBox::StandardButton choice) {
        QTimer::singleShot(0, m_window.get(), [choice] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            ASSERT_NE(box, nullptr);
            box->button(choice)->click();
        });
        QCloseEvent event;
        QCoreApplication::sendEvent(m_window.get(), &event);
        return event.isAccepted();
    }

    [[nodiscard]] MainWindow* window() const { return m_window.get(); }

    [[nodiscard]] QStringList& answers() { return m_answers; }

    [[nodiscard]] const QList<cmd::FilePrompt>& dialogKinds() const { return m_dialogKinds; }

    [[nodiscard]] const QStringList& dialogPaths() const { return m_dialogPaths; }

    [[nodiscard]] const QTemporaryDir& directory() const { return m_directory; }

private:
    const QString m_oldOrganization{QCoreApplication::organizationName()};
    const QString m_oldApplication{QCoreApplication::applicationName()};
    const QSettings::Format m_oldFormat{QSettings::defaultFormat()};
    QTemporaryDir m_directory;
    std::unique_ptr<MainWindow> m_window;
    QStringList m_answers;
    QList<cmd::FilePrompt> m_dialogKinds;
    QStringList m_dialogPaths;
};

TEST_F(FileMenuTest, MenuOrderAndActionsUseTheRunner) {
    const QList<QPair<const char*, const char*>> commands{{"file.new", "NEW"},
                                                          {"file.open", "OPEN"},
                                                          {"file.save", "QSAVE"},
                                                          {"file.saveas", "SAVEAS"}};
    const QList<QObject*> shownIn = action("file.new")->associatedObjects();
    const auto menuOf = std::ranges::find_if(shownIn, [](const QObject* object) {
        return qobject_cast<const QMenu*>(object) != nullptr;
    });
    auto* fileMenu = menuOf != shownIn.end() ? qobject_cast<QMenu*>(*menuOf) : nullptr;
    ASSERT_NE(fileMenu, nullptr);
    ASSERT_GE(fileMenu->actions().size(), 6);
    for (qsizetype i = 0; i < commands.size(); ++i) {
        EXPECT_EQ(fileMenu->actions()[i], action(commands[i].first));
    }
    EXPECT_EQ(fileMenu->actions()[4]->menu(), recentMenu());
    window()->setScriptMode();
    io::DiskFiles files;
    window()->setFileAccess(&files);
    for (const auto& [name, command] : commands) {
        ASSERT_NE(action(name), nullptr);
        action(name)->trigger();
        EXPECT_TRUE(
            history().contains(QStringLiteral("Command: %1").arg(QString::fromLatin1(command))))
            << command << " " << action(name)->shortcut().toString().toStdString() << "\n"
            << history().toStdString() << " active=" << window()->runner().isActive();
        auto* input = window()->findChild<QLineEdit*>(QStringLiteral("input"));
        events::press(input, Qt::Key_Escape);
    }
}

TEST_F(FileMenuTest, StandardShortcutsSubmitAllFourCommands) {
    window()->setScriptMode();
    io::DiskFiles files;
    window()->setFileAccess(&files);

    struct Shortcut {
        const char* action;
        QKeySequence::StandardKey key;
        const char* command;
    };

    const QList<Shortcut> keys{{"file.new", QKeySequence::New, "NEW"},
                               {"file.open", QKeySequence::Open, "OPEN"},
                               {"file.save", QKeySequence::Save, "QSAVE"},
                               {"file.saveas", QKeySequence::SaveAs, "SAVEAS"}};
    for (const auto& [name, key, command] : keys) {
        SCOPED_TRACE(command);
        QKeySequence expected(key);
        if (key == QKeySequence::SaveAs && expected.isEmpty()) {
            expected = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S);
        }
        ASSERT_EQ(action(name)->shortcut(), expected);
        ASSERT_TRUE(action(name)->isEnabled());
        const auto combination = action(name)->shortcut()[0];
        auto* input = window()->findChild<QLineEdit*>(QStringLiteral("input"));
        input->setFocus();
        events::press(input, combination.key(), combination.keyboardModifiers());
        // Match ScriptSession and a real keyboard, including key release.
        QKeyEvent release(QEvent::KeyRelease, combination.key(), combination.keyboardModifiers());
        QCoreApplication::sendEvent(input, &release);
        EXPECT_TRUE(
            history().contains(QStringLiteral("Command: %1").arg(QString::fromLatin1(command))))
            << command << " " << action(name)->shortcut().toString().toStdString() << "\n"
            << history().toStdString() << " active=" << window()->runner().isActive();
        events::press(input, Qt::Key_Escape);
        QCoreApplication::processEvents();
    }
    EXPECT_TRUE(dialogKinds().isEmpty());
}

TEST_F(FileMenuTest, SavesOpensCopiesAndUpdatesTitleAndRecents) {
    EXPECT_EQ(window()->windowTitle(), QStringLiteral("Drawing1.cadx[*] — Tecnigrafo"));
    EXPECT_FALSE(window()->isWindowModified());
    line();
    EXPECT_TRUE(window()->isWindowModified());
    const QString original = path("drawing with spaces.cadx");
    answers().append(original);
    action("file.save")->trigger();
    EXPECT_TRUE(QFileInfo::exists(original));
    EXPECT_FALSE(window()->isWindowModified());
    EXPECT_EQ(window()->windowTitle(), QStringLiteral("drawing with spaces.cadx[*] — Tecnigrafo"));
    EXPECT_EQ(recent(), QStringList{original});
    const QString copy = path("copy.dxf");
    answers().append(copy);
    enter(QStringLiteral("SAVE"));
    EXPECT_TRUE(QFileInfo::exists(copy));
    EXPECT_EQ(QString::fromStdString(window()->runner().documentFile().name()), original);
    EXPECT_EQ(recent().first(), copy);
    action("file.new")->trigger();
    EXPECT_TRUE(window()->document().isEmpty());
    answers().append(copy);
    action("file.open")->trigger();
    EXPECT_EQ(window()->document().entityCount(), 1U);
    EXPECT_EQ(recent().first(), copy);
    EXPECT_EQ(dialogKinds(), (QList<cmd::FilePrompt>{cmd::FilePrompt::Save, cmd::FilePrompt::Save,
                                                     cmd::FilePrompt::Open}));
    EXPECT_EQ(dialogPaths()[1], original);
    EXPECT_EQ(QFileInfo(dialogPaths()[2]).absolutePath(), directory().path());
    const QString renamed = path("renamed.cadx");
    answers().append(renamed);
    action("file.saveas")->trigger();
    EXPECT_EQ(QString::fromStdString(window()->runner().documentFile().name()), renamed);
    EXPECT_EQ(recent().first(), renamed);
    line();
    action("file.save")->trigger();
    EXPECT_FALSE(window()->editor().isModified());
    EXPECT_EQ(dialogKinds().size(), 4);
    enter(QStringLiteral("U"));
    EXPECT_TRUE(window()->isWindowModified());
    enter(QStringLiteral("REDO"));
    EXPECT_FALSE(window()->isWindowModified());
}

TEST_F(FileMenuTest, CancelAndFailuresPreserveDrawingAndRecentFiles) {
    line();
    action("file.saveas")->trigger();
    EXPECT_FALSE(window()->runner().isActive());
    EXPECT_TRUE(window()->editor().isModified());
    EXPECT_TRUE(recent().isEmpty());
    answers().append(path("missing-directory/drawing.cadx"));
    action("file.save")->trigger();
    EXPECT_TRUE(window()->editor().isModified());
    EXPECT_TRUE(recent().isEmpty());
    EXPECT_TRUE(history().contains(QStringLiteral("Cannot save")));
    // OPEN asks nothing about the modified drawing: the dialog comes at once.
    answers().append(path("missing.cadx"));
    enter(QStringLiteral("OPEN"));
    EXPECT_EQ(window()->sessions().size(), 1U);
    EXPECT_EQ(window()->document().entityCount(), 1U);
    EXPECT_TRUE(recent().isEmpty());
    EXPECT_TRUE(history().contains(QStringLiteral("Cannot open")));
}

TEST_F(FileMenuTest, InvalidDrawingDoesNotEnterRecentList) {
    const QString invalid = path("invalid.cadx");
    QFile file(invalid);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    ASSERT_EQ(file.write("invalid drawing"), 15);
    file.close();
    answers().append(invalid);
    action("file.open")->trigger();
    EXPECT_TRUE(window()->document().isEmpty());
    EXPECT_FALSE(window()->runner().documentFile().hasFile());
    EXPECT_TRUE(recent().isEmpty());
    EXPECT_TRUE(history().contains(QStringLiteral("Cannot open")));
}

TEST_F(FileMenuTest, SavingModifiedCopyPreservesDirtyFlagAndRecentOrderOnUndo) {
    const QString original = path("original.cadx");
    line();
    answers().append(original);
    action("file.save")->trigger();
    line();
    const QString copy = path("copy.cadx");
    answers().append(copy);
    enter(QStringLiteral("SAVE"));
    EXPECT_EQ(QString::fromStdString(window()->runner().documentFile().name()), original);
    EXPECT_TRUE(window()->isWindowModified());
    EXPECT_EQ(recent(), (QStringList{copy, original}));
    enter(QStringLiteral("U"));
    EXPECT_FALSE(window()->isWindowModified());
    EXPECT_EQ(recent(), (QStringList{copy, original}));
    enter(QStringLiteral("INSUNITS"));
    enter(QStringLiteral("4"));
    EXPECT_EQ(recent(), (QStringList{copy, original}));
}

TEST_F(FileMenuTest, FileActionsAreDisabledDuringCommands) {
    enter(QStringLiteral("LINE"));
    for (const char* name : {"file.new", "file.open", "file.save", "file.saveas"}) {
        EXPECT_FALSE(action(name)->isEnabled());
    }
    EXPECT_FALSE(recentMenu()->isEnabled());
    auto* input = window()->findChild<QLineEdit*>(QStringLiteral("input"));
    events::press(input, Qt::Key_Escape);
    EXPECT_TRUE(action("file.open")->isEnabled());
}

TEST_F(FileMenuTest, PickerConfirmationReplacesExistingFileWithoutSecondQuestion) {
    const QString drawing = path("existing.cadx");
    line();
    answers().append(drawing);
    action("file.save")->trigger();
    line();
    answers().append(drawing);
    action("file.saveas")->trigger();
    EXPECT_FALSE(window()->runner().isActive());
    EXPECT_FALSE(window()->editor().isModified());
    EXPECT_TRUE(history().contains(QStringLiteral("[Yes/No] <No>: Yes")));
}

TEST_F(FileMenuTest, RecentOpenAddsASessionAndRemovesMissingEntries) {
    const QString original = path("original.cadx");
    line();
    answers().append(original);
    action("file.save")->trigger();
    // A copy of one line, in the recent files but not open.
    const QString drawing = path("recent.cadx");
    answers().append(drawing);
    enter(QStringLiteral("SAVE"));
    line();
    ASSERT_EQ(recentMenu()->actions().size(), 2);
    ASSERT_EQ(recentMenu()->actions().first()->text(), drawing);
    recentMenu()->actions().first()->trigger();
    // No question about the modified drawing, which stays open.
    EXPECT_FALSE(window()->runner().isActive());
    ASSERT_EQ(window()->sessions().size(), 2U);
    EXPECT_EQ(window()->activeSession(), window()->sessions().back().get());
    EXPECT_EQ(window()->document().entityCount(), 1U);
    EXPECT_EQ(window()->sessions().front()->document().entityCount(), 2U);
    EXPECT_EQ(dialogKinds().size(), 2);
    EXPECT_EQ(recent(), (QStringList{drawing, original}));
    ASSERT_TRUE(QFile::remove(drawing));
    recentMenu()->actions().first()->trigger();
    EXPECT_EQ(recent(), QStringList{original});
    EXPECT_TRUE(history().contains(QStringLiteral("file no longer exists")));
    EXPECT_EQ(recentMenu()->actions().size(), 1);
    EXPECT_EQ(window()->sessions().size(), 2U);
}

TEST_F(FileMenuTest, CloseCancelDiscardSaveAndCancelledSave) {
    line();
    EXPECT_FALSE(closeWith(QMessageBox::Cancel));
    EXPECT_TRUE(window()->editor().isModified());
    EXPECT_FALSE(closeWith(QMessageBox::Save));
    EXPECT_TRUE(window()->editor().isModified());
    answers().append(path("missing-directory/closed.cadx"));
    EXPECT_FALSE(closeWith(QMessageBox::Save));
    EXPECT_TRUE(window()->editor().isModified());
    answers().append(path("closed.cadx"));
    EXPECT_TRUE(closeWith(QMessageBox::Save));
    EXPECT_TRUE(QFileInfo::exists(path("closed.cadx")));
    // Closing the window closed the document saved (decision B5): the
    // Start page is left, where New opens another.
    EXPECT_TRUE(window()->sessions().empty());
    action("file.new")->trigger();
    ASSERT_EQ(window()->sessions().size(), 1U);
    line();
    EXPECT_TRUE(closeWith(QMessageBox::Discard));
    EXPECT_TRUE(window()->sessions().empty());
}

TEST_F(FileMenuTest, ScriptSessionDoesNotReadOrWriteSettingsOrShowDialogs) {
    QSettings settings;
    settings.setValue(QStringLiteral("recentFiles"), QStringList{path("untouched.cadx")});
    settings.setValue(QStringLiteral("lastFileDirectory"), QStringLiteral("untouched"));
    settings.sync();
    const QStringList keys = settings.allKeys();
    io::DiskFiles files;
    window()->setFileAccess(&files);
    const ScriptSession session(*window(), directory().path());
    line();
    action("file.save")->trigger();
    EXPECT_EQ(window()->runner().prompt().file, cmd::FilePrompt::Save);
    enter(path("script.cadx"));
    EXPECT_TRUE(QFileInfo::exists(path("script.cadx")));
    line();
    QCloseEvent event;
    QCoreApplication::sendEvent(window(), &event);
    EXPECT_TRUE(event.isAccepted());
    EXPECT_TRUE(dialogKinds().isEmpty());
    settings.sync();
    EXPECT_EQ(settings.allKeys(), keys);
    EXPECT_EQ(settings.value(QStringLiteral("recentFiles")).toStringList(),
              QStringList{path("untouched.cadx")});
    EXPECT_EQ(settings.value(QStringLiteral("lastFileDirectory")).toString(),
              QStringLiteral("untouched"));
}

TEST_F(FileMenuTest, CtrlNWithAModifiedDrawingOpensANewSessionWithoutAQuestion) {
    line();
    const DocumentSession* first = window()->activeSession();
    auto* input = window()->findChild<QLineEdit*>(QStringLiteral("input"));
    input->setFocus();
    const auto combination = action("file.new")->shortcut()[0];
    events::press(input, combination.key(), combination.keyboardModifiers());
    QKeyEvent release(QEvent::KeyRelease, combination.key(), combination.keyboardModifiers());
    QCoreApplication::sendEvent(input, &release);
    ASSERT_EQ(window()->sessions().size(), 2U);
    EXPECT_NE(window()->activeSession(), first);
    EXPECT_EQ(window()->activeSession(), window()->sessions().back().get());
    EXPECT_FALSE(window()->runner().isActive());
    EXPECT_TRUE(window()->document().isEmpty());
    EXPECT_EQ(QString::fromStdString(window()->runner().documentFile().name()),
              QStringLiteral("Drawing2.cadx"));
    EXPECT_EQ(window()->windowTitle(), QStringLiteral("Drawing2.cadx[*] — Tecnigrafo"));
    // The drawing left keeps its changes; nothing asked to save them.
    EXPECT_TRUE(first->editor().isModified());
    EXPECT_FALSE(first->runner().isActive());
    EXPECT_FALSE(history().contains(QStringLiteral("Save changes")));
    EXPECT_TRUE(dialogKinds().isEmpty());
}

/// Writes a DXF file of one line with the program, from a session closed
/// afterwards, so that only the window's first session is left.
void writePlan(MainWindow& window, const QString& plan) {
    io::DiskFiles files;
    DocumentSession& writer = window.openSession();
    writer.runner().setFileAccess(&files);
    for (const char* text : {"LINE", "0,0", "10,0", "", "SAVEAS"}) {
        writer.runner().submitText(text);
    }
    writer.runner().submitText(plan.toStdString());
    window.closeSession(writer);
}

TEST_F(FileMenuTest, OpenFromTheMenuAddsASessionAndPutsTheFileFirstInRecents) {
    const QString plan = path("plan.dxf");
    writePlan(*window(), plan);
    ASSERT_TRUE(QFileInfo::exists(plan));
    line();
    answers().append(plan);
    action("file.open")->trigger();
    ASSERT_EQ(window()->sessions().size(), 2U);
    EXPECT_EQ(window()->activeSession(), window()->sessions().back().get());
    EXPECT_EQ(QString::fromStdString(window()->runner().documentFile().name()), plan);
    EXPECT_EQ(window()->document().entityCount(), 1U);
    EXPECT_EQ(recent().first(), plan);
}

TEST_F(FileMenuTest, OpenTypedOnTheCommandLineAddsASessionAndPutsTheFileFirstInRecents) {
    const QString plan = path("plan.dxf");
    writePlan(*window(), plan);
    line();
    // The dialog gives the name the command asks for, as FILEDIA 1 does.
    answers().append(plan);
    enter(QStringLiteral("OPEN"));
    ASSERT_EQ(window()->sessions().size(), 2U);
    EXPECT_EQ(window()->activeSession(), window()->sessions().back().get());
    EXPECT_EQ(recent().first(), plan);
    EXPECT_EQ(dialogKinds(), QList<cmd::FilePrompt>{cmd::FilePrompt::Open});
}

TEST_F(FileMenuTest, OpenOfAFileAlreadyOpenActivatesItAndLeavesTheRecentsAsTheyAre) {
    const QString plan = path("plan.dxf");
    writePlan(*window(), plan);
    answers().append(plan);
    action("file.open")->trigger();
    ASSERT_EQ(window()->sessions().size(), 2U);
    const DocumentSession* opened = window()->activeSession();
    const QString other = path("other.cadx");
    answers().append(other);
    action("file.new")->trigger();
    action("file.saveas")->trigger();
    ASSERT_EQ(window()->sessions().size(), 3U);
    ASSERT_EQ(recent().first(), other);
    const QStringList before = recent();
    answers().append(plan);
    action("file.open")->trigger();
    EXPECT_EQ(window()->sessions().size(), 3U);
    EXPECT_EQ(window()->activeSession(), opened);
    EXPECT_TRUE(history().contains(QStringLiteral("%1 is already open.").arg(plan)));
    // Activating it moves nothing in the recent files.
    EXPECT_EQ(recent(), before);
}

TEST_F(FileMenuTest, ClosingAsksForAModifiedDocumentThatIsNotActive) {
    line();
    const DocumentSession* first = window()->activeSession();
    action("file.new")->trigger();
    ASSERT_EQ(window()->sessions().size(), 2U);
    ASSERT_NE(window()->activeSession(), first);
    ASSERT_FALSE(window()->editor().isModified());
    QString asked;
    QTimer::singleShot(0, window(), [&asked] {
        auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
        ASSERT_NE(box, nullptr);
        asked = box->text();
        box->button(QMessageBox::Cancel)->click();
    });
    QCloseEvent event;
    QCoreApplication::sendEvent(window(), &event);
    // The active drawing is clean, the first one is not: it is shown and
    // asked for, and Cancel keeps the window open.
    EXPECT_FALSE(event.isAccepted());
    EXPECT_EQ(asked, QStringLiteral("Save changes to Drawing1.cadx?"));
    EXPECT_EQ(window()->activeSession(), first);
    EXPECT_TRUE(first->editor().isModified());
    EXPECT_TRUE(closeWith(QMessageBox::Discard));
}

/// Files in memory, for names typed on the command line in a script session.
class MemoryFiles final : public io::FileAccess {
public:
    io::Result<std::string> read(std::string_view name) override {
        const auto found = m_contents.find(std::string{name});
        if (found == m_contents.end()) {
            return io::Error{io::Error::Kind::File, 0, "File not found."};
        }
        return found->second;
    }

    std::optional<io::Error> write(std::string_view name, std::string_view bytes) override {
        m_contents[std::string{name}] = bytes;
        return {};
    }

    [[nodiscard]] bool exists(std::string_view name) const override {
        return m_contents.contains(std::string{name});
    }

private:
    std::map<std::string, std::string> m_contents;
};

TEST_F(FileMenuTest, OpenOfAFileNamedAsTheUntitledDrawingReadsIt) {
    window()->setScriptMode();
    MemoryFiles files;
    const auto bytes = io::writeCadx(core::Document::newDrawing());
    ASSERT_TRUE(bytes.ok());
    ASSERT_FALSE(files.write("Drawing1.cadx", bytes.value()).has_value());
    window()->setFileAccess(&files);
    // A relative name typed on the command line: the untitled Drawing1.cadx
    // has no file, so the file of that name is not open.
    enter(QStringLiteral("OPEN"));
    enter(QStringLiteral("Drawing1.cadx"));
    ASSERT_EQ(window()->sessions().size(), 2U);
    EXPECT_EQ(window()->activeSession(), window()->sessions().back().get());
    EXPECT_TRUE(window()->runner().documentFile().hasFile());
    EXPECT_FALSE(history().contains(QStringLiteral("is already open")));
}

/// The bytes of the file `path`; empty if it cannot be read.
QByteArray contents(const QString& path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray{};
}

/// While it lives, answers every question box of the window with `choice`,
/// and records its text.
class QuestionAnswer {
public:
    explicit QuestionAnswer(QMessageBox::StandardButton choice) {
        QObject::connect(&m_timer, &QTimer::timeout, &m_timer, [this, choice] {
            auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (box != nullptr && box->button(choice) != nullptr) {
                m_asked.append(box->text());
                box->button(choice)->click();
            }
        });
        m_timer.start(0);
    }

    [[nodiscard]] const QStringList& asked() const { return m_asked; }

private:
    QTimer m_timer;
    QStringList m_asked;
};

TEST_F(FileMenuTest, SaveAsOverADrawingOpenElsewhereIsRefusedBeforeTheQuestionToReplace) {
    // R2 of the revision of the correction (#588): A is a.cadx, B is b.cadx.
    const QString a = path("a.cadx");
    line();
    answers().append(a);
    action("file.save")->trigger();
    ASSERT_TRUE(QFileInfo::exists(a));
    const DocumentSession* first = window()->activeSession();
    action("file.new")->trigger();
    ASSERT_EQ(window()->sessions().size(), 2U);
    ASSERT_NE(window()->activeSession(), first);
    const QString b = path("b.cadx");
    line();
    answers().append(b);
    action("file.saveas")->trigger();
    line();
    ASSERT_EQ(QString::fromStdString(window()->runner().documentFile().name()), b);
    ASSERT_TRUE(window()->editor().isModified());
    const QByteArray before = contents(a);
    ASSERT_FALSE(before.isEmpty());
    const qsizetype saves = history().count(QStringLiteral("Command: SAVEAS"));

    // From B, a.cadx chosen in the dialog: refused at once, without the
    // question, once, and nothing is saved.
    {
        const QuestionAnswer answer(QMessageBox::No);
        answers().append(a);
        action("file.saveas")->trigger();
        EXPECT_TRUE(answer.asked().isEmpty());
    }
    const QString refused =
        QStringLiteral("Cannot save %1: it is open in another document.").arg(a);
    EXPECT_EQ(history().split(QLatin1Char('\n')).count(refused), 1);
    EXPECT_EQ(history().count(QStringLiteral("Command: SAVEAS")), saves);
    EXPECT_EQ(contents(a), before);
    EXPECT_EQ(QString::fromStdString(window()->runner().documentFile().name()), b);
    EXPECT_TRUE(window()->editor().isModified());

    // c.cadx exists and is not open: the question; No saves nothing.
    const QString c = path("c.cadx");
    QFile existing(c);
    ASSERT_TRUE(existing.open(QIODevice::WriteOnly));
    ASSERT_EQ(existing.write("existing"), 8);
    existing.close();
    {
        const QuestionAnswer answer(QMessageBox::No);
        answers().append(c);
        action("file.saveas")->trigger();
        EXPECT_EQ(answer.asked().size(), 1);
    }
    EXPECT_EQ(contents(c), QByteArray("existing"));
    EXPECT_EQ(QString::fromStdString(window()->runner().documentFile().name()), b);
    EXPECT_TRUE(window()->editor().isModified());
    // Yes saves.
    {
        const QuestionAnswer answer(QMessageBox::Yes);
        answers().append(c);
        action("file.saveas")->trigger();
        EXPECT_EQ(answer.asked().size(), 1);
    }
    EXPECT_EQ(QString::fromStdString(window()->runner().documentFile().name()), c);
    EXPECT_FALSE(window()->editor().isModified());
    EXPECT_NE(contents(c), QByteArray("existing"));
    EXPECT_EQ(recent().first(), c);
}

TEST_F(FileMenuTest, SaveFromTheMenuKeepsTheVersionOfAnOldDxfAndSaveAsWritesR2018) {
    // As CurrentDxfPreservesReadVersionAndSaveasWritesR2018 of the commands.
    const QString old = path("old.dxf");
    QFile file(old);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF\n");
    file.close();
    answers().append(old);
    action("file.open")->trigger();
    ASSERT_EQ(window()->sessions().size(), 2U);
    ASSERT_EQ(QString::fromStdString(window()->runner().documentFile().name()), old);
    line();
    action("file.save")->trigger();
    EXPECT_FALSE(window()->editor().isModified());
    auto saved = io::readDxf(contents(old).toStdString());
    ASSERT_TRUE(saved.ok());
    EXPECT_EQ(saved.value().document.raw().version, "AC1015");
    EXPECT_EQ(saved.value().document.entityCount(), 1U);

    const QString renamed = path("new.dxf");
    answers().append(renamed);
    action("file.saveas")->trigger();
    EXPECT_EQ(QString::fromStdString(window()->runner().documentFile().name()), renamed);
    saved = io::readDxf(contents(renamed).toStdString());
    ASSERT_TRUE(saved.ok());
    EXPECT_EQ(saved.value().document.raw().version, "AC1032");
}

}  // namespace
}  // namespace tg::app
