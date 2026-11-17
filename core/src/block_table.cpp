// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/block_table.hpp"

#include "tg/core/document.hpp"
#include "tg/core/layer.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace tg::core {

const BlockDefinition* BlockTable::find(std::string_view name) const {
    const auto found = std::ranges::find_if(m_definitions, [name](const BlockDefinition& block) {
        return sameTableName(block.name, name);
    });
    return found != m_definitions.end() ? &*found : nullptr;
}

bool BlockTable::wouldCycle(const BlockDefinition& definition) const {
    // A depth-first visit of the blocks that `definition` inserts, through the
    // table. The table itself has no cycle (Document::replaceBlock()), but
    // every block is visited once anyway: that also bounds the work.
    std::vector<const BlockDefinition*> pending{&definition};
    std::vector<const BlockDefinition*> visited;
    while (!pending.empty()) {
        const BlockDefinition* block = pending.back();
        pending.pop_back();
        for (const EntityRecord& record : block->entities) {
            const auto* insert = std::get_if<Insert>(&record.entity);
            if (insert == nullptr) {
                continue;
            }
            if (sameTableName(insert->block, definition.name)) {
                return true;
            }
            const BlockDefinition* inner = find(insert->block);
            if (inner != nullptr && std::ranges::find(visited, inner) == visited.end()) {
                visited.push_back(inner);
                pending.push_back(inner);
            }
        }
    }
    return false;
}

}  // namespace tg::core
