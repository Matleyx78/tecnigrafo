// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "action_registry.hpp"

#include <QAction>
#include <QMenu>
#include <QMenuBar>

#include <cassert>
#include <utility>

namespace tg::app {

namespace {

// Separates the levels of ActionSpec::menu: "&Tools/Object Snap &Modes".
constexpr QChar kMenuSeparator = QLatin1Char('/');

}  // namespace

ActionRegistry::ActionRegistry(QObject* parent) : QObject(parent) {}

QAction* ActionRegistry::add(ActionSpec spec) {
    assert(!spec.id.isEmpty() && "action without id");
    assert(!m_index.contains(spec.id) && "action id already registered");
    auto* action = new QAction(this);
    action->setObjectName(spec.id);
    if (!spec.menu.isEmpty() && spec.text.isEmpty()) {
        action->setSeparator(true);
    } else {
        action->setText(spec.text);
    }
    if (!spec.description.isEmpty()) {
        action->setToolTip(spec.description);
    }
    if (!spec.shortcut.isEmpty()) {
        action->setShortcut(spec.shortcut);
    }
    action->setCheckable(spec.checkable);
    const Entry& entry = m_entries.emplace_back(Entry{std::move(spec), action});
    m_index.insert(entry.spec.id, &entry);
    return action;
}

QAction* ActionRegistry::action(const QString& id) const {
    const Entry* entry = m_index.value(id, nullptr);
    return entry != nullptr ? entry->action : nullptr;
}

const ActionSpec* ActionRegistry::spec(const QString& id) const {
    const Entry* entry = m_index.value(id, nullptr);
    return entry != nullptr ? &entry->spec : nullptr;
}

QList<QAction*> ActionRegistry::actions() const {
    QList<QAction*> result;
    result.reserve(static_cast<qsizetype>(m_entries.size()));
    for (const Entry& entry : m_entries) {
        result.append(entry.action);
    }
    return result;
}

QList<QAction*> ActionRegistry::dynamicActions(const QString& prefix, int count) {
    QList<QAction*> result;
    for (int i = 1; i <= count; ++i) {
        const QString id = QStringLiteral("%1.%2").arg(prefix).arg(i);
        QAction* existing = action(id);
        if (existing == nullptr) {
            ActionSpec spec;
            spec.id = id;
            existing = add(std::move(spec));
        }
        result.append(existing);
    }
    return result;
}

void ActionRegistry::buildMenus(QMenuBar* bar) const {
    // Every menu built so far, by its whole path.
    QHash<QString, QMenu*> menus;
    for (const Entry& entry : m_entries) {
        if (entry.spec.menu.isEmpty()) {
            continue;
        }
        QMenu* menu = nullptr;
        QString path;
        for (const QString& title : entry.spec.menu.split(kMenuSeparator)) {
            path = path.isEmpty() ? title : path + kMenuSeparator + title;
            QMenu* found = menus.value(path, nullptr);
            if (found == nullptr) {
                found = menu == nullptr ? bar->addMenu(title) : menu->addMenu(title);
                menus.insert(path, found);
            }
            menu = found;
        }
        // A non-empty path has at least one title, so a menu was found; the
        // check is for the analyser, which cannot know it.
        if (menu != nullptr) {
            menu->addAction(entry.action);
        }
    }
}

}  // namespace tg::app
