// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The header variables of the current values, of the linetype scale and of
// the lineweights (docs/adr/0023, point 3; docs/adr/0025, points 3 and 6).
// Reading is TableReader::readHeader(), in dxf_tables.cpp.

#include "dxf_entity_common.hpp"
#include "dxf_tables.hpp"
#include "tg/io/dxf_groups.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tg::io {
namespace {

using dxf_groups::rgbValue;

constexpr int kColorByLayer = 256;

/// A value group of a variable: its text, and for a number the value a
/// group read from the file must have to be written again as it was.
struct Value {
    core::RawGroup group;
    std::optional<double> number;
};

Value integerValue(std::int16_t code, long long value) {
    return {{code, std::to_string(value)}, static_cast<double>(value)};
}

Value realValue(double value) {
    return {{40, formatNumber(value)}, value};
}

struct Variable {
    std::string_view name;
    std::vector<Value> values;
    bool differsFromNew{false};  ///< the value is not that of a new drawing
};

std::vector<Variable> variables(const core::Document& document) {
    const auto& current = document.current();
    const core::Layer* layer = document.layer(current.layer);
    const core::Linetype* linetype = document.linetype(current.linetype);
    const std::string layerName = layer != nullptr ? layer->name : std::string{"0"};
    const std::string linetypeName = linetype != nullptr ? linetype->name : std::string{"ByLayer"};

    std::vector<Value> color;
    switch (current.color.kind()) {
        case core::Color::Kind::ByLayer:
            color.push_back(integerValue(62, kColorByLayer));
            break;
        case core::Color::Kind::ByBlock:
            color.push_back(integerValue(62, 0));
            break;
        case core::Color::Kind::Index:
            color.push_back(integerValue(62, current.color.index()));
            break;
        case core::Color::Kind::Rgb:
            color.push_back(integerValue(62, core::nearestAci(current.color.rgb())));
            color.push_back(integerValue(420, rgbValue(current.color.rgb())));
            break;
    }
    return {
        {"$CLAYER", {{{8, layerName}, std::nullopt}}, !core::sameTableName(layerName, "0")},
        {"$CECOLOR", color, current.color != core::Color::byLayer()},
        {"$CELTYPE",
         {{{6, linetypeName}, std::nullopt}},
         !core::sameTableName(linetypeName, "ByLayer")},
        {"$CELWEIGHT",
         {integerValue(370, current.lineweight)},
         current.lineweight != core::lineweight::kByLayer},
        {"$CELTSCALE", {realValue(current.linetypeScale)}, current.linetypeScale != 1.0},
        {"$LTSCALE", {realValue(document.ltscale())}, document.ltscale() != 1.0},
        {"$LWDISPLAY", {integerValue(290, document.lwdisplay() ? 1 : 0)}, document.lwdisplay()},
        {"$LWDEFAULT",
         {integerValue(370, document.lwdefault())},
         document.lwdefault() != core::Document::kDefaultLwdefault},
    };
}

/// True when `read`, a value group of the file, says what `wanted` says:
/// then it is written again as it was ("   256" stays so).
bool same(const core::RawGroup& read, const Value& wanted) {
    if (read.code != wanted.group.code) {
        return false;
    }
    if (wanted.number) {
        return parseNumber(trimDxf(read.value)) == *wanted.number;
    }
    return read.value == wanted.group.value;
}

}  // namespace

core::RawGroups headerProperties(const core::RawGroups& header, const core::Document& document,
                                 bool addAll) {
    const auto all = variables(document);
    std::vector<bool> found(all.size(), false);
    core::RawGroups out;
    std::size_t i = 0;
    while (i < header.size()) {
        const auto& group = header[i++];
        out.push_back(group);
        if (group.code != 9) {
            continue;
        }
        const auto variable = std::ranges::find(all, trimDxf(group.value), &Variable::name);
        if (variable == all.end()) {
            continue;
        }
        found[static_cast<std::size_t>(variable - all.begin())] = true;
        // The value groups as read, up to the next variable.
        core::RawGroups read;
        while (i < header.size() && header[i].code != 9) {
            read.push_back(header[i++]);
        }
        std::vector<bool> used(read.size(), false);
        // The 62 read with an RGB that has not changed is written again as
        // it was, as for the entities (docs/adr/0024, point 2).
        const auto read420 = std::ranges::find(read, 420, &core::RawGroup::code);
        const bool unchangedRgb = variable->values.size() == 2 && read420 != read.end() &&
                                  same(*read420, variable->values[1]);
        for (const auto& value : variable->values) {
            const auto at = std::ranges::find(read, value.group.code, &core::RawGroup::code);
            if (at == read.end()) {
                out.push_back(value.group);
                continue;
            }
            used[static_cast<std::size_t>(at - read.begin())] = true;
            const bool keep = same(*at, value) || (unchangedRgb && value.group.code == 62);
            out.push_back(keep ? *at : value.group);
        }
        // A 420 of a colour that is now an index goes; any other group
        // stays as read.
        for (std::size_t j = 0; j < read.size(); ++j) {
            const bool stale = variable->name == "$CECOLOR" && read[j].code == 420;
            if (!used[j] && !stale) {
                out.push_back(read[j]);
            }
        }
    }
    for (std::size_t k = 0; k < all.size(); ++k) {
        if (!found[k] && (addAll || all[k].differsFromNew)) {
            out.push_back({9, std::string{all[k].name}});
            for (const auto& value : all[k].values) {
                out.push_back(value.group);
            }
        }
    }
    return out;
}

}  // namespace tg::io
