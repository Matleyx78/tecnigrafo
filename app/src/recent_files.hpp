// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QStringList>

class QSettings;

namespace tg::app {

/// Last nine drawings, most recent first, in QSettings under "recentFiles".
/// The settings must outlive this object.
class RecentFiles {
public:
    explicit RecentFiles(QSettings& settings);
    void add(const QString& absolutePath);
    void remove(const QString& absolutePath);
    [[nodiscard]] QStringList files() const;

private:
    QSettings& m_settings;
};

}  // namespace tg::app
