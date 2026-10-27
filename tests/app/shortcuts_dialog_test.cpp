// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "shortcuts_dialog.hpp"

#include "builtin_actions.hpp"

#include <QAction>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <gtest/gtest.h>

namespace tg::app {
namespace {

void chooseAction(ShortcutsDialog& dialog, const QString& id) {
    auto* table = dialog.findChild<QTableWidget*>();
    for (int row = 0; row < table->rowCount(); ++row) {
        if (table->item(row, 0)->data(Qt::UserRole).toString() == id) {
            table->selectRow(row);
            return;
        }
    }
    FAIL() << "Action missing: " << id.toStdString();
}

void editShortcut(ShortcutsDialog& dialog, const QString& text) {
    auto* edit = dialog.findChild<QKeySequenceEdit*>();
    ASSERT_TRUE(edit->isEnabled());
    edit->setKeySequence(QKeySequence(text));
    ASSERT_TRUE(QMetaObject::invokeMethod(edit, "editingFinished", Qt::DirectConnection));
}

TEST(ShortcutsDialog, ChangesApplyImmediatelyRejectConflictsAndPersistOnAccept) {
    const QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("ui.ini")), QSettings::IniFormat);
    ActionRegistry registry;
    addBuiltinActions(registry);
    ShortcutsDialog dialog(registry, settings);
    chooseAction(dialog, QStringLiteral("draw.line"));
    editShortcut(dialog, QStringLiteral("F6"));
    EXPECT_EQ(registry.action(QStringLiteral("draw.line"))->shortcut(),
              QKeySequence(QStringLiteral("F6")));
    EXPECT_FALSE(settings.contains(QStringLiteral("shortcuts/draw.line")));
    chooseAction(dialog, QStringLiteral("draw.circle"));
    editShortcut(dialog, QStringLiteral("F6"));
    EXPECT_TRUE(registry.action(QStringLiteral("draw.circle"))->shortcut().isEmpty());
    auto* warning = dialog.findChild<QLabel*>(QStringLiteral("shortcutWarning"));
    EXPECT_TRUE(warning->text().contains(registry.spec(QStringLiteral("draw.line"))->text));
    dialog.accept();
    EXPECT_NE(dialog.result(), QDialog::Accepted);
    editShortcut(dialog, QStringLiteral("F7"));
    dialog.accept();
    EXPECT_EQ(dialog.result(), QDialog::Accepted);
    ActionRegistry restarted;
    addBuiltinActions(restarted);
    ShortcutMap restored(restarted);
    QSettings reloaded(settings.fileName(), QSettings::IniFormat);
    restored.load(reloaded);
    restored.apply(restarted);
    EXPECT_EQ(restarted.action(QStringLiteral("draw.line"))->shortcut(),
              QKeySequence(QStringLiteral("F6")));
    EXPECT_EQ(restarted.action(QStringLiteral("draw.circle"))->shortcut(),
              QKeySequence(QStringLiteral("F7")));
}

TEST(ShortcutsDialog, FilterRemoveResetAndCancelRestoreSession) {
    const QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    QSettings settings(directory.filePath(QStringLiteral("ui.ini")), QSettings::IniFormat);
    ActionRegistry registry;
    addBuiltinActions(registry);
    const auto original = registry.action(QStringLiteral("edit.redo"))->shortcuts();
    ShortcutsDialog dialog(registry, settings);
    dialog.findChild<QLineEdit*>()->setText(QStringLiteral("edit.redo"));
    auto* table = dialog.findChild<QTableWidget*>();
    ASSERT_EQ(table->rowCount(), 1);
    table->selectRow(0);
    dialog.findChild<QPushButton*>(QStringLiteral("removeShortcut"))->click();
    EXPECT_TRUE(registry.action(QStringLiteral("edit.redo"))->shortcut().isEmpty());
    dialog.findChild<QPushButton*>(QStringLiteral("resetShortcut"))->click();
    EXPECT_EQ(registry.action(QStringLiteral("edit.redo"))->shortcut(), original.value(0));
    editShortcut(dialog, QStringLiteral("F6"));
    dialog.findChild<QPushButton*>(QStringLiteral("resetAllShortcuts"))->click();
    EXPECT_EQ(registry.action(QStringLiteral("edit.redo"))->shortcut(), original.value(0));
    editShortcut(dialog, QStringLiteral("F7"));
    dialog.reject();
    EXPECT_EQ(registry.action(QStringLiteral("edit.redo"))->shortcuts(), original);
    EXPECT_FALSE(settings.contains(QStringLiteral("shortcuts/edit.redo")));
}

}  // namespace
}  // namespace tg::app
