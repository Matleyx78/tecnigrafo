// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The own properties of MTEXT, ATTDEF and INSERT: written by PT24
// (docs/tappe/0.8.1/PT24-proprieta-testi-inserimenti.md). Until then none,
// and withOwnProperty() refuses every name before calling these functions.

#include "tg/core/entity_properties.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::core {

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/,
                                         const EntityRecord& /*record*/, const MText& /*mtext*/) {
    return {};
}

PropertyChange withOwnPropertyOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                 const MText& /*mtext*/, std::string_view /*name*/,
                                 const PropertyValue& /*value*/) {
    return PropertyChange{std::nullopt, std::nullopt, std::string{kUnknownProperty}};
}

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/,
                                         const EntityRecord& /*record*/,
                                         const AttributeDefinition& /*definition*/) {
    return {};
}

PropertyChange withOwnPropertyOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                 const AttributeDefinition& /*definition*/,
                                 std::string_view /*name*/, const PropertyValue& /*value*/) {
    return PropertyChange{std::nullopt, std::nullopt, std::string{kUnknownProperty}};
}

std::vector<OwnProperty> ownPropertiesOf(const Document& /*document*/,
                                         const EntityRecord& /*record*/, const Insert& /*insert*/) {
    return {};
}

PropertyChange withOwnPropertyOf(const Document& /*document*/, const EntityRecord& /*record*/,
                                 const Insert& /*insert*/, std::string_view /*name*/,
                                 const PropertyValue& /*value*/) {
    return PropertyChange{std::nullopt, std::nullopt, std::string{kUnknownProperty}};
}

}  // namespace tg::core
