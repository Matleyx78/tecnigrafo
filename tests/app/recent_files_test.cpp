// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "recent_files.hpp"

#include <QSettings>
#include <QTemporaryDir>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

TEST(RecentFilesTest, OrdersDeduplicatesLimitsAndPersists) {
    const QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("settings.ini"));
    QSettings settings(path, QSettings::IniFormat);
    RecentFiles recent(settings);
    EXPECT_TRUE(recent.files().isEmpty());
    for (int i = 0; i < 12; ++i) {
        recent.add(QStringLiteral("/drawings/%1.cadx").arg(i));
    }
    EXPECT_EQ(recent.files().size(), 9);
    EXPECT_EQ(recent.files().first(), QStringLiteral("/drawings/11.cadx"));
    EXPECT_EQ(recent.files().last(), QStringLiteral("/drawings/3.cadx"));
    recent.add(QStringLiteral("/drawings/5.cadx"));
    EXPECT_EQ(recent.files().first(), QStringLiteral("/drawings/5.cadx"));
    EXPECT_EQ(recent.files().count(QStringLiteral("/drawings/5.cadx")), 1);
    recent.remove(QStringLiteral("/drawings/7.cadx"));
    recent.remove(QStringLiteral("/drawings/missing.cadx"));
    EXPECT_EQ(recent.files().size(), 8);
    settings.sync();
    QSettings reopened(path, QSettings::IniFormat);
    EXPECT_EQ(RecentFiles(reopened).files(), recent.files());
    QSettings separate(directory.filePath(QStringLiteral("separate.ini")), QSettings::IniFormat);
    EXPECT_TRUE(RecentFiles(separate).files().isEmpty());
}

}  // namespace
}  // namespace tg::app
