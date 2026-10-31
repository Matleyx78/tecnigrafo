// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/line.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/text.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

using core::Arc;
using core::Circle;
using core::Entity;
using core::EntityId;
using core::Line;
using core::Text;

class MoveCopyTest : public testing::Test {
protected:
    MoveCopyTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
        m_runner.setPixelSize(0.1);
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    /// Adds `entity` outside the undo history, as a drawing that was opened.
    EntityId seed(Entity entity) { return m_document.add(std::move(entity)); }

    void select(std::vector<EntityId> ids) {
        ASSERT_TRUE(runner().replaceSelection(std::move(ids)));
    }

    void point(double x, double y) { runner().submit(PointInput{{x, y}}); }

    [[nodiscard]] const Entity& entityOf(EntityId id) {
        const core::EntityRecord* record = m_document.find(id);
        if (record == nullptr) {
            ADD_FAILURE() << "missing entity " << id.value;
            static const Entity kNone{Line{}};
            return kNone;
        }
        return record->entity;
    }

    [[nodiscard]] static Line lineAt(double x1, double y1, double x2, double y2) {
        return Line{{x1, y1}, {x2, y2}};
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

// MOVE

TEST_F(MoveCopyTest, MoveWithTwoPointsKeepsIdentityAndOrder) {
    const EntityId first = seed(lineAt(0, 0, 10, 0));
    const EntityId second = seed(lineAt(0, 5, 10, 5));
    const std::uint64_t order = document().find(first)->order;
    select({first});
    runner().submitText("MOVE");
    EXPECT_EQ(runner().promptText(), "Specify base point or [Displacement] <Displacement>:");
    point(0, 0);
    EXPECT_EQ(runner().promptText(), "Specify second point or <use first point as displacement>:");
    point(10, 20);
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(first), Entity{lineAt(10, 20, 20, 20)});
    EXPECT_EQ(document().find(first)->order, order);
    EXPECT_EQ(entityOf(second), Entity{lineAt(0, 5, 10, 5)});
    EXPECT_EQ(editor().undoName(), "Move");
}

TEST_F(MoveCopyTest, MoveAliasAndSelectionAskedAfterTheCommand) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    runner().submitText("m");
    EXPECT_EQ(runner().promptText(), "Select objects:");
    runner().click({5, 0}, PickAction::Add);
    runner().submitText("");
    point(0, 0);
    point(1, 1);
    EXPECT_EQ(entityOf(id), Entity{lineAt(1, 1, 11, 1)});
}

TEST_F(MoveCopyTest, MoveMovesEveryKindOfEntity) {
    core::Polyline polyline{{{{0, 0}, 0.0}, {{10, 0}, 1.0}, {{10, 10}, 0.0}}};
    Text text;
    text.position = {1, 2};
    text.alignmentPoint = {1, 2};
    text.value = "A";
    const EntityId a = seed(polyline);
    const EntityId b = seed(lineAt(0, 0, 1, 1));
    const EntityId c = seed(Circle{{5, 5}, 2});
    const EntityId d = seed(Arc{{0, 0}, 3, 0.0, 1.0});
    const EntityId e = seed(text);
    select({a, b, c, d, e});
    runner().submitText("MOVE");
    point(0, 0);
    point(100, -50);
    ASSERT_FALSE(runner().isActive());
    EXPECT_EQ(std::get<core::Polyline>(entityOf(a)).vertices()[2].position, (core::Vec2{110, -40}));
    EXPECT_EQ(std::get<core::Polyline>(entityOf(a)).vertices()[1].bulge, 1.0);
    EXPECT_EQ(entityOf(b), Entity{lineAt(100, -50, 101, -49)});
    EXPECT_EQ(entityOf(c), (Entity{Circle{{105, -45}, 2}}));
    EXPECT_EQ(entityOf(d), (Entity{Arc{{100, -50}, 3, 0.0, 1.0}}));
    EXPECT_EQ(std::get<Text>(entityOf(e)).position, (core::Vec2{101, -48}));
    EXPECT_EQ(std::get<Text>(entityOf(e)).alignmentPoint, (core::Vec2{101, -48}));
}

TEST_F(MoveCopyTest, MoveWithDisplacementOption) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MOVE");
    runner().submitText("D");
    EXPECT_EQ(runner().promptText(), "Specify displacement <0.0000, 0.0000>:");
    point(15, 25);
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(id), Entity{lineAt(15, 25, 25, 25)});
}

TEST_F(MoveCopyTest, EnterAtTheFirstQuestionAsksForTheDisplacement) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MOVE");
    runner().submitText("");
    EXPECT_EQ(runner().promptText(), "Specify displacement <0.0000, 0.0000>:");
    point(1, 2);
    EXPECT_EQ(entityOf(id), Entity{lineAt(1, 2, 11, 2)});
}

TEST_F(MoveCopyTest, EnterAtTheSecondPointUsesTheFirstPointAsDisplacement) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MOVE");
    point(3, 4);
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(id), Entity{lineAt(3, 4, 13, 4)});
}

