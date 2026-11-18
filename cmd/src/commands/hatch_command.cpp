// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/hatch_command.hpp"

#include "../text.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/hatch_boundary.hpp"
#include "tg/core/hatch_patterns.hpp"
#include "tg/core/math.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace tg::cmd {
namespace {

constexpr std::uint32_t kExternalLoop = 1;

bool isPattern(std::string_view text, std::string_view pattern) {
    return detail::equalsIgnoringCase(text, pattern);
}

int islandNumber(core::IslandStyle style) {
    switch (style) {
        case core::IslandStyle::Normal:
            return 0;
        case core::IslandStyle::Outer:
            return 1;
        case core::IslandStyle::Ignore:
            return 2;
    }
    return 1;
}

int measurementOf(const core::Document& document) {
    for (const core::RawSection& section : document.raw().sections) {
        if (section.name != "HEADER") {
            continue;
        }
        for (std::size_t i = 0; i + 1 < section.groups.size(); ++i) {
            if (section.groups[i].code != 9 || section.groups[i].value != "$MEASUREMENT") {
                continue;
            }
            int value = core::kMetricMeasurement;
            const std::string& text = section.groups[i + 1].value;
            const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
            if (result.ec == std::errc{} && result.ptr == text.data() + text.size()) {
                return value;
            }
        }
    }
    return core::kMetricMeasurement;
}

}  // namespace

void HatchCommand::start(CommandContext& context) {
    m_stage = Stage::Boundary;
    m_pattern = context.defaults.hatchPattern;
    m_scale = context.defaults.hatchScale;
    m_angle = context.defaults.hatchAngle;
    m_islands = context.defaults.hatchIslands;
    m_pointLoops.clear();
    m_selectedLoops.clear();
    m_seeds.clear();
    m_analyzed = false;
    context.editor.beginTransaction("Hatch");
    currentPattern(context);
}

Status HatchCommand::handle(CommandContext& context, const Input& input) {
    if (m_stage == Stage::Boundary) {
        if (const auto* point = std::get_if<PointInput>(&input)) {
            context.lastPoint = point->point;
            usePoint(context, point->point);
        } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
            m_stage = keyword->keyword == "Properties" ? Stage::Pattern : Stage::Selection;
        } else if (std::holds_alternative<ConfirmInput>(input)) {
            return finish(context);
        }
        return Status::Continue;
    }
    if (m_stage == Stage::Selection) {
        if (const auto* selection = std::get_if<SelectionInput>(&input)) {
            useSelection(context, *selection);
            m_stage = Stage::Boundary;
            currentPattern(context);
        }
        return Status::Continue;
    }
    if (m_stage == Stage::Pattern) {
        std::string_view answer;
        if (const auto* text = std::get_if<TextInput>(&input)) {
            answer = detail::trim(text->text);
        } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
            answer = keyword->keyword;
        } else if (std::holds_alternative<ConfirmInput>(input)) {
            answer = m_pattern;
        } else {
            return Status::Continue;
        }
        if (isPattern(answer, "Solid") || isPattern(answer, "S")) {
            m_pattern = "SOLID";
            context.defaults.hatchPattern = m_pattern;
            m_stage = Stage::Boundary;
            currentPattern(context);
        } else if (isPattern(answer, "ANSI31") || isPattern(answer, "NET")) {
            m_pattern = isPattern(answer, "NET") ? "NET" : "ANSI31";
            context.defaults.hatchPattern = m_pattern;
            m_stage = Stage::Scale;
        } else {
            context.message("Unknown pattern name.");
        }
        return Status::Continue;
    }
    if (m_stage == Stage::Scale) {
        if (const auto* value = std::get_if<ValueInput>(&input)) {
            if (!(value->value > 0.0) || !std::isfinite(value->value)) {
                context.message("Value must be positive and nonzero.");
            } else {
                m_scale = value->value;
                context.defaults.hatchScale = m_scale;
                m_stage = Stage::Angle;
            }
        } else if (std::holds_alternative<ConfirmInput>(input)) {
            m_stage = Stage::Angle;
        }
        return Status::Continue;
    }
    if (const auto* value = std::get_if<ValueInput>(&input)) {
        if (std::isfinite(value->value)) {
            m_angle = core::reducedDegreesToRadians(value->value);
            context.defaults.hatchAngle = m_angle;
            m_stage = Stage::Boundary;
            currentPattern(context);
        }
    } else if (std::holds_alternative<ConfirmInput>(input)) {
        m_stage = Stage::Boundary;
        currentPattern(context);
    }
    return Status::Continue;
}

Prompt HatchCommand::prompt() const {
    switch (m_stage) {
        case Stage::Boundary:
            return {"Specify internal point or [Properties/Select objects]:",
                    {{"Properties", "P"}, {"Select objects", "S"}}};
        case Stage::Pattern:
            return {"Enter a pattern name or [Solid] <" + m_pattern + ">:",
                    {{"Solid", "S"}},
                    PromptKind::Text};
        case Stage::Scale:
            return {"Specify a scale for the pattern <" + formatLength(m_scale) + ">:",
                    {},
                    PromptKind::Number};
        case Stage::Angle:
            return {"Specify an angle for the pattern <" + formatAngle(m_angle) + ">:",
                    {},
                    PromptKind::Number};
        case Stage::Selection:
            return {"Select objects:", {}, PromptKind::Selection};
    }
    return {};
}

