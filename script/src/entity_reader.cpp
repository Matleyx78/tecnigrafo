// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/script/entity_reader.hpp"

#include "text.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/arc_geometry.hpp"
#include "tg/core/circle.hpp"
#include "tg/core/color.hpp"
#include "tg/core/layer.hpp"
#include "tg/core/line.hpp"
#include "tg/core/linetype_pattern.hpp"
#include "tg/core/lineweight.hpp"
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
    std::optional<double> width;
    bool startWidthGiven = false;
    bool endWidthGiven = false;
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
        } else if (name == "width") {
            const auto number = parseNumber(value);
            if (width || !number) {
                return EntityError{"width is a number given once"};
            }
            width = number;
        } else if (name == "start-width" || name == "end-width") {
            const auto number = parseNumber(value);
            bool& given = name == "start-width" ? startWidthGiven : endWidthGiven;
            if (vertices.empty() || given || !number) {
                return EntityError{"vertex width is a number given once after each vertex"};
            }
            if (name == "start-width") {
                vertices.back().startWidth = *number;
            } else {
                vertices.back().endWidth = *number;
            }
            given = true;
        } else if (name == "vertex") {
            const std::optional<core::Vec2> point = parsePoint(value);
            if (!point) {
                return EntityError{"vertex needs a point x,y, found " + quoted(value)};
            }
            vertices.push_back({*point, 0.0});
            bulgeGiven = false;
            startWidthGiven = false;
            endWidthGiven = false;
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
    core::Polyline polyline{std::move(vertices), closed.value_or(false)};
    polyline.setConstantWidth(width.value_or(0));
    return core::Entity{std::move(polyline)};
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

/// Whether `text` is a vector "x,y,z" of three numbers.
bool isVector(std::string_view text) {
    const std::size_t first = text.find(',');
    const std::size_t second = first == std::string_view::npos ? first : text.find(',', first + 1);
    if (second == std::string_view::npos) {
        return false;
    }
    return parseNumber(text.substr(0, first)) &&
           parseNumber(text.substr(first + 1, second - first - 1)) &&
           parseNumber(text.substr(second + 1));
}

/// The quota of each type (docs/verifica/rapporto.md, stage 0.5).
std::span<const std::string_view> quotaNames(std::string_view type) {
    static constexpr std::array<std::string_view, 1> kPolyline{"elevation"};
    static constexpr std::array<std::string_view, 2> kLine{"start-z", "end-z"};
    static constexpr std::array<std::string_view, 1> kCurve{"z"};
    static constexpr std::array<std::string_view, 2> kText{"z", "alignment-z"};
    if (type == "LWPOLYLINE") {
        return kPolyline;
    }
    if (type == "LINE") {
        return kLine;
    }
    if (type == "CIRCLE" || type == "ARC") {
        return kCurve;
    }
    if (type == "TEXT") {
        return kText;
    }
    return {};
}

/// Checks and takes out of `properties` the quota, thickness and extrusion
/// of stage 0.5. The model keeps them only for an entity read from a file
/// (core::EntityExtras): an entity read here is made as a command makes it.
std::optional<EntityError> takePlacement(std::string_view type, Tokens& properties) {
    const std::span<const std::string_view> quotas = quotaNames(type);
    Tokens rest;
    std::vector<std::string_view> seen;
    for (const std::string_view token : properties) {
        const std::size_t equals = token.find('=');
        const std::string_view name =
            equals == std::string_view::npos ? std::string_view{} : token.substr(0, equals);
        const bool number = name == "thickness" || std::ranges::find(quotas, name) != quotas.end();
        if (!number && name != "extrusion") {
            rest.push_back(token);
            continue;
        }
        const std::string_view value = token.substr(equals + 1);
        if (std::ranges::find(seen, name) != seen.end()) {
            return EntityError{std::string{name} + " is given more than once"};
        }
        seen.push_back(name);
        if (number && !parseNumber(value)) {
            return EntityError{std::string{name} + " needs a number, found " + quoted(value)};
        }
        if (!number && !isVector(value)) {
            return EntityError{"extrusion needs a vector x,y,z, found " + quoted(value)};
        }
    }
    properties = std::move(rest);
    return std::nullopt;
}

// ---------------------------------------------------------------------------
// Properties of stage 0.6 (docs/verifica/rapporto.md)

/// A name of a layer or linetype, between double quotes.
std::variant<std::string, EntityError> tableNameValue(std::string_view name,
                                                      std::string_view value) {
    std::optional<std::string> text = unquoteText(value);
    if (!text || text->empty()) {
        return EntityError{std::string{name} + " needs a name between double quotes, found " +
                           quoted(value)};
    }
    return std::move(*text);
}

/// What readWrittenProperty() found.
struct PropertyRead {
    bool known{false};  ///< the name is a property of stage 0.6
    std::optional<EntityError> error;
};

/// Reads into `written` the property `name`=`value` if it is one of stage
/// 0.6; otherwise `known` is false, so that the caller reads it as a
/// property of the geometry.
PropertyRead readWrittenProperty(std::string_view name, std::string_view value,
                                 WrittenProperties& written) {
    const auto failed = [](std::string message) {
        return PropertyRead{true, EntityError{std::move(message)}};
    };
    const auto twice = [&] {
        return failed(std::string{name} + " is given more than once");
    };
    if (name == "layer" || name == "linetype") {
        std::optional<std::string>& slot = name == "layer" ? written.layer : written.linetype;
        if (slot) {
            return twice();
        }
        auto text = tableNameValue(name, value);
        if (auto* error = std::get_if<EntityError>(&text)) {
            return PropertyRead{true, std::move(*error)};
        }
        slot = std::get<std::string>(std::move(text));
        return PropertyRead{true, std::nullopt};
    }
    if (name == "color") {
        if (written.color) {
            return twice();
        }
        written.color = core::parseColor(value);
        if (!written.color) {
            return failed("color needs bylayer, byblock, an index 1-255 or r,g,b, found " +
                          quoted(value));
        }
        return PropertyRead{true, std::nullopt};
    }
    if (name == "ltscale") {
        if (written.linetypeScale) {
            return twice();
        }
        const std::optional<double> scale = parseNumber(value);
        if (!scale || !(*scale > 0.0)) {
            return failed("ltscale needs a number above 0, found " + quoted(value));
        }
        written.linetypeScale = scale;
        return PropertyRead{true, std::nullopt};
    }
    if (name == "lineweight") {
        if (written.lineweight) {
            return twice();
        }
        written.lineweight = core::parseLineweight(value);
        if (!written.lineweight) {
            return failed(
                "lineweight needs bylayer, byblock, default or an allowed width in mm, found " +
                quoted(value));
        }
        return PropertyRead{true, std::nullopt};
    }
    return PropertyRead{};
}

/// Checks and takes out of `properties` the properties of stage 0.6, and
/// the linetype generation of an LWPOLYLINE: the rest is the geometry.
std::optional<EntityError> takeProperties(std::string_view type, Tokens& properties,
                                          EntityDefinition& definition) {
    Tokens rest;
    for (const std::string_view token : properties) {
        const std::size_t equals = token.find('=');
        if (equals == std::string_view::npos) {
            rest.push_back(token);
            continue;
        }
        const std::string_view name = token.substr(0, equals);
        const std::string_view value = token.substr(equals + 1);
        if (type == "LWPOLYLINE" && name == "linetype-generation") {
            const std::optional<bool> flag = parseYesNo(value);
            if (definition.linetypeGeneration || !flag) {
                return EntityError{"linetype-generation is given once, as yes or no"};
            }
            definition.linetypeGeneration = flag;
            continue;
        }
        PropertyRead read = readWrittenProperty(name, value, definition.properties);
        if (!read.known) {
            rest.push_back(token);
        } else if (read.error) {
            return std::move(read.error);
        }
    }
    properties = std::move(rest);
    return std::nullopt;
}

/// The value of a definition of a table record ("name=\"Muri\""): the
/// properties given once each, and the bare words (`off`) given once each.
struct RecordTokens {
    std::vector<Property> properties;
    std::vector<std::string_view> words;
};

std::variant<RecordTokens, EntityError> readRecordTokens(std::string_view type,
                                                         std::string_view text,
                                                         std::span<const std::string_view> names,
                                                         std::span<const std::string_view> words) {
    const std::optional<Tokens> tokens = detail::splitTokens(text);
    if (!tokens) {
        return EntityError{"a quoted text is not closed"};
    }
    if (tokens->empty() || tokens->front() != type) {
        return EntityError{"expected " + std::string{type}};
    }
    RecordTokens record;
    for (auto token = tokens->begin() + 1; token != tokens->end(); ++token) {
        if (std::ranges::find(words, *token) != words.end()) {
            if (std::ranges::find(record.words, *token) != record.words.end()) {
                return EntityError{std::string{*token} + " is given more than once"};
            }
            record.words.push_back(*token);
            continue;
        }
        std::variant<Property, std::string> split = splitProperty(*token);
        if (auto* error = std::get_if<std::string>(&split)) {
            return EntityError{std::move(*error)};
        }
        const Property property = std::get<Property>(split);
        if (std::ranges::find(names, property.name) == names.end()) {
            return EntityError{"unknown property " + quoted(property.name) + " of " +
                               std::string{type}};
        }
        if (std::ranges::find(record.properties, property.name, &Property::name) !=
            record.properties.end()) {
            return EntityError{std::string{property.name} + " is given more than once"};
        }
        record.properties.push_back(property);
    }
    return record;
}

std::optional<std::string_view> findValue(const RecordTokens& record, std::string_view name) {
    const auto found = std::ranges::find(record.properties, name, &Property::name);
    if (found == record.properties.end()) {
        return std::nullopt;
    }
    return found->value;
}

bool hasWord(const RecordTokens& record, std::string_view word) {
    return std::ranges::find(record.words, word) != record.words.end();
}

/// The name of a table record: required, quoted, valid (docs/adr/0023, point 1).
std::variant<std::string, EntityError> recordName(std::string_view type,
                                                  const RecordTokens& record) {
    const std::optional<std::string_view> value = findValue(record, "name");
    if (!value) {
        return EntityError{"missing property name of " + std::string{type}};
    }
    auto name = tableNameValue("name", *value);
    if (const auto* error = std::get_if<EntityError>(&name)) {
        return *error;
    }
    if (!core::isValidTableName(std::get<std::string>(name))) {
        return EntityError{"invalid name " + quoted(std::get<std::string>(name)) + " of " +
                           std::string{type}};
    }
    return name;
}

/// A linetype pattern: numbers separated by ',', or "continuous".
std::optional<core::LinetypePattern> parsePattern(std::string_view text) {
    core::LinetypePattern pattern;
    if (text == "continuous") {
        return pattern;
    }
    while (true) {
        const std::size_t comma = text.find(',');
        const std::optional<double> element = parseNumber(text.substr(0, comma));
        if (!element) {
            return std::nullopt;
        }
        pattern.elements.push_back(*element);
        if (comma == std::string_view::npos) {
            return pattern;
        }
        text.remove_prefix(comma + 1);
    }
}

std::string unknownName(std::string_view kind, std::string_view name) {
    return "unknown " + std::string{kind} + " " + quoted(name) + ": define it above with " +
           (kind == "layer" ? "LAYER" : "LTYPE");
}

}  // namespace

