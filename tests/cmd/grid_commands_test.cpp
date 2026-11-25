// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/grid_commands.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/cmd/input.hpp"
#include "tg/core/document.hpp"
#include "tg/core/drawing_settings.hpp"
#include "tg/core/vec2.hpp"
#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

using core::Vec2;
using Lines = std::vector<std::string>;

// The commands of docs/tappe/0.8.1/PT28-griglia-cmd.md, point 1, with the
// history of the command line written as a script writes it: the prompt
// and what is typed, then the messages.
class GridCommandsTest : public ::testing::Test {
public:
    GridCommandsTest()
        : document(core::Document::newDrawing()), editor(document), runner(registry, editor) {
        registerBuiltinCommands(registry);
        runner.setMessageSink(
            [this](std::string message) { history.push_back(std::move(message)); });
    }

    /// Types `lines`, and returns the history they wrote.
    Lines type(std::initializer_list<const char*> lines) {
        history.clear();
        for (const std::string line : lines) {
            std::string echo = runner.promptText();
            if (!line.empty()) {
                echo += " ";
                echo += line;
            }
            history.push_back(std::move(echo));
            runner.submitText(line);
        }
        return history;
    }

    [[nodiscard]] const core::GridSettings& grid() const { return document.grid(); }

    core::Document document;
    edit::Editor editor;
    CommandRegistry registry;
    CommandRunner runner;
    Lines history;
};

constexpr const char* kGridPrompt10 =
    "Specify grid spacing(X) or [ON/OFF/Snap/Major/aDaptive/Aspect] <10.0000>:";
constexpr const char* kGridPromptA =
    "Specify grid spacing(X) or [ON/OFF/Snap/Major/aDaptive/Aspect] <A>:";

// --- The grid node (docs/REGOLE-NUMERICHE.md, section 3, point 6) ------------

TEST(GridNode, NearestNodeAxisByAxisWithBaseAndUnequalSpacings) {
    core::GridSettings grid;
    grid.snapUnit = {2.5, 5};
    grid.snapBase = {3, 4};
    // (-4.2 / 2.5 = -1.68, -3.7 / 5 = -0.74): one step back on each axis.
    EXPECT_EQ(gridNode(grid, {-1.2, 0.3}), (Vec2{-2, -1}));
    EXPECT_EQ(gridNode(grid, {3.1, 3.9}), (Vec2{3, 4}));
    EXPECT_EQ(gridNode(grid, {-8.9, -21.4}), (Vec2{-9.5, -21}));
}

TEST(GridNode, AnExactTieGoesAwayFromTheBase) {
    core::GridSettings grid;
    grid.snapUnit = {2.5, 5};
    grid.snapBase = {3, 4};
    // Half way on both axes, above and below the base.
    EXPECT_EQ(gridNode(grid, {4.25, 6.5}), (Vec2{5.5, 9}));
    EXPECT_EQ(gridNode(grid, {1.75, 1.5}), (Vec2{0.5, -1}));
    const core::GridSettings plain;
    EXPECT_EQ(gridNode(plain, {-5, 5}), (Vec2{-10, 10}));
    EXPECT_EQ(gridNode(plain, {4.9, 4.9}), (Vec2{0, 0}));
    EXPECT_EQ(gridNode(plain, {19, 4}), (Vec2{20, 0}));
}

// --- GRID ------------------------------------------------------------------

TEST_F(GridCommandsTest, GridSpacingNegativeAndMultipleOfTheSnap) {
    core::GridSettings off = grid();
    off.gridOn = false;
    editor.tables().setGrid(off);
    EXPECT_EQ(type({"GRID", "-2", "2x"}),
              (Lines{"Command: GRID", std::string{kGridPrompt10} + " -2", "Value must be positive.",
                     std::string{kGridPrompt10} + " 2x"}));
    EXPECT_EQ(grid().gridUnit, (Vec2{20, 20}));
    EXPECT_TRUE(grid().gridOn);
    EXPECT_EQ(type({"GRID"}), (Lines{"Command: GRID"}));
    EXPECT_EQ(runner.promptText(),
              "Specify grid spacing(X) or [ON/OFF/Snap/Major/aDaptive/Aspect] <20.0000>:");
}

