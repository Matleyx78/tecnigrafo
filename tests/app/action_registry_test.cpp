// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "action_registry.hpp"

#include "cad_view.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "tg/io/file_access.hpp"

#include <QAction>
#include <QKeySequence>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QPointF>
#include <QSet>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QToolBar>
#include <gtest/gtest.h>

#include <utility>

namespace tg::app {
namespace {

// A point on the outline of the sample drawing: a click there selects it.
constexpr core::Vec2 kOutline{60.0, 0.0};

ActionSpec spec(const QString& id, const QString& text, const QString& menu = {}) {
    ActionSpec result;
    result.id = id;
    result.text = text;
    result.menu = menu;
    return result;
}

QStringList titles(const QList<QAction*>& actions) {
    QStringList result;
    for (const QAction* action : actions) {
        result.append(action->isSeparator() ? QStringLiteral("-") : action->text());
    }
    return result;
}

TEST(ActionRegistryTest, AddsActionsNamedByTheirIdInOrder) {
    ActionRegistry registry;
    ActionSpec line = spec(QStringLiteral("draw.line"), QStringLiteral("&Line"));
    line.description = QStringLiteral("Draws lines.");
    line.shortcut = QKeySequence(Qt::Key_F5);
    line.command = QStringLiteral("LINE");
    QAction* lineAction = registry.add(line);
    ActionSpec ortho = spec(QStringLiteral("aids.ortho"), QStringLiteral("&Ortho"));
    ortho.checkable = true;
    QAction* orthoAction = registry.add(ortho);

    ASSERT_NE(lineAction, nullptr);
    EXPECT_EQ(lineAction->parent(), &registry);
    EXPECT_EQ(lineAction->objectName(), QStringLiteral("draw.line"));
    EXPECT_EQ(lineAction->text(), QStringLiteral("&Line"));
    EXPECT_EQ(lineAction->toolTip(), QStringLiteral("Draws lines."));
    EXPECT_EQ(lineAction->shortcut(), QKeySequence(Qt::Key_F5));
    EXPECT_FALSE(lineAction->isCheckable());
    EXPECT_TRUE(orthoAction->isCheckable());

    EXPECT_EQ(registry.action(QStringLiteral("draw.line")), lineAction);
    EXPECT_EQ(registry.action(QStringLiteral("aids.ortho")), orthoAction);
    EXPECT_EQ(registry.action(QStringLiteral("draw.circle")), nullptr);
    ASSERT_NE(registry.spec(QStringLiteral("draw.line")), nullptr);
    EXPECT_EQ(registry.spec(QStringLiteral("draw.line"))->command, QStringLiteral("LINE"));
    EXPECT_EQ(registry.spec(QStringLiteral("draw.line"))->environments,
              QStringList{QStringLiteral("drawing2d")});
    EXPECT_EQ(registry.spec(QStringLiteral("draw.circle")), nullptr);
    EXPECT_EQ(registry.actions(), (QList<QAction*>{lineAction, orthoAction}));
}

TEST(ActionRegistryTest, SpecsStayValidWhileActionsAreAdded) {
    ActionRegistry registry;
    registry.add(spec(QStringLiteral("first"), QStringLiteral("First")));
    const ActionSpec* first = registry.spec(QStringLiteral("first"));
    for (int i = 0; i < 100; ++i) {
        registry.add(spec(QStringLiteral("more.%1").arg(i), QStringLiteral("More")));
    }
    EXPECT_EQ(registry.spec(QStringLiteral("first")), first);
    EXPECT_EQ(first->text, QStringLiteral("First"));
}

TEST(ActionRegistryTest, DynamicActionsAreCreatedOnceAndRegistered) {
    ActionRegistry registry;
    registry.add(spec(QStringLiteral("file.recent"), QStringLiteral("Recent Files")));
    const QList<QAction*> three = registry.dynamicActions(QStringLiteral("file.recent"), 3);
    ASSERT_EQ(three.size(), 3);
    for (qsizetype i = 0; i < three.size(); ++i) {
        const QString id = QStringLiteral("file.recent.%1").arg(i + 1);
        EXPECT_EQ(three[i]->objectName(), id);
        EXPECT_EQ(registry.action(id), three[i]);
        EXPECT_EQ(three[i]->parent(), &registry);
        EXPECT_FALSE(three[i]->isSeparator());
    }
    EXPECT_EQ(registry.dynamicActions(QStringLiteral("file.recent"), 2), three.mid(0, 2));
    const QList<QAction*> four = registry.dynamicActions(QStringLiteral("file.recent"), 4);
    ASSERT_EQ(four.size(), 4);
    EXPECT_EQ(four.mid(0, 3), three);
    EXPECT_TRUE(registry.dynamicActions(QStringLiteral("file.recent"), 0).isEmpty());
    EXPECT_EQ(registry.actions().size(), 5);
    EXPECT_EQ(registry.actions().last(), four.last());
}

TEST(ActionRegistryDeathTest, AnIdCanBeAddedOnlyOnce) {
#ifdef NDEBUG
    GTEST_SKIP() << "preconditions are checked with assert, disabled in this build";
#endif
    // The child process starts the tests again: forking a Qt application
    // is not safe.
    GTEST_FLAG_SET(death_test_style, "threadsafe");
    ActionRegistry registry;
    registry.add(spec(QStringLiteral("edit.undo"), QStringLiteral("&Undo")));
    EXPECT_DEATH(registry.add(spec(QStringLiteral("edit.undo"), QStringLiteral("Again"))),
                 "already registered");
}

TEST(ActionRegistryTest, BuildsMenusSubmenusAndSeparatorsInOrder) {
    ActionRegistry registry;
    const QString tools = QStringLiteral("&Tools");
    const QString modes = QStringLiteral("&Tools/Object Snap &Modes");
    registry.add(spec(QStringLiteral("file.new"), QStringLiteral("&New"), QStringLiteral("&File")));
    registry.add(spec(QStringLiteral("aids.osnap"), QStringLiteral("Object &Snap"), tools));
    registry.add(spec(QStringLiteral("aids.snap.endpoint"), QStringLiteral("&Endpoint"), modes));
    registry.add(spec(QStringLiteral("aids.separator"), {}, tools));
    registry.add(spec(QStringLiteral("aids.ortho"), QStringLiteral("&Ortho"), tools));
    registry.add(spec(QStringLiteral("aids.snap.center"), QStringLiteral("&Center"), modes));
    registry.add(spec(QStringLiteral("hidden"), QStringLiteral("Not in a menu")));
    registry.add(
        spec(QStringLiteral("file.quit"), QStringLiteral("&Quit"), QStringLiteral("&File")));

    QMenuBar bar;
    registry.buildMenus(&bar);
    const QList<QAction*> menus = bar.actions();
    ASSERT_EQ(titles(menus), (QStringList{QStringLiteral("&File"), tools}));
    EXPECT_EQ(titles(menus[0]->menu()->actions()),
              (QStringList{QStringLiteral("&New"), QStringLiteral("&Quit")}));
    QMenu* toolsMenu = menus[1]->menu();
    ASSERT_NE(toolsMenu, nullptr);
    EXPECT_EQ(titles(toolsMenu->actions()),
              (QStringList{QStringLiteral("Object &Snap"), QStringLiteral("Object Snap &Modes"),
                           QStringLiteral("-"), QStringLiteral("&Ortho")}));
    EXPECT_EQ(toolsMenu->actions()[2], registry.action(QStringLiteral("aids.separator")));
    QMenu* modesMenu = toolsMenu->actions()[1]->menu();
    ASSERT_NE(modesMenu, nullptr);
    EXPECT_EQ(titles(modesMenu->actions()),
              (QStringList{QStringLiteral("&Endpoint"), QStringLiteral("&Center")}));
    EXPECT_TRUE(registry.action(QStringLiteral("hidden"))->associatedObjects().isEmpty());
}

class MainWindowActionsTest : public testing::Test {
protected:
    void SetUp() override {
        m_window.show();
        m_window.activateWindow();
        ASSERT_TRUE(QTest::qWaitForWindowActive(&m_window));
    }

