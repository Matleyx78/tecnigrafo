// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/text.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
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
using core::Vec2;

// Rotations go through sin and cos: results are exact only up to rounding.
constexpr double kNear = 1e-9;

class RotateScaleTest : public testing::Test {
protected:
    RotateScaleTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
        m_runner.setPixelSize(0.1);
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

    [[nodiscard]] bool wrote(const std::string& text) const {
        return std::ranges::find(m_messages, text) != m_messages.end();
    }

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

    [[nodiscard]] Line lineOf(EntityId id) { return std::get<Line>(entityOf(id)); }

    [[nodiscard]] static Line lineAt(double x1, double y1, double x2, double y2) {
        return Line{{x1, y1}, {x2, y2}};
    }

    static void expectNear(const Vec2& actual, const Vec2& expected) {
        EXPECT_NEAR(actual.x, expected.x, kNear);
        EXPECT_NEAR(actual.y, expected.y, kNear);
    }

    static void expectLine(const Line& actual, const Vec2& start, const Vec2& end) {
        expectNear(actual.start, start);
        expectNear(actual.end, end);
    }

    /// Selects `id` and starts `command`, which asks for its base point.
    void begin(EntityId id, const std::string& command) {
        select({id});
        runner().submitText(command);
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

// ROTATE

TEST_F(RotateScaleTest, RotateWithTypedAngleKeepsIdentityAndOrder) {
    const EntityId first = seed(lineAt(0, 0, 10, 0));
    const EntityId second = seed(lineAt(0, 5, 10, 5));
    const std::uint64_t order = document().find(first)->order;
    begin(first, "ROTATE");
    EXPECT_TRUE(wrote("Current positive angle in UCS:  ANGDIR=counterclockwise  ANGBASE=0"));
    EXPECT_EQ(runner().promptText(), "Specify base point:");
    point(0, 0);
    EXPECT_EQ(runner().promptText(), "Specify rotation angle or [Copy/Reference] <0>:");
    runner().submitText("90");
    EXPECT_FALSE(runner().isActive());
    expectLine(lineOf(first), {0, 0}, {0, 10});
    EXPECT_EQ(document().find(first)->order, order);
    EXPECT_EQ(entityOf(second), Entity{lineAt(0, 5, 10, 5)});
    EXPECT_EQ(editor().undoName(), "Rotate");
}

TEST_F(RotateScaleTest, RotateAliasAndSelectionAskedAfterTheCommand) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    runner().submitText("ro");
    EXPECT_EQ(runner().promptText(), "Select objects:");
    runner().click({5, 0}, PickAction::Add);
    runner().submitText("");
    point(0, 0);
    runner().submitText("180");
    expectLine(lineOf(id), {0, 0}, {-10, 0});
}

TEST_F(RotateScaleTest, RotateTurnsAboutTheBasePoint) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "ROTATE");
    point(10, 0);
    runner().submitText("90");
    expectLine(lineOf(id), {10, -10}, {10, 0});
}

TEST_F(RotateScaleTest, RotateByClickGivesTheAngleFromTheBasePoint) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "ROTATE");
    point(0, 0);
    point(0, 7);
    EXPECT_FALSE(runner().isActive());
    expectLine(lineOf(id), {0, 0}, {0, 10});
}

TEST_F(RotateScaleTest, RotateRemembersTheLastAngleAsTheDefault) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "ROTATE");
    point(0, 0);
    runner().submitText("90");
    select({id});
    runner().submitText("ROTATE");
    point(0, 0);
    EXPECT_EQ(runner().promptText(), "Specify rotation angle or [Copy/Reference] <90>:");
    runner().submitText("");  // Enter takes the default
    expectLine(lineOf(id), {0, 0}, {-10, 0});
    EXPECT_EQ(editor().undoNames().size(), 2U);
}

TEST_F(RotateScaleTest, RotateEnterAtTheBasePointEndsWithoutEffects) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "ROTATE");
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});
    EXPECT_FALSE(editor().canUndo());
}

