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

// --- Selection (docs/adr/0008) ------------------------------------------------

/// What the fake selecting command saw.
struct SelectionJournal {
    int started{0};
    std::vector<Input> inputs;
    int pointerMoves{0};
};

/// A command that works on existing entities: it records the selection it
/// receives and ends at once.
class FakeSelectCommand final : public Command {
public:
    explicit FakeSelectCommand(SelectionJournal& journal) : m_journal(journal) {}

    void start(CommandContext& context) override {
        ++m_journal.started;
        context.editor.beginTransaction("Pick");
    }

    Status handle(CommandContext& context, const Input& input) override {
        m_journal.inputs.push_back(input);
        context.editor.commit();
        return Status::Finished;
    }

    void pointerMoved(const Vec2& /*point*/) override { ++m_journal.pointerMoves; }

    [[nodiscard]] Prompt prompt() const override { return {"Pick:", {}}; }

    [[nodiscard]] std::span<const Entity> preview() const override { return {}; }

    [[nodiscard]] bool needsSelection() const override { return true; }

private:
    SelectionJournal& m_journal;
};

/// A two-point polyline from `from` to `to`.
Polyline line(const Vec2& from, const Vec2& to) {
    Polyline polyline;
    polyline.addVertex(from);
    polyline.addVertex(to);
    return polyline;
}

/// Four lines, with the default pixel size of 1 (pick box of 3 units):
///
///   b  (0,5)-(10,5)
///   d  (8,2)-(25,2)      crosses the rectangle (-5,-5)-(14,8)
///   a  (0,0)-(10,0)      c (20,0)-(30,0)
class SelectionTest : public testing::Test {
protected:
    SelectionTest()
        : m_a(m_document.add(line({0, 0}, {10, 0}))),
          m_b(m_document.add(line({0, 5}, {10, 5}))),
          m_c(m_document.add(line({20, 0}, {30, 0}))),
          m_d(m_document.add(line({8, 2}, {25, 2}))) {
        m_registry.add("FAKE", {"FK"}, [this] { return std::make_unique<FakeCommand>(m_journal); });
        m_registry.add("PICK", {"PK"},
                       [this] { return std::make_unique<FakeSelectCommand>(m_picks); });
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] Journal& journal() { return m_journal; }

    [[nodiscard]] const SelectionJournal& picks() const { return m_picks; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    [[nodiscard]] core::EntityId a() const { return m_a; }

    [[nodiscard]] core::EntityId b() const { return m_b; }

    [[nodiscard]] core::EntityId c() const { return m_c; }

    [[nodiscard]] core::EntityId d() const { return m_d; }

    /// The ids of the runner's selection, in order.
    [[nodiscard]] std::vector<core::EntityId> selected() const {
        const auto ids = m_runner.selection().ids();
        return {ids.begin(), ids.end()};
    }

    /// Traces a rectangle with two clicks; the action is that of the second.
    void rectangle(const Vec2& first, const Vec2& second, PickAction action = PickAction::Add) {
        m_runner.click(first, PickAction::Add);
        m_runner.click(second, action);
    }

private:
    Document m_document;
    core::EntityId m_a;
    core::EntityId m_b;
    core::EntityId m_c;
    core::EntityId m_d;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    Journal m_journal;
    SelectionJournal m_picks;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

using Ids = std::vector<core::EntityId>;

/// A SelectionInput as an Input; also keeps braces out of the gtest macros.
Input selectionInput(Ids ids) {
    return SelectionInput{std::move(ids)};
}

TEST_F(SelectionTest, IdleRunnerIsSelecting) {
    EXPECT_TRUE(runner().isSelecting());
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);
}

TEST_F(SelectionTest, ClickOnAnEntityAddsItSilently) {
    runner().click({5, 1}, PickAction::Add);  // 1 from a, 4 from b
    EXPECT_EQ(selected(), Ids{a()});
    runner().click({5, 4}, PickAction::Add);
    runner().click({5, 0}, PickAction::Add);  // already there: no duplicate
    EXPECT_EQ(selected(), (Ids{a(), b()}));
    EXPECT_EQ(runner().promptText(), "Command:");
    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);
    EXPECT_TRUE(messages().empty());
}