void HatchCommand::currentPattern(CommandContext& context) const {
    context.message("Current hatch pattern: " + m_pattern);
}

void HatchCommand::usePoint(CommandContext& context, const core::Vec2& point) {
    core::DetectedBoundary detected = core::detectHatchBoundary(context.editor.document(), point);
    if (detected.outcome == core::BoundaryOutcome::OnCurve) {
        context.message("Point is directly on an object.");
    } else {
        if (!m_analyzed) {
            context.message("Selecting everything...");
            context.message("Selecting everything visible...");
            context.message("Analyzing the selected data...");
            m_analyzed = true;
        }
        if (detected.outcome == core::BoundaryOutcome::NotEnclosed) {
            context.message("Valid hatch boundary not found.");
            currentPattern(context);
            return;
        }
        context.message("Analyzing internal islands...");
        const bool duplicate =
            !detected.loops.empty() &&
            std::ranges::any_of(m_pointLoops, [&](const core::HatchLoop& loop) {
                return (loop.flags & kExternalLoop) != 0 && loop == detected.loops.front();
            });
        if (!duplicate) {
            m_pointLoops.insert(m_pointLoops.end(), detected.loops.begin(), detected.loops.end());
        }
        m_seeds.push_back(point);
    }
    currentPattern(context);
}

void HatchCommand::useSelection(CommandContext& context, const SelectionInput& selection) {
    core::ChosenContours chosen =
        core::chosenHatchContours(context.editor.document(), selection.ids);
    m_selectedLoops = std::move(chosen.loops);
}

Status HatchCommand::finish(CommandContext& context) {
    if (m_pointLoops.empty() && m_selectedLoops.empty()) {
        context.editor.rollback();
        return Status::Finished;
    }
    core::Hatch hatch;
    hatch.pattern = m_pattern;
    hatch.solid = m_pattern == "SOLID";
    hatch.islands = m_islands;
    hatch.angle = m_angle;
    hatch.scale = m_scale;
    hatch.seeds = m_seeds;
    hatch.loops = m_pointLoops;
    hatch.loops.insert(hatch.loops.end(), m_selectedLoops.begin(), m_selectedLoops.end());
    hatch.patternLines = core::predefinedPatternLines(m_pattern, m_scale, m_angle,
                                                      measurementOf(context.editor.document()));
    if (!hatch.solid && core::patternLines(hatch).outcome == core::PatternOutcome::TooDense) {
        context.message("Hatch spacing too dense, or dash size too small.");
        context.editor.rollback();
        return Status::Finished;
    }
    context.editor.add(std::move(hatch));
    context.editor.commit();
    return Status::Finished;
}

CommandDescription HatchCommand::description() {
    return {"-HATCH",
            {"HATCH"},
            "Creates a hatch from points or selected objects.",
            false,
            {{"internal-point",
              "Specify internal point or [Properties/Select objects]:",
              ValueType::Point,
              {{"Properties", "P"}, {"Select objects", "S"}},
              true},
             {"pattern",
              "Enter a pattern name or [Solid] <{default}>:",
              ValueType::Name,
              {{"Solid", "S"}},
              true},
             {"scale", "Specify a scale for the pattern <{default}>:", ValueType::Real, {}, true},
             {"angle", "Specify an angle for the pattern <{default}>:", ValueType::Real, {}, true},
             {"objects", "Select objects:", ValueType::Selection, {}, true}}};
}

void HpislanddetectionCommand::start(CommandContext& context) {
    m_value = context.defaults.hatchIslands;
}

Status HpislanddetectionCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        std::string_view value = detail::trim(text->text);
        if (value.starts_with('+')) {
            value.remove_prefix(1);
        }
        int number = 0;
        const auto result = std::from_chars(value.data(), value.data() + value.size(), number);
        if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || number < 0 ||
            number > 2) {
            context.message("Requires an integer between 0 and 2.");
            return Status::Continue;
        }
        context.defaults.hatchIslands = static_cast<core::IslandStyle>(number);
        return Status::Finished;
    }
    return std::holds_alternative<ConfirmInput>(input) ? Status::Finished : Status::Continue;
}

Prompt HpislanddetectionCommand::prompt() const {
    return {
        "Enter new value for HPISLANDDETECTION <" + std::to_string(islandNumber(m_value)) + ">:",
        {},
        PromptKind::Text};
}

CommandDescription HpislanddetectionCommand::description() {
    return {"HPISLANDDETECTION",
            {},
            "Sets the hatch island detection style.",
            false,
            {{"value",
              "Enter new value for HPISLANDDETECTION <{default}>:",
              ValueType::Integer,
              {},
              true}}};
}

}  // namespace tg::cmd
