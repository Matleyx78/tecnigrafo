// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#pragma once

#include "tg/core/document.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace tg::cmd {

struct QuickSelectCriteria {
    enum class Property : std::uint8_t { None, Layer, Color, Linetype, Lineweight };
    enum class Operator : std::uint8_t { Equal, NotEqual };

    std::optional<std::string> type;  ///< DXF type; absent means every type
    Property property{Property::None};
    Operator op{Operator::Equal};
    std::string value;  ///< Written property, not the effective one
    bool exclude{false};
};

/// Matches within (all entities if empty), once each in identifier order.
/// Off, frozen and locked layers are always excluded. Names ignore ASCII case.
/// An unreadable color or lineweight matches nothing before exclude is applied.
[[nodiscard]] std::vector<core::EntityId> quickSelect(const core::Document& document,
                                                    std::span<const core::EntityId> within,
                                                    const QuickSelectCriteria& criteria);

}  // namespace tg::cmd
