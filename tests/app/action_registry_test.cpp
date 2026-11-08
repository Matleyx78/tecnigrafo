// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "action_registry.hpp"

#include "cad_view.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "layer_panel.hpp"
#include "main_window.hpp"
#include "properties_panel.hpp"
#include "tg/io/file_access.hpp"

#include <QAction>
#include <QDockWidget>
#include <QIcon>
#include <QKeySequence>
#include <QLineEdit>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPointF>
#include <QSet>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTest>
#include <QToolBar>
#include <QWidgetAction>
#include <gtest/gtest.h>

#include <string>
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

/// An icon of one colour, to tell icons apart by their cache key.
QIcon plainIcon(Qt::GlobalColor colour) {
    QPixmap pixmap(16, 16);
    pixmap.fill(colour);
    return QIcon(pixmap);
}

TEST(ActionRegistryTest, VariantsFormTheMenuOfTheirActionWhichRemembersTheLastUsed) {
    ActionRegistry registry;
    QAction* circle = registry.add(
        spec(QStringLiteral("draw.circle"), QStringLiteral("&Circle"), QStringLiteral("&Draw")));
    EXPECT_EQ(circle->menu(), nullptr);
    EXPECT_TRUE(registry.variants(QStringLiteral("draw.circle")).isEmpty());
    EXPECT_EQ(registry.currentVariant(QStringLiteral("draw.circle")), nullptr);

    ActionSpec radius = spec(QStringLiteral("draw.circle.center-radius"), QStringLiteral("Radius"));
    radius.variantOf = QStringLiteral("draw.circle");
    ActionSpec diameter =
        spec(QStringLiteral("draw.circle.center-diameter"), QStringLiteral("Diameter"));
    diameter.variantOf = QStringLiteral("draw.circle");
    diameter.description = QStringLiteral("By its diameter.");
    diameter.responses = {QString(kWaitForUser), QStringLiteral("D")};
    QAction* first = registry.add(radius);
    QAction* second = registry.add(diameter);
    first->setIcon(plainIcon(Qt::red));
    second->setIcon(plainIcon(Qt::blue));

    const QList<QAction*> variants{first, second};
    EXPECT_EQ(registry.variants(QStringLiteral("draw.circle")), variants);
    ASSERT_NE(circle->menu(), nullptr);
    EXPECT_EQ(circle->menu()->actions(), variants);
    EXPECT_EQ(registry.spec(QStringLiteral("draw.circle.center-diameter"))->responses,
              (QStringList{QStringLiteral("\\"), QStringLiteral("D")}));
    // The first variant until another is used, with its icon.
    EXPECT_EQ(registry.currentVariant(QStringLiteral("draw.circle")), first);
    EXPECT_EQ(circle->icon().cacheKey(), first->icon().cacheKey());

    second->trigger();
    EXPECT_EQ(registry.currentVariant(QStringLiteral("draw.circle")), second);
    EXPECT_EQ(circle->icon().cacheKey(), second->icon().cacheKey());
    EXPECT_EQ(circle->toolTip(), QStringLiteral("By its diameter."));
    // A new icon of the current variant, as a new theme gives, follows.
    second->setIcon(plainIcon(Qt::green));
    EXPECT_EQ(circle->icon().cacheKey(), second->icon().cacheKey());

    // In the classic menus the action is the submenu of its variants.
    QMenuBar bar;
    registry.buildMenus(&bar);
    ASSERT_EQ(bar.actions().size(), 1);
    QMenu* draw = bar.actions().first()->menu();
    ASSERT_NE(draw, nullptr);
    EXPECT_EQ(draw->actions(), QList<QAction*>{circle});
    EXPECT_EQ(titles(circle->menu()->actions()),
              (QStringList{QStringLiteral("Radius"), QStringLiteral("Diameter")}));
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

    /// How many lines of the history end with `end`.
    [[nodiscard]] qsizetype linesEndingWith(const QString& end) const {
        const QString history =
            m_window.findChild<QPlainTextEdit*>(QStringLiteral("history"))->toPlainText();
        qsizetype count = 0;
        for (const QString& line : history.split(QLatin1Char('\n'))) {
            count += line.endsWith(end) ? 1 : 0;
        }
        return count;
    }

    /// Types `text` in the command line and presses Enter.
    void enter(const std::string& text) {
        events::type(input(), text);
        events::press(input(), Qt::Key_Return);
    }

    [[nodiscard]] std::string prompt() const { return m_window.runner().promptText(); }

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
    const auto madeByFramework = [](QAction* item) {
        const auto* menu = qobject_cast<const QMenu*>(item->parent());
        if (menu != nullptr &&
            (item == menu->menuAction() || item->associatedObjects().isEmpty())) {
            return true;
        }
        // Like QMenu::menuAction(), Qt owns the toolbar visibility control;
        // it is not an application command declared in ActionRegistry.
        const auto* toolbar = qobject_cast<const QToolBar*>(item->parent());
        if (toolbar != nullptr && item == toolbar->toggleViewAction()) {
            return true;
        }
        // The same for a dock widget: Qt owns the action that shows or hides
        // the layers panel (the registry has layer.panel for that).
        const auto* dock = qobject_cast<const QDockWidget*>(item->parent());
        if (dock != nullptr && item == dock->toggleViewAction()) {
            return true;
        }
        // The ribbon and the toolbar each hold the layer controls and the
        // property controls in a QWidgetAction made by the toolkit; the
        // actions of the two panels are in the registry.
        const auto* widgetAction = qobject_cast<const QWidgetAction*>(item);
        if (widgetAction != nullptr &&
            (qobject_cast<const LayerControls*>(widgetAction->defaultWidget()) != nullptr ||
             qobject_cast<const PropertyControls*>(widgetAction->defaultWidget()) != nullptr)) {
            return true;
        }
        // SARibbon also supplies native-style window controls, including
        // two QMenu-owned separators. These belong to its title icon's
        // system menu, not to the application's command registry. Inspect
        // the Qt metaobject to keep SARibbon headers confined to UiBuilder.
        const QObject* owner = menu != nullptr ? menu->parent() : item->parent();
        const bool systemMenu = owner != nullptr && owner->inherits("SARibbonTitleIconWidget");
        return systemMenu && item->objectName().isEmpty();
    };
    const QList<QAction*> found = window().findChildren<QAction*>();
    qsizetype counted = 0;
    for (QAction* item : found) {
        if (madeByFramework(item)) {
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
    // 4 of File, ERASE, 5 of Draw and the variants of CIRCLE (5) and ARC (2).
    EXPECT_EQ(withCommands.size(), 17);
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

TEST_F(MainWindowActionsTest, AVariantWaitsForThePointThenGivesItsResponse) {
    window().setScriptMode();
    const QString diameter = QStringLiteral(": D");
    action("draw.circle.center-diameter")->trigger();
    EXPECT_EQ(linesEqualTo(QStringLiteral("Command: CIRCLE")), 1);
    ASSERT_TRUE(window().runner().isActive());
    // "\": the variant waits for the centre, which only the user gives.
    EXPECT_TRUE(prompt().starts_with("Specify center point for circle")) << prompt();
    EXPECT_EQ(linesEndingWith(diameter), 0);
    // An answer that the prompt rejects is no answer: still waiting.
    enter("nonsense");
    EXPECT_TRUE(prompt().starts_with("Specify center point for circle")) << prompt();
    EXPECT_EQ(linesEndingWith(diameter), 0);

    click({100.0, 40.0});
    // Then the response, shown in the history as if typed.
    EXPECT_TRUE(prompt().starts_with("Specify diameter of circle")) << prompt();
    EXPECT_EQ(linesEndingWith(diameter), 1);
    enter("20");
    EXPECT_FALSE(window().runner().isActive());
    // Nothing is left to give to the next command.
    enter("CIRCLE");
    click({110.0, 40.0});
    EXPECT_TRUE(prompt().starts_with("Specify radius of circle")) << prompt();
    EXPECT_EQ(linesEndingWith(diameter), 1);
    events::press(input(), Qt::Key_Escape);
}

TEST_F(MainWindowActionsTest, ACommandThatEndsOrAsksSomethingElseFirstDropsTheResponsesLeft) {
    window().setScriptMode();
    const QString diameter = QStringLiteral(": D");
    // Esc ends CIRCLE while the variant waits for the centre.
    action("draw.circle.center-diameter")->trigger();
    ASSERT_TRUE(window().runner().isActive());
    events::press(input(), Qt::Key_Escape);
    ASSERT_FALSE(window().runner().isActive());
    // The next command, typed, does not receive the "D" left.
    enter("CIRCLE");
    click({100.0, 40.0});
    EXPECT_TRUE(prompt().starts_with("Specify radius of circle")) << prompt();
    EXPECT_EQ(linesEndingWith(diameter), 0);
    events::press(input(), Qt::Key_Escape);

    // An option instead of the centre: CIRCLE asks for something else,
    // which "D" does not answer, so it is dropped as well.
    action("draw.circle.center-diameter")->trigger();
    enter("2P");
    EXPECT_EQ(prompt(), "Specify first end point of circle's diameter:");
    click({100.0, 40.0});
    EXPECT_EQ(prompt(), "Specify second end point of circle's diameter:");
    EXPECT_EQ(linesEndingWith(diameter), 0);
    events::press(input(), Qt::Key_Escape);

    // ARC, Start, Center, End: the start from the user, then "C".
    action("draw.arc.start-center-end")->trigger();
    click({100.0, 40.0});
    EXPECT_EQ(prompt(), "Specify center point of arc:");
    EXPECT_EQ(linesEndingWith(QStringLiteral(": C")), 1);
    events::press(input(), Qt::Key_Escape);
}

TEST_F(MainWindowActionsTest, VariantsWithoutAWaitGiveTheirResponsesAtOnce) {
    window().setScriptMode();
    const QList<std::pair<const char*, std::string>> expected{
        {"draw.circle.center-radius",
         "Specify center point for circle or [3P/2P/Ttr (tan tan "
         "radius)]:"},
        {"draw.circle.2-point", "Specify first end point of circle's diameter:"},
        {"draw.circle.3-point", "Specify first point on circle:"},
        {"draw.circle.tan-tan-radius", "Specify point on object for first tangent of circle:"},
        {"draw.arc.3-point", "Specify start point of arc:"}};
    for (const auto& [id, first] : expected) {
        SCOPED_TRACE(id);
        action(id)->trigger();
        EXPECT_EQ(prompt(), first);
        events::press(input(), Qt::Key_Escape);
        ASSERT_FALSE(window().runner().isActive());
    }
}

TEST_F(MainWindowActionsTest, ShortcutsMatchTheBuiltinDefaults) {
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
        {"aids.polar", {QKeySequence(Qt::Key_F10)}},
        {"aids.dynmode", {QKeySequence(Qt::Key_F12)}},
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
