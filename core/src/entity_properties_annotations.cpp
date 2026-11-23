// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The own properties of HATCH and DIMENSION: written by PT23
// (docs/tappe/0.8.1/PT23-proprieta-retini-quote.md). Until then none, and
// withOwnProperty() refuses every name before calling these functions.

#include "tg/core/entity_properties.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::core {

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/,
                                         const EntityRecord& /*record*/, const Hatch& /*hatch*/) {
    return {};
}

PropertyChange withOwnPropertyOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                 const Hatch& /*hatch*/, std::string_view /*name*/,
                                 const PropertyValue& /*value*/) {
    return PropertyChange{std::nullopt, std::nullopt, std::string{kUnknownProperty}};
}

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/,
                                         const EntityRecord& /*record*/,
                                         const Dimension& /*dimension*/) {
    return {};
}

PropertyChange withOwnPropertyOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                 const Dimension& /*dimension*/, std::string_view /*name*/,
                                 const PropertyValue& /*value*/) {
    return PropertyChange{std::nullopt, std::nullopt, std::string{kUnknownProperty}};
}

}  // namespace tg::core