std::variant<EntityDefinition, EntityError> readEntityDefinition(std::string_view text) {
    const std::optional<Tokens> tokens = detail::splitTokens(text);
    if (!tokens) {
        return EntityError{"a quoted text is not closed"};
    }
    if (tokens->empty()) {
        return EntityError{"missing entity type"};
    }
    const std::string_view type = tokens->front();
    Tokens properties(tokens->begin() + 1, tokens->end());
    if (std::optional<EntityError> error = takePlacement(type, properties)) {
        return std::move(*error);
    }
    EntityDefinition definition{core::Line{}, {}, std::nullopt};
    if (std::optional<EntityError> error = takeProperties(type, properties, definition)) {
        return std::move(*error);
    }
    std::variant<core::Entity, EntityError> entity = EntityError{};
    if (type == "LWPOLYLINE") {
        entity = readPolyline(properties);
    } else if (type == "LINE") {
        entity = readLine(properties);
    } else if (type == "CIRCLE") {
        entity = readCircle(properties);
    } else if (type == "ARC") {
        entity = readArc(properties);
    } else if (type == "TEXT") {
        entity = readText(properties);
    } else if (std::ranges::find(kPlannedTypes, type) != kPlannedTypes.end()) {
        return EntityError{"entity type " + std::string{type} + " is not implemented yet", true};
    } else {
        return EntityError{"unknown entity type " + quoted(type)};
    }
    if (auto* error = std::get_if<EntityError>(&entity)) {
        return std::move(*error);
    }
    definition.entity = std::get<core::Entity>(std::move(entity));
    return definition;
}

