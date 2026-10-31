// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"

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
using core::Polyline;

constexpr double kTolerance = 1e-9;

class ExplodeCommandTest : public testing::Test {
protected:
    ExplodeCommandTest() {
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

    void explode(std::vector<EntityId> ids) {
        select(std::move(ids));
        runner().submitText("EXPLODE");
    }

    [[nodiscard]] std::size_t count(const std::string& message) const {
        return static_cast<std::size_t>(std::ranges::count(m_messages, message));
    }

    [[nodiscard]] const Entity& entityAt(std::size_t index) {
        return m_document.entities()[index].entity;
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(ExplodeCommandTest, ClosedPolylineBecomesOneLinePerSegment) {
    const EntityId id =
        seed(Polyline{{{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}, {{0, 10}, 0.0}}, true});
    explode({id});
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().find(id), nullptr);
    ASSERT_EQ(document().entityCount(), 4U);
    EXPECT_EQ(entityAt(0), (Entity{Line{{0, 0}, {10, 0}}}));
    EXPECT_EQ(entityAt(1), (Entity{Line{{10, 0}, {10, 10}}}));
    EXPECT_EQ(entityAt(2), (Entity{Line{{10, 10}, {0, 10}}}));
    EXPECT_EQ(entityAt(3), (Entity{Line{{0, 10}, {0, 0}}}));
    EXPECT_EQ(editor().undoName(), "Explode");
    EXPECT_TRUE(messages().empty());
}

TEST_F(ExplodeCommandTest, AliasXAndAnOpenPolylineGiveOneLessSegment) {
    const EntityId id = seed(Polyline{{{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}}});
    select({id});
    runner().submitText("x");
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(entityAt(0), (Entity{Line{{0, 0}, {10, 0}}}));
    EXPECT_EQ(entityAt(1), (Entity{Line{{10, 0}, {10, 10}}}));
}

TEST_F(ExplodeCommandTest, ArcSegmentsBecomeCounterClockwiseArcs) {
    // A counter-clockwise half circle (below its chord), then a clockwise one
    // (bulge < 0, over its chord): both become counter-clockwise arcs.
    const EntityId id = seed(Polyline{{{{0, 0}, 1.0}, {{10, 0}, -1.0}, {{20, 0}, 0.0}}});
    explode({id});
    ASSERT_EQ(document().entityCount(), 2U);
    const auto& first = std::get<Arc>(entityAt(0));
    EXPECT_NEAR(first.center.x, 5.0, kTolerance);
    EXPECT_NEAR(first.center.y, 0.0, kTolerance);
    EXPECT_NEAR(first.radius, 5.0, kTolerance);
    EXPECT_NEAR(first.startAngle, core::kPi, kTolerance);
    EXPECT_NEAR(std::remainder(first.endAngle, core::kTwoPi), 0.0, kTolerance);
    const auto& second = std::get<Arc>(entityAt(1));
    EXPECT_NEAR(second.center.x, 15.0, kTolerance);
    EXPECT_NEAR(second.radius, 5.0, kTolerance);
    // Travelled clockwise from (10,0) to (20,0), over the top: the arc runs
    // counter-clockwise from (20,0) at 0 to (10,0) at pi.
    EXPECT_NEAR(second.startAngle, 0.0, kTolerance);
    EXPECT_NEAR(second.endAngle, core::kPi, kTolerance);
}

TEST_F(ExplodeCommandTest, MixedSegmentsKeepTheirOrder) {
    const EntityId id = seed(Polyline{{{{0, 0}, 0.0}, {{10, 0}, 1.0}, {{10, 10}, 0.0}}});
    explode({id});
    ASSERT_EQ(document().entityCount(), 2U);
    EXPECT_TRUE(std::holds_alternative<Line>(entityAt(0)));
    EXPECT_TRUE(std::holds_alternative<Arc>(entityAt(1)));
}

TEST_F(ExplodeCommandTest, WidthsAreLostWithTheMessagesOfAutoCadWrittenOnce) {
    Polyline wide{{{{0, 0}, 0.0, 1.0, 2.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}}};
    Polyline constant{{{{0, 20}, 0.0}, {{10, 20}, 0.0}}};
    constant.setConstantWidth(0.5);
    const EntityId a = seed(wide);
    const EntityId b = seed(constant);
    explode({a, b});
    EXPECT_EQ(document().entityCount(), 3U);
    EXPECT_EQ(count("Exploding this polyline has lost width information."), 1U);
    EXPECT_EQ(count("The UNDO command will restore it."), 1U);
}

TEST_F(ExplodeCommandTest, ASimplePolylineSaysNothingAboutWidths) {
    const EntityId id = seed(Polyline{{{{0, 0}, 0.0}, {{10, 0}, 0.0}}});
    explode({id});
    EXPECT_EQ(count("Exploding this polyline has lost width information."), 0U);
}

TEST_F(ExplodeCommandTest, UndoBringsTheWideWidthBackWithTheSameIdentity) {
    Polyline wide{{{{0, 0}, 0.0, 1.0, 2.0}, {{10, 0}, 0.0}}};
    const EntityId id = seed(wide);
    const core::EntityRecord before = *document().find(id);
    explode({id});
    ASSERT_EQ(document().entityCount(), 1U);
    runner().submitText("U");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(*document().find(id), before);
    runner().submitText("REDO");
    EXPECT_EQ(document().find(id), nullptr);
    EXPECT_EQ(document().entityCount(), 1U);
}

TEST_F(ExplodeCommandTest, PolylinesWithElevationOrThicknessAreNotExploded) {
    std::vector<EntityId> ids;
    std::uint64_t next = 300;
    for (const std::int16_t group : {std::int16_t{38}, std::int16_t{39}}) {
        core::EntityExtras extras;
        extras.placement = {{group, "5.0"}};
        const core::EntityRecord record{EntityId{next}, Polyline{{{{0, 0}, 0.0}, {{10, 0}, 0.0}}},
                                        extras, next, document().current()};
        document().insert(record);
        ids.push_back(record.id);
        ++next;
    }
    explode(ids);
    EXPECT_EQ(document().entityCount(), 2U);
    EXPECT_EQ(count("2 were not able to be exploded."), 1U);
    EXPECT_FALSE(editor().canUndo());
}

TEST_F(ExplodeCommandTest, ZeroElevationAndThicknessDoNotPreventExploding) {
    core::EntityExtras extras;
    extras.placement = {{38, "0.0"}, {39, "0"}};
    const core::EntityRecord record{EntityId{300}, Polyline{{{{0, 0}, 0.0}, {{10, 0}, 0.0}}},
                                    extras, 5, document().current()};
    document().insert(record);
    explode({record.id});
    EXPECT_EQ(document().find(record.id), nullptr);
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_TRUE(std::holds_alternative<Line>(entityAt(0)));
    EXPECT_TRUE(messages().empty());
}

TEST_F(ExplodeCommandTest, OtherEntitiesAreCountedNotExploded) {
    const EntityId line = seed(Line{{0, 0}, {10, 0}});
    explode({line});
    EXPECT_EQ(count("1 was not able to be exploded."), 1U);
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_FALSE(editor().canUndo());
}

TEST_F(ExplodeCommandTest, MixedSelectionExplodesWhatItCanAndCountsTheRest) {
    const EntityId polyline = seed(Polyline{{{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}}});
    const EntityId line = seed(Line{{0, 20}, {10, 20}});
    const EntityId circle = seed(Circle{{30, 30}, 5});
    const EntityId arc = seed(Arc{{0, 0}, 3, 0.0, 1.0});
    explode({polyline, line, circle, arc});
    EXPECT_EQ(document().find(polyline), nullptr);
    EXPECT_NE(document().find(line), nullptr);
    EXPECT_NE(document().find(circle), nullptr);
    EXPECT_NE(document().find(arc), nullptr);
    EXPECT_EQ(document().entityCount(), 5U);
    EXPECT_EQ(count("3 were not able to be exploded."), 1U);
    EXPECT_EQ(editor().undoName(), "Explode");
}

TEST_F(ExplodeCommandTest, NewEntitiesTakePropertiesAndUnhandledDataExceptReactors) {
    core::EntityExtras extras;
    extras.beforeEntity = {{102, "{ACAD_REACTORS"}, {330, "OWNER"}, {102, "}"}, {330, "KEEP"}};
    extras.common = {{60, "1"}};
    extras.extended = {{1001, "APP"}, {1000, "value"}};
    core::EntityProperties properties = document().current();
    properties.color = core::Color::fromIndex(5);
    const core::EntityRecord source{EntityId{300},
                                    Polyline{{{{0, 0}, 0.0}, {{10, 0}, 0.0}, {{10, 10}, 0.0}}},
                                    extras, 7, properties};
    document().insert(source);
    explode({source.id});
    ASSERT_EQ(document().entityCount(), 2U);
    core::EntityExtras expected = extras;
    expected.beforeEntity = {{330, "KEEP"}};
    for (const core::EntityRecord& record : document().entities()) {
        EXPECT_GT(record.id.value, source.id.value);
        EXPECT_EQ(record.extras, expected);
        EXPECT_EQ(record.properties, properties);
    }
    // Undo puts the polyline back as it was, reactors included.
    runner().submitText("U");
    ASSERT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(*document().find(source.id), source);
}

TEST_F(ExplodeCommandTest, ExplodeAsksForTheSelectionAfterTheCommand) {
    const EntityId id = seed(Polyline{{{{0, 0}, 0.0}, {{10, 0}, 0.0}}});
    runner().submitText("EXPLODE");
    EXPECT_EQ(runner().promptText(), "Select objects:");
    runner().click({5, 0}, PickAction::Add);
    runner().submitText("");
    EXPECT_FALSE(runner().isActive());
    EXPECT_EQ(document().find(id), nullptr);
    EXPECT_EQ(document().entityCount(), 1U);
}

}  // namespace
}  // namespace tg::cmd
