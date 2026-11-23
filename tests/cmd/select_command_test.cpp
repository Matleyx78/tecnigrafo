// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/document.hpp"
#include "tg/core/line.hpp"
#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <initializer_list>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

using Messages = std::vector<std::string>;

// Last, Previous and SELECT (docs/tappe/0.8.1/PT34-last-previous.md).
class SelectCommandTest : public testing::Test {
protected:
    SelectCommandTest() : m_document(core::Document::newDrawing()) {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] core::Document& document() { return m_document; }

    [[nodiscard]] edit::Editor& editor() { return m_editor; }

    /// The messages written since the last call.
    [[nodiscard]] Messages takeMessages() { return std::exchange(m_messages, {}); }

    void forgetMessages() { m_messages.clear(); }

    void type(std::initializer_list<const char*> lines) {
        for (const char* line : lines) {
            m_runner.submitText(line);
        }
    }

    core::EntityId addLine(core::Vec2 start, core::Vec2 end) {
        m_editor.beginTransaction("Line");
        const core::EntityId id = m_editor.add(core::Line{start, end});
        m_editor.commit();
        return id;
    }

    [[nodiscard]] const core::Line& lineOf(core::EntityId id) const {
        const core::EntityRecord* record = m_document.find(id);
        EXPECT_NE(record, nullptr);
        return std::get<core::Line>(record->entity);
    }

