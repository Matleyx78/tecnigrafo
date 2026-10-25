// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/editor.hpp"

#include "tg/edit/layer_rules.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ranges>
#include <utility>

namespace tg::edit {
namespace {

bool validProperties(const core::Document& doc, const core::EntityProperties& properties) {
    return doc.layer(properties.layer) != nullptr && doc.linetype(properties.linetype) != nullptr &&
           std::isfinite(properties.linetypeScale) && properties.linetypeScale > 0.0 &&
           (properties.lineweight == core::lineweight::kByLayer ||
            properties.lineweight == core::lineweight::kByBlock ||
            properties.lineweight == core::lineweight::kDefault ||
            core::isAllowedLineweight(properties.lineweight));
}

bool validLayerProperties(const core::Document& doc, const core::Layer& layer) {
    return (layer.color.kind() == core::Color::Kind::Index ||
            layer.color.kind() == core::Color::Kind::Rgb) &&
           doc.linetype(layer.linetype) != nullptr &&
           layer.linetype != doc.byLayerLinetype() && layer.linetype != doc.byBlockLinetype() &&
           (layer.lineweight == core::lineweight::kDefault ||
            core::isAllowedLineweight(layer.lineweight));
}

}  // namespace

Editor::Editor(core::Document& document)
    : m_document(document), m_savedLwdisplay(document.lwdisplay()) {}

void Editor::beginTransaction(std::string name) {
    assert(!m_open && "a transaction is already open");
    m_open = Step{std::move(name), {}};
}

core::EntityId Editor::add(core::Entity entity) {
    assert(m_open && "add() needs an open transaction");
    const core::EntityId id = m_document.add(std::move(entity));
    m_open->changes.push_back(Change{Change::Kind::Added, id, std::nullopt});
    return id;
}

bool Editor::remove(core::EntityId id) {
    assert(m_open && "remove() needs an open transaction");
    std::optional<core::EntityRecord> removed = m_document.remove(id);
    if (!removed) {
        return false;
    }
    m_open->changes.push_back(Change{Change::Kind::Removed, id, std::move(removed)});
    return true;
}

bool Editor::setUnits(int units) {
    assert(m_open && "setUnits() needs an open transaction");
    assert(units >= 0 && units <= core::Document::kMaxUnits && "invalid units");
    const int previous = m_document.units();
    if (previous == units) {
        return false;
    }
    m_open->changes.push_back(Change{Change::Kind::Units, {}, std::nullopt, previous, units});
    m_document.setUnits(units);
    return true;
}

std::optional<core::LayerId> Editor::addLayer(core::Layer layer) {
    assert(m_open && "addLayer() needs an open transaction");
    if (checkNewLayer(m_document, layer.name) != LayerProblem::None ||
        !validLayerProperties(m_document, layer)) {
        return std::nullopt;
    }
    // New records always get a fresh handle, never a caller's stale identifier.
    layer.id = {};
    const std::size_t position = m_document.layers().size();
    const core::LayerId id = m_document.addLayer(std::move(layer));
    Change change;
    change.kind = Change::Kind::LayerAdded;
    change.data = LayerTableChange{*m_document.layer(id), position};
    m_open->changes.push_back(std::move(change));
    return id;
}

bool Editor::renameLayer(core::LayerId id, std::string name) {
    assert(m_open && "renameLayer() needs an open transaction");
    if (checkRename(m_document, id, name) != LayerProblem::None) {
        return false;
    }
    core::Layer renamed = *m_document.layer(id);
    if (renamed.name == name) {
        return false;
    }
    LayerUpdate update{renamed, {}};
    // Record the original spellings too: undo must not normalize raw data.
    const auto collect = [&](const core::RawGroups& groups,
                             std::optional<core::EntityId> entity, std::size_t section) {
        bool extended = false;
        for (std::size_t i = 0; i < groups.size(); ++i) {
            const core::RawGroup& group = groups[i];
            if (group.code == 0) {
                extended = false;
            } else if (group.code == 1001) {
                extended = true;
            }
            if (group.code == 8 && !extended && core::sameTableName(group.value, renamed.name)) {
                update.rawValues.push_back(RawValueChange{entity, section, i, name});
            }
        }
    };
    for (const core::RawEntity& entity : m_document.rawEntities()) {
        collect(entity.groups, entity.id, 0);
    }
    const auto& sections = m_document.raw().sections;
    for (std::size_t i = 0; i < sections.size(); ++i) {
        if (core::sameTableName(sections[i].name, "BLOCKS")) {
            collect(sections[i].groups, std::nullopt, i);
        }
    }
    renamed.name = std::move(name);
    update.record = std::move(renamed);
    Change change;
    change.kind = Change::Kind::LayerUpdated;
    change.data = std::move(update);
    swapUpdate(change);
    m_open->changes.push_back(std::move(change));
    return true;
}

bool Editor::removeLayer(core::LayerId id) {
    assert(m_open && "removeLayer() needs an open transaction");
    if (checkRemove(m_document, id) != LayerProblem::None) {
        return false;
    }
    auto removed = m_document.removeLayer(id);
    Change change;
    change.kind = Change::Kind::LayerRemoved;
    change.data = LayerTableChange{std::move(removed->first), removed->second};
    m_open->changes.push_back(std::move(change));
    return true;
}

bool Editor::changeLayer(const core::Layer& layer) {
    assert(m_open && "changeLayer() needs an open transaction");
    const core::Layer* previous = m_document.layer(layer.id);
    if (previous == nullptr || !validLayerProperties(m_document, layer) ||
        (layer.frozen && checkFreeze(m_document, layer.id) != LayerProblem::None)) {
        return false;
    }
    core::Layer next = *previous;
    next.color = layer.color;
    next.linetype = layer.linetype;
    next.lineweight = layer.lineweight;
    next.on = layer.on;
    next.frozen = layer.frozen;
    next.locked = layer.locked;
    next.plot = layer.plot;
    if (next == *previous) {
        return false;
    }
    Change change;
    change.kind = Change::Kind::LayerUpdated;
    change.data = LayerUpdate{std::move(next), {}};
    swapUpdate(change);
    m_open->changes.push_back(std::move(change));
    return true;
}

std::optional<core::LinetypeId> Editor::addLinetype(core::Linetype linetype) {
    assert(m_open && "addLinetype() needs an open transaction");
    if (!core::isValidTableName(linetype.name) ||
        m_document.findLinetype(linetype.name) != nullptr) {
        return std::nullopt;
    }
    linetype.id = {};
    const std::size_t position = m_document.linetypes().size();
    const core::LinetypeId id = m_document.addLinetype(std::move(linetype));
    Change change;
    change.kind = Change::Kind::LinetypeAdded;
    change.data = LinetypeTableChange{*m_document.linetype(id), position};
    m_open->changes.push_back(std::move(change));
    return id;
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

bool Editor::setCurrent(const core::CurrentProperties& current) {
    assert(m_open && "setCurrent() needs an open transaction");
    const core::Layer* layer = m_document.layer(current.layer);
    if (layer == nullptr || layer->frozen || current == m_document.current() ||
        !validProperties(m_document, current)) {
        return false;
    }
    Change change;
    change.kind = Change::Kind::Current;
    change.data = current;
    swapUpdate(change);
    m_open->changes.push_back(std::move(change));
    return true;
}

bool Editor::setLtscale(double scale) {
    assert(m_open && "setLtscale() needs an open transaction");
    if (!std::isfinite(scale) || scale <= 0.0 || scale == m_document.ltscale()) {
        return false;
    }
    Change change;
    change.kind = Change::Kind::Ltscale;
    change.data = scale;
    swapUpdate(change);
    m_open->changes.push_back(std::move(change));
    return true;
}

void Editor::setLwdisplay(bool on) {
    if (on != m_document.lwdisplay()) {
        m_document.setLwdisplay(on);
        notify();
    }
}

bool Editor::isModified() const {
    return !m_savedPosition || *m_savedPosition != m_undo.size() ||
           (m_open && !m_open->changes.empty()) || m_document.lwdisplay() != m_savedLwdisplay;
}

void Editor::markSaved(std::string name) {
    m_fileName = std::move(name);
    m_hasFile = true;
    m_savedPosition = m_undo.size();
    m_savedLwdisplay = m_document.lwdisplay();
}

void Editor::replaceDocument(core::Document document, std::string name) {
    assert(!m_open && "replaceDocument() requires no open transaction");
    m_document = std::move(document);
    m_undo.clear();
    m_redo.clear();
    m_savedPosition = 0;
    m_savedLwdisplay = m_document.lwdisplay();
    m_hasFile = !name.empty();
    m_fileName = m_hasFile ? std::move(name) : "Drawing1.cadx";
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
