// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "recent_files.hpp"
#include "script_session.hpp"
#include "tg/io/file_access.hpp"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
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
#include <memory>
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
    EXPECT_EQ(QString::fromStdString(window()->editor().fileName()), original);
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
    EXPECT_EQ(QString::fromStdString(window()->editor().fileName()), renamed);
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
    enter(QStringLiteral("OPEN"));
    answers().append(path("missing.cadx"));
    enter(QStringLiteral("N"));
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
    EXPECT_FALSE(window()->editor().hasFile());
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
    EXPECT_EQ(QString::fromStdString(window()->editor().fileName()), original);
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

TEST_F(FileMenuTest, RecentOpenSurvivesUnsavedChangesAndRemovesMissingEntries) {
    const QString drawing = path("recent.cadx");
    line();
    answers().append(drawing);
    action("file.save")->trigger();
    line();
    ASSERT_EQ(recentMenu()->actions().size(), 1);
    recentMenu()->actions().first()->trigger();
    EXPECT_TRUE(window()->runner().isActive());
    enter(QStringLiteral("N"));
    EXPECT_EQ(window()->document().entityCount(), 1U);
    EXPECT_EQ(dialogKinds().size(), 1);
    ASSERT_TRUE(QFile::remove(drawing));
    recentMenu()->actions().first()->trigger();
    EXPECT_TRUE(recent().isEmpty());
    EXPECT_TRUE(history().contains(QStringLiteral("file no longer exists")));
    EXPECT_TRUE(recentMenu()->actions().isEmpty());
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
    line();
    EXPECT_TRUE(closeWith(QMessageBox::Discard));
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

}  // namespace
}  // namespace tg::app
