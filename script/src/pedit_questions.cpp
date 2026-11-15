// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The questions on polylines of stage 0.8 (docs/verifica/domande.md,
// "Polilinee (tappa 0.8)"): conversion, reversal and the join with a gap of
// PEDIT. They ask core (docs/tappe/0.8/PT32-unione-con-distanza.md, point 5)
// and add no logic of their own.

#include "question_table.hpp"
#include "tg/core/document.hpp"
#include "tg/core/pedit_join.hpp"
#include "tg/core/polyline.hpp"
#include "tg/core/tolerance.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/report.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace tg::script::detail {
namespace {

using Arguments = std::span<const std::string_view>;

template <typename T>
using Parsed = std::variant<T, ArgumentError>;

/// INSUNITS of a question without `units=`: millimetres
/// (docs/verifica/domande.md).
constexpr int kDefaultUnits = 4;

std::string quoted(std::string_view text) {
    return "'" + std::string{text} + "'";
}

ArgumentError usage(std::string_view form) {
    return ArgumentError{"wrong arguments, expected: " + std::string{form}};
}

/// The entity named `name`. An entity of a drawing read from a file
/// ("D:2F") is already defined under that name: questions.cpp copies it.
Parsed<const core::Entity*> entityNamed(const QuestionModel& model, std::string_view name) {
    const std::optional<core::EntityId> id = model.find(name);
    if (!id) {
        return ArgumentError{"no entity named " + quoted(name) + " is defined above"};
    }
    return &model.document.find(*id)->entity;
}

/// The polyline named `name`, for `question`.
Parsed<const core::Polyline*> polylineNamed(const QuestionModel& model, std::string_view name,
                                            std::string_view question) {
    const auto entity = entityNamed(model, name);
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    const auto* polyline = std::get_if<core::Polyline>(std::get<const core::Entity*>(entity));
    if (polyline == nullptr) {
        return ArgumentError{std::string{question} + " needs an LWPOLYLINE, " + quoted(name) +
                             " is not one"};
    }
    return polyline;
}

/// The value of "units=<n>", INSUNITS from 0 to 24.
Parsed<core::ToleranceRule> unitsArgument(std::string_view text) {
    const std::optional<double> value = parseNumber(text);
    if (!value || *value != std::floor(*value) || *value < 0 ||
        *value > core::Document::kMaxUnits) {
        return ArgumentError{"units needs a whole number from 0 to 24, found " + quoted(text)};
    }
    return core::ToleranceRule{static_cast<int>(*value)};
}

/// The optional arguments "key=value" after the positional ones: each key of
/// `allowed` at most once, in any order.
template <std::size_t Size>
Parsed<std::array<std::optional<std::string_view>, Size>> keyed(
    Arguments arguments, std::array<std::string_view, Size> allowed) {
    std::array<std::optional<std::string_view>, Size> values;
    for (const std::string_view argument : arguments) {
        const std::size_t equals = argument.find('=');
        bool known = false;
        for (std::size_t k = 0; k < allowed.size(); ++k) {
            if (equals != std::string_view::npos && argument.substr(0, equals) == allowed.at(k) &&
                !values.at(k)) {
                values.at(k) = argument.substr(equals + 1);
                known = true;
            }
        }
        if (!known) {
            return ArgumentError{"unexpected argument " + quoted(argument)};
        }
    }
    return values;
}

Parsed<core::JoinType> joinTypeArgument(const std::optional<std::string_view>& text) {
    if (!text || *text == "extend") {
        return core::JoinType::Extend;
    }
    if (*text == "add") {
        return core::JoinType::Add;
    }
    if (*text == "both") {
        return core::JoinType::Both;
    }
    return ArgumentError{"type needs extend, add or both, found " + quoted(*text)};
}

Parsed<double> fuzzArgument(const std::optional<std::string_view>& text) {
    if (!text) {
        return ArgumentError{"pedit-join needs fuzz=<d>"};
    }
    const std::optional<double> value = parseNumber(*text);
    if (!value || !std::isfinite(*value) || *value < 0.0) {
        return ArgumentError{"fuzz needs a number not below 0, found " + quoted(*text)};
    }
    return *value;
}

std::string polylineText(const core::Polyline& polyline) {
    std::string answer;
    writeEntity(core::Entity{polyline}, answer);
    return answer;
}

}  // namespace

