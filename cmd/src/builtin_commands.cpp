// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"

#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/commands/pline_command.hpp"

#include <memory>

namespace tg::cmd {

void registerBuiltinCommands(CommandRegistry& registry) {
    registry.add("PLINE", {"PL"}, [] { return std::make_unique<PlineCommand>(); });
}

}  // namespace tg::cmd
