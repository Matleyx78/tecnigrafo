// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "question_table.hpp"
#include "tg/core/effective_properties.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/report.hpp"

#include <charconv>
#include <span>
#include <string>
#include <string_view>
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

}  // namespace

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

}  // namespace tg::script::detail