std::variant<core::Entity, EntityError> readEntity(std::string_view text) {
    std::variant<EntityDefinition, EntityError> definition = readEntityDefinition(text);
    if (auto* error = std::get_if<EntityError>(&definition)) {
        return std::move(*error);
    }
    return std::get<EntityDefinition>(std::move(definition)).entity;
}

std::variant<WrittenProperties, EntityError> readProperties(std::string_view text) {
    const std::optional<Tokens> tokens = detail::splitTokens(text);
    if (!tokens) {
        return EntityError{"a quoted text is not closed"};
    }
    WrittenProperties written;
    for (const std::string_view token : *tokens) {
        std::variant<Property, std::string> split = splitProperty(token);
        if (auto* error = std::get_if<std::string>(&split)) {
            return EntityError{std::move(*error)};
        }
        const auto [name, value] = std::get<Property>(split);
        PropertyRead read = readWrittenProperty(name, value, written);
        if (!read.known) {
            return EntityError{"unknown property " + quoted(name) +
                               ": expected layer, color, linetype, ltscale or lineweight"};
        }
        if (read.error) {
            return std::move(*read.error);
        }
    }
    return written;
}

std::variant<core::EntityProperties, EntityError> applyProperties(
    const WrittenProperties& written, const core::EntityProperties& base,
    const core::Document& document) {
    core::EntityProperties properties = base;
    if (written.layer) {
        const core::Layer* layer = document.findLayer(*written.layer);
        if (layer == nullptr) {
            return EntityError{unknownName("layer", *written.layer)};
        }
        properties.layer = layer->id;
    }
    if (written.linetype) {
        const core::Linetype* linetype = document.findLinetype(*written.linetype);
        if (linetype == nullptr) {
            return EntityError{unknownName("linetype", *written.linetype)};
        }
        properties.linetype = linetype->id;
    }
    properties.color = written.color.value_or(properties.color);
    properties.linetypeScale = written.linetypeScale.value_or(properties.linetypeScale);
    properties.lineweight = written.lineweight.value_or(properties.lineweight);
    return properties;
}

