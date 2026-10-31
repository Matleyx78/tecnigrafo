// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "question_table.hpp"
#include "tg/core/grips.hpp"
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
    std::string message = "expected transform <entity> move dx,dy | rotate <degrees> x,y | ";
    message += "scale <factor> x,y | mirror x1,y1 x2,y2 [mirrtext=0|1]";
    return ArgumentError{message};
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
        const std::string name{arguments[0]};
        return ArgumentError{"no entity named '" + name + "' is defined above"};
    }
    const auto parsed = readSimilarity(arguments.subspan(1));
    if (const auto* error = std::get_if<ArgumentError>(&parsed)) {
        return *error;
    }
    const auto& similarity = std::get<std::optional<core::Similarity>>(parsed);
    if (!similarity) {
        return std::string{"invalid"};
    }
    const bool mirrored = arguments.back() == "mirrtext=1";
    const auto mirrorText = mirrored ? core::MirrorText::Mirrored : core::MirrorText::Readable;
    std::string answer;
    writeEntity(core::transformed(*entity, *similarity, mirrorText), answer);
    return answer;
}

namespace {

using GripEntity = std::variant<const core::Entity*, ArgumentError>;

GripEntity gripEntity(const QuestionModel& model, std::string_view name) {
    if (const auto fromFile = fileEntity(model.files, name)) {
        if (const auto* handled = std::get_if<const core::Entity*>(&*fromFile)) {
            return *handled;
        }
        if (std::holds_alternative<UnhandledFileEntity>(*fromFile)) {
            return ArgumentError{"entity type not implemented yet", true};
        }
        return ArgumentError{std::get<std::string>(*fromFile)};
    }
    if (const auto id = model.find(name)) {
        return &model.document.find(*id)->entity;
    }
    return ArgumentError{"no entity named '" + std::string{name} + "' is defined above"};
}

}  // namespace

QuestionResult askGrips(const QuestionModel& model, Arguments arguments) {
    if (arguments.size() != 1) {
        return ArgumentError{"expected grips <entity>"};
    }
    const auto found = gripEntity(model, arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&found)) {
        return *error;
    }
    std::string answer;
    for (const auto& point : core::gripPoints(*std::get<const core::Entity*>(found))) {
        if (!answer.empty()) {
            answer += "; ";
        }
        answer += formatPoint(point);
    }
    return answer.empty() ? std::string{"none"} : answer;
}

QuestionResult askGrip(const QuestionModel& model, Arguments arguments) {
    if (arguments.size() != 3) {
        return ArgumentError{"expected grip <entity> x1,y1 x2,y2"};
    }
    const auto from = parsePoint(arguments[1]);
    const auto target = parsePoint(arguments[2]);
    if (!from || !target) {
        return ArgumentError{"expected grip <entity> x1,y1 x2,y2"};
    }
    const auto found = gripEntity(model, arguments[0]);
    if (const auto* error = std::get_if<ArgumentError>(&found)) {
        return *error;
    }
    const auto& entity = *std::get<const core::Entity*>(found);
    const auto points = core::gripPoints(entity);
    // The catalogue fixes this lookup tolerance in drawing units (P9).
    constexpr double kGripLookupTolerance = 1e-9;
    for (std::size_t i = 0; i < points.size(); ++i) {
        if (core::nearlyEqual(points[i], *from, kGripLookupTolerance)) {
            const auto result = core::moveGrip(entity, i, *target);
            if (result.outcome == core::GripOutcome::Invalid) {
                return std::string{"invalid"};
            }
            std::string answer;
            writeEntity(result.entity, answer);
            return answer;
        }
    }
    return std::string{"none"};
}

}  // namespace tg::script::detail
