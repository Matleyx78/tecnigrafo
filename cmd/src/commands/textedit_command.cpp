// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/textedit_command.hpp"

#include "tg/core/dimension.hpp"

#include <algorithm>
#include <variant>

namespace tg::cmd {
namespace {

std::string dimensionValue(const core::Dimension& dimension) {
    return dimension.presentation.textOverride.empty() ? "<>" : dimension.presentation.textOverride;
}

std::optional<core::MText> dimensionMText(const core::Dimension& dimension) {
    const auto found = std::ranges::find_if(dimension.graphics, [](const auto& record) {
        return std::holds_alternative<core::MText>(record.entity);
    });
    if (found == dimension.graphics.end()) {
        return std::nullopt;
    }
    return std::get<core::MText>(found->entity);
}

}  // namespace

CommandDescription TexteditCommand::description() {
    return {"TEXTEDIT",
            {"ED", "DDEDIT"},
            "Edits text and annotation objects.",
            false,
            {{"object-multiple",
              "Select an annotation object or [Undo/Mode]:",
              ValueType::Object,
              {{"Undo", "U"}, {"Mode", "M"}},
              true},
             {"object-single",
              "Select an annotation object or [Mode]:",
              ValueType::Object,
              {{"Mode", "M"}},
              true},
             {"mode",
              "Enter a text edit mode option [Single/Multiple] <Multiple>:",
              ValueType::Keyword,
              {{"Single", "S"}, {"Multiple", "M"}},
              true},
             {"text", "Enter new text <{default}>:", ValueType::Text, {}, true},
             // A TEXT or an MTEXT whose value is empty: {default} is never
             // empty (docs/tappe/0.8.1/PT12-editor.md, an empty MTEXT opened).
             {"text-empty", "Enter new text <>:", ValueType::Text, {}, true},
             {"dimension-measurement", "Enter new text <<>>:", ValueType::Text, {}, true},
             {"dimension-suffix", "Enter new text <<>{default}>:", ValueType::Text, {}, true},
             {"tag", "Enter attribute tag <{default}>:", ValueType::Text, {}, true},
             {"attribute-prompt", "Enter attribute prompt <{default}>:", ValueType::Text, {}, true},
             {"attribute-value",
              "Enter default attribute value <{default}>:",
              ValueType::Text,
              {},
              true},
             {"insert-value", "{default} <{default}>:", ValueType::Text, {}, true}}};
}

void TexteditCommand::start(CommandContext& context) {
    resetObject();
    m_multiple = context.defaults.textEditMultiple;
    m_edits = 0;
    m_document = &context.editor.document();
    context.message("Current settings: Edit mode = " +
                    std::string(m_multiple ? "Multiple" : "Single"));
    context.editor.beginTransaction("Textedit");
}

Status TexteditCommand::handle(CommandContext& context, const Input& input) {
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const auto* text = std::get_if<TextInput>(&input);
    const bool confirm = std::holds_alternative<ConfirmInput>(input);

    if (m_stage == Stage::Object) {
        if (const auto* object = std::get_if<ObjectInput>(&input)) {
            select(context, object->id);
        } else if (keyword != nullptr && keyword->keyword == "Mode") {
            m_stage = Stage::Mode;
        } else if (keyword != nullptr && keyword->keyword == "Undo" && m_multiple) {
            context.editor.rollback();
            if (m_edits > 0) {
                (void)context.editor.undo();
                --m_edits;
            }
            context.message("The command has been completely undone.");
            context.editor.beginTransaction("Textedit");
        } else if (confirm) {
            context.editor.rollback();
            return Status::Finished;
        }
    } else if (m_stage == Stage::Mode && (keyword != nullptr || confirm)) {
        if (keyword != nullptr) {
            m_multiple = keyword->keyword == "Multiple";
        } else {
            m_multiple = true;
        }
        context.defaults.textEditMultiple = m_multiple;
        m_stage = Stage::Object;
    } else if (m_stage == Stage::Text && (text != nullptr || confirm)) {
        if (text != nullptr &&
            (!text->text.empty() || std::holds_alternative<core::Dimension>(m_original))) {
            core::Entity replacement = m_original;
            if (auto* plainText = std::get_if<core::Text>(&replacement)) {
                plainText->value = text->text;
            } else if (auto* multilineText = std::get_if<core::MText>(&replacement)) {
                multilineText->value = text->text;
            } else if (auto* dimension = std::get_if<core::Dimension>(&replacement)) {
                dimension->presentation.textOverride = text->text;
                if (auto regenerated = core::regenerated(context.editor.document(), *dimension)) {
                    replacement = std::move(*regenerated);
                }
            }
            m_objectChanged = context.editor.replace(m_target, std::move(replacement));
        }
        completeObject(context);
        if (!m_multiple) {
            return Status::Finished;
        }
    } else if (m_stage == Stage::AttributeTag && (text != nullptr || confirm)) {
        auto definition = std::get<core::AttributeDefinition>(m_original);
        if (text != nullptr) {
            if (!core::validAttributeTag(text->text)) {
                context.message("Invalid tag name.");
                return Status::Continue;
            }
            definition.tag = text->text;
        }
        m_objectChanged = context.editor.replace(m_target, definition) || m_objectChanged;
        m_original = std::move(definition);
        m_stage = Stage::AttributePrompt;
    } else if (m_stage == Stage::AttributePrompt && (text != nullptr || confirm)) {
        auto definition = std::get<core::AttributeDefinition>(m_original);
        if (text != nullptr) {
            definition.prompt = text->text;
        }
        m_objectChanged = context.editor.replace(m_target, definition) || m_objectChanged;
        m_original = std::move(definition);
        m_stage = Stage::Attributes;
    } else if (m_stage == Stage::Attributes && (text != nullptr || confirm)) {
        if (auto* definition = std::get_if<core::AttributeDefinition>(&m_original)) {
            if (text != nullptr) {
                definition->text.value = text->text;
                m_objectChanged = context.editor.replace(m_target, *definition) || m_objectChanged;
            }
            completeObject(context);
            if (!m_multiple) {
                return Status::Finished;
            }
        } else {
            auto insert = std::get<core::Insert>(m_original);
            if (text != nullptr) {
                insert.attributes[m_attribute].text.value = text->text;
                m_objectChanged = context.editor.replace(m_target, insert) || m_objectChanged;
            }
            m_original = std::move(insert);
            ++m_attribute;
            if (m_attribute >= std::get<core::Insert>(m_original).attributes.size()) {
                completeObject(context);
                if (!m_multiple) {
                    return Status::Finished;
                }
            }
        }
    }
    return Status::Continue;
}

void TexteditCommand::select(CommandContext& context, core::EntityId id) {
    const core::EntityRecord* record = context.editor.document().find(id);
    if (record == nullptr || (!std::holds_alternative<core::Text>(record->entity) &&
                              !std::holds_alternative<core::MText>(record->entity) &&
                              !std::holds_alternative<core::Dimension>(record->entity) &&
                              !std::holds_alternative<core::AttributeDefinition>(record->entity) &&
                              !std::holds_alternative<core::Insert>(record->entity))) {
        return;
    }
    m_target = id;
    m_original = record->entity;
    m_attribute = 0;
    m_objectChanged = false;
    if (std::holds_alternative<core::AttributeDefinition>(m_original)) {
        m_stage = Stage::AttributeTag;
    } else if (const auto* insert = std::get_if<core::Insert>(&m_original)) {
        if (insert->attributes.empty()) {
            resetObject();
        } else {
            m_stage = Stage::Attributes;
        }
    } else {
        m_stage = Stage::Text;
    }
}

void TexteditCommand::completeObject(CommandContext& context) {
    context.editor.commit();
    if (m_objectChanged) {
        ++m_edits;
    }
    resetObject();
    if (!m_multiple) {
        return;
    }
    context.editor.beginTransaction("Textedit");
}

void TexteditCommand::resetObject() {
    m_stage = Stage::Object;
    m_target = {};
    m_original = core::Line{};
    m_attribute = 0;
}

void TexteditCommand::cancel(CommandContext& context) {
    if (context.editor.inTransaction()) {
        context.editor.rollback();
    }
}

void TexteditCommand::pointerMoved(const core::Vec2& /*point*/) {}

Prompt TexteditCommand::prompt() const {
    if (m_stage == Stage::Object) {
        if (m_multiple) {
            return {"Select an annotation object or [Undo/Mode]:",
                    {{"Undo", "U"}, {"Mode", "M"}},
                    PromptKind::Object};
        }
        return {"Select an annotation object or [Mode]:", {{"Mode", "M"}}, PromptKind::Object};
    }
    if (m_stage == Stage::Mode) {
        return {"Enter a text edit mode option [Single/Multiple] <Multiple>:",
                {{"Single", "S"}, {"Multiple", "M"}}};
    }
    if (m_stage == Stage::AttributeTag) {
        return {
            "Enter attribute tag <" + std::get<core::AttributeDefinition>(m_original).tag + ">:",
            {},
            PromptKind::Text};
    }
    if (m_stage == Stage::AttributePrompt) {
        return {"Enter attribute prompt <" +
                    std::get<core::AttributeDefinition>(m_original).prompt + ">:",
                {},
                PromptKind::Text};
    }
    if (const auto* definition = std::get_if<core::AttributeDefinition>(&m_original)) {
        return {"Enter default attribute value <" + definition->text.value + ">:",
                {},
                PromptKind::Text};
    }
    if (const auto* insert = std::get_if<core::Insert>(&m_original)) {
        const auto& attribute = insert->attributes[m_attribute];
        return {attribute.tag + " <" + attribute.text.value + ">:", {}, PromptKind::Text};
    }
    std::string value;
    if (const auto* plainText = std::get_if<core::Text>(&m_original)) {
        value = plainText->value;
    } else if (const auto* multilineText = std::get_if<core::MText>(&m_original)) {
        value = multilineText->value;
    } else if (const auto* dimension = std::get_if<core::Dimension>(&m_original)) {
        value = dimensionValue(*dimension);
    }
    return {"Enter new text <" + value + ">:", {}, PromptKind::Text};
}

std::span<const core::Entity> TexteditCommand::preview() const {
    return {};
}

std::optional<TextEditRequest> TexteditCommand::textEditing() const {
    if (m_stage != Stage::Text) {
        return std::nullopt;
    }
    if (const auto* text = std::get_if<core::Text>(&m_original)) {
        return TextEditRequest{m_target, *text, text->value, std::nullopt};
    }
    if (const auto* text = std::get_if<core::MText>(&m_original)) {
        return TextEditRequest{m_target, *text, text->value, std::nullopt};
    }
    const auto* dimension = std::get_if<core::Dimension>(&m_original);
    if (dimension == nullptr) {
        return std::nullopt;
    }
    const auto shown = dimensionMText(*dimension);
    if (!shown) {
        return std::nullopt;
    }
    if (m_document == nullptr) {
        return std::nullopt;
    }
    const auto variables = core::dimensionVariables(
        m_document->dimStyles(), dimension->presentation.style, dimension->presentation.overrides);
    double measurement = dimension->value.measurement;
    if (dimension->definition.kind == core::DimensionKind::Angular) {
        measurement = core::radiansToDegrees(measurement);
    }
    return TextEditRequest{
        m_target, *shown, dimensionValue(*dimension),
        core::dimensionText(variables, dimension->definition.kind, measurement, "<>")};
}

}  // namespace tg::cmd
