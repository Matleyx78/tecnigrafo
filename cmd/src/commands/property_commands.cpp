// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/property_commands.hpp"

#include "../name_list.hpp"
#include "../text.hpp"
#include "tg/cmd/coordinate_input.hpp"
#include "tg/cmd/value_format.hpp"

#include <cmath>
#include <optional>
#include <utility>

namespace tg::cmd {
namespace {

std::optional<std::string_view> answer(const Input& input) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        return detail::trim(text->text);
    }
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        return keyword->keyword;
    }
    if (std::holds_alternative<ConfirmInput>(input)) {
        return std::string_view{};
    }
    return std::nullopt;
}

std::string weightLabel(int value) {
    if (value == core::lineweight::kByLayer) {
        return "ByLayer";
    }
    if (value == core::lineweight::kByBlock) {
        return "ByBlock";
    }
    if (value == core::lineweight::kDefault) {
        return "Default";
    }
    return core::formatLineweight(value);
}

bool option(std::string_view text, std::string_view name, std::string_view shortcut) {
    return detail::equalsIgnoringCase(text, name) || detail::equalsIgnoringCase(text, shortcut);
}

}  // namespace

void ColorCommand::start(CommandContext& context) {
    const auto color = context.editor.document().current().color;
    m_color = core::formatColor(color);
    if (color.kind() == core::Color::Kind::ByLayer || color.kind() == core::Color::Kind::ByBlock) {
        for (auto& ch : m_color) {
            ch = detail::toUpperAscii(ch);
        }
    }
}

Status ColorCommand::handle(CommandContext& context, const Input& input) {
    const auto text = answer(input);
    if (!text) {
        return Status::Continue;
    }
    if (text->empty()) {
        return Status::Finished;
    }
    if (!m_rgb && option(*text, "Truecolor", "T")) {
        m_rgb = true;
        return Status::Continue;
    }
    const auto color = core::parseColor(*text);
    if (!color || (m_rgb && color->kind() != core::Color::Kind::Rgb)) {
        context.message("Invalid color.");
        return Status::Continue;
    }
    auto current = context.editor.document().current();
    current.color = *color;
    context.editor.beginTransaction("COLOR");
    context.editor.setCurrent(current);
    context.editor.commit();
    return Status::Finished;
}

Prompt ColorCommand::prompt() const {
    if (m_rgb) {
        return {"Red, Green, Blue:", {}, PromptKind::Text};
    }
    return {"Enter default object color [Truecolor] <" + m_color + ">:",
            {{"Truecolor", "T"}},
            PromptKind::Text};
}

void LinetypeCommand::start(CommandContext& context) {
    const auto& document = context.editor.document();
    m_currentName = document.linetype(document.current().linetype)->name;
    context.editor.beginTransaction("LINETYPE");
    context.message("Current line type: \"" + m_currentName + "\"");
}

void LinetypeCommand::cancel(CommandContext& context) {
    context.editor.commit();
}

void LinetypeCommand::list(CommandContext& context) {
    for (const auto& type : context.editor.document().linetypes()) {
        context.message("\"" + type.name + "\" " + type.description);
    }
}

void LinetypeCommand::load(CommandContext& context, std::string_view names) {
    for (const auto& name : detail::splitNameList(names)) {
        if (context.editor.document().findLinetype(name) != nullptr) {
            context.message("Linetype \"" + name + "\" is already loaded.");
            continue;
        }
        const auto definition = core::standardLinetype(name);
        if (!definition) {
            context.message("Linetype \"" + name + "\" not found.");
            continue;
        }
        core::Linetype type;
        type.name = definition->name;
        type.description = definition->description;
        type.pattern = definition->pattern;
        context.editor.addLinetype(std::move(type));
    }
}

Status LinetypeCommand::handle(CommandContext& context, const Input& input) {
    const auto text = answer(input);
    if (!text) {
        return Status::Continue;
    }
    if (m_stage == Stage::Option) {
        if (text->empty()) {
            context.editor.commit();
            return Status::Finished;
        }
        if (*text == "?") {
            list(context);
        } else if (option(*text, "Load", "L")) {
            m_stage = Stage::Load;
        } else if (option(*text, "Set", "S")) {
            m_stage = Stage::Set;
        } else {
            context.message("Invalid option keyword.");
        }
    } else if (m_stage == Stage::Load) {
        if (!text->empty()) {
            load(context, *text);
        }
        m_stage = Stage::Option;
    } else if (*text == "?") {
        list(context);
    } else {
        const auto name = text->empty() ? std::string_view(m_currentName) : *text;
        const auto* type = context.editor.document().findLinetype(name);
        if (type == nullptr) {
            context.message("Linetype \"" + std::string(name) + "\" is not loaded.");
        } else {
            auto current = context.editor.document().current();
            current.linetype = type->id;
            context.editor.setCurrent(current);
            m_currentName = type->name;
        }
        m_stage = Stage::Option;
    }
    return Status::Continue;
}