TEST_F(SelectionTest, ClickWithRemoveTakesAnEntityOut) {
    runner().click({5, 0}, PickAction::Add);
    runner().click({5, 5}, PickAction::Add);
    runner().click({5, 0.5}, PickAction::Remove);
    EXPECT_EQ(selected(), Ids{b()});
    runner().click({25, 0}, PickAction::Remove);  // c is not selected: nothing changes
    EXPECT_EQ(selected(), Ids{b()});
}

TEST_F(SelectionTest, ClickInTheVoidOpensARectangleThatFollowsThePointer) {
    runner().click({-5, -5}, PickAction::Add);
    EXPECT_EQ(runner().selectionRectangle(), (SelectionRectangle{{-5, -5}, {-5, -5}, false}));
    EXPECT_EQ(runner().promptText(), "Specify opposite corner:");
    EXPECT_TRUE(runner().selection().isEmpty());

    runner().pointerMoved({14, 8});
    EXPECT_EQ(runner().selectionRectangle(), (SelectionRectangle{{-5, -5}, {14, 8}, false}));
    runner().pointerMoved({-5, 6});  // same x: still a window
    EXPECT_EQ(runner().selectionRectangle(), (SelectionRectangle{{-5, -5}, {-5, 6}, false}));
    runner().pointerMoved({-8, 6});
    EXPECT_EQ(runner().selectionRectangle(), (SelectionRectangle{{-5, -5}, {-8, 6}, true}));
}

TEST_F(SelectionTest, WindowFromLeftToRightTakesWhatIsFullyInside) {
    rectangle({-5, -5}, {14, 8});
    EXPECT_EQ(selected(), (Ids{a(), b()}));
    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);
    EXPECT_EQ(runner().promptText(), "Command:");
}

TEST_F(SelectionTest, CrossingFromRightToLeftAlsoTakesWhatItTouches) {
    rectangle({14, 8}, {-5, -5});
    EXPECT_EQ(selected(), (Ids{a(), b(), d()}));
}

TEST_F(SelectionTest, TheSecondClickClosesTheRectangleEvenOnAnEntity) {
    rectangle({-5, -5}, {10, 5});  // the second corner is on b
    EXPECT_EQ(selected(), (Ids{a(), b()}));
}

TEST_F(SelectionTest, RectangleWithRemoveTakesEntitiesOut) {
    rectangle({-5, -5}, {40, 10});  // everything
    ASSERT_EQ(selected(), (Ids{a(), b(), c(), d()}));
    rectangle({14, 8}, {-5, -5}, PickAction::Remove);  // crossing: a, b, d
    EXPECT_EQ(selected(), Ids{c()});
}

TEST_F(SelectionTest, EscClosesTheRectangleThenClearsTheSelection) {
    runner().click({5, 0}, PickAction::Add);
    runner().click({-5, -5}, PickAction::Add);
    ASSERT_TRUE(runner().selectionRectangle().has_value());

    runner().submit(CancelInput{});
    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);
    EXPECT_EQ(selected(), Ids{a()});

    runner().submit(CancelInput{});
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_TRUE(messages().empty());
}

TEST_F(SelectionTest, EnterClosesTheRectangleAndRepeatsTheLastCommand) {
    runner().submitText("FAKE");
    runner().submitText("c");
    runner().click({-5, -5}, PickAction::Add);

    runner().submit(ConfirmInput{});

    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);
    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(journal().started, 2);
}

