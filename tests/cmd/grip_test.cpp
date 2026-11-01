// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// Editing with grips in the command runner (docs/adr/0034, points 1 and 3).

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/document.hpp"
#include "tg/core/grips.hpp"
#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

using core::Arc;
using core::Circle;
using core::Document;
using core::Entity;
using core::EntityId;
using core::Line;
using core::Polyline;
using core::Text;
using core::Vec2;

/// A new drawing with the built-in commands, at the default pixel size of 1:
/// the grip square has a side of 10 units, the pick box is 3 units.
class GripTest : public testing::Test {
protected:
    GripTest() : m_document(Document::newDrawing()) {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] Document& document() { return m_document; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    /// Adds `entity` outside the undo history.
    EntityId add(Entity entity) { return m_document.add(std::move(entity)); }

    [[nodiscard]] const Entity& entity(EntityId id) const {
        const core::EntityRecord* record = m_document.find(id);
        EXPECT_NE(record, nullptr);
        return record->entity;
    }

    /// A mouse click: the pointer reaches the point, then clicks there.
    void clickAt(const Vec2& point, PickAction action = PickAction::Add) {
        m_runner.pointerMoved(point);
        m_runner.click(point, action);
    }

    void select(std::vector<EntityId> ids) {
        ASSERT_TRUE(m_runner.replaceSelection(std::move(ids)));
    }

    [[nodiscard]] std::vector<EntityId> selection() const {
        const std::span<const EntityId> ids = m_runner.selection().ids();
        return {ids.begin(), ids.end()};
    }

    void setAids(bool objectSnap, bool ortho, bool polar) {
        DrawingAids aids = m_runner.drawingAids();
        aids.objectSnap = objectSnap;
        aids.ortho = ortho;
        aids.polar = polar;
        m_runner.setDrawingAids(aids);
    }

private:
    Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

using Points = std::vector<Vec2>;

/// A Text with two alignment points, as Aligned.
Text alignedText() {
    Text text;
    text.position = {600, 20};
    text.alignmentPoint = {620, 20};
    text.horizontal = core::TextHorizontal::Aligned;
    text.value = "Tg2";
    return text;
}

/// An open polyline with a straight segment and an arc (bulge 1).
Polyline polylineWithArc() {
    Polyline polyline;
    polyline.addVertex({200, 0});
    polyline.addVertex({220, 0}, 1.0);
    polyline.addVertex({240, 0});
    return polyline;
}

// --- Which grips are shown (docs/adr/0034, point 1) --------------------------

TEST_F(GripTest, ShowsTheGripsOfTheIdleSelection) {
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    const EntityId circle = add(Circle{{100, 0}, 10});
    EXPECT_TRUE(runner().grips().empty());

    clickAt({10, 0});
    EXPECT_EQ(runner().grips(), (Points{{0, 0}, {20, 0}, {10, 0}}));

    select({line, circle});
    EXPECT_EQ(
        runner().grips(),
        (Points{{0, 0}, {20, 0}, {10, 0}, {100, 0}, {110, 0}, {100, 10}, {90, 0}, {100, -10}}));
    EXPECT_FALSE(runner().hotGrip());
}

TEST_F(GripTest, ShowsNoGripWhileACommandRuns) {
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    select({line});
    runner().submitText("LINE");  // clears the selection
    EXPECT_TRUE(runner().grips().empty());
    runner().submit(CancelInput{});

    // While "Select objects:" is asked for a command.
    runner().submitText("ERASE");
    clickAt({10, 0});
    ASSERT_EQ(selection(), (std::vector<EntityId>{line}));
    EXPECT_TRUE(runner().grips().empty());
    clickAt({20, 0});  // a selection gesture, not a grip
    EXPECT_EQ(runner().promptText(), "Select objects:");
    runner().submit(CancelInput{});
}

TEST_F(GripTest, ShowsNoGripAboveTheLimit) {
    std::vector<EntityId> ids;
    for (std::size_t index = 0; index <= CommandRunner::kMaxGripEntities; ++index) {
        const double y = 10.0 * static_cast<double>(index);
        ids.push_back(add(Line{{0, y}, {20, y}}));
    }
    ASSERT_EQ(ids.size(), 101U);
    select(ids);
    EXPECT_TRUE(runner().grips().empty());

    // A click on what would be a grip is a selection gesture.
    clickAt({20, 0});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(runner().selection().size(), 101U);

    // At the limit the grips come back.
    ASSERT_TRUE(runner().replaceSelection({ids.begin(), ids.end() - 1}));
    EXPECT_EQ(runner().grips().size(), 3 * CommandRunner::kMaxGripEntities);
}

TEST_F(GripTest, AnEntityOnALockedLayerHasNoGrip) {
    core::Layer locked;
    locked.name = "Locked";
    locked.linetype = document().continuousLinetype();
    locked.locked = true;
    const core::CurrentProperties open = document().current();
    core::CurrentProperties onLocked = open;
    onLocked.layer = document().addLayer(locked);
    document().setCurrent(onLocked);
    const EntityId onLockedLayer = add(Line{{0, 0}, {20, 0}});
    document().setCurrent(open);

    // A click does not select it, so it shows no grip.
    clickAt({10, 0});
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_TRUE(runner().grips().empty());

    // Even in the selection, its grips are neither shown nor hot.
    select({onLockedLayer});
    EXPECT_TRUE(runner().grips().empty());
    clickAt({20, 0});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entity(onLockedLayer), Entity{(Line{{0, 0}, {20, 0}})});
}

// --- A grip of every entity (docs/adr/0034, points 3 and 4) ------------------

TEST_F(GripTest, ClickedPointMovesTheGripOfEveryEntity) {
    setAids(false, false, false);
    const std::vector<Entity> entities{Line{{0, 0}, {20, 0}}, Circle{{100, 0}, 10},
                                       Arc{{150, 0}, 10, 0.0, core::kPi / 2.0}, polylineWithArc(),
                                       alignedText()};
    for (const Entity& original : entities) {
        const EntityId id = add(original);
        const Points grips = core::gripPoints(original);
        for (std::size_t index = 0; index < grips.size(); ++index) {
            SCOPED_TRACE(testing::Message() << "entity " << id.value << ", grip " << index);
            select({id});
            const Vec2 grip = grips[index];
            const Vec2 target = grip + Vec2{3, 4};
            const core::GripResult expected = core::moveGrip(original, index, target);
            ASSERT_EQ(expected.outcome, core::GripOutcome::Moved);

            clickAt(grip);
            ASSERT_TRUE(runner().isActive());
            EXPECT_EQ(runner().hotGrip(), grip);
            clickAt(target);
            EXPECT_FALSE(runner().isActive());
            EXPECT_EQ(entity(id), expected.entity);
            EXPECT_EQ(editor().undoName(), "Grip edit");
            EXPECT_EQ(selection(), (std::vector<EntityId>{id}));
            ASSERT_TRUE(editor().undo());
            EXPECT_EQ(entity(id), original);
        }
    }
}

TEST_F(GripTest, TypedPointMovesTheGrip) {
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    clickAt({10, 0});
    clickAt({20, 0});
    EXPECT_EQ(messages(), (std::vector<std::string>{"** STRETCH **"}));
    EXPECT_EQ(runner().promptText(), "Specify stretch point:");
    EXPECT_EQ(runner().prompt().kind, PromptKind::Point);

    runner().submitText("25,10");
    EXPECT_EQ(entity(line), Entity{(Line{{0, 0}, {25, 10}})});
    EXPECT_EQ(editor().undoName(), "Grip edit");
    EXPECT_EQ(runner().promptText(), "Command:");
    EXPECT_EQ(selection(), (std::vector<EntityId>{line}));
    EXPECT_EQ(runner().grips(), (Points{{0, 0}, {25, 10}, {12.5, 5}}));
}

TEST_F(GripTest, RelativeCoordinatesStartFromTheHotGrip) {
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    select({line});
    clickAt({10, 0});  // the midpoint moves the whole line
    runner().submitText("@0,5");
    EXPECT_EQ(entity(line), Entity{(Line{{0, 5}, {20, 5}})});
}

TEST_F(GripTest, PreviewShowsTheModifiedEntity) {
    setAids(false, false, false);
    const EntityId circle = add(Circle{{0, 0}, 10});
    select({circle});
    clickAt({10, 0});
    ASSERT_TRUE(runner().isActive());
    EXPECT_TRUE(runner().grips().empty());

    runner().pointerMoved({0, 15});
    const std::span<const Entity> preview = runner().preview();
    ASSERT_EQ(preview.size(), 1U);
    EXPECT_EQ(preview.front(), Entity{(Circle{{0, 0}, 15})});

    // An invalid result has no preview.
    runner().pointerMoved({0, 0});
    EXPECT_TRUE(runner().preview().empty());
    runner().submit(CancelInput{});
}

// --- Drawing aids from the hot grip ------------------------------------------

TEST_F(GripTest, SnapCorrectsTheStretchPoint) {
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    add(Line{{0, 10}, {30, 10}});
    select({line});
    clickAt({20, 0});
    clickAt({29, 9});  // the endpoint (30,10) is within the snap aperture
    EXPECT_EQ(entity(line), Entity{(Line{{0, 0}, {30, 10}})});
}

TEST_F(GripTest, OrthoStartsFromTheHotGrip) {
    setAids(false, true, false);
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    select({line});
    clickAt({20, 0});
    EXPECT_EQ(runner().hotGrip(), (Vec2{20, 0}));
    clickAt({25, 3});
    EXPECT_EQ(entity(line), Entity{(Line{{0, 0}, {25, 0}})});
}

TEST_F(GripTest, PolarStartsFromTheHotGrip) {
    setAids(false, false, true);
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    select({line});
    clickAt({20, 0});
    clickAt({20.5, 10});  // 0.5 from the 90 degree ray through (20,0)
    EXPECT_EQ(entity(line), Entity{(Line{{0, 0}, {20, 10}})});
}

// --- Leaving a grip; undo and redo; invalid results ---------------------------

TEST_F(GripTest, EscLeavesTheGripWithoutChanges) {
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    select({line});
    clickAt({20, 0});
    runner().pointerMoved({25, 10});
    runner().submit(CancelInput{});

    EXPECT_EQ(messages(), (std::vector<std::string>{"** STRETCH **", "*Cancel*"}));
    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(runner().hotGrip());
    EXPECT_EQ(entity(line), Entity{(Line{{0, 0}, {20, 0}})});
    EXPECT_FALSE(editor().canUndo());
    EXPECT_EQ(selection(), (std::vector<EntityId>{line}));
    EXPECT_EQ(runner().grips().size(), 3U);

    // A second Esc, at rest, clears the selection as usual.
    runner().submit(CancelInput{});
    EXPECT_TRUE(runner().selection().isEmpty());
}

TEST_F(GripTest, EnterLeavesTheGripWithoutChanges) {
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    select({line});
    clickAt({20, 0});
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entity(line), Entity{(Line{{0, 0}, {20, 0}})});
    EXPECT_FALSE(editor().canUndo());
    EXPECT_EQ(selection(), (std::vector<EntityId>{line}));
}