std::variant<core::EntityProperties, EntityError> resolveProperties(
    const WrittenProperties& written, const core::Document& document) {
    // The defaults of docs/verifica/rapporto.md, not the current values: a
    // definition writes what the entity has, as a line of the report.
    core::EntityProperties defaults;
    defaults.layer = document.layerZero();
    defaults.linetype = document.byLayerLinetype();
    return applyProperties(written, defaults, document);
}

std::variant<LayerDefinition, EntityError> readLayerDefinition(std::string_view text) {
    constexpr std::array<std::string_view, 4> kNames{"name", "color", "linetype", "lineweight"};
    constexpr std::array<std::string_view, 4> kWords{"off", "frozen", "locked", "noplot"};
    auto tokens = readRecordTokens("LAYER", text, kNames, kWords);
    if (auto* error = std::get_if<EntityError>(&tokens)) {
        return std::move(*error);
    }
    const RecordTokens& record = std::get<RecordTokens>(tokens);
    auto name = recordName("LAYER", record);
    if (auto* error = std::get_if<EntityError>(&name)) {
        return std::move(*error);
    }
    LayerDefinition layer;
    layer.name = std::get<std::string>(std::move(name));
    if (const auto value = findValue(record, "color")) {
        const std::optional<core::Color> color = core::parseColor(*value);
        // A layer has an index or RGB colour, never ByLayer nor ByBlock
        // (docs/adr/0024, point 1).
        if (!color || color->kind() == core::Color::Kind::ByLayer ||
            color->kind() == core::Color::Kind::ByBlock) {
            return EntityError{"color of a LAYER needs an index 1-255 or r,g,b, found " +
                               quoted(*value)};
        }
        layer.color = *color;
    }
    if (const auto value = findValue(record, "linetype")) {
        auto linetype = tableNameValue("linetype", *value);
        if (auto* error = std::get_if<EntityError>(&linetype)) {
            return std::move(*error);
        }
        layer.linetype = std::get<std::string>(std::move(linetype));
        if (core::sameTableName(layer.linetype, "ByLayer") ||
            core::sameTableName(layer.linetype, "ByBlock")) {
            return EntityError{"linetype of a LAYER cannot be ByLayer nor ByBlock"};
        }
    }
    if (const auto value = findValue(record, "lineweight")) {
        const std::optional<int> lineweight = core::parseLineweight(*value);
        if (!lineweight || *lineweight == core::lineweight::kByLayer ||
            *lineweight == core::lineweight::kByBlock) {
            return EntityError{
                "lineweight of a LAYER needs default or an allowed width in mm, found " +
                quoted(*value)};
        }
        layer.lineweight = *lineweight;
    }
    layer.off = hasWord(record, "off");
    layer.frozen = hasWord(record, "frozen");
    layer.locked = hasWord(record, "locked");
    layer.noplot = hasWord(record, "noplot");
    return layer;
}

