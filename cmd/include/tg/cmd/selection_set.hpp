// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/entity_id.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_set>
#include <vector>

namespace tg::cmd {

/// Entities chosen by the user, in the order they were chosen, each once
/// (docs/adr/0008). It is not part of the document: it is never undone nor
/// saved.
///
/// add() and contains() take constant time; remove() is linear in the size,
/// because it keeps the order of the others.
class SelectionSet {
public:
    /// Appends `id`; false if it is already present.
    bool add(core::EntityId id);

    /// Removes `id`, keeping the order of the others; false if it is absent.
    bool remove(core::EntityId id);

    void clear();

    [[nodiscard]] bool contains(core::EntityId id) const;

    /// In the order they were added. Valid until the set is next modified.
    [[nodiscard]] std::span<const core::EntityId> ids() const { return m_ids; }

    [[nodiscard]] std::size_t size() const { return m_ids.size(); }

    [[nodiscard]] bool isEmpty() const { return m_ids.empty(); }

private:
    std::vector<core::EntityId> m_ids;
    std::unordered_set<std::uint64_t> m_members;  ///< the values of m_ids
};

}  // namespace tg::cmd
