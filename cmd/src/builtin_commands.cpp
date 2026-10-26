// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"

#include "command_descriptions.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/commands/arc_command.hpp"
#include "tg/cmd/commands/circle_command.hpp"
#include "tg/cmd/commands/erase_command.hpp"
#include "tg/cmd/commands/file_commands.hpp"
#include "tg/cmd/commands/layer_commands.hpp"
#include "tg/cmd/commands/line_command.hpp"
#include "tg/cmd/commands/pline_command.hpp"
#include "tg/cmd/commands/text_command.hpp"

#include <memory>

namespace tg::cmd {

void registerBuiltinCommands(CommandRegistry& registry) {
    // Each command with its description (docs/adr/0026), in alphabetical order.
    registry.add(LayerCommand::description(), [] { return std::make_unique<LayerCommand>(); });
    registry.add(detail::arcDescription(), [] { return std::make_unique<ArcCommand>(); });
    registry.add(detail::circleDescription(), [] { return std::make_unique<CircleCommand>(); });
    registry.add(ClayerCommand::description(), [] { return std::make_unique<ClayerCommand>(); });
    registry.add(detail::eraseDescription(), [] { return std::make_unique<EraseCommand>(); });
    registry.add(detail::insunitsDescription(), [] { return std::make_unique<InsunitsCommand>(); });
    registry.add(detail::lineDescription(), [] { return std::make_unique<LineCommand>(); });
    registry.add(detail::newDescription(), [] { return std::make_unique<NewCommand>(); });
    registry.add(detail::openDescription(), [] { return std::make_unique<OpenCommand>(); });
    registry.add(detail::plineDescription(), [] { return std::make_unique<PlineCommand>(); });
    registry.add(detail::qsaveDescription(), [] { return std::make_unique<QsaveCommand>(); });
    registry.add(detail::saveDescription(), [] { return std::make_unique<SaveCommand>(); });
    registry.add(detail::saveasDescription(), [] { return std::make_unique<SaveasCommand>(); });
    registry.add(detail::textDescription(), [] { return std::make_unique<TextCommand>(); });
}

}  // namespace tg::cmd
