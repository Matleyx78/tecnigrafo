// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/blocks.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/block_table.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"

#include <algorithm>
#include <cassert>
#include <optional>
#include <utility>

namespace tg::edit {

bool defineBlock(Editor& editor, std::string name, const core::Vec2& base,
                 std::span<const core::EntityId> ids) {
    core::BlockDefinition definition{std::move(name)};
    if (const core::BlockDefinition* existing = editor.document().blocks().find(definition.name)) {
        definition = *existing;
    }
    definition.base = base;
    definition.entities.clear();
    definition.rawEntities.clear();
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

core::EntityId addInsert(Editor& editor, core::Insert insert, core::RawGroups specific) {
    insert.attributes = core::defaultAttributes(editor.document(), insert);
    const bool hasAttributes = !insert.attributes.empty();
    const core::EntityId id = editor.add(std::move(insert));
    if (hasAttributes || !specific.empty()) {
        // The attributes take the handles after the insert's, as AutoCAD
        // writes them. The added change records only the identifier, so the
        // record can be taken out and put back completed, as addCopy() does.
        core::Document& document = editor.m_document;
        std::optional<core::EntityRecord> record = document.remove(id);
        assert(record);
        for (core::Attribute& attribute : std::get<core::Insert>(record->entity).attributes) {
            attribute.id = core::EntityId{document.nextId()};
            document.reserveHandle(attribute.id);
        }
        record->extras.specific = std::move(specific);
        document.restore(std::move(*record));
    }
    return id;
}

bool applyImport(Editor& editor, BlockImport import, bool redefine) {
    assert(editor.m_open && "applyImport() needs an open transaction");
    core::Document& document = editor.m_document;
    const core::BlockDefinition* existing = document.blocks().find(import.definition.name);
    if (existing != nullptr && !redefine) {
        return false;
    }
    const Editor::Savepoint mark = editor.savepoint();

    // Linetypes before layers, which name them. The records keep the handles
    // the import gave them, which the entities and the other records use.
    for (core::Linetype& linetype : import.linetypes) {
        if (document.findLinetype(linetype.name) != nullptr) {
            continue;
        }
        const std::size_t position = document.linetypes().size();
        const core::LinetypeId id = document.addLinetype(std::move(linetype));
        Editor::Change change;
        change.kind = Editor::Change::Kind::LinetypeAdded;
        change.data = Editor::LinetypeTableChange{*document.linetype(id), position};
        editor.m_open->changes.push_back(std::move(change));
    }
    for (core::Layer& layer : import.layers) {
        if (document.findLayer(layer.name) != nullptr) {
            continue;
        }
        const std::size_t position = document.layers().size();
        const core::LayerId id = document.addLayer(std::move(layer));
        Editor::Change change;
        change.kind = Editor::Change::Kind::LayerAdded;
        change.data = Editor::LayerTableChange{*document.layer(id), position};
        editor.m_open->changes.push_back(std::move(change));
    }
    if (!import.textStyles.empty()) {
        core::TextStyleTable table = document.textStyles();
        for (core::TextStyle& style : import.textStyles) {
            if (table.find(style.name) == nullptr) {
                table.put(std::move(style), document);
            }
        }
        editor.tables().replaceTextStyles(std::move(table));
    }
    if (!import.dimStyles.empty()) {
        core::DimStyleTable table = document.dimStyles();
        for (core::DimStyle& style : import.dimStyles) {
            if (table.find(style.name) == nullptr) {
                table.put(std::move(style), document);
            }
        }
        editor.tables().replaceDimStyles(std::move(table));
    }

    const std::string name = import.definition.name;
    for (core::BlockDefinition& block : import.nested) {
        if (document.blocks().find(block.name) != nullptr ||
            core::sameTableName(block.name, name)) {
            continue;
        }
        if (document.blocks().wouldCycle(block)) {
            editor.rollbackTo(mark);
            return false;
        }
        const std::string nestedName = block.name;
        editor.tables().replaceBlock(nestedName, std::move(block));
    }

    core::BlockDefinition definition = std::move(import.definition);
    // `existing` may dangle after the nested blocks entered the table.
    existing = document.blocks().find(name);
    if (existing != nullptr) {
        // The records of the block stay those of the drawing, as defineBlock().
        definition.recordId = existing->recordId;
        definition.blockId = existing->blockId;
        definition.endBlockId = existing->endBlockId;
        definition.blockExtras = existing->blockExtras;
        definition.endBlockExtras = existing->endBlockExtras;
        definition.recordExtras = existing->recordExtras;
    }
    definition.hasAttributes =
        std::ranges::any_of(definition.entities, [](const core::EntityRecord& record) {
            return std::holds_alternative<core::AttributeDefinition>(record.entity);
        });
    if (document.blocks().wouldCycle(definition)) {
        editor.rollbackTo(mark);
        return false;
    }
    editor.tables().replaceBlock(name, std::move(definition));
    document.setNextId(import.nextId);
    return true;
}

}  // namespace tg::edit
