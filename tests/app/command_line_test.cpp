// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "command_line.hpp"

#include "input_events.hpp"
#include "tg/cmd/command.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSignalSpy>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>
#include <gtest/gtest.h>

#include <memory>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace tg::app {
namespace {

using events::press;
using events::type;

/// A stand-in for PLINE, which arrives with package P6: it collects points
/// and adds a polyline on Enter.
class FakePline final : public cmd::Command {
public:
    void start(cmd::CommandContext& context) override {
        context.editor.beginTransaction("Polyline");
    }

    cmd::Status handle(cmd::CommandContext& context, const cmd::Input& input) override {
        if (const auto* point = std::get_if<cmd::PointInput>(&input)) {
            m_polyline.addVertex(point->point);
            context.lastPoint = point->point;
            return cmd::Status::Continue;
        }
        if (m_polyline.vertexCount() >= 2) {
            context.editor.add(m_polyline);
        }
        context.editor.commit();
        return cmd::Status::Finished;
    }

    void pointerMoved(const core::Vec2& /*point*/) override {}

    [[nodiscard]] cmd::Prompt prompt() const override {
        if (m_polyline.vertexCount() == 0) {
            return {"Specify start point:", {}};
        }
        return {"Specify next point:", {}};
    }

    [[nodiscard]] std::span<const core::Entity> preview() const override { return {}; }

private:
    core::Polyline m_polyline;
};

/// A command line wired to a runner that knows only PLINE (alias PL), below
/// a stand-in for the drawing view.
class CommandLineTest : public testing::Test {
protected:
    CommandLineTest() {
        m_registry.add("PLINE", {"PL"}, [] { return std::make_unique<FakePline>(); });
        auto* layout = new QVBoxLayout(&m_window);
        layout->addWidget(m_view);
        layout->addWidget(m_commandLine);
        m_view->setFocusPolicy(Qt::StrongFocus);
        m_commandLine->forwardKeysFrom(m_view);
        m_window.show();
    }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] CommandLine* commandLine() const { return m_commandLine; }

    [[nodiscard]] QWidget* view() const { return m_view; }

    [[nodiscard]] QLineEdit* input() const { return m_commandLine->findChild<QLineEdit*>("input"); }

    // Strings are compared as std::string, which GoogleTest prints readably.

    [[nodiscard]] std::string inputText() const { return input()->text().toStdString(); }

    [[nodiscard]] std::string prompt() const {
        return m_commandLine->findChild<QLabel*>("prompt")->text().toStdString();
    }

    [[nodiscard]] std::vector<std::string> history() const {
        const auto* history = m_commandLine->findChild<QPlainTextEdit*>("history");
        std::vector<std::string> lines;
        for (const QString& line :
             history->toPlainText().split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            lines.push_back(line.toStdString());
        }
        return lines;
    }

    /// Types `text` in the input field and presses `key`.
    void enter(const std::string& text, Qt::Key key = Qt::Key_Return) const {
        type(input(), text);
        press(input(), key);
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

TEST_F(CommandLineTest, TypesPlineACoordinateAndEsc) {
    EXPECT_EQ(prompt(), "Command:");

    enter("PL");
    EXPECT_EQ(prompt(), "Specify start point:");
    EXPECT_TRUE(inputText().empty());

    enter("10,20");
    EXPECT_EQ(prompt(), "Specify next point:");

    enter("@5", Qt::Key_Escape);
    EXPECT_EQ(prompt(), "Command:");
    EXPECT_TRUE(inputText().empty());
    EXPECT_TRUE(document().isEmpty());

    const std::vector<std::string> expected{
        "Command: PL",
        "Specify start point: 10,20",
        "*Cancel*",
    };
    EXPECT_EQ(history(), expected);
}

TEST_F(CommandLineTest, SpaceSubmitsAndEnterRepeats) {
    enter("pl", Qt::Key_Space);
    enter("0,0", Qt::Key_Space);
    enter("@50,0", Qt::Key_Space);
    enter("", Qt::Key_Space);  // empty: confirm
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(prompt(), "Command:");

    enter("");  // empty: repeat PLINE
    EXPECT_EQ(prompt(), "Specify start point:");
}

TEST_F(CommandLineTest, ShowsMessagesOfTheRunner) {
    enter("XYZ");
    enter("PL");
    enter("abc");

    const std::vector<std::string> expected{
        "Command: XYZ",
        "Unknown command: XYZ.",
        "Command: PL",
        "Specify start point: abc",
        "Invalid point or option.",
    };
    EXPECT_EQ(history(), expected);
    EXPECT_EQ(prompt(), "Specify start point:");
}

TEST_F(CommandLineTest, ForwardsTextKeysFromTheView) {
    type(view(), "pl");
    EXPECT_EQ(inputText(), "pl");

    press(view(), Qt::Key_Return);
    EXPECT_EQ(prompt(), "Specify start point:");

    type(view(), "@-1.5<45,");
    EXPECT_EQ(inputText(), "@-1.5<45,");

    press(view(), Qt::Key_Escape);
    EXPECT_EQ(prompt(), "Command:");
    EXPECT_TRUE(inputText().empty());
}

TEST_F(CommandLineTest, LeavesShortcutsOfTheViewAlone) {
    press(view(), Qt::Key_Z, Qt::ControlModifier, QStringLiteral("z"));
    press(view(), Qt::Key_P, Qt::AltModifier, QStringLiteral("p"));
    press(view(), Qt::Key_Exclam, Qt::NoModifier, QStringLiteral("!"));  // not in coordinates
    EXPECT_TRUE(inputText().empty());
    EXPECT_TRUE(history().empty());
}

TEST_F(CommandLineTest, EmitsInputHandledAfterEverySubmission) {
    const QSignalSpy spy(commandLine(), &CommandLine::inputHandled);
    type(input(), "PL");
    EXPECT_EQ(spy.count(), 0);  // typing alone sends nothing
    press(input(), Qt::Key_Return);
    press(input(), Qt::Key_Escape);
    EXPECT_EQ(spy.count(), 2);
}

}  // namespace
}  // namespace tg::app

// The main() of every test in tg_app_tests.
int main(int argc, char* argv[]) {
    testing::InitGoogleTest(&argc, argv);
    const QApplication application(argc, argv);
    return RUN_ALL_TESTS();
}