TEST_F(GridCommandsTest, GridEnterSwitchesTheGridOnAndZeroFollowsTheSnap) {
    type({"GRID", "OFF"});
    EXPECT_FALSE(grid().gridOn);
    type({"GRID", ""});
    EXPECT_TRUE(grid().gridOn);
    EXPECT_EQ(grid().gridUnit, (Vec2{10, 10}));
    type({"GRID", "OFF", "GRID", "0"});
    EXPECT_TRUE(grid().gridOn);
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 0}));
    type({"GRID", "4", "GRID", "S"});
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 0}));
    type({"GRID", "on"});
    EXPECT_TRUE(grid().gridOn);
}

TEST_F(GridCommandsTest, GridAspectWithAZeroAxisProposesA) {
    EXPECT_EQ(type({"GRID", "Aspect", "0", "10", "GRID", "Aspect"}),
              (Lines{"Command: GRID", std::string{kGridPrompt10} + " Aspect",
                     "Specify the horizontal spacing(X) <10.0000>: 0",
                     "Specify the vertical spacing(Y) <10.0000>: 10", "Command: GRID",
                     std::string{kGridPromptA} + " Aspect"}));
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 10}));
    // Aspect proposes 0 for the axis at 0.
    EXPECT_EQ(runner.promptText(), "Specify the horizontal spacing(X) <0.0000>:");
    EXPECT_EQ(type({"-1", "5", ""}),
              (Lines{"Specify the horizontal spacing(X) <0.0000>: -1", "Value must be positive.",
                     "Specify the horizontal spacing(X) <0.0000>: 5",
                     "Specify the vertical spacing(Y) <10.0000>:"}));
    EXPECT_EQ(grid().gridUnit, (Vec2{5, 10}));
}

TEST_F(GridCommandsTest, GridSnapLeavesGridmodeAsItIs) {
    // From a grid that is off: Snap sets GRIDUNIT to 0,0 and does not switch
    // the grid on.
    core::GridSettings off = grid();
    off.gridOn = false;
    off.gridUnit = {4, 4};
    editor.tables().setGrid(off);
    editor.markSaved();
    EXPECT_EQ(
        type({"GRID", "S"}),
        (Lines{"Command: GRID",
               "Specify grid spacing(X) or [ON/OFF/Snap/Major/aDaptive/Aspect] <4.0000>: S"}));
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 0}));
    EXPECT_FALSE(grid().gridOn);
    EXPECT_TRUE(editor.isModified());
    // From a grid that is on: it stays on.
    type({"GRID", "4", "GRID", "Snap"});
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 0}));
    EXPECT_TRUE(grid().gridOn);
}

TEST_F(GridCommandsTest, GridAspectSwitchesTheGridOn) {
    core::GridSettings off = grid();
    off.gridOn = false;
    editor.tables().setGrid(off);
    EXPECT_EQ(type({"GRID", "A", "0", "10"}),
              (Lines{"Command: GRID", std::string{kGridPrompt10} + " A",
                     "Specify the horizontal spacing(X) <10.0000>: 0",
                     "Specify the vertical spacing(Y) <10.0000>: 10"}));
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 10}));
    EXPECT_TRUE(grid().gridOn);
    // Enter at both spacings switches it on too.
    type({"GRID", "OFF", "GRID", "A", "", ""});
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 10}));
    EXPECT_TRUE(grid().gridOn);
}

TEST_F(GridCommandsTest, GridAspectTakesAMultipleOfTheSnapAxisByAxis) {
    core::GridSettings snap = grid();
    snap.snapUnit = {2.5, 5};
    editor.tables().setGrid(snap);
    EXPECT_EQ(type({"GRID", "Aspect", "2x", "3X"}),
              (Lines{"Command: GRID", std::string{kGridPrompt10} + " Aspect",
                     "Specify the horizontal spacing(X) <10.0000>: 2x",
                     "Specify the vertical spacing(Y) <10.0000>: 3X"}));
    // 2 times the snap spacing of X, 3 times that of Y.
    EXPECT_EQ(grid().gridUnit, (Vec2{5, 15}));
    // The same "2x" gives a different spacing on each axis.
    type({"GRID", "Aspect", "2x", "2x"});
    EXPECT_EQ(grid().gridUnit, (Vec2{5, 10}));
    // Outside Aspect, too.
    type({"GRID", "2x"});
    EXPECT_EQ(grid().gridUnit, (Vec2{5, 10}));
    EXPECT_EQ(type({"GRID", "Aspect", "-1x", "0x", "1x"}),
              (Lines{"Command: GRID", std::string{kGridPromptA} + " Aspect",
                     "Specify the horizontal spacing(X) <5.0000>: -1x", "Value must be positive.",
                     "Specify the horizontal spacing(X) <5.0000>: 0x",
                     "Specify the vertical spacing(Y) <10.0000>: 1x"}));
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 5}));
}

