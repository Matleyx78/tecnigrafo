// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "new_entities_format.hpp"

#include "tg/core/arc.hpp"
#include "tg/core/attribute_definition.hpp"
#include "tg/core/dimension.hpp"
#include "tg/core/ellipse.hpp"
#include "tg/core/generated.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/line.hpp"
#include "tg/core/math.hpp"
#include "tg/core/mtext.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/solid.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/text_format.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

// Angles: the model keeps radians, the lines write degrees, as in the DXF
// groups 50, 52 and 53 and for ARC and TEXT (docs/verifica/rapporto.md).
namespace tg::script::detail {
namespace {

// The names of DXF groups 72 and 73, in the order of their values and of
// core::TextHorizontal and core::TextVertical (docs/verifica/rapporto.md).
constexpr std::array<std::string_view, 6> kHorizontalNames{"left",    "center", "right",
                                                           "aligned", "middle", "fit"};
constexpr std::array<std::string_view, 4> kVerticalNames{"baseline", "bottom", "middle", "top"};

// DXF group 71 of MTEXT, in the order of core::MTextAttachment (1 to 9).
constexpr std::array<std::string_view, 9> kAttachmentNames{
    "top-left",     "top-center",  "top-right",     "middle-left", "middle-center",
    "middle-right", "bottom-left", "bottom-center", "bottom-right"};

// DXF group 75 of HATCH, in the order of core::IslandStyle.
constexpr std::array<std::string_view, 3> kIslandNames{"normal", "outer", "ignore"};

/// A kind of DIMENSION, its name and the DXF type (70 & 7) it has when it is
/// read from a line: the value AutoCAD writes for it.
struct DimensionKindName {
    core::DimensionKind kind;
    std::string_view name;
    int dxfType;
};

// The order of docs/verifica/rapporto.md.
constexpr std::array kDimensionKinds{
    DimensionKindName{core::DimensionKind::Linear, "linear", 0},
    DimensionKindName{core::DimensionKind::Aligned, "aligned", 1},
    DimensionKindName{core::DimensionKind::Angular, "angular", 2},
    DimensionKindName{core::DimensionKind::Radius, "radius", 4},
    DimensionKindName{core::DimensionKind::Diameter, "diameter", 3},
    DimensionKindName{core::DimensionKind::Other, "other", 0},
};

// The largest value of DXF group 70 & 7.
constexpr int kLastDxfDimensionType = 7;

/// A signal of a boundary path, DXF group 92 (docs/adr/0048, point 1).
struct LoopSignal {
    std::string_view name;
    std::uint32_t bit;
};

// The signals written before polyline or edges, in the order of
// docs/verifica/rapporto.md.
constexpr std::array kLoopSignals{LoopSignal{"external", 1U}, LoopSignal{"outermost", 16U},
                                  LoopSignal{"derived", 4U}};

/// The signal of a loop with this name, or null.
const LoopSignal* loopSignalNamed(std::string_view name) {
    for (const LoopSignal& signal : kLoopSignals) {
        if (signal.name == name) {
            return &signal;
        }
    }
    return nullptr;
}

// Bit 2 of group 92: a polyline path; without it the path is made of edges.
constexpr std::uint32_t kPolylineLoop = 2U;

// The default line spacing factor of an MTEXT, DXF group 44.
constexpr double kSingleSpacing = 1.0;

/// Builds one visitor out of several lambdas, one per entity type.
template <typename... Lambdas>
struct Overloaded : Lambdas... {
    using Lambdas::operator()...;
};

// Explicit deduction guide: aggregate CTAD is missing from Apple Clang 15.
template <typename... Lambdas>
Overloaded(Lambdas...) -> Overloaded<Lambdas...>;

std::string_view yesNo(bool value) {
    return value ? "yes" : "no";
}

void writeProperty(std::string_view name, std::string_view value, std::string& out) {
    out += ' ';
    out += name;
    out += '=';
    out += value;
}

void writePoint(std::string_view name, const core::Vec2& point, std::string& out) {
    writeProperty(name, formatPoint(point), out);
}

void writeNumber(std::string_view name, double value, std::string& out) {
    writeProperty(name, formatNumber(value), out);
}

void writeDegrees(std::string_view name, double radians, std::string& out) {
    writeNumber(name, core::radiansToDegrees(radians), out);
}

void writeQuoted(std::string_view name, std::string_view value, std::string& out) {
    writeProperty(name, quoteText(value), out);
}

/// Writes ` name=yes` when `value` is true: the flags whose default is no.
void writeIfYes(std::string_view name, bool value, std::string& out) {
    if (value) {
        writeProperty(name, "yes", out);
    }
}

void writeSolid(const core::Solid& solid, std::string& out) {
    out += "SOLID";
    for (const core::Vec2& vertex : solid.vertices) {
        writePoint("vertex", vertex, out);
    }
}

/// True for the whole ellipse, which DXF writes with the parameters 0 and
/// 2*pi (docs/adr/0052): exact, since these are the values of the format,
/// not the result of a computation.
bool isWholeEllipse(const core::Ellipse& ellipse) {
    return ellipse.startParameter == 0.0 && ellipse.endParameter == core::kTwoPi;
}

void writeEllipse(const core::Ellipse& ellipse, std::string& out) {
    out += "ELLIPSE";
    writePoint("center", ellipse.center, out);
    writePoint("major-axis", ellipse.majorAxis, out);
    writeNumber("ratio", ellipse.ratio, out);
    if (!isWholeEllipse(ellipse)) {
        writeDegrees("start", ellipse.startParameter, out);
        writeDegrees("end", ellipse.endParameter, out);
    }
}

std::string_view attachmentName(core::MTextAttachment attachment) {
    const auto index = static_cast<std::size_t>(attachment) - 1;
    return index < kAttachmentNames.size() ? kAttachmentNames.at(index) : "unknown";
}

void writeMText(const core::MText& mtext, std::string& out) {
    out += "MTEXT";
    writePoint("insertion", mtext.insertion, out);
    writeNumber("height", mtext.height, out);
    writeNumber("width", mtext.width, out);
    writeProperty("attachment", attachmentName(mtext.attachment), out);
    writeDegrees("rotation", mtext.rotation, out);
    writeQuoted("style", mtext.style, out);
    if (mtext.lineSpacing != kSingleSpacing) {
        writeNumber("line-spacing", mtext.lineSpacing, out);
    }
    if (mtext.exactSpacing) {
        writeProperty("line-spacing-style", "exactly", out);
    }
    if (mtext.directionByStyle) {
        writeProperty("direction", "by-style", out);
    }
    writeQuoted("value", mtext.value, out);
}

void writeAttributeDefinition(const core::AttributeDefinition& definition, std::string& out) {
    out += "ATTDEF ";
    writeTextProperties(definition.text, out);
    writeQuoted("tag", definition.tag, out);
    writeQuoted("prompt", definition.prompt, out);
    writeIfYes("invisible", definition.flags.invisible, out);
    writeIfYes("constant", definition.flags.constant, out);
    writeIfYes("verify", definition.flags.verify, out);
    writeIfYes("preset", definition.flags.preset, out);
    writeIfYes("lock-position", definition.lockPosition, out);
}

void writeInsert(const core::Insert& insert, std::string& out) {
    out += "INSERT";
    writeQuoted("block", insert.block, out);
    writePoint("insertion", insert.insertion, out);
    writeProperty("scale", formatNumber(insert.scaleX) + ',' + formatNumber(insert.scaleY), out);
    writeDegrees("rotation", insert.rotation, out);
}

std::string segmentText(const core::Vec2& first, const core::Vec2& second) {
    return formatPoint(first) + ';' + formatPoint(second);
}

std::string_view dimensionKindName(core::DimensionKind kind) {
    for (const DimensionKindName& known : kDimensionKinds) {
        if (known.kind == kind) {
            return known.name;
        }
    }
    return "unknown";
}

/// The overridden variables, NAME:value, in alphabetical order.
std::string overridesText(std::vector<std::pair<std::string, std::string>> overrides) {
    std::ranges::sort(overrides);
    std::string text;
    for (const auto& [name, value] : overrides) {
        text += text.empty() ? "" : ",";
        text += name;
        text += ':';
        text += value;
    }
    return text;
}

void writeDimension(const core::Dimension& dimension, std::string& out) {
    const core::DimensionDefinition& definition = dimension.definition;
    const core::DimensionPresentation& presentation = dimension.presentation;
    out += "DIMENSION";
    writeProperty("kind", dimensionKindName(definition.kind), out);
    writeQuoted("style", presentation.style, out);
    // The definition points of each kind, in the order of the table of
    // docs/verifica/rapporto.md.
    switch (definition.kind) {
        case core::DimensionKind::Linear:
        case core::DimensionKind::Aligned:
            writePoint("origin1", definition.point13, out);
            writePoint("origin2", definition.point14, out);
            writePoint("line-point", definition.linePoint, out);
            if (definition.kind == core::DimensionKind::Linear) {
                writeDegrees("rotation", definition.rotation, out);
            }
            break;
        case core::DimensionKind::Angular:
            writePoint("line-point", definition.linePoint, out);
            writeProperty("first", segmentText(definition.point13, definition.point14), out);
            writeProperty("second", segmentText(definition.linePoint, definition.point15), out);
            writePoint("arc-point", definition.point16, out);
            break;
        case core::DimensionKind::Radius:
            writePoint("center", definition.linePoint, out);
            writePoint("point", definition.point15, out);
            break;
        case core::DimensionKind::Diameter:
            writePoint("point1", definition.point15, out);
            writePoint("point2", definition.linePoint, out);
            break;
        case core::DimensionKind::Other:
            writeNumber("type", definition.dxfType, out);
            break;
    }
    writePoint("text-point", presentation.textPoint, out);
    writeNumber("measurement", dimension.value.measurement, out);
    if (!presentation.textOverride.empty()) {
        writeQuoted("text", presentation.textOverride, out);
    }
    if (presentation.textRotation && *presentation.textRotation != 0.0) {
        writeDegrees("text-rotation", *presentation.textRotation, out);
    }
    writeIfYes("user-text-position", presentation.userTextPosition, out);
    if (!presentation.overrides.empty()) {
        writeProperty("overrides", overridesText(presentation.overrides), out);
    }
}

/// The vertices and bulges of a polyline, as LWPOLYLINE writes them.
void writePolylineVertices(const core::Polyline& polyline, std::string& out) {
    for (const core::PolylineVertex& vertex : polyline.vertices()) {
        writePoint("vertex", vertex.position, out);
        if (vertex.bulge != 0.0) {
            writeNumber("bulge", vertex.bulge, out);
        }
    }
}

/// An arc edge whose angles cover the whole turn: a circle of the boundary.
/// Exact on purpose: core::Arc keeps different angles for an arc
/// (docs/adr/0012), so equal ones, or 0 and 2*pi, can only be a circle.
bool isWholeArc(const core::Arc& arc) {
    return arc.startAngle == arc.endAngle ||
           (arc.startAngle == 0.0 && arc.endAngle == core::kTwoPi);
}

void writeEdge(const core::HatchEdge& edge, std::string& out) {
    std::visit(Overloaded{
                   [&out](const core::Line& line) {
                       writeProperty("line", segmentText(line.start, line.end), out);
                   },
                   [&out](const core::Arc& arc) {
                       std::string value = formatPoint(arc.center) + ';' + formatNumber(arc.radius);
                       if (isWholeArc(arc)) {
                           writeProperty("circle", value, out);
                           return;
                       }
                       value += ';' + formatNumber(core::radiansToDegrees(arc.startAngle)) + ';' +
                                formatNumber(core::radiansToDegrees(arc.endAngle));
                       writeProperty("arc", value, out);
                   },
               },
               edge.curve);
}

void writeHatch(const core::Hatch& hatch, std::string& out) {
    out += "HATCH";
    writeQuoted("pattern", hatch.pattern, out);
    writeProperty("solid", yesNo(hatch.solid), out);
    writeDegrees("angle", hatch.angle, out);
    writeNumber("scale", hatch.scale, out);
    writeIfYes("double", hatch.doubled, out);
    writeProperty("islands", kIslandNames.at(static_cast<std::size_t>(hatch.islands)), out);
    for (const core::HatchLoop& loop : hatch.loops) {
        std::string signals;
        for (const LoopSignal& signal : kLoopSignals) {
            if ((loop.flags & signal.bit) != 0) {
                signals += signal.name;
                signals += '+';
            }
        }
        signals += loop.polyline ? "polyline" : "edges";
        writeProperty("loop", signals, out);
        if (loop.polyline) {
            writePolylineVertices(*loop.polyline, out);
        }
        for (const core::HatchEdge& edge : loop.edges) {
            writeEdge(edge, out);
        }
    }
    for (const core::Vec2& seed : hatch.seeds) {
        writePoint("seed", seed, out);
    }
}

/// Appends `codePoint` in UTF-8.
void appendUtf8(char32_t codePoint, std::string& out) {
    const auto value = static_cast<std::uint32_t>(codePoint);
    const auto byte = [&out](std::uint32_t bits) {
        out += static_cast<char>(bits);
    };
    if (value < 0x80U) {
        byte(value);
    } else if (value < 0x800U) {
        byte(0xC0U | (value >> 6U));
        byte(0x80U | (value & 0x3FU));
    } else if (value < 0x10000U) {
        byte(0xE0U | (value >> 12U));
        byte(0x80U | ((value >> 6U) & 0x3FU));
        byte(0x80U | (value & 0x3FU));
    } else {
        byte(0xF0U | (value >> 18U));
        byte(0x80U | ((value >> 12U) & 0x3FU));
        byte(0x80U | ((value >> 6U) & 0x3FU));
        byte(0x80U | (value & 0x3FU));
    }
}

/// The record with the properties of a new entity: the derived properties
/// describe the entity, not the state of its layer, which can hide the
/// generated geometry (core::generate()).
core::EntityRecord shownRecord(const core::EntityRecord& record) {
    return core::EntityRecord{record.id, record.entity};
}

/// The lines of core::shownLines(), joined by a line feed, in UTF-8.
std::string joinedLines(const std::vector<std::u32string>& lines) {
    std::string text;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i > 0) {
            text += '\n';
        }
        for (const char32_t codePoint : lines[i]) {
            appendUtf8(codePoint, text);
        }
    }
    return text;
}

