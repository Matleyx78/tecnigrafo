// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"
#include "tg/cmd/command_runner.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/snap_points.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <iomanip>
#include <optional>
#include <sstream>
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

// Extension and Parallel (docs/adr/0057, points 3-5;
// docs/tappe/0.8.1/PT31-prolungamento-parallelo.md). Each test sets the snap
// modes, tracking and the aperture it needs; ortho and polar are off.
class ExtensionParallelTest : public testing::Test {
protected:
    using State = CommandRunner::TrackingState;
    using Kind = State::ActiveGuide::Kind;

    ExtensionParallelTest() {
        registerBuiltinCommands(m_registry);
        m_runner.setMessageSink([](const std::string&) {});
    }

    [[nodiscard]] CommandRunner& runner() { return m_runner; }

    core::EntityId add(core::Entity entity) { return m_document.add(std::move(entity)); }

    core::EntityId addLine(const Vec2& start, const Vec2& end) { return add(Line{start, end}); }

    /// The modes listed on, the others off.
    static core::SnapModes modes(std::initializer_list<bool core::SnapModes::*> on) {
        core::SnapModes result{false, false, false, false, false, false,
                               false, false, false, false, false};
        for (bool core::SnapModes::*const mode : on) {
            result.*mode = true;
        }
        return result;
    }

    /// Object snap on with `snapModes`, tracking on or off, and a pixel size
    /// that makes the snap aperture `aperture`.
    void setAids(const core::SnapModes& snapModes, bool tracking, double aperture) {
        DrawingAids aids;
        aids.snapModes = snapModes;
        aids.tracking = tracking;
        m_runner.setDrawingAids(aids);
        m_runner.setPixelSize(aperture / 10.0);
    }

    void setModes(const core::SnapModes& snapModes, bool tracking) {
        DrawingAids aids = m_runner.drawingAids();
        aids.snapModes = snapModes;
        aids.tracking = tracking;
        m_runner.setDrawingAids(aids);
    }

    void hover(const Vec2& point) { m_runner.pointerHeld(point); }

    [[nodiscard]] const State& state() const { return m_runner.tracking(); }

    [[nodiscard]] std::optional<Vec2> tracked(const Vec2& point) {
        m_runner.pointerMoved(point);
        return m_runner.tracking().guidePoint;
    }

    /// The last entity of the drawing, a line.
    [[nodiscard]] const Line& lastLine() const {
        return std::get<Line>(m_document.find(m_document.entities().back().id)->entity);
    }

    [[nodiscard]] std::size_t count() const { return m_document.entities().size(); }

private:
    core::Document m_document;
    edit::Editor m_editor{m_document};
    CommandRegistry m_registry;
    CommandRunner m_runner{m_registry, m_editor};
};

using Acquisition = CommandRunner::TrackingState::Acquisition;
using Extension = Acquisition::Extension;
using Parallel = Acquisition::Parallel;

core::Polyline polyline(std::initializer_list<Vec2> points) {
    core::Polyline result;
    for (const Vec2& point : points) {
        result.addVertex(point);
    }
    return result;
}

void expectNear(const Vec2& obtained, const Vec2& expected) {
    EXPECT_NEAR(obtained.x, expected.x, 1e-9) << obtained.x << "," << obtained.y;
    EXPECT_NEAR(obtained.y, expected.y, 1e-9) << obtained.x << "," << obtained.y;
}

