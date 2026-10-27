// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "settings_dialog.hpp"

#include "action_registry.hpp"
#include "main_window.hpp"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFile>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolButton>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

class SettingsTest : public testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(m_directory.isValid());
        QCoreApplication::setOrganizationName(QStringLiteral("TecnigrafoP17Test"));
        QCoreApplication::setApplicationName(QStringLiteral("Settings"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_directory.path());
    }

    void TearDown() override {
        QSettings::setDefaultFormat(m_format);
        QCoreApplication::setOrganizationName(m_organization);
        QCoreApplication::setApplicationName(m_application);
        applyTheme(*qobject_cast<QApplication*>(QApplication::instance()), Theme::Light);
    }

    static QDockWidget* panel(MainWindow& window, const QString& name) {
        auto* dock = new QDockWidget(name, &window);
        dock->setObjectName(name);
        return dock;
    }

private:
    QTemporaryDir m_directory;
    QString m_organization{QCoreApplication::organizationName()};
    QString m_application{QCoreApplication::applicationName()};
    QSettings::Format m_format{QSettings::defaultFormat()};
};

TEST_F(SettingsTest, RestartRestoresInterfaceThemeGeometryAndPanelLayout) {
    QSize size;
    {
        MainWindow window(MainWindow::InitialDrawing::Empty);
        auto* dock = panel(window, QStringLiteral("testPanel"));
        window.addPanel(dock, Qt::LeftDockWidgetArea);
        window.show();
        window.setInterface(InterfaceKind::Classic);
        window.setTheme(Theme::Dark);
        window.resize(900, 650);
        window.addDockWidget(Qt::RightDockWidgetArea, dock);
        QApplication::processEvents();
        size = window.size();
        ASSERT_TRUE(window.close());
    }
    QSettings settings;
    EXPECT_EQ(settings.value(QStringLiteral("ui/interface")).toString(), QStringLiteral("classic"));
    EXPECT_EQ(settings.value(QStringLiteral("ui/theme")).toString(), QStringLiteral("dark"));
    EXPECT_FALSE(settings.value(QStringLiteral("ui/geometry")).toByteArray().isEmpty());
    EXPECT_FALSE(settings.value(QStringLiteral("ui/state")).toByteArray().isEmpty());
    MainWindow restored(MainWindow::InitialDrawing::Empty);
    auto* dock = panel(restored, QStringLiteral("testPanel"));
    restored.addPanel(dock, Qt::LeftDockWidgetArea);
    restored.show();
    EXPECT_EQ(restored.interfaceKind(), InterfaceKind::Classic);
    EXPECT_EQ(restored.theme(), Theme::Dark);
    EXPECT_EQ(restored.size(), size);
    EXPECT_EQ(restored.dockWidgetArea(dock), Qt::RightDockWidgetArea);
    EXPECT_TRUE(dock->features().testFlag(QDockWidget::DockWidgetFloatable));
    EXPECT_TRUE(restored.dockOptions().testFlag(QMainWindow::AllowTabbedDocks));
    EXPECT_TRUE(restored.close());
}

TEST_F(SettingsTest, LatePanelRestoresFloatingStateAndPanelsCanBeTabbed) {
    {
        MainWindow window(MainWindow::InitialDrawing::Empty);
        auto* first = panel(window, QStringLiteral("first"));
        auto* second = panel(window, QStringLiteral("second"));
        window.addPanel(first, Qt::LeftDockWidgetArea);
        window.addPanel(second, Qt::LeftDockWidgetArea);
        window.tabifyDockWidget(first, second);
        EXPECT_TRUE(window.tabifiedDockWidgets(first).contains(second));
        window.show();
        second->setFloating(true);
        EXPECT_TRUE(window.close());
    }
    MainWindow restored(MainWindow::InitialDrawing::Empty);
    restored.show();
    auto* second = panel(restored, QStringLiteral("second"));
    restored.addPanel(second, Qt::RightDockWidgetArea);
    EXPECT_TRUE(second->isFloating());
    EXPECT_TRUE(restored.close());
}