/// The name of a face in face= (docs/verifica/rapporto.md), as the argument
/// of the questions text-width and font-metrics.
std::string_view faceName(core::FontFace face) {
    switch (face) {
        case core::FontFace::Bold:
            return "bold";
        case core::FontFace::Italic:
            return "italic";
        case core::FontFace::BoldItalic:
            return "bold-italic";
        case core::FontFace::Regular:
            break;
    }
    return "regular";
}

// ---------------------------------------------------------------------------
// Reading

using Error = EntityError;

template <typename T>
using Read = std::variant<T, Error>;

std::string quoted(std::string_view text) {
    return "'" + std::string{text} + "'";
}

struct Property {
    std::string_view name;
    std::string_view value;
};

/// The property written by `token` ("name=value").
Read<Property> splitProperty(std::string_view token) {
    const std::size_t equals = token.find('=');
    if (equals == std::string_view::npos || equals == 0) {
        return Error{"expected <name>=<value>, found " + quoted(token)};
    }
    return Property{token.substr(0, equals), token.substr(equals + 1)};
}

/// The properties of a line, without the derived ones, which are never read.
Read<std::vector<Property>> splitProperties(PropertyTokens tokens) {
    std::vector<Property> properties;
    for (const std::string_view token : tokens) {
        Read<Property> split = splitProperty(token);
        if (auto* error = std::get_if<Error>(&split)) {
            return std::move(*error);
        }
        if (!std::get<Property>(split).name.starts_with('~')) {
            properties.push_back(std::get<Property>(split));
        }
    }
    return properties;
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

/// The position of `value` in `names`.
template <std::size_t Count>
std::optional<std::size_t> indexOf(std::string_view value,
                                   const std::array<std::string_view, Count>& names) {
    const auto found = std::ranges::find(names, value);
    if (found == names.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(found - names.begin());
}

template <std::size_t Count>
std::string optionsText(const std::array<std::string_view, Count>& names) {
    std::string text;
    for (const std::string_view name : names) {
        text += text.empty() ? "" : ", ";
        text += name;
    }
    return text;
}

/// What a number must be.
enum class NumberRule : std::uint8_t { Any, Positive, NonZero };

/// A segment "x1,y1;x2,y2".
struct Segment {
    core::Vec2 first;
    core::Vec2 second;
};

/// The properties of an entity each given at most once: the reader of
/// ELLIPSE, MTEXT, INSERT and DIMENSION. Every getter leaves its target as
/// it is when the property is left out, so that the target keeps its
/// default, and gives the error of a value that cannot be read.
class Fields {
public:
    Fields(std::string_view type, std::span<const std::string_view> names)
        : m_type(type), m_names(names), m_values(names.size()) {}

    std::optional<Error> read(PropertyTokens tokens) {
        Read<std::vector<Property>> split = splitProperties(tokens);
        if (auto* error = std::get_if<Error>(&split)) {
            return std::move(*error);
        }
        for (const Property& property : std::get<std::vector<Property>>(split)) {
            if (std::optional<Error> error = add(property)) {
                return error;
            }
        }
        return std::nullopt;
    }

    /// Records one property, which must be one of the names, given once.
    std::optional<Error> add(const Property& property) {
        const auto known = std::ranges::find(m_names, property.name);
        if (known == m_names.end()) {
            return Error{"unknown property " + quoted(property.name) + " of " +
                         std::string{m_type}};
        }
        std::optional<std::string_view>& slot =
            m_values[static_cast<std::size_t>(known - m_names.begin())];
        if (slot) {
            return Error{std::string{property.name} + " is given more than once"};
        }
        slot = property.value;
        return std::nullopt;
    }

    [[nodiscard]] std::optional<std::string_view> value(std::string_view name) const {
        const auto known = std::ranges::find(m_names, name);
        if (known == m_names.end()) {
            return std::nullopt;
        }
        return m_values[static_cast<std::size_t>(known - m_names.begin())];
    }

    [[nodiscard]] bool has(std::string_view name) const { return value(name).has_value(); }

    /// The error of a required property left out.
    [[nodiscard]] std::optional<Error> require(std::span<const std::string_view> names) const {
        for (const std::string_view name : names) {
            if (!has(name)) {
                return Error{"missing property " + std::string{name} + " of " +
                             std::string{m_type}};
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<Error> require(
        std::initializer_list<std::string_view> names) const {
        return require(std::span{names.begin(), names.size()});
    }

    std::optional<Error> point(std::string_view name, core::Vec2& target) const {
        const auto text = value(name);
        if (!text) {
            return std::nullopt;
        }
        const std::optional<core::Vec2> point = parsePoint(*text);
        if (!point) {
            return Error{std::string{name} + " needs a point x,y, found " + quoted(*text)};
        }
        target = *point;
        return std::nullopt;
    }

    std::optional<Error> number(std::string_view name, double& target,
                                NumberRule rule = NumberRule::Any) const {
        const auto text = value(name);
        if (!text) {
            return std::nullopt;
        }
        const std::optional<double> number = parseNumber(*text);
        if (!number || (rule == NumberRule::Positive && !(*number > 0.0)) ||
            (rule == NumberRule::NonZero && *number == 0.0)) {
            std::string_view expected = "a number";
            if (rule == NumberRule::Positive) {
                expected = "a number above 0";
            } else if (rule == NumberRule::NonZero) {
                expected = "a number other than 0";
            }
            return Error{std::string{name} + " needs " + std::string{expected} + ", found " +
                         quoted(*text)};
        }
        target = *number;
        return std::nullopt;
    }

    /// An angle in degrees, any value, converted to radians as it is.
    std::optional<Error> degrees(std::string_view name, double& target) const {
        const auto text = value(name);
        if (!text) {
            return std::nullopt;
        }
        const std::optional<double> number = parseNumber(*text);
        if (!number) {
            return Error{std::string{name} + " needs degrees, found " + quoted(*text)};
        }
        target = core::degreesToRadians(*number);
        return std::nullopt;
    }

    std::optional<Error> text(std::string_view name, std::string& target) const {
        const auto text = value(name);
        if (!text) {
            return std::nullopt;
        }
        std::optional<std::string> unquoted = unquoteText(*text);
        if (!unquoted) {
            return Error{std::string{name} + " needs a text between double quotes, found " +
                         quoted(*text)};
        }
        target = std::move(*unquoted);
        return std::nullopt;
    }

    std::optional<Error> flag(std::string_view name, bool& target) const {
        const auto text = value(name);
        if (!text) {
            return std::nullopt;
        }
        const std::optional<bool> flag = parseYesNo(*text);
        if (!flag) {
            return Error{std::string{name} + " needs yes or no, found " + quoted(*text)};
        }
        target = *flag;
        return std::nullopt;
    }

    /// One of `names`; `target` is its position.
    template <std::size_t Count>
    std::optional<Error> option(std::string_view name,
                                const std::array<std::string_view, Count>& names,
                                std::size_t& target) const {
        const auto text = value(name);
        if (!text) {
            return std::nullopt;
        }
        const std::optional<std::size_t> index = indexOf(*text, names);
        if (!index) {
            return Error{std::string{name} + " needs one of " + optionsText(names) + ", found " +
                         quoted(*text)};
        }
        target = *index;
        return std::nullopt;
    }

    std::optional<Error> segment(std::string_view name, Segment& target) const {
        const auto text = value(name);
        if (!text) {
            return std::nullopt;
        }
        const std::size_t semicolon = text->find(';');
        const std::optional<core::Vec2> first = parsePoint(text->substr(0, semicolon));
        const std::optional<core::Vec2> second = semicolon == std::string_view::npos
                                                     ? std::nullopt
                                                     : parsePoint(text->substr(semicolon + 1));
        if (!first || !second) {
            return Error{std::string{name} + " needs two points x1,y1;x2,y2, found " +
                         quoted(*text)};
        }
        target = Segment{*first, *second};
        return std::nullopt;
    }

private:
    std::string_view m_type;
    std::span<const std::string_view> m_names;
    std::vector<std::optional<std::string_view>> m_values;
};

/// The first error of `errors`, in the order of the properties.
std::optional<Error> firstError(std::initializer_list<std::optional<Error>> errors) {
    for (const std::optional<Error>& error : errors) {
        if (error) {
            return error;
        }
    }
    return std::nullopt;
}

Read<core::Entity> readSolid(PropertyTokens tokens) {
    // Always four vertices, in the order of DXF groups 10, 11, 12 and 13.
    Read<std::vector<Property>> split = splitProperties(tokens);
    if (auto* error = std::get_if<Error>(&split)) {
        return std::move(*error);
    }
    core::Solid solid;
    std::size_t count = 0;
    for (const Property& property : std::get<std::vector<Property>>(split)) {
        if (property.name != "vertex") {
            return Error{"unknown property " + quoted(property.name) + " of SOLID"};
        }
        const std::optional<core::Vec2> point = parsePoint(property.value);
        if (!point) {
            return Error{"vertex needs a point x,y, found " + quoted(property.value)};
        }
        if (count == solid.vertices.size()) {
            return Error{"a SOLID has four vertices"};
        }
        solid.vertices.at(count) = *point;
        ++count;
    }
    if (count != solid.vertices.size()) {
        return Error{"a SOLID has four vertices"};
    }
    return core::Entity{solid};
}

Read<core::Entity> readEllipse(PropertyTokens tokens) {
    constexpr std::array<std::string_view, 5> kNames{"center", "major-axis", "ratio", "start",
                                                     "end"};
    Fields fields{"ELLIPSE", kNames};
    if (auto error = fields.read(tokens)) {
        return std::move(*error);
    }
    if (auto error = fields.require({"center", "major-axis", "ratio"})) {
        return std::move(*error);
    }
    if (fields.has("start") != fields.has("end")) {
        return Error{"start and end of an ELLIPSE are given together, or left out together"};
    }
    // Left out, start and end are those of the whole ellipse.
    core::Ellipse ellipse{{}, {}, 1.0, 0.0, core::kTwoPi};
    if (auto error = firstError({fields.point("center", ellipse.center),
                                 fields.point("major-axis", ellipse.majorAxis),
                                 fields.number("ratio", ellipse.ratio, NumberRule::Positive),
                                 fields.degrees("start", ellipse.startParameter),
                                 fields.degrees("end", ellipse.endParameter)})) {
        return std::move(*error);
    }
    // The checks of the DXF format (docs/adr/0052): a ratio in (0, 1] and a
    // major axis that has a length.
    if (ellipse.ratio > 1.0) {
        return Error{"ratio needs a number in (0, 1], found " +
                     quoted(fields.value("ratio").value_or(""))};
    }
    if (ellipse.majorAxis == core::Vec2{}) {
        return Error{"major-axis needs a vector other than 0,0"};
    }
    return core::Entity{ellipse};
}

Read<core::Entity> readMText(PropertyTokens tokens) {
    constexpr std::array<std::string_view, 10> kNames{
        "insertion", "height", "width",        "attachment",
        "rotation",  "style",  "line-spacing", "line-spacing-style",
        "direction", "value"};
    constexpr std::array<std::string_view, 2> kSpacingStyles{"at-least", "exactly"};
    constexpr std::array<std::string_view, 2> kDirections{"left-to-right", "by-style"};
    Fields fields{"MTEXT", kNames};
    if (auto error = fields.read(tokens)) {
        return std::move(*error);
    }
    if (auto error = fields.require({"insertion", "value"})) {
        return std::move(*error);
    }
    core::MText mtext;
    std::size_t attachment = static_cast<std::size_t>(mtext.attachment) - 1;
    std::size_t spacingStyle = 0;
    std::size_t direction = 0;
    double width = mtext.width;
    if (auto error = firstError(
            {fields.point("insertion", mtext.insertion),
             fields.number("height", mtext.height, NumberRule::Positive),
             fields.number("width", width),
             fields.option("attachment", kAttachmentNames, attachment),
             fields.degrees("rotation", mtext.rotation), fields.text("style", mtext.style),
             fields.number("line-spacing", mtext.lineSpacing, NumberRule::Positive),
             fields.option("line-spacing-style", kSpacingStyles, spacingStyle),
             fields.option("direction", kDirections, direction),
             fields.text("value", mtext.value)})) {
        return std::move(*error);
    }
    if (width < 0.0) {
        return Error{"width needs a number not below 0, found " +
                     quoted(fields.value("width").value_or(""))};
    }
    mtext.width = width;
    mtext.attachment = static_cast<core::MTextAttachment>(attachment + 1);
    mtext.exactSpacing = spacingStyle == 1;
    mtext.directionByStyle = direction == 1;
    return core::Entity{std::move(mtext)};
}

Read<core::Entity> readAttributeDefinition(PropertyTokens tokens) {
    // The properties of TEXT, read by its reader, and those of ATTDEF.
    constexpr std::array<std::string_view, 6> kOwnNames{"tag",      "prompt", "invisible",
                                                        "constant", "verify", "preset"};
    std::vector<std::string_view> ownTokens;
    std::vector<std::string_view> textTokens;
    for (const std::string_view token : tokens) {
        const std::string_view name = token.substr(0, token.find('='));
        const bool own = std::ranges::find(kOwnNames, name) != kOwnNames.end();
        (own ? ownTokens : textTokens).push_back(token);
    }
    Fields fields{"ATTDEF", kOwnNames};
    if (auto error = fields.read(ownTokens)) {
        return std::move(*error);
    }
    Read<core::Text> text = readTextProperties(textTokens);
    if (auto* error = std::get_if<Error>(&text)) {
        // The reader of TEXT names its own type.
        constexpr std::string_view kTextType = "of TEXT";
        if (const std::size_t at = error->message.rfind(kTextType); at != std::string::npos) {
            error->message.replace(at, kTextType.size(), "of ATTDEF");
        }
        return std::move(*error);
    }
    if (auto error = fields.require({"tag"})) {
        return std::move(*error);
    }
    core::AttributeDefinition definition;
    definition.text = std::get<core::Text>(std::move(text));
    if (auto error = firstError({fields.text("tag", definition.tag),
                                 fields.text("prompt", definition.prompt),
                                 fields.flag("invisible", definition.flags.invisible),
                                 fields.flag("constant", definition.flags.constant),
                                 fields.flag("verify", definition.flags.verify),
                                 fields.flag("preset", definition.flags.preset)})) {
        return std::move(*error);
    }
    if (definition.tag.empty()) {
        return Error{"tag of an ATTDEF cannot be empty"};
    }
    return core::Entity{std::move(definition)};
}

Read<core::Entity> readInsert(PropertyTokens tokens) {
    constexpr std::array<std::string_view, 4> kNames{"block", "insertion", "scale", "rotation"};
    Fields fields{"INSERT", kNames};
    if (auto error = fields.read(tokens)) {
        return std::move(*error);
    }
    if (auto error = fields.require({"block", "insertion"})) {
        return std::move(*error);
    }
    core::Insert insert;
    if (auto error = firstError({fields.text("block", insert.block),
                                 fields.point("insertion", insert.insertion),
                                 fields.degrees("rotation", insert.rotation)})) {
        return std::move(*error);
    }
    if (insert.block.empty()) {
        return Error{"block of an INSERT cannot be empty"};
    }
    if (const auto scale = fields.value("scale")) {
        // A scale of 0 has no image; a negative one mirrors, as in DXF.
        const std::optional<core::Vec2> factors = parsePoint(*scale);
        if (!factors || factors->x == 0.0 || factors->y == 0.0) {
            return Error{"scale needs two numbers x,y other than 0, found " + quoted(*scale)};
        }
        insert.scaleX = factors->x;
        insert.scaleY = factors->y;
    }
    return core::Entity{std::move(insert)};
}

/// "NAME:value,NAME:value", each name once.
Read<std::vector<std::pair<std::string, std::string>>> readOverrides(std::string_view text) {
    std::vector<std::pair<std::string, std::string>> overrides;
    while (true) {
        const std::size_t comma = text.find(',');
        const std::string_view item = text.substr(0, comma);
        const std::size_t colon = item.find(':');
        if (colon == std::string_view::npos || colon == 0 || colon + 1 == item.size()) {
            return Error{"overrides needs NAME:value items separated by ',', found " +
                         quoted(item)};
        }
        std::string name{item.substr(0, colon)};
        if (std::ranges::find(overrides, name, &std::pair<std::string, std::string>::first) !=
            overrides.end()) {
            return Error{"the override " + quoted(name) + " is given more than once"};
        }
        overrides.emplace_back(std::move(name), std::string{item.substr(colon + 1)});
        if (comma == std::string_view::npos) {
            return overrides;
        }
        text.remove_prefix(comma + 1);
    }
}

Read<core::Entity> readDimension(PropertyTokens tokens) {
    constexpr std::array<std::string_view, 20> kNames{"kind",
                                                      "style",
                                                      "origin1",
                                                      "origin2",
                                                      "line-point",
                                                      "rotation",
                                                      "first",
                                                      "second",
                                                      "arc-point",
                                                      "center",
                                                      "point",
                                                      "point1",
                                                      "point2",
                                                      "type",
                                                      "text-point",
                                                      "measurement",
                                                      "text",
                                                      "text-rotation",
                                                      "user-text-position",
                                                      "overrides"};
    Fields fields{"DIMENSION", kNames};
    if (auto error = fields.read(tokens)) {
        return std::move(*error);
    }
    if (auto error = fields.require({"kind"})) {
        return std::move(*error);
    }
    std::array<std::string_view, kDimensionKinds.size()> kindNames{};
    std::ranges::transform(kDimensionKinds, kindNames.begin(), &DimensionKindName::name);
    std::size_t kindIndex = 0;
    if (auto error = fields.option("kind", kindNames, kindIndex)) {
        return std::move(*error);
    }
    const DimensionKindName& kind = kDimensionKinds.at(kindIndex);

    // The points of each kind (docs/verifica/rapporto.md): those of the
    // kind are required, those of the other kinds are refused.
    static constexpr std::array<std::string_view, 3> kTwoOrigins{"origin1", "origin2",
                                                                 "line-point"};
    static constexpr std::array<std::string_view, 3> kAngular{"first", "second", "arc-point"};
    static constexpr std::array<std::string_view, 2> kRadius{"center", "point"};
    static constexpr std::array<std::string_view, 2> kDiameter{"point1", "point2"};
    static constexpr std::array<std::string_view, 1> kOther{"type"};
    std::span<const std::string_view> required;
    switch (kind.kind) {
        case core::DimensionKind::Linear:
        case core::DimensionKind::Aligned:
            required = kTwoOrigins;
            break;
        case core::DimensionKind::Angular:
            required = kAngular;
            break;
        case core::DimensionKind::Radius:
            required = kRadius;
            break;
        case core::DimensionKind::Diameter:
            required = kDiameter;
            break;
        case core::DimensionKind::Other:
            required = kOther;
            break;
    }
    if (auto error = fields.require(required)) {
        return std::move(*error);
    }
    if (auto error = fields.require({"text-point", "measurement"})) {
        return std::move(*error);
    }
    constexpr std::array<std::string_view, 12> kPointNames{
        "origin1",   "origin2", "line-point", "rotation", "first",  "second",
        "arc-point", "center",  "point",      "point1",   "point2", "type"};
    for (const std::string_view name : kPointNames) {
        const bool own = std::ranges::find(required, name) != required.end() ||
                         (kind.kind == core::DimensionKind::Linear && name == "rotation") ||
                         (kind.kind == core::DimensionKind::Angular && name == "line-point");
        if (!own && fields.has(name)) {
            return Error{std::string{name} +
                         " is not a property of DIMENSION kind=" + std::string{kind.name}};
        }
    }

    core::Dimension dimension;
    core::DimensionDefinition& definition = dimension.definition;
    core::DimensionPresentation& presentation = dimension.presentation;
    definition.kind = kind.kind;
    definition.dxfType = kind.dxfType;
    Segment first;
    Segment second;
    double type = 0.0;
    double textRotation = 0.0;
    if (auto error = firstError(
            {fields.text("style", presentation.style), fields.point("origin1", definition.point13),
             fields.point("origin2", definition.point14),
             fields.point("line-point", definition.linePoint),
             fields.degrees("rotation", definition.rotation), fields.segment("first", first),
             fields.segment("second", second), fields.point("arc-point", definition.point16),
             fields.point("center", definition.linePoint),
             fields.point("point", definition.point15), fields.point("point1", definition.point15),
             fields.point("point2", definition.linePoint), fields.number("type", type),
             fields.point("text-point", presentation.textPoint),
             fields.number("measurement", dimension.value.measurement),
             fields.text("text", presentation.textOverride),
             fields.degrees("text-rotation", textRotation),
             fields.flag("user-text-position", presentation.userTextPosition)})) {
        return std::move(*error);
    }
    if (kind.kind == core::DimensionKind::Angular) {
        // DXF group 10 is both the line point and the first point of the
        // second line: written twice, it is the same point.
        if (fields.has("line-point") && definition.linePoint != second.first) {
            return Error{"line-point of an angular DIMENSION is the first point of second"};
        }
        definition.point13 = first.first;
        definition.point14 = first.second;
        definition.linePoint = second.first;
        definition.point15 = second.second;
    }
    if (kind.kind == core::DimensionKind::Other) {
        const auto whole = static_cast<int>(type);
        if (static_cast<double>(whole) != type || whole < 0 || whole > kLastDxfDimensionType) {
            return Error{"type needs an integer from 0 to 7, found " +
                         quoted(fields.value("type").value_or(""))};
        }
        definition.dxfType = whole;
    }
    if (fields.has("text-rotation")) {
        presentation.textRotation = textRotation;
    }
    if (const auto overrides = fields.value("overrides")) {
        auto read = readOverrides(*overrides);
        if (auto* error = std::get_if<Error>(&read)) {
            return std::move(*error);
        }
        presentation.overrides =
            std::get<std::vector<std::pair<std::string, std::string>>>(std::move(read));
    }
    return core::Entity{std::move(dimension)};
}

/// The signals of a boundary path ("external+polyline"): group 92.
Read<std::uint32_t> readLoopSignals(std::string_view text) {
    std::uint32_t flags = 0;
    std::vector<std::string_view> seen;
    int kinds = 0;
    while (true) {
        const std::size_t plus = text.find('+');
        const std::string_view name = text.substr(0, plus);
        if (std::ranges::find(seen, name) != seen.end()) {
            return Error{"the signal " + quoted(name) + " of loop is given more than once"};
        }
        seen.push_back(name);
        if (const LoopSignal* const signal = loopSignalNamed(name); signal != nullptr) {
            flags |= signal->bit;
        } else if (name == "polyline") {
            flags |= kPolylineLoop;
            ++kinds;
        } else if (name == "edges") {
            ++kinds;
        } else {
            return Error{
                "loop needs signals among external, outermost, derived, polyline, "
                "edges joined by '+', found " +
                quoted(name)};
        }
        if (plus == std::string_view::npos) {
            break;
        }
        text.remove_prefix(plus + 1);
    }
    if (kinds != 1) {
        return Error{"loop needs exactly one of polyline and edges"};
    }
    return flags;
}

/// Splits "a;b;c" into its parts.
std::vector<std::string_view> splitParts(std::string_view text) {
    std::vector<std::string_view> parts;
    while (true) {
        const std::size_t semicolon = text.find(';');
        parts.push_back(text.substr(0, semicolon));
        if (semicolon == std::string_view::npos) {
            return parts;
        }
        text.remove_prefix(semicolon + 1);
    }
}

/// An edge of a boundary path: "line=x1,y1;x2,y2", "arc=cx,cy;r;a1;a2",
/// "circle=cx,cy;r".
Read<core::HatchEdge> readEdge(const Property& property) {
    const std::vector<std::string_view> parts = splitParts(property.value);
    const auto failed = [&](std::string_view form) {
        return Error{std::string{property.name} + " needs " + std::string{form} + ", found " +
                     quoted(property.value)};
    };
    if (property.name == "line") {
        const auto start = parts.size() == 2 ? parsePoint(parts[0]) : std::nullopt;
        const auto end = parts.size() == 2 ? parsePoint(parts[1]) : std::nullopt;
        if (!start || !end) {
            return failed("x1,y1;x2,y2");
        }
        return core::HatchEdge{core::Line{*start, *end}};
    }
    const std::size_t count = property.name == "arc" ? 4 : 2;
    const auto center = parts.size() == count ? parsePoint(parts[0]) : std::nullopt;
    const auto radius = parts.size() == count ? parseNumber(parts[1]) : std::nullopt;
    if (!center || !radius || !(*radius > 0.0)) {
        return failed(count == 4 ? "cx,cy;r;a1;a2 with r above 0" : "cx,cy;r with r above 0");
    }
    if (count == 2) {
        // A whole turn, as DXF writes a circle of a boundary: 0 to 360 degrees.
        return core::HatchEdge{core::Arc{*center, *radius, 0.0, core::kTwoPi}};
    }
    // The angles of an ARC: degrees in [0, 360), different.
    constexpr double kFullTurnDegrees = 360.0;
    const auto start = parseNumber(parts[2]);
    const auto end = parseNumber(parts[3]);
    const auto inTurn = [&](const std::optional<double>& angle) {
        return angle && *angle >= 0.0 && *angle < kFullTurnDegrees;
    };
    if (!start || !end || !inTurn(start) || !inTurn(end) || *start == *end) {
        return failed("cx,cy;r;a1;a2 with different angles in [0, 360)");
    }
    return core::HatchEdge{
        core::Arc{*center, *radius, core::degreesToRadians(*start), core::degreesToRadians(*end)}};
}

/// A boundary path being read.
struct LoopBeingRead {
    std::uint32_t flags{0};
    std::vector<core::PolylineVertex> vertices;
    std::vector<core::HatchEdge> edges;
    bool bulgeGiven{false};  ///< for the last vertex
};

Read<core::Entity> readHatch(PropertyTokens tokens) {
    // pattern … islands are given once each, in any order; loop starts a
    // boundary path, followed by its elements; seed may come anywhere.
    constexpr std::array<std::string_view, 6> kNames{"pattern", "solid",  "angle",
                                                     "scale",   "double", "islands"};
    Read<std::vector<Property>> split = splitProperties(tokens);
    if (auto* error = std::get_if<Error>(&split)) {
        return std::move(*error);
    }
    Fields fields{"HATCH", kNames};
    std::vector<LoopBeingRead> loops;
    std::vector<core::Vec2> seeds;
    for (const Property& property : std::get<std::vector<Property>>(split)) {
        if (std::ranges::find(kNames, property.name) != kNames.end()) {
            if (auto error = fields.add(property)) {
                return std::move(*error);
            }
        } else if (property.name == "loop") {
            Read<std::uint32_t> flags = readLoopSignals(property.value);
            if (auto* error = std::get_if<Error>(&flags)) {
                return std::move(*error);
            }
            loops.push_back(LoopBeingRead{std::get<std::uint32_t>(flags), {}, {}, false});
        } else if (property.name == "seed") {
            const std::optional<core::Vec2> seed = parsePoint(property.value);
            if (!seed) {
                return Error{"seed needs a point x,y, found " + quoted(property.value)};
            }
            seeds.push_back(*seed);
        } else if (property.name == "vertex" || property.name == "bulge") {
            if (loops.empty() || (loops.back().flags & kPolylineLoop) == 0) {
                return Error{std::string{property.name} + " belongs to a loop of polyline"};
            }
            LoopBeingRead& loop = loops.back();
            if (property.name == "vertex") {
                const std::optional<core::Vec2> point = parsePoint(property.value);
                if (!point) {
                    return Error{"vertex needs a point x,y, found " + quoted(property.value)};
                }
                loop.vertices.push_back({*point, 0.0});
                loop.bulgeGiven = false;
                continue;
            }
            const std::optional<double> bulge = parseNumber(property.value);
            if (loop.vertices.empty() || loop.bulgeGiven || !bulge) {
                return Error{"bulge is a number given at most once after each vertex"};
            }
            loop.vertices.back().bulge = *bulge;
            loop.bulgeGiven = true;
        } else if (property.name == "line" || property.name == "arc" || property.name == "circle") {
            if (loops.empty() || (loops.back().flags & kPolylineLoop) != 0) {
                return Error{std::string{property.name} + " belongs to a loop of edges"};
            }
            Read<core::HatchEdge> edge = readEdge(property);
            if (auto* error = std::get_if<Error>(&edge)) {
                return std::move(*error);
            }
            loops.back().edges.push_back(std::get<core::HatchEdge>(std::move(edge)));
        } else {
            return Error{"unknown property " + quoted(property.name) + " of HATCH"};
        }
    }
    if (auto error = fields.require({"pattern"})) {
        return std::move(*error);
    }
    core::Hatch hatch;
    std::size_t islands = 0;
    if (auto error =
            firstError({fields.text("pattern", hatch.pattern), fields.flag("solid", hatch.solid),
                        fields.degrees("angle", hatch.angle),
                        fields.number("scale", hatch.scale, NumberRule::Positive),
                        fields.flag("double", hatch.doubled),
                        fields.option("islands", kIslandNames, islands)})) {
        return std::move(*error);
    }
    hatch.islands = static_cast<core::IslandStyle>(islands);
    for (LoopBeingRead& loop : loops) {
        core::HatchLoop path;
        path.flags = loop.flags;
        if ((loop.flags & kPolylineLoop) != 0) {
            // A polyline path is always closed (DXF group 73 of the path).
            path.polyline = core::Polyline{std::move(loop.vertices), true};
        }
        path.edges = std::move(loop.edges);
        hatch.loops.push_back(std::move(path));
    }
    hatch.seeds = std::move(seeds);
    return core::Entity{std::move(hatch)};
}

}  // namespace

void writeTextProperties(const core::Text& text, std::string& out) {
    // Every property, in the order of docs/verifica/rapporto.md.
    out += "position=";
    out += formatPoint(text.position);
    writePoint("alignment-point", text.alignmentPoint, out);
    writeNumber("height", text.height, out);
    writeDegrees("rotation", text.rotation, out);
    writeNumber("width-factor", text.widthFactor, out);
    writeDegrees("oblique", text.obliqueAngle, out);
    writeQuoted("style", text.style, out);
    writeProperty("mirror-x", yesNo(text.mirroredX), out);
    writeProperty("mirror-y", yesNo(text.mirroredY), out);
    const auto horizontal = static_cast<std::size_t>(text.horizontal);
    const auto vertical = static_cast<std::size_t>(text.vertical);
    writeProperty(
        "horizontal",
        horizontal < kHorizontalNames.size() ? kHorizontalNames.at(horizontal) : "unknown", out);
    writeProperty("vertical",
                  vertical < kVerticalNames.size() ? kVerticalNames.at(vertical) : "unknown", out);
    writeQuoted("value", text.value, out);
}

namespace {

void writeBoxCorners(const std::array<core::Vec2, 4>& box, std::string& out) {
    std::string corners;
    for (const core::Vec2& corner : box) {
        if (!corners.empty()) {
            corners += ';';
        }
        corners += formatPoint(corner);
    }
    writeProperty("~box", corners, out);
}

}  // namespace

void writeTextBox(const core::Text& text, std::string& out) {
    writeBoxCorners(core::textBox(text), out);
}

void writeFormattedText(const core::FormattedText& piece, std::string& out) {
    out += "TEXT ";
    writeTextProperties(piece.text, out);
    writeProperty("face", faceName(piece.face), out);
}

void writeFormattedTextBox(const core::FormattedText& piece, std::string& out) {
    writeBoxCorners(core::formattedTextBox(piece), out);
}

void writeNewEntity(const core::Entity& entity, std::string& out) {
    std::visit(Overloaded{
                   [&out](const core::Solid& solid) { writeSolid(solid, out); },
                   [&out](const core::Ellipse& ellipse) { writeEllipse(ellipse, out); },
                   [&out](const core::MText& mtext) { writeMText(mtext, out); },
                   [&out](const core::AttributeDefinition& definition) {
                       writeAttributeDefinition(definition, out);
                   },
                   [&out](const core::Insert& insert) { writeInsert(insert, out); },
                   [&out](const core::Dimension& dimension) { writeDimension(dimension, out); },
                   [&out](const core::Hatch& hatch) { writeHatch(hatch, out); },
                   // The types of before the 0.8 are written by report.cpp.
                   [](const auto& /*older*/) {},
               },
               entity);
}

void writeAttribute(const core::Attribute& attribute, core::EntityId owner, std::string& out) {
    out += "ATTRIB";
    writeProperty("owner", std::to_string(owner.value), out);
    out += ' ';
    writeTextProperties(attribute.text, out);
    writeQuoted("tag", attribute.tag, out);
    writeIfYes("invisible", attribute.invisible, out);
    writeIfYes("lock-position", attribute.lockPosition, out);
}

void writeNewDerived(const core::Entity& entity, std::string& out) {
    // In the drawing an ATTDEF shows its tag (docs/adr/0045, point 6): its box
    // is that of the text it shows, not of its default value.
    if (const auto* definition = std::get_if<core::AttributeDefinition>(&entity)) {
        writeTextBox(core::shownText(*definition), out);
    }
}

void writeGeneratedDerived(const core::Document& document, const core::EntityRecord& record,
                           std::string& out) {
    const bool lines = std::holds_alternative<core::MText>(record.entity);
    const bool text = std::holds_alternative<core::Dimension>(record.entity);
    if ((!lines && !text) || !core::isSupported(record.entity)) {
        return;
    }
    // The rows of the layout, not the pieces of the generated geometry
    // (docs/tappe/0.8.1/PT8-impaginazione-formati.md, point 10).
    const std::vector<std::u32string> shown = core::shownLines(document, shownRecord(record));
    if (lines) {
        writeProperty("~lines", std::to_string(shown.size()), out);
    } else {
        writeQuoted("~text", joinedLines(shown), out);
    }
}

std::optional<std::variant<core::Entity, EntityError>> readNewEntity(std::string_view type,
                                                                     PropertyTokens properties) {
    if (type == "SOLID") {
        return readSolid(properties);
    }
    if (type == "ELLIPSE") {
        return readEllipse(properties);
    }
    if (type == "MTEXT") {
        return readMText(properties);
    }
    if (type == "ATTDEF") {
        return readAttributeDefinition(properties);
    }
    if (type == "INSERT") {
        return readInsert(properties);
    }
    if (type == "DIMENSION") {
        return readDimension(properties);
    }
    if (type == "HATCH") {
        return readHatch(properties);
    }
    return std::nullopt;
}

}  // namespace tg::script::detail
