// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// One session per document and the contract of activation
// (docs/adr/0041, point 2; docs/tappe/0.7.5/P28).

#include "action_registry.hpp"
#include "cad_view.hpp"
#include "command_line.hpp"
#include "document_session.hpp"
#include "dynamic_input_box.hpp"
#include "input_events.hpp"
#include "layer_panel.hpp"
#include "main_window.hpp"
#include "properties_panel.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/line.hpp"
#include "theme.hpp"

#include <QAbstractItemModel>
#include <QAction>
#include <QApplication>
#include <QImage>
#include <QLineEdit>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QTableView>
#include <QTemporaryDir>
#include <QTest>
#include <gtest/gtest.h>

#include <string>
#include <variant>

namespace tg::app {
namespace {

/// Coordinates typed and computed here are exact or nearly so.
constexpr double kTolerance = 1e-9;

/// A drawing with a layer "Walls" and a line from (-50, -50) to (50, 50).
core::Document wallsDrawing() {
    core::Document document = core::Document::newDrawing();
    core::Layer walls;
    walls.name = "Walls";
    walls.linetype = document.continuousLinetype();
    document.addLayer(walls);
    core::Line line;
    line.start = {-50.0, -50.0};
    line.end = {50.0, 50.0};
    document.add(line);
    return document;
}

class SessionTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_directory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("TecnigrafoP28Test"));
        QCoreApplication::setApplicationName(QStringLiteral("Session"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
        m_window.show();
        m_window.activateWindow();
        ASSERT_TRUE(QTest::qWaitForWindowActive(&m_window));
        view()->setView({0, 0}, 1.0);
        ASSERT_NE(m_window.activeSession(), nullptr);
        m_first = m_window.activeSession();
    }

    void TearDown() override {
        // Destruction avoids the save prompt of close() for modified drawings.
        QSettings::setDefaultFormat(m_format);
        QCoreApplication::setOrganizationName(m_organization);
        QCoreApplication::setApplicationName(m_application);
        applyTheme(*qobject_cast<QApplication*>(QApplication::instance()), Theme::Light);
    }

    [[nodiscard]] MainWindow& window() { return m_window; }

    /// The session the window opened with.
    [[nodiscard]] DocumentSession& first() const { return *m_first; }

    [[nodiscard]] CadView* view() const { return m_window.view(); }

    [[nodiscard]] QLineEdit* input() const {
        return m_window.commandLine()->findChild<QLineEdit*>(QStringLiteral("input"));
    }

    [[nodiscard]] LayerPanel* layers() const {
        return m_window.findChild<LayerPanel*>(QStringLiteral("layerPanel"));
    }

    [[nodiscard]] PropertiesPanel* properties() const {
        return m_window.findChild<PropertiesPanel*>(QStringLiteral("propertiesPanel"));
    }

    [[nodiscard]] DynamicInputBox* box() const { return view()->dynamicInputBox(); }

    [[nodiscard]] QLineEdit* field(int index) const {
        return box()->findChild<QLineEdit*>(QStringLiteral("dynamicField%1").arg(index));
    }

    /// `text` typed in the command line, then Enter.
    void enter(const std::string& text) const {
        events::type(input(), text);
        events::press(input(), Qt::Key_Return);
    }

    /// A LINE from `from` to `to`, ended with Enter.
    void line(const std::string& from, const std::string& to) const {
        enter("LINE");
        enter(from);
        enter(to);
        enter("");
    }

    /// The names of the layers the panel's table shows.
    [[nodiscard]] QStringList layerNames() const {
        const auto* table = layers()->findChild<QTableView*>(QStringLiteral("layerTable"));
        const QAbstractItemModel* model = table->model();
        QStringList names;
        for (int row = 0; row < model->rowCount(); ++row) {
            names.append(model->index(row, static_cast<int>(LayerColumn::Name)).data().toString());
        }
        return names;
    }

    [[nodiscard]] QImage picture() const { return view()->grab().toImage(); }

private:
    QTemporaryDir m_directory;
    QString m_organization{QCoreApplication::organizationName()};
    QString m_application{QCoreApplication::applicationName()};
    QSettings::Format m_format{QSettings::defaultFormat()};
    MainWindow m_window{MainWindow::InitialDrawing::Empty};
    DocumentSession* m_first{nullptr};
};

TEST_F(SessionTest, TheWindowOpensWithOneActiveDrawingSession) {
    ASSERT_EQ(window().sessions().size(), 1U);
    EXPECT_EQ(window().sessions().front().get(), &first());
    EXPECT_EQ(first().documentType(), QString::fromLatin1(DocumentSession::kDrawingType));
    EXPECT_EQ(&window().document(), &first().document());
    EXPECT_EQ(&window().editor(), &first().editor());
    EXPECT_EQ(&window().runner(), &first().runner());
    EXPECT_EQ(view()->session(), &first());
    EXPECT_EQ(window().commandLine()->session(), &first());
    EXPECT_EQ(layers()->session(), &first());
    EXPECT_EQ(properties()->session(), &first());
}

TEST_F(SessionTest, ActivateShowsTheOtherDocumentInTheViewAndThePanels) {
    const QImage empty = picture();
    DocumentSession& second = window().openSession(wallsDrawing());
    EXPECT_EQ(window().activeSession(), &first());
    ASSERT_EQ(layerNames(), QStringList{QStringLiteral("0")});

    window().activate(second);
    EXPECT_EQ(window().activeSession(), &second);
    EXPECT_EQ(&window().document(), &second.document());
    EXPECT_EQ(view()->session(), &second);
    // Never shown before: the view fits its drawing, so its line shows.
    EXPECT_NE(picture(), empty);
    EXPECT_EQ(layerNames(), (QStringList{QStringLiteral("0"), QStringLiteral("Walls")}));
    EXPECT_EQ(&layers()->document(), &second.document());
    EXPECT_EQ(&properties()->document(), &second.document());
    EXPECT_TRUE(window().windowTitle().startsWith(QStringLiteral("Drawing1.cadx")));

    window().activate(first());
    EXPECT_EQ(picture(), empty);
    EXPECT_EQ(layerNames(), QStringList{QStringLiteral("0")});
    EXPECT_EQ(&properties()->document(), &first().document());
}

TEST_F(SessionTest, TheViewOfASessionComesBackWithIt) {
    DocumentSession& second = window().openSession(wallsDrawing());
    window().activate(second);
    view()->setView({7.0, 3.0}, 0.25);
    window().activate(first());
    EXPECT_EQ(view()->viewport().center(), (core::Vec2{0.0, 0.0}));
    EXPECT_DOUBLE_EQ(view()->viewport().scale(), 1.0);
    window().activate(second);
    EXPECT_EQ(view()->viewport().center(), (core::Vec2{7.0, 3.0}));
    EXPECT_DOUBLE_EQ(view()->viewport().scale(), 4.0);
}

TEST_F(SessionTest, UndoActsOnTheActiveSessionOnly) {
    line("0,0", "10,0");
    DocumentSession& second = window().openSession();
    window().activate(second);
    line("0,0", "0,10");
    line("0,0", "5,5");
    ASSERT_EQ(first().document().entityCount(), 1U);
    ASSERT_EQ(second.document().entityCount(), 2U);

    enter("U");
    EXPECT_EQ(second.document().entityCount(), 1U);
    EXPECT_EQ(first().document().entityCount(), 1U);
    EXPECT_TRUE(first().editor().canUndo());

    // The Edit menu's Undo follows the active session too.
    QAction* undo = window().actionRegistry().action(QStringLiteral("edit.undo"));
    window().activate(first());
    ASSERT_TRUE(undo->isEnabled());
    undo->trigger();
    EXPECT_TRUE(first().document().isEmpty());
    EXPECT_EQ(second.document().entityCount(), 1U);
    EXPECT_FALSE(undo->isEnabled());
    window().activate(second);
    EXPECT_TRUE(undo->isEnabled());
}

TEST_F(SessionTest, ACommandLeftHalfwayResumesWhenItsSessionIsActiveAgain) {
    enter("LINE");
    enter("0,0");
    ASSERT_TRUE(first().runner().isActive());
    DocumentSession& second = window().openSession();
    window().activate(second);
    EXPECT_FALSE(window().runner().isActive());
    // The other session works meanwhile.
    line("1,1", "2,2");
    EXPECT_EQ(second.document().entityCount(), 1U);
    EXPECT_TRUE(first().document().isEmpty());

    window().activate(first());
    ASSERT_TRUE(window().runner().isActive());
    enter("10,0");
    enter("");
    EXPECT_FALSE(window().runner().isActive());
    ASSERT_EQ(first().document().entityCount(), 1U);
    const auto& made = std::get<core::Line>(first().document().entities().front().entity);
    EXPECT_EQ(made.start, (core::Vec2{0.0, 0.0}));
    EXPECT_EQ(made.end, (core::Vec2{10.0, 0.0}));
    EXPECT_EQ(second.document().entityCount(), 1U);
}

TEST_F(SessionTest, TextTypedAndNotSentStaysInItsSession) {
    enter("LINE");
    enter("0,0");
    events::type(input(), "10,20");
    DocumentSession& second = window().openSession();
    window().activate(second);
    EXPECT_TRUE(input()->text().isEmpty());
    // Enter in the other session hands nothing to the LINE of the first.
    events::press(input(), Qt::Key_Return);
    EXPECT_TRUE(first().document().isEmpty());
    EXPECT_TRUE(first().runner().isActive());
    EXPECT_TRUE(second.document().isEmpty());

    window().activate(first());
    EXPECT_EQ(input()->text(), QStringLiteral("10,20"));
    events::press(input(), Qt::Key_Return);
    ASSERT_EQ(first().document().entityCount(), 1U);
    const auto& made = std::get<core::Line>(first().document().entities().front().entity);
    EXPECT_EQ(made.end, (core::Vec2{10.0, 20.0}));
    EXPECT_TRUE(second.document().isEmpty());
}

TEST_F(SessionTest, ValuesTypedInTheDynamicInputComeBackInTheirFields) {
    ASSERT_TRUE(window().actionRegistry().action(QStringLiteral("aids.dynmode"))->isChecked());
    enter("LINE");
    enter("0,0");
    ASSERT_TRUE(box()->isVisible());
    ASSERT_EQ(field(0)->accessibleName(), QStringLiteral("Length"));
    events::type(view(), "5");
    events::press(view(), Qt::Key_Tab);
    events::type(view(), "9");
    ASSERT_EQ(field(0)->text(), QStringLiteral("5"));
    ASSERT_EQ(field(1)->text(), QStringLiteral("9"));

    DocumentSession& second = window().openSession();
    window().activate(second);
    EXPECT_FALSE(box()->isVisible());
    // Enter in the other session hands nothing to the LINE of the first.
    events::press(view(), Qt::Key_Return);
    EXPECT_TRUE(first().document().isEmpty());
    EXPECT_TRUE(first().runner().isActive());

    window().activate(first());
    ASSERT_TRUE(box()->isVisible());
    EXPECT_EQ(field(0)->text(), QStringLiteral("5"));
    EXPECT_EQ(field(1)->text(), QStringLiteral("9"));
    // The angle is still the active field: a digit goes on there.
    events::type(view(), "0");
    EXPECT_EQ(field(1)->text(), QStringLiteral("90"));
    events::press(view(), Qt::Key_Return);
    ASSERT_EQ(first().document().entityCount(), 1U);
    const auto& made = std::get<core::Line>(first().document().entities().front().entity);
    EXPECT_NEAR(made.end.x, 0.0, kTolerance);
    EXPECT_NEAR(made.end.y, 5.0, kTolerance);
    EXPECT_TRUE(second.document().isEmpty());
}

TEST_F(SessionTest, ClosingAnInactiveSessionLeavesNothingPointingAtIt) {
    DocumentSession* const second = &window().openSession(wallsDrawing());
    window().activate(*second);
    view()->setView({1.0, 1.0}, 0.5);
    enter("LINE");
    enter("0,0");
    events::type(input(), "3,3");
    window().activate(first());
    window().closeSession(*second);
    ASSERT_EQ(window().sessions().size(), 1U);
    EXPECT_EQ(window().activeSession(), &first());

    // Everything that followed the closed session works on the active one.
    line("0,0", "10,0");
    enter("U");
    line("0,0", "0,10");
    EXPECT_EQ(first().document().entityCount(), 1U);
    EXPECT_EQ(layerNames(), QStringList{QStringLiteral("0")});
    properties()->refresh();
    layers()->refresh();
    EXPECT_FALSE(picture().isNull());
    EXPECT_TRUE(input()->text().isEmpty());
}

TEST_F(SessionTest, ClosingTheActiveSessionActivatesAnother) {
    DocumentSession& second = window().openSession(wallsDrawing());
    window().activate(second);
    window().closeSession(second);
    EXPECT_EQ(window().activeSession(), &first());
    EXPECT_EQ(view()->session(), &first());
    EXPECT_EQ(layerNames(), QStringList{QStringLiteral("0")});

    window().closeSession(first());
    EXPECT_EQ(window().activeSession(), nullptr);
    EXPECT_TRUE(window().sessions().empty());
    EXPECT_EQ(view()->session(), nullptr);
    EXPECT_EQ(layers()->session(), nullptr);
    EXPECT_EQ(properties()->session(), nullptr);
    EXPECT_FALSE(input()->isEnabled());
    EXPECT_TRUE(layerNames().isEmpty());
    // Without a session the view paints and takes no input.
    EXPECT_FALSE(picture().isNull());
    events::click(view(), Qt::LeftButton, QPoint(10, 10));
    EXPECT_FALSE(window().actionRegistry().action(QStringLiteral("edit.undo"))->isEnabled());

    window().activate(window().openSession());
    line("0,0", "1,0");
    ASSERT_NE(window().activeSession(), nullptr);
    EXPECT_EQ(window().document().entityCount(), 1U);
}

}  // namespace
}  // namespace tg::app