TEST_F(SettingsTest, ScriptModeIgnoresAndNeverWritesExistingSettings) {
    QSettings settings;
    settings.setValue(QStringLiteral("ui/interface"), QStringLiteral("classic"));
    settings.setValue(QStringLiteral("ui/theme"), QStringLiteral("dark"));
    settings.setValue(QStringLiteral("ui/geometry"), QByteArray("sentinel geometry"));
    settings.setValue(QStringLiteral("ui/state"), QByteArray("sentinel state"));
    settings.sync();
    QFile file(settings.fileName());
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    const QByteArray before = file.readAll();
    file.close();
    {
        MainWindow window(MainWindow::InitialDrawing::Empty);
        window.setScriptMode();
        window.show();
        EXPECT_EQ(window.theme(), Theme::Light);
        EXPECT_EQ(window.interfaceKind(), InterfaceKind::Ribbon);
        EXPECT_EQ(window.size(), QSize(1100, 720));
        window.setTheme(Theme::Dark);
        window.setInterface(InterfaceKind::Classic);
        EXPECT_TRUE(window.close());
    }
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    EXPECT_EQ(file.readAll(), before);
}

TEST_F(SettingsTest, ScriptModeDoesNotCreateSettings) {
    const QString path = QSettings{}.fileName();
    {
        MainWindow window(MainWindow::InitialDrawing::Empty);
        window.setScriptMode();
        window.show();
        EXPECT_TRUE(window.close());
    }
    EXPECT_FALSE(QFile::exists(path));
}

TEST_F(SettingsTest, ChoicesPersistImmediatelyAndSettingsDialogAppliesOnlyOnAccept) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.show();
    window.actionRegistry().action(QStringLiteral("ui.interface"))->trigger();
    EXPECT_EQ(QSettings{}.value(QStringLiteral("ui/interface")).toString(), QStringLiteral("classic"));
    window.actionRegistry().action(QStringLiteral("ui.theme"))->trigger();
    EXPECT_EQ(QSettings{}.value(QStringLiteral("ui/theme")).toString(), QStringLiteral("dark"));
    for (const bool accept : {false, true}) {
        QTimer::singleShot(0, &window, [&window, accept] {
            auto* dialog = window.findChild<SettingsDialog*>();
            ASSERT_NE(dialog, nullptr);
            dialog->findChild<QComboBox*>(QStringLiteral("interfaceCombo"))->setCurrentIndex(0);
            dialog->findChild<QComboBox*>(QStringLiteral("themeCombo"))->setCurrentIndex(0);
            auto* buttons = dialog->findChild<QDialogButtonBox*>();
            buttons->button(accept ? QDialogButtonBox::Ok : QDialogButtonBox::Cancel)->click();
        });
        window.actionRegistry().action(QStringLiteral("tools.settings"))->trigger();
        EXPECT_EQ(window.theme(), accept ? Theme::Light : Theme::Dark);
        EXPECT_EQ(window.interfaceKind(), accept ? InterfaceKind::Ribbon : InterfaceKind::Classic);
    }
    EXPECT_TRUE(window.close());
}

TEST_F(SettingsTest, LwtButtonAndActionTrackDrawing) {
    MainWindow window(MainWindow::InitialDrawing::Empty);
    window.setScriptMode();
    window.show();
    auto* button = window.findChild<QToolButton*>(QStringLiteral("lwdisplayButton"));
    auto* action = window.actionRegistry().action(QStringLiteral("aids.lwdisplay"));
    ASSERT_NE(button, nullptr);
    ASSERT_NE(action, nullptr);
    EXPECT_FALSE(window.document().lwdisplay());
    button->click();
    EXPECT_TRUE(window.document().lwdisplay());
    EXPECT_TRUE(action->isChecked());
    action->trigger();
    EXPECT_FALSE(window.document().lwdisplay());
    EXPECT_FALSE(button->isChecked());
}

}  // namespace
}  // namespace tg::app
