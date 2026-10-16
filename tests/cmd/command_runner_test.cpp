// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

using core::Document;
using core::Entity;
using core::Polyline;
using core::Vec2;

/// A PointInput as an Input; also keeps braces out of the gtest macros.
Input pointInput(double x, double y) {
    return PointInput{{x, y}};
}

/// What the fake commands saw, shared with the test because the runner owns
/// the command objects.
struct Journal {
    int started{0};
    std::vector<Input> inputs;
    std::vector<Vec2> pointerPositions;
};

/// A minimal command: every point becomes a vertex of a polyline added to
/// the document at once; "Close" or Enter commits, "Fail" leaves the
/// transaction open by mistake.
class FakeCommand final : public Command {
public:
    explicit FakeCommand(Journal& journal) : m_journal(journal) {}

    void start(CommandContext& context) override {
        ++m_journal.started;
        context.editor.beginTransaction("Fake");
        context.message("Fake started.");
    }

    Status handle(CommandContext& context, const Input& input) override {
        m_journal.inputs.push_back(input);
        if (const auto* point = std::get_if<PointInput>(&input)) {
            Polyline polyline;
            polyline.addVertex(point->point);
            context.editor.add(polyline);
            context.lastPoint = point->point;
            m_preview.emplace_back(polyline);
            return Status::Continue;
        }
        if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
            if (keyword->keyword == "Fail") {
                return Status::Finished;
            }
        }
        context.editor.commit();
        return Status::Finished;
    }

    void pointerMoved(const Vec2& point) override { m_journal.pointerPositions.push_back(point); }

    [[nodiscard]] Prompt prompt() const override {
        return {"Specify point or [Close/Fail]:", {{"Close", "C"}, {"Fail", "F"}}};
    }

    [[nodiscard]] std::span<const Entity> preview() const override { return m_preview; }

private:
    Journal& m_journal;
    std::vector<Entity> m_preview;
};

