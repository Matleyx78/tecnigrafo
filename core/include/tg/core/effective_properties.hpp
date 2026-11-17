// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

namespace tg::core {

/// What the drawing shows of an entity (docs/adr/0023-0025).
struct EffectiveProperties {
    Rgb color;
    bool colorIsSeven{false};           ///< index 7 follows the renderer's theme
    const Linetype* linetype{nullptr};  ///< never null
    double linetypeScale{1.0};
    int lineweight{Document::kDefaultLwdefault};  ///< hundredths of a millimetre
    bool visible{true};
    bool locked{false};
};

/// Resolves ByLayer, ByBlock and Default. A document without tables uses
/// the new-drawing defaults; the returned linetype then has static lifetime.
[[nodiscard]] EffectiveProperties effectiveProperties(const Document& document,
                                                      const EntityRecord& record);

/// effectiveProperties() of an entity with `properties`: for an entity of a
/// block, those that propertiesInBlock() gives. ByBlock that is still there
/// is shown as outside a block: colour 7, Continuous, Default.
[[nodiscard]] EffectiveProperties effectiveProperties(const Document& document,
                                                      const EntityProperties& properties);

/// True when the entity's layer is on and thawed. An absent layer is visible.
[[nodiscard]] bool isVisible(const Document& document, const EntityRecord& record);

/// The properties with which an entity of a block definition is shown in an
/// insert (docs/adr/0045, point 3). `insert` are those of the insert as it is
/// shown: the properties of its record, or, for an insert among the entities
/// of another block, what this function gave it, so that it nests. The layer
/// 0 becomes the layer of the insert, and colour, linetype and lineweight
/// ByBlock become those of the insert; ByLayer stays, and then follows the
/// layer the entity is shown on. The rest is the entity's.
[[nodiscard]] EntityProperties propertiesInBlock(const Document& document,
                                                 const EntityProperties& insert,
                                                 const EntityProperties& entity);

/// The state of the layers an entity is shown through: its own, and inside
/// a block those of every insert that contains it (docs/adr/0045, point 3).
struct LayerState {
    bool on{true};       ///< every one is on: the entity is drawn
    bool frozen{false};  ///< one is frozen: the entity is not generated at all

    friend bool operator==(const LayerState&, const LayerState&) = default;
};

/// `outer` combined with the layer of `properties`, as shown (the result of
/// propertiesInBlock() inside a block): off when either is off, frozen when
/// either is frozen. An absent layer is on and thawed.
[[nodiscard]] LayerState layerState(const Document& document, const EntityProperties& properties,
                                    const LayerState& outer = {});

}  // namespace tg::core
