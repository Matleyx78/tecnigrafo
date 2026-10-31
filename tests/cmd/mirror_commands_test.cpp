// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/similarity.hpp"
#include "tg/core/text.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
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
using core::Vec2;

constexpr double kNear = 1e-9;

class MirrorTest : public testing::Test {
protected:
    MirrorTest() {
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

    static void expectNear(const Vec2& actual, const Vec2& expected) {
        EXPECT_NEAR(actual.x, expected.x, kNear);
        EXPECT_NEAR(actual.y, expected.y, kNear);
    }

    /// A reflection goes through sin and cos: its results are exact only up to
    /// rounding, so a reflected entity is compared with a tolerance.
    static void expectLine(const Entity& actual, const Vec2& start, const Vec2& end) {
        const auto* line = std::get_if<Line>(&actual);
        ASSERT_NE(line, nullptr);
        expectNear(line->start, start);
        expectNear(line->end, end);
    }

    /// Selects `id`, starts MIRROR and gives it the mirror line (0,0)-(0,10):
    /// the Y axis. The question about the source is next.
    void mirrorAboutTheYAxis(EntityId id) {
        select({id});
        runner().submitText("MIRROR");
        point(0, 0);
        point(0, 10);
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

// MIRROR

TEST_F(MirrorTest, MirrorKeepsTheSourceAndAddsAReflectedCopy) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    select({source});
    runner().submitText("MIRROR");
    EXPECT_EQ(runner().promptText(), "Specify first point of mirror line:");
    point(0, 0);
    EXPECT_EQ(runner().promptText(), "Specify second point of mirror line:");
    point(0, 10);
    EXPECT_EQ(runner().promptText(), "Erase source objects? [Yes/No] <No>:");
    runner().submitText("");  // the default is No
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(source), Entity{lineAt(0, 0, 10, 0)});
    ASSERT_EQ(document().entityCount(), 2U);
    const core::EntityRecord& copy = document().entities()[1];
    EXPECT_NE(copy.id, source);
    EXPECT_GT(copy.order, document().find(source)->order);  // at the end of the order
    expectLine(copy.entity, {0, 0}, {-10, 0});
    EXPECT_EQ(editor().undoName(), "Mirror");
}

TEST_F(MirrorTest, MirrorNoAnswersLikeEnter) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    mirrorAboutTheYAxis(source);
    runner().submitText("n");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(entityOf(source), Entity{lineAt(0, 0, 10, 0)});
}

TEST_F(MirrorTest, MirrorYesReflectsInPlaceKeepingIdentityAndOrder) {
    const EntityId first = seed(lineAt(0, 0, 10, 0));
    const EntityId second = seed(lineAt(0, 5, 10, 5));
    const std::uint64_t order = document().find(first)->order;
    mirrorAboutTheYAxis(first);
    runner().submitText("Y");
    EXPECT_FALSE(runner().isActive());
    ASSERT_EQ(document().entityCount(), 2U);
    expectLine(entityOf(first), {0, 0}, {-10, 0});
    EXPECT_EQ(document().find(first)->order, order);
    EXPECT_EQ(entityOf(second), Entity{lineAt(0, 5, 10, 5)});
    EXPECT_EQ(editor().undoName(), "Mirror");
}

TEST_F(MirrorTest, MirrorAliasAndSelectionAskedAfterTheCommand) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    runner().submitText("mi");
    EXPECT_EQ(runner().promptText(), "Select objects:");
    runner().click({5, 0}, PickAction::Add);
    runner().submitText("");
    point(0, 0);
    point(0, 10);
    runner().submitText("Yes");
    expectLine(entityOf(id), {0, 0}, {-10, 0});
}

TEST_F(MirrorTest, MirrorReflectsAboutAnObliqueLine) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MIRROR");
    point(0, 0);
    point(5, 5);  // the diagonal: x and y swap
    runner().submitText("Y");
    const auto& line = std::get<Line>(entityOf(id));
    expectNear(line.start, {0, 0});
    expectNear(line.end, {0, 10});
}

