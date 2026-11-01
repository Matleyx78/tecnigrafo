// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "cad_view.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "theme.hpp"

#include <QAction>
#include <QColor>
#include <QCoreApplication>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QTest>
#include <QToolButton>
#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace tg::app {
namespace {

// A vertex of the sample drawing: the end of the outline's bottom side.
constexpr core::Vec2 kVertex{110.0, 0.0};

// Inside the outline, far (in pixels) from every entity of the sample
// drawing: clicks here never snap.
constexpr core::Vec2 kFirst{30.0, 22.0};
constexpr core::Vec2 kNearlyHorizontal{50.0, 27.0};
constexpr core::Vec2 kNearlyVertical{35.0, 62.0};
constexpr core::Vec2 kFar{90.0, 20.0};

/// Counts the paint events of the widget it filters.
class PaintCounter final : public QObject {
public:
    [[nodiscard]] int count() const { return m_count; }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (event->type() == QEvent::Paint) {
            ++m_count;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    int m_count{0};
};

class DrawingAidsTest : public testing::Test {
protected:
    void SetUp() override {
        m_window.show();
        m_window.activateWindow();
        ASSERT_TRUE(QTest::qWaitForWindowActive(&m_window));
        for (const char* name : {"aids.osnap", "aids.ortho", "aids.snap.endpoint",
                                 "aids.snap.midpoint", "aids.snap.center", "aids.polar",
                                 "aids.snap.intersection", "aids.snap.perpendicular",
                                 "aids.snap.tangent", "aids.snap.quadrant"}) {
            ASSERT_NE(action(name), nullptr) << name;
        }
        ASSERT_NE(button("osnapButton"), nullptr);
        ASSERT_NE(button("orthoButton"), nullptr);
        ASSERT_NE(button("polarButton"), nullptr);
        ASSERT_NE(input(), nullptr);
    }

    [[nodiscard]] CadView* view() const { return m_window.view(); }

    [[nodiscard]] const core::Document& document() const { return m_window.document(); }

    [[nodiscard]] const cmd::DrawingAids& aids() const { return m_window.drawingAids(); }

    void setSnapModes(const core::SnapModes& modes) { m_window.setSnapModes(modes); }

    void setPolar(bool on) { m_window.setPolar(on); }

    [[nodiscard]] const cmd::CommandRunner& runner() const { return m_window.runner(); }

    [[nodiscard]] QAction* action(const char* name) const {
        return m_window.findChild<QAction*>(QString::fromLatin1(name));
    }

    [[nodiscard]] QToolButton* button(const char* name) const {
        return m_window.findChild<QToolButton*>(QString::fromLatin1(name));
    }

    [[nodiscard]] QLineEdit* input() const {
        return m_window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    }

    /// The last line of the command line's history.
    [[nodiscard]] std::string lastHistoryLine() const {
        const QString text = m_window.commandLine()
                                 ->findChild<QPlainTextEdit*>(QStringLiteral("history"))
                                 ->toPlainText();
        return text.section(QLatin1Char('\n'), -1).toStdString();
    }

    [[nodiscard]] QPointF screen(const core::Vec2& world) const {
        const core::Vec2 pixel = view()->viewport().worldToScreen(world);
        return {pixel.x, pixel.y};
    }

    void clickScreen(const QPointF& local) {
        const QPointF global = view()->mapToGlobal(local);
        QMouseEvent down(QEvent::MouseButtonPress, local, global, Qt::LeftButton, Qt::LeftButton,
                         Qt::NoModifier);
        QCoreApplication::sendEvent(view(), &down);
        QMouseEvent up(QEvent::MouseButtonRelease, local, global, Qt::LeftButton, Qt::NoButton,
                       Qt::NoModifier);
        QCoreApplication::sendEvent(view(), &up);
    }

    /// The pointer moves to the point, then clicks there, as with a mouse.
    void moveAndClickScreen(const QPointF& local) {
        const QPointF global = view()->mapToGlobal(local);
        QMouseEvent move(QEvent::MouseMove, local, global, Qt::NoButton, Qt::NoButton,
                         Qt::NoModifier);
        QCoreApplication::sendEvent(view(), &move);
        clickScreen(local);
    }

    void click(const core::Vec2& world) { moveAndClickScreen(screen(world)); }

