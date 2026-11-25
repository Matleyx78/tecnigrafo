// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "question_table.hpp"
#include "tg/cmd/quick_select.hpp"
#include "tg/core/dashes.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/core/entity_properties.hpp"
#include "tg/core/math.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/report.hpp"
#include "tg/script/text_format.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace tg::script::detail {
namespace {

/// The lines separated by "; ", or "none" for an empty list.
std::string joinItems(const std::vector<std::string>& lines) {
    if (lines.empty()) {
        return "none";
    }
    std::string text;
    for (const std::string& line : lines) {
        if (!text.empty()) {
            text += "; ";
        }
        text += line;
    }
    return text;
}

/// The question "<question> <drawing>" about a drawing read from a file,
/// answered with the lines `lines` gives for its document.
QuestionResult askAboutTable(const QuestionModel& model,
                             std::span<const std::string_view> arguments, std::string_view form,
                             std::vector<std::string> (*lines)(const core::Document&)) {
    if (arguments.size() != 1) {
        return ArgumentError{"wrong arguments, expected: " + std::string{form}};
    }
    const auto found = model.files.find(arguments[0]);
    if (found == model.files.end()) {
        return ArgumentError{"no drawing named '" + std::string{arguments[0]} + "' is read above"};
    }
    return joinItems(lines(found->second.read.document));
}

// ---------------------------------------------------------------------------
// properties and set-property (docs/verifica/domande.md, "Domande della tappa
// 0.8.1"; docs/tappe/0.8.1/PT22-proprieta-core.md, point 6).

/// The entity a question about own properties is about: a definition, or an
/// entity of a drawing read from a file, with the document it belongs to.
struct PropertyTarget {
    const core::Document* document{nullptr};
    const core::EntityRecord* record{nullptr};
    bool fromFile{false};
};

/// The record of "<E>" (one argument) or "<drawing> <handle>" (two), or the
/// error of the question.
std::variant<PropertyTarget, ArgumentError> propertyTarget(const QuestionModel& model,
                                                           std::span<const std::string_view> names,
                                                           std::string_view form) {
    if (names.size() == 1) {
        const auto id = model.find(names.front());
        if (!id) {
            return ArgumentError{"no entity named '" + std::string{names.front()} +
                                 "' is defined above"};
        }
        return PropertyTarget{&model.document, model.document.find(*id), false};
    }
    if (names.size() != 2) {
        return ArgumentError{"wrong arguments, expected: " + std::string{form}};
    }
    const auto drawing = model.files.find(names[0]);
    if (drawing == model.files.end()) {
        return ArgumentError{"no drawing named '" + std::string{names[0]} + "' is read above"};
    }
    // A handle written in hexadecimal, in either case, never 0.
    const std::string_view text = names[1];
    std::uint64_t handle = 0;
    const char* end = text.data() + text.size();
    constexpr int kHexadecimal = 16;
    const auto [stop, error] = std::from_chars(text.data(), end, handle, kHexadecimal);
    if (text.empty() || error != std::errc{} || stop != end || handle == 0) {
        return ArgumentError{"wrong arguments, expected: " + std::string{form}};
    }
    const core::Document& document = drawing->second.read.document;
    if (const core::EntityRecord* record = document.find(core::EntityId{handle})) {
        return PropertyTarget{&document, record, true};
    }
    const auto raw =
        std::ranges::find(document.rawEntities(), core::EntityId{handle}, &core::RawEntity::id);
    if (raw != document.rawEntities().end()) {
        // Kept unhandled by io: a later package may handle its type.
        return ArgumentError{"entity type not implemented yet", true};
    }
    return ArgumentError{std::string{names[0]} + " has no entity with handle '" +
                         std::string{text} + "'"};
}

/// True for the five types of before the 0.8, which have no own properties
/// ("none"); a type of the 0.8 without them waits for its package.
bool isTypeBefore08(const core::Entity& entity) {
    return std::holds_alternative<core::Polyline>(entity) ||
           std::holds_alternative<core::Line>(entity) ||
           std::holds_alternative<core::Circle>(entity) ||
           std::holds_alternative<core::Arc>(entity) || std::holds_alternative<core::Text>(entity);
}

/// The own properties of `target`, or the error of a type of the 0.8 whose
/// package has not written them yet (docs/verifica/domande.md).
std::variant<std::vector<core::OwnProperty>, ArgumentError> targetProperties(
    const PropertyTarget& target) {
    std::vector<core::OwnProperty> properties =
        core::ownProperties(*target.document, *target.record);
    if (properties.empty() && !isTypeBefore08(target.record->entity)) {
        return ArgumentError{"entity type not implemented yet", true};
    }
    return properties;
}

/// The value of a property as the report writes it: angles in degrees.
std::string formatPropertyValue(const core::OwnProperty& property) {
    return std::visit(
        [&property](const auto& value) -> std::string {
            using Value = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Value, double>) {
                return formatNumber(property.kind == core::PropertyKind::Angle
                                        ? core::radiansToDegrees(value)
                                        : value);
            } else if constexpr (std::is_same_v<Value, core::Vec2>) {
                return formatPoint(value);
            } else if constexpr (std::is_same_v<Value, bool>) {
                return value ? "yes" : "no";
            } else if (property.kind == core::PropertyKind::Text) {
                return quoteText(value);
            } else {
                return value;
            }
        },
        property.value);
}

