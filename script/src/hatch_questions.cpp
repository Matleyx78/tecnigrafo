// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The questions on the boundaries of hatches of stage 0.8
// (docs/verifica/domande.md, "Retini (tappa 0.8)"): the contour detected from
// a point and the contours chosen as objects. They ask core
// (docs/tappe/0.8/PT43-contorni-dei-retini.md, point 5) and add no logic of
// their own. Stage 0.8.1 adds the families of the pattern of a hatch
// (pattern-lines, docs/tappe/0.8.1/PT23-proprieta-retini-quote.md, point 4).

#include "new_entities_format.hpp"
#include "question_table.hpp"
#include "tg/core/document.hpp"
#include "tg/core/entity_id.hpp"
#include "tg/core/hatch.hpp"
#include "tg/core/hatch_boundary.hpp"
#include "tg/core/math.hpp"
#include "tg/core/vec2.hpp"
#include "tg/script/number_format.hpp"

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

/// INSUNITS of a question without `units=`: millimetres
/// (docs/verifica/domande.md).
constexpr int kDefaultUnits = 4;

std::string quoted(std::string_view text) {
    return "'" + std::string{text} + "'";
}

ArgumentError usage(std::string_view form) {
    return ArgumentError{"wrong arguments, expected: " + std::string{form}};
}

/// The value of "units=<n>", INSUNITS from 0 to 24, or std::nullopt.
std::optional<int> unitsValue(std::string_view text) {
    const std::optional<double> value = parseNumber(text);
    if (!value || *value != std::floor(*value) || *value < 0 ||
        *value > core::Document::kMaxUnits) {
        return std::nullopt;
    }
    return static_cast<int>(*value);
}

/// The loops as the boundary paths of the HATCH line of
/// docs/verifica/rapporto.md, each with a leading blank
/// (" loop=external+polyline vertex=0,0 …"): the line of a hatch with only
/// these loops, from its first loop on, so that both are written alike.
std::string loopsText(const std::vector<core::HatchLoop>& loops) {
    core::Hatch hatch;
    hatch.loops = loops;
    std::string line;
    writeNewEntity(core::Entity{hatch}, line);
    // The pattern name is empty: the first " loop=" starts the first loop.
    const std::size_t first = line.find(" loop=");
    return first == std::string::npos ? std::string{} : line.substr(first);
}

/// A family of a pattern: "angle=<degrees> origin=x,y offset=x,y dashes=a,b,…".
std::string familyText(const core::PatternLine& line) {
    std::string text = "angle=" + formatNumber(core::radiansToDegrees(line.angle));
    text += " origin=" + formatPoint(line.origin);
    text += " offset=" + formatPoint(line.offset);
    text += " dashes=";
    for (std::size_t i = 0; i < line.dashes.size(); ++i) {
        if (i > 0) {
            text += ',';
        }
        text += formatNumber(line.dashes[i]);
    }
    return text;
}

}  // namespace

QuestionResult askPatternLines(const QuestionModel& model, Arguments arguments) {
    if (arguments.size() != 1) {
        return usage("pattern-lines <H>");
    }
    const std::optional<core::EntityId> id = model.find(arguments[0]);
    if (!id) {
        return ArgumentError{"no entity named " + quoted(arguments[0]) + " is defined above"};
    }
    const auto* hatch = std::get_if<core::Hatch>(&model.document.find(*id)->entity);
    if (hatch == nullptr) {
        return ArgumentError{quoted(arguments[0]) + " is not a HATCH"};
    }
    // In the order of the pattern; none for a solid fill.
    std::string answer;
    if (!hatch->solid) {
        for (const core::PatternLine& line : hatch->patternLines) {
            if (!answer.empty()) {
                answer += "; ";
            }
            answer += familyText(line);
        }
    }
    return answer.empty() ? std::string{"none"} : answer;
}

QuestionResult askHatchBoundary(const QuestionModel& model, Arguments arguments) {
    constexpr std::string_view kForm = "hatch-boundary x,y [units=<n>]";
    if (arguments.empty() || arguments.size() > 2) {
        return usage(kForm);
    }
    const std::optional<core::Vec2> picked = parsePoint(arguments[0]);
    if (!picked || !std::isfinite(picked->x) || !std::isfinite(picked->y)) {
        return ArgumentError{"expected a point x,y, found " + quoted(arguments[0])};
    }
    int units = kDefaultUnits;
    if (arguments.size() == 2) {
        constexpr std::string_view kUnits = "units=";
        const std::optional<int> value = arguments[1].starts_with(kUnits)
                                             ? unitsValue(arguments[1].substr(kUnits.size()))
                                             : std::nullopt;
        if (!value) {
            return usage(kForm);
        }
        units = *value;
    }
    // units= is INSUNITS of the drawing: a copy with those units when the
    // drawing of the file has others.
    std::optional<core::Document> withUnits;
    if (model.document.units() != units) {
        withUnits.emplace(model.document);
        withUnits->setUnits(units);
    }
    const core::DetectedBoundary boundary =
        core::detectHatchBoundary(withUnits ? *withUnits : model.document, *picked);
    std::string answer;
    switch (boundary.outcome) {
        case core::BoundaryOutcome::Enclosed:
            answer = "enclosed" + loopsText(boundary.loops);
            break;
        case core::BoundaryOutcome::OnCurve:
            answer = "on-curve";
            // detectHatchBoundary always names the entity with OnCurve.
            if (boundary.nearEntity) {
                answer += ' ';
                answer += model.nameOf(*boundary.nearEntity);
            }
            break;
        case core::BoundaryOutcome::NotEnclosed:
            answer = "not-enclosed";
            for (const core::Vec2& end : boundary.openEnds) {
                answer += " open=" + formatPoint(end);
            }
            break;
    }
    if (!boundary.complete) {
        answer += " incomplete";
    }
    return answer;
}

QuestionResult askHatchContours(const QuestionModel& model, Arguments arguments) {
    if (arguments.empty()) {
        return usage("hatch-contours <E1> <E2> ...");
    }
    std::vector<core::EntityId> chosen;
    chosen.reserve(arguments.size());
    for (const std::string_view name : arguments) {
        const std::optional<core::EntityId> id = model.find(name);
        if (!id) {
            return ArgumentError{"no entity named " + quoted(name) + " is defined above"};
        }
        chosen.push_back(*id);
    }
    const core::ChosenContours contours = core::chosenHatchContours(model.document, chosen);
    std::string answer = loopsText(contours.loops);
    for (const core::EntityId id : contours.leftOut) {
        answer += " left-out=";
        answer += model.nameOf(id);
    }
    if (answer.empty()) {
        return std::string{"none"};
    }
    // Every item was written with a leading blank.
    return answer.substr(1);
}

}  // namespace tg::script::detail
