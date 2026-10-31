// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"

#include "command_descriptions.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/commands/aid_commands.hpp"
#include "tg/cmd/commands/arc_command.hpp"
#include "tg/cmd/commands/chprop_command.hpp"
#include "tg/cmd/commands/circle_command.hpp"
#include "tg/cmd/commands/erase_command.hpp"
#include "tg/cmd/commands/explode_command.hpp"
#include "tg/cmd/commands/file_commands.hpp"
#include "tg/cmd/commands/layer_commands.hpp"
#include "tg/cmd/commands/line_command.hpp"
#include "tg/cmd/commands/mirror_commands.hpp"
#include "tg/cmd/commands/move_copy_commands.hpp"
#include "tg/cmd/commands/pline_command.hpp"
#include "tg/cmd/commands/property_commands.hpp"
#include "tg/cmd/commands/rotate_scale_commands.hpp"
#include "tg/cmd/commands/text_command.hpp"

#include <memory>

namespace tg::cmd {

void registerBuiltinCommands(CommandRegistry& registry) {
    // Each command with its description (docs/adr/0026), in alphabetical order.
    registry.add(LayerCommand::description(), [] { return std::make_unique<LayerCommand>(); });
    registry.add(LinetypeCommand::description(),
                 [] { return std::make_unique<LinetypeCommand>(); });
    registry.add(detail::arcDescription(), [] { return std::make_unique<ArcCommand>(); });
    registry.add(ScaleCommand::description(ScaleCommand::Variable::Current),
                 [] { return std::make_unique<ScaleCommand>(ScaleCommand::Variable::Current); });
    registry.add(ChpropCommand::description(), [] { return std::make_unique<ChpropCommand>(); });
    registry.add(detail::circleDescription(), [] { return std::make_unique<CircleCommand>(); });
    registry.add(ClayerCommand::description(), [] { return std::make_unique<ClayerCommand>(); });
    registry.add(ColorCommand::description(), [] { return std::make_unique<ColorCommand>(); });
    registry.add(CopyCommand::description(), [] { return std::make_unique<CopyCommand>(); });
    registry.add(detail::eraseDescription(), [] { return std::make_unique<EraseCommand>(); });
    registry.add(ExplodeCommand::description(), [] { return std::make_unique<ExplodeCommand>(); });
    registry.add(detail::insunitsDescription(), [] { return std::make_unique<InsunitsCommand>(); });
    registry.add(detail::lineDescription(), [] { return std::make_unique<LineCommand>(); });
    registry.add(ScaleCommand::description(ScaleCommand::Variable::Global),
                 [] { return std::make_unique<ScaleCommand>(ScaleCommand::Variable::Global); });
    registry.add(LwdisplayCommand::description(),
                 [] { return std::make_unique<LwdisplayCommand>(); });
    registry.add(LweightCommand::description(), [] { return std::make_unique<LweightCommand>(); });
    registry.add(MirrorCommand::description(), [] { return std::make_unique<MirrorCommand>(); });
    registry.add(MirrtextCommand::description(),
                 [] { return std::make_unique<MirrtextCommand>(); });
    registry.add(MoveCommand::description(), [] { return std::make_unique<MoveCommand>(); });
    registry.add(detail::newDescription(), [] { return std::make_unique<NewCommand>(); });
    registry.add(detail::openDescription(), [] { return std::make_unique<OpenCommand>(); });
    registry.add(detail::plineDescription(), [] { return std::make_unique<PlineCommand>(); });
    registry.add(PolarangCommand::description(),
                 [] { return std::make_unique<PolarangCommand>(); });
    registry.add(detail::qsaveDescription(), [] { return std::make_unique<QsaveCommand>(); });
    registry.add(RotateCommand::description(), [] { return std::make_unique<RotateCommand>(); });
    registry.add(detail::saveDescription(), [] { return std::make_unique<SaveCommand>(); });
    registry.add(detail::saveasDescription(), [] { return std::make_unique<SaveasCommand>(); });
    registry.add(ScaleObjectsCommand::description(),
                 [] { return std::make_unique<ScaleObjectsCommand>(); });
    registry.add(detail::textDescription(), [] { return std::make_unique<TextCommand>(); });
}

}  // namespace tg::cmd
