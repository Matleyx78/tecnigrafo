// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/hatchedit_command.hpp"

#include "../text.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/hatch_patterns.hpp"

#include <charconv>
#include <cmath>
#include <string_view>
#include <system_error>
#include <variant>

namespace tg::cmd {
namespace {

bool answerIs(std::string_view answer, std::string_view value, std::string_view shortcut) {
    return detail::equalsIgnoringCase(answer, value) ||
           detail::equalsIgnoringCase(answer, shortcut);
}

bool generatedPattern(std::string_view name) {
    return answerIs(name, "ANSI31", "ANSI31") || answerIs(name, "NET", "NET") ||
           answerIs(name, "SOLID", "SOLID");
}

int measurementOf(const core::Document& document) {
    for (const core::RawSection& section : document.raw().sections) {
        if (section.name != "HEADER") {
            continue;
        }
        for (std::size_t i = 0; i + 1 < section.groups.size(); ++i) {
            if (section.groups[i].code != 9 ||
                detail::trim(section.groups[i].value) != "$MEASUREMENT") {
                continue;
            }
            int value = core::kMetricMeasurement;
            const std::string_view text = detail::trim(section.groups[i + 1].value);
            const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
            if (result.ec == std::errc{} && result.ptr == text.data() + text.size()) {
                return value;
            }
        }
    }
    return core::kMetricMeasurement;
}

std::string styleName(core::IslandStyle style) {
    switch (style) {
        case core::IslandStyle::Ignore:
            return "Ignore";
        case core::IslandStyle::Outer:
            return "Outer";
        case core::IslandStyle::Normal:
            return "Normal";
    }
    return "Normal";
}

}  // namespace

void HatcheditCommand::start(CommandContext& context) {
    m_stage = Stage::Select;
    context.editor.beginTransaction("Hatchedit");
}

bool HatcheditCommand::select(CommandContext& context, core::EntityId id) {
    const core::EntityRecord* record = context.editor.document().find(id);
    const auto* hatch = record == nullptr ? nullptr : std::get_if<core::Hatch>(&record->entity);
    if (hatch == nullptr) {
        context.message("The selected object must be an associative hatch object.");
        return false;
    }
    m_target = record->id;
    m_hatch = *hatch;
    m_extras = record->extras;
    m_origin = core::patternOrigin(m_extras);
    m_newOrigin = m_origin;
    m_pattern = m_hatch.pattern;
    m_scale = m_hatch.scale;
    m_angle = m_hatch.angle;
    return true;
}

Status HatcheditCommand::handle(CommandContext& context, const Input& input) {
    if (m_stage == Stage::Select) {
        if (const auto* object = std::get_if<ObjectInput>(&input);
            object != nullptr && select(context, object->id)) {
            m_stage = Stage::Option;
        }
        return Status::Continue;
    }
    if (m_stage == Stage::Option) {
        std::string_view answer = "Properties";
        if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
            answer = keyword->keyword;
        } else if (!std::holds_alternative<ConfirmInput>(input)) {
            return Status::Continue;
        }
        if (answerIs(answer, "Style", "S")) {
            m_stage = Stage::Style;
        } else if (answerIs(answer, "Origin", "O")) {
            m_stage = Stage::OriginOption;
        } else {
            m_stage = Stage::Pattern;
        }
        return Status::Continue;
    }
    if (m_stage == Stage::Pattern) {
        std::string_view answer = m_pattern;
        if (const auto* text = std::get_if<TextInput>(&input)) {
            answer = detail::trim(text->text);
        } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
            answer = keyword->keyword;
        } else if (!std::holds_alternative<ConfirmInput>(input)) {
            return Status::Continue;
        }
        if (!answerIs(answer, m_pattern, m_pattern) && !generatedPattern(answer)) {
            context.message("Unknown pattern name.");
            return Status::Continue;
        }
        if (answerIs(answer, "NET", "NET")) {
            m_pattern = "NET";
        } else if (answerIs(answer, "SOLID", "SOLID")) {
            m_pattern = "SOLID";
        } else if (generatedPattern(answer)) {
            m_pattern = "ANSI31";
        }
        m_stage = m_pattern == "SOLID" ? Stage::Angle : Stage::Scale;
        return Status::Continue;
    }
    if (m_stage == Stage::Scale) {
        if (const auto* value = std::get_if<ValueInput>(&input)) {
            if (!(value->value > 0.0) || !std::isfinite(value->value)) {
                context.message("Value must be positive and nonzero.");
                return Status::Continue;
            }
            m_scale = value->value;
        } else if (!std::holds_alternative<ConfirmInput>(input)) {
            return Status::Continue;
        }
        m_stage = Stage::Angle;
        return Status::Continue;
    }
    if (m_stage == Stage::Angle) {
        if (const auto* value = std::get_if<ValueInput>(&input)) {
            if (!std::isfinite(value->value)) {
                return Status::Continue;
            }
            m_angle = core::reducedDegreesToRadians(value->value);
        } else if (!std::holds_alternative<ConfirmInput>(input)) {
            return Status::Continue;
        }
        return apply(context);
    }
    if (m_stage == Stage::Style) {
        std::string answer = styleName(m_hatch.islands);
        if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
            answer = keyword->keyword;
        } else if (!std::holds_alternative<ConfirmInput>(input)) {
            return Status::Continue;
        }
        if (answerIs(answer, "Ignore", "I")) {
            m_hatch.islands = core::IslandStyle::Ignore;
        } else if (answerIs(answer, "Outer", "O")) {
            m_hatch.islands = core::IslandStyle::Outer;
        } else {
            m_hatch.islands = core::IslandStyle::Normal;
        }
        context.defaults.hatchIslands = m_hatch.islands;
        return apply(context);
    }
    if (m_stage == Stage::OriginOption) {
        if (const auto* keyword = std::get_if<KeywordInput>(&input);
            keyword != nullptr && answerIs(keyword->keyword, "Set new origin", "S")) {
            m_stage = Stage::OriginPoint;
            return Status::Continue;
        }
        if (std::holds_alternative<ConfirmInput>(input) ||
            std::holds_alternative<KeywordInput>(input)) {
            m_newOrigin = context.defaults.hatchOrigin;
            return apply(context);
        }
        return Status::Continue;
    }
    if (m_stage == Stage::OriginPoint) {
        if (const auto* point = std::get_if<PointInput>(&input)) {
            m_newOrigin = point->point;
            context.lastPoint = point->point;
            m_stage = Stage::StoreOrigin;
        }
        return Status::Continue;
    }
    if (const auto* keyword = std::get_if<KeywordInput>(&input);
        keyword != nullptr && answerIs(keyword->keyword, "Yes", "Y")) {
        context.defaults.hatchOrigin = m_newOrigin;
    }
    if (std::holds_alternative<ConfirmInput>(input) ||
        std::holds_alternative<KeywordInput>(input)) {
        return apply(context);
    }
    return Status::Continue;
}

