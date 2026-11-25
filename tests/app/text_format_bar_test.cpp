// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "text_format_bar.hpp"

#include "text_editor_overlay.hpp"
#include "tg/cmd/text_editing.hpp"
#include "tg/core/document.hpp"
#include "tg/render/viewport.hpp"

#include <QToolButton>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

TEST(TextFormatBarTest, ButtonsApplyCodesAndFollowCaretFormat) {
    QWidget parent;
    render::Viewport viewport;
    TextEditorOverlay editor(&parent, viewport);
    core::Document document = core::Document::newDrawing();
    core::MText shown;
    shown.value = "AB";
    cmd::TextEditRequest request;
    request.shown = shown;
    request.value = shown.value;
    editor.open(request, document);

    TextFormatBar* bar = editor.findChild<TextFormatBar*>(QStringLiteral("textFormatBar"));
    ASSERT_NE(bar, nullptr);
    auto* bold = bar->findChild<QToolButton*>(QStringLiteral("formatBold"));
    ASSERT_NE(bold, nullptr);
    auto* model = const_cast<core::MTextEditModel*>(editor.model());
    ASSERT_NE(model, nullptr);
    model->select(0, 1);
    bold->click();
    EXPECT_EQ(editor.value(), "{\\fArial|b1|i0;A}B");

    model = const_cast<core::MTextEditModel*>(editor.model());
    model->setCaret(1);
    bar->refresh();
    EXPECT_TRUE(bold->isChecked());
}

TEST(TextFormatBarTest, DecorationAndStackButtonsUseEditorModel) {
    QWidget parent;
    render::Viewport viewport;
    TextEditorOverlay editor(&parent, viewport);
    core::Document document = core::Document::newDrawing();
    core::MText shown;
    shown.value = "1/2";
    cmd::TextEditRequest request;
    request.shown = shown;
    request.value = shown.value;
    editor.open(request, document);
    auto* bar = editor.findChild<TextFormatBar*>(QStringLiteral("textFormatBar"));
    ASSERT_NE(bar, nullptr);

    auto* model = const_cast<core::MTextEditModel*>(editor.model());
    ASSERT_NE(model, nullptr);
    model->select(0, 3);
    bar->findChild<QToolButton*>(QStringLiteral("formatUnderline"))->click();
    EXPECT_EQ(editor.value(), "{\\L1/2}");
    model = const_cast<core::MTextEditModel*>(editor.model());
    model->select(0, 3);
    bar->findChild<QToolButton*>(QStringLiteral("formatStack"))->click();
    EXPECT_NE(editor.value().find("\\S1/2;"), std::string::npos);
}

}  // namespace
}  // namespace tg::app