    [[nodiscard]] MainWindow& window() { return m_window; }

    [[nodiscard]] const ActionRegistry& registry() const { return m_window.actionRegistry(); }

    [[nodiscard]] QAction* action(const char* id) const {
        return registry().action(QString::fromLatin1(id));
    }

    [[nodiscard]] QLineEdit* input() const {
        return m_window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    }

    /// How many lines of the history are exactly `line`.
    [[nodiscard]] qsizetype linesEqualTo(const QString& line) const {
        const QString history =
            m_window.findChild<QPlainTextEdit*>(QStringLiteral("history"))->toPlainText();
        return history.split(QLatin1Char('\n')).count(line);
    }

    void click(const core::Vec2& world) {
        const core::Vec2 pixel = m_window.view()->viewport().worldToScreen(world);
        events::click(m_window.view(), Qt::LeftButton, QPointF{pixel.x, pixel.y}.toPoint());
    }

private:
    MainWindow m_window;
};

TEST_F(MainWindowActionsTest, EveryActionOfTheWindowIsInTheRegistry) {
    // Qt gives every QMenu its own menuAction(), a child of the menu. When
    // a registry action takes the menu (file.recent, with setMenu()), Qt's
    // action stays a child of the menu but is shown nowhere.
    const auto madeByQt = [](QAction* item) {
        const auto* menu = qobject_cast<const QMenu*>(item->parent());
        if (menu != nullptr &&
            (item == menu->menuAction() || item->associatedObjects().isEmpty())) {
            return true;
        }
        // Like QMenu::menuAction(), Qt owns the toolbar visibility control;
        // it is not an application command declared in ActionRegistry.
        const auto* toolbar = qobject_cast<const QToolBar*>(item->parent());
        return toolbar != nullptr && item == toolbar->toggleViewAction();
    };
    const QList<QAction*> found = window().findChildren<QAction*>();
    qsizetype counted = 0;
    for (QAction* item : found) {
        if (madeByQt(item)) {
            continue;
        }
        ++counted;
        EXPECT_EQ(registry().action(item->objectName()), item)
            << "not in the registry: \"" << item->objectName().toStdString() << "\" ("
            << item->text().toStdString() << ")";
    }
    EXPECT_EQ(counted, registry().actions().size());
}

TEST_F(MainWindowActionsTest, MenusComeFromTheRegistryInTheClassicOrder) {
    EXPECT_EQ(
        titles(window().menuBar()->actions()),
        (QStringList{QStringLiteral("&File"), QStringLiteral("&Edit"), QStringLiteral("&View"),
                     QStringLiteral("&Tools"), QStringLiteral("&Draw"), QStringLiteral("&Help")}));
    const QList<QAction*> menus = window().menuBar()->actions();
    EXPECT_EQ(titles(menus[1]->menu()->actions()),
              (QStringList{QStringLiteral("&Undo"), QStringLiteral("&Redo"), QStringLiteral("-"),
                           QStringLiteral("&Erase")}));
    EXPECT_EQ(menus[4]->menu()->actions(),
              (QList<QAction*>{action("draw.line"), action("draw.pline"), action("draw.circle"),
                               action("draw.arc"), action("draw.text")}));
    for (QAction* item : registry().actions()) {
        const ActionSpec* spec = registry().spec(item->objectName());
        ASSERT_NE(spec, nullptr);
        EXPECT_EQ(spec->environments, QStringList{QStringLiteral("drawing2d")})
            << spec->id.toStdString();
        EXPECT_EQ(item->isCheckable(), spec->checkable) << spec->id.toStdString();
    }
}

TEST_F(MainWindowActionsTest, EveryActionWithACommandRunsItAsTyped) {
    window().setScriptMode();
    io::DiskFiles files;
    window().setFileAccess(&files);
    // ERASE first, while the sample drawing is there to select from; NEW
    // replaces it.
    QList<QAction*> withCommands{action("edit.erase")};
    for (QAction* item : registry().actions()) {
        if (!registry().spec(item->objectName())->command.isEmpty() &&
            item != withCommands.first()) {
            withCommands.append(item);
        }
    }
    EXPECT_EQ(withCommands.size(), 10);  // 4 of File, ERASE, 5 of Draw
    for (QAction* item : withCommands) {
        const ActionSpec* spec = registry().spec(item->objectName());
        SCOPED_TRACE(spec->id.toStdString());
        if (spec->id == QStringLiteral("edit.erase")) {
            click(kOutline);  // ERASE is enabled by a selection
        }
        ASSERT_TRUE(item->isEnabled());
        const QString echo = QStringLiteral("Command: %1").arg(spec->command);
        const qsizetype before = linesEqualTo(echo);
        item->trigger();
        EXPECT_EQ(linesEqualTo(echo), before + 1);
        events::press(input(), Qt::Key_Escape);
        // A NEW or OPEN of a changed drawing first asks about saving it.
        events::press(input(), Qt::Key_Escape);
        ASSERT_FALSE(window().runner().isActive());
    }
}

TEST_F(MainWindowActionsTest, ShortcutsAreTheSameAsBeforeTheRegistry) {
    const auto single = [](const QKeySequence& key) {
        return key.isEmpty() ? QList<QKeySequence>{} : QList<QKeySequence>{key};
    };
    QList<QKeySequence> saveAs = QKeySequence::keyBindings(QKeySequence::SaveAs);
    if (!saveAs.contains(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S))) {
        saveAs.append(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    }
    QList<QKeySequence> redo = QKeySequence::keyBindings(QKeySequence::Redo);
    if (!redo.contains(QKeySequence(Qt::CTRL | Qt::Key_Y))) {
        redo.append(QKeySequence(Qt::CTRL | Qt::Key_Y));
    }
    const QList<std::pair<const char*, QList<QKeySequence>>> expected{
        {"file.new", QKeySequence::keyBindings(QKeySequence::New)},
        {"file.open", QKeySequence::keyBindings(QKeySequence::Open)},
        {"file.save", QKeySequence::keyBindings(QKeySequence::Save)},
        {"file.saveas", saveAs},
        {"file.quit", single(QKeySequence(QKeySequence::Quit))},
        {"edit.undo", QKeySequence::keyBindings(QKeySequence::Undo)},
        {"edit.redo", redo},
        {"edit.erase", single(QKeySequence(QKeySequence::Delete))},
        {"view.zoom-extents", {QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E)}},
        {"aids.osnap", {QKeySequence(Qt::Key_F3)}},
        {"aids.ortho", {QKeySequence(Qt::Key_F8)}},
    };
    QSet<QString> withKeys;
    for (const auto& [id, keys] : expected) {
        SCOPED_TRACE(id);
        ASSERT_NE(action(id), nullptr);
        EXPECT_EQ(action(id)->shortcuts(), keys);
        EXPECT_EQ(registry().spec(QString::fromLatin1(id))->shortcut, keys.value(0));
        withKeys.insert(QString::fromLatin1(id));
    }
    for (QAction* item : registry().actions()) {
        if (!withKeys.contains(item->objectName())) {
            EXPECT_TRUE(item->shortcuts().isEmpty()) << item->objectName().toStdString();
        }
    }
}

TEST_F(MainWindowActionsTest, ZoomExtentsKeyTriggersItsAction) {
    const QSignalSpy triggered(action("view.zoom-extents"), &QAction::triggered);
    ASSERT_TRUE(triggered.isValid());
    window().view()->setFocus();
    QTest::keyPress(window().view(), Qt::Key_E, Qt::ControlModifier | Qt::ShiftModifier);
    EXPECT_EQ(triggered.count(), 1);
}

}  // namespace
}  // namespace tg::app
