// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/entity_reader.hpp"

#include "text.hpp"
#include "tg/core/polyline.hpp"
#include "tg/script/number_format.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <utility>
#include <vector>

namespace tg::script {
namespace {

using Tokens = std::vector<std::string_view>;

// Types described by docs/verifica/rapporto.md for stage 0.4 that the model
// does not have yet: packages P6 and P13 add them, and remove them from here.
constexpr std::array kPlannedTypes = {std::string_view{"LINE"}, std::string_view{"CIRCLE"},
                                      std::string_view{"ARC"}, std::string_view{"TEXT"}};

struct Property {
    std::string_view name;
    std::string_view value;
};

std::string quoted(std::string_view text) {
    return "'" + std::string{text} + "'";
}

/// The property written by `token` ("name=value"), or the error message.
std::variant<Property, std::string> splitProperty(std::string_view token) {
    const std::size_t equals = token.find('=');
    if (equals == std::string_view::npos || equals == 0) {
        return "expected <name>=<value>, found " + quoted(token);
    }
    return Property{token.substr(0, equals), token.substr(equals + 1)};
}

std::optional<bool> parseYesNo(std::string_view text) {
    if (text == "yes") {
        return true;
    }
    if (text == "no") {
        return false;
    }
    return std::nullopt;
}

std::variant<core::Entity, EntityError> readPolyline(const Tokens& properties) {
    std::vector<core::PolylineVertex> vertices;
    std::optional<bool> closed;
    bool bulgeGiven = false;  // for the last vertex
    for (const std::string_view token : properties) {
        std::variant<Property, std::string> split = splitProperty(token);
        if (auto* error = std::get_if<std::string>(&split)) {
            return EntityError{std::move(*error)};
        }
        const auto [name, value] = std::get<Property>(split);
        if (name.starts_with('~')) {
            continue;  // derived: computed by the program, never read
        }
        if (name == "closed") {
            const std::optional<bool> flag = parseYesNo(value);
            if (closed || !flag) {
                return EntityError{"closed is given once, as yes or no"};
            }
            closed = flag;
        } else if (name == "vertex") {
            const std::optional<core::Vec2> point = parsePoint(value);
            if (!point) {
                return EntityError{"vertex needs a point x,y, found " + quoted(value)};
            }
            vertices.push_back({*point, 0.0});
            bulgeGiven = false;
        } else if (name == "bulge") {
            const std::optional<double> bulge = parseNumber(value);
            if (vertices.empty() || bulgeGiven || !bulge) {
                return EntityError{"bulge is a number given at most once after each vertex"};
            }
            vertices.back().bulge = *bulge;
            bulgeGiven = true;
        } else {
            return EntityError{"unknown property " + quoted(name) + " of LWPOLYLINE"};
        }
    }
    return core::Entity{core::Polyline{std::move(vertices), closed.value_or(false)}};
}

}  // namespace

std::variant<core::Entity, EntityError> readEntity(std::string_view text) {
    const std::optional<Tokens> tokens = detail::splitTokens(text);
    if (!tokens) {
        return EntityError{"a quoted text is not closed"};
    }
    if (tokens->empty()) {
        return EntityError{"missing entity type"};
    }
    const std::string_view type = tokens->front();
    const Tokens properties(tokens->begin() + 1, tokens->end());
    if (type == "LWPOLYLINE") {
        return readPolyline(properties);
    }
    if (std::ranges::find(kPlannedTypes, type) != kPlannedTypes.end()) {
        return EntityError{"entity type " + std::string{type} + " is not implemented yet", true};
    }
    return EntityError{"unknown entity type " + quoted(type)};
}

}  // namespace tg::script
