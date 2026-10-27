// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/command_runner.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/core/math.hpp"

#include <gtest/gtest.h>

#include <cmath>
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

/// The description of FakeCommand under `name`.
CommandDescription fakeDescription(std::string name, std::vector<std::string> aliases) {
    return {std::move(name),
            std::move(aliases),
            "Adds a vertex for each point.",
            false,
            {{"point",
              "Specify point or [Close/Fail]:",
              ValueType::Point,
              {{"Close", "C"}, {"Fail", "F"}},
              true}}};
}

class CommandRunnerTest : public testing::Test {
protected:
    CommandRunnerTest() {
        m_registry.add(fakeDescription("FAKE", {"FK", "F2"}),
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

TEST_F(CommandRunnerTest, ReplaceSelectionIsSilentAndClosesTheRectangle) {
    editor().beginTransaction("Create test entities");
    const auto first = editor().add(core::Line{{0, 0}, {10, 0}});
    const auto second = editor().add(core::Line{{0, 10}, {10, 10}});
    editor().commit();
    runner().click({100, 100}, PickAction::Add);
    ASSERT_TRUE(runner().selectionRectangle().has_value());
    EXPECT_TRUE(runner().replaceSelection({second, first, second}));
    EXPECT_FALSE(runner().selectionRectangle().has_value());
    const auto ids = runner().selection().ids();
    EXPECT_EQ((std::vector<core::EntityId>{ids.begin(), ids.end()}),
              (std::vector{second, first}));
    EXPECT_TRUE(messages().empty());
    EXPECT_TRUE(runner().replaceSelection({}));
    EXPECT_TRUE(runner().selection().isEmpty());
}

TEST_F(CommandRunnerTest, ReplaceSelectionDoesNothingWhileCommandRuns) {
    runner().submitText("FAKE");
    const auto beforeMessages = messages();
    EXPECT_FALSE(runner().replaceSelection({core::EntityId{999}}));
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_EQ(messages(), beforeMessages);
    EXPECT_TRUE(runner().isActive());
    runner().submit(CancelInput{});
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

/// The description of FakeSelectCommand.
CommandDescription pickDescription(std::string name, std::vector<std::string> aliases) {
    return {std::move(name),
            std::move(aliases),
            "Records a selection.",
            true,
            {{"pick", "Pick:", ValueType::Point, {}, false}}};
}

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
        m_registry.add(fakeDescription("FAKE", {"FK"}),
                       [this] { return std::make_unique<FakeCommand>(m_journal); });
        m_registry.add(pickDescription("PICK", {"PK"}),
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

// --- Locked layers (docs/adr/0023, point 4) ------------------------------------

/// A new drawing with four horizontal lines, two of them on the locked layer
/// "Locked", and the built-in commands (ERASE) next to the fake PICK:
///
///   y  (0,15)-(10,15)  locked      b  (0,10)-(10,10)
///   x  (0,5)-(10,5)    locked      a  (0,0)-(10,0)
class LockedLayerTest : public testing::Test {
protected:
    LockedLayerTest() : m_document(Document::newDrawing()) {
        core::Layer locked;
        locked.name = "Locked";
        locked.linetype = m_document.continuousLinetype();
        locked.locked = true;
        const core::LayerId lockedId = m_document.addLayer(locked);

        // A new entity takes the current properties, so the current layer
        // says where each line goes.
        const core::CurrentProperties open = m_document.current();
        core::CurrentProperties onLocked = open;
        onLocked.layer = lockedId;
        m_a = m_document.add(line({0, 0}, {10, 0}));
        m_document.setCurrent(onLocked);
        m_x = m_document.add(line({0, 5}, {10, 5}));
        m_document.setCurrent(open);
        m_b = m_document.add(line({0, 10}, {10, 10}));
        m_document.setCurrent(onLocked);
        m_y = m_document.add(line({0, 15}, {10, 15}));
        m_document.setCurrent(open);

        registerBuiltinCommands(m_registry);
        m_registry.add(pickDescription("PICK", {"PK"}),
                       [this] { return std::make_unique<FakeSelectCommand>(m_picks); });
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    [[nodiscard]] const Document& document() const { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const SelectionJournal& picks() const { return m_picks; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    [[nodiscard]] core::EntityId a() const { return m_a; }

    [[nodiscard]] core::EntityId b() const { return m_b; }

    [[nodiscard]] core::EntityId x() const { return m_x; }

    [[nodiscard]] core::EntityId y() const { return m_y; }

    /// The ids of the runner's selection, in order.
    [[nodiscard]] Ids selected() const {
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
    core::EntityId m_x;
    core::EntityId m_y;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    SelectionJournal m_picks;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(LockedLayerTest, AClickOnALockedEntitySelectsNothingAndSaysNothing) {
    runner().click({5, 5}, PickAction::Add);  // x, which the click found and the runner dropped
    EXPECT_TRUE(selected().empty());
    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);  // the click found an entity
    EXPECT_TRUE(messages().empty());

    runner().click({5, 0}, PickAction::Add);  // a, on the open layer
    EXPECT_EQ(selected(), Ids{a()});
    EXPECT_TRUE(messages().empty());
}

TEST_F(LockedLayerTest, WindowAndCrossingKeepOnlyTheEntitiesOfOpenLayers) {
    rectangle({-5, -5}, {15, 20});  // window: all four lines are inside
    EXPECT_EQ(selected(), (Ids{a(), b()}));
    EXPECT_TRUE(messages().empty());

    runner().submit(CancelInput{});  // with no rectangle open, Esc clears the selection
    EXPECT_TRUE(selected().empty());

    rectangle({15, 12}, {-5, 3});  // crossing: touches x and b
    EXPECT_EQ(selected(), Ids{b()});
    EXPECT_TRUE(messages().empty());
}

TEST_F(LockedLayerTest, TheQuestionSaysHowManyEntitiesWereOnLockedLayers) {
    runner().submitText("PICK");

    runner().click({5, 5}, PickAction::Add);  // x alone: singular
    EXPECT_EQ(messages(),
              (std::vector<std::string>{"0 found, 0 total", "1 was on a locked layer."}));

    runner().click({5, 0}, PickAction::Add);  // a alone: no sentence about locked layers
    ASSERT_EQ(messages().size(), 3U);
    EXPECT_EQ(messages().back(), "1 found, 1 total");

    rectangle({-5, -5}, {15, 20});  // a again and b; x and y are dropped: plural
    ASSERT_EQ(messages().size(), 5U);
    EXPECT_EQ(messages()[3], "2 found, 2 total");
    EXPECT_EQ(messages()[4], "2 were on a locked layer.");
    EXPECT_EQ(selected(), (Ids{a(), b()}));
}

TEST_F(LockedLayerTest, TakingEntitiesOutSaysNothingAboutLockedLayers) {
    runner().submitText("PICK");
    runner().click({5, 0}, PickAction::Add);
    rectangle({-5, -5}, {15, 20}, PickAction::Remove);  // takes a out; x and y were never in

    ASSERT_EQ(messages().size(), 2U);
    EXPECT_EQ(messages().back(), "1 removed, 0 total");
    EXPECT_TRUE(selected().empty());
}

TEST_F(LockedLayerTest, EraseDoesNotRemoveTheEntitiesOfLockedLayers) {
    runner().submitText("ERASE");
    rectangle({-5, -5}, {15, 20});
    EXPECT_EQ(selected(), (Ids{a(), b()}));

    runner().submitText("");

    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().find(a()), nullptr);
    EXPECT_EQ(document().find(b()), nullptr);
    EXPECT_NE(document().find(x()), nullptr);
    EXPECT_NE(document().find(y()), nullptr);
    EXPECT_EQ(editor().undoName(), "Erase");
}

TEST_F(LockedLayerTest, ASelectionMadeBeforeTheCommandHasNoLockedEntities) {
    rectangle({-5, -5}, {15, 20});
    runner().submitText("ERASE");  // starts at once with the selection

    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 2U);
    EXPECT_NE(document().find(x()), nullptr);
    EXPECT_NE(document().find(y()), nullptr);
    EXPECT_TRUE(messages().empty());
}

TEST_F(LockedLayerTest, EraseWithOnlyLockedEntitiesFoundEndsWithNoEffect) {
    runner().submitText("ERASE");
    runner().click({5, 5}, PickAction::Add);
    runner().click({5, 15}, PickAction::Add);
    runner().submitText("");

    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 4U);
    EXPECT_TRUE(editor().undoNames().empty());
    EXPECT_EQ(messages(),
              (std::vector<std::string>{"0 found, 0 total", "1 was on a locked layer.",
                                        "0 found, 0 total", "1 was on a locked layer."}));
}

// --- Snap and ortho (docs/adr/0008) --------------------------------------------

/// What the fake point command saw.
struct PointJournal {
    std::vector<Vec2> points;
    std::vector<Vec2> pointerPositions;
};

/// A command that asks for points and leaves the document alone, so that it
/// adds no snap points of its own: every point becomes the last point and
/// the base point, Enter ends it. The preview is a single vertex where the
/// pointer is.
class FakePointCommand final : public Command {
public:
    explicit FakePointCommand(PointJournal& journal) : m_journal(journal) {}

    void start(CommandContext& /*context*/) override {}

    Status handle(CommandContext& context, const Input& input) override {
        if (const auto* point = std::get_if<PointInput>(&input)) {
            m_journal.points.push_back(point->point);
            context.lastPoint = point->point;
            m_basePoint = point->point;
            return Status::Continue;
        }
        return Status::Finished;
    }

    [[nodiscard]] std::optional<Vec2> basePoint() const override { return m_basePoint; }

    void pointerMoved(const Vec2& point) override {
        m_journal.pointerPositions.push_back(point);
        Polyline polyline;
        polyline.addVertex(point);
        m_preview.assign(1, polyline);
    }

    [[nodiscard]] Prompt prompt() const override { return {"Specify point:", {}}; }

    [[nodiscard]] std::span<const Entity> preview() const override { return m_preview; }

private:
    PointJournal& m_journal;
    std::vector<Entity> m_preview;
    std::optional<Vec2> m_basePoint;
};

/// A line and a half circle, with the default pixel size of 1 (snap
/// aperture of 10 units, pick box of 3):
///
///   line  (0,0)-(100,0)       endpoints (0,0), (100,0); midpoint (50,0)
///   arc   (200,0)-(300,0)     bulge 1: centre (250,0), radius 50,
///                             midpoint (250,-50), below the x axis
class DrawingAidsTest : public testing::Test {
protected:
    DrawingAidsTest() {
        m_document.add(line({0, 0}, {100, 0}));
        m_document.add(Polyline{{{{200, 0}, 1.0}, {{300, 0}, 0.0}}});
        m_registry.add({"POINT",
                        {"PT"},
                        "Records points.",
                        false,
                        {{"point", "Specify point:", ValueType::Point, {}, true}}},
                       [this] { return std::make_unique<FakePointCommand>(m_journal); });
        m_registry.add(pickDescription("PICK", {}),
                       [this] { return std::make_unique<FakeSelectCommand>(m_picks); });
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const PointJournal& journal() const { return m_journal; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    /// The points the command received, as given by the runner.
    [[nodiscard]] const std::vector<Vec2>& points() const { return m_journal.points; }

    /// The last pointer position the command received.
    [[nodiscard]] std::optional<Vec2> commandPointer() const {
        if (m_journal.pointerPositions.empty()) {
            return std::nullopt;
        }
        return m_journal.pointerPositions.back();
    }

    /// The single vertex of the runner's preview.
    [[nodiscard]] std::optional<Vec2> previewPoint() const {
        const std::span<const Entity> preview = m_runner.preview();
        if (preview.size() != 1) {
            return std::nullopt;
        }
        const auto& polyline = std::get<Polyline>(preview.front());
        if (polyline.vertexCount() != 1) {
            return std::nullopt;
        }
        return polyline.vertices().front().position;
    }

    void setAids(bool objectSnap, bool ortho) {
        DrawingAids aids = m_runner.drawingAids();
        aids.objectSnap = objectSnap;
        aids.ortho = ortho;
        m_runner.setDrawingAids(aids);
    }

private:
    Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    PointJournal m_journal;
    SelectionJournal m_picks;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

using Points = std::vector<Vec2>;

TEST_F(DrawingAidsTest, StartsWithSnapOnAndOrthoOff) {
    EXPECT_EQ(runner().drawingAids(), DrawingAids{});

    DrawingAids aids;
    aids.ortho = true;
    aids.snapModes.midpoint = false;
    runner().setDrawingAids(aids);
    EXPECT_EQ(runner().drawingAids(), aids);
}

TEST_F(DrawingAidsTest, AClickNearAnEndpointSnapsToIt) {
    runner().submitText("POINT");
    runner().click({3, 4}, PickAction::Add);       // 5 from (0,0)
    runner().click({97, -4}, PickAction::Remove);  // the action does not matter
    EXPECT_EQ(points(), (Points{{0, 0}, {100, 0}}));
}

TEST_F(DrawingAidsTest, AClickIsLeftAsItIsWithObjectSnapOff) {
    setAids(false, false);
    runner().submitText("POINT");
    runner().click({3, 4}, PickAction::Add);
    EXPECT_EQ(points(), (Points{{3, 4}}));
}

TEST_F(DrawingAidsTest, AClickOutsideTheApertureIsLeftAsItIs) {
    runner().submitText("POINT");
    runner().click({15, 4}, PickAction::Add);  // on the line, but 15.5 from (0,0)
    EXPECT_EQ(points(), (Points{{15, 4}}));
}

TEST_F(DrawingAidsTest, EachSnapModeCanBeTurnedOff) {
    runner().submitText("POINT");
    runner().click({52, 3}, PickAction::Add);
    EXPECT_EQ(points().back(), (Vec2{50, 0}));  // midpoint

    DrawingAids aids;
    aids.snapModes.midpoint = false;
    runner().setDrawingAids(aids);
    runner().click({52, 3}, PickAction::Add);
    EXPECT_EQ(points().back(), (Vec2{52, 3}));

    aids.snapModes = {false, true, true};  // endpoint off
    runner().setDrawingAids(aids);
    runner().click({3, 4}, PickAction::Add);
    EXPECT_EQ(points().back(), (Vec2{3, 4}));

    aids.snapModes = {true, true, false};  // center off
    runner().setDrawingAids(aids);
    runner().click({286, -32}, PickAction::Add);  // on the arc, far from its points
    EXPECT_EQ(points().back(), (Vec2{286, -32}));

    aids.snapModes = {false, false, false};  // object snap on, every mode off
    runner().setDrawingAids(aids);
    runner().click({3, 4}, PickAction::Add);
    EXPECT_EQ(points().back(), (Vec2{3, 4}));
}

TEST_F(DrawingAidsTest, TheCentreSnapsOnlyPassingOverTheArc) {
    runner().submitText("POINT");
    runner().click({286, -32}, PickAction::Add);  // about 1 from the arc
    EXPECT_EQ(points().back(), (Vec2{250, 0}));

    runner().click({251, -2}, PickAction::Add);  // near the centre, 48 from the arc
    EXPECT_EQ(points().back(), (Vec2{251, -2}));

    runner().click({252, -48}, PickAction::Add);  // the midpoint is nearer
    EXPECT_EQ(points().back(), (Vec2{250, -50}));
}

TEST_F(DrawingAidsTest, OrthoWithoutALastPointLeavesTheClick) {
    setAids(false, true);
    runner().submitText("POINT");
    runner().click({30, 40}, PickAction::Add);
    EXPECT_EQ(points(), (Points{{30, 40}}));
}

TEST_F(DrawingAidsTest, OrthoKeepsClicksOnTheAxesOfTheLastPoint) {
    setAids(false, true);
    runner().submitText("POINT");
    runner().click({30, 40}, PickAction::Add);
    runner().click({70, 52}, PickAction::Add);  // nearer to the horizontal
    runner().click({75, 90}, PickAction::Add);  // nearer to the vertical
    runner().click({35, 55}, PickAction::Add);  // at 45 degrees: horizontal
    EXPECT_EQ(points(), (Points{{30, 40}, {70, 40}, {70, 90}, {35, 90}}));
}

// Issue #64: ortho belongs to the running command, as in AutoCAD.
TEST_F(DrawingAidsTest, OrthoDoesNotAlignTheFirstClickOfANewCommand) {
    setAids(false, true);
    runner().submitText("POINT");
    runner().click({30, 40}, PickAction::Add);
    runner().click({70, 52}, PickAction::Add);
    runner().submitText("");

    runner().submitText("POINT");
    runner().click({100, 90}, PickAction::Add);  // off both axes of (70,40)
    runner().click({130, 95}, PickAction::Add);  // ortho from (100,90) again
    EXPECT_EQ(points(), (Points{{30, 40}, {70, 40}, {100, 90}, {130, 90}}));
}

TEST_F(DrawingAidsTest, OrthoDoesNotAlignThePreviewOfTheFirstPoint) {
    setAids(false, true);
    runner().submitText("POINT");
    runner().submit(PointInput{{10, 10}});
    runner().submitText("");

    runner().submitText("POINT");
    runner().pointerMoved({40, 25});
    EXPECT_EQ(previewPoint(), (Vec2{40, 25}));
    setAids(false, false);
    setAids(false, true);  // turned on again while asking for the first point
    EXPECT_EQ(previewPoint(), (Vec2{40, 25}));

    runner().submit(PointInput{{40, 25}});
    runner().pointerMoved({70, 30});
    EXPECT_EQ(previewPoint(), (Vec2{70, 25}));
}

TEST_F(DrawingAidsTest, TypedPointsOfANewCommandStillStartFromTheLastPoint) {
    setAids(false, true);
    runner().submitText("POINT");
    runner().submit(PointInput{{30, 40}});
    runner().submitText("");

    runner().submitText("POINT");
    runner().submitText("@20,0");
    runner().submitText("");
    ASSERT_EQ(points().back(), (Vec2{50, 40}));

    // A distance typed alone is measured from the last point, so ortho takes
    // its direction from there, even for the first point of a command.
    runner().submitText("POINT");
    runner().pointerMoved({90, 52});  // from (50,40): nearer to the horizontal
    runner().submitText("10");
    runner().submitText("");
    ASSERT_EQ(points().back(), (Vec2{60, 40}));

    runner().submitText("POINT");
    runner().submitText("@50<90");
    EXPECT_NEAR(points().back().x, 60.0, 1e-9);
    EXPECT_NEAR(points().back().y, 90.0, 1e-9);
    EXPECT_EQ(messages(), std::vector<std::string>{});
}

TEST_F(DrawingAidsTest, SnapWinsOverOrtho) {
    setAids(true, true);
    runner().submitText("POINT");
    runner().submit(PointInput{{60, 60}});

    runner().click({97, 4}, PickAction::Add);  // 5 from (100,0)
    EXPECT_EQ(points().back(), (Vec2{100, 0}));

    runner().click({140, 30}, PickAction::Add);  // no snap: ortho from (100,0)
    EXPECT_EQ(points().back(), (Vec2{140, 0}));
}

TEST_F(DrawingAidsTest, ThePreviewFollowsSnapAndOrtho) {
    runner().submitText("POINT");
    runner().pointerMoved({3, 4});
    EXPECT_EQ(commandPointer(), (Vec2{0, 0}));
    EXPECT_EQ(previewPoint(), (Vec2{0, 0}));

    runner().pointerMoved({20, 30});  // nothing near
    EXPECT_EQ(previewPoint(), (Vec2{20, 30}));

    runner().submit(PointInput{{10, 10}});
    setAids(true, true);
    runner().pointerMoved({40, 25});
    EXPECT_EQ(previewPoint(), (Vec2{40, 10}));
}

TEST_F(DrawingAidsTest, NewSettingsReachThePreviewAtOnce) {
    runner().submitText("POINT");
    runner().pointerMoved({3, 4});
    ASSERT_EQ(previewPoint(), (Vec2{0, 0}));

    setAids(false, false);
    EXPECT_EQ(previewPoint(), (Vec2{3, 4}));

    runner().submit(PointInput{{60, 60}});
    setAids(false, true);  // 57 across, 56 down: horizontal
    EXPECT_EQ(previewPoint(), (Vec2{3, 60}));

    const std::size_t moves = journal().pointerPositions.size();
    setAids(false, true);  // nothing changed: the command is not told
    EXPECT_EQ(journal().pointerPositions.size(), moves);
}

TEST_F(DrawingAidsTest, ANewPixelSizeReachesThePreviewAtOnce) {
    runner().submitText("POINT");
    runner().pointerMoved({15, 4});  // 15.5 from (0,0): outside 10 pixels of 1
    ASSERT_EQ(previewPoint(), (Vec2{15, 4}));
    EXPECT_EQ(runner().activeSnap(), std::nullopt);

    runner().setPixelSize(2.0);  // the aperture is now 20 units
    EXPECT_EQ(previewPoint(), (Vec2{0, 0}));
    EXPECT_EQ(runner().activeSnap(), (core::SnapPoint{{0, 0}, core::SnapKind::Endpoint}));

    runner().click({15, 4}, PickAction::Add);
    EXPECT_EQ(points().back(), (Vec2{0, 0}));
}

TEST_F(DrawingAidsTest, ActiveSnapOnlyWhileACommandAsksForAPoint) {
    runner().pointerMoved({3, 4});
    EXPECT_EQ(runner().activeSnap(), std::nullopt);  // idle

    runner().submitText("PICK");
    ASSERT_EQ(runner().promptText(), "Select objects:");
    runner().pointerMoved({3, 4});
    EXPECT_EQ(runner().activeSnap(), std::nullopt);  // selecting
    runner().submit(CancelInput{});

    runner().submitText("POINT");
    // The pointer has not moved since: the snap is where it already is.
    EXPECT_EQ(runner().activeSnap(), (core::SnapPoint{{0, 0}, core::SnapKind::Endpoint}));
    runner().pointerMoved({48, -2});
    EXPECT_EQ(runner().activeSnap(), (core::SnapPoint{{50, 0}, core::SnapKind::Midpoint}));
    runner().pointerMoved({286, -32});
    EXPECT_EQ(runner().activeSnap(), (core::SnapPoint{{250, 0}, core::SnapKind::Center}));
    runner().pointerMoved({20, 30});
    EXPECT_EQ(runner().activeSnap(), std::nullopt);  // nothing near

    runner().pointerMoved({3, 4});
    setAids(false, false);
    EXPECT_EQ(runner().activeSnap(), std::nullopt);  // object snap off
    setAids(true, false);

    runner().submitText("");  // the command ends
    EXPECT_EQ(runner().activeSnap(), std::nullopt);
}

TEST_F(DrawingAidsTest, ActiveSnapNeedsAPointerPosition) {
    runner().submitText("POINT");
    EXPECT_EQ(runner().activeSnap(), std::nullopt);
}

TEST_F(DrawingAidsTest, SelectionClicksAreNeverCorrected) {
    setAids(true, true);
    runner().click({3, 4}, PickAction::Add);  // 4 from the line: void
    EXPECT_EQ(runner().selectionRectangle(), (SelectionRectangle{{3, 4}, {3, 4}, false}));
    runner().pointerMoved({97, 4});
    EXPECT_EQ(runner().selectionRectangle(), (SelectionRectangle{{3, 4}, {97, 4}, false}));
    runner().submit(CancelInput{});

    runner().submitText("PICK");
    runner().click({3, 4}, PickAction::Add);
    EXPECT_EQ(runner().selectionRectangle(), (SelectionRectangle{{3, 4}, {3, 4}, false}));
}

TEST_F(DrawingAidsTest, DistanceTypedAloneFollowsTheOrtho) {
    setAids(false, true);
    runner().submitText("POINT");
    runner().submit(PointInput{{10, 10}});
    runner().pointerMoved({40, 20});  // nearer to the horizontal

    runner().submitText("50");
    EXPECT_EQ(points().back(), (Vec2{60, 10}));

    runner().pointerMoved({50, 90});  // from (60,10): nearer to the vertical
    runner().submitText("30");
    EXPECT_EQ(points().back(), (Vec2{60, 40}));
}

TEST_F(DrawingAidsTest, DistanceTypedAloneFollowsTheSnap) {
    runner().submitText("POINT");
    runner().submit(PointInput{{100, 50}});
    runner().pointerMoved({97, 4});  // snaps to (100,0): straight down

    runner().submitText("20");
    EXPECT_EQ(points().back(), (Vec2{100, 30}));
}

TEST_F(DrawingAidsTest, TypedCoordinatesAreNotCorrected) {
    setAids(true, true);
    runner().submitText("POINT");
    runner().submit(PointInput{{60, 60}});
    runner().pointerMoved({97, 4});

    runner().submitText("3,4");  // 5 from (0,0)
    runner().submitText("@40,12");
    runner().submitText("@2<0");
    runner().submitText("98,1");

    EXPECT_EQ(points(), (Points{{60, 60}, {3, 4}, {43, 16}, {45, 16}, {98, 1}}));
    EXPECT_EQ(messages(), std::vector<std::string>{});
}

TEST_F(DrawingAidsTest, SubmittedPointsAreNotCorrected) {
    setAids(true, true);
    runner().submitText("POINT");
    runner().submit(PointInput{{3, 4}});
    runner().submit(PointInput{{97, 50}});
    EXPECT_EQ(points(), (Points{{3, 4}, {97, 50}}));
}

// FakeCommand sets the last point but declares no base point.
TEST_F(CommandRunnerTest, OrthoNeedsABasePointOfTheCommand) {
    DrawingAids aids;
    aids.objectSnap = false;
    aids.ortho = true;
    runner().setDrawingAids(aids);
    runner().submitText("FAKE");
    runner().click({30, 40}, PickAction::Add);
    runner().click({70, 52}, PickAction::Add);
    EXPECT_EQ(journal().inputs, (std::vector<Input>{pointInput(30, 40), pointInput(70, 52)}));
}

// --- Value and text prompts (docs/adr/0014 and docs/adr/0015) --------------------

struct PromptJournal {
    PromptKind kind{PromptKind::Point};
    std::vector<Input> inputs;
    std::vector<Vec2> pointerPositions;
    std::vector<CommandDefaults> defaults;
    int cancellations{0};
    bool keepOnCancel{false};
    bool needsSelection{false};
};

/// A fixed base different from lastPoint detects accidental ortho alignment
/// to a previous point. Only the preserving command adds entities, so they
/// cannot interfere with the snap tests.
class FakePromptCommand final : public Command {
public:
    explicit FakePromptCommand(PromptJournal& journal) : m_journal(journal) {}

    void start(CommandContext& context) override {
        context.editor.beginTransaction("Prompt");
        context.lastPoint = Vec2{100, 200};
        m_journal.defaults.push_back(context.defaults);
    }

    Status handle(CommandContext& context, const Input& input) override {
        m_journal.inputs.push_back(input);
        if (std::holds_alternative<ConfirmInput>(input)) {
            context.editor.commit();
            return Status::Finished;
        }
        if (const auto* keyword = std::get_if<KeywordInput>(&input);
            keyword != nullptr && keyword->keyword == "Remember") {
            context.defaults = {7.5, -core::kPi / 2.0, 12.0};
        }
        if (const auto* point = std::get_if<PointInput>(&input);
            point != nullptr && m_journal.keepOnCancel) {
            context.editor.add(line({10, 20}, point->point));
        }
        return Status::Continue;
    }

    void cancel(CommandContext& context) override {
        ++m_journal.cancellations;
        if (m_journal.keepOnCancel) {
            context.editor.commit();
        } else {
            Command::cancel(context);
        }
    }

    void pointerMoved(const Vec2& point) override { m_journal.pointerPositions.push_back(point); }

    [[nodiscard]] Prompt prompt() const override {
        return {
            "Test prompt [Other/Remember]:", {{"Other", "O"}, {"Remember", "R"}}, m_journal.kind};
    }

    [[nodiscard]] std::span<const Entity> preview() const override { return {}; }

    [[nodiscard]] std::optional<Vec2> basePoint() const override { return Vec2{10, 20}; }

    [[nodiscard]] bool needsSelection() const override { return m_journal.needsSelection; }

private:
    PromptJournal& m_journal;
};

/// The description of FakePromptCommand: its prompt changes kind, so only
/// the options describe its value.
CommandDescription promptDescription() {
    return {"PROMPT",
            {},
            "Asks one question of any kind.",
            false,
            {{"value",
              "Test prompt [Other/Remember]:",
              ValueType::Keyword,
              {{"Other", "O"}, {"Remember", "R"}},
              true}}};
}

class PromptRunnerTest : public testing::Test {
protected:
    PromptRunnerTest() {
        m_document.add(line({0, 0}, {100, 0}));
        m_registry.add(promptDescription(),
                       [this] { return std::make_unique<FakePromptCommand>(m_journal); });
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    void start(PromptKind kind) {
        m_journal.kind = kind;
        m_runner.submitText("PROMPT");
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] PromptJournal& journal() { return m_journal; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] const Document& document() const { return m_document; }

private:
    Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    PromptJournal m_journal;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

class NumericPromptTest : public PromptRunnerTest,
                          public testing::WithParamInterface<PromptKind> {};

TEST_P(NumericPromptTest, BareNumbersAreValuesWithoutAPointerDirection) {
    start(GetParam());
    const double factor = GetParam() == PromptKind::Angle ? core::kPi / 180.0 : 1.0;
    for (const char* text : {"50", " -2.5 ", "+3E1", ".5", "0", "-0"}) {
        runner().submitText(text);
    }
    ASSERT_EQ(journal().inputs.size(), 6U);
    const std::vector<double> expected{50, -2.5, 30, 0.5, 0, 0};
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto* value = std::get_if<ValueInput>(&journal().inputs[i]);
        ASSERT_NE(value, nullptr);
        EXPECT_DOUBLE_EQ(value->value, expected[i] * factor);
    }
    EXPECT_TRUE(messages().empty());
    EXPECT_FALSE(runner().acceptsText());
}

TEST_P(NumericPromptTest, CoordinatesRemainExactPoints) {
    start(GetParam());
    DrawingAids aids;
    aids.ortho = true;
    runner().setDrawingAids(aids);
    runner().submitText("3,4");
    runner().submitText("@5,-2");
    runner().submitText("5<0");
    runner().submitText("@5<0");
    runner().submit(PointInput{{3, 4}});
    EXPECT_EQ(journal().inputs,
              (std::vector<Input>{pointInput(3, 4), pointInput(105, 198), pointInput(5, 0),
                                  pointInput(105, 200), pointInput(3, 4)}));
}

TEST_P(NumericPromptTest, ClickAndPointerUseSnapBeforeOrtho) {
    start(GetParam());
    DrawingAids aids;
    aids.ortho = true;
    runner().setDrawingAids(aids);
    runner().pointerMoved({3, 4});
    runner().click({3, 4}, PickAction::Add);
    EXPECT_EQ(journal().inputs.back(), pointInput(0, 0));
    ASSERT_FALSE(journal().pointerPositions.empty());
    EXPECT_EQ(journal().pointerPositions.back(), (Vec2{0, 0}));
    EXPECT_EQ(runner().activeSnap(), (core::SnapPoint{{0, 0}, core::SnapKind::Endpoint}));

    // With no snap the base is (10,20), not the last point (100,200).
    runner().pointerMoved({60, 35});
    runner().click({60, 35}, PickAction::Remove);
    EXPECT_EQ(journal().inputs.back(), pointInput(60, 20));
    EXPECT_EQ(journal().pointerPositions.back(), (Vec2{60, 20}));
    EXPECT_EQ(runner().activeSnap(), std::nullopt);

    aids.ortho = false;
    runner().setDrawingAids(aids);
    EXPECT_EQ(journal().pointerPositions.back(), (Vec2{60, 35}));
    runner().click({60, 35}, PickAction::Add);
    EXPECT_EQ(journal().inputs.back(), pointInput(60, 35));
}

TEST_P(NumericPromptTest, KeywordsStillMatchNamesAndShortcuts) {
    start(GetParam());
    for (const char* text : {"Other", "other", " O ", "o"}) {
        runner().submitText(text);
        EXPECT_EQ(journal().inputs.back(), Input{KeywordInput{"Other"}});
    }
    EXPECT_TRUE(messages().empty());
}

TEST_P(NumericPromptTest, InvalidInputReportsThePromptKindAndKeepsTheQuestion) {
    start(GetParam());
    for (const char* text : {"abc", "1;2", "1 2", "nan", "inf", "0x10", "1e400", "@5"}) {
        runner().submitText(text);
        EXPECT_EQ(messages().back(), GetParam() == PromptKind::Distance
                                         ? "Requires a distance, a point or an option."
                                         : "Requires an angle, a point or an option.");
        EXPECT_EQ(runner().promptText(), "Test prompt [Other/Remember]:");
        EXPECT_TRUE(runner().isActive());
    }
    EXPECT_TRUE(journal().inputs.empty());
}

TEST_P(NumericPromptTest, EnterIsAConfirmationAndEscCallsCancel) {
    start(GetParam());
    runner().submitText("  ");
    EXPECT_EQ(journal().inputs, (std::vector<Input>{ConfirmInput{}}));
    EXPECT_FALSE(runner().isActive());

    start(GetParam());
    runner().submit(CancelInput{});
    EXPECT_EQ(journal().cancellations, 1);
    EXPECT_EQ(journal().inputs.size(), 1U);  // Esc never reaches handle().
    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_EQ(messages().back(), "*Cancel*");
}

INSTANTIATE_TEST_SUITE_P(ValuePrompts, NumericPromptTest,
                         testing::Values(PromptKind::Distance, PromptKind::Angle));

TEST_F(PromptRunnerTest, LargeFiniteAnglesDoNotOverflowDuringConversion) {
    start(PromptKind::Angle);
    runner().submitText("1e308");
    ASSERT_EQ(journal().inputs.size(), 1U);
    const auto* value = std::get_if<ValueInput>(&journal().inputs.front());
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(std::isfinite(value->value));
    EXPECT_DOUBLE_EQ(value->value, 1e308 * (core::kPi / 180.0));
}

TEST_F(PromptRunnerTest, TextPreservesSpacesAndUtf8WithoutParsing) {
    start(PromptKind::Text);
    for (const char* text : {"  Hello world  ", " \t ", "10,20", "@5<90", "Other", "o", "50", "U",
                             "Caffè, Ø e 日本語 %%c"}) {
        runner().submitText(text);
        EXPECT_EQ(journal().inputs.back(), Input{TextInput{text}});
        EXPECT_TRUE(runner().acceptsText());
    }
    EXPECT_TRUE(messages().empty());
}

TEST_F(PromptRunnerTest, OnlyAnEmptyTextLineConfirms) {
    start(PromptKind::Text);
    runner().submitText(" ");
    ASSERT_EQ(journal().inputs, (std::vector<Input>{TextInput{" "}}));
    runner().submitText("");
    EXPECT_EQ(journal().inputs.back(), Input{ConfirmInput{}});
    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(runner().acceptsText());
}

TEST_F(PromptRunnerTest, TextDoesNotAcceptMousePointsOrShowSnap) {
    start(PromptKind::Text);
    runner().pointerMoved({3, 4});
    runner().click({3, 4}, PickAction::Add);
    runner().setPixelSize(2.0);
    DrawingAids aids;
    aids.ortho = true;
    runner().setDrawingAids(aids);
    EXPECT_TRUE(journal().inputs.empty());
    EXPECT_TRUE(journal().pointerPositions.empty());
    EXPECT_EQ(runner().activeSnap(), std::nullopt);
    EXPECT_EQ(runner().selectionRectangle(), std::nullopt);
}

TEST_F(PromptRunnerTest, AcceptsTextTracksTheCurrentPromptAndCommandLifetime) {
    EXPECT_FALSE(runner().acceptsText());
    start(PromptKind::Distance);
    EXPECT_FALSE(runner().acceptsText());
    journal().kind = PromptKind::Text;
    EXPECT_TRUE(runner().acceptsText());
    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().acceptsText());
    EXPECT_EQ(journal().cancellations, 1);
    EXPECT_TRUE(journal().inputs.empty());
    EXPECT_EQ(messages().back(), "*Cancel*");
}

TEST_F(PromptRunnerTest, SelectionDoesNotExposeTheTextPromptOrCallCancelBeforeStart) {
    journal().needsSelection = true;
    start(PromptKind::Text);
    EXPECT_FALSE(runner().acceptsText());
    EXPECT_EQ(runner().promptText(), "Select objects:");
    runner().click({200, 300}, PickAction::Add);
    runner().submit(CancelInput{});  // close only the rectangle
    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(journal().cancellations, 0);
    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(journal().cancellations, 0);
    EXPECT_TRUE(journal().defaults.empty());  // not started
}

TEST_F(PromptRunnerTest, DefaultsStartFreshAndSurviveCommandsAndCancellation) {
    start(PromptKind::Distance);
    ASSERT_EQ(journal().defaults.size(), 1U);
    EXPECT_DOUBLE_EQ(journal().defaults.front().textHeight, 2.5);
    EXPECT_DOUBLE_EQ(journal().defaults.front().textRotation, 0.0);
    EXPECT_EQ(journal().defaults.front().circleRadius, std::nullopt);
    runner().submitText("Remember");
    runner().submitText("");

    start(PromptKind::Angle);
    runner().submit(CancelInput{});
    start(PromptKind::Text);
    ASSERT_EQ(journal().defaults.size(), 3U);
    for (std::size_t i = 1; i < journal().defaults.size(); ++i) {
        EXPECT_DOUBLE_EQ(journal().defaults[i].textHeight, 7.5);
        EXPECT_DOUBLE_EQ(journal().defaults[i].textRotation, -core::kPi / 2.0);
        EXPECT_EQ(journal().defaults[i].circleRadius, 12.0);
    }

    // A separate runner is a new session, even with the same editor.
    CommandRegistry registry;
    registry.add(promptDescription(),
                 [this] { return std::make_unique<FakePromptCommand>(journal()); });
    runner().submit(CancelInput{});
    CommandRunner fresh{registry, editor()};
    fresh.submitText("PROMPT");
    EXPECT_DOUBLE_EQ(journal().defaults.back().textHeight, 2.5);
    EXPECT_DOUBLE_EQ(journal().defaults.back().textRotation, 0.0);
    EXPECT_EQ(journal().defaults.back().circleRadius, std::nullopt);
    fresh.submit(CancelInput{});
}

TEST_F(PromptRunnerTest, EscCanKeepCompletedEntitiesAsOneUndoStep) {
    journal().keepOnCancel = true;
    start(PromptKind::Point);
    runner().submit(PointInput{{30, 40}});
    runner().submit(PointInput{{50, 60}});
    ASSERT_EQ(document().entityCount(), 3U);  // two new entities and the snap fixture
    const core::EntityId first = document().entities()[1].id;
    const core::EntityId second = document().entities()[2].id;
    std::optional<bool> activeInListener;
    editor().setChangeListener([&] { activeInListener = runner().isActive(); });
    runner().submit(CancelInput{});

    EXPECT_EQ(activeInListener, false);
    EXPECT_EQ(journal().cancellations, 1);
    EXPECT_EQ(journal().inputs.size(), 2U);
    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(editor().inTransaction());
    EXPECT_EQ(document().entityCount(), 3U);
    EXPECT_EQ(editor().undoName(), "Prompt");
    EXPECT_EQ(messages().back(), "*Cancel*");
    runner().submitText("U");
    EXPECT_EQ(document().entityCount(), 1U);
    runner().submitText("REDO");
    EXPECT_EQ(document().entityCount(), 3U);
    EXPECT_NE(document().find(first), nullptr);
    EXPECT_NE(document().find(second), nullptr);
}

// --- Description check (docs/adr/0026, point 3) ------------------------------

/// FAKE registered with a description that knows only the start of
/// FakeCommand's question, and a FAKE2 that says the right text with other
/// options.
class DescriptionCheckTest : public testing::Test {
protected:
    DescriptionCheckTest() {
        CommandDescription wrongText = fakeDescription("FAKE", {});
        wrongText.prompts.front().text = "Specify a point or [Close/Fail]:";
        m_registry.add(std::move(wrongText),
                       [this] { return std::make_unique<FakeCommand>(m_journal); });
        CommandDescription wrongKeywords = fakeDescription("FAKE2", {});
        wrongKeywords.prompts.front().keywords = {{"Fail", "F"}, {"Close", "C"}};
        m_registry.add(std::move(wrongKeywords),
                       [this] { return std::make_unique<FakeCommand>(m_journal); });
        m_runner.setDescriptionCheck(
            [this](const std::string& message) { m_reports.push_back(message); });
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const std::vector<std::string>& reports() const { return m_reports; }

private:
    Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    Journal m_journal;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_reports;
};

TEST_F(DescriptionCheckTest, AnUndescribedPromptCallsTheCheckAtStartAndAfterEachInput) {
    runner().submitText("FAKE");
    ASSERT_EQ(reports().size(), 1U);
    EXPECT_EQ(reports().back(), "FAKE: undescribed prompt \"Specify point or [Close/Fail]:\"");

    runner().submit(pointInput(1, 2));
    EXPECT_EQ(reports().size(), 2U);
    runner().submitText("C");  // the command ends: no prompt to check
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(reports().size(), 2U);
}

TEST_F(DescriptionCheckTest, KeywordsInAnotherOrderAreUndescribed) {
    runner().submitText("fake2");
    ASSERT_EQ(reports().size(), 1U);
    EXPECT_EQ(reports().back(), "FAKE2: undescribed prompt \"Specify point or [Close/Fail]:\"");
    runner().submit(CancelInput{});
}

TEST_F(DescriptionCheckTest, TheRunnerPromptsAreNotChecked) {
    runner().submitText("");  // Enter with no command
    runner().submit(CancelInput{});
    EXPECT_EQ(runner().promptText(), "Command:");
    EXPECT_TRUE(reports().empty());
}

TEST_F(DescriptionCheckTest, AnEmptyCheckTurnsTheCheckOff) {
    runner().setDescriptionCheck({});
    runner().submitText("FAKE");
    runner().submit(pointInput(1, 2));
    runner().submit(CancelInput{});
    EXPECT_TRUE(reports().empty());
}

TEST_F(CommandRunnerTest, DescribedPromptsPassTheDefaultCheck) {
    // The default check aborts: reaching the end is the test.
    runner().submitText("FAKE");
    runner().submit(pointInput(1, 2));
    runner().submitText("C");
    EXPECT_FALSE(runner().isActive());
}

TEST(DescriptionCheckDeathTest, TheDefaultCheckAborts) {
    Journal journal;
    Document document;
    edit::Editor editor{document};
    CommandRegistry registry;
    registry.add({"FAKE", {}, "Asks nothing it describes.", false, {}},
                 [&journal] { return std::make_unique<FakeCommand>(journal); });
    CommandRunner runner{registry, editor};
    EXPECT_DEATH(runner.submitText("FAKE"), "FAKE: undescribed prompt");
}

}  // namespace
}  // namespace tg::cmd
