// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "recent_files.hpp"

#include <QSettings>

namespace tg::app {
namespace {
constexpr qsizetype kMaxRecentFiles = 9;
const QString kRecentFilesKey = QStringLiteral("recentFiles");
}  // namespace

RecentFiles::RecentFiles(QSettings& settings) : m_settings(settings) {}

void RecentFiles::add(const QString& absolutePath) {
    QStringList paths = files();
    paths.removeAll(absolutePath);
    paths.prepend(absolutePath);
    while (paths.size() > kMaxRecentFiles) {
        paths.removeLast();
    }
    m_settings.setValue(kRecentFilesKey, paths);
}

void RecentFiles::remove(const QString& absolutePath) {
    QStringList paths = files();
    paths.removeAll(absolutePath);
    m_settings.setValue(kRecentFilesKey, paths);
}

QStringList RecentFiles::files() const {
    return m_settings.value(kRecentFilesKey).toStringList();
}

}  // namespace tg::app
