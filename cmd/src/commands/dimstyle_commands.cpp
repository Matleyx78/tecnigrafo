// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/dimstyle_commands.hpp"

#include "../text.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/edit/dim_styles.hpp"

#include <charconv>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>

namespace tg::cmd {
namespace {

Prompt textPrompt(std::string text, std::vector<Keyword> keywords = {}) {
    return {std::move(text), std::move(keywords), PromptKind::Text};
}

std::string valueText(const core::DimValue& value, bool quoted = false) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        return quoted ? "\"" + *text + "\"" : *text;
    }
    const double number = std::get<double>(value);
    if (number == std::trunc(number)) {
        return std::to_string(static_cast<int>(number));
    }
    return formatLength(number);
}

bool isRealVariable(std::string_view name) {
    return name == "DIMASZ" || name == "DIMCEN" || name == "DIMEXE" || name == "DIMEXO" ||
           name == "DIMGAP" || name == "DIMLFAC" || name == "DIMRND" || name == "DIMSCALE" ||
           name == "DIMTXT";
}

std::string displayValue(std::string_view name, const core::DimValue& value) {
    if (const auto* text = std::get_if<std::string>(&value)) {
        return *text;
    }
    const double number = std::get<double>(value);
    if (name == "DIMTIH" || name == "DIMTOH" || name == "DIMSE1" || name == "DIMSE2" ||
        name == "DIMSD1" || name == "DIMSD2") {
        return number == 0.0 ? "Off" : "On";
    }
    if (name.starts_with("DIMCLR") && number == 0.0) {
        return "BYBLOCK";
    }
    return isRealVariable(name) ? formatLength(number) : std::to_string(static_cast<int>(number));
}

std::string descriptionOf(std::string_view name) {
    if (name == "DIMADEC") {
        return "Angular decimal places";
    }
    if (name == "DIMASZ") {
        return "Arrow size";
    }
    if (name == "DIMAZIN") {
        return "Angular zero suppression";
    }
    if (name == "DIMCEN") {
        return "Center mark size";
    }
    if (name == "DIMCLRD") {
        return "Dimension line and leader color";
    }
    if (name == "DIMCLRE") {
        return "Extension line color";
    }
    if (name == "DIMCLRT") {
        return "Dimension text color";
    }
    if (name == "DIMDEC") {
        return "Decimal places";
    }
    if (name == "DIMDSEP") {
        return "Decimal separator";
    }
    if (name == "DIMEXE") {
        return "Extension above dimension line";
    }
    if (name == "DIMEXO") {
        return "Extension line origin offset";
    }
    if (name == "DIMGAP") {
        return "Gap from dimension line to text";
    }
    if (name == "DIMLFAC") {
        return "Linear unit scale factor";
    }
    if (name == "DIMLWD") {
        return "Dimension line and leader lineweight";
    }
    if (name == "DIMLWE") {
        return "Extension line lineweight";
    }
    if (name == "DIMPOST") {
        return "Default prefix and suffix for dimension text";
    }
    if (name == "DIMRND") {
        return "Rounding value";
    }
    if (name == "DIMSCALE") {
        return "Overall scale factor";
    }
    if (name == "DIMSD1") {
        return "Suppress the first dimension line";
    }
    if (name == "DIMSD2") {
        return "Suppress the second dimension line";
    }
    if (name == "DIMSE1") {
        return "Suppress the first extension line";
    }
    if (name == "DIMSE2") {
        return "Suppress the second extension line";
    }
    if (name == "DIMTAD") {
        return "Place text above the dimension line";
    }
    if (name == "DIMTIH") {
        return "Text inside extensions is horizontal";
    }
    if (name == "DIMTOH") {
        return "Text outside horizontal";
    }
    if (name == "DIMTXSTY") {
        return "Text style";
    }
    if (name == "DIMTXT") {
        return "Text height";
    }
    return "Zero suppression";
}

