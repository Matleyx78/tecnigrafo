// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "action_registry.hpp"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QWidget>

#include <cassert>
#include <memory>
#include <utility>

namespace tg::app {

namespace {

// Separates the levels of ActionSpec::menu: "&Tools/Object Snap &Modes".
constexpr QChar kMenuSeparator = QLatin1Char('/');

}  // namespace

ActionRegistry::ActionRegistry(QObject* parent) : QObject(parent) {}

ActionRegistry::~ActionRegistry() {
    // The actions, children of the registry, outlive the menus of the
    // entries: they must not keep pointing at them.
    for (Entry& entry : m_entries) {
        if (entry.menu != nullptr) {
            entry.action->setMenu(static_cast<QMenu*>(nullptr));
        }
    }
}

QAction* ActionRegistry::add(ActionSpec spec) {
    assert(!spec.id.isEmpty() && "action without id");
    assert(!m_index.contains(spec.id) && "action id already registered");
    Entry* owner = nullptr;
    if (!spec.variantOf.isEmpty()) {
        owner = m_index.value(spec.variantOf, nullptr);
        assert(owner != nullptr && "variant of an action not yet added");
        assert(spec.menu.isEmpty() && spec.ribbon.isEmpty() && "a variant is in its action's menu");
    }
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
    Entry& entry = m_entries.emplace_back();
    entry.spec = std::move(spec);
    entry.action = action;
    m_index.insert(entry.spec.id, &entry);
    if (owner != nullptr) {
        addVariant(*owner, action);
    }
    return action;
}

void ActionRegistry::addVariant(Entry& owner, QAction* variant) {
    if (owner.menu == nullptr) {
        // The window, when the registry has one, is the menu's parent, so
        // that the menu pops up as the window's own; the entry still owns
        // it, and the registry, which is older than the menu, is destroyed
        // first.
        owner.menu = std::make_unique<QMenu>(qobject_cast<QWidget*>(parent()));
        owner.menu->setObjectName(owner.spec.id + QStringLiteral(".variants"));
        owner.action->setMenu(owner.menu.get());
        owner.current = variant;
    }
    owner.menu->addAction(variant);
    owner.variants.append(variant);
    // The entries stay where they are (a deque), so `owner` can be kept.
    Entry* kept = &owner;
    connect(variant, &QAction::triggered, this, [kept, variant] { useVariant(*kept, variant); });
    // The icon of the current variant follows the theme: MainWindow sets
    // the icons of the variants after the one of their action.
    connect(variant, &QAction::changed, this, [kept, variant] {
        if (kept->current == variant &&
            kept->action->icon().cacheKey() != variant->icon().cacheKey()) {
            kept->action->setIcon(variant->icon());
        }
    });
}

void ActionRegistry::useVariant(Entry& owner, QAction* variant) {
    owner.current = variant;
    owner.action->setIcon(variant->icon());
    owner.action->setToolTip(variant->toolTip());
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

QList<QAction*> ActionRegistry::variants(const QString& id) const {
    const Entry* entry = m_index.value(id, nullptr);
    return entry != nullptr ? entry->variants : QList<QAction*>{};
}

QAction* ActionRegistry::currentVariant(const QString& id) const {
    const Entry* entry = m_index.value(id, nullptr);
    return entry != nullptr ? entry->current : nullptr;
}

QList<QAction*> ActionRegistry::dynamicActions(const QString& prefix, int count) {
    QList<QAction*> result;
    for (int i = 1; i <= count; ++i) {
        const QString id = QStringLiteral("%1.%2").arg(prefix).arg(i);
        QAction* existing = action(id);
        if (existing == nullptr) {
            ActionSpec spec;
            spec.id = id;
            // An entry of the list works where the list does: without a
            // document, for the recent files (docs/tappe/0.7.5/P29, B7).
            if (const ActionSpec* list = this->spec(prefix); list != nullptr) {
                spec.requiresDocument = list->requiresDocument;
            }
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
