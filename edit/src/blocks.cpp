// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/blocks.hpp"

#include "tg/core/attribute_definition.hpp"
#include "tg/core/block_table.hpp"
#include "tg/core/document.hpp"
#include "tg/edit/editor.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace tg::edit {

namespace {

bool containsAttributeDefinition(const core::BlockDefinition& definition) {
    return std::ranges::any_of(definition.entities, [](const core::EntityRecord& record) {
        return std::holds_alternative<core::AttributeDefinition>(record.entity);
    });
}

/// The positions of the ATTDEFs among the entities of `definition`, in order.
std::vector<std::size_t> attributeDefinitionPositions(const core::BlockDefinition& definition) {
    std::vector<std::size_t> positions;
    for (std::size_t i = 0; i < definition.entities.size(); ++i) {
        if (std::holds_alternative<core::AttributeDefinition>(definition.entities[i].entity)) {
            positions.push_back(i);
        }
    }
    return positions;
}

const core::AttributeDefinition& attributeDefinitionAt(const core::BlockDefinition& definition,
                                                       std::size_t position) {
    return std::get<core::AttributeDefinition>(definition.entities[position].entity);
}

/// The position of the first ATTDEF of `definition` with `tag`.
std::optional<std::size_t> findAttributeDefinition(const core::BlockDefinition& definition,
                                                   std::string_view tag) {
    for (const std::size_t position : attributeDefinitionPositions(definition)) {
        if (core::sameTableName(attributeDefinitionAt(definition, position).tag, tag)) {
            return position;
        }
    }
    return std::nullopt;
}

/// Puts `definition` in place of the block it names, in the open transaction.
void putDefinition(Editor& editor, core::BlockDefinition definition) {
    definition.hasAttributes = containsAttributeDefinition(definition);
    const std::string name = definition.name;
    editor.tables().replaceBlock(name, std::move(definition));
}

}  // namespace

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
    definition.hasAttributes = containsAttributeDefinition(definition);
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
    definition.hasAttributes = containsAttributeDefinition(definition);
    if (document.blocks().wouldCycle(definition)) {
        editor.rollbackTo(mark);
        return false;
    }
    editor.tables().replaceBlock(name, std::move(definition));
    document.setNextId(import.nextId);
    return true;
}

bool reorderAttributeDefinitions(Editor& editor, std::string_view block,
                                 std::span<const std::string> tags) {
    assert(editor.inTransaction() && "reorderAttributeDefinitions() needs an open transaction");
    const core::BlockDefinition* found = editor.document().blocks().find(block);
    if (found == nullptr) {
        return false;
    }
    core::BlockDefinition definition = *found;
    const std::vector<std::size_t> positions = attributeDefinitionPositions(definition);
    if (tags.size() != positions.size()) {
        return false;
    }
    // The i-th tag takes the first ATTDEF with it not yet taken.
    std::vector<bool> taken(positions.size(), false);
    std::vector<core::EntityRecord> reordered;
    reordered.reserve(positions.size());
    for (const std::string& tag : tags) {
        std::size_t k = 0;
        while (k < positions.size() &&
               (taken[k] ||
                !core::sameTableName(attributeDefinitionAt(definition, positions[k]).tag, tag))) {
            ++k;
        }
        if (k == positions.size()) {
            return false;
        }
        taken[k] = true;
        reordered.push_back(definition.entities[positions[k]]);
    }
    // Each ATTDEF takes the place, and so the order key, of the one it
    // replaces: the entities stay in key order, merged with the raw ones.
    for (std::size_t i = 0; i < positions.size(); ++i) {
        core::EntityRecord& slot = definition.entities[positions[i]];
        const std::uint64_t order = slot.order;
        slot = std::move(reordered[i]);
        slot.order = order;
    }
    putDefinition(editor, std::move(definition));
    return true;
}

