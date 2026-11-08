// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "action_registry.hpp"
#include "cad_view.hpp"
#include "command_line.hpp"
#include "dynamic_input_box.hpp"
#include "input_events.hpp"
#include "main_window.hpp"
#include "theme.hpp"
#include "tg/core/line.hpp"

#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QToolButton>
#include <gtest/gtest.h>

#include <string>
#include <variant>

namespace tg::app {
namespace {

class DynamicInputTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_directory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("TecnigrafoP16Test"));
        QCoreApplication::setApplicationName(QStringLiteral("DynamicInput"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
        m_window.show();
        m_window.activateWindow();
        ASSERT_TRUE(QTest::qWaitForWindowActive(&m_window));
        view()->setView({0, 0}, 1.0);
    }

    void TearDown() override {
        m_window.close();
        QSettings::setDefaultFormat(m_format);
        QCoreApplication::setOrganizationName(m_organization);
        QCoreApplication::setApplicationName(m_application);
        applyTheme(*qobject_cast<QApplication*>(QApplication::instance()), Theme::Light);
    }

    [[nodiscard]] MainWindow& window() { return m_window; }
    [[nodiscard]] CadView* view() const { return m_window.view(); }
    [[nodiscard]] DynamicInputBox* box() const { return view()->dynamicInputBox(); }
    [[nodiscard]] QLineEdit* input() const {
        return m_window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    }
    [[nodiscard]] QLineEdit* field(int index) const {
        return box()->findChild<QLineEdit*>(QStringLiteral("dynamicField%1").arg(index));
    }
    [[nodiscard]] QAction* toggle() const {
        return m_window.actionRegistry().action(QStringLiteral("aids.dynmode"));
    }
    [[nodiscard]] QString history() const {
        return m_window.commandLine()->findChild<QPlainTextEdit*>(QStringLiteral("history"))
            ->toPlainText();
    }
    void enter(const std::string& text) const {
        events::type(input(), text);
        events::press(input(), Qt::Key_Return);
    }
    void firstPoint() const {
        events::type(view(), "10");
        events::press(view(), Qt::Key_Tab);
        events::type(view(), "10");
        events::press(view(), Qt::Key_Return);
    }
    void moveTo(core::Vec2 point) const {
        const auto screen = view()->viewport().worldToScreen(point);
        events::move(view(), QPoint(qRound(screen.x), qRound(screen.y)));
    }

private:
    QTemporaryDir m_directory;
    QString m_organization{QCoreApplication::organizationName()};
    QString m_application{QCoreApplication::applicationName()};
    QSettings::Format m_format{QSettings::defaultFormat()};
    MainWindow m_window{MainWindow::InitialDrawing::Empty};
};

TEST_F(DynamicInputTest, AbsoluteThenPolarMatchesCommandLineAndEchoesBothLines) {
    EXPECT_TRUE(toggle()->isChecked());
    EXPECT_FALSE(box()->isVisible());
    enter("LINE");
    ASSERT_TRUE(box()->isVisible());
    firstPoint();
    events::type(view(), "50");
    events::press(view(), Qt::Key_Tab);
    events::type(view(), "30");
    events::press(view(), Qt::Key_Return);
    events::press(view(), Qt::Key_Return);
    EXPECT_FALSE(window().runner().isActive());
    EXPECT_FALSE(box()->isVisible());
    ASSERT_EQ(window().document().entityCount(), 1U);
    const auto& line = std::get<core::Line>(window().document().entities().back().entity);

    MainWindow reference(MainWindow::InitialDrawing::Empty);
    reference.setScriptMode();
    auto* referenceInput = reference.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    for (const auto* text : {"LINE", "10,10", "@50<30", ""}) {
        events::type(referenceInput, text);
        events::press(referenceInput, Qt::Key_Return);
    }
    ASSERT_EQ(reference.document().entityCount(), 1U);
    const auto& expected = std::get<core::Line>(reference.document().entities().back().entity);
    EXPECT_EQ(line.start, expected.start);
    EXPECT_EQ(line.end, expected.end);
    EXPECT_TRUE(history().contains(QStringLiteral("10,10")));
    EXPECT_TRUE(history().contains(QStringLiteral("@50<30")));
}

TEST_F(DynamicInputTest, EmptyEnterEndsLineAndClearsTheFieldsAfterEachPoint) {
    enter("LINE");
    firstPoint();
    EXPECT_EQ(field(0)->accessibleName(), QStringLiteral("Length"));
    EXPECT_EQ(field(1)->accessibleName(), QStringLiteral("Angle"));
    events::press(view(), Qt::Key_Return);
    EXPECT_FALSE(window().runner().isActive());
    EXPECT_TRUE(window().document().isEmpty());
    EXPECT_FALSE(box()->isVisible());
}

TEST_F(DynamicInputTest, LetterGoesToCommandLineAndLeavesLockedFieldsAlone) {
    enter("LINE");
    firstPoint();
    events::type(view(), "50");
    events::type(view(), "C");
    EXPECT_EQ(input()->text(), QStringLiteral("C"));
    EXPECT_EQ(field(0)->text(), QStringLiteral("50"));
    EXPECT_TRUE(box()->isVisible());
    events::type(view(), "2");
    EXPECT_EQ(input()->text(), QStringLiteral("C2"));
    events::press(view(), Qt::Key_Escape);
    EXPECT_TRUE(input()->text().isEmpty());
    EXPECT_FALSE(box()->isVisible());
    EXPECT_TRUE(history().contains(QStringLiteral("*Cancel*")));
}

TEST_F(DynamicInputTest, F12AndStatusButtonToggleAndDisabledKeysUseCommandLine) {
    enter("LINE");
    auto* button = window().findChild<QToolButton*>(QStringLiteral("dynmodeButton"));
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->focusPolicy(), Qt::NoFocus);
    view()->setFocus();
    QTest::keyPress(view(), Qt::Key_F12);
    EXPECT_FALSE(toggle()->isChecked());
    EXPECT_FALSE(button->isChecked());
    EXPECT_FALSE(box()->isVisible());
    EXPECT_TRUE(history().endsWith(QStringLiteral("<Dynamic input off>")));
    events::type(view(), "10,10");
    EXPECT_EQ(input()->text(), QStringLiteral("10,10"));
    events::press(view(), Qt::Key_Return);
    button->click();
    EXPECT_TRUE(toggle()->isChecked());
    EXPECT_TRUE(box()->isVisible());
    EXPECT_TRUE(history().endsWith(QStringLiteral("<Dynamic input on>")));
}

