// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

using core::Arc;
using core::Line;
using core::Polyline;

class PeditCommandTest : public testing::Test {
protected:
    PeditCommandTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setPixelSize(0.01);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
        m_runner.setDescriptionCheck([](const std::string& text) { ADD_FAILURE() << text; });
    }

    void type(std::initializer_list<std::string_view> lines) {
        for (const auto line : lines) {
            m_runner.submitText(line);
        }
    }

    core::EntityId addPolyline(core::Vec2 first, core::Vec2 second, std::string_view height = {}) {
        Polyline polyline;
        polyline.addVertex(first);
        polyline.addVertex(second);
        const auto id = m_document.add(polyline);
        if (!height.empty()) {
            auto record = m_document.remove(id);
            record->extras.placement.push_back({38, std::string{height}});
            m_document.restore(std::move(*record));
        }
        return id;
    }

    const Polyline& polyline(core::EntityId id) const {
        return std::get<Polyline>(m_document.find(id)->entity);
    }

    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(PeditCommandTest, ConversionCanBeDeclinedCancelledOrAcceptedWithSourceData) {
    const auto source = m_document.add(Line{{0, 0}, {30, 0}});
    auto record = m_document.remove(source);
    record->properties.color = core::Color::fromIndex(3);
    record->extras.extended = {{1001, "PEDIT_TEST"}, {1000, "kept"}};
    const auto original = *record;
    m_document.restore(std::move(*record));

    type({"PEDIT", "15,0", "No"});
    EXPECT_FALSE(m_runner.isActive());
    EXPECT_EQ(*m_document.find(source), original);
    EXPECT_FALSE(m_editor.canUndo());

    type({"PEDIT", "15,0"});
    EXPECT_EQ(m_runner.promptText(), "Do you want to turn it into one? <Y>");
    m_runner.submit(CancelInput{});
    EXPECT_EQ(*m_document.find(source), original);
    EXPECT_FALSE(m_editor.canUndo());

    type({"PE", "15,0", "Yes", ""});
    ASSERT_EQ(m_document.entityCount(), 1U);
    EXPECT_EQ(m_document.find(source), nullptr);
    const auto& converted = m_document.entities().front();
    EXPECT_NE(converted.id, source);
    EXPECT_EQ(converted.properties, original.properties);
    EXPECT_EQ(converted.extras, original.extras);
    EXPECT_EQ(std::get<Polyline>(converted.entity),
              core::toPolyline(std::get<Line>(original.entity)));
    EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Pedit"});
    type({"U"});
    EXPECT_EQ(*m_document.find(source), original);
    type({"REDO"});
    EXPECT_EQ(m_document.find(source), nullptr);
    EXPECT_EQ(m_document.entityCount(), 1U);
}

TEST_F(PeditCommandTest, PeditacceptIsValidatedRememberedAndHasNoUndoStep) {
    type({"PEDITACCEPT", "2", "1.0", "-1"});
    EXPECT_TRUE(m_runner.isActive());
    EXPECT_EQ(m_messages.back(), "Requires an integer between 0 and 1.");
    type({"+1"});
    EXPECT_FALSE(m_editor.canUndo());
    type({"PEDITACCEPT"});
    EXPECT_EQ(m_runner.promptText(), "Enter new value for PEDITACCEPT <1>:");
    type({""});

    const auto source = m_document.add(Arc{{50, 0}, 10, 0, core::kPi});
    m_messages.clear();
    type({"PEDIT", "50,10", ""});
    EXPECT_EQ(m_document.find(source), nullptr);
    ASSERT_EQ(m_document.entityCount(), 1U);
    const auto& result = std::get<Polyline>(m_document.entities().front().entity);
    EXPECT_EQ(result.vertices()[0].position, (core::Vec2{60, 0}));
    EXPECT_EQ(result.vertices()[1].position, (core::Vec2{40, 0}));
    EXPECT_NEAR(result.vertices()[0].bulge, 1.0, 1e-12);
    EXPECT_EQ(std::ranges::find(m_messages, "Object selected is not a polyline"), m_messages.end());
    type({"PEDITACCEPT", "0", "PEDITACCEPT"});
    EXPECT_EQ(m_runner.promptText(), "Enter new value for PEDITACCEPT <0>:");
    type({""});
    EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Pedit"});
}

TEST_F(PeditCommandTest, OptionsUndoAndGlobalUndoKeepPolylineIdentityAndData) {
    Polyline original{{{{0, 0}, 0.5, 1, 2}, {{30, 0}, -0.25, 3, 4}, {{30, 20}, 0, 5, 6}}};
    original.setConstantWidth(7);
    const auto id = m_document.add(original);
    type({"PEDIT", "30,20", "C", "R", "U"});
    EXPECT_TRUE(polyline(id).isClosed());
    EXPECT_EQ(polyline(id).vertices()[0], original.vertices()[0]);
    EXPECT_DOUBLE_EQ(polyline(id).constantWidth(), 7);
    type({"U"});
    EXPECT_EQ(polyline(id), original);
    type({"U"});
    EXPECT_EQ(m_messages.back(), "Nothing to undo.");
    type({"L", "ON", "R"});
    auto expected = core::reversed(original);
    expected.setContinuousLinetype(true);
    EXPECT_EQ(polyline(id), expected);
    type({"C", "O", ""});
    EXPECT_FALSE(polyline(id).isClosed());
    EXPECT_EQ(m_editor.undoNames(), std::vector<std::string>{"Pedit"});
    type({"U"});
    EXPECT_EQ(polyline(id), original);
    type({"REDO"});
    EXPECT_FALSE(polyline(id).isClosed());
    EXPECT_TRUE(polyline(id).continuousLinetype());
}