void showCurrent(CommandContext& context) {
    const auto& table = context.editor.document().dimStyles();
    context.message("Current dimension style: " + table.current());
    const auto* style = table.find(table.current());
    if (style == nullptr || style->vars == table.currentVariables()) {
        return;
    }
    context.message("Current dimension overrides:");
    for (const auto& info : core::dimVariableInfos()) {
        const auto current = core::dimValue(table.currentVariables(), info.name);
        if (current != core::dimValue(style->vars, info.name)) {
            std::ostringstream line;
            line << ' ' << std::left << std::setw(12) << info.name
                 << displayValue(info.name, current);
            context.message(line.str());
        }
    }
}

void listStyles(CommandContext& context) {
    context.message("Named dimension styles:");
    for (const auto& style : context.editor.document().dimStyles().styles()) {
        context.message("  " + style.name);
    }
}

void showStatus(CommandContext& context) {
    const auto& variables = context.editor.document().dimStyles().currentVariables();
    for (const auto& info : core::dimVariableInfos()) {
        std::ostringstream line;
        line << std::left << std::setw(12) << info.name << std::setw(21)
             << displayValue(info.name, core::dimValue(variables, info.name))
             << descriptionOf(info.name);
        context.message(line.str());
    }
}

}  // namespace

void DimstyleCommand::start(CommandContext& context) {
    m_stage = Stage::Option;
    context.editor.beginTransaction("-DIMSTYLE");
    showCurrent(context);
}

Status DimstyleCommand::handle(CommandContext& context, const Input& input) {
    const auto* text = std::get_if<TextInput>(&input);
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const bool confirm = std::holds_alternative<ConfirmInput>(input);
    std::string_view entered;
    if (text != nullptr) {
        entered = detail::trim(text->text);
    }
    if (keyword != nullptr) {
        entered = keyword->keyword;
    }
    if (text == nullptr && keyword == nullptr && !confirm) {
        return Status::Continue;
    }

    if (m_stage == Stage::Option) {
        if (entered.empty() || detail::equalsIgnoringCase(entered, "Restore") ||
            detail::equalsIgnoringCase(entered, "R")) {
            m_stage = Stage::RestoreName;
        } else if (detail::equalsIgnoringCase(entered, "Save") ||
                   detail::equalsIgnoringCase(entered, "S")) {
            m_stage = Stage::SaveName;
        } else if (detail::equalsIgnoringCase(entered, "Status") ||
                   detail::equalsIgnoringCase(entered, "ST")) {
            showStatus(context);
            context.editor.rollback();
            return Status::Finished;
        } else if (entered == "?") {
            m_stage = Stage::ListPattern;
        }
    } else if (m_stage == Stage::ListPattern) {
        listStyles(context);
        context.editor.rollback();
        return Status::Finished;
    } else if (m_stage == Stage::SaveName) {
        if (entered == "?") {
            listStyles(context);
        } else if (context.editor.document().dimStyles().find(entered) != nullptr) {
            m_name = entered;
            m_stage = Stage::Redefine;
        } else if (edit::saveDimStyle(context.editor, entered)) {
            context.editor.commit();
            return Status::Finished;
        }
    } else if (m_stage == Stage::Redefine) {
        if (detail::equalsIgnoringCase(entered, "Yes") ||
            detail::equalsIgnoringCase(entered, "Y")) {
            edit::saveDimStyle(context.editor, m_name);
            context.editor.commit();
            return Status::Finished;
        }
        m_stage = Stage::SaveName;
    } else if (m_stage == Stage::RestoreName) {
        if (entered == "?") {
            listStyles(context);
        } else if (!edit::restoreDimStyle(context.editor, entered)) {
            context.message("Unknown dimension style.");
            showCurrent(context);
            context.editor.rollback();
            return Status::Finished;
        } else {
            context.editor.commit();
            showCurrent(context);
            return Status::Finished;
        }
    }
    return Status::Continue;
}

