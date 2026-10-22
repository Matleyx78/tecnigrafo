// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/raw_group.hpp"

namespace tg::core {

/// The DXF data of a handled entity that the model does not represent
/// (docs/adr/0018, points 4 and 5). Empty for entities made by commands.
///
/// The extras are part of the entity record: they leave and come back with
/// the entity, so undo and redo keep them.
struct EntityExtras {
    RawGroups beforeEntity;        ///< between handle and AcDbEntity: 102 {…}, 330
    RawGroups common;              ///< AcDbEntity groups not in the model (8, 62, 6, …)
    RawGroups specific;            ///< groups of the entity subclass not in the model
    RawGroups placement;           ///< 30, 31, 38, 39, 210, 220, 230 as read
    RawGroups extended;            ///< XDATA, from the first 1001 on
    bool flippedExtrusion{false};  ///< extrusion (0,0,-1): geometry is in WCS

    /// True when the entity carries nothing beyond the model, as an entity
    /// made by a command.
    [[nodiscard]] bool empty() const {
        return beforeEntity.empty() && common.empty() && specific.empty() && placement.empty() &&
               extended.empty() && !flippedExtrusion;
    }

    friend bool operator==(const EntityExtras&, const EntityExtras&) = default;
};

}  // namespace tg::core