TEST_F(SelectionTest, PickBoxFollowsThePixelSize) {
    runner().click({25, -4}, PickAction::Add);  // 4 from c: outside 3 pixels of 1
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_TRUE(runner().selectionRectangle().has_value());
    runner().submit(CancelInput{});

    runner().setPixelSize(2.0);  // 3 pixels are now 6 units
    runner().click({25, -4}, PickAction::Add);
    EXPECT_EQ(selected(), Ids{c()});

    runner().setPixelSize(0.5);  // 1.5 units
    runner().click({5, -2}, PickAction::Add);
    EXPECT_EQ(selected(), Ids{c()});
    EXPECT_TRUE(runner().selectionRectangle().has_value());
}

TEST_F(SelectionTest, SelectionBeforeTheCommandIsHandedOverAtOnce) {
    runner().click({5, 5}, PickAction::Add);
    runner().click({5, 0}, PickAction::Add);

    runner().submitText("PICK");

    EXPECT_EQ(picks().started, 1);
    ASSERT_EQ(picks().inputs.size(), 1U);
    EXPECT_EQ(picks().inputs.front(), selectionInput({b(), a()}));
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_TRUE(messages().empty());  // no question asked
}

TEST_F(SelectionTest, SelectObjectsQuestionReportsEachGesture) {
    runner().submitText("pk");
    EXPECT_TRUE(runner().isActive());
    EXPECT_TRUE(runner().isSelecting());
    EXPECT_EQ(runner().promptText(), "Select objects:");
    EXPECT_EQ(picks().started, 0);  // start() waits for the selection

    runner().click({5, 0}, PickAction::Add);
    EXPECT_EQ(messages().back(), "1 found, 1 total");

    runner().click({-5, -5}, PickAction::Add);
    EXPECT_EQ(runner().promptText(), "Specify opposite corner:");
    runner().click({14, 8}, PickAction::Add);  // a again, and b
    EXPECT_EQ(messages().back(), "2 found, 2 total");
    EXPECT_EQ(runner().promptText(), "Select objects:");

    runner().click({5, 5}, PickAction::Remove);
    EXPECT_EQ(messages().back(), "1 removed, 1 total");

    rectangle({40, 10}, {-5, -5}, PickAction::Remove);  // a removed; b, c, d were not there
    EXPECT_EQ(messages().back(), "1 removed, 0 total");

    rectangle({40, 10}, {35, 8});  // nothing inside
    EXPECT_EQ(messages().back(), "0 found, 0 total");
    EXPECT_EQ(messages().size(), 5U);
}

TEST_F(SelectionTest, EnterEndsTheQuestionAndStartsTheCommand) {
    runner().submitText("PICK");
    runner().click({25, 0}, PickAction::Add);
    runner().click({5, 0}, PickAction::Add);

    runner().submitText("");

    EXPECT_EQ(picks().started, 1);
    ASSERT_EQ(picks().inputs.size(), 1U);
    EXPECT_EQ(picks().inputs.front(), selectionInput({c(), a()}));
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_EQ(runner().promptText(), "Command:");
}

TEST_F(SelectionTest, ConfirmInputAlsoEndsTheQuestion) {
    runner().submitText("PICK");
    runner().click({5, 0}, PickAction::Add);
    runner().click({-5, -5}, PickAction::Add);  // left open

    runner().submit(ConfirmInput{});

    EXPECT_EQ(picks().started, 1);
    EXPECT_EQ(picks().inputs, (std::vector<Input>{selectionInput({a()})}));
    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);
}

TEST_F(SelectionTest, EnterWithNothingSelectedEndsTheCommandWithNoEffect) {
    runner().submitText("PICK");
    runner().submitText("");

    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(picks().started, 0);
    EXPECT_TRUE(picks().inputs.empty());
    EXPECT_TRUE(messages().empty());
}

