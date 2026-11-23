// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <variant>

namespace tg::cmd {
namespace {

class RectangCommandTest : public testing::Test {
protected:
    RectangCommandTest() { registerBuiltinCommands(m_registry); }

    void text(const char* value) { m_runner.submitText(value); }

    void point(double x, double y) { m_runner.submit(PointInput{{x, y}}); }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    [[nodiscard]] const edit::Editor& editor() const { return m_editor; }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const core::EntityRecord& record() const {
        return *m_document.find(m_document.entities().front().id);
    }

    [[nodiscard]] const core::Polyline& polyline() const {
        return std::get<core::Polyline>(record().entity);
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
};

TEST_F(RectangCommandTest, RegistersNameAndAliases) {
    for (const char* name : {"RECTANG", "REC", "RECTANGLE"}) {
        text(name);
        EXPECT_EQ(runner().promptText(),
                  "Specify first corner point or [Chamfer/Elevation/Fillet/Thickness/Width]:");
        runner().submit(CancelInput{});
    }
}

TEST_F(RectangCommandTest, DrawsClosedPolylineInOneUndoStep) {
    text("RECTANG");
    point(0.0, 0.0);
    point(40.0, 20.0);
    ASSERT_EQ(document().entityCount(), 1U);
    ASSERT_EQ(polyline().vertexCount(), 4U);
    EXPECT_TRUE(polyline().isClosed());
    EXPECT_EQ(polyline().vertices()[0].position, (core::Vec2{0.0, 0.0}));
    EXPECT_EQ(polyline().vertices()[2].position, (core::Vec2{40.0, 20.0}));
    EXPECT_EQ(editor().undoName(), "Rectang");
}

TEST_F(RectangCommandTest, RecordsZeroConstantWidthForDxfOutput) {
    text("RECTANG");
    point(0.0, 0.0);
    point(40.0, 20.0);
    ASSERT_EQ(record().extras.specific.size(), 1U);
    EXPECT_EQ(record().extras.specific.front(), (core::RawGroup{43, "0.0"}));
}

TEST_F(RectangCommandTest, PreservesElevationAndThicknessPrecision) {
    text("RECTANG");
    text("E");
    text("0.03125");
    text("T");
    text("1.234567");
    point(0.0, 0.0);
    point(40.0, 20.0);
    ASSERT_EQ(record().extras.placement.size(), 2U);
    EXPECT_EQ(record().extras.placement[0], (core::RawGroup{38, "0.03125"}));
    EXPECT_EQ(record().extras.placement[1], (core::RawGroup{39, "1.234567"}));
}

TEST_F(RectangCommandTest, RotationAndDimensionsUseExactCardinalDirections) {
    text("RECTANG");
    point(0.0, 0.0);
    text("R");
    text("90");
    text("D");
    text("40");
    text("20");
    point(-10.0, 50.0);
    ASSERT_EQ(polyline().vertexCount(), 4U);
    EXPECT_EQ(polyline().vertices()[1].position, (core::Vec2{0.0, 40.0}));
    EXPECT_EQ(polyline().vertices()[2].position, (core::Vec2{-20.0, 40.0}));
}

TEST_F(RectangCommandTest, FilletUsesDeterministicQuarterCircleBulges) {
    text("RECTANG");
    text("F");
    text("4");
    point(0.0, 0.0);
    point(40.0, 20.0);
    ASSERT_EQ(polyline().vertexCount(), 8U);
    EXPECT_EQ(polyline().vertices()[0].position, (core::Vec2{0.0, 4.0}));
    EXPECT_DOUBLE_EQ(polyline().vertices()[0].bulge, 0.4142135623730951);
}

TEST_F(RectangCommandTest, OversizedChamferRepeatsEveryCorner) {
    text("RECTANG");
    text("C");
    text("25");
    text("25");
    point(0.0, 0.0);
    point(40.0, 20.0);
    ASSERT_EQ(polyline().vertexCount(), 8U);
    EXPECT_EQ(polyline().vertices()[0].position, polyline().vertices()[1].position);
    EXPECT_EQ(polyline().vertices()[6].position, polyline().vertices()[7].position);
}

}  // namespace
}  // namespace tg::cmd