TEST_F(DynamicInputTest, FreeFieldTracksOrthoWhileTypedFieldStaysLockedAndTabCycles) {
    enter("LINE");
    firstPoint();
    window().actionRegistry().action(QStringLiteral("aids.ortho"))->setChecked(true);
    events::type(view(), "50");
    moveTo({60, 15});
    EXPECT_EQ(field(0)->text(), QStringLiteral("50"));
    EXPECT_EQ(field(1)->text(), QStringLiteral("0"));
    events::press(view(), Qt::Key_Tab);
    events::press(view(), Qt::Key_Tab);
    events::press(view(), Qt::Key_Return);
    ASSERT_EQ(window().document().entityCount(), 1U);
    const auto& line = std::get<core::Line>(window().document().entities().back().entity);
    EXPECT_EQ(line.end, (core::Vec2{60, 10}));
    // The fields reset after Enter: typing starts in Length again.
    events::type(view(), "25");
    EXPECT_EQ(field(0)->text(), QStringLiteral("25"));
}

TEST_F(DynamicInputTest, FreeCoordinateUsesSnapAndCommaUsesRelativeCartesianText) {
    enter("LINE");
    enter("10,10");
    enter("20,20");
    enter("");
    enter("LINE");
    moveTo({12, 13});  // within the endpoint pickbox of (10, 10)
    events::type(view(), "30");
    events::press(view(), Qt::Key_Return);
    EXPECT_TRUE(history().contains(QStringLiteral("30,10")));
    events::type(view(), "5,-2");
    events::press(view(), Qt::Key_Return);
    ASSERT_EQ(window().document().entityCount(), 2U);
    const auto& line = std::get<core::Line>(window().document().entities().back().entity);
    EXPECT_EQ(line.start, (core::Vec2{30, 10}));
    EXPECT_EQ(line.end, (core::Vec2{35, 8}));
    EXPECT_TRUE(history().contains(QStringLiteral("@5,-2")));
}

TEST_F(DynamicInputTest, OverlayStaysInsideViewAndUsesCurrentThemeAccent) {
    enter("LINE");
    events::move(view(), view()->rect().bottomRight());
    EXPECT_TRUE(view()->rect().contains(box()->geometry()));
    window().resize(800, 600);
    QApplication::processEvents();
    EXPECT_TRUE(view()->rect().contains(box()->geometry()));
    window().setTheme(Theme::Dark);
    events::move(view(), {1, 1});
    EXPECT_TRUE(field(0)->styleSheet().contains(themeTokens(Theme::Dark).accent.name()));
    events::press(view(), Qt::Key_Tab);
    EXPECT_TRUE(field(1)->styleSheet().contains(themeTokens(Theme::Dark).accent.name()));
    EXPECT_FALSE(field(0)->styleSheet().contains(themeTokens(Theme::Dark).accent.name()));
}

TEST_F(DynamicInputTest, PreferencePersistsAndScriptModeSuppressesOverlayAndHistory) {
    toggle()->setChecked(false);
    EXPECT_FALSE(QSettings{}.value(QStringLiteral("ui/dynmode"), true).toBool());
    MainWindow restored(MainWindow::InitialDrawing::Empty);
    restored.show();
    EXPECT_FALSE(restored.actionRegistry().action(QStringLiteral("aids.dynmode"))->isChecked());
    EXPECT_TRUE(restored.close());
    toggle()->setChecked(true);
    const QString before = history();
    window().setScriptMode();
    EXPECT_FALSE(toggle()->isChecked());
    EXPECT_FALSE(toggle()->isEnabled());
    EXPECT_EQ(history(), before);
    enter("LINE");
    EXPECT_FALSE(box()->isVisible());
    EXPECT_TRUE(QSettings{}.value(QStringLiteral("ui/dynmode")).toBool());
}

}  // namespace
}  // namespace tg::app
