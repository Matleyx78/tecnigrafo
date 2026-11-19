// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

class DimensionAngularCommandsTest : public testing::Test {
protected:
    DimensionAngularCommandsTest()
        : m_document(core::Document::newDrawing()),
          m_editor(m_document),
          m_runner(m_registry, m_editor) {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string message) { m_messages.push_back(std::move(message)); });
        m_runner.setPixelSize(0.1);
    }

    core::EntityId add(core::Entity entity) {
        m_editor.beginTransaction("fixture");
        const core::EntityId id = m_editor.add(std::move(entity));
        m_editor.commit();
        return id;
    }

    void point(double x, double y) { m_runner.submit(PointInput{{x, y}}); }

    [[nodiscard]] const core::Dimension& dimension() const {
        return std::get<core::Dimension>(m_document.find(m_document.entities().back().id)->entity);
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const std::vector<std::string>& messages() const { return m_messages; }

private:
    core::Document m_document;
    edit::Editor m_editor;
    CommandRegistry m_registry;
    CommandRunner m_runner;
    std::vector<std::string> m_messages;
};

TEST_F(DimensionAngularCommandsTest, AngularSelectsLinesAndQuadrantFixesTheSector) {
    add(core::Line{{0.0, 0.0}, {40.0, 0.0}});
    add(core::Line{{0.0, 0.0}, {30.0, 40.0}});

    runner().submitText("DIMANGULAR");
    runner().click({20.0, 0.0}, PickAction::Add);
    runner().click({15.0, 20.0}, PickAction::Add);
    runner().submitText("Quadrant");
    point(10.0, 10.0);
    point(-30.0, -30.0);

    EXPECT_EQ(dimension().definition.kind, core::DimensionKind::Angular);
    EXPECT_NEAR(dimension().value.measurement, std::atan2(4.0, 3.0), 1e-12);
    EXPECT_GT(dimension().definition.point16.x, 0.0);
    EXPECT_GT(dimension().definition.point16.y, 0.0);
    EXPECT_NEAR(core::distance(dimension().definition.point16, {0.0, 0.0}), std::sqrt(1800.0),
                1e-12);
    EXPECT_EQ(messages().back(), "Dimension text = 53");
}

TEST_F(DimensionAngularCommandsTest, AngularLocationSelectsAllFourSectors) {
    add(core::Line{{-40.0, 0.0}, {40.0, 0.0}});
    add(core::Line{{-30.0, -40.0}, {30.0, 40.0}});
    const std::vector<core::Vec2> locations{
        {10.0, 10.0}, {-10.0, 10.0}, {-10.0, -10.0}, {10.0, -10.0}};
    const std::vector<double> expected{std::atan2(4.0, 3.0), core::kPi - std::atan2(4.0, 3.0),
                                       std::atan2(4.0, 3.0), core::kPi - std::atan2(4.0, 3.0)};

    for (std::size_t index = 0; index < locations.size(); ++index) {
        runner().submitText("DIMANGULAR");
        runner().click({20.0, 0.0}, PickAction::Add);
        runner().click({15.0, 20.0}, PickAction::Add);
        runner().submit(PointInput{locations[index]});
        EXPECT_NEAR(dimension().value.measurement, expected[index], 1e-12);
    }
}

TEST_F(DimensionAngularCommandsTest, AngularRejectsAnArcAndPolylineArc) {
    add(core::Arc{{0.0, 0.0}, 10.0, 0.0, core::kPi});
    core::Polyline polyline;
    polyline.addVertex({0.0, 0.0});
    polyline.addVertex({20.0, 0.0}, 1.0);
    polyline.addVertex({20.0, 20.0});
    add(std::move(polyline));

    runner().submitText("DIMANGULAR");
    runner().click({0.0, 10.0}, PickAction::Add);
    EXPECT_EQ(messages().back(), "Object selected is not a line.");
    runner().click({30.0, 10.0}, PickAction::Add);
    EXPECT_EQ(messages().back(), "Object selected is not a line.");
    EXPECT_EQ(runner().promptText(), "Select arc, circle, line, or <specify vertex>:");
}

