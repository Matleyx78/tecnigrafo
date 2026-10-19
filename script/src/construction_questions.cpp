// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The constructions of the catalogue of questions (docs/verifica/domande.md):
// circle and arc through three points (docs/adr/0012, point 4).

#include "question_table.hpp"
#include "tg/core/arc.hpp"
#include "tg/core/circle.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/report.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace tg::script::detail {
namespace {

/// The three points of "<question> x1,y1 x2,y2 x3,y3".
std::variant<std::array<core::Vec2, 3>, ArgumentError> threePoints(
    std::span<const std::string_view> arguments, std::string_view form) {
    if (arguments.size() != 3) {
        return ArgumentError{"wrong arguments, expected: " + std::string{form}};
    }
    std::array<core::Vec2, 3> points;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const std::optional<core::Vec2> point = parsePoint(arguments[i]);
        if (!point) {
            return ArgumentError{"expected a point x,y, found '" + std::string{arguments[i]} + "'"};
        }
        points.at(i) = *point;
    }
    return points;
}

/// The entity in the format of the report, or "collinear".
template <typename Shape>
std::string constructionText(const std::optional<Shape>& shape) {
    if (!shape) {
        return "collinear";
    }
    std::string text;
    writeEntity(core::Entity{*shape}, text);
    return text;
}

}  // namespace

QuestionResult askCircleThrough(const QuestionModel& /*model*/,
                                std::span<const std::string_view> arguments) {
    const auto points = threePoints(arguments, "circle-through x1,y1 x2,y2 x3,y3");
    if (const auto* error = std::get_if<ArgumentError>(&points)) {
        return *error;
    }
    const auto& [a, b, c] = std::get<0>(points);
    return constructionText(core::circleThroughPoints(a, b, c));
}

QuestionResult askArcThrough(const QuestionModel& /*model*/,
                             std::span<const std::string_view> arguments) {
    const auto points = threePoints(arguments, "arc-through x1,y1 x2,y2 x3,y3");
    if (const auto* error = std::get_if<ArgumentError>(&points)) {
        return *error;
    }
    const auto& [start, middle, end] = std::get<0>(points);
    return constructionText(core::arcThroughPoints(start, middle, end));
}

}  // namespace tg::script::detail