TEST_F(RotateScaleTest, RotateCopyKeepsTheSourceAndAddsARotatedCopy) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    begin(source, "ROTATE");
    point(0, 0);
    runner().submitText("C");
    EXPECT_TRUE(wrote("Rotating a copy of the selected objects."));
    EXPECT_EQ(runner().promptText(), "Specify rotation angle or [Copy/Reference] <0>:");
    runner().submitText("90");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(source), Entity{lineAt(0, 0, 10, 0)});
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_NE(document().entities()[1].id, source);
    expectLine(std::get<Line>(document().entities()[1].entity), {0, 0}, {0, 10});
    EXPECT_EQ(editor().undoName(), "Rotate");
}

TEST_F(RotateScaleTest, RotateReferenceWithValuesTurnsByTheDifference) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "ROTATE");
    point(0, 0);
    runner().submitText("R");
    EXPECT_EQ(runner().promptText(), "Specify the reference angle <0>:");
    runner().submitText("30");
    EXPECT_EQ(runner().promptText(), "Specify the new angle or [Points] <0>:");
    runner().submitText("75");
    EXPECT_FALSE(runner().isActive());
    const double angle = core::degreesToRadians(45.0);
    expectLine(lineOf(id), {0, 0}, {10 * std::cos(angle), 10 * std::sin(angle)});
}

TEST_F(RotateScaleTest, RotateReferenceWithPointsMeasuresTheReferenceBetweenThem) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "ROTATE");
    point(0, 0);
    runner().submitText("R");
    point(1, 1);
    EXPECT_EQ(runner().promptText(), "Specify second point:");
    point(2, 2);  // a reference angle of 45 degrees
    EXPECT_EQ(runner().promptText(), "Specify the new angle or [Points] <0>:");
    point(0, 3);  // a click: the new angle is 90 degrees, from the base point
    EXPECT_FALSE(runner().isActive());
    const double angle = core::degreesToRadians(45.0);
    expectLine(lineOf(id), {0, 0}, {10 * std::cos(angle), 10 * std::sin(angle)});
}

TEST_F(RotateScaleTest, RotateNewAngleWithThePointsOption) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "ROTATE");
    point(0, 0);
    runner().submitText("R");
    runner().submitText("10");
    runner().submitText("P");
    EXPECT_EQ(runner().promptText(), "Specify first point:");
    point(5, 5);
    EXPECT_EQ(runner().promptText(), "Specify second point:");
    point(5, 8);  // 90 degrees: the rotation is 80
    EXPECT_FALSE(runner().isActive());
    const double angle = core::degreesToRadians(80.0);
    expectLine(lineOf(id), {0, 0}, {10 * std::cos(angle), 10 * std::sin(angle)});
}

TEST_F(RotateScaleTest, RotateReferenceWithEnterTakesTheDefaultsOfBothAngles) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "ROTATE");
    point(0, 0);
    runner().submitText("R");
    runner().submitText("");
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});
    EXPECT_FALSE(editor().canUndo());  // a zero rotation leaves no step
}

TEST_F(RotateScaleTest, RotateRotatesEveryKindOfEntity) {
    core::Polyline polyline{{{{0, 0}, 0.0}, {{10, 0}, 1.0}, {{10, 10}, 0.0}}};
    Text text;
    text.position = {1, 0};
    text.alignmentPoint = {1, 0};
    text.value = "A";
    const EntityId a = seed(polyline);
    const EntityId c = seed(Circle{{5, 0}, 2});
    const EntityId d = seed(Arc{{0, 0}, 3, 0.0, 1.0});
    const EntityId e = seed(text);
    select({a, c, d, e});
    runner().submitText("ROTATE");
    point(0, 0);
    runner().submitText("90");
    ASSERT_FALSE(runner().isActive());
    const auto& rotated = std::get<core::Polyline>(entityOf(a));
    expectNear(rotated.vertices()[1].position, {0, 10});
    expectNear(rotated.vertices()[2].position, {-10, 10});
    EXPECT_EQ(rotated.vertices()[1].bulge, 1.0);
    expectNear(std::get<Circle>(entityOf(c)).center, {0, 5});
    const auto& arc = std::get<Arc>(entityOf(d));
    EXPECT_NEAR(arc.startAngle, core::kPi / 2, kNear);
    EXPECT_NEAR(arc.endAngle, 1.0 + (core::kPi / 2), kNear);
    const auto& turned = std::get<Text>(entityOf(e));
    expectNear(turned.position, {0, 1});
    EXPECT_NEAR(turned.rotation, core::kPi / 2, kNear);
}

