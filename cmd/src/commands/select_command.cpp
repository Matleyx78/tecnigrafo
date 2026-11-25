// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/commands/select_command.hpp"

namespace tg::cmd {

CommandDescription SelectCommand::description() {
    return {"SELECT", {}, "Chooses objects for the Previous option of later commands.", true, {}};
}

void SelectCommand::start(CommandContext& /*context*/) {
    // The runner has already recorded the objects as the previous selection
    // set: an empty prompt ends the command at once.
}

Status SelectCommand::handle(CommandContext& /*context*/, const Input& /*input*/) {
    return Status::Finished;
}

void SelectCommand::pointerMoved(const core::Vec2& /*point*/) {}

Prompt SelectCommand::prompt() const {
    return {};
}

std::span<const core::Entity> SelectCommand::preview() const {
    return {};
}

bool SelectCommand::needsSelection() const {
    return true;
}

}  // namespace tg::cmd