/// "name=value" separated by "; ", or "none".
std::string propertiesText(const std::vector<core::OwnProperty>& properties) {
    std::vector<std::string> items;
    items.reserve(properties.size());
    for (const core::OwnProperty& property : properties) {
        items.push_back(property.name + "=" + formatPropertyValue(property));
    }
    return joinItems(items);
}

/// The value written for a property of kind `kind`, std::nullopt if it is
/// not written as that kind asks. Angles in degrees pass through
/// reducedDegreesToRadians (docs/REGOLE-NUMERICHE.md, section 2, point 2).
std::optional<core::PropertyValue> parsePropertyValue(core::PropertyKind kind,
                                                      std::string_view text) {
    switch (kind) {
        case core::PropertyKind::Number:
        case core::PropertyKind::Distance:
            if (const auto number = parseNumber(text)) {
                return core::PropertyValue{*number};
            }
            return std::nullopt;
        case core::PropertyKind::Angle:
            if (const auto degrees = parseNumber(text)) {
                return core::PropertyValue{core::reducedDegreesToRadians(*degrees)};
            }
            return std::nullopt;
        case core::PropertyKind::Point:
            if (const auto point = parsePoint(text)) {
                return core::PropertyValue{*point};
            }
            return std::nullopt;
        case core::PropertyKind::Text:
            if (auto unquoted = unquoteText(text)) {
                return core::PropertyValue{std::move(*unquoted)};
            }
            return std::nullopt;
        case core::PropertyKind::Choice:
            return core::PropertyValue{std::string{text}};
        case core::PropertyKind::Flag:
            if (text == "yes" || text == "no") {
                return core::PropertyValue{text == "yes"};
            }
            return std::nullopt;
    }
    return std::nullopt;
}

}  // namespace

QuestionResult askProperties(const QuestionModel& model,
                             std::span<const std::string_view> arguments) {
    constexpr std::string_view kForm = "properties <E> or properties <drawing> <handle>";
    const auto target = propertyTarget(model, arguments, kForm);
    if (const auto* error = std::get_if<ArgumentError>(&target)) {
        return *error;
    }
    auto properties = targetProperties(std::get<PropertyTarget>(target));
    if (auto* error = std::get_if<ArgumentError>(&properties)) {
        return std::move(*error);
    }
    return propertiesText(std::get<std::vector<core::OwnProperty>>(properties));
}

