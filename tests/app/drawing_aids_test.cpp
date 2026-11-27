// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "cad_view.hpp"
#include "command_line.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "settings_dialog.hpp"
#include "theme.hpp"

#include <QAction>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPoint>
#include <QSettings>
#include <QStatusBar>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
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
        ASSERT_TRUE(m_directory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("TecnigrafoPT33Test"));
        QCoreApplication::setApplicationName(QStringLiteral("DrawingAids"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
        m_window.show();
        m_window.activateWindow();
        ASSERT_TRUE(QTest::qWaitForWindowActive(&m_window));
        constexpr std::array kNames{
            "aids.osnap",
            "aids.ortho",
            "aids.snap.endpoint",
            "aids.snap.midpoint",
            "aids.snap.center",
            "aids.polar",
            "aids.otrack",
            "aids.snap.intersection",
            "aids.snap.perpendicular",
            "aids.snap.tangent",
            "aids.snap.quadrant",
            "aids.grid",
            "aids.snapgrid",
            "aids.snap.nearest",
            "aids.snap.extension",
            "aids.snap.parallel",
        };
        for (const char* name : kNames) {
            ASSERT_NE(action(name), nullptr) << name;
        }
        m_window.setFileDialog([this](cmd::FilePrompt /*kind*/, const QString& /*path*/) {
            return m_fileAnswers.isEmpty() ? QString{} : m_fileAnswers.takeFirst();
        });
        ASSERT_NE(button("gridButton"), nullptr);
        ASSERT_NE(button("snapgridButton"), nullptr);
        ASSERT_NE(button("osnapButton"), nullptr);
        ASSERT_NE(button("orthoButton"), nullptr);
        ASSERT_NE(button("polarButton"), nullptr);
        ASSERT_NE(button("otrackButton"), nullptr);
        ASSERT_NE(input(), nullptr);
    }

    void TearDown() override {
        QSettings::setDefaultFormat(m_format);
        QCoreApplication::setOrganizationName(m_organization);
        QCoreApplication::setApplicationName(m_application);
    }

    [[nodiscard]] CadView* view() const { return m_window.view(); }

    [[nodiscard]] MainWindow& window() { return m_window; }

    [[nodiscard]] const core::Document& document() const { return m_window.document(); }

    [[nodiscard]] const cmd::DrawingAids& aids() const { return m_window.drawingAids(); }

    void setSnapModes(const core::SnapModes& modes) { m_window.setSnapModes(modes); }

    void setPolar(bool on) { m_window.setPolar(on); }

    [[nodiscard]] const cmd::CommandRunner& runner() const { return m_window.runner(); }

    [[nodiscard]] cmd::CommandRunner& mutableRunner() { return m_window.activeSession()->runner(); }

    [[nodiscard]] edit::Editor& editor() { return m_window.activeSession()->editor(); }

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

    [[nodiscard]] QString temporaryPath(const QString& name) const {
        return m_directory.filePath(name);
    }

    void answerFileDialog(const QString& path) { m_fileAnswers.append(path); }

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
            const auto entities = document().entities();
            const auto& polyline = std::get<core::Polyline>(entities.back().entity);
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

    // A guide whose screen coordinate ends in .5, as with a view of odd
    // height, is painted on the pixel below the rounded one: callers look
    // one pixel either side (bandAround).
    [[nodiscard]] std::size_t markerPixelsIn(const QRect& area) const {
        const render::Color marker = sceneStyle(m_window.theme()).snapMarker;
        const QImage image = view()->grab().toImage();
        std::size_t count = 0;
        for (int y = area.top(); y <= area.bottom(); ++y) {
            for (int x = area.left(); x <= area.right(); ++x) {
                if (image.pixelColor(x, y) == QColor(marker.r, marker.g, marker.b)) {
                    ++count;
                }
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t orangePixelsIn(const QRect& area) const {
        const QImage image = view()->grab().toImage();
        std::size_t count = 0;
        for (int y = area.top(); y <= area.bottom(); ++y) {
            for (int x = area.left(); x <= area.right(); ++x) {
                const QColor color = image.pixelColor(x, y);
                if (color.red() > color.green() + 20 && color.green() > color.blue() + 20) {
                    ++count;
                }
            }
        }
        return count;
    }

    void setSnapMagnet(bool on) {
        QTimer::singleShot(0, &m_window, [this, on] {
            auto* dialog = m_window.findChild<SettingsDialog*>();
            ASSERT_NE(dialog, nullptr);
            auto* magnet = dialog->findChild<QComboBox*>(QStringLiteral("snapMagnetCombo"));
            ASSERT_NE(magnet, nullptr);
            magnet->setCurrentIndex(on ? 0 : 1);
            dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
        });
        action("tools.settings")->trigger();
        ASSERT_EQ(m_window.snapMagnet(), on);
    }

private:
    QTemporaryDir m_directory;
    QStringList m_fileAnswers;
    QString m_organization{QCoreApplication::organizationName()};
    QString m_application{QCoreApplication::applicationName()};
    QSettings::Format m_format{QSettings::defaultFormat()};
    MainWindow m_window;
};

/// The rows from one above to one below area, or the columns from one left
/// to one right of it: the pixels where a guide on area can be painted.
QRect bandAround(const QRect& area) {
    return area.width() == 1 ? area.adjusted(-1, 0, 1, 0) : area.adjusted(0, -1, 0, 1);
}

TEST_F(DrawingAidsTest, StartsWithObjectSnapOnWithEveryModeAndOrthoOff) {
    EXPECT_EQ(aids(), cmd::DrawingAids{});
    EXPECT_TRUE(action("aids.osnap")->isChecked());
    EXPECT_TRUE(action("aids.snap.endpoint")->isChecked());
    EXPECT_TRUE(action("aids.snap.midpoint")->isChecked());
    EXPECT_TRUE(action("aids.snap.center")->isChecked());
    EXPECT_FALSE(action("aids.ortho")->isChecked());
    EXPECT_TRUE(button("osnapButton")->isChecked());
    EXPECT_FALSE(button("orthoButton")->isChecked());
    EXPECT_FALSE(button("otrackButton")->isChecked());
    EXPECT_EQ(action("aids.osnap")->shortcut(), QKeySequence(Qt::Key_F3));
    EXPECT_EQ(action("aids.ortho")->shortcut(), QKeySequence(Qt::Key_F8));
    EXPECT_EQ(action("aids.otrack")->shortcut(), QKeySequence(Qt::Key_F11));
}

TEST_F(DrawingAidsTest, EverySwitchOfTheStatusBarIsAnIconThatFollowsItsAction) {
    // docs/adr/0039, point 1, area 10: icons without text, with the name and
    // the key in the tooltip.
    const std::array<std::pair<const char*, const char*>, 7> kSwitches{{
        {"gridButton", "aids.grid"},
        {"snapgridButton", "aids.snapgrid"},
        {"orthoButton", "aids.ortho"},
        {"polarButton", "aids.polar"},
        {"otrackButton", "aids.otrack"},
        {"osnapButton", "aids.osnap"},
        {"lwdisplayButton", "aids.lwdisplay"},
    }};
    for (const auto& [name, id] : kSwitches) {
        SCOPED_TRACE(name);
        QToolButton* switchButton = button(name);
        QAction* switchAction = action(id);
        ASSERT_NE(switchButton, nullptr);
        ASSERT_NE(switchAction, nullptr);
        EXPECT_FALSE(switchButton->icon().isNull());
        EXPECT_EQ(switchButton->icon().cacheKey(), switchAction->icon().cacheKey());
        EXPECT_TRUE(switchButton->text().isEmpty());
        EXPECT_EQ(switchButton->toolButtonStyle(), Qt::ToolButtonIconOnly);
        EXPECT_TRUE(
            switchButton->toolTip().startsWith(switchAction->text().remove(QLatin1Char('&'))));
        const QString key = switchAction->shortcut().toString(QKeySequence::NativeText);
        EXPECT_TRUE(switchButton->toolTip().contains(key));
        // Both ways, and back.
        const bool initial = switchAction->isChecked();
        switchAction->trigger();
        EXPECT_NE(switchAction->isChecked(), initial);
        EXPECT_EQ(switchButton->isChecked(), switchAction->isChecked());
        switchButton->click();
        EXPECT_EQ(switchAction->isChecked(), initial);
        EXPECT_EQ(switchButton->isChecked(), initial);
        switchButton->click();
        EXPECT_NE(switchAction->isChecked(), initial);
        switchAction->trigger();
        EXPECT_EQ(switchButton->isChecked(), initial);
    }
    // The ribbon switch is on while the ribbon is shown.
    QToolButton* ribbon = button("ribbonButton");
    ASSERT_NE(ribbon, nullptr);
    EXPECT_FALSE(ribbon->icon().isNull());
    EXPECT_TRUE(ribbon->text().isEmpty());
    EXPECT_EQ(ribbon->isChecked(), window().interfaceKind() == InterfaceKind::Ribbon);
    QToolButton* settings = button("settingsButton");
    ASSERT_NE(settings, nullptr);
    EXPECT_FALSE(settings->icon().isNull());
    EXPECT_EQ(settings->toolButtonStyle(), Qt::ToolButtonIconOnly);
}

TEST_F(DrawingAidsTest, TheStatusBarHasTheOrderOfTheReference) {
    // docs/adr/0039, point 1, area 10: the coordinates on the left, then the
    // place of the environment indicator, the switches, the ribbon, the
    // scale and the settings, from left to right.
    QStatusBar* bar = window().statusBar();
    const auto left = [bar](QWidget* widget) {
        EXPECT_NE(widget, nullptr);
        return widget == nullptr ? -1 : widget->mapTo(bar, QPoint(0, 0)).x();
    };
    const QList<QWidget*> order{
        window().findChild<QWidget*>(QStringLiteral("environmentIndicator")),
        button("gridButton"),
        button("snapgridButton"),
        button("orthoButton"),
        button("polarButton"),
        button("otrackButton"),
        button("osnapButton"),
        button("lwdisplayButton"),
        button("ribbonButton"),
        button("settingsButton"),
    };
    for (qsizetype i = 1; i < order.size(); ++i) {
        EXPECT_LT(left(order[i - 1]), left(order[i])) << i;
    }
    const auto labels = bar->findChildren<QLabel*>();
    ASSERT_FALSE(labels.isEmpty());
    // The coordinates are the leftmost text of the bar.
    int leftmost = left(order.front());
    for (QLabel* label : labels) {
        leftmost = std::min(leftmost, left(label));
    }
    EXPECT_TRUE(std::any_of(labels.begin(), labels.end(), [&](QLabel* label) {
        return left(label) == leftmost && label->text().startsWith(QStringLiteral("X:"));
    }));
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

TEST_F(DrawingAidsTest, UcsIconKeepsTheRenderSize) {
    EXPECT_EQ(sceneStyle(window().theme()).ucsIconSizePx, render::SceneStyle{}.ucsIconSizePx);
    EXPECT_EQ(sceneStyle(window().theme()).ucsIconSizePx, 37.0);
}

TEST_F(DrawingAidsTest, GridSwitchesFollowEachDocumentWithoutUndoSteps) {
    DocumentSession& first = *window().activeSession();
    DocumentSession& second = window().openSession();
    first.editor().markSaved();
    second.editor().markSaved();

    EXPECT_TRUE(action("aids.grid")->isChecked());
    EXPECT_FALSE(action("aids.snapgrid")->isChecked());
    action("aids.grid")->trigger();
    EXPECT_FALSE(first.document().grid().gridOn);
    EXPECT_TRUE(first.editor().isModified());
    EXPECT_TRUE(first.editor().undoNames().empty());
    EXPECT_FALSE(second.editor().isModified());
    EXPECT_EQ(lastHistoryLine(), "<Grid off>");

    window().activate(second);
    EXPECT_TRUE(action("aids.grid")->isChecked());
    action("aids.snapgrid")->trigger();
    EXPECT_TRUE(second.document().grid().snapOn);
    EXPECT_TRUE(second.editor().isModified());
    EXPECT_TRUE(second.editor().undoNames().empty());
    EXPECT_TRUE(button("gridButton")->isChecked());
    EXPECT_TRUE(button("snapgridButton")->isChecked());

    window().activate(first);
    EXPECT_FALSE(action("aids.grid")->isChecked());
    EXPECT_FALSE(action("aids.snapgrid")->isChecked());
    EXPECT_EQ(action("aids.grid")->shortcut(), QKeySequence(Qt::Key_F7));
    EXPECT_EQ(action("aids.snapgrid")->shortcut(), QKeySequence(Qt::Key_F9));

    const QString path = temporaryPath(QStringLiteral("grid-state.cadx"));
    answerFileDialog(path);
    action("file.save")->trigger();
    ASSERT_TRUE(QFileInfo::exists(path));
    window().closeSession(first);
    answerFileDialog(path);
    action("file.open")->trigger();
    EXPECT_FALSE(window().document().grid().gridOn);
    EXPECT_FALSE(action("aids.grid")->isChecked());
    EXPECT_FALSE(button("gridButton")->isChecked());
}

TEST_F(DrawingAidsTest, NearestExtensionAndParallelSwitchesReachTheRunner) {
    for (const char* id : {"aids.snap.nearest", "aids.snap.extension", "aids.snap.parallel"}) {
        EXPECT_FALSE(action(id)->isChecked());
        action(id)->trigger();
    }
    EXPECT_TRUE(aids().snapModes.nearest);
    EXPECT_TRUE(aids().snapModes.extension);
    EXPECT_TRUE(aids().snapModes.parallel);
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
        window().commandLine()->findChild<QPlainTextEdit*>(QStringLiteral("history"));
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

TEST_F(DrawingAidsTest, F11ChangesTrackingAndItsStatusButton) {
    EXPECT_FALSE(aids().tracking);
    shortcut(view(), Qt::Key_F11);
    EXPECT_TRUE(aids().tracking);
    EXPECT_TRUE(action("aids.otrack")->isChecked());
    EXPECT_TRUE(button("otrackButton")->isChecked());
    EXPECT_EQ(lastHistoryLine(), "<Object Snap Tracking on>");
    button("otrackButton")->click();
    EXPECT_FALSE(aids().tracking);
    EXPECT_EQ(lastHistoryLine(), "<Object Snap Tracking off>");
}

TEST_F(DrawingAidsTest, AStationaryPointerAcquiresAndPaintsTrackingGuides) {
    view()->setView({70.0, 30.0}, 1.0);
    shortcut(view(), Qt::Key_F11);
    enter("LINE");
    moveScreen(screen(kVertex));
    auto* holdTimer = view()->findChild<QTimer*>(QStringLiteral("trackingHoldTimer"));
    ASSERT_NE(holdTimer, nullptr);
    EXPECT_TRUE(holdTimer->isActive());
    EXPECT_TRUE(holdTimer->isSingleShot());
    EXPECT_EQ(holdTimer->interval(), 500);
    holdTimer->stop();
    ASSERT_TRUE(QMetaObject::invokeMethod(holdTimer, "timeout", Qt::DirectConnection));
    ASSERT_EQ(runner().tracking().acquired, (std::vector<core::Vec2>{kVertex}));
    moveScreen(screen({90.0, 2.0}));
    ASSERT_EQ(runner().tracking().guide, cmd::CommandRunner::TrackingState::Guide::Horizontal);
    const int guideY = screen({0.0, kVertex.y}).toPoint().y();
    EXPECT_GT(markerPixelsIn(bandAround(QRect{10, guideY, 200, 1})), 0U);
}

TEST_F(DrawingAidsTest, PolarCrossingPaintsTheTrackingGuideThatProducedIt) {
    view()->setView({70.0, 30.0}, 1.0);
    shortcut(view(), Qt::Key_F11);
    shortcut(view(), Qt::Key_F10);
    enter("LINE");
    enter("80,20");
    moveScreen(screen(kVertex));
    auto* holdTimer = view()->findChild<QTimer*>(QStringLiteral("trackingHoldTimer"));
    ASSERT_NE(holdTimer, nullptr);
    holdTimer->stop();
    ASSERT_TRUE(QMetaObject::invokeMethod(holdTimer, "timeout", Qt::DirectConnection));
    ASSERT_EQ(runner().tracking().acquired, (std::vector<core::Vec2>{kVertex}));
    moveScreen(screen({82.0, 2.0}));
    ASSERT_EQ(runner().tracking().guide, cmd::CommandRunner::TrackingState::Guide::WithPolar);
    const std::optional<core::Vec2> guidePointWorld = runner().tracking().guidePoint;
    if (!guidePointWorld) {
        ADD_FAILURE() << "Expected a polar crossing point";
        return;
    }

    const QPoint guidePoint = screen(*guidePointWorld).toPoint();
    const int baseY = screen({80.0, 20.0}).toPoint().y();
    EXPECT_GT(markerPixelsIn(bandAround(QRect{10, guidePoint.y(), 200, 1})), 0U);
    EXPECT_EQ(markerPixelsIn(bandAround(QRect{guidePoint.x(), 10, 1, baseY - 20})), 0U);
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
    if (!polar) {
        ADD_FAILURE() << "Expected an active polar ray";
        return;
    }
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

TEST_F(DrawingAidsTest, PanningKeepsTheClosedHandCursorWhileThePointerMoves) {
    const QPointF start = view()->rect().center();
    QTest::mousePress(view(), Qt::MiddleButton, {}, start.toPoint());
    EXPECT_EQ(view()->cursor().shape(), Qt::ClosedHandCursor);
    QTest::mouseMove(view(), (start + QPointF{20.0, 10.0}).toPoint());
    EXPECT_EQ(view()->cursor().shape(), Qt::ClosedHandCursor);
    QTest::mouseRelease(view(), Qt::MiddleButton, {}, (start + QPointF{20.0, 10.0}).toPoint());
}

TEST_F(DrawingAidsTest, ArcAndStraightExtensionGuidesArePaintedUntilReleased) {
    view()->setView({90.0, -50.0}, 0.2);
    editor().beginTransaction("Test geometry");
    [[maybe_unused]] const core::EntityId arcId =
        editor().add(core::Arc{{100.0, -60.0}, 20.0, 0.0, core::kPi / 2.0});
    [[maybe_unused]] const core::EntityId lineId =
        editor().add(core::Line{{60.0, -60.0}, {70.0, -60.0}});
    editor().commit();
    view()->update();
    setSnapModes(core::SnapModes{false, false, false, false, false, false, false, false, false,
                                 true, false});
    ASSERT_FALSE(aids().tracking);
    enter("LINE");
    mutableRunner().pointerHeld({100.0, -40.0});
    mutableRunner().pointerHeld({70.0, -60.0});
    mutableRunner().pointerMoved({80.3, -59.8});
    view()->update();
    QCoreApplication::processEvents();

    const auto& guides = runner().tracking().guides;
    ASSERT_EQ(guides.size(), 2U);
    const auto arc = std::find_if(guides.begin(), guides.end(),
                                  [](const auto& guide) { return guide.arc.has_value(); });
    const auto straight = std::find_if(guides.begin(), guides.end(),
                                       [](const auto& guide) { return !guide.arc.has_value(); });
    ASSERT_NE(arc, guides.end());
    ASSERT_NE(straight, guides.end());
    if (!arc->arc.has_value()) {
        FAIL() << "arc guide has no arc geometry";
    }
    const core::ArcGeometry& arcGeometry = arc->arc.value();
    const double middleAngle = arcGeometry.startAngle + arcGeometry.sweep * 0.5;
    const core::Vec2 arcPoint =
        arcGeometry.center +
        core::Vec2{std::cos(middleAngle), std::sin(middleAngle)} * arcGeometry.radius;
    const QPoint arcPixel = screen(arcPoint).toPoint();
    const QPoint straightPixel = screen(straight->origin + straight->direction * 0.5).toPoint();
    const QRect arcArea{arcPixel - QPoint{12, 12}, QSize{25, 25}};
    const QRect straightArea{straightPixel - QPoint{12, 12}, QSize{25, 25}};
    EXPECT_GT(orangePixelsIn(arcArea), 0U);
    EXPECT_GT(markerPixelsIn(straightArea), 0U);

    mutableRunner().pointerHeld({100.0, -40.0});
    mutableRunner().pointerHeld({70.0, -60.0});
    mutableRunner().pointerMoved({80.3, -59.8});
    view()->update();
    QCoreApplication::processEvents();
    EXPECT_TRUE(runner().tracking().guides.empty());
    EXPECT_EQ(orangePixelsIn(arcArea), 0U);
    EXPECT_EQ(markerPixelsIn(straightArea), 0U);
}

TEST_F(DrawingAidsTest, CurrentVertexCircleFollowsEditsUndoAndSelection) {
    view()->setView({70.0, 30.0}, 1.0);
    const core::EntityId outline = document().entities().front().id;
    ASSERT_TRUE(mutableRunner().replaceSelection({outline}));
    view()->setCurrentVertex(outline, 1);
    const auto hasCircleAt = [&](const core::Vec2& point) {
        const QPoint center = screen(point).toPoint();
        const QImage image = view()->grab().toImage();
        std::size_t count = 0;
        for (int y = center.y() - 9; y <= center.y() + 9; ++y) {
            for (int x = center.x() - 9; x <= center.x() + 9; ++x) {
                const int squared =
                    (x - center.x()) * (x - center.x()) + (y - center.y()) * (y - center.y());
                const QColor color = image.pixelColor(x, y);
                if (squared >= 36 && squared <= 100 && color.blue() > color.green() + 20 &&
                    color.green() > color.red() + 20) {
                    ++count;
                }
            }
        }
        return count > 12;
    };
    EXPECT_TRUE(hasCircleAt(kVertex));

    const auto& old = std::get<core::Polyline>(document().find(outline)->entity);
    std::vector<core::PolylineVertex> vertices(old.vertices().begin(), old.vertices().end());
    vertices.at(1).position = {100.0, 0.0};
    core::Polyline changed(std::move(vertices), old.isClosed());
    editor().beginTransaction("Properties");
    ASSERT_TRUE(editor().replace(outline, std::move(changed)));
    editor().commit();
    view()->update();
    EXPECT_FALSE(hasCircleAt(kVertex));
    EXPECT_TRUE(hasCircleAt({100.0, 0.0}));

    enter("U");
    // U clears the selection (docs/adr/0008), and with it the circle.
    EXPECT_TRUE(runner().selection().isEmpty());
    EXPECT_FALSE(hasCircleAt(kVertex));
    ASSERT_TRUE(
        mutableRunner().replaceSelection(std::vector<core::EntityId>{document().entities()[1].id}));
    view()->update();
    QCoreApplication::processEvents();
    EXPECT_FALSE(hasCircleAt(kVertex));
}

TEST_F(DrawingAidsTest, HatchSourcesAreHighlightedOnlyWhileTheCommandRuns) {
    view()->setView({70.0, 30.0}, 1.0);
    enter("-HATCH");
    click({30.0, 22.0});
    // The dynamic input box follows the pointer: keep it off the bottom side.
    moveScreen(QPointF{5.0, 5.0});
    ASSERT_FALSE(runner().highlighted().empty());
    const QPoint bottom = screen({60.0, 0.0}).toPoint();
    const auto highlightedPixels = [&] {
        const QImage image = view()->grab().toImage();
        std::size_t count = 0;
        for (int y = bottom.y() - 2; y <= bottom.y() + 2; ++y) {
            for (int x = bottom.x() - 10; x <= bottom.x() + 10; ++x) {
                const QColor color = image.pixelColor(x, y);
                if (color.blue() > color.green() + 20 && color.green() > color.red() + 20) {
                    ++count;
                }
            }
        }
        return count;
    };
    EXPECT_GT(highlightedPixels(), 0U);

    events::press(input(), Qt::Key_Return);
    EXPECT_TRUE(runner().highlighted().empty());
    EXPECT_EQ(highlightedPixels(), 0U);
}

TEST_F(DrawingAidsTest, SnapMagnetMovesOnlyTheDrawnCursorToTheSnap) {
    view()->setView({70.0, 30.0}, 1.0);
    setSnapMagnet(false);
    enter("LINE");
    const QPointF nearVertex = screen(kVertex) + QPointF{3.0, 4.0};
    moveScreen(nearVertex);
    ASSERT_TRUE(runner().activeSnap());
    EXPECT_NE(view()->cursor().shape(), Qt::BlankCursor);
    // The dynamic input box can cover the lower arm; sample the left arm instead.
    const QPoint arm = screen(kVertex).toPoint() + QPoint{-20, 0};
    const QColor cursor = window().theme() == Theme::Dark ? Qt::white : Qt::black;
    EXPECT_NE(view()->grab().toImage().pixelColor(arm), cursor);

    events::press(input(), Qt::Key_Escape);
    setSnapMagnet(true);
    enter("LINE");
    moveScreen(nearVertex);
    ASSERT_TRUE(runner().activeSnap());
    EXPECT_EQ(view()->cursor().shape(), Qt::BlankCursor);
    EXPECT_EQ(view()->grab().toImage().pixelColor(arm), cursor);
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