TEST_F(ExtensionParallelTest, TheTwelveLinesOfTheAutoCadDrawingExtension) {
    // autocad-prolungamento-parallelo: lines 97-9C, typed distances along
    // the extensions of a line, of an arc (along the arc) and of a polyline.
    addLine({0, 0}, {40, 30});
    add(core::Arc{{100, 0}, 20, 0, core::kPi / 2});
    add(polyline({{160, 0}, {180, 15}, {200, 15}, {224, 25}}));
    setAids(modes({&core::SnapModes::extension, &core::SnapModes::parallel}), false, 1.0);
    const auto firstPoint = [&](const Vec2& held, const Vec2& pointer, const char* typed) {
        runner().submitText("LINE");
        hover(held);
        runner().pointerMoved(pointer);
        EXPECT_EQ(state().acquisitions.size(), 1U);
        runner().submitText(typed);
        runner().submitText("@0,-10");
        runner().submitText("");
        return lastLine().start;
    };
    expectNear(firstPoint({40, 30}, {45, 34}, "10"), {48, 36});
    expectNear(firstPoint({0, 0}, {-5, -3.6}, "10"), {-8, -6});
    const double beyondEnd = core::kPi / 2 + 0.2;  // 10 degrees and more past (100,20)
    expectNear(firstPoint({100, 20},
                          {100 + 20 * std::cos(beyondEnd), 20 * std::sin(beyondEnd) + 0.1}, "10"),
               {90.41148922791593, 17.55165123780745});
    expectNear(firstPoint({120, 0}, {100 + 20 * std::cos(-0.2), 20 * std::sin(-0.2)}, "10"),
               {117.5516512378074, -9.588510772084063});
    expectNear(firstPoint({224, 25}, {230, 27.6}, "13"), {236, 30});
    expectNear(firstPoint({160, 0}, {156, -3.1}, "10"), {152, -6});
}

TEST_F(ExtensionParallelTest, TheTwelveLinesOfTheAutoCadDrawingCrossingsAndRelease) {
    // Lines 9D-9F: two extensions crossing, an arc extension crossing a line
    // extension, and the release of one of two extensions.
    addLine({300, 0}, {320, 10});
    addLine({360, 0}, {340, 10});
    add(core::Arc{{100, -60}, 20, 0, core::kPi / 2});
    addLine({60, -60}, {70, -60});
    setAids(modes({&core::SnapModes::extension, &core::SnapModes::parallel}), false, 1.0);

    runner().submitText("LINE");
    hover({320, 10});
    hover({340, 10});
    runner().click({330.3, 14.8}, PickAction::Add);
    runner().submitText("@0,-10");
    runner().submitText("");
    expectNear(lastLine().start, {330, 15});

    runner().submitText("LINE");
    hover({100, -40});
    hover({70, -60});
    runner().click({80.3, -59.8}, PickAction::Add);
    runner().submitText("@0,-10");
    runner().submitText("");
    expectNear(lastLine().start, {80, -60});

    runner().submitText("LINE");
    hover({320, 10});
    hover({340, 10});
    hover({320, 10});  // released
    ASSERT_EQ(state().acquisitions.size(), 1U);
    EXPECT_EQ(state().acquisitions[0].extension.value_or(Extension{}).end, (Vec2{340, 10}));
    runner().pointerMoved({330, 15.2});
    runner().submitText("15");
    runner().submitText("@0,-10");
    runner().submitText("");
    expectNear(lastLine().start, {326.5835921350013, 16.70820393249937});
}

