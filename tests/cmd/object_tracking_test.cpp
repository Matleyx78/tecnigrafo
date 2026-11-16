// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/line.hpp"
#include "tg/core/snap_points.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace tg::cmd {
namespace {

using core::Line;
using core::Vec2;
using Guide = CommandRunner::TrackingState::Guide;
using Points = std::vector<Vec2>;

// Object snap tracking (docs/adr/0049; docs/tappe/0.8/PT34-otrack-cmd.md).
// The pixel size is 1, so the snap aperture, and the aperture of the guides,
// is 10 units. Only the endpoint snap is on, and ortho and polar are off,
// unless a test says otherwise.
class ObjectTrackingTest : public testing::Test {
protected:
    ObjectTrackingTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink(
            [this](std::string text) { m_messages.push_back(std::move(text)); });
        setAids(true, false);
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    [[nodiscard]] const core::Document& document() const { return m_document; }

    void addLine(const Vec2& start, const Vec2& end) { m_document.add(Line{start, end}); }

    /// The line added by LINE, after the `existing` lines of the drawing.
    [[nodiscard]] const Line& drawn(std::size_t existing) const {
        return std::get<Line>(document().find(document().entities()[existing].id)->entity);
    }

    /// Tracking on or off and polar on or off, with only the endpoint snap.
    void setAids(bool tracking, bool polar) {
        DrawingAids aids;
        aids.snapModes = {true, false, false, false, false, false, false, false};
        aids.tracking = tracking;
        aids.polar = polar;
        m_runner.setDrawingAids(aids);
    }

    void hover(const Vec2& point) { m_runner.pointerHeld(point); }

    [[nodiscard]] Points acquired() { return m_runner.tracking().acquired; }

