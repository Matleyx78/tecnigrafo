// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "cad_view.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "tg/cmd/builtin_commands.hpp"

#include <QAction>
#include <QLineEdit>
#include <QMenu>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <variant>

namespace tg::app {
namespace {

class TextInputTest : public testing::Test {
protected:
    TextInputTest() {
        cmd::registerBuiltinCommands(m_registry);
        auto* layout = new QVBoxLayout(&m_window);
        layout->addWidget(m_view);
        layout->addWidget(m_commandLine);
        m_view->setFocusPolicy(Qt::StrongFocus);
        m_commandLine->forwardKeysFrom(m_view);
        m_window.show();
    }

    [[nodiscard]] QLineEdit* input() const { return m_commandLine->findChild<QLineEdit*>("input"); }

    [[nodiscard]] QWidget* view() const { return m_view; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] cmd::CommandRunner& runner() { return m_runner; }

    void enter(const std::string& text, Qt::Key key = Qt::Key_Return) const {
        events::type(input(), text);
        events::press(input(), key);
    }

    void startText() const {
        enter("TEXT");
        enter("0,0");
        enter("");
        enter("");
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    cmd::CommandRegistry m_registry;
    cmd::CommandRunner m_runner{m_registry, m_editor};
    QWidget m_window;
    QWidget* m_view{new QWidget(&m_window)};
    CommandLine* m_commandLine{new CommandLine(m_runner, &m_window)};
};

TEST_F(TextInputTest, SpacesAreLiteralWhileTextIsRequested) {
    startText();
    ASSERT_TRUE(runner().acceptsText());
    events::type(input(), "  Hello world  ");
    EXPECT_TRUE(document().isEmpty());
    EXPECT_EQ(input()->text().toStdString(), "  Hello world  ");
    events::press(input(), Qt::Key_Return);
    enter("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(std::get<core::Text>(document().entities().front().entity).value, "  Hello world  ");
    enter("U");
    EXPECT_TRUE(document().isEmpty());
    enter("REDO");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(std::get<core::Text>(document().entities().front().entity).value, "  Hello world  ");
}

TEST_F(TextInputTest, UnicodeAndSpacesFromTheViewReachTheTextField) {
    startText();
    view()->setFocus();
    events::press(view(), Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
    events::press(view(), Qt::Key_unknown, Qt::NoModifier, QString::fromUtf8("Città ☕"));
    events::press(view(), Qt::Key_Space, Qt::NoModifier, QStringLiteral(" "));
    events::type(input(), "%%c");
    EXPECT_TRUE(document().isEmpty());
    EXPECT_EQ(input()->text().toStdString(), " Città ☕ %%c");
    events::press(input(), Qt::Key_Return);
    enter("");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(std::get<core::Text>(document().entities().front().entity).value, " Città ☕ %%c");
}

TEST_F(TextInputTest, SpaceStillSubmitsCommandsAndPointsAndEscapeKeepsConfirmedLines) {
    enter("PL", Qt::Key_Space);
    EXPECT_TRUE(runner().isActive());
    enter("0,0", Qt::Key_Space);
    enter("10,0", Qt::Key_Space);
    enter("", Qt::Key_Space);
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(std::get<core::Polyline>(document().entities().front().entity).vertexCount(), 2U);
    startText();
    enter("First line");
    events::type(input(), "unfinished line");
    events::press(input(), Qt::Key_Escape);
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(input()->text().isEmpty());
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(std::get<core::Text>(document().entities().back().entity).value, "First line");
    enter("U");
    EXPECT_EQ(document().entityCount(), 1U);
}

// The text of the sample drawing: its insertion point, and a point inside
// its box, away from the other entities.
constexpr core::Vec2 kSampleTextInsertion{130.0, 72.0};
constexpr core::Vec2 kInsideSampleText{135.0, 73.0};

class InsertionSnapMenuTest : public testing::Test {
protected:
    void SetUp() override {
        m_window.show();
        m_window.activateWindow();
        ASSERT_TRUE(QTest::qWaitForWindowActive(&m_window));
        ASSERT_NE(insertion(), nullptr);
    }

    [[nodiscard]] QAction* insertion() const {
        return m_window.findChild<QAction*>(QStringLiteral("insertionSnapAction"));
    }

    [[nodiscard]] QAction* objectSnap() const {
        return m_window.findChild<QAction*>(QStringLiteral("objectSnapAction"));
    }

    [[nodiscard]] const cmd::DrawingAids& aids() const { return m_window.drawingAids(); }

    [[nodiscard]] const cmd::CommandRunner& runner() const { return m_window.runner(); }

    /// PL typed in the command line, then the pointer over `world`.
    void startPolylineWithPointerAt(const core::Vec2& world) const {
        QLineEdit* input = m_window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
        events::type(input, "PL");
        events::press(input, Qt::Key_Return);
        const core::Vec2 pixel = m_window.view()->viewport().worldToScreen(world);
        events::move(m_window.view(), QPointF{pixel.x, pixel.y}.toPoint());
    }

private:
    MainWindow m_window;
};

TEST_F(InsertionSnapMenuTest, IsAnObjectSnapModeCheckedAtStart) {
    const QList<QObject*> menus = insertion()->associatedObjects();
    const bool inModesMenu = std::ranges::any_of(menus, [](const QObject* object) {
        const auto* menu = qobject_cast<const QMenu*>(object);
        return menu != nullptr && menu->title() == QStringLiteral("Object Snap &Modes");
    });
    EXPECT_TRUE(inModesMenu);
    EXPECT_TRUE(insertion()->isCheckable());
    EXPECT_TRUE(insertion()->isChecked());
    EXPECT_TRUE(aids().snapModes.insertion);
}

TEST_F(InsertionSnapMenuTest, TogglesTheInsertionSnapOfTheRunner) {
    startPolylineWithPointerAt(kInsideSampleText);
    ASSERT_TRUE(runner().activeSnap().has_value());
    EXPECT_EQ(runner().activeSnap()->kind, core::SnapKind::Insertion);
    EXPECT_EQ(runner().activeSnap()->point, kSampleTextInsertion);

    insertion()->trigger();
    EXPECT_FALSE(insertion()->isChecked());
    EXPECT_FALSE(aids().snapModes.insertion);
    // Only that mode changes.
    EXPECT_TRUE(aids().objectSnap);
    EXPECT_TRUE(aids().snapModes.endpoint);
    EXPECT_TRUE(aids().snapModes.midpoint);
    EXPECT_TRUE(aids().snapModes.center);
    EXPECT_FALSE(runner().activeSnap().has_value());

    insertion()->trigger();
    EXPECT_TRUE(insertion()->isChecked());
    EXPECT_EQ(aids(), cmd::DrawingAids{});
    ASSERT_TRUE(runner().activeSnap().has_value());
    EXPECT_EQ(runner().activeSnap()->kind, core::SnapKind::Insertion);
}

TEST_F(InsertionSnapMenuTest, ObjectSnapOffHidesTheInsertionSnap) {
    startPolylineWithPointerAt(kInsideSampleText);
    objectSnap()->trigger();
    EXPECT_TRUE(insertion()->isChecked());  // the mode stays chosen
    EXPECT_FALSE(runner().activeSnap().has_value());
}

}  // namespace
}  // namespace tg::app
