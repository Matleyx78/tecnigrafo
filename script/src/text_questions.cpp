// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "question_table.hpp"
#include "tg/core/text.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/text_format.hpp"

#include <optional>
#include <string>
#include <variant>

namespace tg::script::detail {
namespace {

// PT6 will select the metrics of the requested face.
bool isAnnouncedFace(std::string_view argument) {
    return argument == "face=regular" || argument == "face=bold" || argument == "face=italic" ||
           argument == "face=bold-italic";
}

}  // namespace

QuestionResult askFontMetrics(const QuestionModel& /*model*/,
                              std::span<const std::string_view> arguments) {
    if (arguments.size() == 1 && isAnnouncedFace(arguments[0])) {
        return ArgumentError{"the argument 'face' is not implemented yet", true};
    }
    if (!arguments.empty()) {
        return ArgumentError{"wrong arguments, expected: font-metrics"};
    }
    const auto& metrics = core::fontMetrics();
    return "ascent=" + formatNumber(metrics.ascent) + " descent=" + formatNumber(metrics.descent);
}

QuestionResult askTextWidth(const QuestionModel& /*model*/,
                            std::span<const std::string_view> arguments) {
    const bool hasAnnouncedFace = arguments.size() == 2 && isAnnouncedFace(arguments[1]);
    if (arguments.size() != 1 && !hasAnnouncedFace) {
        return ArgumentError{"wrong arguments, expected: text-width \"<text>\""};
    }
    const auto text = unquoteText(arguments[0]);
    if (!text) {
        return ArgumentError{"expected a quoted text"};
    }
    if (hasAnnouncedFace) {
        return ArgumentError{"the argument 'face' is not implemented yet", true};
    }
    return formatNumber(core::textWidth(core::decodeUtf8(*text)));
}

QuestionResult askBox(const QuestionModel& model, std::span<const std::string_view> arguments) {
    if (arguments.size() != 1) {
        return ArgumentError{"wrong arguments, expected: box <entity>"};
    }
    const std::optional<core::EntityId> id = model.find(arguments[0]);
    if (!id) {
        return ArgumentError{"no entity named '" + std::string{arguments[0]} +
                             "' is defined above"};
    }
    const core::EntityRecord* record = model.document.find(*id);
    const auto* text = record != nullptr ? std::get_if<core::Text>(&record->entity) : nullptr;
    if (text == nullptr) {
        return ArgumentError{"box needs a TEXT, and '" + std::string{arguments[0]} +
                             "' is not one"};
    }
    std::string answer;
    for (const core::Vec2& corner : core::textBox(*text)) {
        if (!answer.empty()) {
            answer += ' ';
        }
        answer += formatPoint(corner);
    }
    return answer;
}
}  // namespace tg::script::detail