TEST_F(DimensionAngularCommandsTest, ParallelLinesRepeatTheSecondSelection) {
    add(core::Line{{0.0, 0.0}, {40.0, 0.0}});
    add(core::Line{{0.0, 10.0}, {40.0, 10.0}});
    add(core::Line{{0.0, 0.0}, {0.0, 40.0}});

    runner().submitText("DIMANGULAR");
    runner().click({20.0, 0.0}, PickAction::Add);
    runner().click({20.0, 10.0}, PickAction::Add);
    EXPECT_EQ(messages().back(), "Lines are parallel.");
    EXPECT_EQ(runner().promptText(), "Select second line:");
    runner().click({0.0, 20.0}, PickAction::Add);
    point(10.0, 10.0);
    EXPECT_EQ(dimension().definition.kind, core::DimensionKind::Angular);
}

TEST_F(DimensionAngularCommandsTest, RadiusAndDiameterUseLocationDirection) {
    add(core::Circle{{0.0, 0.0}, 10.0});

    runner().submitText("DIMRADIUS");
    runner().click({10.0, 0.0}, PickAction::Add);
    point(20.0, 15.0);
    EXPECT_EQ(dimension().definition.kind, core::DimensionKind::Radius);
    EXPECT_EQ(dimension().definition.point15, (core::Vec2{8.0, 6.0}));
    EXPECT_DOUBLE_EQ(dimension().value.measurement, 10.0);

    runner().submitText("DIMDIAMETER");
    runner().click({10.0, 0.0}, PickAction::Add);
    point(20.0, 15.0);
    EXPECT_EQ(dimension().definition.kind, core::DimensionKind::Diameter);
    EXPECT_EQ(dimension().definition.linePoint, (core::Vec2{-8.0, -6.0}));
    EXPECT_DOUBLE_EQ(dimension().value.measurement, 20.0);
}

TEST_F(DimensionAngularCommandsTest, RadiusOnPolylineArcAndInvalidCenterRepeatLocation) {
    core::Polyline polyline;
    polyline.addVertex({-10.0, 0.0}, 1.0);
    polyline.addVertex({10.0, 0.0});
    add(std::move(polyline));

    runner().submitText("DIMRADIUS");
    runner().click({0.0, -10.0}, PickAction::Add);
    point(0.0, 0.0);
    EXPECT_EQ(messages().back(), "Points coincide.");
    EXPECT_EQ(runner().promptText(), "Specify dimension line location or [Text/Angle]:");
    point(0.0, -20.0);
    EXPECT_EQ(dimension().definition.kind, core::DimensionKind::Radius);

    runner().submitText("U");
    runner().submitText("DIMDIAMETER");
    runner().click({0.0, -10.0}, PickAction::Add);
    point(0.0, -20.0);
    EXPECT_EQ(dimension().definition.kind, core::DimensionKind::Diameter);
    EXPECT_DOUBLE_EQ(dimension().value.measurement, 20.0);
}

TEST_F(DimensionAngularCommandsTest, RadialRejectsTheStraightPartOfAPolyline) {
    core::Polyline polyline;
    polyline.addVertex({0.0, 0.0});
    polyline.addVertex({20.0, 0.0}, 1.0);
    polyline.addVertex({20.0, 20.0});
    add(std::move(polyline));

    runner().submitText("DIMRADIUS");
    runner().click({10.0, 0.0}, PickAction::Add);
    EXPECT_EQ(runner().promptText(), "Select arc or circle:");
}

TEST_F(DimensionAngularCommandsTest, AngularVertexRepeatsLocationPrompt) {
    add(core::Line{{0.0, 0.0}, {40.0, 0.0}});
    add(core::Line{{0.0, 0.0}, {30.0, 40.0}});

    runner().submitText("DIMANGULAR");
    runner().click({20.0, 0.0}, PickAction::Add);
    runner().click({15.0, 20.0}, PickAction::Add);
    point(0.0, 0.0);
    EXPECT_EQ(messages().back(), "Points coincide.");
    EXPECT_EQ(runner().promptText(),
              "Specify dimension arc line location or [Text/Angle/Quadrant]:");
}

}  // namespace
}  // namespace tg::cmd
