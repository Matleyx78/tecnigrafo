// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Integration of view, command line, commands and undo in the main window.

#include "cad_view.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "tg/core/document.hpp"

#include <QAction>
#include <QLineEdit>
#include <QPoint>
#include <QString>
#include <QTest>
#include <gtest/gtest.h>

#include <cstddef>
#include <variant>

namespace tg::app {
namespace {

using events::click;
using events::move;
using events::press;
using events::type;

// Three points well inside the view, in pixels.
constexpr QPoint kFirst{100, 100};
constexpr QPoint kSecond{300, 100};
constexpr QPoint kThird{300, 300};

class DrawingTest : public testing::Test {
protected:
    void SetUp() override {
        m_window.show();
        m_window.activateWindow();
        // Shortcuts only fire in the active window.
        ASSERT_TRUE(QTest::qWaitForWindowActive(&m_window))
            << "the main window did not become active";
    }

    [[nodiscard]] const core::Document& document() const { return m_window.document(); }

    [[nodiscard]] CadView* view() const { return m_window.view(); }

    [[nodiscard]] QLineEdit* input() const {
        return m_window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    }

    [[nodiscard]] QAction* action(const char* name) const {
        return m_window.findChild<QAction*>(QString::fromLatin1(name));
    }

    /// Identifier of the most recently added entity.
    [[nodiscard]] core::EntityId lastId() const { return document().entities().back().id; }

    /// PL, three clicks in the view, Enter: as the user does it.
    void drawPolyline() {
        type(view(), "PL");
        press(view(), Qt::Key_Return);
        click(view(), Qt::LeftButton, kFirst);
        click(view(), Qt::LeftButton, kSecond);
        click(view(), Qt::LeftButton, kThird);
        press(view(), Qt::Key_Return);
    }

    /// A shortcut typed with the focus in the command line's input field,
    /// through Qt's shortcut handling.
    void shortcut(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::ControlModifier) {
        input()->setFocus();
        QTest::keyPress(input(), key, modifiers);
    }

private:
    MainWindow m_window;
};

TEST_F(DrawingTest, SampleDrawingCannotBeUndone) {
    EXPECT_FALSE(document().isEmpty());
    EXPECT_FALSE(action("edit.undo")->isEnabled());
    EXPECT_FALSE(action("edit.redo")->isEnabled());
}

TEST_F(DrawingTest, DrawsAPolylineThenUndoesAndRedoesIt) {
    const std::size_t before = document().entityCount();

    drawPolyline();
    ASSERT_EQ(document().entityCount(), before + 1);
    const core::EntityId id = lastId();
    ASSERT_NE(document().find(id), nullptr);
    EXPECT_EQ(std::get<core::Polyline>(document().find(id)->entity).vertexCount(), 3U);

    shortcut(Qt::Key_Z);
    EXPECT_EQ(document().entityCount(), before);
    EXPECT_EQ(document().find(id), nullptr);

    shortcut(Qt::Key_Y);
    EXPECT_EQ(document().entityCount(), before + 1);
    EXPECT_NE(document().find(id), nullptr);
}

TEST_F(DrawingTest, MenuShowsTheStepNameAndFollowsTheStacks) {
    drawPolyline();
    EXPECT_TRUE(action("edit.undo")->isEnabled());
    EXPECT_EQ(action("edit.undo")->text().toStdString(), "&Undo Polyline");
    EXPECT_FALSE(action("edit.redo")->isEnabled());

    action("edit.undo")->trigger();
    EXPECT_FALSE(action("edit.undo")->isEnabled());
    EXPECT_TRUE(action("edit.redo")->isEnabled());
    EXPECT_EQ(action("edit.redo")->text().toStdString(), "&Redo Polyline");
}

TEST_F(DrawingTest, UndoAndRedoAreDisabledWhileACommandRuns) {
    drawPolyline();
    type(view(), "PL");
    press(view(), Qt::Key_Return);
    EXPECT_FALSE(action("edit.undo")->isEnabled());

    click(view(), Qt::LeftButton, kFirst);
    EXPECT_FALSE(action("edit.undo")->isEnabled());

    press(view(), Qt::Key_Escape);
    EXPECT_TRUE(action("edit.undo")->isEnabled());
}

TEST_F(DrawingTest, UndoKeysInTheCommandLineActOnTheDrawing) {
    const std::size_t before = document().entityCount();
    drawPolyline();
    type(input(), "ab");

    shortcut(Qt::Key_Z);

    EXPECT_EQ(document().entityCount(), before);
    EXPECT_EQ(input()->text().toStdString(), "ab");  // the typed text stays
}

TEST_F(DrawingTest, RightClickConfirmsAndThePreviewIsDrawn) {
    const std::size_t before = document().entityCount();
    type(view(), "PL");
    press(view(), Qt::Key_Return);
    click(view(), Qt::LeftButton, kFirst);
    click(view(), Qt::LeftButton, kSecond);
    move(view(), kThird);
    EXPECT_FALSE(view()->grab().isNull());  // paints scene and preview

    click(view(), Qt::RightButton, kThird);

    EXPECT_EQ(document().entityCount(), before + 1);
    EXPECT_TRUE(action("edit.undo")->isEnabled());
}

}  // namespace
}  // namespace tg::app