    /// Adds a line on a new layer, turned off or locked, keeping the current
    /// layer.
    core::EntityId addLineOnNewLayer(const char* name, bool on, bool locked, core::Vec2 start,
                                     core::Vec2 end) {
        core::Layer layer;
        layer.name = name;
        layer.linetype = m_document.continuousLinetype();
        layer.on = on;
        layer.locked = locked;
        const core::CurrentProperties previous = m_document.current();
        core::CurrentProperties onLayer = previous;
        onLayer.layer = m_document.addLayer(layer);
        m_document.setCurrent(onLayer);
        const core::EntityId id = addLine(start, end);
        m_document.setCurrent(previous);
        return id;
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    Messages m_messages;
};

TEST_F(SelectCommandTest, TheSequenceOfTheAutocadSelectionTranscript) {
    const core::EntityId first = addLine({0, 0}, {30, 0});
    addLine({0, 20}, {30, 20});
    const core::EntityId third = addLine({0, 40}, {30, 40});

    // ERASE Last twice: each time the last entity left.
    type({"ERASE", "L", ""});
    EXPECT_EQ(document().find(third), nullptr);
    type({"ERASE", "L", ""});
    EXPECT_EQ(document().entityCount(), 1U);
    EXPECT_EQ(takeMessages(), (Messages{"1 found, 1 total", "1 found, 1 total"}));

    // After an ERASE, Previous holds only erased entities.
    type({"SELECT", "P", ""});
    EXPECT_EQ(takeMessages(), (Messages{"No previous selection set."}));

    // SELECT, then MOVE Previous; U leaves the Previous of the MOVE undone.
    type({"SELECT", "0,0", "", "MOVE", "P", "", "0,0", "0,-10"});
    EXPECT_EQ(lineOf(first).start, (core::Vec2{0, -10}));
    type({"U", "SELECT", "P", ""});
    EXPECT_EQ(lineOf(first).start, (core::Vec2{0, 0}));
    EXPECT_EQ(takeMessages(), (Messages{"1 found, 1 total", "1 found, 1 total", "Undone: Move.",
                                        "1 found, 1 total"}));
    // SELECT changes neither the drawing nor the undo history.
    EXPECT_EQ(editor().undoName(), "Erase");
}

TEST_F(SelectCommandTest, LastSkipsEntitiesOnLayersThatAreOff) {
    const core::EntityId first = addLine({0, 0}, {10, 0});
    const core::EntityId second = addLineOnNewLayer("Off", false, false, {0, 5}, {10, 5});

    type({"ERASE", "L", ""});
    EXPECT_EQ(document().find(first), nullptr);
    EXPECT_NE(document().find(second), nullptr);
}

TEST_F(SelectCommandTest, LastOnALockedLayerIsDiscarded) {
    const core::EntityId first = addLine({0, 0}, {10, 0});
    const core::EntityId second = addLineOnNewLayer("Locked", true, true, {0, 5}, {10, 5});

    type({"ERASE", "L", ""});
    EXPECT_EQ(takeMessages(), (Messages{"0 found, 0 total", "1 was on a locked layer."}));
    EXPECT_NE(document().find(first), nullptr);
    EXPECT_NE(document().find(second), nullptr);
    EXPECT_FALSE(runner().isActive());
}

TEST_F(SelectCommandTest, PreviousAfterEraseIsEmpty) {
    addLine({0, 0}, {10, 0});
    addLine({0, 5}, {10, 5});
    type({"ERASE", "5,0", ""});
    type({"ERASE", "P"});
    EXPECT_EQ(takeMessages(), (Messages{"1 found, 1 total", "No previous selection set."}));
    EXPECT_EQ(runner().promptText(), "Select objects:");
}

TEST_F(SelectCommandTest, AnEmptyQuestionLeavesThePreviousSet) {
    const core::EntityId a = addLine({0, 0}, {10, 0});
    addLine({0, 5}, {10, 5});
    type({"SELECT", "5,0", "", "SELECT", ""});
    EXPECT_FALSE(runner().isActive());
    type({"MOVE", "P"});
    EXPECT_EQ(takeMessages(), (Messages{"1 found, 1 total", "1 found, 1 total"}));
    type({"", "0,0", "1,0"});
    EXPECT_EQ(lineOf(a).start, (core::Vec2{1, 0}));
}

TEST_F(SelectCommandTest, ACancelledQuestionLeavesThePreviousSet) {
    const core::EntityId a = addLine({0, 0}, {10, 0});
    const core::EntityId b = addLine({0, 5}, {10, 5});
    type({"SELECT", "5,0", "", "SELECT", "5,5"});
    runner().submit(CancelInput{});
    forgetMessages();
    type({"ERASE", "P", ""});
    EXPECT_EQ(takeMessages(), (Messages{"1 found, 1 total"}));
    EXPECT_EQ(document().find(a), nullptr);
    EXPECT_NE(document().find(b), nullptr);
}

TEST_F(SelectCommandTest, APreselectionBecomesThePreviousSet) {
    const core::EntityId a = addLine({0, 0}, {10, 0});
    const core::EntityId b = addLine({0, 5}, {10, 5});
    type({"SELECT", "5,0", ""});
    runner().click({5, 5}, PickAction::Add);  // B preselected
    type({"MOVE", "0,0", "0,1"});
    EXPECT_EQ(lineOf(b).start, (core::Vec2{0, 6}));
    forgetMessages();
    type({"MOVE", "P"});
    EXPECT_EQ(takeMessages(), (Messages{"1 found, 1 total"}));
    type({"", "0,0", "0,1"});
    EXPECT_EQ(lineOf(b).start, (core::Vec2{0, 7}));
    EXPECT_EQ(lineOf(a).start, (core::Vec2{0, 0}));
}

TEST_F(SelectCommandTest, ANewDrawingHasNoPreviousSet) {
    addLine({0, 0}, {10, 0});
    type({"SELECT", "5,0", ""});
    type({"NEW", "N"});  // no to saving the changes
    ASSERT_EQ(document().entityCount(), 0U);
    // The same identifier in the new drawing.
    addLine({0, 0}, {10, 0});
    forgetMessages();
    type({"SELECT", "P"});
    EXPECT_EQ(takeMessages(), (Messages{"No previous selection set."}));
}

TEST_F(SelectCommandTest, AllGivesTheMostRecentFirst) {
    addLine({0, 0}, {10, 0});
    editor().beginTransaction("Circle");
    editor().add(core::Circle{{30, 0}, 5});
    editor().commit();
    type({"-BLOCK", "Pezzo", "0,0", "ALL", ""});
    const core::BlockDefinition* block = document().blocks().find("Pezzo");
    ASSERT_NE(block, nullptr);
    ASSERT_EQ(block->entities.size(), 2U);
    EXPECT_TRUE(std::holds_alternative<core::Circle>(block->entities[0].entity));
    EXPECT_TRUE(std::holds_alternative<core::Line>(block->entities[1].entity));
}

TEST_F(SelectCommandTest, SelectIsQuietWithoutObjects) {
    type({"SELECT", ""});
    EXPECT_FALSE(runner().isActive());
    EXPECT_TRUE(takeMessages().empty());
    EXPECT_TRUE(runner().highlighted().empty());
}

}  // namespace
}  // namespace tg::cmd
