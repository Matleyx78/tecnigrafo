// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/editor.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ranges>
#include <utility>

namespace tg::edit {

bool Editor::validProperties(const core::Document& doc,
                             const core::EntityProperties& properties) {
    return doc.layer(properties.layer) != nullptr && doc.linetype(properties.linetype) != nullptr &&
           std::isfinite(properties.linetypeScale) && properties.linetypeScale > 0.0 &&
           (properties.lineweight == core::lineweight::kByLayer ||
            properties.lineweight == core::lineweight::kByBlock ||
            properties.lineweight == core::lineweight::kDefault ||
            core::isAllowedLineweight(properties.lineweight));
}

Editor::Editor(core::Document& document)
    : m_document(document), m_tables(*this), m_savedLwdisplay(document.lwdisplay()) {}

void Editor::beginTransaction(std::string name) {
    assert(!m_open && "a transaction is already open");
    m_open = Step{std::move(name), {}};
}

core::EntityId Editor::add(core::Entity entity) {
    assert(m_open && "add() needs an open transaction");
    const core::EntityId id = m_document.add(std::move(entity));
    Change change;
    change.id = id;
    m_open->changes.push_back(std::move(change));
    return id;
}

bool Editor::remove(core::EntityId id) {
    assert(m_open && "remove() needs an open transaction");
    std::optional<core::EntityRecord> removed = m_document.remove(id);
    if (!removed) {
        return false;
    }
    Change change;
    change.kind = Change::Kind::Removed;
    change.id = id;
    change.detached = std::move(removed);
    m_open->changes.push_back(std::move(change));
    return true;
}

bool Editor::replace(core::EntityId id, core::Entity entity) {
    assert(m_open && "replace() needs an open transaction");
    const core::EntityRecord* previous = m_document.find(id);
    if (previous == nullptr || previous->entity == entity) {
        return false;
    }
    const bool circleToArc = std::holds_alternative<core::Circle>(previous->entity) &&
                             std::holds_alternative<core::Arc>(entity);
    if (previous->entity.index() != entity.index() && !circleToArc) {
        return false;
    }
    Change change;
    change.kind = Change::Kind::Replaced;
    change.id = id;
    change.data = m_document.replaceEntity(id, std::move(entity));
    m_open->changes.push_back(std::move(change));
    return true;
}

std::optional<core::EntityId> Editor::addCopy(core::EntityId source, core::Entity entity) {
    assert(m_open && "addCopy() needs an open transaction");
    const core::EntityRecord* original = m_document.find(source);
    if (original == nullptr) {
        return std::nullopt;
    }
    // Copy before add() invalidates pointers into the document.
    core::EntityExtras extras = original->extras;
    const core::EntityProperties properties = original->properties;
    core::RawGroups retained;
    std::size_t depth = 0;
    for (const core::RawGroup& group : extras.beforeEntity) {
        if (group.code == 102 && group.value.starts_with('{')) {
            ++depth;
        } else if (depth > 0) {
            if (group.code == 102 && group.value == "}") {
                --depth;
            }
        } else {
            retained.push_back(group);
        }
    }
    extras.beforeEntity = std::move(retained);
    const core::EntityId id = add(std::move(entity));
    // restore() sets extras too; add() already recorded this entity for undo.
    auto record = m_document.remove(id);
    assert(record);
    record->extras = std::move(extras);
    record->properties = properties;
    m_document.restore(std::move(*record));
    return id;
}

Editor::Savepoint Editor::savepoint() const {
    assert(m_open && "savepoint() needs an open transaction");
    return Savepoint{m_open->changes.size()};
}

void Editor::rollbackTo(Savepoint mark) {
    assert(m_open && "rollbackTo() needs an open transaction");
    assert(mark.changes <= m_open->changes.size() && "invalid savepoint");
    while (m_open->changes.size() > mark.changes) {
        revert(m_open->changes.back());
        m_open->changes.pop_back();
    }
}

bool Editor::setProperties(core::EntityId id, const core::EntityProperties& properties) {
    assert(m_open && "setProperties() needs an open transaction");
    const core::EntityRecord* previous = m_document.find(id);
    if (previous == nullptr || previous->properties == properties ||
        !validProperties(m_document, properties)) {
        return false;
    }
    core::EntityRecord next = *previous;
    if (next.properties.color != properties.color) {
        constexpr int kColorBook = 430;  // DXF colour book no longer matches the chosen colour.
        std::erase_if(next.extras.common,
                      [](const core::RawGroup& group) { return group.code == kColorBook; });
    }
    next.properties = properties;
    Change change;
    change.kind = Change::Kind::Properties;
    change.data = std::move(next);
    swapUpdate(change);
    m_open->changes.push_back(std::move(change));
    return true;
}

bool Editor::isModified() const {
    return !m_savedPosition || *m_savedPosition != m_undo.size() ||
           (m_open && !m_open->changes.empty()) || m_document.lwdisplay() != m_savedLwdisplay;
}

void Editor::markSaved() {
    m_savedPosition = m_undo.size();
    m_savedLwdisplay = m_document.lwdisplay();
}

void Editor::replaceDocument(core::Document document) {
    assert(!m_open && "replaceDocument() requires no open transaction");
    m_document = std::move(document);
    m_undo.clear();
    m_redo.clear();
    m_savedPosition = 0;
    m_savedLwdisplay = m_document.lwdisplay();
    notify();
}

void Editor::commit() {
    assert(m_open && "commit() needs an open transaction");
    Step step = std::move(*m_open);
    m_open.reset();
    if (!step.changes.empty()) {
        if (m_savedPosition && *m_savedPosition > m_undo.size()) {
            m_savedPosition.reset();
        }
        m_undo.push_back(std::move(step));
        m_redo.clear();
    }
    notify();
}

void Editor::rollback() {
    assert(m_open && "rollback() needs an open transaction");
    for (Change& change : std::views::reverse(m_open->changes)) {
        revert(change);
    }
    m_open.reset();
    notify();
}

bool Editor::canUndo() const {
    return !m_open && !m_undo.empty();
}

bool Editor::canRedo() const {
    return !m_open && !m_redo.empty();
}

std::string Editor::undoName() const {
    return m_undo.empty() ? std::string{} : m_undo.back().name;
}

std::string Editor::redoName() const {
    return m_redo.empty() ? std::string{} : m_redo.back().name;
}

std::vector<std::string> Editor::undoNames() const {
    std::vector<std::string> names;
    names.reserve(m_undo.size());
    for (const Step& step : m_undo) {
        names.push_back(step.name);
    }
    return names;
}

std::vector<std::string> Editor::redoNames() const {
    std::vector<std::string> names;
    names.reserve(m_redo.size());
    for (const Step& step : m_redo) {
        names.push_back(step.name);
    }
    return names;
}

bool Editor::undo() {
    if (!canUndo()) {
        return false;
    }
    Step step = std::move(m_undo.back());
    m_undo.pop_back();
    for (Change& change : std::views::reverse(step.changes)) {
        revert(change);
    }
    m_redo.push_back(std::move(step));
    notify();
    return true;
}

bool Editor::redo() {
    if (!canRedo()) {
        return false;
    }
    Step step = std::move(m_redo.back());
    m_redo.pop_back();
    for (Change& change : step.changes) {
        apply(change);
    }
    m_undo.push_back(std::move(step));
    notify();
    return true;
}

void Editor::setChangeListener(Listener listener) {
    m_listener = std::move(listener);
}

// apply() and revert() run only in the order recorded by the transaction,
// so the entity they look for is always where the change left it.

void Editor::apply(Change& change) {
    switch (change.kind) {
        case Change::Kind::Added:
            assert(change.detached && "the added entity must be outside the document");
            m_document.restore(std::move(*change.detached));
            change.detached.reset();
            break;
        case Change::Kind::Removed:
            change.detached = m_document.remove(change.id);
            assert(change.detached && "the removed entity must be in the document");
            break;
        case Change::Kind::Units:
            m_document.setUnits(change.units);
            break;
        case Change::Kind::LayerAdded: {
            const auto& table = std::get<LayerTableChange>(change.data);
            m_document.insertLayer(table.record, table.position);
            break;
        }
        case Change::Kind::LayerRemoved: {
            const auto& table = std::get<LayerTableChange>(change.data);
            m_document.removeLayer(table.record.id);
            break;
        }
        case Change::Kind::LinetypeAdded: {
            const auto& table = std::get<LinetypeTableChange>(change.data);
            m_document.insertLinetype(table.record, table.position);
            break;
        }
        default:
            swapUpdate(change);
            break;
    }
}

void Editor::revert(Change& change) {
    switch (change.kind) {
        case Change::Kind::Added:
            change.detached = m_document.remove(change.id);
            assert(change.detached && "the added entity must be in the document");
            break;
        case Change::Kind::Removed:
            assert(change.detached && "the removed entity must be outside the document");
            m_document.restore(std::move(*change.detached));
            change.detached.reset();
            break;
        case Change::Kind::Units:
            m_document.setUnits(change.previousUnits);
            break;
        case Change::Kind::LayerAdded: {
            const auto& table = std::get<LayerTableChange>(change.data);
            m_document.removeLayer(table.record.id);
            break;
        }
        case Change::Kind::LayerRemoved: {
            const auto& table = std::get<LayerTableChange>(change.data);
            m_document.insertLayer(table.record, table.position);
            break;
        }
        case Change::Kind::LinetypeAdded: {
            const auto& table = std::get<LinetypeTableChange>(change.data);
            m_document.removeLinetype(table.record.id);
            break;
        }
        default:
            swapUpdate(change);
            break;
    }
}

void Editor::swapUpdate(Change& change) {
    switch (change.kind) {
        case Change::Kind::Replaced: {
            auto& entity = std::get<core::Entity>(change.data);
            entity = m_document.replaceEntity(change.id, std::move(entity));
            break;
        }
        case Change::Kind::LayerUpdated: {
            auto& update = std::get<LayerUpdate>(change.data);
            core::Layer previous = *m_document.layer(update.record.id);
            m_document.replaceLayer(update.record);
            update.record = std::move(previous);
            swapRawValues(update.rawValues);
            break;
        }
        case Change::Kind::Properties: {
            auto& record = std::get<core::EntityRecord>(change.data);
            auto previous = m_document.remove(record.id);
            assert(previous);
            m_document.restore(std::move(record));
            record = std::move(*previous);
            break;
        }
        case Change::Kind::Current: {
            auto& current = std::get<core::CurrentProperties>(change.data);
            const core::CurrentProperties previous = m_document.current();
            m_document.setCurrent(current);
            current = previous;
            break;
        }
        case Change::Kind::Ltscale: {
            auto& scale = std::get<double>(change.data);
            const double previous = m_document.ltscale();
            m_document.setLtscale(scale);
            scale = previous;
            break;
        }
        default:
            assert(false && "not a symmetric update");
            break;
    }
}

void Editor::swapRawValues(std::vector<RawValueChange>& values) {
    std::optional<core::RawDrawing> raw;
    for (RawValueChange& value : values) {
        if (value.entity) {
            const auto entities = m_document.rawEntities();
            const auto found = std::ranges::find(entities, *value.entity, &core::RawEntity::id);
            assert(found != entities.end());
            core::RawEntity entity = *found;
            std::swap(entity.groups[value.group].value, value.value);
            m_document.replaceRaw(entity);
        } else {
            if (!raw) {
                raw = m_document.raw();
            }
            std::swap(raw->sections[value.section].groups[value.group].value, value.value);
        }
    }
    if (raw) {
        m_document.setRaw(std::move(*raw));
    }
}

void Editor::notify() const {
    if (m_listener) {
        m_listener();
    }
}

}  // namespace tg::edit