TEST_F(GridCommandsTest, GridTextThatIsNotANumber) {
    // At the question with options: "Invalid option keyword.".
    EXPECT_EQ(
        type({"GRID", "ten", "x"}),
        (Lines{"Command: GRID", std::string{kGridPrompt10} + " ten", "Invalid option keyword.",
               std::string{kGridPrompt10} + " x", "Invalid option keyword."}));
    EXPECT_EQ(runner.promptText(), kGridPrompt10);
    runner.submit(CancelInput{});
    // At a spacing alone: "Requires numeric distance.", even for the name of
    // an option of the first question.
    EXPECT_EQ(type({"GRID", "Aspect", "ten", "S", "4", "ten", "6"}),
              (Lines{"Command: GRID", std::string{kGridPrompt10} + " Aspect",
                     "Specify the horizontal spacing(X) <10.0000>: ten",
                     "Requires numeric distance.", "Specify the horizontal spacing(X) <10.0000>: S",
                     "Requires numeric distance.", "Specify the horizontal spacing(X) <10.0000>: 4",
                     "Specify the vertical spacing(Y) <10.0000>: ten", "Requires numeric distance.",
                     "Specify the vertical spacing(Y) <10.0000>: 6"}));
    EXPECT_EQ(grid().gridUnit, (Vec2{4, 6}));
    // At the Yes/No questions of aDaptive: "Invalid option keyword.".
    EXPECT_EQ(type({"GRID", "D", "maybe", "Y", "maybe", "Y"}),
              (Lines{"Command: GRID", std::string{kGridPromptA} + " D",
                     "Turn adaptive behavior on [Yes/No] <Yes>: maybe", "Invalid option keyword.",
                     "Turn adaptive behavior on [Yes/No] <Yes>: Y",
                     "Allow subdivision below grid spacing [Yes/No] <No>: maybe",
                     "Invalid option keyword.",
                     "Allow subdivision below grid spacing [Yes/No] <No>: Y"}));
    EXPECT_EQ(grid().gridDisplay, 7);
    // At Major, which asks an integer: the message of its range.
    EXPECT_EQ(type({"GRID", "M", "ten", "2.5", "8"}),
              (Lines{"Command: GRID", std::string{kGridPromptA} + " M",
                     "Enter number of grid divisions per major grid line <5>: ten",
                     "Requires an integer between 1 and 100.",
                     "Enter number of grid divisions per major grid line <5>: 2.5",
                     "Requires an integer between 1 and 100.",
                     "Enter number of grid divisions per major grid line <5>: 8"}));
    EXPECT_EQ(grid().gridMajor, 8);
}

TEST_F(GridCommandsTest, GridMajorBetweenOneAndAHundred) {
    EXPECT_EQ(type({"GRID", "Major", "0", "101", "7"}),
              (Lines{"Command: GRID", std::string{kGridPrompt10} + " Major",
                     "Enter number of grid divisions per major grid line <5>: 0",
                     "Requires an integer between 1 and 100.",
                     "Enter number of grid divisions per major grid line <5>: 101",
                     "Requires an integer between 1 and 100.",
                     "Enter number of grid divisions per major grid line <5>: 7"}));
    EXPECT_EQ(grid().gridMajor, 7);
}