Status HatcheditCommand::apply(CommandContext& context) {
    if (m_stage == Stage::Angle) {
        const auto changed = core::withPattern(m_hatch, m_origin, m_pattern, m_scale, m_angle,
                                               measurementOf(context.editor.document()));
        if (!changed) {
            context.message("Unknown pattern name.");
            return Status::Continue;
        }
        m_hatch = *changed;
        m_hatch.pattern = m_pattern;
        if (generatedPattern(m_pattern)) {
            context.defaults.hatchPattern = m_pattern;
        }
        context.defaults.hatchScale = m_scale;
        context.defaults.hatchAngle = m_angle;
    }
    if (m_stage == Stage::StoreOrigin || m_stage == Stage::OriginOption) {
        m_hatch = core::withPatternOrigin(m_hatch, m_origin, m_newOrigin);
        m_extras = core::withPatternOriginXdata(std::move(m_extras), m_newOrigin);
    }
    context.editor.replace(m_target, std::move(m_hatch), std::move(m_extras));
    context.editor.commit();
    return Status::Finished;
}

Prompt HatcheditCommand::prompt() const {
    switch (m_stage) {
        case Stage::Select:
            return {"Select hatch object:", {}, PromptKind::Object};
        case Stage::Option:
            return {"Enter hatch option [Style/Properties/Origin] <Properties>:",
                    {{"Style", "S"}, {"Properties", "P"}, {"Origin", "O"}}};
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
        case Stage::Style:
            return {
                "Enter hatching style [Ignore/Outer/Normal] <" + styleName(m_hatch.islands) + ">:",
                {{"Ignore", "I"}, {"Outer", "O"}, {"Normal", "N"}}};
        case Stage::OriginOption:
            return {"[Use current origin/Set new origin] <Use current origin>:",
                    {{"Use current origin", "U"}, {"Set new origin", "S"}}};
        case Stage::OriginPoint:
            return {"Specify origin point:", {}, PromptKind::Point};
        case Stage::StoreOrigin:
            return {"Store this as default origin? [Yes/No] <N>:", {{"Yes", "Y"}, {"No", "N"}}};
    }
    return {};
}

CommandDescription HatcheditCommand::description() {
    return {"-HATCHEDIT",
            {"HATCHEDIT", "HE"},
            "Edits an associative hatch.",
            false,
            {{"hatch", "Select hatch object:", ValueType::Object, {}, false},
             {"option",
              "Enter hatch option [Style/Properties/Origin] <Properties>:",
              ValueType::Keyword,
              {{"Style", "S"}, {"Properties", "P"}, {"Origin", "O"}},
              true},
             {"pattern",
              "Enter a pattern name or [Solid] <{default}>:",
              ValueType::Name,
              {{"Solid", "S"}},
              true},
             {"scale", "Specify a scale for the pattern <{default}>:", ValueType::Real, {}, true},
             {"angle", "Specify an angle for the pattern <{default}>:", ValueType::Real, {}, true},
             {"style",
              "Enter hatching style [Ignore/Outer/Normal] <{default}>:",
              ValueType::Keyword,
              {{"Ignore", "I"}, {"Outer", "O"}, {"Normal", "N"}},
              true},
             {"origin-option",
              "[Use current origin/Set new origin] <Use current origin>:",
              ValueType::Keyword,
              {{"Use current origin", "U"}, {"Set new origin", "S"}},
              true},
             {"origin", "Specify origin point:", ValueType::Point, {}, true},
             {"store-origin",
              "Store this as default origin? [Yes/No] <N>:",
              ValueType::Keyword,
              {{"Yes", "Y"}, {"No", "N"}},
              true}}};
}

}  // namespace tg::cmd