class CommandRunnerTest : public testing::Test {
protected:
    CommandRunnerTest() {
        m_registry.add("FAKE", {"FK", "F2"},
                       [this] { return std::make_unique<FakeCommand>(m_journal); });
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    [[nodiscard]] const Document& document() const { return m_document; }

    [[nodiscard]] Journal& journal() { return m_journal; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    /// The last input the fake command received, if any.
    [[nodiscard]] std::optional<Input> lastInput() const {
        if (m_journal.inputs.empty()) {
            return std::nullopt;
        }
        return m_journal.inputs.back();
    }

private:
    Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    Journal m_journal;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

// --- Registry -------------------------------------------------------------

TEST(CommandRegistry, FindsNamesAndAliasesIgnoringCase) {
    Journal journal;
    CommandRegistry registry;
    registry.add("PLINE", {"PL"}, [&journal] { return std::make_unique<FakeCommand>(journal); });

    EXPECT_EQ(registry.canonicalName("PLINE"), "PLINE");
    EXPECT_EQ(registry.canonicalName("pline"), "PLINE");
    EXPECT_EQ(registry.canonicalName("Pl"), "PLINE");
    EXPECT_EQ(registry.canonicalName("PLIN"), std::nullopt);
    EXPECT_EQ(registry.canonicalName(""), std::nullopt);

    EXPECT_NE(registry.create("pl"), nullptr);
    EXPECT_EQ(registry.create("LINE"), nullptr);
}

TEST(CommandRegistry, CreatesANewInstanceEveryTime) {
    Journal journal;
    CommandRegistry registry;
    registry.add("FAKE", {}, [&journal] { return std::make_unique<FakeCommand>(journal); });

    const std::unique_ptr<Command> first = registry.create("FAKE");
    const std::unique_ptr<Command> second = registry.create("FAKE");
    ASSERT_NE(first, nullptr);
    EXPECT_NE(first, second);
}

// --- Starting commands ------------------------------------------------------

TEST_F(CommandRunnerTest, IdleRunnerShowsTheCommandPrompt) {
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(runner().promptText(), "Command:");
    EXPECT_TRUE(runner().preview().empty());
}

TEST_F(CommandRunnerTest, StartsACommandByNameOrAliasInAnyCase) {
    for (const char* text : {"FAKE", "fake", "Fk", "  f2  "}) {
        runner().submitText(text);
        EXPECT_TRUE(runner().isActive()) << text;
        EXPECT_EQ(runner().promptText(), "Specify point or [Close/Fail]:");
        runner().submit(CancelInput{});
    }
    EXPECT_EQ(journal().started, 4);
}

TEST_F(CommandRunnerTest, UnknownCommandWritesAMessage) {
    runner().submitText(" Xyz ");

    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(messages(), (std::vector<std::string>{"Unknown command: Xyz."}));
}

TEST_F(CommandRunnerTest, CommandMessagesReachTheSink) {
    runner().submitText("FAKE");
    EXPECT_EQ(messages(), (std::vector<std::string>{"Fake started."}));
}

// --- Typed input while a command runs --------------------------------------

TEST_F(CommandRunnerTest, KeywordsMatchNameOrShortcutIgnoringCase) {
    for (const char* text : {"Close", "close", "C", "c"}) {
        runner().submitText("FAKE");
        runner().submitText(text);
        EXPECT_EQ(lastInput(), Input{KeywordInput{"Close"}}) << text;
        EXPECT_FALSE(runner().isActive());
    }
}

TEST_F(CommandRunnerTest, TypedCoordinatesBecomePoints) {
    runner().submitText("FAKE");

    runner().submitText("10,20");
    EXPECT_EQ(lastInput(), pointInput(10.0, 20.0));

    runner().submitText("@5,-5");  // relative to the last point, set by the command
    EXPECT_EQ(lastInput(), pointInput(15.0, 15.0));

    runner().pointerMoved({15.0, 100.0});
    runner().submitText("10");  // distance towards the pointer
    EXPECT_EQ(lastInput(), pointInput(15.0, 25.0));

    EXPECT_EQ(messages().size(), 1U);  // only "Fake started."
}

TEST_F(CommandRunnerTest, InvalidTextKeepsThePromptAndWritesAMessage) {
    runner().submitText("FAKE");
    const std::string prompt = runner().promptText();

    // Not an option nor a coordinate; then coordinates that cannot be
    // resolved: no last point yet.
    for (const char* text : {"abc", "1;2", "Closed", "@1,1", "@5<90", "5"}) {
        runner().submitText(text);
    }

    EXPECT_TRUE(journal().inputs.empty());
    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(runner().promptText(), prompt);
    EXPECT_EQ(messages().size(), 7U);
    EXPECT_EQ(messages().back(), "Invalid point or option.");
}

TEST_F(CommandRunnerTest, DistanceWithoutPointerMovementIsInvalid) {
    runner().submitText("FAKE");
    runner().submitText("0,0");
    runner().submitText("5");

    EXPECT_EQ(journal().inputs.size(), 1U);
    EXPECT_EQ(messages().back(), "Invalid point or option.");
}

TEST_F(CommandRunnerTest, EmptyTextConfirmsTheRunningCommand) {
    runner().submitText("FAKE");
    runner().submitText("1,1");
    runner().submitText("   ");

    EXPECT_EQ(lastInput(), Input{ConfirmInput{}});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(editor().undoName(), "Fake");
}

// --- Mouse and keyboard input ------------------------------------------------

TEST_F(CommandRunnerTest, InputsAndPointerReachTheCommand) {
    runner().pointerMoved({1.0, 1.0});  // idle: nobody to tell
    runner().submitText("FAKE");
    runner().pointerMoved({2.0, 3.0});
    runner().submit(PointInput{{4.0, 5.0}});

    EXPECT_EQ(journal().pointerPositions, (std::vector<Vec2>{{2.0, 3.0}}));
    EXPECT_EQ(lastInput(), pointInput(4.0, 5.0));
    EXPECT_EQ(runner().preview().size(), 1U);
}

TEST_F(CommandRunnerTest, EscRollsBackAndEndsTheCommand) {
    runner().submitText("FAKE");
    runner().submit(PointInput{{1.0, 1.0}});
    runner().submit(PointInput{{2.0, 2.0}});
    ASSERT_EQ(document().entityCount(), 2U);

    runner().submit(CancelInput{});

    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_TRUE(document().isEmpty());
    EXPECT_FALSE(editor().canUndo());
    EXPECT_TRUE(runner().preview().empty());
    EXPECT_EQ(messages().back(), "*Cancel*");
    // The command never sees Esc.
    EXPECT_EQ(journal().inputs.size(), 2U);
}

TEST_F(CommandRunnerTest, EditorListenerSeesTheRunnerIdleAfterEsc) {
    std::optional<bool> activeInListener;
    editor().setChangeListener([&] { activeInListener = runner().isActive(); });
    runner().submitText("FAKE");
    runner().submit(CancelInput{});

    EXPECT_EQ(activeInListener, false);
}

TEST_F(CommandRunnerTest, IdleRunnerIgnoresPointsKeywordsAndEsc) {
    runner().submit(PointInput{{1.0, 1.0}});
    runner().submit(KeywordInput{"Close"});
    runner().submit(CancelInput{});

    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(journal().started, 0);
    EXPECT_TRUE(messages().empty());
}

// --- Repeating the last command ----------------------------------------------

TEST_F(CommandRunnerTest, EnterRepeatsTheLastCommand) {
    runner().submitText("");  // nothing to repeat yet
    EXPECT_FALSE(runner().isActive());

    runner().submitText("fk");
    runner().submitText("c");
    runner().submitText("");
    EXPECT_TRUE(runner().isActive());
    runner().submitText("c");

    runner().submit(ConfirmInput{});  // space or right click
    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(journal().started, 3);
}

TEST_F(CommandRunnerTest, UnknownCommandDoesNotReplaceTheLastOne) {
    runner().submitText("FAKE");
    runner().submit(CancelInput{});
    runner().submitText("NOPE");
    runner().submitText("");

    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(journal().started, 2);
}

TEST_F(CommandRunnerTest, LastPointCarriesOverToTheNextCommand) {
    runner().submitText("FAKE");
    runner().submitText("10,10");
    runner().submitText("");

    runner().submitText("FAKE");
    runner().submitText("@1,0");
    EXPECT_EQ(lastInput(), pointInput(11.0, 10.0));
}

// --- Undo and redo -----------------------------------------------------------

TEST_F(CommandRunnerTest, UndoAndRedoCommands) {
    runner().submitText("FAKE");
    runner().submitText("1,1");
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 1U);

    runner().submitText("u");
    EXPECT_TRUE(document().isEmpty());
    EXPECT_EQ(messages().back(), "Undone: Fake.");

    runner().submitText("REDO");
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(messages().back(), "Redone: Fake.");

    runner().submitText("Undo");
    EXPECT_TRUE(document().isEmpty());
    EXPECT_FALSE(runner().isActive());
}

TEST_F(CommandRunnerTest, UndoAndRedoWithNothingToDo) {
    runner().submitText("U");
    EXPECT_EQ(messages().back(), "Nothing to undo.");
    runner().submitText("redo");
    EXPECT_EQ(messages().back(), "Nothing to redo.");
}

TEST_F(CommandRunnerTest, EnterRepeatsUndo) {
    for (int i = 0; i < 2; ++i) {
        runner().submitText("FAKE");
        runner().submitText("1,1");
        runner().submitText("c");
    }
    ASSERT_EQ(document().entityCount(), 2U);

    runner().submitText("U");
    runner().submitText("");
    EXPECT_TRUE(document().isEmpty());
}

TEST_F(CommandRunnerTest, UndoIsNotAvailableWhileACommandRuns) {
    runner().submitText("FAKE");
    runner().submitText("1,1");
    runner().submitText("");
    runner().submitText("FAKE");

    runner().submitText("U");  // not an option of the prompt: invalid input

    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(messages().back(), "Invalid point or option.");
}

// --- Contract with commands ----------------------------------------------------

TEST_F(CommandRunnerTest, FinishingWithAnOpenTransactionIsCaught) {
#ifdef NDEBUG
    GTEST_SKIP() << "checked with assert, disabled in this build";
#endif
    runner().submitText("FAKE");
    EXPECT_DEATH(runner().submitText("F"), "without closing its transaction");
}

}  // namespace
}  // namespace tg::cmd
