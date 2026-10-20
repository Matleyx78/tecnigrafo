// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "question_table.hpp"
#include "tg/core/text_metrics.hpp"
#include "tg/script/number_format.hpp"
#include "tg/script/text_format.hpp"

namespace tg::script::detail {
QuestionResult askFontMetrics(const QuestionModel& /*model*/,
                              std::span<const std::string_view> arguments) {
    if (!arguments.empty()) {
        return ArgumentError{"wrong arguments, expected: font-metrics"};
    }
    const auto& metrics = core::fontMetrics();
    return "ascent=" + formatNumber(metrics.ascent) + " descent=" + formatNumber(metrics.descent);
}

QuestionResult askTextWidth(const QuestionModel& /*model*/,
                            std::span<const std::string_view> arguments) {
    if (arguments.size() != 1) {
        return ArgumentError{"wrong arguments, expected: text-width \"<text>\""};
    }
    const auto text = unquoteText(arguments[0]);
    if (!text) {
        return ArgumentError{"expected a quoted text"};
    }
    return formatNumber(core::textWidth(core::decodeUtf8(*text)));
}
}  // namespace tg::script::detail