bool replaceAttributeDefinition(Editor& editor, std::string_view block, std::string_view tag,
                                core::AttributeDefinition definition) {
    assert(editor.inTransaction() && "replaceAttributeDefinition() needs an open transaction");
    const core::BlockDefinition* found = editor.document().blocks().find(block);
    if (found == nullptr) {
        return false;
    }
    const std::optional<std::size_t> position = findAttributeDefinition(*found, tag);
    if (!position) {
        return false;
    }
    for (const std::size_t other : attributeDefinitionPositions(*found)) {
        if (other != *position &&
            core::sameTableName(attributeDefinitionAt(*found, other).tag, definition.tag)) {
            return false;
        }
    }
    core::BlockDefinition changed = *found;
    changed.entities[*position].entity = std::move(definition);
    putDefinition(editor, std::move(changed));
    return true;
}

bool removeAttributeDefinition(Editor& editor, std::string_view block, std::string_view tag) {
    assert(editor.inTransaction() && "removeAttributeDefinition() needs an open transaction");
    const core::BlockDefinition* found = editor.document().blocks().find(block);
    if (found == nullptr) {
        return false;
    }
    const std::optional<std::size_t> position = findAttributeDefinition(*found, tag);
    if (!position) {
        return false;
    }
    core::BlockDefinition changed = *found;
    changed.entities.erase(changed.entities.begin() + static_cast<std::ptrdiff_t>(*position));
    putDefinition(editor, std::move(changed));
    return true;
}

std::size_t synchronizeInserts(Editor& editor, std::string_view block) {
    assert(editor.m_open && "synchronizeInserts() needs an open transaction");
    core::Document& document = editor.m_document;
    if (document.blocks().find(block) == nullptr) {
        return 0;
    }

    // An INSERT record to change: in the drawing (no owner) or at `position`
    // among the entities of the definition `owner`.
    struct Target {
        core::EntityId id;
        std::optional<std::string> owner;
        std::size_t position{0};
        std::vector<core::Attribute> attributes;
    };

    std::vector<Target> targets;
    const auto consider = [&](const core::EntityRecord& record, std::optional<std::string> owner,
                              std::size_t position) {
        const auto* insert = std::get_if<core::Insert>(&record.entity);
        if (insert == nullptr || !core::sameTableName(insert->block, block)) {
            return;
        }
        std::vector<core::Attribute> attributes = core::synchronizedAttributes(document, *insert);
        if (attributes != insert->attributes) {
            targets.push_back({record.id, std::move(owner), position, std::move(attributes)});
        }
    };
    for (const core::EntityRecord& record : document.entities()) {
        consider(record, std::nullopt, 0);
    }
    for (const core::BlockDefinition& definition : document.blocks().definitions()) {
        for (std::size_t i = 0; i < definition.entities.size(); ++i) {
            consider(definition.entities[i], definition.name, i);
        }
    }
    std::ranges::sort(targets, {}, [](const Target& target) { return target.id.value; });

    // The new ATTRIBs take new handles, which add() never gives again, even
    // after undo; those inside a definition stay reserved by it.
    for (Target& target : targets) {
        for (core::Attribute& attribute : target.attributes) {
            if (attribute.id.value == 0) {
                attribute.id = core::EntityId{document.nextId()};
                document.reserveHandle(attribute.id);
            }
        }
    }

    std::vector<core::BlockDefinition> changedDefinitions;
    for (Target& target : targets) {
        if (!target.owner) {
            core::Insert insert = std::get<core::Insert>(document.find(target.id)->entity);
            insert.attributes = std::move(target.attributes);
            const bool replaced = editor.replace(target.id, std::move(insert));
            assert(replaced);
            static_cast<void>(replaced);
            continue;
        }
        auto owner =
            std::ranges::find(changedDefinitions, *target.owner, &core::BlockDefinition::name);
        if (owner == changedDefinitions.end()) {
            changedDefinitions.push_back(*document.blocks().find(*target.owner));
            owner = changedDefinitions.end() - 1;
        }
        std::get<core::Insert>(owner->entities[target.position].entity).attributes =
            std::move(target.attributes);
    }
    for (core::BlockDefinition& definition : changedDefinitions) {
        const std::string name = definition.name;
        editor.tables().replaceBlock(name, std::move(definition));
    }
    return targets.size();
}

}  // namespace tg::edit
