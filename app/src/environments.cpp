// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "environments.hpp"

#include <cassert>
#include <utility>

namespace tg::app {

void EnvironmentRegistry::add(Environment environment) {
    assert(!environment.id.isEmpty() && "environment without id");
    assert(find(environment.id) == nullptr && "environment id already registered");
    m_environments.push_back(std::move(environment));
}

const Environment* EnvironmentRegistry::find(const QString& id) const {
    for (const Environment& environment : m_environments) {
        if (environment.id == id) {
            return &environment;
        }
    }
    return nullptr;
}

QList<const Environment*> EnvironmentRegistry::list() const {
    QList<const Environment*> result;
    result.reserve(static_cast<qsizetype>(m_environments.size()));
    for (const Environment& environment : m_environments) {
        result.append(&environment);
    }
    return result;
}

}  // namespace tg::app