TEST_F(PeditCommandTest, JoinAtZeroKeepsSourceAndUndoRestoresConsumedObjects) {
    const auto source = addPolyline({0, 0}, {20, 0});
    const auto line = m_document.add(Line{{20, 0}, {20, 20}});
    const auto arc = m_document.add(Arc{{10, 20}, 10, 0, core::kPi});
    type({"PEDIT", "10,0", "J", "20,10", "10,30", ""});
    EXPECT_EQ(m_document.entityCount(), 1U);
    EXPECT_EQ(polyline(source).segmentCount(), 3U);
    EXPECT_EQ(m_messages.back(), "2 segments added to polyline");
    type({"U"});
    EXPECT_EQ(m_document.entityCount(), 3U);
    EXPECT_NE(m_document.find(line), nullptr);
    EXPECT_NE(m_document.find(arc), nullptr);
    EXPECT_EQ(polyline(source).segmentCount(), 1U);
    type({"J", "20,10", "10,30", "", ""});
    EXPECT_EQ(m_editor.undoName(), "Pedit");
    type({"U"});
    EXPECT_EQ(m_document.entityCount(), 3U);
    EXPECT_EQ(polyline(source).segmentCount(), 1U);
}

TEST_F(PeditCommandTest, MultipleConvertsInSelectionOrderAndJoinsWithEachType) {
    for (const std::string_view joinType : {"E", "A", "B"}) {
        const auto first = m_document.add(Line{{0, 0}, {20, 0}});
        const auto second = m_document.add(Line{{21, 1}, {21, 15}});
        type({"PEDIT", "M", "21,10", "10,0", ""});
        EXPECT_EQ(m_runner.promptText(),
                  "Convert Lines, Arcs and Splines to polylines [Yes/No]? <Y>");
        type({"", "J", "J", joinType, "2", ""});
        ASSERT_EQ(m_document.entityCount(), 1U);
        const auto& result = std::get<Polyline>(m_document.entities().front().entity);
        const bool add = joinType == "A";
        EXPECT_EQ(result.vertexCount(), add ? 4U : 3U);
        EXPECT_EQ(result.vertices().front().position, (core::Vec2{0, 0}));
        EXPECT_EQ(result.vertices().back().position, (core::Vec2{21, 15}));
        EXPECT_EQ(result.vertices()[1].position, (add ? core::Vec2{20, 0} : core::Vec2{21, 0}));
        type({"U"});
        EXPECT_NE(m_document.find(first), nullptr);
        EXPECT_NE(m_document.find(second), nullptr);
        m_document.remove(first);
        m_document.remove(second);
    }
}

TEST_F(PeditCommandTest, ElevationFiltersJoinCandidatesAndMapsConsumedIndices) {
    struct Heights {
        std::string_view source;
        std::string_view other;
        bool joined;
    };

    const std::vector<Heights> cases{
        {"", "1", false},
        {"1", "", false},
        {"1", "1.0", true},
    };
    for (const auto& heights : cases) {
        for (const bool multiple : {false, true}) {
            const auto source = addPolyline({0, 0}, {1, 0}, heights.source);
            const auto other = addPolyline({1, 0}, {2, 0}, heights.other);
            // This first candidate is filtered out. The second candidate's
            // consumed index must still refer to the correct selected ID.
            const auto filtered = addPolyline({1, 0}, {1, 1}, "2");
            if (multiple) {
                type({"PEDIT", "M", "0.5,0", "1,0.5", "1.5,0", "", "J", "0", ""});
            } else {
                type({"PEDIT", "0.5,0", "J", "1,0.5", "1.5,0", "", ""});
            }
            EXPECT_EQ(polyline(source).segmentCount(), heights.joined ? 2U : 1U);
            EXPECT_EQ(m_document.find(other) == nullptr, heights.joined);
            EXPECT_NE(m_document.find(filtered), nullptr);
            type({"U"});
            EXPECT_NE(m_document.find(other), nullptr);
            m_document.remove(source);
            m_document.remove(other);
            m_document.remove(filtered);
        }
    }
}

TEST_F(PeditCommandTest, EscapeKeepsConversionAndCompletedOptions) {
    const auto source = m_document.add(Line{{0, 0}, {30, 0}});
    type({"PEDIT", "15,0", "Y", "C", "L"});
    m_runner.submit(CancelInput{});
    EXPECT_FALSE(m_runner.isActive());
    ASSERT_EQ(m_document.entityCount(), 1U);
    const auto id = m_document.entities().front().id;
    EXPECT_TRUE(polyline(id).isClosed());
    EXPECT_FALSE(polyline(id).continuousLinetype());
    EXPECT_EQ(m_document.find(source), nullptr);
    type({"U"});
    EXPECT_NE(m_document.find(source), nullptr);
    EXPECT_EQ(m_document.find(id), nullptr);
}

TEST_F(PeditCommandTest, MultipleCanDeclineConversionAndEditExistingPolylines) {
    const auto source = addPolyline({0, 0}, {30, 0});
    const auto line = m_document.add(Line{{0, 20}, {30, 20}});
    type({"PEDIT", "M", "15,0", "15,20", "", "No", "C", ""});
    EXPECT_TRUE(polyline(source).isClosed());
    EXPECT_TRUE(std::holds_alternative<Line>(m_document.find(line)->entity));
    EXPECT_EQ(m_document.entityCount(), 2U);
}

}  // namespace
}  // namespace tg::cmd
