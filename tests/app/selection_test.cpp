// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "cad_view.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"

#include <QAction>
#include <QColor>
#include <QCoreApplication>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QTest>
#include <QWheelEvent>
#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <string>

namespace tg::app {
namespace {

// Points on the sample drawing's outline, slot and lower-left hole.
constexpr core::Vec2 kOutline{60.0, 0.0};
constexpr core::Vec2 kSlot{60.0, 32.0};
constexpr core::Vec2 kHole{11.0, 15.0};

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

class SelectionTest : public testing::Test {
protected:
    void SetUp() override {
        m_window.show();
        m_window.activateWindow();
        ASSERT_TRUE(QTest::qWaitForWindowActive(&m_window));
        ASSERT_NE(eraseAction(), nullptr);
        ASSERT_NE(input(), nullptr);
    }

    [[nodiscard]] MainWindow& window() { return m_window; }

    [[nodiscard]] CadView* view() const { return m_window.view(); }

    [[nodiscard]] const core::Document& document() const { return m_window.document(); }

    [[nodiscard]] const cmd::SelectionSet& selection() const { return m_window.selection(); }

    [[nodiscard]] QAction* eraseAction() const {
        return m_window.findChild<QAction*>(QStringLiteral("edit.erase"));
    }

    [[nodiscard]] QLineEdit* input() const {
        return m_window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    }

    [[nodiscard]] std::string prompt() const {
        return m_window.commandLine()
            ->findChild<QLabel*>(QStringLiteral("prompt"))
            ->text()
            .toStdString();
    }

    [[nodiscard]] QPointF screen(const core::Vec2& world) const {
        const core::Vec2 pixel = view()->viewport().worldToScreen(world);
        return {pixel.x, pixel.y};
    }

    void clickScreen(const QPointF& local, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        const QPointF global = view()->mapToGlobal(local);
        QMouseEvent down(QEvent::MouseButtonPress, local, global, Qt::LeftButton, Qt::LeftButton,
                         modifiers);
        QCoreApplication::sendEvent(view(), &down);
        QMouseEvent up(QEvent::MouseButtonRelease, local, global, Qt::LeftButton, Qt::NoButton,
                       modifiers);
        QCoreApplication::sendEvent(view(), &up);
    }

    void click(const core::Vec2& world, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        clickScreen(screen(world), modifiers);
    }

    void dragScreen(const QPointF& fromLocal, const QPointF& toLocal,
                    Qt::KeyboardModifiers pressModifiers = Qt::NoModifier,
                    Qt::KeyboardModifiers releaseModifiers = Qt::NoModifier) {
        const QPointF fromGlobal = view()->mapToGlobal(fromLocal);
        QMouseEvent down(QEvent::MouseButtonPress, fromLocal, fromGlobal, Qt::LeftButton,
                         Qt::LeftButton, pressModifiers);
        QCoreApplication::sendEvent(view(), &down);

        const QPointF toGlobal = view()->mapToGlobal(toLocal);
        QMouseEvent move(QEvent::MouseMove, toLocal, toGlobal, Qt::LeftButton, Qt::LeftButton,
                         releaseModifiers);
        QCoreApplication::sendEvent(view(), &move);

        QMouseEvent up(QEvent::MouseButtonRelease, toLocal, toGlobal, Qt::LeftButton, Qt::NoButton,
                       releaseModifiers);
        QCoreApplication::sendEvent(view(), &up);
    }

    void drag(const core::Vec2& fromWorld, const core::Vec2& toWorld,
              Qt::KeyboardModifiers releaseModifiers = Qt::NoModifier) {
        dragScreen(screen(fromWorld), screen(toWorld), Qt::NoModifier, releaseModifiers);
    }

    void move(const core::Vec2& world) { events::move(view(), screen(world).toPoint()); }

    void enter(const std::string& text) {
        events::type(input(), text);
        events::press(input(), Qt::Key_Return);
    }

    static void escape(QWidget* widget) { events::press(widget, Qt::Key_Escape); }

    static void shortcut(QWidget* widget, Qt::Key key,
                         Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        widget->setFocus();
        QTest::keyPress(widget, key, modifiers);
    }