QuestionResult askSetProperty(const QuestionModel& model,
                              std::span<const std::string_view> arguments) {
    constexpr std::string_view kForm =
        "set-property <E> <name>=<value> or set-property <drawing> <handle> <name>=<value>";
    if (arguments.size() < 2) {
        return ArgumentError{"wrong arguments, expected: " + std::string{kForm}};
    }
    const auto found = propertyTarget(model, arguments.first(arguments.size() - 1), kForm);
    if (const auto* error = std::get_if<ArgumentError>(&found)) {
        return *error;
    }
    const auto& target = std::get<PropertyTarget>(found);
    auto read = targetProperties(target);
    if (auto* error = std::get_if<ArgumentError>(&read)) {
        return std::move(*error);
    }
    const auto& properties = std::get<std::vector<core::OwnProperty>>(read);
    const std::string_view assignment = arguments.back();
    const auto equals = assignment.find('=');
    if (equals == std::string_view::npos || equals == 0) {
        return ArgumentError{"wrong arguments, expected: " + std::string{kForm}};
    }
    const std::string_view name = assignment.substr(0, equals);
    const auto property = std::ranges::find(properties, name, &core::OwnProperty::name);
    if (property == properties.end()) {
        // Refused as withOwnProperty() refuses it (core::kUnknownProperty),
        // also for the types of before the 0.8, which have none.
        return std::string{"invalid"};
    }
    const auto value = parsePropertyValue(property->kind, assignment.substr(equals + 1));
    if (!value) {
        return ArgumentError{"unexpected value of property '" + std::string{name} + "'"};
    }
    const core::PropertyChange change =
        core::withOwnProperty(*target.document, *target.record, name, *value);
    if (!change.entity) {
        return std::string{"invalid"};
    }
    // The document of the questions is read only: the change is answered,
    // not kept (docs/tappe/0.8.1/PT22-proprieta-core.md, point 6).
    core::EntityRecord changed = *target.record;
    changed.entity = *change.entity;
    if (change.extras) {
        changed.extras = *change.extras;
    }
    std::string answer;
    if (target.fromFile) {
        writeEntity(changed, *target.document, answer);
    } else {
        // With its extras: those of the change (the origin of a hatch).
        writeEntity(changed, answer);
    }
    answer += " | ";
    answer += propertiesText(core::ownProperties(*target.document, changed));
    return answer;
}

QuestionResult askLayers(const QuestionModel& model, std::span<const std::string_view> arguments) {
    // Until package 0.6-P10 io reads no table: a drawing has no layers yet,
    // and the answer is "none".
    return askAboutTable(model, arguments, "layers <drawing>", &layerLines);
}

QuestionResult askLinetypes(const QuestionModel& model,
                            std::span<const std::string_view> arguments) {
    return askAboutTable(model, arguments, "linetypes <drawing>", &linetypeLines);
}

QuestionResult askAci(const QuestionModel& /*model*/, std::span<const std::string_view> arguments) {
    if (arguments.size() != 1) {
        return ArgumentError{"wrong arguments, expected: aci <n>"};
    }
    int index = 0;
    const auto text = arguments.front();
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), index);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size() || index < 1 ||
        index > 255) {
        return ArgumentError{"expected an ACI index from 1 to 255"};
    }
    return core::formatColor(core::Color::fromRgb(core::aciToRgb(index)));
}

QuestionResult askEffective(const QuestionModel& model,
                            std::span<const std::string_view> arguments) {
    if (arguments.size() != 1) {
        return ArgumentError{"wrong arguments, expected: effective <E>"};
    }
    const auto id = model.find(arguments.front());
    if (!id) {
        return ArgumentError{"no entity named '" + std::string{arguments.front()} +
                             "' is defined above"};
    }
    const auto properties = core::effectiveProperties(model.document, *model.document.find(*id));
    return "color=" + core::formatColor(core::Color::fromRgb(properties.color)) + " linetype=\"" +
           properties.linetype->name + "\" ltscale=" + formatNumber(properties.linetypeScale) +
           " lineweight=" + core::formatLineweight(properties.lineweight) +
           " visible=" + (properties.visible ? "yes" : "no") +
           " locked=" + (properties.locked ? "yes" : "no");
}

