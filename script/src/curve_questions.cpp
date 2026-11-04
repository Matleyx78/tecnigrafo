// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The questions on curves answered by the facade of core over solid-space
// (docs/verifica/domande.md, "Intersezioni, lato e tolleranze" and "Curva
// parallela"): where curves meet, on which side a point lies, the tolerance
// rule of a drawing and the parallel curve.

#include "question_table.hpp"
#include "tg/core/document.hpp"
#include "tg/core/intersections.hpp"
#include "tg/core/join.hpp"
#include "tg/core/offset.hpp"
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

/// Largest coordinate solid-space accepts, 2^50 (solid_space/robust/point.hpp,
/// kMaxCoordinate): the tolerance rule has it as a precondition.
constexpr double kMaxCoordinate = 0x1p50;

std::string quoted(std::string_view text) {
    return "'" + std::string{text} + "'";
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

Parsed<core::Vec2> pointNamed(std::string_view text) {
    const std::optional<core::Vec2> point = parsePoint(text);
    if (!point) {
        return ArgumentError{"expected a point x,y, found " + quoted(text)};
    }
    return *point;
}

/// A whole number in [low, high], as "units=" and "segment=" take it.
std::optional<int> wholeNumber(std::string_view text, int low, int high) {
    const std::optional<double> value = parseNumber(text);
    if (!value || *value != std::floor(*value) || *value < low || *value > high) {
        return std::nullopt;
    }
    return static_cast<int>(*value);
}

/// The value of "units=<n>", INSUNITS from 0 to 24.
Parsed<core::ToleranceRule> unitsArgument(std::string_view text) {
    const std::optional<int> units = wholeNumber(text, 0, core::Document::kMaxUnits);
    if (!units) {
        return ArgumentError{"units needs a whole number from 0 to 24, found " + quoted(text)};
    }
    return core::ToleranceRule{*units};
}

/// The optional arguments "key=value" after the positional ones: each key of
/// `allowed` at most once, in any order.
Parsed<std::array<std::optional<std::string_view>, 2>> keyed(
    Arguments arguments, std::array<std::string_view, 2> allowed) {
    std::array<std::optional<std::string_view>, 2> values;
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

bool withinDomain(const core::Vec2& point) {
    return std::abs(point.x) <= kMaxCoordinate && std::abs(point.y) <= kMaxCoordinate;
}

// ---------------------------------------------------------------------------
// Answers

std::string_view whereName(core::Location location) {
    switch (location) {
        case core::Location::BeforeStart:
            return "before";
        case core::Location::AtStart:
            return "start";
        case core::Location::Inside:
            return "inside";
        case core::Location::AtEnd:
            return "end";
        case core::Location::AfterEnd:
            return "after";
    }
    return "inside";
}

std::string_view contactName(core::Contact contact) {
    switch (contact) {
        case core::Contact::Crossing:
            return "crossing";
        case core::Contact::Touching:
            return "touching";
        case core::Contact::SameCurve:
            return "same-curve";
    }
    return "crossing";
}

std::string positionText(const core::CurvePosition& position) {
    return std::to_string(position.segment) + ":" + std::string{whereName(position.location)};
}

/// "x,y <contact> <on A> <on B>" and "overlap x1,y1 x2,y2", separated by
/// "; ", or "none".
std::string intersectionsText(const core::Intersections& found) {
    std::string text;
    const auto add = [&text](const std::string& item) {
        if (!text.empty()) {
            text += "; ";
        }
        text += item;
    };
    for (const core::Crossing& crossing : found.crossings) {
        add(formatPoint(crossing.point) + " " + std::string{contactName(crossing.contact)} + " " +
            positionText(crossing.onA) + " " + positionText(crossing.onB));
    }
    for (const core::SharedStretch& stretch : found.shared) {
        add("overlap " + formatPoint(stretch.from) + " " + formatPoint(stretch.to));
    }
    return text.empty() ? std::string{"none"} : text;
}

std::string yesNo(bool value) {
    return value ? "yes" : "no";
}

}  // namespace

QuestionResult askIntersections(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "intersections <A> <B> [extend=a|b|both]";
    if (arguments.size() != 2 && arguments.size() != 3) {
        return ArgumentError{"wrong arguments, expected: " + std::string{kForm}};
    }
    core::Extension extendA = core::Extension::None;
    core::Extension extendB = core::Extension::None;
    if (arguments.size() == 3) {
        const std::string_view extend = arguments[2];
        if (extend == "extend=a" || extend == "extend=both") {
            extendA = core::Extension::Extended;
        }
        if (extend == "extend=b" || extend == "extend=both") {
            extendB = core::Extension::Extended;
        }
        if (extendA == core::Extension::None && extendB == core::Extension::None) {
            return ArgumentError{"wrong arguments, expected: " + std::string{kForm}};
        }
    }
    const auto a = entityNamed(model, arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&a)) {
        return *error;
    }
    const auto b = entityNamed(model, arguments[1]);
    if (const auto* error = std::get_if<ArgumentError>(&b)) {
        return *error;
    }
    return intersectionsText(core::intersect(*std::get<const core::Entity*>(a), extendA,
                                             *std::get<const core::Entity*>(b), extendB));
}

