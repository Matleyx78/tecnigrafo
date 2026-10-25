// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "input_events.hpp"

#include <QAction>
#include <QApplication>
#include <QSignalSpy>
#include <QString>
#include <QToolButton>
#include <gtest/gtest.h>

#include <SARibbon.h>

namespace tg::app {
namespace {

TEST(RibbonSmoke, VisibleButtonTriggersItsAction) {
    SARibbonMainWindow window;
    auto* ribbon = window.ribbonBar();
    ASSERT_NE(ribbon, nullptr);
    auto* category = ribbon->addCategoryPage(QStringLiteral("Drawing"));
    ASSERT_NE(category, nullptr);
    auto* panel = category->addPanel(QStringLiteral("Draw"));
    ASSERT_NE(panel, nullptr);
    QAction action(QStringLiteral("Line"), &window);
    const QSignalSpy triggered(&action, &QAction::triggered);
    ASSERT_TRUE(triggered.isValid());
    panel->addLargeAction(&action);

    window.show();
    QApplication::processEvents();
    ASSERT_TRUE(window.isVisible());
    QToolButton* actionButton = nullptr;
    for (auto* button : panel->findChildren<QToolButton*>()) {
        if (button->defaultAction() == &action) {
            actionButton = button;
            break;
        }
    }
    ASSERT_NE(actionButton, nullptr);
    ASSERT_TRUE(actionButton->isVisible());
    ASSERT_TRUE(actionButton->isEnabled());
    EXPECT_EQ(triggered.count(), 0);
    events::click(actionButton, Qt::LeftButton, actionButton->rect().center());
    EXPECT_EQ(triggered.count(), 1);
}

}  // namespace
}  // namespace tg::app