TEST_F(RotateScaleTest, RotateEscapeAtEveryQuestionChangesNothing) {
    const std::vector<std::vector<std::string>> answers{
        {},
        {"0,0"},
        {"0,0", "R"},
        {"0,0", "R", "20"},
        {"0,0", "R", "20", "P"},
        {"0,0", "R", "20", "P", "5,5"},
        {"0,0", "R", "1,1"},
    };
    for (const std::vector<std::string>& sequence : answers) {
        const EntityId id = seed(lineAt(0, 0, 10, 0));
        begin(id, "ROTATE");
        for (const std::string& answer : sequence) {
            runner().submitText(answer);
        }
        runner().submit(CancelInput{});
        EXPECT_FALSE(runner().isActive()) << sequence.size();
        EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)}) << sequence.size();
        EXPECT_FALSE(editor().canUndo()) << sequence.size();
        EXPECT_EQ(messages().back(), "*Cancel*");
    }
}

TEST_F(RotateScaleTest, RotateUndoAndRedo) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "ROTATE");
    point(0, 0);
    runner().submitText("90");
    ASSERT_TRUE(editor().undo());
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});
    ASSERT_TRUE(editor().redo());
    expectLine(lineOf(id), {0, 0}, {0, 10});
}

TEST_F(RotateScaleTest, RotatePreviewFollowsThePointer) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "ROTATE");
    EXPECT_TRUE(runner().preview().empty());
    point(0, 0);
    runner().pointerMoved({0, 5});
    ASSERT_EQ(runner().preview().size(), 1U);
    expectLine(std::get<Line>(runner().preview()[0]), {0, 0}, {0, 10});
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});  // only a preview
}

// SCALE

TEST_F(RotateScaleTest, ScaleWithTypedFactorKeepsIdentityAndOrder) {
    const EntityId first = seed(lineAt(0, 0, 10, 0));
    const EntityId second = seed(lineAt(0, 5, 10, 5));
    const std::uint64_t order = document().find(first)->order;
    begin(first, "SCALE");
    EXPECT_EQ(runner().promptText(), "Specify base point:");
    point(0, 0);
    EXPECT_EQ(runner().promptText(), "Specify scale factor or [Copy/Reference]:");
    runner().submitText("2");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(first), Entity{lineAt(0, 0, 20, 0)});
    EXPECT_EQ(document().find(first)->order, order);
    EXPECT_EQ(entityOf(second), Entity{lineAt(0, 5, 10, 5)});
    EXPECT_EQ(editor().undoName(), "Scale");
}

TEST_F(RotateScaleTest, ScaleAliasAndSelectionAskedAfterTheCommand) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    runner().submitText("sc");
    EXPECT_EQ(runner().promptText(), "Select objects:");
    runner().click({5, 0}, PickAction::Add);
    runner().submitText("");
    point(0, 0);
    runner().submitText("0.5");
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 5, 0)});
}

TEST_F(RotateScaleTest, ScaleKeepsTheBasePointFixed) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "SCALE");
    point(10, 0);
    runner().submitText("3");
    EXPECT_EQ(entityOf(id), Entity{lineAt(-20, 0, 10, 0)});
}

TEST_F(RotateScaleTest, ScaleByClickGivesTheDistanceFromTheBasePoint) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "SCALE");
    point(0, 0);
    point(3, 4);  // five units from the base point
    EXPECT_FALSE(runner().isActive());
    expectLine(lineOf(id), {0, 0}, {50, 0});
}

