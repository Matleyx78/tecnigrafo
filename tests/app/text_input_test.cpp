// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "command_line.hpp"
#include "input_events.hpp"
#include "tg/cmd/builtin_commands.hpp"

#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <gtest/gtest.h>

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

}  // namespace
}  // namespace tg::app
