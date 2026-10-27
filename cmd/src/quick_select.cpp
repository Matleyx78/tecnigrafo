// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/quick_select.hpp"

#include "tg/core/effective_properties.hpp"

#include <algorithm>
#include <array>
#include <string_view>
#include <unordered_set>

namespace tg::cmd {
namespace {

bool matchesProperty(const core::Document& document, const core::EntityRecord& record,
                     const QuickSelectCriteria& criteria) {
    using Property = QuickSelectCriteria::Property;
    const auto& properties = record.properties;
    bool equal = false;
    switch (criteria.property) {
        case Property::None:
            return true;
        case Property::Layer: {
            const auto* layer = document.layer(properties.layer);
            equal = core::sameTableName(layer != nullptr ? layer->name : "0", criteria.value);
            break;
        }
        case Property::Color: {
            const auto color = core::parseColor(criteria.value);
            if (!color) {
                return false;
            }
            equal = properties.color == *color;
            break;
        }
        case Property::Linetype: {
            const auto* linetype = document.linetype(properties.linetype);
            equal = core::sameTableName(linetype != nullptr ? linetype->name : "ByLayer",
                                        criteria.value);
            break;
        }
        case Property::Lineweight: {
            const auto weight = core::parseLineweight(criteria.value);
            if (!weight) {
                return false;
            }
            equal = properties.lineweight == *weight;
            break;
        }
    }
    return criteria.op == QuickSelectCriteria::Operator::Equal ? equal : !equal;
}

}  // namespace

std::vector<core::EntityId> quickSelect(const core::Document& document,
                                        std::span<const core::EntityId> within,
                                        const QuickSelectCriteria& criteria) {
    // The order follows core::Entity, whose alternatives are the handled DXF types.
    constexpr std::array<std::string_view, std::variant_size_v<core::Entity>> kTypes{
        "LWPOLYLINE", "LINE", "CIRCLE", "ARC", "TEXT"};
    std::unordered_set<std::uint64_t> members;
    for (const auto id : within) {
        members.insert(id.value);
    }
    std::vector<core::EntityId> result;
    for (const auto& record : document.entities()) {
        if (!within.empty() && !members.contains(record.id.value)) {
            continue;
        }
        const auto* layer = document.layer(record.properties.layer);
        if (!core::isVisible(document, record) || (layer != nullptr && layer->locked)) {
            continue;
        }
        const bool matches =
            (!criteria.type ||
             core::sameTableName(*criteria.type, kTypes.at(record.entity.index()))) &&
            matchesProperty(document, record, criteria);
        if (matches != criteria.exclude) {
            result.push_back(record.id);
        }
    }
    std::ranges::sort(result, {}, &core::EntityId::value);
    return result;
}

}  // namespace tg::cmd
