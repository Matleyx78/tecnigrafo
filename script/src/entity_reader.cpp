// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/entity_reader.hpp"

#include "text.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/text.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/text_format.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace tg::script {
namespace {

using Tokens = std::vector<std::string_view>;

// Types described by docs/verifica/rapporto.md that the model does not have
// yet. Since package P13 added TEXT, the model has every type of stage 0.4.
constexpr std::array<std::string_view, 0> kPlannedTypes{};

// The names of DXF groups 72 and 73, in the order of their values and of
// core::TextHorizontal and core::TextVertical (docs/verifica/rapporto.md).
constexpr std::array<std::string_view, 6> kHorizontalNames{"left",    "center", "right",
                                                           "aligned", "middle", "fit"};
constexpr std::array<std::string_view, 4> kVerticalNames{"baseline", "bottom", "middle", "top"};

// One full turn in degrees: ARC angles are written in [0, 360).
constexpr double kFullTurnDegrees = 360.0;

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

/// The values of an entity whose properties are each given at most once:
/// `values[i]` is the value of `names[i]`, or std::nullopt if it is left out.
using OptionalValues = std::vector<std::optional<std::string_view>>;

std::variant<OptionalValues, EntityError> readOnce(std::string_view type,
                                                   std::span<const std::string_view> names,
                                                   const Tokens& properties) {
    OptionalValues found(names.size());
    for (const std::string_view token : properties) {
        std::variant<Property, std::string> split = splitProperty(token);
        if (auto* error = std::get_if<std::string>(&split)) {
            return EntityError{std::move(*error)};
        }
        const auto [name, value] = std::get<Property>(split);
        if (name.starts_with('~')) {
            continue;  // derived: computed by the program, never read
        }
        const auto known = std::ranges::find(names, name);
        if (known == names.end()) {
            return EntityError{"unknown property " + quoted(name) + " of " + std::string{type}};
        }
        std::optional<std::string_view>& slot =
            found[static_cast<std::size_t>(known - names.begin())];
        if (slot) {
            return EntityError{std::string{name} + " is given more than once"};
        }
        slot = value;
    }
    return found;
}

/// The values of an entity whose properties are all required, each given
/// once: LINE, CIRCLE and ARC. `values[i]` is the value of `names[i]`.
using RequiredValues = std::vector<std::string_view>;

std::variant<RequiredValues, EntityError> readRequired(std::string_view type,
                                                       std::span<const std::string_view> names,
                                                       const Tokens& properties) {
    auto read = readOnce(type, names, properties);
    if (auto* error = std::get_if<EntityError>(&read)) {
        return std::move(*error);
    }
    const OptionalValues& found = std::get<OptionalValues>(read);
    RequiredValues values;
    for (std::size_t i = 0; i < names.size(); ++i) {
        const std::optional<std::string_view>& slot = found[i];
        if (!slot.has_value()) {
            return EntityError{"missing property " + std::string{names[i]} + " of " +
                               std::string{type}};
        }
        values.push_back(*slot);
    }
    return values;
}

std::variant<core::Vec2, EntityError> pointValue(std::string_view name, std::string_view value) {
    const std::optional<core::Vec2> point = parsePoint(value);
    if (!point) {
        return EntityError{std::string{name} + " needs a point x,y, found " + quoted(value)};
    }
    return *point;
}

std::variant<double, EntityError> radiusValue(std::string_view value) {
    const std::optional<double> radius = parseNumber(value);
    if (!radius || !(*radius > 0.0)) {
        return EntityError{"radius needs a number above 0, found " + quoted(value)};
    }
    return *radius;
}

/// An ARC angle in degrees, in [0, 360), converted to radians.
std::variant<double, EntityError> angleValue(std::string_view name, std::string_view value) {
    const std::optional<double> degrees = parseNumber(value);
    if (!degrees || *degrees < 0.0 || !(*degrees < kFullTurnDegrees)) {
        return EntityError{std::string{name} + " needs degrees in [0, 360), found " +
                           quoted(value)};
    }
    return core::normalizeAngle(core::degreesToRadians(*degrees));
}

std::variant<core::Entity, EntityError> readLine(const Tokens& properties) {
    constexpr std::array<std::string_view, 2> kNames{"start", "end"};
    const auto values = readRequired("LINE", kNames, properties);
    if (const auto* error = std::get_if<EntityError>(&values)) {
        return *error;
    }
    const auto& value = std::get<RequiredValues>(values);
    const auto start = pointValue(kNames[0], value[0]);
    if (const auto* error = std::get_if<EntityError>(&start)) {
        return *error;
    }
    const auto end = pointValue(kNames[1], value[1]);
    if (const auto* error = std::get_if<EntityError>(&end)) {
        return *error;
    }
    return core::Entity{core::Line{std::get<core::Vec2>(start), std::get<core::Vec2>(end)}};
}

std::variant<core::Entity, EntityError> readCircle(const Tokens& properties) {
    constexpr std::array<std::string_view, 2> kNames{"center", "radius"};
    const auto values = readRequired("CIRCLE", kNames, properties);
    if (const auto* error = std::get_if<EntityError>(&values)) {
        return *error;
    }
    const auto& value = std::get<RequiredValues>(values);
    const auto center = pointValue(kNames[0], value[0]);
    if (const auto* error = std::get_if<EntityError>(&center)) {
        return *error;
    }
    const auto radius = radiusValue(value[1]);
    if (const auto* error = std::get_if<EntityError>(&radius)) {
        return *error;
    }
    return core::Entity{core::Circle{std::get<core::Vec2>(center), std::get<double>(radius)}};
}

std::variant<core::Entity, EntityError> readArc(const Tokens& properties) {
    constexpr std::array<std::string_view, 4> kNames{"center", "radius", "start-angle",
                                                     "end-angle"};
    const auto values = readRequired("ARC", kNames, properties);
    if (const auto* error = std::get_if<EntityError>(&values)) {
        return *error;
    }
    const auto& value = std::get<RequiredValues>(values);
    const auto center = pointValue(kNames[0], value[0]);
    if (const auto* error = std::get_if<EntityError>(&center)) {
        return *error;
    }
    const auto radius = radiusValue(value[1]);
    if (const auto* error = std::get_if<EntityError>(&radius)) {
        return *error;
    }
    const auto startAngle = angleValue(kNames[2], value[2]);
    if (const auto* error = std::get_if<EntityError>(&startAngle)) {
        return *error;
    }
    const auto endAngle = angleValue(kNames[3], value[3]);
    if (const auto* error = std::get_if<EntityError>(&endAngle)) {
        return *error;
    }
    // Exact on purpose: the invariant of core::Arc is that the stored angles
    // differ; an arc of 360 degrees is a CIRCLE (docs/adr/0012).
    if (std::get<double>(startAngle) == std::get<double>(endAngle)) {
        return EntityError{"start-angle and end-angle must differ"};
    }
    return core::Entity{core::Arc{std::get<core::Vec2>(center), std::get<double>(radius),
                                  std::get<double>(startAngle), std::get<double>(endAngle)}};
}

/// A number above 0, as the height and the width factor of a TEXT.
std::variant<double, EntityError> positiveValue(std::string_view name, std::string_view value) {
    const std::optional<double> number = parseNumber(value);
    if (!number || !(*number > 0.0)) {
        return EntityError{std::string{name} + " needs a number above 0, found " + quoted(value)};
    }
    return *number;
}

/// An angle of a TEXT in degrees, any value, converted to radians as it is.
std::variant<double, EntityError> degreesValue(std::string_view name, std::string_view value) {
    const std::optional<double> degrees = parseNumber(value);
    if (!degrees) {
        return EntityError{std::string{name} + " needs degrees, found " + quoted(value)};
    }
    return core::degreesToRadians(*degrees);
}

std::variant<std::string, EntityError> textValue(std::string_view name, std::string_view value) {
    std::optional<std::string> text = unquoteText(value);
    if (!text) {
        return EntityError{std::string{name} + " needs a text between double quotes, found " +
                           quoted(value)};
    }
    return std::move(*text);
}

std::variant<bool, EntityError> flagValue(std::string_view name, std::string_view value) {
    const std::optional<bool> flag = parseYesNo(value);
    if (!flag) {
        return EntityError{std::string{name} + " needs yes or no, found " + quoted(value)};
    }
    return *flag;
}

/// The position of `value` in `names`, as the enumerator of the same value.
template <typename Enum, std::size_t Count>
std::variant<Enum, EntityError> enumValue(std::string_view name, std::string_view value,
                                          const std::array<std::string_view, Count>& names) {
    const auto found = std::ranges::find(names, value);
    if (found == names.end()) {
        std::string expected;
        for (const std::string_view option : names) {
            expected += expected.empty() ? "" : ", ";
            expected += option;
        }
        return EntityError{std::string{name} + " needs one of " + expected + ", found " +
                           quoted(value)};
    }
    return static_cast<Enum>(found - names.begin());
}

/// Stores in `target` the value of one TEXT property, read by `convert`, if
/// the property was given; otherwise `target` keeps the default of
/// core::Text. Returns the error of the conversion, if any.
template <typename Target, typename Convert>
std::optional<EntityError> readInto(Target& target, const std::optional<std::string_view>& value,
                                    const Convert& convert) {
    if (!value) {
        return std::nullopt;
    }
    auto converted = convert(*value);
    if (auto* error = std::get_if<EntityError>(&converted)) {
        return std::move(*error);
    }
    target = std::move(std::get<0>(converted));
    return std::nullopt;
}

std::variant<core::Entity, EntityError> readText(const Tokens& properties) {
    // The order of docs/verifica/rapporto.md. Left-out properties keep the
    // defaults of core::Text, which are those of the DXF format; position
    // and value are required.
    enum Index : std::uint8_t {
        Position,
        AlignmentPoint,
        Height,
        Rotation,
        WidthFactor,
        Oblique,
        Style,
        MirrorX,
        MirrorY,
        Horizontal,
        Vertical,
        Value,
        Count
    };

    constexpr std::array<std::string_view, Count> kNames{
        "position", "alignment-point", "height",   "rotation",   "width-factor", "oblique",
        "style",    "mirror-x",        "mirror-y", "horizontal", "vertical",     "value"};
    auto read = readOnce("TEXT", kNames, properties);
    if (auto* error = std::get_if<EntityError>(&read)) {
        return std::move(*error);
    }
    const OptionalValues& found = std::get<OptionalValues>(read);
    for (const Index required : {Position, Value}) {
        if (!found[required]) {
            return EntityError{"missing property " + std::string{kNames.at(required)} + " of TEXT"};
        }
    }

    core::Text text;
    const auto point = [&](Index index) {
        return [name = kNames.at(index)](std::string_view value) {
            return pointValue(name, value);
        };
    };
    const auto positive = [&](Index index) {
        return [name = kNames.at(index)](std::string_view value) {
            return positiveValue(name, value);
        };
    };
    const auto degrees = [&](Index index) {
        return [name = kNames.at(index)](std::string_view value) {
            return degreesValue(name, value);
        };
    };
    const auto string = [&](Index index) {
        return [name = kNames.at(index)](std::string_view value) {
            return textValue(name, value);
        };
    };
    const auto flag = [&](Index index) {
        return [name = kNames.at(index)](std::string_view value) {
            return flagValue(name, value);
        };
    };
    const auto horizontal = [name = kNames[Horizontal]](std::string_view value) {
        return enumValue<core::TextHorizontal>(name, value, kHorizontalNames);
    };
    const auto vertical = [name = kNames[Vertical]](std::string_view value) {
        return enumValue<core::TextVertical>(name, value, kVerticalNames);
    };
    // In the order of the properties: the first error is reported.
    for (std::optional<EntityError> error :
         {readInto(text.position, found[Position], point(Position)),
          readInto(text.alignmentPoint, found[AlignmentPoint], point(AlignmentPoint)),
          readInto(text.height, found[Height], positive(Height)),
          readInto(text.rotation, found[Rotation], degrees(Rotation)),
          readInto(text.widthFactor, found[WidthFactor], positive(WidthFactor)),
          readInto(text.obliqueAngle, found[Oblique], degrees(Oblique)),
          readInto(text.style, found[Style], string(Style)),
          readInto(text.mirroredX, found[MirrorX], flag(MirrorX)),
          readInto(text.mirroredY, found[MirrorY], flag(MirrorY)),
          readInto(text.horizontal, found[Horizontal], horizontal),
          readInto(text.vertical, found[Vertical], vertical),
          readInto(text.value, found[Value], string(Value))}) {
        if (error) {
            return std::move(*error);
        }
    }
    return core::Entity{std::move(text)};
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
    if (type == "LINE") {
        return readLine(properties);
    }
    if (type == "CIRCLE") {
        return readCircle(properties);
    }
    if (type == "ARC") {
        return readArc(properties);
    }
    if (type == "TEXT") {
        return readText(properties);
    }
    if (std::ranges::find(kPlannedTypes, type) != kPlannedTypes.end()) {
        return EntityError{"entity type " + std::string{type} + " is not implemented yet", true};
    }
    return EntityError{"unknown entity type " + quoted(type)};
}

}  // namespace tg::script
