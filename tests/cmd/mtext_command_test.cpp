// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

class MTextCommandTest : public testing::Test {
protected:
    MTextCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string message) { m_messages.push_back(std::move(message)); });
    }

    void submit(std::initializer_list<const char*> inputs) {
        for (const char* input : inputs) {
            m_runner.submitText(input);
        }
    }

    [[nodiscard]] const core::MText& mtext(std::size_t index = 0) const {
        return std::get<core::MText>(m_document.entities()[index].entity);
    }

    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(MTextCommandTest, CreatesParagraphsAndOneUndoStep) {
    submit({"MTEXT", "10,20", "70,0", "First", "Second", ""});
    ASSERT_EQ(m_document.entityCount(), 1U);
    EXPECT_EQ(mtext().insertion, (core::Vec2{10, 20}));
    EXPECT_DOUBLE_EQ(mtext().width, 60.0);
    EXPECT_EQ(mtext().value, "First\\PSecond");
    EXPECT_EQ(m_editor.undoName(), "Mtext");
    m_runner.submitText("U");
    EXPECT_TRUE(m_document.isEmpty());
}

TEST_F(MTextCommandTest, WidthJustificationStyleAndRotationOptions) {
    submit({"MTEXT", "100,50", "H", "4", "J", "MC", "R", "30", "W", "40", "Center", ""});
    ASSERT_EQ(m_document.entityCount(), 1U);
    EXPECT_DOUBLE_EQ(mtext().height, 4.0);
    EXPECT_DOUBLE_EQ(mtext().width, 40.0);
    EXPECT_EQ(mtext().attachment, core::MTextAttachment::MiddleCenter);
    EXPECT_DOUBLE_EQ(mtext().rotation, core::degreesToRadians(30.0));

    submit({"MTEXT", "0,0", "30,-10", "Next", ""});
    EXPECT_DOUBLE_EQ(mtext(1).height, 4.0);
    EXPECT_EQ(mtext(1).attachment, core::MTextAttachment::TopLeft);
    EXPECT_DOUBLE_EQ(mtext(1).rotation, 0.0);
}

TEST_F(MTextCommandTest, MissingStyleRepeatsQuestion) {
    submit({"MTEXT", "0,0", "S", "Missing"});
    ASSERT_FALSE(m_messages.empty());
    EXPECT_EQ(m_messages.back(), "Cannot find text style \"Missing\".");
    EXPECT_EQ(m_runner.promptText(), "Enter style name or [?] <Standard>:");
    m_runner.submit(CancelInput{});
}

TEST_F(MTextCommandTest, LineSpacingSurvivesCancellationButNotAnotherRunner) {
    submit({"MTEXT", "0,0", "L", "E", "1.5x"});
    m_runner.submit(CancelInput{});
    EXPECT_TRUE(m_document.isEmpty());
    submit({"MTEXT", "0,0", "40,-10", "One", ""});
    EXPECT_TRUE(mtext().exactSpacing);
    EXPECT_DOUBLE_EQ(mtext().lineSpacing, 1.5);

    core::Document other = core::Document::newDrawing();
    edit::Editor otherEditor(other);
    CommandRunner otherRunner(m_registry, otherEditor);
    for (const char* input : {"MTEXT", "0,0", "40,-10", "One", ""}) {
        otherRunner.submitText(input);
    }
    const auto& otherMtext = std::get<core::MText>(other.entities().front().entity);
    EXPECT_FALSE(otherMtext.exactSpacing);
    EXPECT_DOUBLE_EQ(otherMtext.lineSpacing, 1.0);
}

TEST_F(MTextCommandTest, EscapeAfterLineKeepsText) {
    submit({"MTEXT", "0,0", "40,-10", "One"});
    m_runner.submit(CancelInput{});
    ASSERT_EQ(m_document.entityCount(), 1U);
    EXPECT_EQ(mtext().value, "One");
}

}  // namespace
}  // namespace tg::cmd