TEST_F(MoveCopyTest, AZeroDisplacementLeavesNoUndoStep) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MOVE");
    point(5, 5);
    point(5, 5);
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});
    EXPECT_FALSE(editor().canUndo());
}

TEST_F(MoveCopyTest, TheNextPointCanBeRelativeToTheBasePoint) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MOVE");
    point(100, 100);
    runner().submitText("@5,6");
    EXPECT_EQ(entityOf(id), Entity{lineAt(5, 6, 15, 6)});
}

TEST_F(MoveCopyTest, MovePreviewFollowsThePointerAndLeavesTheDrawingAlone) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MOVE");
    point(0, 0);
    runner().pointerMoved({2, 3});
    ASSERT_EQ(runner().preview().size(), 1U);
    EXPECT_EQ(runner().preview()[0], Entity{lineAt(2, 3, 12, 3)});
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});
}

TEST_F(MoveCopyTest, EscapeInTheMiddleOfMoveChangesNothing) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MOVE");
    point(0, 0);
    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(messages().back(), "*Cancel*");
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});
    EXPECT_FALSE(editor().canUndo());
}

TEST_F(MoveCopyTest, MoveCanBeUndoneAndRedone) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MOVE");
    point(0, 0);
    point(10, 20);
    runner().submitText("U");
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});
    runner().submitText("REDO");
    EXPECT_EQ(entityOf(id), Entity{lineAt(10, 20, 20, 20)});
}

TEST_F(MoveCopyTest, MoveKeepsPropertiesAndUnhandledData) {
    core::EntityExtras extras;
    extras.common = {{60, "1"}};
    extras.extended = {{1001, "APP"}, {1000, "value"}};
    core::EntityProperties properties = document().current();
    properties.color = core::Color::fromIndex(3);
    const core::EntityRecord source{EntityId{300}, lineAt(0, 0, 10, 0), extras, 7, properties};
    document().insert(source);
    select({source.id});
    runner().submitText("MOVE");
    point(0, 0);
    point(1, 1);
    const core::EntityRecord* moved = document().find(source.id);
    ASSERT_NE(moved, nullptr);
    EXPECT_EQ(moved->extras, extras);
    EXPECT_EQ(moved->properties, properties);
    EXPECT_EQ(moved->order, source.order);
    EXPECT_EQ(moved->entity, Entity{lineAt(1, 1, 11, 1)});
}

// COPY

TEST_F(MoveCopyTest, CopyMakesAsManyCopiesAsPointsAndUndoRemovesTheLastOne) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    select({source});
    runner().submitText("COPY");
    EXPECT_EQ(messages().back(), "Current settings:  Copy mode = Multiple");
    EXPECT_EQ(runner().promptText(), "Specify base point or [Displacement] <Displacement>:");
    point(0, 0);
    EXPECT_EQ(runner().promptText(), "Specify second point or <use first point as displacement>:");
    point(0, 10);
    EXPECT_EQ(runner().promptText(), "Specify second point or [Exit/Undo] <Exit>:");
    point(0, 20);
    point(0, 30);
    EXPECT_EQ(document().entityCount(), 4U);
    runner().submitText("U");
    EXPECT_EQ(document().entityCount(), 3U);
    point(0, 40);
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    ASSERT_EQ(document().entityCount(), 4U);
    EXPECT_EQ(document().entities()[0].entity, Entity{lineAt(0, 0, 10, 0)});
    EXPECT_EQ(document().entities()[1].entity, Entity{lineAt(0, 10, 10, 10)});
    EXPECT_EQ(document().entities()[2].entity, Entity{lineAt(0, 20, 10, 20)});
    EXPECT_EQ(document().entities()[3].entity, Entity{lineAt(0, 40, 10, 40)});
    EXPECT_EQ(entityOf(source), Entity{lineAt(0, 0, 10, 0)});
    EXPECT_EQ(editor().undoName(), "Copy");
    EXPECT_EQ(editor().undoNames().size(), 1U);
}

TEST_F(MoveCopyTest, CopyExitKeywordAndAliasesClose) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    std::size_t expected = 1;
    for (const char* name : {"CO", "CP", "copy"}) {
        select({source});
        runner().submitText(name);
        point(0, 0);
        point(5, 5);
        runner().submitText("E");
        ++expected;
        EXPECT_FALSE(runner().isActive()) << name;
        EXPECT_EQ(document().entityCount(), expected) << name;
    }
}

TEST_F(MoveCopyTest, CopyCopiesEverySelectedEntityInSelectionOrder) {
    const EntityId a = seed(lineAt(0, 0, 1, 0));
    const EntityId b = seed(Circle{{5, 5}, 2});
    select({b, a});
    runner().submitText("COPY");
    point(0, 0);
    point(0, 100);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 4U);
    EXPECT_EQ(document().entities()[2].entity, (Entity{Circle{{5, 105}, 2}}));
    EXPECT_EQ(document().entities()[3].entity, Entity{lineAt(0, 100, 1, 100)});
}

