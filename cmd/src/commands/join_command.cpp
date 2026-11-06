// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/join_command.hpp"

#include <algorithm>
#include <utility>

namespace tg::cmd {
namespace {

std::string discardedMessage(std::size_t count) {
    const char* suffix = count == 1 ? " object discarded from the operation."
                                    : " objects discarded from the operation.";
    return std::to_string(count) + suffix;
}

}  // namespace

void JoinCommand::start(CommandContext& context) {
    m_script = context.scriptSequences;
    m_stage = m_script ? Stage::Source : Stage::Interactive;
    m_ids.clear();
    m_pending.reset();
    context.editor.beginTransaction("Join");
}

Status JoinCommand::handle(CommandContext& context, const Input& input) {
    const auto* keyword = std::get_if<KeywordInput>(&input);
    const auto* object = std::get_if<ObjectInput>(&input);
    const bool confirmed = std::holds_alternative<ConfirmInput>(input);

    switch (m_stage) {
        case Stage::Interactive:
        case Stage::Lines:
        case Stage::Objects:
            if (const auto* selection = std::get_if<SelectionInput>(&input)) {
                for (const auto id : selection->ids) {
                    if (std::ranges::find(m_ids, id) == m_ids.end()) {
                        m_ids.push_back(id);
                    }
                }
                return joinSelected(context);
            }
            break;
        case Stage::Source:
            if (object != nullptr) {
                const auto* record = context.editor.document().find(object->id);
                if (record != nullptr) {
                    m_ids.push_back(object->id);
                    if (std::holds_alternative<core::Line>(record->entity)) {
                        m_stage = Stage::Lines;
                    } else if (std::holds_alternative<core::Arc>(record->entity)) {
                        m_stage = Stage::Arcs;
                    } else {
                        m_stage = Stage::Objects;
                    }
                }
            } else if (confirmed) {
                context.editor.rollback();
                return Status::Finished;
            }
            break;
        case Stage::Arcs:
            if (object != nullptr && std::ranges::find(m_ids, object->id) == m_ids.end()) {
                m_ids.push_back(object->id);
            } else if (keyword != nullptr && keyword->keyword == "cLose") {
                return joinSelected(context, true);
            } else if (confirmed) {
                return joinSelected(context);
            }
            break;
        case Stage::Circle:
            if (m_pending && (confirmed || (keyword != nullptr && keyword->keyword == "Yes"))) {
                return apply(context, std::move(*m_pending));
            }
            if (keyword != nullptr && keyword->keyword == "No") {
                context.editor.rollback();
                return Status::Finished;
            }
            break;
    }
    return Status::Continue;
}

Prompt JoinCommand::prompt() const {
    switch (m_stage) {
        case Stage::Interactive:
            return {"Select source object or multiple objects to join at once:",
                    {},
                    PromptKind::Selection};
        case Stage::Source:
            return {"Select source object:", {}, PromptKind::Object};
        case Stage::Lines:
            return {"Select lines to join to source:", {}, PromptKind::Selection};
        case Stage::Arcs:
            return {
                "Select arcs to join to source or [cLose]:", {{"cLose", "L"}}, PromptKind::Object};
        case Stage::Objects:
            return {"Select objects to join to source:", {}, PromptKind::Selection};
        case Stage::Circle:
            return {"Arc segments combined form a circle. Convert to circle? [Yes/No] <Yes>:",
                    {{"Yes", "Y"}, {"No", "N"}}};
    }
    return {};
}

Status JoinCommand::joinSelected(CommandContext& context, bool close) {
    if (m_ids.empty()) {
        context.editor.rollback();
        return Status::Finished;
    }

    const auto& document = context.editor.document();
    std::vector<core::Entity> entities;
    entities.reserve(m_ids.size());
    for (const auto id : m_ids) {
        const auto* record = document.find(id);
        if (record == nullptr) {
            context.editor.rollback();
            return Status::Finished;
        }
        entities.push_back(record->entity);
    }

    const auto rule = core::ToleranceRule::of(document);
    auto result = m_script ? core::joinToSource(entities, rule, close) : core::join(entities, rule);
    if (!result) {
        context.message("0 objects joined, " + discardedMessage(m_ids.size()));
        context.editor.rollback();
        return Status::Finished;
    }
    if (result->formsCircle) {
        m_pending = std::move(result);
        m_stage = Stage::Circle;
        return Status::Continue;
    }
    return apply(context, std::move(*result));
}

Status JoinCommand::apply(CommandContext& context, core::JoinResult result) {
    const auto source = m_ids.front();
    const auto& entity = context.editor.document().find(source)->entity;
    std::string message = successMessage(result, entity);
    const auto discarded = m_ids.size() - result.consumed.size() - 1;
    if (discarded != 0) {
        if (message.ends_with('.')) {
            message.pop_back();
        }
        message += ", " + discardedMessage(discarded);
    }

    if (result.created) {
        context.editor.addCopy(source, std::move(*result.created));
        context.editor.remove(source);
    } else if (result.source) {
        context.editor.replace(source, std::move(*result.source));
    }
    for (const auto index : result.consumed) {
        context.editor.remove(m_ids[index]);
    }
    context.editor.commit();
    context.message(std::move(message));
    return Status::Finished;
}

std::string JoinCommand::successMessage(const core::JoinResult& result,
                                        const core::Entity& source) const {
    const auto consumed = result.consumed.size();
    const auto joined = std::to_string(consumed + 1);
    if (result.created) {
        if (std::holds_alternative<core::Circle>(*result.created)) {
            return consumed == 0 ? "Arc converted to circle."
                                 : joined + " arcs joined and converted to circle";
        }
        return joined + " objects converted to 1 polyline";
    }
    if (std::holds_alternative<core::Line>(source)) {
        if (m_script && consumed == 1) {
            return "1 line joined to source";
        }
        return joined + " lines joined into 1 line";
    }
    if (std::holds_alternative<core::Arc>(source)) {
        return joined + " arcs joined into 1 arc";
    }

    const auto segments = std::get<core::Polyline>(result.source.value()).segmentCount() -
                          std::get<core::Polyline>(source).segmentCount();
    return std::to_string(segments) +
           (segments == 1 ? " segment added to polyline" : " segments added to polyline");
}

}  // namespace tg::cmd