TEST_F(ExtensionParallelTest, TheTwelveLinesOfTheAutoCadDrawingParallels) {
    // Lines A0-A2: parallel to a line and to a polyline segment, and an
    // extension crossing the vertical guide of a point of the tracking.
    addLine({500, 0}, {530, 10});
    add(polyline({{560, 0}, {580, 0}, {600, 15}, {620, 15}}));
    addLine({410, -15}, {430, 15});
    addLine({450, -20}, {450, -40});
    setAids(modes({&core::SnapModes::extension, &core::SnapModes::parallel}), false, 1.0);

    runner().submitText("LINE");
    runner().submitText("500,30");
    hover({515, 5});
    ASSERT_EQ(state().acquisitions.size(), 1U);
    ASSERT_TRUE(state().acquisitions[0].parallel);
    runner().pointerMoved({510, 33.5});
    EXPECT_EQ(state().guide, Guide::Parallel);
    runner().submitText("20");
    runner().submitText("");
    expectNear(lastLine().end, {518.9736659610103, 36.32455532033676});

    runner().submitText("LINE");
    runner().submitText("570,30");
    hover({590, 7.5});
    ASSERT_EQ(state().acquisitions.size(), 1U);
    ASSERT_TRUE(state().acquisitions[0].parallel);
    EXPECT_EQ(state().acquisitions[0].parallel.value_or(Parallel{}).segment, 1U);
    runner().pointerMoved({578, 36.1});
    runner().submitText("25");
    runner().submitText("");
    expectNear(lastLine().end, {590, 45});

    // OSMODE 12289: Endpoint too, with tracking on.
    setModes(modes({&core::SnapModes::endpoint, &core::SnapModes::extension,
                    &core::SnapModes::parallel}),
             true);
    runner().submitText("LINE");
    hover({430, 15});
    hover({450, -20});
    ASSERT_EQ(state().acquired, (Points{{430, 15}, {450, -20}}));
    runner().click({450.2, 44.7}, PickAction::Add);
    runner().submitText("@0,-10");
    runner().submitText("");
    expectNear(lastLine().start, {450, 45});
}

TEST_F(ExtensionParallelTest, ACommonEndAcquiresThePointAndTheExtensionOfTheHighestId) {
    // Revision PT5, [decisione] 4: the snap that wins, ties to the highest id.
    addLine({0, 0}, {10, 0});
    const core::EntityId upward = addLine({0, 0}, {0, 10});
    setAids(modes({&core::SnapModes::endpoint, &core::SnapModes::extension}), true, 1.0);
    runner().submitText("LINE");
    hover({0, 0});
    ASSERT_EQ(state().acquisitions.size(), 1U);
    const Acquisition& acquisition = state().acquisitions[0];
    EXPECT_EQ(acquisition.point, (Vec2{0, 0}));
    ASSERT_TRUE(acquisition.extension);
    const Extension extension = acquisition.extension.value_or(Extension{});
    EXPECT_EQ(extension.entity, upward);
    EXPECT_TRUE(extension.atStart);
    EXPECT_EQ(extension.direction, (Vec2{0, -10}));  // away from (0,10)
    hover({0, 0});                                   // the second hold releases both
    EXPECT_TRUE(state().acquisitions.empty());
}

TEST_F(ExtensionParallelTest, ParallelTakesTheDirectionOfTheHighestId) {
    addLine({-10, -10}, {10, 10});
    addLine({-10, 10}, {10, -10});
    setAids(modes({&core::SnapModes::parallel}), false, 1.0);
    runner().submitText("LINE");
    runner().submitText("0,30");
    hover({0, 0});
    ASSERT_EQ(state().acquisitions.size(), 1U);
    ASSERT_TRUE(state().acquisitions[0].parallel);
    const Parallel parallel = state().acquisitions[0].parallel.value_or(Parallel{});
    EXPECT_EQ(parallel.start, (Vec2{-10, 10}));
    EXPECT_EQ(parallel.end, (Vec2{10, -10}));
}

TEST_F(ExtensionParallelTest, ParallelNeedsABasePointAndAStraightSegmentAwayFromItsEnds) {
    addLine({-10, 0}, {10, 0});
    add(core::Arc{{0, 50}, 10, 0, core::kPi});
    setAids(modes({&core::SnapModes::parallel}), false, 1.0);
    runner().submitText("LINE");
    hover({0, 0});  // no base point at the first point
    EXPECT_TRUE(state().acquisitions.empty());
    runner().submitText("0,30");
    hover({9.5, 0});  // within the aperture of an end
    EXPECT_TRUE(state().acquisitions.empty());
    hover({0, 60});  // an arc
    EXPECT_TRUE(state().acquisitions.empty());
    hover({0, 0});
    EXPECT_EQ(state().acquisitions.size(), 1U);
}

