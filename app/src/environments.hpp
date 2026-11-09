// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <deque>

namespace tg::app {

/// An environment (docs/adr/0041, point 1): the commands, ribbon and menus
/// that are valid while a document is edited in it.
struct Environment {
    QString id;                    ///< "drawing2d", as in ActionSpec::environments
    QString name;                  ///< "Drawing"
    QString icon;                  ///< icon name, docs/adr/0040
    QStringList contextualRibbon;  ///< ribbon tabs it adds, docs/adr/0039 point 5
};

/// The environments of the program, in the order they were added. It is
/// filled once, when the window is put together (docs/adr/0041, point 7):
/// no environment is ever removed.
class EnvironmentRegistry {
public:
    /// Precondition: the id is not empty and not yet added.
    void add(Environment environment);

    /// nullptr if unknown; valid as long as the registry.
    [[nodiscard]] const Environment* find(const QString& id) const;

    /// Every environment, in the order they were added.
    [[nodiscard]] QList<const Environment*> list() const;

private:
    // A deque keeps the environments where they are when others are added,
    // so that the pointers returned by find() stay valid.
    std::deque<Environment> m_environments;
};

}  // namespace tg::app