    /// The point the guides give to the pointer at `point`, if any.
    [[nodiscard]] std::optional<Vec2> tracked(const Vec2& point) {
        m_runner.pointerMoved(point);
        return m_runner.tracking().guidePoint;
    }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
    std::vector<std::string> m_messages;
};

TEST_F(ObjectTrackingTest, IsOffInANewSession) {
    EXPECT_FALSE(DrawingAids{}.tracking);
}

TEST_F(ObjectTrackingTest, HoldingOnASnapPointAcquiresAndReleasesIt) {
    addLine({-100, 0}, {0, 0});
    runner().submitText("LINE");
    hover({3, 4});  // 5 from the endpoint: the snap point is acquired
    EXPECT_EQ(acquired(), (Points{{0, 0}}));
    hover({0, 0});  // the same point again releases it
    EXPECT_TRUE(acquired().empty());

    hover({50, 50});  // no snap point there
    EXPECT_TRUE(acquired().empty());
    runner().pointerMoved({0, 0});  // moving over a snap point does not acquire
    EXPECT_TRUE(acquired().empty());
}

TEST_F(ObjectTrackingTest, AcquiresOnlyWithTrackingAndSnapOnAtAPointPrompt) {
    addLine({-100, 0}, {0, 0});
    hover({0, 0});  // no command asks for a point
    EXPECT_TRUE(acquired().empty());

    setAids(false, false);
    runner().submitText("LINE");
    hover({0, 0});
    EXPECT_TRUE(acquired().empty());

    setAids(true, false);
    hover({0, 0});
    EXPECT_EQ(acquired(), (Points{{0, 0}}));
    DrawingAids aids = runner().drawingAids();
    aids.objectSnap = false;  // tracking works only with object snap
    runner().setDrawingAids(aids);
    EXPECT_TRUE(acquired().empty());
    aids.objectSnap = true;
    runner().setDrawingAids(aids);
    hover({0, 0});
    EXPECT_EQ(acquired(), (Points{{0, 0}}));
    setAids(false, false);
    EXPECT_TRUE(acquired().empty());
}

TEST_F(ObjectTrackingTest, TheEighthPointDropsTheOldest) {
    Points ends;
    for (int index = 0; index < 4; ++index) {
        const double x = 30.0 * index;
        addLine({x, 100}, {x, 200});
        ends.insert(ends.end(), {Vec2{x, 100}, Vec2{x, 200}});
    }
    runner().submitText("LINE");
    for (const Vec2& end : ends) {
        hover(end);
    }
    ASSERT_EQ(acquired().size(), CommandRunner::kMaxAcquired);
    EXPECT_EQ(acquired(), (Points{ends.begin() + 1, ends.end()}));
}

TEST_F(ObjectTrackingTest, APointerNearAGuideGoesOntoIt) {
    addLine({-100, 0}, {0, 0});
    runner().submitText("LINE");
    hover({0, 0});
    EXPECT_EQ(tracked({50, 4}), (Vec2{50, 0}));
    EXPECT_EQ(runner().tracking().guide, Guide::Horizontal);
    EXPECT_EQ(runner().tracking().from, (Vec2{0, 0}));

    EXPECT_EQ(tracked({50, 11}), std::nullopt);  // 11 from the guide: too far
    EXPECT_EQ(runner().tracking().guide, Guide::None);
    EXPECT_EQ(tracked({3, 4}), std::nullopt);  // the snap wins
    EXPECT_EQ(runner().activeSnap(), (core::SnapPoint{{0, 0}, core::SnapKind::Endpoint}));

    runner().click({50, 4}, PickAction::Add);  // the command receives (50,0)
    runner().submit(PointInput{{50, 30}});
    EXPECT_EQ(drawn(1), (Line{{50, 0}, {50, 30}}));
}

TEST_F(ObjectTrackingTest, TwoGuidesOfDifferentPointsGiveTheirCrossing) {
    addLine({0, 0}, {0, -50});
    addLine({60, 40}, {100, 40});
    runner().submitText("LINE");
    hover({0, 0});
    hover({60, 40});
    // Within 3 of y = 0 (from (0,0)) and of x = 60 (from (60,40)).
    EXPECT_EQ(tracked({57, 3}), (Vec2{60, 0}));
    EXPECT_EQ(runner().tracking().guide, Guide::Crossing);
    EXPECT_EQ(runner().tracking().from, std::nullopt);
}

TEST_F(ObjectTrackingTest, TheNearestGuideWins) {
    addLine({-100, 0}, {0, 0});
    addLine({100, 6}, {150, 6});
    runner().submitText("LINE");
    hover({0, 0});
    hover({100, 6});
    EXPECT_EQ(tracked({50, 4}), (Vec2{50, 6}));  // 2 from y = 6, 4 from y = 0
    EXPECT_EQ(runner().tracking().from, (Vec2{100, 6}));
    EXPECT_EQ(tracked({50, 2}), (Vec2{50, 0}));  // 2 from y = 0, 4 from y = 6
    EXPECT_EQ(runner().tracking().from, (Vec2{0, 0}));
}

TEST_F(ObjectTrackingTest, TheHorizontalWinsATieWithTheVerticalOfItsPoint) {
    addLine({-100, 0}, {0, 0});
    runner().submitText("LINE");
    hover({0, 0});
    const std::vector<std::pair<Vec2, Vec2>> ties{
        {{8, 8}, {8, 0}}, {{-8, 8}, {-8, 0}}, {{-8, -8}, {-8, 0}}, {{8, -8}, {8, 0}}};
    for (const auto& [pointer, expected] : ties) {
        EXPECT_EQ(tracked(pointer), expected);
        EXPECT_EQ(runner().tracking().guide, Guide::Horizontal);
        EXPECT_EQ(runner().tracking().from, (Vec2{0, 0}));
    }
    // Without a tie the nearer guide wins.
    EXPECT_EQ(tracked({7, 8}), (Vec2{0, 8}));
    EXPECT_EQ(runner().tracking().guide, Guide::Vertical);
    EXPECT_EQ(runner().tracking().from, (Vec2{0, 0}));
}

TEST_F(ObjectTrackingTest, ATieOfCrossingsGoesToTheHorizontalOfTheLatestPoint) {
    addLine({0, 0}, {18, 18});
    runner().submitText("LINE");
    hover({0, 0});
    hover({18, 18});
    EXPECT_EQ(tracked({9, 9}), (Vec2{0, 18}));  // 9 from all four guides
    EXPECT_EQ(runner().tracking().guide, Guide::Crossing);
    runner().submit(CancelInput{});  // ends LINE and drops the points

    runner().submitText("LINE");
    hover({18, 18});
    hover({0, 0});
    EXPECT_EQ(tracked({9, 9}), (Vec2{18, 0}));
}

TEST_F(ObjectTrackingTest, WithTheSameHorizontalTheVerticalOfTheLatestPointWins) {
    addLine({50, 0}, {50, -20});
    addLine({14, 30}, {26, 30});
    for (const auto& [last, expected] :
         {std::pair<Vec2, Vec2>{{26, 30}, {26, 0}}, std::pair<Vec2, Vec2>{{14, 30}, {14, 0}}}) {
        runner().submitText("LINE");
        hover({50, 0});
        hover(last.x == 26 ? Vec2{14, 30} : Vec2{26, 30});
        hover(last);
        // (14,0) and (26,0) are both at sqrt(61) from the pointer.
        EXPECT_EQ(tracked({20, 5}), expected);
        EXPECT_EQ(runner().tracking().guide, Guide::Crossing);
        runner().submit(CancelInput{});
    }
}

TEST_F(ObjectTrackingTest, ACrossingOfGuidesWinsATieWithACrossingOfThePolarRay) {
    addLine({40, 0}, {0, 40});
    setAids(true, true);  // POLARANG is 90 degrees in a new session
    runner().submitText("LINE");
    runner().submit(PointInput{{-20, 10}});
    hover({40, 0});
    hover({0, 40});
    // Within the aperture of y = 0, x = 0 and the polar ray y = 10; the
    // crossings (0,0) and (0,10) are both at sqrt(29).
    EXPECT_EQ(tracked({2, 5}), (Vec2{0, 0}));
    EXPECT_EQ(runner().tracking().guide, Guide::Crossing);
    EXPECT_EQ(runner().activePolar(), std::nullopt);
}

TEST_F(ObjectTrackingTest, AGuideCrossesThePolarRay) {
    addLine({0, 30}, {0, 60});
    addLine({12, 40}, {12, 70});
    setAids(true, true);
    for (const auto& [last, expected] :
         {std::pair<Vec2, Vec2>{{12, 40}, {12, 0}}, std::pair<Vec2, Vec2>{{0, 30}, {0, 0}}}) {
        runner().submitText("LINE");
        runner().submit(PointInput{{-20, 0}});
        hover(last.x == 12 ? Vec2{0, 30} : Vec2{12, 40});
        hover(last);
        // The crossings with the ray y = 0 are both at sqrt(40).
        EXPECT_EQ(tracked({6, 2}), expected);
        EXPECT_EQ(runner().tracking().guide, Guide::WithPolar);
        if (const std::optional<CommandRunner::PolarTracking> polar = runner().activePolar()) {
            EXPECT_EQ(polar->point, expected);  // the polar guide ends at the crossing
            EXPECT_EQ(polar->base, (Vec2{-20, 0}));
        } else {
            ADD_FAILURE() << "no active polar ray";
        }
        runner().click({6, 2}, PickAction::Add);
        EXPECT_EQ(drawn(2), (Line{{-20, 0}, expected}));
        runner().submitText("U");  // undoes the segment, LINE goes on
        runner().submit(CancelInput{});
    }
}

TEST_F(ObjectTrackingTest, DistancesAreComparedSquared) {
    addLine({0, 40}, {0, 60});
    addLine({40, -15}, {60, -15});
    addLine({40, 1}, {60, 1});
    runner().submitText("LINE");
    hover({0, 40});
    hover({40, -15});
    hover({40, 1});
    // The squares are 64.99999999999999 and 65; their roots would be equal.
    EXPECT_EQ(tracked({1, -7.000000000000001}), (Vec2{0, -15}));
    EXPECT_EQ(runner().tracking().guide, Guide::Crossing);
}

TEST_F(ObjectTrackingTest, ADistanceTypedAloneGoesAlongTheGuide) {
    addLine({-100, 0}, {0, 0});
    runner().submitText("LINE");
    runner().submit(PointInput{{-50, -50}});
    hover({0, 0});
    runner().pointerMoved({30, 3});  // the horizontal guide, towards +x
    runner().submitText("12");
    EXPECT_EQ(drawn(1), (Line{{-50, -50}, {12, 0}}));
    EXPECT_TRUE(acquired().empty());

    hover({0, 0});
    runner().pointerMoved({2, -30});  // the vertical guide, towards -y
    runner().submitText("5");
    EXPECT_EQ(drawn(2), (Line{{12, 0}, {0, -5}}));
}

TEST_F(ObjectTrackingTest, ThePointsAreDroppedWhenThePromptEnds) {
    addLine({-100, 0}, {0, 0});
    runner().submitText("LINE");
    hover({0, 0});
    runner().submit(PointInput{{50, 50}});  // a point
    EXPECT_TRUE(acquired().empty());

    hover({0, 0});
    runner().submitText("");  // the command finishes
    EXPECT_TRUE(acquired().empty());

    runner().submitText("LINE");
    hover({0, 0});
    runner().submit(CancelInput{});  // the command is cancelled
    EXPECT_TRUE(acquired().empty());
    EXPECT_EQ(runner().tracking().guide, Guide::None);
}

}  // namespace
}  // namespace tg::cmd