QuestionResult askToPolyline(const QuestionModel& model, Arguments arguments) {
    if (arguments.size() != 1) {
        return usage("to-polyline <E>");
    }
    const auto entity = entityNamed(model, arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    const core::Entity& chosen = *std::get<const core::Entity*>(entity);
    if (const auto* line = std::get_if<core::Line>(&chosen)) {
        return polylineText(core::toPolyline(*line));
    }
    if (const auto* arc = std::get_if<core::Arc>(&chosen)) {
        return polylineText(core::toPolyline(*arc));
    }
    return ArgumentError{"to-polyline needs a LINE or an ARC, " + quoted(arguments[0]) +
                         " is neither"};
}

QuestionResult askReverse(const QuestionModel& model, Arguments arguments) {
    if (arguments.size() != 1) {
        return usage("reverse <P>");
    }
    const auto polyline = polylineNamed(model, arguments[0], "reverse");
    if (const auto* error = std::get_if<ArgumentError>(&polyline)) {
        return *error;
    }
    return polylineText(core::reversed(*std::get<const core::Polyline*>(polyline)));
}

QuestionResult askPeditJoin(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm =
        "pedit-join <P> <B> ... fuzz=<d> [type=extend|add|both] [units=<n>]";
    std::size_t count = 0;
    while (count < arguments.size() && arguments[count].find('=') == std::string_view::npos) {
        ++count;
    }
    if (count < 2) {
        return usage(kForm);
    }
    const auto options =
        keyed(arguments.subspan(count), std::array<std::string_view, 3>{"fuzz", "type", "units"});
    if (const auto* error = std::get_if<ArgumentError>(&options)) {
        return *error;
    }
    const auto& [fuzzText, typeText, unitsText] = std::get<0>(options);
    const auto fuzz = fuzzArgument(fuzzText);
    if (const auto* error = std::get_if<ArgumentError>(&fuzz)) {
        return *error;
    }
    const auto type = joinTypeArgument(typeText);
    if (const auto* error = std::get_if<ArgumentError>(&type)) {
        return *error;
    }
    const auto rule = unitsText ? unitsArgument(*unitsText)
                                : Parsed<core::ToleranceRule>{core::ToleranceRule{kDefaultUnits}};
    if (const auto* error = std::get_if<ArgumentError>(&rule)) {
        return *error;
    }
    const auto source = polylineNamed(model, arguments[0], "pedit-join");
    if (const auto* error = std::get_if<ArgumentError>(&source)) {
        return *error;
    }
    const auto names = arguments.subspan(1, count - 1);
    std::vector<core::Entity> others;
    others.reserve(names.size());
    for (const auto name : names) {
        const auto entity = entityNamed(model, name);
        if (const auto* error = std::get_if<ArgumentError>(&entity)) {
            return *error;
        }
        others.push_back(*std::get<const core::Entity*>(entity));
    }
    const core::GapJoinResult result =
        core::joinWithGaps(*std::get<const core::Polyline*>(source), others, std::get<double>(fuzz),
                           std::get<core::JoinType>(type), std::get<core::ToleranceRule>(rule));
    if (result.consumed.empty()) {
        return std::string{"none"};
    }
    std::string answer = std::string{arguments[0]} + " " + polylineText(result.polyline);
    answer += " added=" + std::to_string(result.added) + " consumed=";
    for (std::size_t i = 0; i < result.consumed.size(); ++i) {
        if (i > 0) {
            answer += ',';
        }
        answer += names[result.consumed[i]];
    }
    return answer;
}

}  // namespace tg::script::detail