    void moveScreen(const QPointF& local) {
        QMouseEvent move(QEvent::MouseMove, local, view()->mapToGlobal(local), Qt::NoButton,
                         Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(view(), &move);
    }

    void enter(const std::string& text) {
        events::type(input(), text);
        events::press(input(), Qt::Key_Return);
    }

    static void shortcut(QWidget* widget, Qt::Key key) {
        widget->setFocus();
        QTest::keyPress(widget, key);
    }

    /// PL, a click at `first`, a click at `second`, Enter: the vertices of
    /// the polyline drawn, or none if no polyline was added.
    std::vector<core::Vec2> drawTwoPoints(const QPointF& first, const QPointF& second) {
        const std::size_t before = document().entityCount();
        enter("PL");
        moveAndClickScreen(first);
        moveAndClickScreen(second);
        events::press(input(), Qt::Key_Return);
        std::vector<core::Vec2> vertices;
        if (document().entityCount() == before + 1) {
            const auto& polyline = std::get<core::Polyline>(document().entities().back().entity);
            for (const core::PolylineVertex& vertex : polyline.vertices()) {
                vertices.push_back(vertex.position);
            }
        }
        return vertices;
    }

    /// Count only the snap marker, using the window's current theme.
    [[nodiscard]] std::size_t markerPixels() const {
        const render::Color marker = sceneStyle(m_window.theme()).snapMarker;
        const QImage image = view()->grab().toImage();
        std::size_t count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor color = image.pixelColor(x, y);
                if (color == QColor(marker.r, marker.g, marker.b)) {
                    ++count;
                }
            }
        }
        return count;
    }

private:
    MainWindow m_window;
};

TEST_F(DrawingAidsTest, StartsWithObjectSnapOnWithEveryModeAndOrthoOff) {
    EXPECT_EQ(aids(), cmd::DrawingAids{});
    EXPECT_TRUE(action("aids.osnap")->isChecked());
    EXPECT_TRUE(action("aids.snap.endpoint")->isChecked());
    EXPECT_TRUE(action("aids.snap.midpoint")->isChecked());
    EXPECT_TRUE(action("aids.snap.center")->isChecked());
    EXPECT_FALSE(action("aids.ortho")->isChecked());
    EXPECT_TRUE(button("osnapButton")->isChecked());
    EXPECT_FALSE(button("orthoButton")->isChecked());
    EXPECT_EQ(action("aids.osnap")->shortcut(), QKeySequence(Qt::Key_F3));
    EXPECT_EQ(action("aids.ortho")->shortcut(), QKeySequence(Qt::Key_F8));
    EXPECT_EQ(button("osnapButton")->text().toStdString(), "OSNAP");
    EXPECT_EQ(button("orthoButton")->text().toStdString(), "ORTHO");
}

TEST_F(DrawingAidsTest, NewSnapModesHaveTheAdrDefaultsAndReachTheRunner) {
    EXPECT_TRUE(action("aids.snap.intersection")->isChecked());
    EXPECT_FALSE(action("aids.snap.perpendicular")->isChecked());
    EXPECT_FALSE(action("aids.snap.tangent")->isChecked());
    EXPECT_FALSE(action("aids.snap.quadrant")->isChecked());
    const core::SnapModes defaults = aids().snapModes;
    for (const char* name : {"aids.snap.intersection", "aids.snap.perpendicular",
                             "aids.snap.tangent", "aids.snap.quadrant"}) {
        action(name)->trigger();
    }
    auto wanted = defaults;
    wanted.intersection = false;
    wanted.perpendicular = true;
    wanted.tangent = true;
    wanted.quadrant = true;
    EXPECT_EQ(aids().snapModes, wanted);
    setSnapModes(defaults);
    EXPECT_EQ(aids().snapModes, defaults);
    EXPECT_TRUE(action("aids.snap.intersection")->isChecked());
    EXPECT_FALSE(action("aids.snap.perpendicular")->isChecked());
    EXPECT_FALSE(action("aids.snap.tangent")->isChecked());
    EXPECT_FALSE(action("aids.snap.quadrant")->isChecked());
}

TEST_F(DrawingAidsTest, F10AndF8ExcludeEachOtherAndEchoBothChanges) {
    EXPECT_FALSE(aids().polar);
    EXPECT_FALSE(button("polarButton")->isChecked());
    EXPECT_EQ(action("aids.polar")->shortcut(), QKeySequence(Qt::Key_F10));
    shortcut(view(), Qt::Key_F8);
    shortcut(input(), Qt::Key_F10);
    EXPECT_TRUE(aids().polar);
    EXPECT_FALSE(aids().ortho);
    EXPECT_TRUE(button("polarButton")->isChecked());
    EXPECT_FALSE(button("orthoButton")->isChecked());
    EXPECT_EQ(lastHistoryLine(), "<Polar on>");
    shortcut(view(), Qt::Key_F8);
    EXPECT_FALSE(aids().polar);
    EXPECT_TRUE(aids().ortho);
    EXPECT_FALSE(action("aids.polar")->isChecked());
    const auto* history =
        input()->parentWidget()->findChild<QPlainTextEdit*>(QStringLiteral("history"));
    ASSERT_NE(history, nullptr);
    EXPECT_TRUE(history->toPlainText().contains(QStringLiteral("<Polar off>\n<Ortho on>")));
    shortcut(view(), Qt::Key_F10);
    button("polarButton")->click();
    EXPECT_FALSE(aids().polar);
    EXPECT_FALSE(aids().ortho);
    EXPECT_EQ(lastHistoryLine(), "<Polar off>");
    EXPECT_EQ(button("polarButton")->focusPolicy(), Qt::NoFocus);
}

TEST_F(DrawingAidsTest, PolarDirectiveUpdatesTheButtonWithoutEchoingPolar) {
    const auto before = lastHistoryLine();
    setPolar(true);
    EXPECT_TRUE(aids().polar);
    EXPECT_TRUE(action("aids.polar")->isChecked());
    EXPECT_TRUE(button("polarButton")->isChecked());
    EXPECT_EQ(lastHistoryLine(), before);
    setPolar(false);
    EXPECT_FALSE(aids().polar);
    EXPECT_FALSE(button("polarButton")->isChecked());
    EXPECT_EQ(lastHistoryLine(), before);
}

TEST_F(DrawingAidsTest, PolarOverlayFollowsTheRunnerAndYieldsToObjectSnap) {
    action("aids.osnap")->setChecked(false);
    view()->setView({50.0, 30.0}, 1.0);
    shortcut(view(), Qt::Key_F10);
    EXPECT_FALSE(runner().activePolar());
    enter("LINE");
    EXPECT_FALSE(runner().activePolar());
    enter("30,22");
    moveScreen(screen({70.0, 24.0}));
    const auto polar = runner().activePolar();
    ASSERT_TRUE(polar);
    EXPECT_EQ(polar->base, kFirst);
    EXPECT_EQ(polar->point, (core::Vec2{70.0, 22.0}));
    EXPECT_GT(markerPixels(), 0U);  // dashed ray and its hint
    moveScreen(screen({70.0, 42.0}));
    EXPECT_FALSE(runner().activePolar());
    EXPECT_EQ(markerPixels(), 0U);
    moveScreen(screen({70.0, 24.0}));
    shortcut(view(), Qt::Key_F8);
    EXPECT_FALSE(runner().activePolar());
    EXPECT_EQ(markerPixels(), 0U);
    shortcut(view(), Qt::Key_F10);
    action("aids.osnap")->setChecked(true);
    moveScreen(screen(kVertex));
    ASSERT_TRUE(runner().activeSnap());
    EXPECT_FALSE(runner().activePolar());
    events::press(input(), Qt::Key_Escape);
    EXPECT_FALSE(runner().activePolar());
}

TEST_F(DrawingAidsTest, SelectedAndHotGripsArePaintedAboveTheDrawing) {
    click(kVertex);
    ASSERT_FALSE(runner().grips().empty());
    const auto pixel = screen(kVertex).toPoint();
    const auto style = sceneStyle(Theme::Light);
    EXPECT_EQ(view()->grab().toImage().pixelColor(pixel),
              QColor(style.grip.r, style.grip.g, style.grip.b));
    click(kVertex);
    ASSERT_TRUE(runner().hotGrip());
    EXPECT_EQ(view()->grab().toImage().pixelColor(pixel),
              QColor(style.hotGrip.r, style.hotGrip.g, style.hotGrip.b));
    events::press(input(), Qt::Key_Escape);
    EXPECT_FALSE(runner().hotGrip());
    EXPECT_EQ(view()->grab().toImage().pixelColor(pixel),
              QColor(style.grip.r, style.grip.g, style.grip.b));
}

TEST_F(DrawingAidsTest, OrthoKeepsTheSecondVertexOnTheAxesOfTheFirst) {
    shortcut(view(), Qt::Key_F8);
    ASSERT_TRUE(aids().ortho);

    const std::vector<core::Vec2> horizontal =
        drawTwoPoints(screen(kFirst), screen(kNearlyHorizontal));
    ASSERT_EQ(horizontal.size(), 2U);
    EXPECT_EQ(horizontal[1].y, horizontal[0].y);
    EXPECT_NEAR(horizontal[1].x, kNearlyHorizontal.x, 0.5);

    const std::vector<core::Vec2> vertical = drawTwoPoints(screen(kFirst), screen(kNearlyVertical));
    ASSERT_EQ(vertical.size(), 2U);
    EXPECT_EQ(vertical[1].x, vertical[0].x);
    EXPECT_NEAR(vertical[1].y, kNearlyVertical.y, 0.5);
}

TEST_F(DrawingAidsTest, WithoutOrthoTheSecondVertexFollowsThePointer) {
    const std::vector<core::Vec2> vertices =
        drawTwoPoints(screen(kFirst), screen(kNearlyHorizontal));
    ASSERT_EQ(vertices.size(), 2U);
    EXPECT_GT(std::abs(vertices[1].y - vertices[0].y), 4.0);
}

TEST_F(DrawingAidsTest, AClickFivePixelsFromAVertexSnapsExactlyOnIt) {
    const QPointF nearVertex = screen(kVertex) + QPointF{3.0, 4.0};
    const std::vector<core::Vec2> vertices = drawTwoPoints(nearVertex, screen(kFar));
    ASSERT_EQ(vertices.size(), 2U);
    EXPECT_EQ(vertices[0], kVertex);
}

TEST_F(DrawingAidsTest, F3TurnsTheSnapOffForTheSameClick) {
    shortcut(view(), Qt::Key_F3);
    ASSERT_FALSE(aids().objectSnap);

    const QPointF nearVertex = screen(kVertex) + QPointF{3.0, 4.0};
    const std::vector<core::Vec2> vertices = drawTwoPoints(nearVertex, screen(kFar));
    ASSERT_EQ(vertices.size(), 2U);
    EXPECT_NE(vertices[0], kVertex);
    // Five pixels away from the vertex, on the screen.
    const QPointF startOnScreen = screen(vertices[0]);
    EXPECT_NEAR(startOnScreen.x() - screen(kVertex).x(), 3.0, 0.01);
    EXPECT_NEAR(startOnScreen.y() - screen(kVertex).y(), 4.0, 0.01);
}

TEST_F(DrawingAidsTest, SnapWinsOverOrtho) {
    shortcut(view(), Qt::Key_F8);
    const std::vector<core::Vec2> vertices =
        drawTwoPoints(screen(kFar), screen(kVertex) + QPointF{3.0, 4.0});
    ASSERT_EQ(vertices.size(), 2U);
    EXPECT_EQ(vertices[1], kVertex);
}

TEST_F(DrawingAidsTest, EachModeOfTheMenuReachesTheRunner) {
    // The midpoint of the outline's bottom side.
    const core::Vec2 midpoint{60.0, 0.0};

    action("aids.snap.midpoint")->trigger();
    EXPECT_FALSE(aids().snapModes.midpoint);
    std::vector<core::Vec2> vertices =
        drawTwoPoints(screen(midpoint) + QPointF{3.0, 4.0}, screen(kFar));
    ASSERT_EQ(vertices.size(), 2U);
    EXPECT_NE(vertices[0], midpoint);

    action("aids.snap.midpoint")->trigger();
    EXPECT_TRUE(aids().snapModes.midpoint);
    // Five pixels from the midpoint again, but seven from the start of the
    // polyline just drawn, whose endpoint would otherwise be nearer.
    vertices = drawTwoPoints(screen(midpoint) + QPointF{-4.0, 3.0}, screen(kFar));
    ASSERT_EQ(vertices.size(), 2U);
    EXPECT_EQ(vertices[0], midpoint);

    action("aids.snap.endpoint")->trigger();
    action("aids.snap.center")->trigger();
    EXPECT_EQ(aids().snapModes, (core::SnapModes{false, true, false}));
    EXPECT_TRUE(aids().objectSnap);  // the modes do not touch the switch
}

TEST_F(DrawingAidsTest, SetSnapModesReachesTheSwitchesAndTheRunner) {
    // The directive !osnap keeps every switch in step with the runner.
    const core::SnapModes wanted{false, true, false, true, false, true, false, true};
    setSnapModes(wanted);
    EXPECT_EQ(aids().snapModes, wanted);
    EXPECT_FALSE(action("aids.snap.endpoint")->isChecked());
    EXPECT_TRUE(action("aids.snap.midpoint")->isChecked());
    EXPECT_FALSE(action("aids.snap.center")->isChecked());
    EXPECT_TRUE(action("aids.snap.insertion")->isChecked());
    EXPECT_TRUE(aids().objectSnap);  // the modes do not touch the switch
    EXPECT_EQ(lastHistoryLine().find("Osnap"), std::string::npos);

    // Toggling a switch later keeps all the other modes.
    action("aids.snap.center")->trigger();
    EXPECT_EQ(aids().snapModes,
              (core::SnapModes{false, true, true, true, false, true, false, true}));
}

TEST_F(DrawingAidsTest, ActionsButtonsAndHistoryStayInStep) {
    QToolButton* osnap = button("osnapButton");
    QToolButton* ortho = button("orthoButton");

    shortcut(view(), Qt::Key_F3);
    EXPECT_FALSE(action("aids.osnap")->isChecked());
    EXPECT_FALSE(osnap->isChecked());
    EXPECT_FALSE(aids().objectSnap);
    EXPECT_EQ(lastHistoryLine(), "<Osnap off>");

    osnap->click();
    EXPECT_TRUE(action("aids.osnap")->isChecked());
    EXPECT_TRUE(aids().objectSnap);
    EXPECT_EQ(lastHistoryLine(), "<Osnap on>");

    // F8 works also while the command line has the keyboard.
    shortcut(input(), Qt::Key_F8);
    EXPECT_TRUE(action("aids.ortho")->isChecked());
    EXPECT_TRUE(ortho->isChecked());
    EXPECT_TRUE(aids().ortho);
    EXPECT_EQ(lastHistoryLine(), "<Ortho on>");

    ortho->click();
    EXPECT_FALSE(action("aids.ortho")->isChecked());
    EXPECT_FALSE(aids().ortho);
    EXPECT_EQ(lastHistoryLine(), "<Ortho off>");

    action("aids.ortho")->trigger();
    EXPECT_TRUE(ortho->isChecked());
    EXPECT_EQ(lastHistoryLine(), "<Ortho on>");

    // The buttons never take the keyboard away.
    EXPECT_EQ(osnap->focusPolicy(), Qt::NoFocus);
    EXPECT_EQ(ortho->focusPolicy(), Qt::NoFocus);
}

TEST_F(DrawingAidsTest, TheMarkerShowsWhileAPointIsAskedAndF3HidesIt) {
    const QPointF nearVertex = screen(kVertex) + QPointF{3.0, 4.0};
    moveScreen(nearVertex);
    EXPECT_EQ(markerPixels(), 0U);  // idle: no marker

    enter("PL");
    moveScreen(nearVertex);
    EXPECT_GT(markerPixels(), 0U);

    // The pointer does not move: the switch alone repaints the view.
    PaintCounter paints;
    view()->installEventFilter(&paints);
    QCoreApplication::processEvents();
    const int before = paints.count();
    shortcut(view(), Qt::Key_F3);
    QCoreApplication::processEvents();
    EXPECT_GT(paints.count(), before);
    view()->removeEventFilter(&paints);
    EXPECT_EQ(markerPixels(), 0U);
    shortcut(view(), Qt::Key_F3);
    EXPECT_GT(markerPixels(), 0U);

    moveScreen(screen(kFar));
    EXPECT_EQ(markerPixels(), 0U);
    events::press(input(), Qt::Key_Escape);
}

}  // namespace
}  // namespace tg::app