TEST_F(GripTest, UndoAndRedoTheGripEdit) {
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    select({line});
    clickAt({20, 0});
    runner().submitText("25,10");

    runner().submitText("U");
    EXPECT_EQ(entity(line), Entity{(Line{{0, 0}, {20, 0}})});
    EXPECT_EQ(messages().back(), "Undone: Grip edit.");
    EXPECT_TRUE(runner().selection().isEmpty());  // U clears it (docs/adr/0008)

    runner().submitText("REDO");
    EXPECT_EQ(entity(line), Entity{(Line{{0, 0}, {25, 10}})});
    EXPECT_EQ(messages().back(), "Redone: Grip edit.");
    EXPECT_EQ(editor().undoNames(), (std::vector<std::string>{"Grip edit"}));
}

TEST_F(GripTest, InvalidEditChangesNothing) {
    const EntityId circle = add(Circle{{10, 10}, 10});
    select({circle});
    clickAt({20, 10});             // a quadrant
    runner().submitText("10,10");  // the centre: radius zero
    EXPECT_EQ(messages(), (std::vector<std::string>{"** STRETCH **", "Invalid grip edit."}));
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entity(circle), Entity{(Circle{{10, 10}, 10})});
    EXPECT_FALSE(editor().canUndo());
    EXPECT_EQ(selection(), (std::vector<EntityId>{circle}));
}

