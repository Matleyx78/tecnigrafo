// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/polyline.hpp"

#include <gtest/gtest.h>

#include <initializer_list>
#include <string_view>

namespace tg::cmd {
namespace {

class HatcheditTest : public testing::Test {
protected:
    HatcheditTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setPixelSize(0.1);
        m_runner.setDescriptionCheck([](const std::string& error) { ADD_FAILURE() << error; });
        core::Polyline boundary;
        for (const core::Vec2 point : {core::Vec2{0, 0}, {40, 0}, {40, 20}, {0, 20}}) {
            boundary.addVertex(point);
        }
        boundary.setClosed(true);
        m_document.add(boundary);
        type({"-HATCH", "10,10", ""});
        m_hatchId = m_document.entities().back().id;
    }

    void type(std::initializer_list<std::string_view> lines) {
        for (const std::string_view line : lines) {
            m_runner.submitText(line);
        }
    }

    void selectHatch() { m_runner.click({10, 10}, PickAction::Add); }

    [[nodiscard]] const core::EntityRecord& record() const { return *m_document.find(m_hatchId); }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const edit::Editor& editor() const { return m_editor; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] const core::Hatch& hatch() const {
        return std::get<core::Hatch>(record().entity);
    }

private:
    core::Document m_document = core::Document::newDrawing();
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    core::EntityId m_hatchId{};
};

TEST_F(HatcheditTest, ChangesPropertiesAndRemembersThemForTheNextHatch) {
    type({"-HATCHEDIT"});
    selectHatch();
    type({"P", "NET", "2", "30"});

    EXPECT_EQ(hatch().pattern, "NET");
    EXPECT_DOUBLE_EQ(hatch().scale, 2.0);
    EXPECT_DOUBLE_EQ(hatch().angle, core::reducedDegreesToRadians(30.0));
    EXPECT_EQ(editor().undoName(), "Hatchedit");

    type({"-HATCH", "P"});
    EXPECT_EQ(runner().promptText(), "Enter a pattern name or [Solid] <NET>:");
    runner().submit(CancelInput{});
}

TEST_F(HatcheditTest, ChangesAndStoresPatternOriginAndUndoRestoresExtras) {
    type({"HE"});
    selectHatch();
    type({"O", "S", "5,6", "Y"});

    EXPECT_EQ(core::patternOrigin(record().extras), (core::Vec2{5, 6}));
    type({"U"});
    EXPECT_EQ(core::patternOrigin(record().extras), (core::Vec2{}));

    type({"-HATCH", "10,10", ""});
    EXPECT_EQ(core::patternOrigin(document().entities().back().extras), (core::Vec2{5, 6}));
}

}  // namespace
}  // namespace tg::cmd
