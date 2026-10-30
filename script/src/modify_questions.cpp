// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "question_table.hpp"

#include "tg/core/math.hpp"
#include "tg/core/similarity.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/report.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace tg::script::detail {
namespace {

using Arguments = std::span<const std::string_view>;

ArgumentError transformUsage() {
    return ArgumentError{"expected transform <entity> move dx,dy | rotate <degrees> x,y | "
                         "scale <factor> x,y | mirror x1,y1 x2,y2 [mirrtext=0|1]"};
}

std::variant<std::optional<core::Similarity>, ArgumentError> readSimilarity(Arguments arguments) {
    const std::string_view operation = arguments[0];
    if (operation == "move" && arguments.size() == 2) {
        if (const auto point = parsePoint(arguments[1])) {
            return core::Similarity::translation(*point);
        }
    } else if ((operation == "rotate" || operation == "scale") && arguments.size() == 3) {
        const auto number = parseNumber(arguments[1]);
        const auto center = parsePoint(arguments[2]);
        if (number && center) {
            if (operation == "scale") {
                return core::Similarity::scaling(*number, *center);
            }
            return core::Similarity::rotation(core::degreesToRadians(*number), *center);
        }
    } else if (operation == "mirror" && (arguments.size() == 3 || arguments.size() == 4)) {
        if (arguments.size() == 4 && arguments[3] != "mirrtext=0" && arguments[3] != "mirrtext=1") {
            return transformUsage();
        }
        const auto a = parsePoint(arguments[1]);
        const auto b = parsePoint(arguments[2]);
        if (a && b) {
            return core::Similarity::reflection(*a, *b);
        }
    }
    return transformUsage();
}

}  // namespace

QuestionResult askTransform(const QuestionModel& model, Arguments arguments) {
    if (arguments.size() < 3) {
        return transformUsage();
    }
    const core::Entity* entity = nullptr;
    if (const auto fromFile = fileEntity(model.files, arguments[0])) {
        if (const auto* handled = std::get_if<const core::Entity*>(&*fromFile)) {
            entity = *handled;
        } else if (std::holds_alternative<UnhandledFileEntity>(*fromFile)) {
            return ArgumentError{"entity type not implemented yet", true};
        } else {
            return ArgumentError{std::get<std::string>(*fromFile)};
        }
    } else if (const auto id = model.find(arguments[0])) {
        entity = &model.document.find(*id)->entity;
    } else {
        return ArgumentError{"no entity named '" + std::string{arguments[0]} + "' is defined above"};
    }
    const auto parsed = readSimilarity(arguments.subspan(1));
    if (const auto* error = std::get_if<ArgumentError>(&parsed)) {
        return *error;
    }
    const auto& similarity = std::get<std::optional<core::Similarity>>(parsed);
    if (!similarity) {
        return std::string{"invalid"};
    }
    const core::MirrorText mirrorText = arguments.back() == "mirrtext=1"
                                          ? core::MirrorText::Mirrored
                                          : core::MirrorText::Readable;
    std::string answer;
    writeEntity(core::transformed(*entity, *similarity, mirrorText), answer);
    return answer;
}

}  // namespace tg::script::detail
