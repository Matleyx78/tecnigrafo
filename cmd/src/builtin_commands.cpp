// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"

#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/commands/arc_command.hpp"
#include "tg/cmd/commands/circle_command.hpp"
#include "tg/cmd/commands/erase_command.hpp"
#include "tg/cmd/commands/line_command.hpp"
#include "tg/cmd/commands/pline_command.hpp"

#include <memory>

namespace tg::cmd {

void registerBuiltinCommands(CommandRegistry& registry) {
    registry.add("ARC", {"A"}, [] { return std::make_unique<ArcCommand>(); });
    registry.add("CIRCLE", {"C"}, [] { return std::make_unique<CircleCommand>(); });
    registry.add("ERASE", {"E"}, [] { return std::make_unique<EraseCommand>(); });
    registry.add("LINE", {"L"}, [] { return std::make_unique<LineCommand>(); });
    registry.add("PLINE", {"PL"}, [] { return std::make_unique<PlineCommand>(); });
}

}  // namespace tg::cmd
