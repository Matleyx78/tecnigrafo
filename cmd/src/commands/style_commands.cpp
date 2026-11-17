// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/style_commands.hpp"

#include "../text.hpp"
#include "tg/cmd/value_format.hpp"
#include "tg/core/math.hpp"
#include "tg/edit/text_styles.hpp"

#include <cmath>
#include <utility>

namespace tg::cmd {
namespace {

Prompt textPrompt(std::string text, std::vector<Keyword> keywords = {}) {
    return {std::move(text), std::move(keywords), PromptKind::Text};
}

bool yes(std::string_view value) {
    return detail::equalsIgnoringCase(value, "Yes") || detail::equalsIgnoringCase(value, "Y");
}

bool no(std::string_view value) {
    return detail::equalsIgnoringCase(value, "No") || detail::equalsIgnoringCase(value, "N");
}

}  // namespace

void StyleCommand::start(CommandContext& context) {
    m_stage = Stage::Name;
    m_current = context.editor.document().textStyles().current();
    context.editor.beginTransaction("Style");
}

void StyleCommand::cancel(CommandContext& context) {
    context.editor.rollback();
}

Status StyleCommand::handle(CommandContext& context, const Input& input) {
    const auto* text = std::get_if<TextInput>(&input);
    const auto* value = std::get_if<ValueInput>(&input);
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const bool confirm = std::holds_alternative<ConfirmInput>(input);
    std::string_view entered;
    if (text != nullptr) {
        entered = detail::trim(text->text);
    }
    if (keyword != nullptr) {
        entered = keyword->keyword;
    }

    if (m_stage == Stage::Name && (text != nullptr || keyword != nullptr || confirm)) {
        const auto name = entered.empty() ? std::string_view(m_current) : entered;
        if (name == "?") {
            for (const auto& style : context.editor.document().textStyles().styles()) {
                context.message(style.name);
            }
            return Status::Continue;
        }
        if (!core::isValidTableName(name)) {
            context.message("Invalid text style name.");
            return Status::Continue;
        }
        const auto* existing = context.editor.document().textStyles().find(name);
        if (existing != nullptr) {
            m_style = *existing;
            context.message("Existing style.");
        } else {
            m_style = {};
            m_style.name = std::string(name);
            m_style.font = "txt";
            context.message("New style.");
        }
        m_stage = Stage::Font;
    } else if (m_stage == Stage::Font && (text != nullptr || confirm)) {
        if (!entered.empty()) {
            m_style.font = entered;
        }
        m_stage = Stage::Height;
    } else if ((m_stage == Stage::Height || m_stage == Stage::Width || m_stage == Stage::Oblique) &&
               (value != nullptr || confirm)) {
        const double number = value == nullptr ? 0.0 : value->value;
        if (value != nullptr &&
            (!std::isfinite(number) || (m_stage == Stage::Height && number < 0.0) ||
             (m_stage == Stage::Width && number <= 0.0))) {
            context.message(m_stage == Stage::Width ? "Value must be positive and nonzero."
                                                    : "Value must be nonnegative.");
            return Status::Continue;
        }
        if (value != nullptr) {
            if (m_stage == Stage::Height) {
                m_style.fixedHeight = number;
            }
            if (m_stage == Stage::Width) {
                m_style.widthFactor = number;
            }
            if (m_stage == Stage::Oblique) {
                m_style.oblique = number;
            }
        }
        if (m_stage == Stage::Height) {
            m_stage = Stage::Width;
        } else if (m_stage == Stage::Width) {
            m_stage = Stage::Oblique;
        } else {
            m_stage = Stage::Backwards;
        }
    } else if ((m_stage == Stage::Backwards || m_stage == Stage::UpsideDown) &&
               (text != nullptr || keyword != nullptr || confirm)) {
        bool result = m_stage == Stage::Backwards ? m_style.backwards : m_style.upsideDown;
        if (!entered.empty()) {
            if (!yes(entered) && !no(entered)) {
                context.message("Invalid option keyword.");
                return Status::Continue;
            }
            result = yes(entered);
        }
        if (m_stage == Stage::Backwards) {
            m_style.backwards = result;
            m_stage = Stage::UpsideDown;
        } else {
            m_style.upsideDown = result;
            edit::putTextStyle(context.editor, m_style);
            edit::setCurrentTextStyle(context.editor, m_style.name);
            context.editor.commit();
            context.message("\"" + m_style.name + "\" is now the current text style.");
            return Status::Finished;
        }
    }
    return Status::Continue;
}

Prompt StyleCommand::prompt() const {
    switch (m_stage) {
        case Stage::Name:
            return textPrompt("Enter name of text style or [?] <" + m_current + ">:", {{"?", "?"}});
        case Stage::Font:
            return textPrompt("Specify full font name or font filename (TTF or SHX) <" +
                              m_style.font + ">:");
        case Stage::Height:
            return {"Specify height of text <" + formatLength(m_style.fixedHeight) + ">:",
                    {},
                    PromptKind::Distance};
        case Stage::Width:
            return {"Specify width factor <" + formatLength(m_style.widthFactor) + ">:",
                    {},
                    PromptKind::Number};
        case Stage::Oblique:
            return {"Specify obliquing angle <" + formatAngle(m_style.oblique) + ">:",
                    {},
                    PromptKind::Angle};
        case Stage::Backwards:
            return textPrompt("Display text backwards? [Yes/No] <" +
                                  std::string(m_style.backwards ? "Yes" : "No") + ">:",
                              {{"Yes", "Y"}, {"No", "N"}});
        case Stage::UpsideDown:
            return textPrompt("Display text upside-down? [Yes/No] <" +
                                  std::string(m_style.upsideDown ? "Yes" : "No") + ">:",
                              {{"Yes", "Y"}, {"No", "N"}});
    }
    return {};
}

void TextstyleCommand::start(CommandContext& context) {
    m_current = context.editor.document().textStyles().current();
    context.editor.beginTransaction("TEXTSTYLE");
}

Status TextstyleCommand::handle(CommandContext& context, const Input& input) {
    const auto* text = std::get_if<TextInput>(&input);
    if (text == nullptr && !std::holds_alternative<ConfirmInput>(input)) {
        return Status::Continue;
    }
    const auto name = text == nullptr ? std::string_view(m_current) : detail::trim(text->text);
    if (context.editor.document().textStyles().find(name) == nullptr) {
        context.message("Cannot find text style \"" + std::string(name) + "\".");
        return Status::Continue;
    }
    edit::setCurrentTextStyle(context.editor, name);
    context.editor.commit();
    return Status::Finished;
}

Prompt TextstyleCommand::prompt() const {
    return textPrompt("Enter new value for TEXTSTYLE <\"" + m_current + "\">:");
}

CommandDescription StyleCommand::description() {
    return {"-STYLE",
            {"STYLE"},
            "Creates or changes a text style.",
            false,
            {{"style-name",
              "Enter name of text style or [?] <{default}>:",
              ValueType::Name,
              {{"?", "?"}},
              true},
             {"font",
              "Specify full font name or font filename (TTF or SHX) <{default}>:",
              ValueType::Text,
              {},
              true},
             {"height", "Specify height of text <{default}>:", ValueType::Distance, {}, true},
             {"width", "Specify width factor <{default}>:", ValueType::Real, {}, true},
             {"oblique", "Specify obliquing angle <{default}>:", ValueType::Angle, {}, true},
             {"backwards",
              "Display text backwards? [Yes/No] <{default}>:",
              ValueType::Keyword,
              {{"Yes", "Y"}, {"No", "N"}},
              true},
             {"upside-down",
              "Display text upside-down? [Yes/No] <{default}>:",
              ValueType::Keyword,
              {{"Yes", "Y"}, {"No", "N"}},
              true}}};
}

CommandDescription TextstyleCommand::description() {
    return {"TEXTSTYLE",
            {},
            "Sets the current text style.",
            false,
            {{"style-name",
              "Enter new value for TEXTSTYLE <\"{default}\">:",
              ValueType::Name,
              {},
              true}}};
}

}  // namespace tg::cmd