TEST_F(SelectionTest, TextIsNotAValidSelection) {
    runner().submitText("PICK");
    for (const char* text : {"abc", "10,10", "PICK", "U"}) {
        runner().submitText(text);
        EXPECT_EQ(messages().back(), "Invalid selection.") << text;
    }
    EXPECT_EQ(messages().size(), 4U);
    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(runner().promptText(), "Select objects:");
}

TEST_F(SelectionTest, EscInTheQuestionClosesTheRectangleThenCancels) {
    runner().submitText("PICK");
    runner().click({5, 0}, PickAction::Add);
    runner().click({-5, -5}, PickAction::Add);

    runner().submit(CancelInput{});
    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);
    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(runner().promptText(), "Select objects:");

    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(messages().back(), "*Cancel*");
    EXPECT_EQ(picks().started, 0);
    EXPECT_TRUE(runner().selection().isEmpty());
}

TEST_F(SelectionTest, TheQuestionIgnoresOtherInputsAndHidesTheCommand) {
    runner().submitText("PICK");
    runner().submit(PointInput{{5, 0}});
    runner().submit(KeywordInput{"Close"});
    runner().submit(SelectionInput{{a()}});
    runner().pointerMoved({1, 1});

    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_EQ(picks().pointerMoves, 0);
    EXPECT_TRUE(runner().preview().empty());
    EXPECT_TRUE(runner().isActive());
}

TEST_F(SelectionTest, ACommandThatDoesNotUseTheSelectionClearsIt) {
    runner().click({5, 0}, PickAction::Add);
    runner().submitText("FAKE");

    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_FALSE(runner().isSelecting());
    EXPECT_EQ(runner().promptText(), "Specify point or [Close/Fail]:");
}

TEST_F(SelectionTest, AClickBecomesAPointWhileACommandAsksForOne) {
    runner().submitText("FAKE");
    runner().click({5, 0}, PickAction::Add);  // on a: still a point
    runner().click({50, 50}, PickAction::Remove);

    EXPECT_EQ(journal().inputs, (std::vector<Input>{pointInput(5, 0), pointInput(50, 50)}));
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);
}

TEST_F(SelectionTest, OnlyTheRunnerHandsSelectionsToCommands) {
    runner().submit(SelectionInput{{a()}});  // idle: ignored
    EXPECT_TRUE(runner().selection().isEmpty());

    runner().submitText("FAKE");
    runner().submit(SelectionInput{{a()}});
    EXPECT_TRUE(journal().inputs.empty());
    EXPECT_TRUE(runner().isActive());
}

TEST_F(SelectionTest, TheEndOfACommandClearsTheSelection) {
    runner().submitText("FAKE");
    runner().submitText("c");
    runner().click({5, 0}, PickAction::Add);
    runner().submitText("FAKE");
    runner().submit(CancelInput{});

    EXPECT_TRUE(runner().selection().isEmpty());
}

TEST_F(SelectionTest, UndoAndRedoClearTheSelection) {
    runner().submitText("FAKE");
    runner().submitText("40,40");
    runner().submitText("c");

    runner().click({5, 0}, PickAction::Add);
    runner().submitText("U");
    EXPECT_TRUE(runner().selection().isEmpty());

    runner().click({5, 0}, PickAction::Add);
    runner().submitText("REDO");
    EXPECT_TRUE(runner().selection().isEmpty());

    runner().click({-5, -5}, PickAction::Add);
    runner().submitText("UNDO");  // typing also abandons the rectangle
    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);
}

TEST_F(SelectionTest, EnterRepeatsACommandWithTheCurrentSelection) {
    runner().click({5, 0}, PickAction::Add);
    runner().submitText("PICK");
    runner().click({25, 0}, PickAction::Add);

    runner().submitText("");

    EXPECT_EQ(picks().started, 2);
    EXPECT_EQ(picks().inputs, (std::vector<Input>{selectionInput({a()}), selectionInput({c()})}));
    EXPECT_TRUE(messages().empty());
}

}  // namespace
}  // namespace tg::cmd
