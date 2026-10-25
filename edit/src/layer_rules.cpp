// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/edit/layer_rules.hpp"

#include <algorithm>

namespace tg::edit {
namespace {

// XDATA starts at 1001 and ends with the entity, not at an arbitrary group.
// A group 8 within XDATA must not be treated as the entity's layer.
bool mentions(const core::RawGroups& groups, std::string_view name, bool layerGroups,
              bool otherGroups) {
    bool extended = false;
    for (const core::RawGroup& group : groups) {
        if (group.code == 0) {
            extended = false;
        } else if (group.code == 1001) {
            extended = true;
        }
        const bool layerGroup = layerGroups && group.code == 8 && !extended;
        if ((otherGroups != layerGroup) && core::sameTableName(group.value, name)) {
            return true;
        }
    }
    return false;
}

bool extrasMention(const core::EntityExtras& extras, std::string_view name) {
    return mentions(extras.beforeEntity, name, false, true) ||
           mentions(extras.common, name, false, true) ||
           mentions(extras.specific, name, false, true) ||
           mentions(extras.placement, name, false, true) ||
           mentions(extras.extended, name, false, true);
}

}  // namespace

LayerProblem checkNewLayer(const core::Document& doc, std::string_view name) {
    if (!core::isValidTableName(name)) {
        return LayerProblem::InvalidName;
    }
    return doc.findLayer(name) != nullptr ? LayerProblem::NameInUse : LayerProblem::None;
}

LayerProblem checkRename(const core::Document& doc, core::LayerId id, std::string_view newName) {
    if (doc.layer(id) == nullptr) {
        return LayerProblem::NotFound;
    }
    if (id == doc.layerZero()) {
        return LayerProblem::LayerZero;
    }
    if (!core::isValidTableName(newName)) {
        return LayerProblem::InvalidName;
    }
    const core::Layer* other = doc.findLayer(newName);
    return other != nullptr && other->id != id ? LayerProblem::NameInUse : LayerProblem::None;
}

LayerProblem checkRemove(const core::Document& doc, core::LayerId id) {
    const core::Layer* layer = doc.layer(id);
    if (layer == nullptr) {
        return LayerProblem::NotFound;
    }
    if (id == doc.layerZero()) {
        return LayerProblem::LayerZero;
    }
    if (id == doc.current().layer) {
        return LayerProblem::Current;
    }
    if (std::ranges::any_of(doc.entities(), [id](const core::EntityRecord& entity) {
            return entity.properties.layer == id;
        })) {
        return LayerProblem::InUse;
    }
    for (const core::RawEntity& entity : doc.rawEntities()) {
        if (mentions(entity.groups, layer->name, true, false)) {
            return LayerProblem::InUse;
        }
    }
    for (const core::RawSection& section : doc.raw().sections) {
        if (core::sameTableName(section.name, "BLOCKS") &&
            mentions(section.groups, layer->name, true, false)) {
            return LayerProblem::InUse;
        }
    }
    return LayerProblem::None;
}

LayerProblem checkFreeze(const core::Document& doc, core::LayerId id) {
    if (doc.layer(id) == nullptr) {
        return LayerProblem::NotFound;
    }
    return id == doc.current().layer ? LayerProblem::Current : LayerProblem::None;
}

bool namedInOtherUnhandledData(const core::Document& doc, core::LayerId id) {
    const core::Layer* layer = doc.layer(id);
    if (layer == nullptr) {
        return false;
    }
    const std::string_view name = layer->name;
    for (const core::RawSection& section : doc.raw().sections) {
        if (mentions(section.groups, name, core::sameTableName(section.name, "BLOCKS"), true)) {
            return true;
        }
    }
    for (const core::RawEntity& entity : doc.rawEntities()) {
        if (mentions(entity.groups, name, true, true)) {
            return true;
        }
    }
    for (const core::EntityRecord& entity : doc.entities()) {
        if (extrasMention(entity.extras, name)) {
            return true;
        }
    }
    for (const core::Layer& record : doc.layers()) {
        if (mentions(record.extras, name, false, true)) {
            return true;
        }
    }
    return std::ranges::any_of(doc.linetypes(), [name](const core::Linetype& record) {
        return mentions(record.extras, name, false, true);
    });
}

}  // namespace tg::edit
