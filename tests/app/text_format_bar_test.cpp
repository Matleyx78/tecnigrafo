// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "text_format_bar.hpp"

#include "text_editor_overlay.hpp"
#include "tg/cmd/text_editing.hpp"
#include "tg/core/document.hpp"
#include "tg/render/viewport.hpp"

#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QTest>
#include <QToolButton>
#include <gtest/gtest.h>

#include <utility>

namespace tg::app {
namespace {

class TextFormatBarTest : public ::testing::Test {
protected:
    TextFormatBarTest() : editor(&parent, viewport) {}

    void open(std::string value) {
        core::MText shown;
        shown.value = std::move(value);
        cmd::TextEditRequest request;
        request.shown = shown;
        request.value = shown.value;
        editor.open(request, document);
        bar = editor.findChild<TextFormatBar*>(QStringLiteral("textFormatBar"));
        ASSERT_NE(bar, nullptr);
    }

    void select(std::size_t anchor, std::size_t caret) {
        ASSERT_LE(anchor, caret);
        QTest::keyClick(&editor, Qt::Key_Home);
        for (std::size_t index = 0; index < anchor; ++index) {
            QTest::keyClick(&editor, Qt::Key_Right);
        }
        for (std::size_t index = anchor; index < caret; ++index) {
            QTest::keyClick(&editor, Qt::Key_Right, Qt::ShiftModifier);
        }
    }

    void click(const char* name) {
        auto* button = bar->findChild<QToolButton*>(QString::fromLatin1(name));
        ASSERT_NE(button, nullptr);
        button->click();
    }

    TextEditorOverlay& overlay() { return editor; }

    TextFormatBar& formatBar() { return *bar; }

    core::Document& drawing() { return document; }

private:
    QWidget parent;
    render::Viewport viewport;
    TextEditorOverlay editor;
    core::Document document = core::Document::newDrawing();
    TextFormatBar* bar{nullptr};
};

TEST_F(TextFormatBarTest, ToggleButtonsApplyCodesAndFollowCaretFormat) {
    open("AB");
    select(0, 1);
    click("formatBold");
    EXPECT_EQ(overlay().value(), "{\\fArial|b1|i0;A}B");

    QTest::keyClick(&overlay(), Qt::Key_Right);
    formatBar().refresh();
    EXPECT_TRUE(formatBar().findChild<QToolButton*>(QStringLiteral("formatBold"))->isChecked());
}

TEST_F(TextFormatBarTest, EveryToggleButtonAppliesItsMtextCode) {
    struct Case {
        const char* button;
        const char* expected;
    };

    constexpr Case cases[] = {{"formatItalic", "{\\fArial|b0|i1;A}B"},
                              {"formatUnderline", "{\\LA}B"},
                              {"formatOverline", "{\\OA}B"},
                              {"formatStrike", "{\\KA}B"}};
    for (const Case& test : cases) {
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

    constexpr NumberCase cases[] = {{"formatHeight", 3.5, "{\\H3.5;A}B"},
                                    {"formatWidth", 1.5, "{\\W1.5;A}B"},
                                    {"formatOblique", 12.0, "{\\Q12;A}B"},
                                    {"formatTracking", 1.25, "{\\T1.25;A}B"}};
    for (const NumberCase& test : cases) {
        open("AB");
        select(0, 1);
        auto* field = formatBar().findChild<QDoubleSpinBox*>(QString::fromLatin1(test.field));
        ASSERT_NE(field, nullptr);
        field->setValue(test.value);
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

    const struct Shortcut {
        Qt::Key key;
        const char* expected;
    } shortcuts[] = {{Qt::Key_B, "{\\fArial|b1|i0;A}BC"},
                     {Qt::Key_I, "{\\fArial|b0|i1;A}BC"},
                     {Qt::Key_U, "{\\LA}BC"}};

    for (const Shortcut& shortcut : shortcuts) {
        overlay().close();
        open("ABC");
        select(0, 1);
        QTest::keyClick(&overlay(), shortcut.key, Qt::ControlModifier);
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
