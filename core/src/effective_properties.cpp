// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/effective_properties.hpp"

namespace tg::core {
namespace {
// Table-free documents are used by geometry clients and legacy file readers.
const Linetype kContinuous{{}, "Continuous", "Solid line", {}, {}};
constexpr int kThemeColorIndex = 7;
}  // namespace

bool isVisible(const Document& document, const EntityRecord& record) {
    const Layer* layer = document.layer(record.properties.layer);
    return layer == nullptr || (layer->on && !layer->frozen);
}

EffectiveProperties effectiveProperties(const Document& document, const EntityRecord& record) {
    const auto& properties = record.properties;
    const Layer* layer = document.layer(properties.layer);
    Color color = properties.color;
    if (color.kind() == Color::Kind::ByLayer) {
        color = layer != nullptr ? layer->color : Color::fromIndex(kThemeColorIndex);
    }
    if (color.kind() == Color::Kind::ByBlock) {
        color = Color::fromIndex(kThemeColorIndex);
    }
    EffectiveProperties result;
    result.colorIsSeven = color.kind() == Color::Kind::Index && color.index() == kThemeColorIndex;
    result.color = color.kind() == Color::Kind::Rgb ? color.rgb() : aciToRgb(color.index());

    LinetypeId type = properties.linetype;
    if (type == document.byLayerLinetype()) {
        type = layer != nullptr ? layer->linetype : document.continuousLinetype();
    }
    if (type == document.byBlockLinetype()) {
        type = document.continuousLinetype();
    }
    result.linetype = document.linetype(type);
    if (result.linetype == nullptr) {
        result.linetype = &kContinuous;
    }
    result.linetypeScale = document.ltscale() * properties.linetypeScale;
    int weight = properties.lineweight;
    if (weight == lineweight::kByLayer) {
        weight = layer != nullptr ? layer->lineweight : lineweight::kDefault;
    }
    result.lineweight = weight == lineweight::kByBlock || weight == lineweight::kDefault
                            ? document.lwdefault()
                            : weight;
    result.visible = isVisible(document, record);
    result.locked = layer != nullptr && layer->locked;
    return result;
}

}  // namespace tg::core
