// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/erase_command.hpp"

#include <variant>

namespace tg::cmd {

void EraseCommand::start(CommandContext& context) {
    context.editor.beginTransaction("Erase");
}

Status EraseCommand::handle(CommandContext& context, const Input& input) {
    if (const auto* selection = std::get_if<SelectionInput>(&input)) {
        for (const core::EntityId id : selection->ids) {
            // Editor::remove leaves an already absent entity unchanged.
            context.editor.remove(id);
        }
        context.editor.commit();
        return Status::Finished;
    }
    return Status::Continue;
}

void EraseCommand::pointerMoved(const core::Vec2& /*point*/) {}

Prompt EraseCommand::prompt() const {
    return {"Select objects:", {}};
}

std::span<const core::Entity> EraseCommand::preview() const {
    return {};
}

bool EraseCommand::needsSelection() const {
    return true;
}

}  // namespace tg::cmd
