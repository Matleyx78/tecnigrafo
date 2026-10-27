// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "shortcut_map.hpp"

#include <QAction>
#include <QTemporaryDir>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

void addShortcutAction(ActionRegistry& registry, const QString& id, const QString& keys,
                       const QString& environment = QStringLiteral("drawing2d")) {
    ActionSpec spec;
    spec.id = id;
    spec.text = id;
    spec.shortcut = QKeySequence(keys);
    spec.environments = {environment};
    registry.add(spec);
}

TEST(ShortcutMap, ChangesRemovalConflictsEnvironmentsAndReset) {
    ActionRegistry registry;
    addShortcutAction(registry, QStringLiteral("a"), QStringLiteral("Ctrl+A"));
    addShortcutAction(registry, QStringLiteral("b"), QStringLiteral("Ctrl+B"));
    addShortcutAction(registry, QStringLiteral("c"), QStringLiteral("Ctrl+A"),
                      QStringLiteral("part"));
    ShortcutMap map(registry);
    EXPECT_EQ(map.conflict(QStringLiteral("b"), QKeySequence(QStringLiteral("Ctrl+A"))),
              QStringLiteral("a"));
    EXPECT_FALSE(map.set(QStringLiteral("b"), QKeySequence(QStringLiteral("Ctrl+A"))));
    EXPECT_EQ(map.shortcut(QStringLiteral("b")), QKeySequence(QStringLiteral("Ctrl+B")));
    EXPECT_TRUE(map.set(QStringLiteral("a"), {}));
    EXPECT_TRUE(map.conflict(QStringLiteral("b"), {}).isEmpty());
    EXPECT_TRUE(map.set(QStringLiteral("b"), QKeySequence(QStringLiteral("Ctrl+A"))));
    EXPECT_FALSE(map.set(QStringLiteral("unknown"), {}));
    map.resetAll();
    EXPECT_TRUE(map.set(QStringLiteral("a"), QKeySequence(QStringLiteral("F6"))));
    map.apply(registry);
    EXPECT_EQ(registry.action(QStringLiteral("a"))->shortcut(), QKeySequence(QStringLiteral("F6")));
    map.reset(QStringLiteral("a"));
    EXPECT_TRUE(map.exportText().isEmpty());
}

TEST(ShortcutMap, SettingsStoreOnlyDifferencesAndIgnoreBadEntries) {
    const QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("shortcuts.ini")), QSettings::IniFormat);
    ActionRegistry registry;
    addShortcutAction(registry, QStringLiteral("a"), QStringLiteral("Ctrl+A"));
    addShortcutAction(registry, QStringLiteral("b"), QStringLiteral("Ctrl+B"));
    ShortcutMap map(registry);
    ASSERT_TRUE(map.set(QStringLiteral("a"), {}));
    map.save(settings);
    settings.sync();
    {
        QSettings reloaded(settings.fileName(), QSettings::IniFormat);
        ShortcutMap restored(registry);
        restored.load(reloaded);
        EXPECT_EQ(restored.exportText(), QStringLiteral("a\t\n"));
        EXPECT_FALSE(reloaded.contains(QStringLiteral("shortcuts/b")));
    }
    settings.setValue(QStringLiteral("shortcuts/a"), QStringLiteral("not a key"));
    settings.setValue(QStringLiteral("shortcuts/b"), QStringLiteral("Ctrl+A"));
    settings.setValue(QStringLiteral("shortcuts/unknown"), QStringLiteral("F6"));
    map.load(settings);
    EXPECT_TRUE(map.exportText().isEmpty());
    map.save(settings);
    EXPECT_FALSE(settings.contains(QStringLiteral("shortcuts/unknown")));
}

TEST(ShortcutMap, TextRoundTripSwapsDefaultsAndReportsErrorsAtomically) {
    ActionRegistry registry;
    addShortcutAction(registry, QStringLiteral("a"), QStringLiteral("Ctrl+A"));
    addShortcutAction(registry, QStringLiteral("b"), QStringLiteral("Ctrl+B"));
    ShortcutMap map(registry);
    const QString text = QStringLiteral("a\tCtrl+B\nb\tCtrl+A\n");
    EXPECT_TRUE(map.importText(text).isEmpty());
    EXPECT_EQ(map.exportText(), text);
    const QStringList errors = map.importText(QStringLiteral("a\tF6\nunknown\tF7\nb\tnot a key\n"));
    ASSERT_EQ(errors.size(), 2);
    EXPECT_TRUE(errors[0].contains(QStringLiteral("2")));
    EXPECT_TRUE(errors[1].contains(QStringLiteral("3")));
    EXPECT_EQ(map.exportText(), text);
    EXPECT_FALSE(map.importText(QStringLiteral("a\tCtrl+A\n")).isEmpty());
    EXPECT_EQ(map.exportText(), text);
    EXPECT_FALSE(map.importText(QStringLiteral("a\tF6\na\tF7\n")).isEmpty());
    EXPECT_TRUE(map.importText(QStringLiteral("a\t\r\nb\tCtrl+A\r\n")).isEmpty());
    EXPECT_TRUE(map.shortcut(QStringLiteral("a")).isEmpty());
    const QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("swap.ini")), QSettings::IniFormat);
    ASSERT_TRUE(map.importText(text).isEmpty());
    map.save(settings);
    ShortcutMap restored(registry);
    restored.load(settings);
    EXPECT_EQ(restored.exportText(), text);
}

}  // namespace
}  // namespace tg::app