TEST_F(RotateScaleTest, ScaleRefusesAFactorThatIsNotPositive) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "SCALE");
    point(0, 0);
    for (const char* const factor : {"0", "-2", "-0.5"}) {
        const std::size_t before = messages().size();
        runner().submitText(factor);
        EXPECT_TRUE(runner().isActive()) << factor;
        EXPECT_EQ(runner().promptText(), "Specify scale factor or [Copy/Reference]:") << factor;
        EXPECT_EQ(messages().back(), "Value must be positive and nonzero.") << factor;
        EXPECT_GT(messages().size(), before) << factor;
    }
    point(0, 0);  // a click on the base point is a zero factor
    EXPECT_TRUE(runner().isActive());
    EXPECT_EQ(messages().back(), "Value must be positive and nonzero.");
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});
    runner().submitText("2");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 20, 0)});
}

TEST_F(RotateScaleTest, ScaleEnterAtTheFactorEndsWithoutEffects) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "SCALE");
    point(0, 0);
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});
    EXPECT_FALSE(editor().canUndo());
}

TEST_F(RotateScaleTest, ScaleCopyKeepsTheSourceAndAddsAScaledCopy) {
    const EntityId source = seed(lineAt(0, 0, 10, 0));
    begin(source, "SCALE");
    point(0, 0);
    runner().submitText("C");
    EXPECT_TRUE(wrote("Scaling a copy of the selected objects."));
    EXPECT_EQ(runner().promptText(), "Specify scale factor or [Copy/Reference]:");
    runner().submitText("3");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(source), Entity{lineAt(0, 0, 10, 0)});
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(document().entities()[1].entity, Entity{lineAt(0, 0, 30, 0)});
    EXPECT_EQ(editor().undoName(), "Scale");
}

TEST_F(RotateScaleTest, ScaleReferenceWithValuesScalesByTheRatio) {
    const EntityId id = seed(Circle{{0, 0}, 4});
    begin(id, "SCALE");
    point(0, 0);
    runner().submitText("R");
    EXPECT_EQ(runner().promptText(), "Specify reference length <1.0000>:");
    runner().submitText("4");
    EXPECT_EQ(runner().promptText(), "Specify new length or [Points] <1.0000>:");
    runner().submitText("10");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(std::get<Circle>(entityOf(id)).radius, 10.0);
}

TEST_F(RotateScaleTest, ScaleReferenceWithPointsMeasuresBothLengths) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "SCALE");
    point(0, 0);
    runner().submitText("R");
    point(1, 1);
    EXPECT_EQ(runner().promptText(), "Specify second point:");
    point(1, 5);  // a reference length of 4
    EXPECT_EQ(runner().promptText(), "Specify new length or [Points] <1.0000>:");
    runner().submitText("P");
    EXPECT_EQ(runner().promptText(), "Specify first point:");
    point(0, 0);
    EXPECT_EQ(runner().promptText(), "Specify second point:");
    point(0, 10);  // a new length of 10
    EXPECT_FALSE(runner().isActive());
    expectLine(lineOf(id), {0, 0}, {25, 0});
}

TEST_F(RotateScaleTest, ScaleNewLengthByClickIsMeasuredFromTheBasePoint) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "SCALE");
    point(0, 0);
    runner().submitText("R");
    runner().submitText("5");
    point(6, 8);  // ten units from the base point: a factor of 2
    expectLine(lineOf(id), {0, 0}, {20, 0});
}

TEST_F(RotateScaleTest, ScaleReferenceWithEnterTakesTheDefaultLengths) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "SCALE");
    point(0, 0);
    runner().submitText("R");
    runner().submitText("");
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});  // a factor of 1
}

TEST_F(RotateScaleTest, ScaleReferenceRefusesLengthsThatAreNotPositive) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "SCALE");
    point(0, 0);
    runner().submitText("R");
    runner().submitText("0");
    EXPECT_EQ(runner().promptText(), "Specify reference length <1.0000>:");
    EXPECT_EQ(messages().back(), "Value must be positive and nonzero.");
    point(2, 2);
    point(2, 2);  // two coincident points
    EXPECT_EQ(runner().promptText(), "Specify second point:");
    EXPECT_EQ(messages().back(), "Value must be positive and nonzero.");
    point(2, 6);  // a reference length of 4
    EXPECT_EQ(runner().promptText(), "Specify new length or [Points] <1.0000>:");
    runner().submitText("-3");
    EXPECT_EQ(runner().promptText(), "Specify new length or [Points] <1.0000>:");
    EXPECT_EQ(messages().back(), "Value must be positive and nonzero.");
    runner().submitText("8");
    EXPECT_FALSE(runner().isActive());
    expectLine(lineOf(id), {0, 0}, {20, 0});
}

