// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "log_file.hpp"

#include <QFile>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

TEST(LogFileTest, TheLogNamesItselfAndKeepsTheMessagesOfTheSession) {
    // Never the log of the user: the folders of the test mode of Qt.
    QStandardPaths::setTestModeEnabled(true);
    const QString path = startLogFile();
    ASSERT_FALSE(path.isEmpty());
    qWarning("message of the log test, %d", 42);
    // The messages go where they went before.
    (void)qInstallMessageHandler(nullptr);
    QStandardPaths::setTestModeEnabled(false);

    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    ASSERT_GE(lines.size(), 2);
    EXPECT_TRUE(lines.front().contains(QStringLiteral("Qt "))) << lines.front().toStdString();
    EXPECT_TRUE(lines.front().endsWith(QStringLiteral("tecnigrafo.log")))
        << lines.front().toStdString();
    EXPECT_TRUE(lines.at(1).endsWith(QStringLiteral("warning: message of the log test, 42")))
        << lines.at(1).toStdString();
}

}  // namespace
}  // namespace tg::app
