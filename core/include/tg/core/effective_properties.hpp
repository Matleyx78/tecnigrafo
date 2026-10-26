// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

namespace tg::core {

/// What the drawing shows of an entity (docs/adr/0023-0025).
struct EffectiveProperties {
    Rgb color;
    bool colorIsSeven{false};  ///< index 7 follows the renderer's theme
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

/// True when the entity's layer is on and thawed. An absent layer is visible.
[[nodiscard]] bool isVisible(const Document& document, const EntityRecord& record);

}  // namespace tg::core
