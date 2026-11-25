// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The editor of a text in the window and the double click
// (docs/tappe/0.8.1/PT12-editor.md).

#include "text_editor_overlay.hpp"

#include "cad_view.hpp"
#include "command_line.hpp"
#include "document_session.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/document.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/text.hpp"

#include <QCoreApplication>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPoint>
#include <QPointF>
#include <QWheelEvent>
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::app {
namespace {

using Inputs = std::vector<cmd::Input>;

// The drawing of the tests: a TEXT and three MTEXT, one per case.
struct Drawing {
    core::Document document = core::Document::newDrawing();
    core::EntityId text;
    core::EntityId hiddenCode;  // "A\A1;B"
    core::EntityId empty;       // ""
    core::EntityId fraction;    // "x\S1/2;y"
};

core::MText mtextAt(core::Vec2 insertion, std::string value) {
    core::MText mtext;
    mtext.insertion = insertion;
    mtext.width = 60.0;
    mtext.value = std::move(value);
    return mtext;
}

Drawing drawing() {
    Drawing result;
    core::Text text;
    text.position = {0.0, 0.0};
    text.height = 2.5;
    text.value = "Primo";
    result.text = result.document.add(text);
    result.hiddenCode = result.document.add(mtextAt({0.0, -10.0}, "A\\A1;B"));
    result.empty = result.document.add(mtextAt({0.0, -20.0}, ""));
    result.fraction = result.document.add(mtextAt({0.0, -30.0}, "x\\S1/2;y"));
    return result;
}

class TextEditorTest : public testing::Test {
protected:
    TextEditorTest() {
        cmd::registerBuiltinCommands(m_registry);
        m_view.resize(kViewWidth, kViewHeight);
        m_view.show();
        m_view.setSession(&m_session);
        // 10 pixels per drawing unit.
        m_view.setView({20.0, -20.0}, 0.1);
    }

    [[nodiscard]] cmd::CommandRunner& runner() { return m_session.runner(); }

    [[nodiscard]] const core::Document& document() const { return m_session.document(); }

    [[nodiscard]] CadView& view() { return m_view; }

    [[nodiscard]] DocumentSession& session() { return m_session; }

    [[nodiscard]] TextEditorOverlay* editor() const { return m_view.textEditor(); }

    [[nodiscard]] const Drawing& ids() const { return m_ids; }

    /// The request the editor is open on; an empty one when it is closed.
    [[nodiscard]] cmd::TextEditRequest request() const {
        return editor()->request().value_or(cmd::TextEditRequest{});
    }

    /// A line of the command line, and the refresh of the main window after it.
    void enter(const std::string& text) {
        runner().submitText(text);
        m_view.refreshTextEditor();
    }

    /// TEXTEDIT on `id`, as the double click starts it.
    void edit(core::EntityId id) {
        ASSERT_TRUE(runner().replaceSelection({id}));
        enter("TEXTEDIT");
        ASSERT_TRUE(editor()->isOpen());
    }

    /// The end of a TEXTEDIT in Multiple mode.
    void endTextedit() {
        enter("");
        ASSERT_FALSE(runner().isActive());
    }

    void confirm() const { events::press(editor(), Qt::Key_Return, Qt::ControlModifier); }

    void selectAll() const { events::press(editor(), Qt::Key_A, Qt::ControlModifier); }

    /// A linear dimension 30 long whose text override is `override`.
    [[nodiscard]] core::EntityId dimension(const std::string& override) {
        enter("DIMLINEAR");
        enter("0,-60");
        enter("30,-60");
        enter("15,-65");
        const core::EntityId id = document().entities().back().id;
        EXPECT_TRUE(std::holds_alternative<core::Dimension>(document().entities().back().entity));
        EXPECT_TRUE(runner().replaceSelection({id}));
        runner().submitText("TEXTEDIT");
        runner().submitText(override);
        runner().submitText("");
        return id;
    }

    [[nodiscard]] const core::Entity& entity(core::EntityId id) const {
        return document().find(id)->entity;
    }

    [[nodiscard]] std::string textOverride(core::EntityId id) const {
        return std::get<core::Dimension>(entity(id)).presentation.textOverride;
    }

    [[nodiscard]] QPoint screen(const core::Vec2& point) const {
        const core::Vec2 at = m_view.viewport().worldToScreen(point);
        return {static_cast<int>(std::lround(at.x)), static_cast<int>(std::lround(at.y))};
    }

private:
    static constexpr int kViewWidth = 800;
    static constexpr int kViewHeight = 600;

    // The runner looks commands up when they start: registered in the
    // constructor, after the session.
    cmd::CommandRegistry m_registry;
    Drawing m_ids = drawing();
    DocumentSession m_session{m_registry, nullptr, std::move(m_ids.document)};
    CadView m_view;
};

TEST_F(TextEditorTest, WritingDeletingAndBreakingANewMTextGivesItsValue) {
    enter("MTEXT");
    enter("0,-80");
    enter("40,-90");
    ASSERT_TRUE(editor()->isOpen());
    EXPECT_FALSE(request().target.has_value());
    ASSERT_NE(editor()->model(), nullptr);

    events::type(editor(), "AB");
    events::press(editor(), Qt::Key_Backspace);
    events::press(editor(), Qt::Key_Return);
    events::type(editor(), "C");
    EXPECT_EQ(editor()->value(), "A\\PC");
    // The value on one line, then the empty line that ends MTEXT.
    EXPECT_EQ(editor()->confirmation(), (Inputs{cmd::TextInput{"A\\PC"}, cmd::ConfirmInput{}}));

    const std::size_t before = document().entityCount();
    confirm();
    EXPECT_FALSE(editor()->isOpen());
    EXPECT_FALSE(runner().isActive());
    ASSERT_EQ(document().entityCount(), before + 1);
    EXPECT_EQ(std::get<core::MText>(document().entities().back().entity).value, "A\\PC");
}

TEST_F(TextEditorTest, EscapeLeavesTheDrawingAsItWas) {
    edit(ids().text);
    ASSERT_NE(editor()->lineModel(), nullptr);
    events::type(editor(), "X");
    EXPECT_EQ(editor()->value(), "PrimoX");
    events::press(editor(), Qt::Key_Escape);
    EXPECT_FALSE(editor()->isOpen());
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(std::get<core::Text>(entity(ids().text)).value, "Primo");
}

TEST_F(TextEditorTest, EnterConfirmsATextOfOneLine) {
    edit(ids().text);
    // A TEXT has no codes: a backslash is a character.
    events::type(editor(), "\\P");
    events::press(editor(), Qt::Key_Return);
    EXPECT_FALSE(editor()->isOpen());
    EXPECT_EQ(std::get<core::Text>(entity(ids().text)).value, "Primo\\P");
    endTextedit();
}

TEST_F(TextEditorTest, ATextEmptiedStaysAsItWas) {
    edit(ids().text);
    selectAll();
    events::press(editor(), Qt::Key_Delete);
    EXPECT_EQ(editor()->value(), "");
    EXPECT_EQ(editor()->confirmation(), Inputs{cmd::ConfirmInput{}});
    confirm();
    EXPECT_EQ(std::get<core::Text>(entity(ids().text)).value, "Primo");
    endTextedit();
}

TEST_F(TextEditorTest, DoubleClickOnATextStartsTexteditOnIt) {
    const QPoint at = screen({1.0, 1.0});
    // The first click of the two selects the text, as any click does.
    events::click(&view(), Qt::LeftButton, at);
    const QPointF local(at);
    QMouseEvent doubleClick(QEvent::MouseButtonDblClick, local, view().mapToGlobal(local),
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&view(), &doubleClick);

    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(runner().promptText(), "Enter new text <Primo>:");
    ASSERT_TRUE(editor()->isOpen());
    EXPECT_EQ(request().target, ids().text);
    EXPECT_EQ(editor()->value(), "Primo");
}

TEST_F(TextEditorTest, DoubleClickInTheVoidStartsNothing) {
    const QPoint at = screen({50.0, 0.0});
    events::click(&view(), Qt::LeftButton, at);
    const QPointF local(at);
    QMouseEvent doubleClick(QEvent::MouseButtonDblClick, local, view().mapToGlobal(local),
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&view(), &doubleClick);
    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(editor()->isOpen());
}

TEST_F(TextEditorTest, ZoomWithTheEditorOpenKeepsItOverTheText) {
    edit(ids().text);
    const double scale = view().viewport().scale();
    // Towards a point away from the text, which then moves on the screen.
    const QPointF anchor(screen({40.0, -40.0}));
    QWheelEvent wheel(anchor, view().mapToGlobal(anchor), {}, {0, 120}, Qt::NoButton,
                      Qt::NoModifier, Qt::NoScrollPhase, false);
    QCoreApplication::sendEvent(editor(), &wheel);
    EXPECT_GT(view().viewport().scale(), scale);
    ASSERT_TRUE(editor()->isOpen());

    // A click where the text is now falls inside the border: the cursor
    // moves there, before the end of "Primo".
    events::click(editor(), Qt::LeftButton, screen({2.0, 1.0}));
    ASSERT_TRUE(editor()->isOpen());
    ASSERT_NE(editor()->lineModel(), nullptr);
    EXPECT_LT(editor()->lineModel()->caret(), 5U);

    // A click outside confirms: nothing changed, the value is kept.
    events::click(editor(), Qt::LeftButton, screen({40.0, -40.0}));
    EXPECT_FALSE(editor()->isOpen());
    EXPECT_EQ(std::get<core::Text>(entity(ids().text)).value, "Primo");
    endTextedit();
}

TEST_F(TextEditorTest, TheMeasurementOfADimensionIsShownAndKept) {
    const core::EntityId id = dimension("<> mm");
    ASSERT_EQ(textOverride(id), "<> mm");
    edit(id);
    const cmd::TextEditRequest opened = request();
    EXPECT_EQ(opened.target, id);
    EXPECT_EQ(opened.value, "<> mm");
    ASSERT_TRUE(opened.measurement.has_value());
    const std::string measurement = opened.measurement.value_or("");
    ASSERT_NE(editor()->model(), nullptr);
    // The measurement as the drawing shows it, one unit: two positions
    // around it, then those of " mm".
    EXPECT_EQ(editor()->model()->shown().value, measurement + " mm");
    EXPECT_EQ(editor()->model()->positionCount(), 5U);

    EXPECT_EQ(editor()->confirmation(), Inputs{cmd::ConfirmInput{}});
    confirm();
    EXPECT_EQ(textOverride(id), "<> mm");
    endTextedit();

    // Emptied, back to the measurement.
    edit(id);
    selectAll();
    events::press(editor(), Qt::Key_Delete);
    EXPECT_TRUE(editor()->model()->isEmpty());
    EXPECT_EQ(editor()->confirmation(), Inputs{cmd::TextInput{""}});
    confirm();
    EXPECT_EQ(textOverride(id), "");
    endTextedit();
}

TEST_F(TextEditorTest, ADimensionEmptiedWithAHiddenCodeGoesBackToTheMeasurement) {
    const core::EntityId id = dimension("<>\\A1; mm");
    edit(id);
    selectAll();
    events::press(editor(), Qt::Key_Delete);
    EXPECT_EQ(editor()->value(), "\\A1;");
    EXPECT_EQ(editor()->confirmation(), Inputs{cmd::TextInput{""}});
    confirm();
    EXPECT_EQ(textOverride(id), "");
    endTextedit();
}

TEST_F(TextEditorTest, AnMTextEmptiedWithAHiddenCodeStaysAsItWas) {
    edit(ids().hiddenCode);
    events::press(editor(), Qt::Key_Backspace);
    events::press(editor(), Qt::Key_Backspace);
    // The code stays, nothing shows: emptied.
    EXPECT_EQ(editor()->value(), "\\A1;");
    EXPECT_EQ(editor()->confirmation(), Inputs{cmd::ConfirmInput{}});
    confirm();
    EXPECT_EQ(std::get<core::MText>(entity(ids().hiddenCode)).value, "A\\A1;B");
    endTextedit();
}

TEST_F(TextEditorTest, AnEmptyMTextConfirmedUnchangedStays) {
    edit(ids().empty);
    EXPECT_EQ(editor()->confirmation(), Inputs{cmd::ConfirmInput{}});
    confirm();
    ASSERT_NE(document().find(ids().empty), nullptr);
    EXPECT_EQ(std::get<core::MText>(entity(ids().empty)).value, "");
    endTextedit();
}

TEST_F(TextEditorTest, AFractionIsSkippedAndDeletedWhole) {
    edit(ids().fraction);
    ASSERT_EQ(editor()->model()->positionCount(), 4U);
    EXPECT_EQ(editor()->model()->caret(), 3U);
    events::press(editor(), Qt::Key_Left);
    events::press(editor(), Qt::Key_Left);
    // Before the fraction, never inside it.
    EXPECT_EQ(editor()->model()->caret(), 1U);
    events::press(editor(), Qt::Key_Delete);
    EXPECT_EQ(editor()->value(), "xy");
    confirm();
    EXPECT_EQ(std::get<core::MText>(entity(ids().fraction)).value, "xy");
    endTextedit();
}

TEST_F(TextEditorTest, TheEditorFollowsTheRequestOfTheSessionShown) {
    edit(ids().text);
    view().setSession(nullptr);
    EXPECT_FALSE(editor()->isOpen());
    // Back to the session, the editor opens again on its request.
    view().setSession(&session());
    EXPECT_TRUE(editor()->isOpen());
    view().setTextEditorEnabled(false);
    EXPECT_FALSE(editor()->isOpen());
}

// The main window opens the editor after an input of the command line, and
// never in a script session (docs/adr/0054, point 5).
TEST(TextEditorWindowTest, TheCommandLineOpensItButNotInAScript) {
    const auto startMText = [](MainWindow& window) {
        window.show();
        auto* input = window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
        ASSERT_NE(input, nullptr);
        const std::array<std::string, 3> lines{"MTEXT", "0,0", "20,-10"};
        for (const std::string& line : lines) {
            events::type(input, line);
            events::press(input, Qt::Key_Return);
        }
    };
    MainWindow window;
    startMText(window);
    EXPECT_TRUE(window.view()->textEditor()->isOpen());

    MainWindow script;
    script.setScriptMode();
    startMText(script);
    EXPECT_FALSE(script.view()->textEditor()->isOpen());
}

}  // namespace
}  // namespace tg::app