TEST_F(GridCommandsTest, GridAdaptiveSetsBitsTwoAndFour) {
    EXPECT_EQ(type({"GRID", "aDaptive", "No"}),
              (Lines{"Command: GRID", std::string{kGridPrompt10} + " aDaptive",
                     "Turn adaptive behavior on [Yes/No] <Yes>: No"}));
    EXPECT_EQ(grid().gridDisplay, 1);
    EXPECT_EQ(type({"GRID", "aDaptive", "Yes", "Yes"}),
              (Lines{"Command: GRID", std::string{kGridPrompt10} + " aDaptive",
                     "Turn adaptive behavior on [Yes/No] <No>: Yes",
                     "Allow subdivision below grid spacing [Yes/No] <No>: Yes"}));
    EXPECT_EQ(grid().gridDisplay, 7);
    // No clears the subdivision too, which works only with the adaptive grid.
    type({"GRID", "D", "N"});
    EXPECT_EQ(grid().gridDisplay, 1);
    // Enter takes the proposal: Yes asks for the subdivision.
    type({"GRID", "D", "Y", "N", "GRID", "D", ""});
    EXPECT_EQ(runner.promptText(), "Allow subdivision below grid spacing [Yes/No] <No>:");
    type({""});
    EXPECT_EQ(grid().gridDisplay, 3);
}

TEST_F(GridCommandsTest, GridCancelledChangesNothing) {
    type({"GRID", "Aspect", "3"});
    runner.submit(CancelInput{});
    EXPECT_EQ(grid(), core::GridSettings{});
    EXPECT_FALSE(editor.isModified());
}

// --- SNAP ------------------------------------------------------------------

TEST_F(GridCommandsTest, SnapRefusesZeroAndNegativeSpacings) {
    EXPECT_EQ(type({"SNAP", "0", "-1", "2.5"}),
              (Lines{"Command: SNAP", "Specify snap spacing or [ON/OFF/Aspect] <10.0000>: 0",
                     "Value must be positive and nonzero.",
                     "Specify snap spacing or [ON/OFF/Aspect] <10.0000>: -1",
                     "Value must be positive and nonzero.",
                     "Specify snap spacing or [ON/OFF/Aspect] <10.0000>: 2.5"}));
    EXPECT_EQ(grid().snapUnit, (Vec2{2.5, 2.5}));
    EXPECT_TRUE(grid().snapOn);
}

TEST_F(GridCommandsTest, SnapAspectSwitchesTheSnapOnAndProposesA) {
    EXPECT_EQ(
        type({"SNAP", "Aspect", "0", "5", "2.5", "SNAP", "OFF", "SNAP"}),
        (Lines{"Command: SNAP", "Specify snap spacing or [ON/OFF/Aspect] <10.0000>: Aspect",
               "Specify horizontal spacing <10.0000>: 0", "Value must be positive and nonzero.",
               "Specify horizontal spacing <10.0000>: 5", "Specify vertical spacing <10.0000>: 2.5",
               "Command: SNAP", "Specify snap spacing or [ON/OFF/Aspect] <A>: OFF",
               "Command: SNAP"}));
    EXPECT_EQ(grid().snapUnit, (Vec2{5, 2.5}));
    EXPECT_FALSE(grid().snapOn);
    type({""});  // Enter switches it on
    EXPECT_TRUE(grid().snapOn);
    type({"SNAP", "OFF", "SNAP", "ON"});
    EXPECT_TRUE(grid().snapOn);
}

TEST_F(GridCommandsTest, SnapTextThatIsNotANumber) {
    // At the question with options: "Invalid option keyword.".
    EXPECT_EQ(type({"SNAP", "ten"}),
              (Lines{"Command: SNAP", "Specify snap spacing or [ON/OFF/Aspect] <10.0000>: ten",
                     "Invalid option keyword."}));
    EXPECT_EQ(runner.promptText(), "Specify snap spacing or [ON/OFF/Aspect] <10.0000>:");
    runner.submit(CancelInput{});
    // At a spacing alone: "Requires numeric distance.".
    EXPECT_EQ(
        type({"SNAP", "A", "ten", "ON", "5", "ten", "2.5"}),
        (Lines{"Command: SNAP", "Specify snap spacing or [ON/OFF/Aspect] <10.0000>: A",
               "Specify horizontal spacing <10.0000>: ten", "Requires numeric distance.",
               "Specify horizontal spacing <10.0000>: ON", "Requires numeric distance.",
               "Specify horizontal spacing <10.0000>: 5", "Specify vertical spacing <10.0000>: ten",
               "Requires numeric distance.", "Specify vertical spacing <10.0000>: 2.5"}));
    EXPECT_EQ(grid().snapUnit, (Vec2{5, 2.5}));
    EXPECT_TRUE(grid().snapOn);
}

