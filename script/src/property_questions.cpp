// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

// The questions on layers and properties of stage 0.6
// (docs/verifica/domande.md, "Layer e proprietà"). This package answers
// `layers` and `linetypes`; `aci`, `effective`, `dashes` and `quickselect`
// are in the catalogue with no function until their packages (0.6-P9,
// P11, P23) add them here.

#include "question_table.hpp"
#include "tg/script/report.hpp"

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

}  // namespace tg::script::detail