TEST_F(ExtensionParallelTest, CrossingsOfTwoOlderGuidesGoToTheLaterOtherGuide) {
    // Revision PT5, [decisione] 5: extensions at x=-1 (first) and x=1
    // (second), then the horizontal y=0 (last), all within the aperture.
    addLine({-1, -30}, {-1, -20});
    addLine({1, -30}, {1, -20});
    addLine({20, -10}, {20, 10});
    setAids(modes({&core::SnapModes::midpoint, &core::SnapModes::extension}), false, 1.5);
    runner().submitText("LINE");
    hover({-1, -20});
    hover({1, -20});
    setModes(modes({&core::SnapModes::midpoint, &core::SnapModes::extension}), true);
    hover({20, 0});
    ASSERT_EQ(state().acquisitions.size(), 3U);
    EXPECT_EQ(tracked({0, 0.25}), (Vec2{1, 0}));
    EXPECT_EQ(state().guide, Guide::Crossing);
    ASSERT_EQ(state().guides.size(), 2U);
    EXPECT_EQ(state().guides[0].kind, Kind::Horizontal);
    EXPECT_EQ(state().guides[1].kind, Kind::Extension);
    EXPECT_EQ(state().guides[1].origin, (Vec2{1, -20}));
}

TEST_F(ExtensionParallelTest, AnArcExtensionCrossingALineTwiceGivesTheFirstAlongIt) {
    // An arc from 0 to 90 degrees acquired at (0,10): beyond it the circle
    // turns on through (-10,0) to (10,0), both on the horizontal y=0.
    add(core::Arc{{0, 0}, 10, 0, core::kPi / 2});
    addLine({40, -50}, {40, 50});
    setAids(modes({&core::SnapModes::midpoint, &core::SnapModes::extension}), true, 10.0);
    runner().submitText("LINE");
    hover({0, 10});
    hover({40, 0});
    ASSERT_EQ(state().acquisitions.size(), 2U);
    EXPECT_FALSE(state().acquisitions[0].point);  // Endpoint is off
    const std::optional<Vec2> point = tracked({0, -0.5});
    ASSERT_TRUE(point);
    expectNear(point.value_or(Vec2{}), {-10, 0});
    EXPECT_EQ(state().guide, Guide::Crossing);
}

TEST_F(ExtensionParallelTest, ADistanceAlongAnArcStopsAtItsOtherEnd) {
    // Revision PT5, [decisione] 5: ARC at 0,0, radius 10, from 0 to 90
    // degrees, acquired at (0,10).
    add(core::Arc{{0, 0}, 10, 0, core::kPi / 2});
    setAids(modes({&core::SnapModes::extension}), false, 1.0);
    const Vec2 base{20, 20};
    const Vec2 pointer{10 * std::cos(3.0), 10 * std::sin(3.0)};
    const auto typed = [&](const std::string& distance) {
        runner().submitText("LINE");
        runner().submitText("20,20");
        hover({0, 10});
        runner().pointerMoved(pointer);
        const std::optional<Vec2> onGuide = state().guidePoint;
        EXPECT_EQ(state().guide, Guide::Extension);
        runner().submitText(distance);
        runner().submitText("");
        return std::pair{lastLine().end, onGuide.value_or(Vec2{})};
    };
    // 50 is beyond the rest of the circle: a direct distance towards the
    // pointer on the guide, as without guides.
    const auto [direct, onGuide] = typed("50");
    const Vec2 towards = onGuide - base;
    expectNear(direct, base + (towards * (50 / std::hypot(towards.x, towards.y))));
    // The distance that reaches the other end exactly stays on the guide.
    const double rest = (core::kTwoPi - (core::kPi / 2)) * 10;
    std::ostringstream text;
    text << std::setprecision(17) << rest;
    expectNear(typed(text.str()).first, {10, 0});
    // A negative distance is a direct distance with its sign; 0 gives the
    // end (last: its line ends there).
    const auto [negative, guide] = typed("-5");
    const Vec2 away = guide - base;
    expectNear(negative, base + (away * (-5 / std::hypot(away.x, away.y))));
    expectNear(typed("0").first, {0, 10});
}