std::variant<core::LinetypeDefinition, EntityError> readLinetypeDefinition(std::string_view text) {
    constexpr std::array<std::string_view, 3> kNames{"name", "pattern", "description"};
    auto tokens = readRecordTokens("LTYPE", text, kNames, {});
    if (auto* error = std::get_if<EntityError>(&tokens)) {
        return std::move(*error);
    }
    const RecordTokens& record = std::get<RecordTokens>(tokens);
    auto name = recordName("LTYPE", record);
    if (auto* error = std::get_if<EntityError>(&name)) {
        return std::move(*error);
    }
    core::LinetypeDefinition linetype;
    linetype.name = std::get<std::string>(std::move(name));
    if (const auto value = findValue(record, "pattern")) {
        std::optional<core::LinetypePattern> pattern = parsePattern(*value);
        if (!pattern) {
            return EntityError{"pattern needs numbers separated by ',' or continuous, found " +
                               quoted(*value)};
        }
        linetype.pattern = std::move(*pattern);
    } else if (std::optional<core::LinetypeDefinition> standard =
                   core::standardLinetype(linetype.name)) {
        // The standard definitions are written with the name alone
        // (docs/adr/0025, point 2); the name keeps the case it was given.
        linetype.pattern = std::move(standard->pattern);
        linetype.description = std::move(standard->description);
    } else {
        return EntityError{"missing property pattern of LTYPE " + quoted(linetype.name) +
                           ": only a standard linetype is defined by its name alone"};
    }
    if (const auto value = findValue(record, "description")) {
        std::optional<std::string> description = unquoteText(*value);
        if (!description) {
            return EntityError{"description needs a text between double quotes, found " +
                               quoted(*value)};
        }
        linetype.description = std::move(*description);
    }
    return linetype;
}

}  // namespace tg::script
