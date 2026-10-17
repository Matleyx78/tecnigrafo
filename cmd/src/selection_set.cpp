// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/selection_set.hpp"

#include <algorithm>
#include <cassert>

namespace tg::cmd {

bool SelectionSet::add(core::EntityId id) {
    if (!m_members.insert(id.value).second) {
        return false;
    }
    m_ids.push_back(id);
    return true;
}

bool SelectionSet::remove(core::EntityId id) {
    if (m_members.erase(id.value) == 0) {
        return false;
    }
    const auto position = std::ranges::find(m_ids, id);
    assert(position != m_ids.end() && "m_members and m_ids disagree");
    m_ids.erase(position);
    return true;
}

void SelectionSet::clear() {
    m_ids.clear();
    m_members.clear();
}

bool SelectionSet::contains(core::EntityId id) const {
    return m_members.contains(id.value);
}

}  // namespace tg::cmd
