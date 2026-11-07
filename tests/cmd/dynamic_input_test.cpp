// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/dynamic_input.hpp"

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/cmd/drawing_aids.hpp"
#include "tg/core/document.hpp"
#include "tg/core/vec2.hpp"
#include "tg/edit/editor.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace tg::cmd {
namespace {

using Kind = DynamicField::Kind;

/// A runner with its document and the history of its messages.
class Session {
public:
    Session() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
        // A small aperture keeps the pointers of these tests off any snap.
        m_runner.setPixelSize(0.01);
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const CommandRunner& runner() const { return m_runner; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] const edit::Editor& editor() const { return m_editor; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

/// Two sessions driven alike, one with the dynamic input and one with the
/// command line, whose results must stay the same (docs/adr/0042, point 1).
class DynamicInputTest : public testing::Test {
protected:
    /// The same text on both command lines.
    void both(const std::string& text) {
        m_dynamic.runner().submitText(text);
        m_written.runner().submitText(text);
    }

    void pointerBoth(const core::Vec2& pointer) {
        m_dynamic.runner().pointerMoved(pointer);
        m_written.runner().pointerMoved(pointer);
    }

    void aidsBoth(const DrawingAids& aids) {
        m_dynamic.runner().setDrawingAids(aids);
        m_written.runner().setDrawingAids(aids);
    }

    /// The fields for the dynamic session's prompt, with `typed` written in
    /// them in order.
    [[nodiscard]] DynamicInput fields(const std::vector<std::string>& typed) {
        DynamicInput input = dynamicInputFor(m_dynamic.runner());
        EXPECT_LE(typed.size(), input.fields.size());
        for (std::size_t index = 0; index < typed.size() && index < input.fields.size(); ++index) {
            input.fields[index].typed = typed[index];
        }
        return input;
    }

    /// Enter on the dynamic input of one session with the pointer at
    /// `pointer`, the line `expected` on the command line of the other: the
    /// composed line is `expected`, and the two sessions end alike.
    void enter(const DynamicInput& input, const core::Vec2& pointer, const std::string& expected) {
        pointerBoth(pointer);
        const std::string text = dynamicInputText(input, m_dynamic.runner(), pointer);
        EXPECT_EQ(text, expected);
        m_dynamic.runner().submitText(text);
        m_written.runner().submitText(expected);
        expectSame();
    }

    void expectSame() const {
        ASSERT_EQ(m_dynamic.document().entityCount(), m_written.document().entityCount());
        for (std::size_t index = 0; index < m_written.document().entityCount(); ++index) {
            EXPECT_EQ(m_dynamic.document().entities()[index],
                      m_written.document().entities()[index]);
        }
        EXPECT_EQ(m_dynamic.messages(), m_written.messages());
        EXPECT_EQ(m_dynamic.runner().promptText(), m_written.runner().promptText());
        EXPECT_EQ(m_dynamic.runner().isActive(), m_written.runner().isActive());
        EXPECT_EQ(m_dynamic.editor().undoName(), m_written.editor().undoName());
    }

    [[nodiscard]] Session& dynamic() { return m_dynamic; }

    [[nodiscard]] static std::vector<Kind> kinds(const DynamicInput& input) {
        std::vector<Kind> result;
        result.reserve(input.fields.size());
        for (const DynamicField& field : input.fields) {
            result.push_back(field.kind);
        }
        return result;
    }

private:
    Session m_dynamic;
    Session m_written;
};

TEST_F(DynamicInputTest, LineFirstPointFromTypedXAndY) {
    both("LINE");
    const DynamicInput input = fields({"1.5", "-2"});
    EXPECT_EQ(input.prompt, "Specify first point:");
    EXPECT_EQ(kinds(input), (std::vector<Kind>{Kind::X, Kind::Y}));
    EXPECT_EQ(input.active, 0U);
    enter(input, {7.0, 8.0}, "1.5,-2");
}

TEST_F(DynamicInputTest, LineFirstPointTakesTheFreeFieldFromThePointer) {
    both("LINE");
    const DynamicInput empty = dynamicInputFor(dynamic().runner());
    EXPECT_EQ(dynamicInputShown(empty, dynamic().runner(), {1.23456, 2.0}),
              (std::vector<std::string>{"1.2346", "2.0000"}));
    const DynamicInput input = fields({"3"});
    EXPECT_EQ(dynamicInputShown(input, dynamic().runner(), {1.23456, 2.0}),
              (std::vector<std::string>{"3", "2.0000"}));
    enter(input, {1.23456, 2.0}, "3,2.0000");
}

TEST_F(DynamicInputTest, LineNextPointFromTypedLengthAndAngle) {
    both("LINE");
    both("0,0");
    const DynamicInput input = fields({"5", "30"});
    EXPECT_EQ(kinds(input), (std::vector<Kind>{Kind::Length, Kind::Angle}));
    enter(input, {-3.0, -1.0}, "@5<30");
}

TEST_F(DynamicInputTest, LineNextPointWithOnlyTheLengthTakesTheAngleFromThePointer) {
    both("LINE");
    both("1,1");
    const DynamicInput empty = dynamicInputFor(dynamic().runner());
    EXPECT_EQ(dynamicInputShown(empty, dynamic().runner(), {1.0, 11.0}),
              (std::vector<std::string>{"10.0000", "90"}));
    EXPECT_EQ(dynamicInputShown(empty, dynamic().runner(), {1.0, -9.0}),
              (std::vector<std::string>{"10.0000", "270"}));
    enter(fields({"4"}), {1.0, 11.0}, "@4<90");
    enter(fields({"2"}), {1.0, -9.0}, "@2<270");
}

TEST_F(DynamicInputTest, LineNextPointWithOnlyTheAngleTakesTheLengthFromThePointer) {
    both("LINE");
    both("0,0");
    DynamicInput input = dynamicInputFor(dynamic().runner());
    input.fields[1].typed = "45";
    input.active = 1;
    enter(input, {3.0, 4.0}, "@5.0000<45");
}

TEST_F(DynamicInputTest, LineNextPointWithACommaInTheLengthIsRelativeCartesian) {
    both("LINE");
    both("2,3");
    enter(fields({"3,4", "60"}), {0.0, 0.0}, "@3,4");
    enter(fields({"-1,0"}), {0.0, 0.0}, "@-1,0");
}

TEST_F(DynamicInputTest, EnterWithNoFieldTypedEndsLine) {
    both("LINE");
    both("0,0");
    both("10,0");
    const DynamicInput input = dynamicInputFor(dynamic().runner());
    ASSERT_FALSE(input.fields.empty());
    enter(input, {20.0, 5.0}, "");
    EXPECT_FALSE(dynamic().runner().isActive());
    EXPECT_EQ(dynamic().document().entityCount(), 1U);
}

TEST_F(DynamicInputTest, CircleRadiusFromTypedValue) {
    both("CIRCLE");
    enter(fields({"0", "0"}), {0.0, 0.0}, "0,0");
    const DynamicInput input = fields({"5"});
    EXPECT_EQ(kinds(input), (std::vector<Kind>{Kind::Value}));
    EXPECT_EQ(
        dynamicInputShown(dynamicInputFor(dynamic().runner()), dynamic().runner(), {3.0, 4.0}),
        (std::vector<std::string>{"5.0000"}));
    enter(input, {3.0, 4.0}, "5");
    EXPECT_FALSE(dynamic().runner().isActive());
    EXPECT_EQ(dynamic().document().entityCount(), 1U);
}

TEST_F(DynamicInputTest, WithOrthoTheFreeAngleFollowsThePointCorrectedByOrtho) {
    DrawingAids aids;
    aids.ortho = true;
    aidsBoth(aids);
    both("LINE");
    both("0,0");
    const DynamicInput empty = dynamicInputFor(dynamic().runner());
    EXPECT_EQ(dynamicInputShown(empty, dynamic().runner(), {10.0, 2.0}),
              (std::vector<std::string>{"10.0000", "0"}));
    EXPECT_EQ(dynamicInputShown(empty, dynamic().runner(), {2.0, -10.0}),
              (std::vector<std::string>{"10.0000", "270"}));
    enter(fields({"5"}), {10.0, 2.0}, "@5<0");
    enter(fields({"3"}), {6.0, 10.0}, "@3<90");
}

TEST_F(DynamicInputTest, TextAndValuePromptsAndNoFieldAtTheTextPrompt) {
    both("TEXT");
    enter(fields({"1", "1"}), {0.0, 0.0}, "1,1");
    EXPECT_EQ(kinds(dynamicInputFor(dynamic().runner())), (std::vector<Kind>{Kind::Value}));
    enter(fields({"3"}), {0.0, 0.0}, "3");
    EXPECT_EQ(kinds(dynamicInputFor(dynamic().runner())), (std::vector<Kind>{Kind::Value}));
    enter(fields({"15"}), {0.0, 0.0}, "15");
    const DynamicInput input = dynamicInputFor(dynamic().runner());
    EXPECT_EQ(input.prompt, "Enter text:");
    EXPECT_TRUE(input.fields.empty());
    EXPECT_EQ(dynamicInputText(input, dynamic().runner(), {0.0, 0.0}), "");
    both("Hello");
    both("");
    expectSame();
}

TEST_F(DynamicInputTest, NoFieldWhileSelectingOrWithNoCommand) {
    const DynamicInput idle = dynamicInputFor(dynamic().runner());
    EXPECT_EQ(idle.prompt, "Command:");
    EXPECT_TRUE(idle.fields.empty());
    both("LINE");
    both("0,0");
    both("10,0");
    both("");
    both("ERASE");
    const DynamicInput selecting = dynamicInputFor(dynamic().runner());
    EXPECT_EQ(selecting.prompt, "Select objects:");
    EXPECT_TRUE(selecting.fields.empty());
    EXPECT_EQ(dynamicInputText(selecting, dynamic().runner(), {5.0, 0.0}), "");
    both("5,0");
    both("");
    expectSame();
    EXPECT_TRUE(dynamic().document().isEmpty());
}

// COPY keeps its base point for the copies after the first, while the last
// point moves to each copy: the fields measure from the base point, the
// composed "@" line from the last point (docs/adr/0042, point 3). The
// guarantee is the composed line, not the point the fields show.
TEST_F(DynamicInputTest, CopyAfterTheFirstMeasuresFromTheBaseAndMovesFromTheLastPoint) {
    both("LINE");
    both("0,0");
    both("10,0");
    both("");
    both("COPY");
    both("5,0");
    both("");
    both("0,0");
    both("10,0");
    const DynamicInput empty = dynamicInputFor(dynamic().runner());
    EXPECT_EQ(empty.prompt, "Specify second point or [Exit/Undo] <Exit>:");
    EXPECT_EQ(kinds(empty), (std::vector<Kind>{Kind::Length, Kind::Angle}));
    EXPECT_EQ(dynamicInputShown(empty, dynamic().runner(), {20.0, 0.0}),
              (std::vector<std::string>{"20.0000", "0"}));
    enter(fields({"20", "0"}), {20.0, 0.0}, "@20<0");

    // The same copy written as the absolute point (30,0): 20 from the last
    // point (10,0), not from the base point (0,0).
    Session absolute;
    for (const char* text : {"LINE", "0,0", "10,0", "", "COPY", "5,0", "", "0,0", "10,0", "30,0"}) {
        absolute.runner().submitText(text);
    }
    ASSERT_EQ(absolute.document().entityCount(), dynamic().document().entityCount());
    EXPECT_EQ(absolute.document().entities().back(), dynamic().document().entities().back());
}

TEST(CommandRunnerDynamicInputTest, BasePointAndClickPointChangeNothing) {
    core::Document document;
    edit::Editor editor{document};
    CommandRegistry registry;
    registerBuiltinCommands(registry);
    CommandRunner runner{registry, editor};
    runner.setPixelSize(0.01);
    EXPECT_FALSE(runner.basePoint().has_value());
    EXPECT_EQ(runner.clickPoint({3.0, 4.0}), (core::Vec2{3.0, 4.0}));
    DrawingAids aids;
    aids.ortho = true;
    runner.setDrawingAids(aids);
    runner.submitText("LINE");
    EXPECT_FALSE(runner.basePoint().has_value());
    runner.submitText("1,1");
    EXPECT_EQ(runner.basePoint(), (std::optional<core::Vec2>{core::Vec2{1.0, 1.0}}));
    EXPECT_EQ(runner.clickPoint({9.0, 2.0}), (core::Vec2{9.0, 1.0}));
    EXPECT_EQ(document.entityCount(), 0U);
    EXPECT_EQ(runner.promptText(), "Specify next point or [Undo]:");
}

}  // namespace
}  // namespace tg::cmd