Prompt DimstyleCommand::prompt() const {
    switch (m_stage) {
        case Stage::Option:
            return textPrompt("Enter a dimension style option [Save/Restore/STatus/?] <Restore>:",
                              {{"Save", "S"}, {"Restore", "R"}, {"STatus", "ST"}, {"?", "?"}});
        case Stage::SaveName:
            return textPrompt("Enter name for new dimension style or [?]:", {{"?", "?"}});
        case Stage::Redefine:
            return textPrompt("That name is already in use, redefine it? <N>:");
        case Stage::RestoreName:
            return textPrompt("Enter a dimension style name, [?] or <select dimension>:",
                              {{"?", "?"}});
        case Stage::ListPattern:
            return textPrompt("Enter dimension style(s) to list <*>:");
    }
    return {};
}

CommandDescription DimstyleCommand::description() {
    return {"-DIMSTYLE",
            {"DIMSTYLE"},
            "Manages dimension styles.",
            false,
            {{"option",
              "Enter a dimension style option [Save/Restore/STatus/?] <{default}>:",
              ValueType::Keyword,
              {{"Save", "S"}, {"Restore", "R"}, {"STatus", "ST"}, {"?", "?"}},
              true},
             {"save-name",
              "Enter name for new dimension style or [?]:",
              ValueType::Name,
              {{"?", "?"}},
              false},
             {"redefine",
              "That name is already in use, redefine it? <{default}>:",
              ValueType::Keyword,
              {},
              true},
             {"restore-name",
              "Enter a dimension style name, [?] or <{default}>:",
              ValueType::Name,
              {{"?", "?"}},
              true},
             {"list-pattern",
              "Enter dimension style(s) to list <{default}>:",
              ValueType::Text,
              {},
              true}}};
}

void DimVariableCommand::start(CommandContext& context) {
    context.editor.beginTransaction(m_name);
    m_current = core::dimValue(context.editor.document().dimStyles().currentVariables(), m_name);
}

Status DimVariableCommand::handle(CommandContext& context, const Input& input) {
    if (std::holds_alternative<ConfirmInput>(input)) {
        context.editor.rollback();
        return Status::Finished;
    }
    const auto* text = std::get_if<TextInput>(&input);
    if (!text) {
        return Status::Continue;
    }
    const std::string_view entered = detail::trim(text->text);
    core::DimValue value{std::string{entered}};
    if (!std::holds_alternative<std::string>(m_current) || m_name == "DIMDSEP") {
        double number = 0.0;
        auto parsed = std::from_chars(entered.data(), entered.data() + entered.size(), number);
        if (parsed.ec == std::errc{} && parsed.ptr == entered.data() + entered.size()) {
            value = number;
        } else if (m_name != "DIMDSEP") {
            value = std::string{entered};
        }
    }
    auto candidate = context.editor.document().dimStyles().currentVariables();
    const bool kindOk = core::setDimValue(candidate, m_name, value);
    const bool domainOk =
        kindOk && core::DimStyleTable::inDomain(candidate, context.editor.document().textStyles());
    if (!domainOk) {
        if (m_name == "DIMDEC" && std::holds_alternative<double>(value)) {
            context.message("Requires an integer between 0 and 8.");
            return Status::Continue;
        }
        context.message("Cannot set " + m_name + " to that value.");
        context.message("*Invalid*");
        context.editor.rollback();
        return Status::Finished;
    }
    edit::setCurrentDimVariable(context.editor, m_name, value);
    context.editor.commit();
    return Status::Finished;
}

Prompt DimVariableCommand::prompt() const {
    const bool quoted = m_name == "DIMDSEP" || m_name == "DIMTXSTY";
    std::string shown = valueText(m_current, quoted);
    if (std::holds_alternative<double>(m_current) && isRealVariable(m_name)) {
        shown = formatLength(std::get<double>(m_current));
    }
    return textPrompt("Enter new value for " + m_name + " <" + shown + ">:");
}

CommandDescription DimVariableCommand::description(std::string_view name) {
    return {std::string{name},
            {},
            "Sets a current dimension variable.",
            false,
            {{"value",
              "Enter new value for " + std::string{name} + " <{default}>:",
              ValueType::Text,
              {},
              true}}};
}

}  // namespace tg::cmd