TEST_F(MirrorTest, MirrorCoincidentPointsKeepTheSecondQuestion) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MIRROR");
    point(3, 3);
    point(3, 3);
    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(runner().promptText(), "Specify second point of mirror line:");
    point(3, 8);
    EXPECT_EQ(runner().promptText(), "Erase source objects? [Yes/No] <No>:");
    runner().submitText("Y");
    expectLine(entityOf(id), {6, 0}, {-4, 0});
}

TEST_F(MirrorTest, MirrorEnterAtTheLinePointsEndsWithoutEffects) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MIRROR");
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    select({id});
    runner().submitText("MIRROR");
    point(0, 0);
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_FALSE(editor().canUndo());
}

TEST_F(MirrorTest, MirrorReflectsEveryKindOfEntity) {
    core::Polyline polyline{{{{0, 0}, 1.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}}};
    const EntityId a = seed(polyline);
    const EntityId b = seed(lineAt(1, 2, 3, 4));
    const EntityId c = seed(Circle{{5, 5}, 2});
    const EntityId d = seed(Arc{{0, 0}, 5, 0.0, core::kPi / 2});
    select({a, b, c, d});
    runner().submitText("MIRROR");
    point(0, 0);
    point(0, 10);
    runner().submitText("Y");
    ASSERT_FALSE(runner().isActive());
    const auto& mirrored = std::get<core::Polyline>(entityOf(a));
    expectNear(mirrored.vertices()[1].position, {-10, 0});
    expectNear(mirrored.vertices()[2].position, {-10, 10});
    EXPECT_EQ(mirrored.vertices()[0].bulge, -1.0);  // a reflection reverses the turn
    expectLine(entityOf(b), {-1, 2}, {-3, 4});
    expectNear(std::get<Circle>(entityOf(c)).center, {-5, 5});
    EXPECT_EQ(std::get<Circle>(entityOf(c)).radius, 2.0);
    // The arc stays counterclockwise: its end points swap (docs/adr/0031).
    const auto& arc = std::get<Arc>(entityOf(d));
    expectNear(arc.center, {0, 0});
    EXPECT_NEAR(arc.startAngle, core::kPi / 2, kNear);
    EXPECT_NEAR(arc.endAngle, core::kPi, kNear);
}

TEST_F(MirrorTest, MirrorTextFollowsMirrtext) {
    Text text;
    text.position = {2, 3};
    text.alignmentPoint = {2, 3};
    text.rotation = 0.3;
    text.value = "AB";
    const Entity original{text};
    const std::optional<core::Similarity> axis = core::Similarity::reflection({0, 0}, {0, 10});
    ASSERT_TRUE(axis.has_value());
    const Entity readable = core::transformed(original, *axis, core::MirrorText::Readable);
    const Entity reflected = core::transformed(original, *axis, core::MirrorText::Mirrored);
    ASSERT_NE(readable, reflected);  // otherwise this test would prove nothing

    // MIRRTEXT is 0 when the program starts.
    const EntityId first = seed(original);
    mirrorAboutTheYAxis(first);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(document().entities()[1].entity, readable);

    runner().submitText("MIRRTEXT");
    runner().submitText("1");
    mirrorAboutTheYAxis(first);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 3U);
    EXPECT_EQ(document().entities()[2].entity, reflected);

    // In place, too.
    mirrorAboutTheYAxis(first);
    runner().submitText("Y");
    EXPECT_EQ(entityOf(first), reflected);
}

TEST_F(MirrorTest, MirrorEscapeAtEveryQuestionChangesNothing) {
    const std::vector<std::vector<std::string>> answers{
        {},
        {"0,0"},
        {"0,0", "0,10"},
    };
    for (const std::vector<std::string>& sequence : answers) {
        const EntityId id = seed(lineAt(0, 0, 10, 0));
        select({id});
        runner().submitText("MIRROR");
        for (const std::string& answer : sequence) {
            runner().submitText(answer);
        }
        runner().submit(CancelInput{});
        EXPECT_FALSE(runner().isActive()) << sequence.size();
        EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)}) << sequence.size();
        EXPECT_FALSE(editor().canUndo()) << sequence.size();
        EXPECT_EQ(messages().back(), "*Cancel*");
    }
    EXPECT_EQ(document().entityCount(), 3U);  // no copy was left behind
}

