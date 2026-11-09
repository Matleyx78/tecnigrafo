// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/document_tables.hpp"

#include "tg/edit/editor.hpp"
#include "tg/edit/layer_rules.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <utility>

namespace tg::edit {
namespace {

bool validLayerProperties(const core::Document& doc, const core::Layer& layer) {
    return (layer.color.kind() == core::Color::Kind::Index ||
            layer.color.kind() == core::Color::Kind::Rgb) &&
           doc.linetype(layer.linetype) != nullptr && layer.linetype != doc.byLayerLinetype() &&
           layer.linetype != doc.byBlockLinetype() &&
           (layer.lineweight == core::lineweight::kDefault ||
            core::isAllowedLineweight(layer.lineweight));
}

}  // namespace

bool DocumentTables::setUnits(int units) {
    assert(m_editor.m_open && "setUnits() needs an open transaction");
    assert(units >= 0 && units <= core::Document::kMaxUnits && "invalid units");
    auto& document = m_editor.m_document;
    const int previous = document.units();
    if (previous == units) {
        return false;
    }
    Editor::Change change;
    change.kind = Editor::Change::Kind::Units;
    change.previousUnits = previous;
    change.units = units;
    m_editor.m_open->changes.push_back(std::move(change));
    document.setUnits(units);
    return true;
}

std::optional<core::LayerId> DocumentTables::addLayer(core::Layer layer) {
    assert(m_editor.m_open && "addLayer() needs an open transaction");
    auto& document = m_editor.m_document;
    if (checkNewLayer(document, layer.name) != LayerProblem::None ||
        !validLayerProperties(document, layer)) {
        return std::nullopt;
    }
    // New records always get a fresh handle, never a caller's stale identifier.
    layer.id = {};
    const std::size_t position = document.layers().size();
    const core::LayerId id = document.addLayer(std::move(layer));
    Editor::Change change;
    change.kind = Editor::Change::Kind::LayerAdded;
    change.data = Editor::LayerTableChange{*document.layer(id), position};
    m_editor.m_open->changes.push_back(std::move(change));
    return id;
}

bool DocumentTables::renameLayer(core::LayerId id, std::string name) {
    assert(m_editor.m_open && "renameLayer() needs an open transaction");
    auto& document = m_editor.m_document;
    if (checkRename(document, id, name) != LayerProblem::None) {
        return false;
    }
    core::Layer renamed = *document.layer(id);
    if (renamed.name == name) {
        return false;
    }
    Editor::LayerUpdate update{renamed, {}};
    // Record the original spellings too: undo must not normalize raw data.
    const auto collect = [&](const core::RawGroups& groups, std::optional<core::EntityId> entity,
                             std::size_t section) {
        bool extended = false;
        for (std::size_t i = 0; i < groups.size(); ++i) {
            const core::RawGroup& group = groups[i];
            if (group.code == 0) {
                extended = false;
            } else if (group.code == 1001) {
                extended = true;
            }
            if (group.code == 8 && !extended && core::sameTableName(group.value, renamed.name)) {
                update.rawValues.push_back(Editor::RawValueChange{entity, section, i, name});
            }
        }
    };
    for (const core::RawEntity& entity : document.rawEntities()) {
        collect(entity.groups, entity.id, 0);
    }
    const auto& sections = document.raw().sections;
    for (std::size_t i = 0; i < sections.size(); ++i) {
        if (core::sameTableName(sections[i].name, "BLOCKS")) {
            collect(sections[i].groups, std::nullopt, i);
        }
    }
    renamed.name = std::move(name);
    update.record = std::move(renamed);
    Editor::Change change;
    change.kind = Editor::Change::Kind::LayerUpdated;
    change.data = std::move(update);
    m_editor.swapUpdate(change);
    m_editor.m_open->changes.push_back(std::move(change));
    return true;
}

bool DocumentTables::removeLayer(core::LayerId id) {
    assert(m_editor.m_open && "removeLayer() needs an open transaction");
    auto& document = m_editor.m_document;
    if (checkRemove(document, id) != LayerProblem::None) {
        return false;
    }
    auto removed = document.removeLayer(id);
    assert(removed && "the checked layer must exist");
    Editor::Change change;
    change.kind = Editor::Change::Kind::LayerRemoved;
    change.data = Editor::LayerTableChange{std::move(removed->first), removed->second};
    m_editor.m_open->changes.push_back(std::move(change));
    return true;
}

bool DocumentTables::changeLayer(const core::Layer& layer) {
    assert(m_editor.m_open && "changeLayer() needs an open transaction");
    auto& document = m_editor.m_document;
    const core::Layer* previous = document.layer(layer.id);
    if (previous == nullptr || !validLayerProperties(document, layer) ||
        (layer.frozen && checkFreeze(document, layer.id) != LayerProblem::None)) {
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
    Editor::Change change;
    change.kind = Editor::Change::Kind::LayerUpdated;
    change.data = Editor::LayerUpdate{std::move(next), {}};
    m_editor.swapUpdate(change);
    m_editor.m_open->changes.push_back(std::move(change));
    return true;
}

std::optional<core::LinetypeId> DocumentTables::addLinetype(core::Linetype linetype) {
    assert(m_editor.m_open && "addLinetype() needs an open transaction");
    auto& document = m_editor.m_document;
    if (!core::isValidTableName(linetype.name) || document.findLinetype(linetype.name) != nullptr) {
        return std::nullopt;
    }
    linetype.id = {};
    const std::size_t position = document.linetypes().size();
    const core::LinetypeId id = document.addLinetype(std::move(linetype));
    Editor::Change change;
    change.kind = Editor::Change::Kind::LinetypeAdded;
    change.data = Editor::LinetypeTableChange{*document.linetype(id), position};
    m_editor.m_open->changes.push_back(std::move(change));
    return id;
}

bool DocumentTables::setCurrent(const core::CurrentProperties& current) {
    assert(m_editor.m_open && "setCurrent() needs an open transaction");
    auto& document = m_editor.m_document;
    const core::Layer* layer = document.layer(current.layer);
    if (layer == nullptr || layer->frozen || current == document.current() ||
        !Editor::validProperties(document, current)) {
        return false;
    }
    Editor::Change change;
    change.kind = Editor::Change::Kind::Current;
    change.data = current;
    m_editor.swapUpdate(change);
    m_editor.m_open->changes.push_back(std::move(change));
    return true;
}

bool DocumentTables::setLtscale(double scale) {
    assert(m_editor.m_open && "setLtscale() needs an open transaction");
    auto& document = m_editor.m_document;
    if (!std::isfinite(scale) || scale <= 0.0 || scale == document.ltscale()) {
        return false;
    }
    Editor::Change change;
    change.kind = Editor::Change::Kind::Ltscale;
    change.data = scale;
    m_editor.swapUpdate(change);
    m_editor.m_open->changes.push_back(std::move(change));
    return true;
}

void DocumentTables::setLwdisplay(bool on) {
    auto& document = m_editor.m_document;
    if (on != document.lwdisplay()) {
        document.setLwdisplay(on);
        m_editor.notify();
    }
}

}  // namespace tg::edit