// --- UCSICON -----------------------------------------------------------------

TEST_F(GridCommandsTest, UcsiconProposesTheStateAndSetsItsValues) {
    EXPECT_EQ(type({"UCSICON", "OFF", "UCSICON", "", "UCSICON", "Noorigin", "UCSICON", "ORigin",
                    "UCSICON", "ON"}),
              (Lines{"Command: UCSICON", "Enter an option [ON/OFF/Noorigin/ORigin] <ON>: OFF",
                     "Command: UCSICON", "Enter an option [ON/OFF/Noorigin/ORigin] <OFF>:",
                     "Command: UCSICON", "Enter an option [ON/OFF/Noorigin/ORigin] <OFF>: Noorigin",
                     "Command: UCSICON", "Enter an option [ON/OFF/Noorigin/ORigin] <ON>: ORigin",
                     "Command: UCSICON", "Enter an option [ON/OFF/Noorigin/ORigin] <ON>: ON"}));
    EXPECT_EQ(grid().ucsIcon, 3);
    type({"UCSICON", "OFF"});
    EXPECT_EQ(grid().ucsIcon, 2);
    type({"UCSICON", "N"});
    EXPECT_EQ(grid().ucsIcon, 1);
}

TEST_F(GridCommandsTest, UcsiconRefusesAnUnknownOption) {
    EXPECT_EQ(
        type({"UCSICON", "maybe", "OFF"}),
        (Lines{"Command: UCSICON", "Enter an option [ON/OFF/Noorigin/ORigin] <ON>: maybe",
               "Invalid option keyword.", "Enter an option [ON/OFF/Noorigin/ORigin] <ON>: OFF"}));
    EXPECT_EQ(grid().ucsIcon, 2);
}

// --- Variables -----------------------------------------------------------------

TEST_F(GridCommandsTest, GridunitAcceptsZeroOnOneAxisAndRefusesANegativeSpacing) {
    EXPECT_EQ(type({"GRIDUNIT", "0,10", "GRIDUNIT", "-1,10"}),
              (Lines{"Command: GRIDUNIT", "Enter new value for GRIDUNIT <10.0000,10.0000>: 0,10",
                     "Command: GRIDUNIT", "Enter new value for GRIDUNIT <0.0000,10.0000>: -1,10",
                     "Cannot set GRIDUNIT to that value.", "*Invalid*"}));
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 10}));
    EXPECT_EQ(runner.promptText(), "Command:");
    type({"GRIDUNIT", "0,0"});
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 0}));
    EXPECT_EQ(type({"GRIDUNIT", "ten"}),
              (Lines{"Command: GRIDUNIT", "Enter new value for GRIDUNIT <0.0000,0.0000>: ten",
                     "Cannot set GRIDUNIT to that value.", "*Invalid*"}));
}

TEST_F(GridCommandsTest, SnapunitRefusesZeroAndNegativeSpacings) {
    EXPECT_EQ(type({"SNAPUNIT", "0,5", "SNAPUNIT", "-1,5", "SNAPUNIT", "2.5,5"}),
              (Lines{"Command: SNAPUNIT", "Enter new value for SNAPUNIT <10.0000,10.0000>: 0,5",
                     "Cannot set SNAPUNIT to that value.", "*Invalid*", "Command: SNAPUNIT",
                     "Enter new value for SNAPUNIT <10.0000,10.0000>: -1,5",
                     "Cannot set SNAPUNIT to that value.", "*Invalid*", "Command: SNAPUNIT",
                     "Enter new value for SNAPUNIT <10.0000,10.0000>: 2.5,5"}));
    EXPECT_EQ(grid().snapUnit, (Vec2{2.5, 5}));
}