Prompt LinetypeCommand::prompt() const {
    if (m_stage == Stage::Load) {
        return {"Enter linetype(s) to load:", {}, PromptKind::Text};
    }
    if (m_stage == Stage::Set) {
        return {"Specify linetype name or [?] <" + m_currentName + ">:",
                {{"?", "?"}},
                PromptKind::Text};
    }
    return {"Enter an option [?/Load/Set]:",
            {{"?", "?"}, {"Load", "L"}, {"Set", "S"}},
            PromptKind::Text};
}

void ScaleCommand::start(CommandContext& context) {
    const auto& document = context.editor.document();
    m_scale =
        m_variable == Variable::Global ? document.ltscale() : document.current().linetypeScale;
}

Status ScaleCommand::handle(CommandContext& context, const Input& input) {
    const auto text = answer(input);
    if (!text) {
        return Status::Continue;
    }
    if (text->empty()) {
        return Status::Finished;
    }
    const auto number = parseCoordinate(*text);
    if (!number || number->kind != PointSpec::Kind::Distance || !std::isfinite(number->a) ||
        number->a <= 0.0) {
        context.message("Requires a positive number.");
        return Status::Continue;
    }
    context.editor.beginTransaction(m_variable == Variable::Global ? "LTSCALE" : "CELTSCALE");
    if (m_variable == Variable::Global) {
        context.editor.setLtscale(number->a);
    } else {
        auto current = context.editor.document().current();
        current.linetypeScale = number->a;
        context.editor.setCurrent(current);
    }
    context.editor.commit();
    return Status::Finished;
}

Prompt ScaleCommand::prompt() const {
    std::string text = "Enter new linetype scale factor <";
    if (m_variable == Variable::Current) {
        text = "Enter new value for CELTSCALE <";
    }
    return {text + formatLength(m_scale) + ">:", {}, PromptKind::Text};
}

void LweightCommand::start(CommandContext& context) {
    context.message("Current lineweight: " +
                    weightLabel(context.editor.document().current().lineweight));
}

Status LweightCommand::handle(CommandContext& context, const Input& input) {
    const auto text = answer(input);
    if (!text) {
        return Status::Continue;
    }
    if (text->empty()) {
        return Status::Finished;
    }
    if (*text == "?") {
        std::string values = "ByLayer, ByBlock, Default";
        for (const int weight : core::lineweight::kAllowed) {
            values += ", " + core::formatLineweight(weight);
        }
        context.message(std::move(values));
        return Status::Continue;
    }
    const auto weight = core::parseLineweight(*text);
    if (!weight) {
        context.message("Invalid lineweight.");
        return Status::Continue;
    }
    auto current = context.editor.document().current();
    current.lineweight = *weight;
    context.editor.beginTransaction("LWEIGHT");
    context.editor.setCurrent(current);
    context.editor.commit();
    return Status::Finished;
}

Prompt LweightCommand::prompt() const {
    return {"Enter default lineweight for new objects or [?]:", {{"?", "?"}}, PromptKind::Text};
}

void LwdisplayCommand::start(CommandContext& context) {
    m_on = context.editor.document().lwdisplay();
}

Status LwdisplayCommand::handle(CommandContext& context, const Input& input) {
    const auto text = answer(input);
    if (!text) {
        return Status::Continue;
    }
    if (text->empty()) {
        return Status::Finished;
    }
    if (!detail::equalsIgnoringCase(*text, "ON") && !detail::equalsIgnoringCase(*text, "OFF")) {
        context.message("Requires ON or OFF.");
        return Status::Continue;
    }
    context.editor.setLwdisplay(detail::equalsIgnoringCase(*text, "ON"));
    return Status::Finished;
}

Prompt LwdisplayCommand::prompt() const {
    return {std::string("Enter new value for LWDISPLAY <") + (m_on ? "ON" : "OFF") + ">:",
            {},
            PromptKind::Text};
}

}  // namespace tg::cmd