QuestionResult askSelfIntersections(const QuestionModel& model, Arguments arguments) {
    if (arguments.size() != 1) {
        return ArgumentError{"wrong arguments, expected: self-intersections <P>"};
    }
    const auto entity = entityNamed(model, arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    const auto* polyline = std::get_if<core::Polyline>(std::get<const core::Entity*>(entity));
    if (polyline == nullptr) {
        return ArgumentError{"self-intersections needs a LWPOLYLINE, " + quoted(arguments[0]) +
                             " is not one"};
    }
    return intersectionsText(core::selfIntersections(*polyline));
}

QuestionResult askSide(const QuestionModel& model, Arguments arguments) {
    if (arguments.size() != 2) {
        return ArgumentError{"wrong arguments, expected: side <E> x,y"};
    }
    const auto entity = entityNamed(model, arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    const auto point = pointNamed(arguments[1]);
    if (const auto* error = std::get_if<ArgumentError>(&point)) {
        return *error;
    }
    const std::optional<core::Side> side =
        core::sideOf(*std::get<const core::Entity*>(entity), std::get<core::Vec2>(point));
    if (!side) {
        return std::string{"none"};
    }
    switch (*side) {
        case core::Side::Left:
            return std::string{"left"};
        case core::Side::On:
            return std::string{"on"};
        case core::Side::Right:
            return std::string{"right"};
    }
    return std::string{"none"};
}

QuestionResult askCoincident(const QuestionModel& /*model*/, Arguments arguments) {
    if (arguments.size() != 2 && arguments.size() != 3) {
        return ArgumentError{"wrong arguments, expected: coincident x1,y1 x2,y2 [units=<n>]"};
    }
    const auto first = pointNamed(arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&first)) {
        return *error;
    }
    const auto second = pointNamed(arguments[1]);
    if (const auto* error = std::get_if<ArgumentError>(&second)) {
        return *error;
    }
    const auto options = keyed(arguments.subspan(2), {"units", ""});
    if (const auto* error = std::get_if<ArgumentError>(&options)) {
        return *error;
    }
    const auto& units = std::get<0>(options)[0];
    const auto rule = units ? unitsArgument(*units)
                            : Parsed<core::ToleranceRule>{core::ToleranceRule{kDefaultUnits}};
    if (const auto* error = std::get_if<ArgumentError>(&rule)) {
        return *error;
    }
    const core::Vec2 a = std::get<core::Vec2>(first);
    const core::Vec2 b = std::get<core::Vec2>(second);
    if (!withinDomain(a) || !withinDomain(b)) {
        return ArgumentError{"coincident needs coordinates of magnitude at most 2^50"};
    }
    return yesNo(std::get<core::ToleranceRule>(rule).coincident(a, b));
}

QuestionResult askDegenerate(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "degenerate <E> [segment=<i>] [units=<n>]";
    if (arguments.empty() || arguments.size() > 3) {
        return ArgumentError{"wrong arguments, expected: " + std::string{kForm}};
    }
    const auto entity = entityNamed(model, arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    const auto options = keyed(arguments.subspan(1), {"segment", "units"});
    if (const auto* error = std::get_if<ArgumentError>(&options)) {
        return *error;
    }
    const auto& [segment, units] = std::get<0>(options);
    const auto rule = units ? unitsArgument(*units)
                            : Parsed<core::ToleranceRule>{core::ToleranceRule{kDefaultUnits}};
    if (const auto* error = std::get_if<ArgumentError>(&rule)) {
        return *error;
    }
    const auto& tolerance = std::get<core::ToleranceRule>(rule);
    const auto& chosen = *std::get<const core::Entity*>(entity);
    if (!segment) {
        return yesNo(core::isDegenerate(chosen, tolerance));
    }
    const auto* polyline = std::get_if<core::Polyline>(&chosen);
    if (polyline == nullptr) {
        return ArgumentError{"segment= needs a LWPOLYLINE, " + quoted(arguments[0]) +
                             " is not one"};
    }
    const int count = static_cast<int>(polyline->segmentCount());
    const std::optional<int> index = wholeNumber(*segment, 0, count - 1);
    if (!index) {
        return ArgumentError{"segment needs a segment index of " + quoted(arguments[0]) +
                             ", found " + quoted(*segment)};
    }
    return yesNo(
        core::isDegenerate(polyline->segment(static_cast<std::size_t>(*index)), tolerance));
}

QuestionResult askJoin(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "join <A> <B> … [circle=no] [units=<n>]";
    std::size_t count = 0;
    while (count < arguments.size() && arguments[count].find('=') == std::string_view::npos) {
        ++count;
    }
    if (count < 2) {
        return ArgumentError{"wrong arguments, expected: " + std::string{kForm}};
    }
    const auto options = keyed(arguments.subspan(count), {"circle", "units"});
    if (const auto* error = std::get_if<ArgumentError>(&options)) {
        return *error;
    }
    const auto& [circle, units] = std::get<0>(options);
    if (circle && *circle != "no") {
        return ArgumentError{"circle only accepts no"};
    }
    const auto rule = units ? unitsArgument(*units)
                            : Parsed<core::ToleranceRule>{core::ToleranceRule{kDefaultUnits}};
    if (const auto* error = std::get_if<ArgumentError>(&rule)) {
        return *error;
    }
    std::vector<core::Entity> entities;
    entities.reserve(count);
    for (const auto name : arguments.first(count)) {
        const auto entity = entityNamed(model, name);
        if (const auto* error = std::get_if<ArgumentError>(&entity)) {
            return *error;
        }
        entities.push_back(*std::get<const core::Entity*>(entity));
    }
    const auto result = core::join(entities, std::get<core::ToleranceRule>(rule));
    if (!result || (result->formsCircle && circle)) {
        return std::string{"none"};
    }
    if (result->source) {
        std::string answer = std::string{arguments.front()} + " ";
        writeEntity(*result->source, answer);
        return answer;
    }
    if (result->created) {
        std::string answer = "new ";
        writeEntity(*result->created, answer);
        return answer;
    }
    return std::string{"none"};
}

QuestionResult askOffset(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "offset <E> <d> x,y [units=<n>]";
    if (arguments.size() != 3 && arguments.size() != 4) {
        return ArgumentError{"wrong arguments, expected: " + std::string{kForm}};
    }
    const auto entity = entityNamed(model, arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&entity)) {
        return *error;
    }
    const core::Entity& chosen = *std::get<const core::Entity*>(entity);
    if (std::holds_alternative<core::Text>(chosen)) {
        return ArgumentError{"offset needs a curve, " + quoted(arguments[0]) + " is a TEXT"};
    }
    const std::optional<double> distance = parseNumber(arguments[1]);
    if (!distance || !std::isfinite(*distance) || !(*distance > 0.0)) {
        return ArgumentError{"offset needs a distance greater than 0, found " +
                             quoted(arguments[1])};
    }
    const auto point = pointNamed(arguments[2]);
    if (const auto* error = std::get_if<ArgumentError>(&point)) {
        return *error;
    }
    const auto options = keyed(arguments.subspan(3), {"units", ""});
    if (const auto* error = std::get_if<ArgumentError>(&options)) {
        return *error;
    }
    const auto& units = std::get<0>(options)[0];
    const auto rule = units ? unitsArgument(*units)
                            : Parsed<core::ToleranceRule>{core::ToleranceRule{kDefaultUnits}};
    if (const auto* error = std::get_if<ArgumentError>(&rule)) {
        return *error;
    }
    const core::OffsetResult result = core::offsetEntity(
        chosen, *distance, std::get<core::Vec2>(point), std::get<core::ToleranceRule>(rule));
    switch (result.outcome) {
        case core::OffsetOutcome::Done:
            // Done always carries the copy (tg/core/offset.hpp).
            if (result.entity) {
                std::string answer;
                writeEntity(*result.entity, answer);
                return answer;
            }
            break;
        case core::OffsetOutcome::Vanished:
            return std::string{"vanished"};
        case core::OffsetOutcome::Failed:
            return std::string{"failed"};
        case core::OffsetOutcome::NoSide:
            return std::string{"no-side"};
    }
    return std::string{"failed"};
}

}  // namespace tg::script::detail