TEST_F(ExtensionParallelTest, ANegativeDistanceAlongALineExtensionIsDirect) {
    addLine({-10, 0}, {0, 0});
    setAids(modes({&core::SnapModes::extension}), false, 1.0);
    runner().submitText("LINE");
    runner().submitText("0,-20");
    hover({0, 0});
    runner().pointerMoved({5, 0.3});
    runner().submitText("-5");
    // From the last point, (0,-20), away from the pointer on the guide (5,0).
    const Vec2 towards = Vec2{5, 0} - Vec2{0, -20};
    expectNear(lastLine().end, Vec2{0, -20} + (towards * (-5 / std::hypot(towards.x, towards.y))));
    hover({0, 0});
    runner().pointerMoved({5, 0.3});
    runner().submitText("0");
    expectNear(lastLine().end, {0, 0});
}

TEST_F(ExtensionParallelTest, ANearerMidpointWinsOverAnExtensionEnd) {
    // Revision PT5, [decisione] 4: the midpoint (0.5,0) wins, so nothing of
    // the end (0,0) is acquired, with Endpoint on or off.
    addLine({-10, 0}, {0, 0});
    addLine({0.5, -5}, {0.5, 5});
    for (const bool endpoint : {true, false}) {
        core::SnapModes snapModes =
            modes({&core::SnapModes::midpoint, &core::SnapModes::extension});
        snapModes.endpoint = endpoint;
        setAids(snapModes, true, 0.75);
        runner().submitText("LINE");
        hover({0.5, 0});
        ASSERT_EQ(state().acquisitions.size(), 1U) << endpoint;
        EXPECT_EQ(state().acquisitions[0].point, (Vec2{0.5, 0}));
        EXPECT_FALSE(state().acquisitions[0].extension);
        runner().submit(CancelInput{});
    }
}

TEST_F(ExtensionParallelTest, WithoutEndpointTheHoldAcquiresOnlyTheExtension) {
    // #992, second round, [bloccante] 1: as in the dry run of the card.
    addLine({0, -50}, {30, -50});
    setAids(modes({&core::SnapModes::extension, &core::SnapModes::parallel}), true, 0.5);
    runner().submitText("LINE");
    runner().pointerMoved({30, -50});
    EXPECT_EQ(runner().activeSnap(), std::nullopt);  // not for the snap
    hover({30, -50});
    ASSERT_EQ(state().acquisitions.size(), 1U);
    EXPECT_FALSE(state().acquisitions[0].point);
    ASSERT_TRUE(state().acquisitions[0].extension);
    EXPECT_EQ(state().acquisitions[0].extension.value_or(Extension{}).end, (Vec2{30, -50}));
    runner().submit(CancelInput{});

    setAids(modes({&core::SnapModes::endpoint, &core::SnapModes::extension}), true, 0.5);
    runner().submitText("LINE");
    hover({30, -50});
    ASSERT_EQ(state().acquisitions.size(), 1U);
    EXPECT_EQ(state().acquisitions[0].point, (Vec2{30, -50}));
    EXPECT_TRUE(state().acquisitions[0].extension);
}

TEST_F(ExtensionParallelTest, AnEndThatWinsAcquiresNoDirection) {
    // #992, second round, [decisione] 2.
    addLine({-10, 0}, {0, 0});
    addLine({0, -5}, {0, 5});
    setAids(modes({&core::SnapModes::endpoint, &core::SnapModes::extension,
                   &core::SnapModes::parallel}),
            true, 0.75);
    runner().submitText("LINE");
    runner().submitText("0,5");
    hover({0, 0});
    ASSERT_EQ(state().acquisitions.size(), 1U);
    EXPECT_EQ(state().acquisitions[0].point, (Vec2{0, 0}));
    EXPECT_TRUE(state().acquisitions[0].extension);
    EXPECT_FALSE(state().acquisitions[0].parallel);
}