// --- Choosing the grip ----------------------------------------------------------

TEST_F(GripTest, AtTheSamePointTheHighestIdentifierWins) {
    const EntityId first = add(Line{{0, 0}, {10, 0}});
    const EntityId second = add(Line{{10, 0}, {20, 0}});
    select({second, first});  // the order of the selection does not matter
    clickAt({10, 0});
    runner().submitText("10,10");
    EXPECT_EQ(entity(first), Entity{(Line{{0, 0}, {10, 0}})});
    EXPECT_EQ(entity(second), Entity{(Line{{10, 10}, {20, 0}})});
}

TEST_F(GripTest, TheNearestGripWins) {
    const EntityId line = add(Line{{0, 0}, {6, 0}});  // grips 3 units apart
    select({line});
    clickAt({4, 1});  // in the squares of (3,0) and (6,0): nearer to (3,0)
    EXPECT_EQ(runner().hotGrip(), (Vec2{3, 0}));
    runner().submit(CancelInput{});
}

TEST_F(GripTest, TheGripSquareFollowsThePixelSize) {
    const EntityId line = add(Line{{0, 0}, {100, 0}});
    select({line});
    clickAt({104, 4});  // in the square, edges included: half side 5
    EXPECT_EQ(runner().hotGrip(), (Vec2{100, 0}));
    runner().submit(CancelInput{});

    clickAt({105.5, 0});  // outside: a click in the void opens a rectangle
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(runner().selectionRectangle());
    runner().submit(CancelInput{});

    runner().setPixelSize(2.0);  // half side 10
    clickAt({109, -9});
    EXPECT_EQ(runner().hotGrip(), (Vec2{100, 0}));
    runner().submit(CancelInput{});
}

TEST_F(GripTest, ClicksOutsideTheGripsSelectAsBefore) {
    const EntityId first = add(Line{{0, 0}, {20, 0}});
    const EntityId second = add(Line{{0, 10}, {20, 10}});
    select({first});

    clickAt({16, 10});  // on the second line, outside every grip square
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(selection(), (std::vector<EntityId>{first, second}));

    clickAt({10, 40});  // in the void: a rectangle, the selection stays
    ASSERT_TRUE(runner().selectionRectangle());
    EXPECT_EQ(runner().promptText(), "Specify opposite corner:");
    EXPECT_EQ(selection(), (std::vector<EntityId>{first, second}));

    // With a rectangle open, a click on a grip is its second corner.
    clickAt({0, 0});
    EXPECT_FALSE(runner().isActive());
    EXPECT_FALSE(runner().selectionRectangle());
}

TEST_F(GripTest, ShiftClickOnAGripTakesTheEntityOut) {
    const EntityId line = add(Line{{0, 0}, {20, 0}});
    select({line});
    clickAt({20, 0}, PickAction::Remove);
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(runner().selection().isEmpty());
}

}  // namespace
}  // namespace tg::cmd