    void wheel(int delta) {
        const QPointF local = screen(kOutline);
        QWheelEvent event(local, view()->mapToGlobal(local), {}, {0, delta}, Qt::NoButton,
                          Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(view(), &event);
    }

    [[nodiscard]] QColor colorAt(const QImage& image, const core::Vec2& world) const {
        const QPointF pixel = screen(world) * image.devicePixelRatio();
        return image.pixelColor(static_cast<int>(std::lround(pixel.x())),
                                static_cast<int>(std::lround(pixel.y())));
    }

    [[nodiscard]] static std::size_t highlightPixels(const QImage& image) {
        std::size_t count = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QColor color = image.pixelColor(x, y);
                // Include antialiased highlight pixels, but not the grey
                // grid, axes or entity colour.
                if (color.blue() > color.red() + 30 && color.blue() > color.green() + 20) {
                    ++count;
                }
            }
        }
        return count;
    }

private:
    MainWindow m_window;
};

TEST_F(SelectionTest, ClickAddsAndShiftClickRemovesTheSampleEntity) {
    const core::EntityId id = document().entities().front().id;
    EXPECT_TRUE(selection().isEmpty());
    EXPECT_FALSE(eraseAction()->isEnabled());
    click(kOutline);
    EXPECT_EQ(selection().size(), 1U);
    EXPECT_TRUE(selection().contains(id));
    EXPECT_TRUE(eraseAction()->isEnabled());
    click(kOutline, Qt::ShiftModifier);
    EXPECT_TRUE(selection().isEmpty());
    EXPECT_FALSE(eraseAction()->isEnabled());
    EXPECT_EQ(document().entityCount(), 10U);
}

TEST_F(SelectionTest, WindowIncludesOnlyWholeEntitiesAndCrossingIncludesTouchedEntities) {
    const core::EntityId hole = document().entities()[2].id;
    click({8.0, 8.0});
    click({22.0, 22.0});
    ASSERT_EQ(selection().size(), 1U);
    EXPECT_TRUE(selection().contains(hole));
    escape(view());

    // The narrow rectangle cuts the hole. Left to right excludes it.
    click({18.0, 8.0});
    click({22.0, 22.0});
    EXPECT_TRUE(selection().isEmpty());
    // The same rectangle, right to left, includes it.
    click({22.0, 8.0});
    click({18.0, 22.0});
    ASSERT_EQ(selection().size(), 1U);
    EXPECT_TRUE(selection().contains(hole));
    click({22.0, 8.0});
    click({18.0, 22.0}, Qt::ShiftModifier);
    EXPECT_TRUE(selection().isEmpty());
}

TEST_F(SelectionTest, HighlightsTheSelectionAndClearsTheHighlightOnEscape) {
    const std::size_t before = highlightPixels(view()->grab().toImage());
    click(kOutline);
    EXPECT_GT(highlightPixels(view()->grab().toImage()), before);
    escape(view());
    EXPECT_EQ(highlightPixels(view()->grab().toImage()), before);
}

TEST_F(SelectionTest, RectangleRepaintsAtRestAndUsesWindowAndCrossingColours) {
    PaintCounter paints;
    view()->installEventFilter(&paints);
    click({8.0, 8.0});
    QCoreApplication::processEvents();
    const int before = paints.count();
    move({22.0, 22.0});
    QCoreApplication::processEvents();
    EXPECT_GT(paints.count(), before);
    EXPECT_EQ(prompt(), "Specify opposite corner:");
    const QColor windowColor = colorAt(view()->grab().toImage(), {15.0, 13.0});
    EXPECT_GT(windowColor.blue(), windowColor.green());
    escape(view());
    click({22.0, 8.0});
    move({8.0, 22.0});
    const QColor crossingColor = colorAt(view()->grab().toImage(), {15.0, 13.0});
    EXPECT_GT(crossingColor.green(), crossingColor.blue());
}

TEST_F(SelectionTest, DeleteFromViewAndEmptyCommandLineErasesAndUndoRestoresTheSameRecord) {
    const core::EntityRecord original = document().entities().front();
    for (QWidget* target : {static_cast<QWidget*>(view()), static_cast<QWidget*>(input())}) {
        click(kOutline);
        shortcut(target, Qt::Key_Delete);
        EXPECT_EQ(document().entityCount(), 9U);
        EXPECT_EQ(document().find(original.id), nullptr);
        EXPECT_TRUE(selection().isEmpty());
        EXPECT_FALSE(eraseAction()->isEnabled());
        EXPECT_EQ(prompt(), "Command:");
        shortcut(input(), Qt::Key_Z, Qt::ControlModifier);
        ASSERT_NE(document().find(original.id), nullptr);
        EXPECT_EQ(*document().find(original.id), original);
        EXPECT_EQ(document().entityCount(), 10U);
    }
}

TEST_F(SelectionTest, RedoDeletesTheEntitiesRestoredByUndo) {
    const core::EntityId outline = document().entities().front().id;
    const core::EntityId slot = document().entities()[1].id;
    click(kOutline);
    click(kSlot);
    shortcut(view(), Qt::Key_Delete);
    EXPECT_EQ(document().entityCount(), 8U);
    shortcut(input(), Qt::Key_Z, Qt::ControlModifier);
    EXPECT_NE(document().find(outline), nullptr);
    EXPECT_NE(document().find(slot), nullptr);
    shortcut(input(), Qt::Key_Y, Qt::ControlModifier);
    EXPECT_EQ(document().entityCount(), 8U);
    EXPECT_EQ(document().find(outline), nullptr);
    EXPECT_EQ(document().find(slot), nullptr);
}

TEST_F(SelectionTest, EraseMenuAndTypedCommandConsumePreselection) {
    click(kOutline);
    eraseAction()->trigger();
    EXPECT_EQ(document().entityCount(), 9U);
    click(kSlot);
    enter("ERASE");
    EXPECT_EQ(document().entityCount(), 8U);
    EXPECT_EQ(prompt(), "Command:");
    EXPECT_FALSE(eraseAction()->isEnabled());
}

TEST_F(SelectionTest, EFromViewAsksForSelectionAndEnterErasesIt) {
    const core::EntityId hole = document().entities()[2].id;
    events::type(view(), "E");
    events::press(view(), Qt::Key_Return);
    EXPECT_EQ(prompt(), "Select objects:");
    EXPECT_FALSE(eraseAction()->isEnabled());
    click(kHole);
    EXPECT_TRUE(selection().contains(hole));
    EXPECT_FALSE(eraseAction()->isEnabled());
    EXPECT_EQ(document().entityCount(), 10U);
    events::press(view(), Qt::Key_Return);
    EXPECT_EQ(document().entityCount(), 9U);
    EXPECT_EQ(document().find(hole), nullptr);
    EXPECT_EQ(prompt(), "Command:");
    EXPECT_TRUE(selection().isEmpty());
}

TEST_F(SelectionTest, EscapeFromViewAndInputClearsSelectionAndFirstClosesAnOpenRectangle) {
    for (QWidget* target : {static_cast<QWidget*>(view()), static_cast<QWidget*>(input())}) {
        click(kOutline);
        click({8.0, 8.0});
        EXPECT_EQ(prompt(), "Specify opposite corner:");
        escape(target);
        EXPECT_EQ(prompt(), "Command:");
        EXPECT_EQ(selection().size(), 1U);
        escape(target);
        EXPECT_TRUE(selection().isEmpty());
        EXPECT_FALSE(eraseAction()->isEnabled());
    }
}

TEST_F(SelectionTest, DeleteEditsNonemptyInputWithoutErasingTheDrawing) {
    click(kOutline);
    input()->setText(QStringLiteral("abc"));
    input()->setCursorPosition(1);
    shortcut(input(), Qt::Key_Delete);
    EXPECT_EQ(input()->text().toStdString(), "ac");
    EXPECT_EQ(document().entityCount(), 10U);
    EXPECT_EQ(selection().size(), 1U);
    EXPECT_TRUE(eraseAction()->isEnabled());
    input()->selectAll();
    shortcut(input(), Qt::Key_Delete);
    EXPECT_TRUE(input()->text().isEmpty());
    EXPECT_EQ(document().entityCount(), 10U);
    shortcut(input(), Qt::Key_Delete);
    EXPECT_EQ(document().entityCount(), 9U);
}

TEST_F(SelectionTest, DeleteDoesNothingWithoutSelectionOrDuringAnActiveCommand) {
    shortcut(input(), Qt::Key_Delete);
    EXPECT_EQ(prompt(), "Command:");
    EXPECT_EQ(document().entityCount(), 10U);
    enter("E");
    click(kOutline);
    shortcut(input(), Qt::Key_Delete);
    EXPECT_EQ(prompt(), "Select objects:");
    EXPECT_EQ(document().entityCount(), 10U);
    escape(input());
    EXPECT_TRUE(selection().isEmpty());
    EXPECT_FALSE(eraseAction()->isEnabled());
    click(kOutline);
    enter("PL");
    EXPECT_TRUE(selection().isEmpty());
    EXPECT_FALSE(eraseAction()->isEnabled());
    shortcut(input(), Qt::Key_Delete);
    EXPECT_EQ(prompt(), "Specify start point:");
    escape(input());
}

TEST_F(SelectionTest, PickingKeepsAThreePixelToleranceAfterInitialFitWheelAndZoomExtents) {
    for (const int delta : {0, -240, 360, 0}) {
        if (delta != 0) {
            wheel(delta);
        }
        clickScreen(screen(kOutline) + QPointF{0.0, 2.0});
        EXPECT_EQ(selection().size(), 1U);
        escape(view());
        clickScreen(screen(kOutline) + QPointF{0.0, 4.0});
        EXPECT_TRUE(selection().isEmpty());
        EXPECT_EQ(prompt(), "Specify opposite corner:");
        escape(view());
        view()->zoomExtents();
    }
}

TEST_F(SelectionTest, DragToRightSelectsByWindowAndClosesRectangleOnRelease) {
    const core::EntityId hole = document().entities()[2].id;
    // Drag left-to-right enclosing the hole.
    drag({8.0, 8.0}, {22.0, 22.0});
    EXPECT_EQ(prompt(), "Command:");
    ASSERT_EQ(selection().size(), 1U);
    EXPECT_TRUE(selection().contains(hole));
    escape(view());

    // Drag left-to-right cutting the hole: window excludes it.
    drag({18.0, 8.0}, {22.0, 22.0});
    EXPECT_EQ(prompt(), "Command:");
    EXPECT_TRUE(selection().isEmpty());
}

TEST_F(SelectionTest, DragToLeftSelectsByCrossingAndClosesRectangleOnRelease) {
    const core::EntityId hole = document().entities()[2].id;
    // Drag right-to-left cutting the hole: crossing includes it.
    drag({22.0, 8.0}, {18.0, 22.0});
    EXPECT_EQ(prompt(), "Command:");
    ASSERT_EQ(selection().size(), 1U);
    EXPECT_TRUE(selection().contains(hole));
}

TEST_F(SelectionTest, DragReleaseWithShiftRemovesFromSelection) {
    const core::EntityId hole = document().entities()[2].id;
    click(kHole);
    ASSERT_TRUE(selection().contains(hole));
    // Drag crossing over the hole with Shift removes it on release.
    drag({22.0, 8.0}, {18.0, 22.0}, Qt::ShiftModifier);
    EXPECT_EQ(prompt(), "Command:");
    EXPECT_TRUE(selection().isEmpty());
}

TEST_F(SelectionTest, DragUnderFivePixelsLeavesRectangleOpen) {
    const QPointF start = screen({8.0, 8.0});
    // Drag 3 pixels to the right, under the 5 pixel threshold.
    dragScreen(start, start + QPointF{3.0, 0.0});
    EXPECT_EQ(prompt(), "Specify opposite corner:");
    // Second stationary click closes it.
    click({22.0, 22.0});
    EXPECT_EQ(prompt(), "Command:");
    EXPECT_EQ(selection().size(), 1U);
}

TEST_F(SelectionTest, DragDuringPointPromptHasNoEffectOnRelease) {
    const std::size_t before = document().entityCount();
    enter("LINE");
    EXPECT_EQ(prompt(), "Specify first point:");
    // Drag press specifies the first point; release does not specify a second point.
    drag({10.0, 10.0}, {50.0, 50.0});
    EXPECT_EQ(prompt(), "Specify next point or [Undo]:");
    // Enter without a second point finishes LINE without creating entities.
    events::press(view(), Qt::Key_Return);
    EXPECT_EQ(prompt(), "Command:");
    EXPECT_EQ(document().entityCount(), before);
}

}  // namespace
}  // namespace tg::app