TEST_F(ExtensionParallelTest, OnATieTheExtensionOfTheLatestAcquisitionWins) {
    // #992, second round, [decisione] 1: (-1,-4) along y=2x-2, then (1,4)
    // along y=2x+2.
    addLine({-2, -6}, {-1, -4});
    addLine({2, 6}, {1, 4});
    setAids(modes({&core::SnapModes::endpoint, &core::SnapModes::extension}), true, 1.1);
    runner().submitText("LINE");
    hover({-1, -4});
    hover({1, 4});
    ASSERT_EQ(state().acquisitions.size(), 2U);
    const std::optional<Vec2> point = tracked({0, 0});
    ASSERT_TRUE(point);
    expectNear(point.value_or(Vec2{}), {-1, 0});
}

TEST_F(ExtensionParallelTest, OnATieTheHorizontalOfTheOtherAcquisitionWins) {
    // #992, [decisione] 1: the point (-1,1), then an extension along y=x.
    addLine({-1, -5}, {-1, 7});
    addLine({-5, -5}, {-1, -1});
    setAids(modes({&core::SnapModes::midpoint, &core::SnapModes::extension}), true, 1.3);
    runner().submitText("LINE");
    hover({-1, 1});
    hover({-1, -1});
    ASSERT_EQ(state().acquisitions.size(), 2U);
    EXPECT_EQ(state().acquired, (Points{{-1, 1}}));
    const std::optional<Vec2> point = tracked({0.2, -0.2});
    ASSERT_TRUE(point);
    expectNear(point.value_or(Vec2{}), {1, 1});
}

TEST_F(ExtensionParallelTest, ASelfCrossingPolylineGivesTheDirectionOfItsFirstSegment) {
    // #992, [decisione] 3.
    add(polyline({{-2, -2}, {2, 2}, {-2, 2}, {2, -2}}));
    setAids(modes({&core::SnapModes::parallel}), false, 0.25);
    runner().submitText("LINE");
    runner().submitText("0,5");
    hover({0, 0});
    ASSERT_EQ(state().acquisitions.size(), 1U);
    ASSERT_TRUE(state().acquisitions[0].parallel);
    EXPECT_EQ(state().acquisitions[0].parallel.value_or(Parallel{}).segment, 0U);
    const std::optional<Vec2> point = tracked({3, 8.1});
    ASSERT_TRUE(point);
    expectNear(point.value_or(Vec2{}), {3.05, 8.05});  // on y = 5 + x
    EXPECT_EQ(state().guide, Guide::Parallel);
    EXPECT_EQ(state().from, (Vec2{0, 5}));
}

TEST_F(ExtensionParallelTest, ADistanceAlongAParallelStartsFromTheBasePoint) {
    // [bloccante] 8: in COPY the base point stays while the last point moves.
    addLine({0, 0}, {3, 4});
    addLine({100, -100}, {110, -100});
    setAids(modes({&core::SnapModes::parallel}), false, 1.0);
    runner().submitText("COPY");
    runner().click({100.5, -100}, PickAction::Add);
    runner().submitText("");
    runner().submitText("0,0");
    runner().submitText("1000,0");  // the last point is now (1000,0)
    hover({1.5, 2});
    ASSERT_EQ(state().acquisitions.size(), 1U);
    runner().pointerMoved({30, 40.2});
    EXPECT_EQ(state().guide, Guide::Parallel);
    EXPECT_EQ(state().from, (Vec2{0, 0}));
    runner().submitText("10");  // (6,8) from the base point
    const Line& copied = lastLine();
    expectNear(copied.start, {106, -92});
}

