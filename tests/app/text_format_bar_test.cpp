// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "text_format_bar.hpp"

#include "input_events.hpp"
#include "text_editor_overlay.hpp"
#include "tg/cmd/text_editing.hpp"
#include "tg/core/document.hpp"
#include "tg/render/viewport.hpp"

#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QToolButton>
#include <gtest/gtest.h>

#include <array>
#include <utility>

namespace tg::app {
namespace {

class TextFormatBarTest : public ::testing::Test {
protected:
    TextFormatBarTest() : m_editor(*new TextEditorOverlay(&m_parent, m_viewport)) {}

    void open(std::string value) {
        core::MText shown;
        shown.value = std::move(value);
        cmd::TextEditRequest request;
        request.shown = shown;
        request.value = shown.value;
        m_editor.open(request, m_document);
        m_bar = m_editor.findChild<TextFormatBar*>(QStringLiteral("textFormatBar"));
        ASSERT_NE(m_bar, nullptr);
    }

    void select(std::size_t anchor, std::size_t caret) {
        ASSERT_LE(anchor, caret);
        events::press(&m_editor, Qt::Key_Home);
        for (std::size_t index = 0; index < anchor; ++index) {
            events::press(&m_editor, Qt::Key_Right);
        }
        for (std::size_t index = anchor; index < caret; ++index) {
            events::press(&m_editor, Qt::Key_Right, Qt::ShiftModifier);
        }
    }

    void click(const char* name) {
        auto* button = m_bar->findChild<QToolButton*>(QString::fromLatin1(name));
        ASSERT_NE(button, nullptr);
        button->click();
    }

    TextEditorOverlay& overlay() { return m_editor; }

    TextFormatBar& formatBar() { return *m_bar; }

    core::Document& drawing() { return m_document; }

private:
    QWidget m_parent;
    render::Viewport m_viewport;
    TextEditorOverlay& m_editor;
    core::Document m_document = core::Document::newDrawing();
    TextFormatBar* m_bar{nullptr};
};

TEST_F(TextFormatBarTest, ToggleButtonsApplyCodesAndFollowCaretFormat) {
    open("AB");
    select(0, 1);
    click("formatBold");
    EXPECT_EQ(overlay().value(), "{\\fArial|b1|i0;A}B");

    events::press(&overlay(), Qt::Key_Right);
    formatBar().refresh();
    EXPECT_TRUE(formatBar().findChild<QToolButton*>(QStringLiteral("formatBold"))->isChecked());
}

TEST_F(TextFormatBarTest, EveryToggleButtonAppliesItsMtextCode) {
    struct Case {
        const char* button;
        const char* expected;
    };

    constexpr std::array kCases = {
        Case{"formatItalic", "{\\fArial|b0|i1;A}B"}, Case{"formatUnderline", "{\\LA}B"},
        Case{"formatOverline", "{\\OA}B"}, Case{"formatStrike", "{\\KA}B"}};
    for (const Case& test : kCases) {
        open("AB");
        select(0, 1);
        click(test.button);
        EXPECT_EQ(overlay().value(), test.expected) << test.button;
        overlay().close();
    }
}

TEST_F(TextFormatBarTest, NumberAndColorFieldsApplyTheirMtextCodes) {
    struct NumberCase {
        const char* field;
        double value;
        const char* expected;
    };

    constexpr std::array kCases = {NumberCase{"formatHeight", 3.5, "{\\H3.5;A}B"},
                                   NumberCase{"formatWidth", 1.5, "{\\W1.5;A}B"},
                                   NumberCase{"formatOblique", 12.0, "{\\Q12;A}B"},
                                   NumberCase{"formatTracking", 1.25, "{\\T1.25;A}B"}};
    for (const NumberCase& test : kCases) {
        open("AB");
        select(0, 1);
        auto* field = formatBar().findChild<QDoubleSpinBox*>(QString::fromLatin1(test.field));
        ASSERT_NE(field, nullptr);
        field->setValue(test.value);
        field->editingFinished();
        EXPECT_EQ(overlay().value(), test.expected) << test.field;
        overlay().close();
    }

    open("AB");
    select(0, 1);
    auto* color = formatBar().findChild<QLineEdit*>(QStringLiteral("formatColor"));
    ASSERT_NE(color, nullptr);
    color->setText(QStringLiteral("1"));
    color->editingFinished();
    EXPECT_EQ(overlay().value(), "{\\C1;A}B");
}

TEST_F(TextFormatBarTest, EnterAppliesFieldsOnceAndReturnsFocusToEditor) {
    open("AB");
    select(0, 1);
    auto* height = formatBar().findChild<QDoubleSpinBox*>(QStringLiteral("formatHeight"));
    ASSERT_NE(height, nullptr);
    height->selectAll();
    events::type(height, "12");
    events::press(height, Qt::Key_Return);
    EXPECT_EQ(overlay().value(), "{\\H12;A}B");

    overlay().close();
    open("AB");
    select(0, 1);
    auto* color = formatBar().findChild<QLineEdit*>(QStringLiteral("formatColor"));
    ASSERT_NE(color, nullptr);
    color->setText(QStringLiteral("1"));
    events::press(color, Qt::Key_Return);
    EXPECT_EQ(overlay().value(), "{\\C1;A}B");
}

TEST_F(TextFormatBarTest, LeavingAnUnchangedNumberFieldKeepsTheText) {
    open("{\\H5;A}B");
    select(0, 2);
    auto* height = formatBar().findChild<QDoubleSpinBox*>(QStringLiteral("formatHeight"));
    ASSERT_NE(height, nullptr);

    height->editingFinished();

    EXPECT_EQ(overlay().value(), "{\\H5;A}B");
}

TEST_F(TextFormatBarTest, StackAndUnstackButtonsUseEditorModel) {
    open("1/2");
    select(0, 3);
    click("formatStack");
    EXPECT_EQ(overlay().value(), "\\S1/2;");

    select(0, 1);
    click("formatUnstack");
    EXPECT_EQ(overlay().value(), "1/2");
}

TEST_F(TextFormatBarTest, EditorShortcutsToggleBoldItalicAndUnderline) {
    open("ABC");

    struct Shortcut {
        Qt::Key key;
        const char* expected;
    };

    constexpr std::array kShortcuts = {Shortcut{Qt::Key_B, "{\\fArial|b1|i0;A}BC"},
                                       Shortcut{Qt::Key_I, "{\\fArial|b0|i1;A}BC"},
                                       Shortcut{Qt::Key_U, "{\\LA}BC"}};

    for (const Shortcut& shortcut : kShortcuts) {
        overlay().close();
        open("ABC");
        select(0, 1);
        events::press(&overlay(), shortcut.key, Qt::ControlModifier);
        EXPECT_EQ(overlay().value(), shortcut.expected);
    }
}

TEST_F(TextFormatBarTest, BarIsHiddenForSingleLineText) {
    core::Text text;
    text.value = "plain";
    cmd::TextEditRequest request;
    request.shown = text;
    request.value = text.value;
    overlay().open(request, drawing());
    auto* singleLineFormatBar =
        overlay().findChild<TextFormatBar*>(QStringLiteral("textFormatBar"));
    ASSERT_NE(singleLineFormatBar, nullptr);
    EXPECT_TRUE(singleLineFormatBar->isHidden());
}

}  // namespace
}  // namespace tg::app
