// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "action_registry.hpp"

#include <QSettings>

namespace tg::app {

/// Registry shortcuts with user overrides. Only differences are persisted.
class ShortcutMap {
public:
    explicit ShortcutMap(const ActionRegistry& registry);
    [[nodiscard]] QKeySequence shortcut(const QString& id) const;
    [[nodiscard]] QString conflict(const QString& id, const QKeySequence& sequence) const;
    bool set(const QString& id, const QKeySequence& sequence);
    void reset(const QString& id);
    void resetAll();
    void apply(ActionRegistry& registry) const;
    void save(QSettings& settings) const;
    void load(QSettings& settings);
    [[nodiscard]] QString exportText() const;
    [[nodiscard]] QStringList importText(const QString& text);

private:
    QHash<QString, ActionSpec> m_specs;
    QStringList m_ids;
    QHash<QString, QKeySequence> m_changes;
};

}  // namespace tg::app