TEST_F(MoveCopyTest, CopyWithDisplacementMakesOneCopyAndEnds) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    select({source});
    runner().submitText("COPY");
    runner().submitText("D");
    EXPECT_EQ(runner().promptText(), "Specify displacement <0.0000, 0.0000>:");
    point(0, 7);
    EXPECT_FALSE(runner().isActive());
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(document().entities()[1].entity, Entity{lineAt(0, 7, 10, 7)});
}

TEST_F(MoveCopyTest, CopyEnterAtTheSecondPointUsesTheFirstPointAsDisplacement) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    select({source});
    runner().submitText("COPY");
    point(3, 4);
    runner().submitText("");
    EXPECT_EQ(runner().promptText(), "Specify second point or [Exit/Undo] <Exit>:");
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(document().entities()[1].entity, Entity{lineAt(3, 4, 13, 4)});
}

TEST_F(MoveCopyTest, EscapeKeepsTheCopiesAlreadyMade) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    select({source});
    runner().submitText("COPY");
    point(0, 0);
    point(0, 10);
    point(0, 20);
    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(messages().back(), "*Cancel*");
    EXPECT_EQ(document().entityCount(), 3U);
    EXPECT_EQ(editor().undoName(), "Copy");
    ASSERT_TRUE(editor().undo());
    EXPECT_EQ(document().entityCount(), 1U);
}

TEST_F(MoveCopyTest, EscapeBeforeAnyCopyLeavesNoStep) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    select({source});
    runner().submitText("COPY");
    point(0, 0);
    runner().submit(CancelInput{});
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_FALSE(editor().canUndo());
}

TEST_F(MoveCopyTest, UndoOfEveryCopyLeavesNoStep) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    select({source});
    runner().submitText("COPY");
    point(0, 0);
    point(0, 10);
    runner().submitText("U");
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_FALSE(editor().canUndo());
}

TEST_F(MoveCopyTest, UndoOnlyRemovesTheCopiesOfTheLastPoint) {
    const EntityId a = seed(lineAt(0, 0, 1, 0));
    const EntityId b = seed(lineAt(0, 5, 1, 5));
    select({a, b});
    runner().submitText("COPY");
    point(0, 0);
    point(0, 10);
    point(0, 20);
    EXPECT_EQ(document().entityCount(), 6U);
    runner().submitText("U");
    EXPECT_EQ(document().entityCount(), 4U);
    runner().submitText("U");
    EXPECT_EQ(document().entityCount(), 2U);
    runner().submitText("U");  // nothing left to undo
    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 2U);
}

TEST_F(MoveCopyTest, TheSequenceOfScriptsMakesOneCopy) {
    runner().setScriptSequences(true);
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    select({source});
    runner().submitText("COPY");
    point(0, 0);
    point(0, 10);
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(editor().undoName(), "Copy");
}

TEST_F(MoveCopyTest, CopiesTakePropertiesAndUnhandledDataExceptReactors) {
    core::EntityExtras extras;
    extras.beforeEntity = {{102, "{ACAD_REACTORS"}, {330, "OWNER"}, {102, "}"}, {330, "KEEP"}};
    extras.common = {{60, "1"}};
    extras.specific = {{999, "opaque"}};
    extras.extended = {{1001, "APP"}, {1000, "value"}};
    core::EntityProperties properties = document().current();
    properties.color = core::Color::fromIndex(5);
    const core::EntityRecord source{EntityId{300}, lineAt(0, 0, 10, 0), extras, 7, properties};
    document().insert(source);
    select({source.id});
    runner().submitText("COPY");
    point(0, 0);
    point(0, 10);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 2U);
    const core::EntityRecord& copy = document().entities()[1];
    EXPECT_GT(copy.id.value, source.id.value);
    core::EntityExtras expected = extras;
    expected.beforeEntity = {{330, "KEEP"}};
    EXPECT_EQ(copy.extras, expected);
    EXPECT_EQ(copy.properties, properties);
    EXPECT_EQ(*document().find(source.id), source);
}

TEST_F(MoveCopyTest, CopyPreviewShowsTheNextCopy) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    select({source});
    runner().submitText("COPY");
    point(0, 0);
    runner().pointerMoved({0, 9});
    ASSERT_EQ(runner().preview().size(), 1U);
    EXPECT_EQ(runner().preview()[0], Entity{lineAt(0, 9, 10, 9)});
    EXPECT_EQ(document().entityCount(), 1U);
}

TEST_F(MoveCopyTest, CopyCanBeUndoneAndRedoneAsOneStep) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    select({source});
    runner().submitText("COPY");
    point(0, 0);
    point(0, 10);
    point(0, 20);
    runner().submitText("");
    runner().submitText("U");
    EXPECT_EQ(document().entityCount(), 1U);
    runner().submitText("REDO");
    EXPECT_EQ(document().entityCount(), 3U);
}

}  // namespace
}  // namespace tg::cmd