TEST_F(RotateScaleTest, ScaleScalesEveryKindOfEntity) {
    core::Polyline polyline{{{{0, 0}, 0.0}, {{10, 0}, 1.0}, {{10, 10}, 0.0}}};
    Text text;
    text.position = {1, 2};
    text.alignmentPoint = {1, 2};
    text.value = "A";
    const EntityId a = seed(polyline);
    const EntityId c = seed(Circle{{5, 5}, 2});
    const EntityId d = seed(Arc{{1, 1}, 3, 0.0, 1.0});
    const EntityId e = seed(text);
    select({a, c, d, e});
    runner().submitText("SCALE");
    point(0, 0);
    runner().submitText("2");
    ASSERT_FALSE(runner().isActive());
    const auto& scaled = std::get<core::Polyline>(entityOf(a));
    EXPECT_EQ(scaled.vertices()[2].position, (Vec2{20, 20}));
    EXPECT_EQ(scaled.vertices()[1].bulge, 1.0);
    EXPECT_EQ(entityOf(c), (Entity{Circle{{10, 10}, 4}}));
    EXPECT_EQ(entityOf(d), (Entity{Arc{{2, 2}, 6, 0.0, 1.0}}));
    const auto& grown = std::get<Text>(entityOf(e));
    EXPECT_EQ(grown.position, (Vec2{2, 4}));
    EXPECT_EQ(grown.height, 5.0);
}

TEST_F(RotateScaleTest, ScaleEscapeAtEveryQuestionChangesNothing) {
    const std::vector<std::vector<std::string>> answers{
        {},
        {"0,0"},
        {"0,0", "R"},
        {"0,0", "R", "4"},
        {"0,0", "R", "4", "P"},
        {"0,0", "R", "4", "P", "5,5"},
        {"0,0", "R", "1,1"},
    };
    for (const std::vector<std::string>& sequence : answers) {
        const EntityId id = seed(lineAt(0, 0, 10, 0));
        begin(id, "SCALE");
        for (const std::string& answer : sequence) {
            runner().submitText(answer);
        }
        runner().submit(CancelInput{});
        EXPECT_FALSE(runner().isActive()) << sequence.size();
        EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)}) << sequence.size();
        EXPECT_FALSE(editor().canUndo()) << sequence.size();
        EXPECT_EQ(messages().back(), "*Cancel*");
    }
}

TEST_F(RotateScaleTest, ScaleUndoAndRedo) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "SCALE");
    point(0, 0);
    runner().submitText("2");
    ASSERT_TRUE(editor().undo());
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});
    ASSERT_TRUE(editor().redo());
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 20, 0)});
}

TEST_F(RotateScaleTest, ScalePreviewFollowsThePointer) {
    const EntityId id = seed(lineAt(0, 0, 10, 0));
    begin(id, "SCALE");
    point(0, 0);
    runner().pointerMoved({0, 3});
    ASSERT_EQ(runner().preview().size(), 1U);
    expectLine(std::get<Line>(runner().preview()[0]), {0, 0}, {30, 0});
    runner().pointerMoved({0, 0});  // a zero factor draws nothing
    EXPECT_TRUE(runner().preview().empty());
    EXPECT_EQ(entityOf(id), Entity{lineAt(0, 0, 10, 0)});  // only a preview
}

TEST(CommandDefaultsTest, RotationStartsAtZeroAndMirrorTextAtReadable) {
    const CommandDefaults defaults;
    EXPECT_EQ(defaults.rotation, 0.0);
    EXPECT_EQ(defaults.mirrorText, core::MirrorText::Readable);
}

}  // namespace
}  // namespace tg::cmd
