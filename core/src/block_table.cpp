// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/block_table.hpp"

#include "tg/core/layer.hpp"

#include <algorithm>
#include <string_view>

namespace tg::core {

const BlockDefinition* BlockTable::find(std::string_view name) const {
    const auto found = std::ranges::find_if(m_definitions, [name](const BlockDefinition& block) {
        return sameTableName(block.name, name);
    });
    return found != m_definitions.end() ? &*found : nullptr;
}

}  // namespace tg::core