QuestionResult askDashes(const QuestionModel& model, std::span<const std::string_view> arguments) {
    if (arguments.size() != 1) {
        return ArgumentError{"wrong arguments, expected: dashes <E>"};
    }
    const auto id = model.find(arguments.front());
    if (!id) {
        return ArgumentError{"no entity named '" + std::string{arguments.front()} +
                             "' is defined above"};
    }
    const auto& record = *model.document.find(*id);
    const auto properties = core::effectiveProperties(model.document, record);
    const auto pieces =
        core::dashes(record.entity, properties.linetype->pattern, properties.linetypeScale);
    if (!pieces) {
        return std::string{"continuous"};
    }
    std::vector<std::string> lines;
    lines.reserve(pieces->size());
    for (const auto& piece : *pieces) {
        lines.push_back(std::visit(
            [](const auto& value) -> std::string {
                using Piece = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Piece, core::DashLine>) {
                    return "LINE " + formatPoint(value.start) + " " + formatPoint(value.end);
                } else if constexpr (std::is_same_v<Piece, core::DashArc>) {
                    const double startDegrees = core::radiansToDegrees(value.startAngle);
                    std::string text = "ARC center=" + formatPoint(value.center);
                    text += " radius=" + formatNumber(value.radius);
                    text += " start-angle=" + formatNumber(startDegrees);
                    text += " end-angle=" + formatNumber(core::radiansToDegrees(value.endAngle));
                    return text;
                } else {
                    return "POINT " + formatPoint(value.position);
                }
            },
            piece));
    }
    return joinItems(lines);
}

QuestionResult askQuickSelect(const QuestionModel& model,
                              std::span<const std::string_view> arguments) {
    using Property = cmd::QuickSelectCriteria::Property;
    cmd::QuickSelectCriteria criteria;
    std::vector<core::EntityId> within;
    bool hasWithin = false;
    for (const auto argument : arguments) {
        if (argument == "exclude" && !criteria.exclude) {
            criteria.exclude = true;
            continue;
        }
        const auto equals = argument.find('=');
        if (equals == std::string_view::npos || equals == 0) {
            return ArgumentError{"unexpected quickselect argument"};
        }
        const bool notEqual = argument[equals - 1] == '!';
        const auto key = argument.substr(0, equals - (notEqual ? 1 : 0));
        const auto written = argument.substr(equals + 1);
        const auto value = !written.empty() && written.front() == '"'
                               ? unquoteText(written)
                               : std::optional<std::string>{std::string{written}};
        if (!value) {
            return ArgumentError{"invalid quoted quickselect value"};
        }
        if (key == "type" && !notEqual && !criteria.type) {
            criteria.type = *value;
        } else if (key == "within" && !notEqual && !hasWithin) {
            hasWithin = true;
            std::string_view rest{*value};
            while (!rest.empty()) {
                const auto comma = rest.find(',');
                const auto name = rest.substr(0, comma);
                const auto id = model.find(name);
                if (!id) {
                    return ArgumentError{"no entity named '" + std::string{name} +
                                         "' is defined above"};
                }
                within.push_back(*id);
                if (comma == std::string_view::npos) {
                    break;
                }
                rest.remove_prefix(comma + 1);
                if (rest.empty()) {
                    return ArgumentError{"expected an entity after ','"};
                }
            }
        } else {
            if (criteria.property != Property::None) {
                return ArgumentError{"expected one quickselect property condition"};
            }
            if (key == "layer") {
                criteria.property = Property::Layer;
            } else if (key == "color") {
                criteria.property = Property::Color;
            } else if (key == "linetype") {
                criteria.property = Property::Linetype;
            } else if (key == "lineweight") {
                criteria.property = Property::Lineweight;
            } else {
                return ArgumentError{"unexpected quickselect argument"};
            }
            criteria.op = notEqual ? cmd::QuickSelectCriteria::Operator::NotEqual
                                   : cmd::QuickSelectCriteria::Operator::Equal;
            criteria.value = *value;
        }
    }
    if (hasWithin && within.empty()) {
        return std::string{"none"};
    }
    std::string answer;
    for (const auto id : cmd::quickSelect(model.document, within, criteria)) {
        if (!answer.empty()) {
            answer += ' ';
        }
        answer += model.nameOf(id);
    }
    return answer.empty() ? std::string{"none"} : answer;
}

}  // namespace tg::script::detail
