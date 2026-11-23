// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/core/entity_properties.hpp"

#include "tg/core/document.hpp"

#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace tg::core {
namespace {

template <typename T>
inline constexpr bool kHasNoName = false;

/// The DXF name of the alternative T of Entity.
template <typename T>
constexpr std::string_view typeName() {
    if constexpr (std::is_same_v<T, Polyline>) {
        return "LWPOLYLINE";
    } else if constexpr (std::is_same_v<T, Line>) {
        return "LINE";
    } else if constexpr (std::is_same_v<T, Circle>) {
        return "CIRCLE";
    } else if constexpr (std::is_same_v<T, Arc>) {
        return "ARC";
    } else if constexpr (std::is_same_v<T, Text>) {
        return "TEXT";
    } else if constexpr (std::is_same_v<T, Solid>) {
        return "SOLID";
    } else if constexpr (std::is_same_v<T, Ellipse>) {
        return "ELLIPSE";
    } else if constexpr (std::is_same_v<T, MText>) {
        return "MTEXT";
    } else if constexpr (std::is_same_v<T, AttributeDefinition>) {
        return "ATTDEF";
    } else if constexpr (std::is_same_v<T, Insert>) {
        return "INSERT";
    } else if constexpr (std::is_same_v<T, Dimension>) {
        return "DIMENSION";
    } else if constexpr (std::is_same_v<T, Hatch>) {
        return "HATCH";
    } else {
        static_assert(kHasNoName<T>, "every alternative of core::Entity has a DXF name (#913)");
        return {};
    }
}

/// True for the types of before the 0.8, which have no own properties.
template <typename T>
constexpr bool kBefore08 =
    std::is_same_v<T, Polyline> || std::is_same_v<T, Line> || std::is_same_v<T, Circle> ||
    std::is_same_v<T, Arc> || std::is_same_v<T, Text>;

PropertyChange refused(std::string_view reason) {
    return PropertyChange{std::nullopt, std::nullopt, std::string{reason}};
}

}  // namespace

std::string_view entityTypeName(const Entity& entity) {
    return std::visit(
        [](const auto& alternative) { return typeName<std::decay_t<decltype(alternative)>>(); },
        entity);
}

std::vector<OwnProperty> ownProperties(const Document& document, const EntityRecord& record) {
    assert(isSupported(record.entity));
    return std::visit(
        [&document, &record](const auto& entity) -> std::vector<OwnProperty> {
            if constexpr (kBefore08<std::decay_t<decltype(entity)>>) {
                return {};
            } else {
                return ownPropertiesOf(document, record, entity);
            }
        },
        record.entity);
}

PropertyChange withOwnProperty(const Document& document, const EntityRecord& record,
                               std::string_view name, const PropertyValue& value) {
    assert(isSupported(record.entity));
    const std::vector<OwnProperty> properties = ownProperties(document, record);
    const auto found = std::ranges::find(properties, name, &OwnProperty::name);
    if (found == properties.end()) {
        return refused(kUnknownProperty);
    }
    if (found->readOnly) {
        return refused(kReadOnlyProperty);
    }
    return std::visit(
        [&document, &record, name, &value](const auto& entity) -> PropertyChange {
            if constexpr (kBefore08<std::decay_t<decltype(entity)>>) {
                return refused(kUnknownProperty);  // unreachable: they have no property
            } else {
                return withOwnPropertyOf(document, record, entity, name, value);
            }
        },
        record.entity);
}

}  // namespace tg::core
