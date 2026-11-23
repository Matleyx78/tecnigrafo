// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/block_command.hpp"

#include "../text.hpp"
#include "tg/core/layer.hpp"
#include "tg/edit/blocks.hpp"

#include <variant>

namespace tg::cmd {
namespace {

bool isYes(std::string_view text) {
    return detail::equalsIgnoringCase(text, "Yes") || detail::equalsIgnoringCase(text, "Y");
}

bool isNo(std::string_view text) {
    return text.empty() || detail::equalsIgnoringCase(text, "No") ||
           detail::equalsIgnoringCase(text, "N");
}

}  // namespace

void BlockCommand::start(CommandContext& context) {
    context.editor.beginTransaction("Block");
}

Status BlockCommand::handle(CommandContext& context, const Input& input) {
    if (m_stage == Stage::Name) {
        std::string text;
        if (const auto* typed = std::get_if<TextInput>(&input)) {
            text = std::string(detail::trim(typed->text));
        } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
            text = keyword->keyword;
        } else {
            return Status::Continue;
        }
        if (text == "?") {
            listBlocks(context);
            return Status::Continue;
        }
        if (!core::isValidTableName(text)) {
            context.message("Invalid block name.");
            return Status::Continue;
        }
        m_name = std::move(text);
        m_redefining = context.editor.document().blocks().find(m_name) != nullptr;
        m_stage = m_redefining ? Stage::Redefine : Stage::Base;
        return Status::Continue;
    }
    if (m_stage == Stage::Redefine) {
        std::string_view answer;
        if (const auto* typed = std::get_if<TextInput>(&input)) {
            answer = detail::trim(typed->text);
        } else if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
            answer = keyword->keyword;
        } else if (!std::holds_alternative<ConfirmInput>(input)) {
            return Status::Continue;
        }
        if (isNo(answer)) {
            context.editor.rollback();
            return Status::Finished;
        }
        if (!isYes(answer)) {
            context.message("Invalid option keyword.");
            return Status::Continue;
        }
        m_stage = Stage::Base;
        return Status::Continue;
    }
    if (m_stage == Stage::Base) {
        if (const auto* point = std::get_if<PointInput>(&input)) {
            m_base = point->point;
            context.lastPoint = point->point;
            m_stage = Stage::Selection;
        }
        return Status::Continue;
    }
    const auto* selection = std::get_if<SelectionInput>(&input);
    if (selection == nullptr) {
        return Status::Continue;
    }
    if (selection->ids.empty()) {
        context.message("No objects selected.");
        context.editor.rollback();
        return Status::Finished;
    }
    if (!edit::defineBlock(context.editor, m_name, m_base, selection->ids)) {
        context.message("Block \"" + m_name + "\" references itself.");
        context.editor.rollback();
        return Status::Finished;
    }
    if (m_redefining) {
        context.message("Block \"" + m_name + "\" redefined");
    }
    context.editor.commit();
    return Status::Finished;
}

void BlockCommand::listBlocks(CommandContext& context) {
    for (const core::BlockDefinition& block : context.editor.document().blocks().definitions()) {
        context.message(block.name);
    }
}

Prompt BlockCommand::prompt() const {
    switch (m_stage) {
        case Stage::Name:
            return {"Enter block name or [?]:", {{"?", "?"}}, PromptKind::Text};
        case Stage::Redefine:
            return {"Block \"" + m_name + "\" already exists. Redefine it? [Yes/No] <N>:",
                    {{"Yes", "Y"}, {"No", "N"}},
                    PromptKind::Text};
        case Stage::Base:
            return {"Specify insertion base point:", {}, PromptKind::Point};
        case Stage::Selection:
            return {"Select objects:", {}, PromptKind::Selection};
    }
    return {};
}

std::optional<core::Vec2> BlockCommand::basePoint() const {
    return m_stage == Stage::Selection ? std::optional{m_base} : std::nullopt;
}

CommandDescription BlockCommand::description() {
    return {"-BLOCK",
            {"BLOCK", "B"},
            "Creates a block definition from selected objects.",
            false,
            {{"block-name", "Enter block name or [?]:", ValueType::Name, {{"?", "?"}}},
             {"redefine",
              "Block \"{default}\" already exists. Redefine it? [Yes/No] <N>:",
              ValueType::Keyword,
              {{"Yes", "Y"}, {"No", "N"}},
              true},
             {"base-point", "Specify insertion base point:", ValueType::Point, {}, false},
             {"objects", "Select objects:", ValueType::Selection, {}, true}}};
}

}  // namespace tg::cmd
