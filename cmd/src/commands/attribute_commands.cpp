// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/attribute_commands.hpp"

#include "../text.hpp"
#include "tg/core/insert.hpp"
#include "tg/edit/blocks.hpp"
#include "tg/edit/document_tables.hpp"

#include <variant>

namespace tg::cmd {
namespace {

bool hasAttributes(const core::BlockDefinition& block) {
    return block.hasAttributes;
}

std::string answer(const Input& input) {
    if (const auto* text = std::get_if<TextInput>(&input)) {
        return std::string{detail::trim(text->text)};
    }
    if (const auto* keyword = std::get_if<KeywordInput>(&input)) {
        return keyword->keyword;
    }
    return {};
}

}  // namespace

void AttsyncCommand::start(CommandContext& context) {
    const auto blocks = context.editor.document().blocks().definitions();
    if (std::ranges::none_of(blocks, hasAttributes)) {
        context.message("This drawing contains no blocks with attributes.");
        m_stage = Stage::Finished;
        return;
    }
    context.editor.beginTransaction("Attsync");
}

Status AttsyncCommand::handle(CommandContext& context, const Input& input) {
    if (m_stage == Stage::Option) {
        const std::string value = answer(input);
        if (value == "?") {
            listBlocks(context);
        } else if (detail::equalsIgnoringCase(value, "Name") ||
                   detail::equalsIgnoringCase(value, "N")) {
            m_stage = Stage::Name;
        } else if (value.empty() || detail::equalsIgnoringCase(value, "Select") ||
                   detail::equalsIgnoringCase(value, "S")) {
            m_stage = Stage::Select;
        }
        return Status::Continue;
    }
    if (m_stage == Stage::Name) {
        const std::string name = answer(input);
        if (name == "?") {
            listBlocks(context);
            return Status::Continue;
        }
        return synchronize(context, name);
    }
    if (m_stage == Stage::Select) {
        const auto* object = std::get_if<ObjectInput>(&input);
        if (object == nullptr) {
            return Status::Continue;
        }
        const core::EntityRecord* record = context.editor.document().find(object->id);
        const auto* insert =
            record == nullptr ? nullptr : std::get_if<core::Insert>(&record->entity);
        if (insert == nullptr) {
            context.message("Object is not a block reference.");
            return Status::Continue;
        }
        return synchronize(context, insert->block);
    }
    return Status::Finished;
}

void AttsyncCommand::listBlocks(CommandContext& context) {
    for (const core::BlockDefinition& block : context.editor.document().blocks().definitions()) {
        if (hasAttributes(block)) {
            context.message(block.name);
        }
    }
}

Status AttsyncCommand::synchronize(CommandContext& context, std::string_view name) {
    const core::BlockDefinition* block = context.editor.document().blocks().find(name);
    if (block == nullptr || !hasAttributes(*block)) {
        context.message("The block table contains no block with attributes named " +
                        std::string{name} + ".");
        context.editor.rollback();
        m_stage = Stage::Finished;
        return Status::Finished;
    }
    edit::synchronizeInserts(context.editor, block->name);
    context.editor.commit();
    context.message("ATTSYNC complete.");
    m_stage = Stage::Finished;
    return Status::Finished;
}

Prompt AttsyncCommand::prompt() const {
    switch (m_stage) {
        case Stage::Option:
            return {"Enter an option [?/Name/Select] <Select>:",
                    {{"?", "?"}, {"Name", "N"}, {"Select", "S"}},
                    PromptKind::Text};
        case Stage::Name:
            return {"Enter name of block to synchronize or [?]:", {{"?", "?"}}, PromptKind::Text};
        case Stage::Select:
            return {"Select a block:", {}, PromptKind::Object};
        case Stage::Finished:
            return {};
    }
    return {};
}

CommandDescription AttsyncCommand::description() {
    return {"ATTSYNC",
            {},
            "Synchronizes block attributes with their definitions.",
            false,
            {{"option",
              "Enter an option [?/Name/Select] <Select>:",
              ValueType::Keyword,
              {{"?", "?"}, {"Name", "N"}, {"Select", "S"}}},
             {"name",
              "Enter name of block to synchronize or [?]:",
              ValueType::Name,
              {{"?", "?"}},
              true},
             {"block", "Select a block:", ValueType::Object, {}, true}}};
}

void AttdispCommand::start(CommandContext& /*context*/) {}

Status AttdispCommand::handle(CommandContext& context, const Input& input) {
    const std::string value = answer(input);
    if (!value.empty()) {
        if (detail::equalsIgnoringCase(value, "Normal") || detail::equalsIgnoringCase(value, "N")) {
            m_display = core::AttributeDisplay::Normal;
        } else if (detail::equalsIgnoringCase(value, "ON")) {
            m_display = core::AttributeDisplay::On;
        } else if (detail::equalsIgnoringCase(value, "OFF")) {
            m_display = core::AttributeDisplay::Off;
        } else {
            context.message("Invalid option keyword.");
            return Status::Continue;
        }
    }
    context.editor.tables().setAttributeDisplay(m_display);
    context.message("Regenerating model.");
    return Status::Finished;
}

Prompt AttdispCommand::prompt() const {
    return {"Enter attribute visibility setting [Normal/ON/OFF] <Normal>:",
            {{"Normal", "N"}, {"ON", "ON"}, {"OFF", "OFF"}},
            PromptKind::Text};
}

CommandDescription AttdispCommand::description() {
    return {"ATTDISP",
            {},
            "Controls the visibility of attributes.",
            false,
            {{"visibility",
              "Enter attribute visibility setting [Normal/ON/OFF] <Normal>:",
              ValueType::Keyword,
              {{"Normal", "N"}, {"ON", "ON"}, {"OFF", "OFF"}}}}};
}

}  // namespace tg::cmd