TEST_F(MirrorTest, MirrorUndoAndRedo) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    mirrorAboutTheYAxis(source);
    runner().submitText("");
    ASSERT_EQ(document().entityCount(), 2U);
    ASSERT_TRUE(editor().undo());
    EXPECT_EQ(document().entityCount(), 1U);
    ASSERT_TRUE(editor().redo());
    EXPECT_EQ(document().entityCount(), 2U);
    mirrorAboutTheYAxis(source);
    runner().submitText("Y");
    ASSERT_TRUE(editor().undo());
    EXPECT_EQ(entityOf(source), Entity{lineAt(0, 0, 10, 0)});
    ASSERT_TRUE(editor().redo());
    expectLine(entityOf(source), {0, 0}, {-10, 0});
}

TEST_F(MirrorTest, MirrorPreviewFollowsThePointerAndStaysAtTheLastQuestion) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    select({id});
    runner().submitText("MIRROR");
    EXPECT_TRUE(runner().preview().empty());
    point(0, 0);
    runner().pointerMoved({0, 10});
    ASSERT_EQ(runner().preview().size(), 1U);
    expectLine(runner().preview()[0], {0, 0}, {-10, 0});
    runner().pointerMoved({0, 0});  // the pointer on the first point: no line
    EXPECT_TRUE(runner().preview().empty());
    point(0, 10);
    ASSERT_EQ(runner().preview().size(), 1U);
    expectLine(runner().preview()[0], {0, 0}, {-10, 0});
    EXPECT_EQ(document().entityCount(), 1U);  // only a preview
}

// MIRRTEXT

TEST_F(MirrorTest, MirrtextAsksForZeroOrOneAndRemembersTheValue) {
    runner().submitText("MIRRTEXT");
    EXPECT_EQ(runner().promptText(), "Enter new value for MIRRTEXT <0>:");
    runner().submitText("1");
    EXPECT_FALSE(runner().isActive());
    runner().submitText("MIRRTEXT");
    EXPECT_EQ(runner().promptText(), "Enter new value for MIRRTEXT <1>:");
    runner().submitText("");  // Enter keeps the value
    EXPECT_FALSE(runner().isActive());
    runner().submitText("MIRRTEXT");
    EXPECT_EQ(runner().promptText(), "Enter new value for MIRRTEXT <1>:");
    runner().submitText(" 0 ");
    runner().submitText("MIRRTEXT");
    EXPECT_EQ(runner().promptText(), "Enter new value for MIRRTEXT <0>:");
    runner().submit(CancelInput{});
    EXPECT_FALSE(runner().isActive());
}

TEST_F(MirrorTest, MirrtextRefusesAnythingButZeroOrOne) {
    runner().submitText("MIRRTEXT");
    for (const char* const value : {"2", "-1", "0.5", "abc", "1 0", "10"}) {
        runner().submitText(value);
        EXPECT_TRUE(runner().isActive()) << value;
        EXPECT_EQ(runner().promptText(), "Enter new value for MIRRTEXT <0>:") << value;
        EXPECT_EQ(messages().back(), "Requires an integer between 0 and 1.") << value;
    }
    runner().submitText("1");
    EXPECT_FALSE(runner().isActive());
    runner().submitText("MIRRTEXT");
    EXPECT_EQ(runner().promptText(), "Enter new value for MIRRTEXT <1>:");
}

TEST_F(MirrorTest, MirrtextIsASessionValueWithNoUndoStep) {
    runner().submitText("MIRRTEXT");
    runner().submitText("1");
    EXPECT_FALSE(editor().canUndo());
}

}  // namespace
}  // namespace tg::cmd
