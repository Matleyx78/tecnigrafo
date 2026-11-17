// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/blocks.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/block_table.hpp"
#include "tg/edit/editor.hpp"

#include <algorithm>
#include <utility>

namespace tg::edit {

bool defineBlock(Editor& editor, std::string name, const core::Vec2& base,
                 std::span<const core::EntityId> ids) {
    core::BlockDefinition definition{std::move(name)};
    definition.base = base;
    definition.entities.reserve(ids.size());
    for (const core::EntityId id : ids) {
        if (const core::EntityRecord* record = editor.document().find(id)) {
            definition.entities.push_back(*record);
        }
    }
    definition.hasAttributes =
        std::ranges::any_of(definition.entities, [](const core::EntityRecord& record) {
            return std::holds_alternative<core::AttributeDefinition>(record.entity);
        });
    if (editor.document().blocks().wouldCycle(definition)) {
        return false;
    }

    for (const core::EntityId id : ids) {
        editor.remove(id);
    }
    // Removing first makes undo remove the definition before it restores the
    // same handles to model space.
    const std::string lookupName = definition.name;
    editor.tables().replaceBlock(lookupName, std::move(definition));
    return true;
}

}  // namespace tg::edit