TEST_F(GridCommandsTest, SnapbaseModesDisplayAndMajor) {
    EXPECT_EQ(
        type({"SNAPBASE", "3,4", "SNAPMODE", "1", "GRIDMODE", "0", "GRIDDISPLAY", "6", "GRIDMAJOR",
              "101", "10"}),
        (Lines{"Command: SNAPBASE", "Enter new value for SNAPBASE <0.0000,0.0000>: 3,4",
               "Command: SNAPMODE", "Enter new value for SNAPMODE <0>: 1", "Command: GRIDMODE",
               "Enter new value for GRIDMODE <1>: 0", "Command: GRIDDISPLAY",
               "Enter new value for GRIDDISPLAY <3>: 6", "Command: GRIDMAJOR",
               "Enter new value for GRIDMAJOR <5>: 101", "Requires an integer between 1 and 100.",
               "Enter new value for GRIDMAJOR <5>: 10"}));
    EXPECT_EQ(grid().snapBase, (Vec2{3, 4}));
    EXPECT_TRUE(grid().snapOn);
    EXPECT_FALSE(grid().gridOn);
    EXPECT_EQ(grid().gridDisplay, 6);
    EXPECT_EQ(grid().gridMajor, 10);
    EXPECT_EQ(
        type({"SNAPMODE", "2", "", "GRIDDISPLAY", "16", ""}),
        (Lines{"Command: SNAPMODE", "Enter new value for SNAPMODE <1>: 2",
               "Requires an integer between 0 and 1.", "Enter new value for SNAPMODE <1>:",
               "Command: GRIDDISPLAY", "Enter new value for GRIDDISPLAY <6>: 16",
               "Requires an integer between 0 and 15.", "Enter new value for GRIDDISPLAY <6>:"}));
    EXPECT_TRUE(grid().snapOn);
    EXPECT_EQ(grid().gridDisplay, 6);
}

TEST_F(GridCommandsTest, GridmodeAndSnapmodeRequireZeroOrOne) {
    EXPECT_EQ(
        type({"GRIDMODE", "2", "-1", "on", "0.5", "0"}),
        (Lines{"Command: GRIDMODE", "Enter new value for GRIDMODE <1>: 2",
               "Requires an integer between 0 and 1.", "Enter new value for GRIDMODE <1>: -1",
               "Requires an integer between 0 and 1.", "Enter new value for GRIDMODE <1>: on",
               "Requires an integer between 0 and 1.", "Enter new value for GRIDMODE <1>: 0.5",
               "Requires an integer between 0 and 1.", "Enter new value for GRIDMODE <1>: 0"}));
    EXPECT_FALSE(grid().gridOn);
    EXPECT_EQ(
        type({"SNAPMODE", "-1", "1"}),
        (Lines{"Command: SNAPMODE", "Enter new value for SNAPMODE <0>: -1",
               "Requires an integer between 0 and 1.", "Enter new value for SNAPMODE <0>: 1"}));
    EXPECT_TRUE(grid().snapOn);
}

TEST_F(GridCommandsTest, GriddisplayFromZeroToFifteen) {
    EXPECT_EQ(
        type({"GRIDDISPLAY", "-1", "ten", "15", "GRIDDISPLAY", "0"}),
        (Lines{"Command: GRIDDISPLAY", "Enter new value for GRIDDISPLAY <3>: -1",
               "Requires an integer between 0 and 15.", "Enter new value for GRIDDISPLAY <3>: ten",
               "Requires an integer between 0 and 15.", "Enter new value for GRIDDISPLAY <3>: 15",
               "Command: GRIDDISPLAY", "Enter new value for GRIDDISPLAY <15>: 0"}));
    EXPECT_EQ(grid().gridDisplay, 0);
}

TEST_F(GridCommandsTest, TheVariablesMarkTheDrawingModifiedWithoutAnUndoStep) {
    type({"GRIDMODE", "0"});
    EXPECT_TRUE(editor.isModified());
    EXPECT_TRUE(editor.undoNames().empty());
    editor.markSaved();
    EXPECT_FALSE(editor.isModified());
    type({"GRIDUNIT", "0,10"});
    EXPECT_TRUE(editor.isModified());
    EXPECT_TRUE(editor.undoNames().empty());
    // U has nothing to undo: the grid stays.
    type({"U"});
    EXPECT_EQ(grid().gridUnit, (Vec2{0, 10}));
    EXPECT_FALSE(grid().gridOn);
}

TEST_F(GridCommandsTest, TheSameValueLeavesTheDrawingUnmodified) {
    type({"GRIDMODE", "1", "GRID", "", "SNAP", "OFF", "UCSICON", "ON"});
    EXPECT_FALSE(editor.isModified());
}

}  // namespace
}  // namespace tg::cmd
