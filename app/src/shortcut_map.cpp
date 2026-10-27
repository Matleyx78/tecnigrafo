// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "shortcut_map.hpp"

#include <QAction>

namespace tg::app {
namespace {

bool validSequence(const QString& text, QKeySequence& sequence) {
    sequence = QKeySequence::fromString(text, QKeySequence::PortableText);
    if (text.isEmpty()) {
        return true;
    }
    if (sequence.isEmpty()) {
        return false;
    }
    for (int i = 0; i < sequence.count(); ++i) {
        const auto key = sequence[i].key();
        if (key == Qt::Key_unknown || key == static_cast<Qt::Key>(0)) {
            return false;
        }
    }
    // Qt can accept a valid prefix and silently discard malformed suffixes.
    return sequence.toString(QKeySequence::PortableText) == text;
}

}  // namespace

ShortcutMap::ShortcutMap(const ActionRegistry& registry) {
    for (const QAction* action : registry.actions()) {
        const ActionSpec* spec = registry.spec(action->objectName());
        // Separators and run-time placeholders are not user actions.
        if (!spec->text.isEmpty()) {
            m_ids.append(spec->id);
            m_specs.insert(spec->id, *spec);
        }
    }
}

QKeySequence ShortcutMap::shortcut(const QString& id) const {
    return m_changes.value(id, m_specs.value(id).shortcut);
}

QString ShortcutMap::conflict(const QString& id, const QKeySequence& sequence) const {
    if (sequence.isEmpty() || !m_specs.contains(id)) {
        return {};
    }
    const auto environments = m_specs.value(id).environments;
    for (const QString& other : m_ids) {
        if (other == id || shortcut(other) != sequence) {
            continue;
        }
        for (const QString& environment : m_specs.value(other).environments) {
            if (environments.contains(environment)) {
                return other;
            }
        }
    }
    return {};
}

bool ShortcutMap::set(const QString& id, const QKeySequence& sequence) {
    if (!m_specs.contains(id) || !conflict(id, sequence).isEmpty()) {
        return false;
    }
    if (sequence == m_specs.value(id).shortcut) {
        m_changes.remove(id);
    } else {
        m_changes.insert(id, sequence);
    }
    return true;
}

void ShortcutMap::reset(const QString& id) {
    set(id, m_specs.value(id).shortcut);
}

void ShortcutMap::resetAll() {
    m_changes.clear();
}

void ShortcutMap::apply(ActionRegistry& registry) const {
    for (const QString& id : m_ids) {
        if (QAction* action = registry.action(id); action != nullptr) {
            if (action->shortcut() != shortcut(id)) {
                action->setShortcut(shortcut(id));
            }
        }
    }
}

void ShortcutMap::save(QSettings& settings) const {
    settings.beginGroup(QStringLiteral("shortcuts"));
    settings.remove(QString{});
    for (const QString& id : m_ids) {
        if (m_changes.contains(id)) {
            settings.setValue(id, shortcut(id).toString(QKeySequence::PortableText));
        }
    }
    settings.endGroup();
}

void ShortcutMap::load(QSettings& settings) {
    settings.beginGroup(QStringLiteral("shortcuts"));
    ShortcutMap candidate = *this;
    candidate.resetAll();
    QString batch;
    for (const QString& id : settings.childKeys()) {
        QKeySequence sequence;
        if (m_specs.contains(id) && validSequence(settings.value(id).toString(), sequence)) {
            batch += id + QLatin1Char('\t') + sequence.toString(QKeySequence::PortableText) +
                     QLatin1Char('\n');
        }
    }
    if (candidate.importText(batch).isEmpty()) {
        settings.endGroup();
        m_changes = candidate.m_changes;
        return;
    }
    // Keep valid entries when a corrupt settings file contains conflicts.
    for (const QString& id : settings.childKeys()) {
        QKeySequence sequence;
        if (m_specs.contains(id) && validSequence(settings.value(id).toString(), sequence)) {
            candidate.set(id, sequence);
        }
    }
    settings.endGroup();
    m_changes = candidate.m_changes;
}

QString ShortcutMap::exportText() const {
    QString result;
    for (const QString& id : m_ids) {
        if (m_changes.contains(id)) {
            result += id + QLatin1Char('\t') + shortcut(id).toString(QKeySequence::PortableText) +
                      QLatin1Char('\n');
        }
    }
    return result;
}

QStringList ShortcutMap::importText(const QString& text) {
    ShortcutMap candidate = *this;
    QStringList errors;
    const QStringList lines = text.split(QLatin1Char('\n'));
    QHash<QString, QKeySequence> parsed;
    for (qsizetype i = 0; i < lines.size(); ++i) {
        QString line = lines[i];
        if (line.endsWith(QLatin1Char('\r'))) {
            line.chop(1);
        }
        if (line.isEmpty()) {
            continue;
        }
        const QStringList fields = line.split(QLatin1Char('\t'));
        QKeySequence sequence;
        if (fields.size() != 2 || !m_specs.contains(fields[0]) || parsed.contains(fields[0]) ||
            !validSequence(fields.value(1), sequence)) {
            errors.append(
                QStringLiteral("Line %1: unknown action, duplicate or invalid sequence.").arg(i + 1));
            continue;
        }
        parsed.insert(fields[0], sequence);
        candidate.set(fields[0], {});
    }
    for (qsizetype i = 0; i < lines.size(); ++i) {
        const QString id = lines[i].section(QLatin1Char('\t'), 0, 0);
        if (parsed.contains(id) && !candidate.set(id, parsed.value(id))) {
            errors.append(QStringLiteral("Line %1: shortcut conflict with %2.")
                              .arg(i + 1)
                              .arg(candidate.conflict(id, parsed.value(id))));
        }
    }
    if (errors.isEmpty()) {
        m_changes = candidate.m_changes;
    }
    return errors;
}

}  // namespace tg::app