TEST_F(ExtensionParallelTest, AnExtensionStaysWithTrackingOffUntilReleased) {
    // [bloccante] 9.
    addLine({-10, 0}, {0, 0});
    setAids(modes({&core::SnapModes::endpoint, &core::SnapModes::extension}), true, 1.0);
    runner().submitText("LINE");
    hover({0, 0});
    setModes(modes({&core::SnapModes::endpoint, &core::SnapModes::extension}), false);
    ASSERT_EQ(state().acquisitions.size(), 1U);
    EXPECT_FALSE(state().acquisitions[0].point);
    EXPECT_TRUE(state().acquisitions[0].extension);
    EXPECT_EQ(tracked({5, 0.5}), (Vec2{5, 0}));
    ASSERT_EQ(state().guides.size(), 1U);
    EXPECT_EQ(state().guides[0].kind, Kind::Extension);
    EXPECT_EQ(state().guides[0].origin, (Vec2{0, 0}));
    EXPECT_EQ(state().guides[0].direction, (Vec2{10, 0}));
    hover({0, 0});  // released
    EXPECT_TRUE(state().acquisitions.empty());
    EXPECT_EQ(tracked({5, 0.5}), std::nullopt);
    EXPECT_TRUE(state().guides.empty());
}

TEST_F(ExtensionParallelTest, SwitchingAModeOffDropsItsPartForGood) {
    // #992, [decisione] 5.
    addLine({-10, 0}, {0, 0});
    const core::SnapModes both = modes({&core::SnapModes::endpoint, &core::SnapModes::extension});
    setAids(both, true, 1.0);
    runner().submitText("LINE");
    hover({0, 0});
    setModes(modes({&core::SnapModes::endpoint}), true);
    setModes(both, true);
    ASSERT_EQ(state().acquisitions.size(), 1U);
    EXPECT_EQ(state().acquisitions[0].point, (Vec2{0, 0}));
    EXPECT_FALSE(state().acquisitions[0].extension);
    runner().submit(CancelInput{});

    runner().submitText("LINE");
    hover({0, 0});
    setModes(both, false);
    setModes(both, true);
    ASSERT_EQ(state().acquisitions.size(), 1U);
    EXPECT_FALSE(state().acquisitions[0].point);
    EXPECT_TRUE(state().acquisitions[0].extension);
    runner().submit(CancelInput{});

    // Seven acquisitions, two of them only extensions: switching Extension
    // off frees their places, and a new hold drops none of the others.
    std::vector<Vec2> middles;
    for (int index = 0; index < 6; ++index) {
        const double x = 30.0 * index;
        addLine({x, 100}, {x, 120});
        middles.emplace_back(x, 110);
    }
    const core::SnapModes withMidpoint =
        modes({&core::SnapModes::midpoint, &core::SnapModes::extension});
    setAids(withMidpoint, false, 1.0);
    runner().submitText("LINE");
    hover({0, 0});
    hover({-10, 0});
    setModes(withMidpoint, true);
    for (std::size_t index = 0; index < 5; ++index) {
        hover(middles[index]);
    }
    ASSERT_EQ(state().acquisitions.size(), CommandRunner::kMaxAcquired);
    setModes(modes({&core::SnapModes::midpoint}), true);
    EXPECT_EQ(state().acquisitions.size(), 5U);
    hover(middles[5]);
    EXPECT_EQ(state().acquired, middles);
}

TEST_F(ExtensionParallelTest, TheEighthAcquisitionDropsTheOldest) {
    std::vector<Vec2> ends;
    for (int index = 0; index < 8; ++index) {
        const double x = 30.0 * index;
        addLine({x, 0}, {x, 20});
        ends.emplace_back(x, 20);
    }
    setAids(modes({&core::SnapModes::extension}), false, 1.0);
    runner().submitText("LINE");
    for (const Vec2& end : ends) {
        hover(end);
    }
    ASSERT_EQ(state().acquisitions.size(), CommandRunner::kMaxAcquired);
    EXPECT_EQ(state().acquisitions.front().extension.value_or(Extension{}).end, ends[1]);
    EXPECT_EQ(state().acquisitions.back().extension.value_or(Extension{}).end, ends[7]);
}

}  // namespace
}  // namespace tg::cmd
