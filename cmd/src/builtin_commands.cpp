// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 The Tecnigrafo authors

#include "tg/cmd/builtin_commands.hpp"

#include "command_descriptions.hpp"
#include "tg/cmd/command_registry.hpp"
#include "tg/cmd/commands/aid_commands.hpp"
#include "tg/cmd/commands/arc_command.hpp"
#include "tg/cmd/commands/block_command.hpp"
#include "tg/cmd/commands/chamfer_command.hpp"
#include "tg/cmd/commands/chprop_command.hpp"
#include "tg/cmd/commands/circle_command.hpp"
#include "tg/cmd/commands/dimension_angular_commands.hpp"
#include "tg/cmd/commands/dimension_commands.hpp"
#include "tg/cmd/commands/dimstyle_commands.hpp"
#include "tg/cmd/commands/ellipse_command.hpp"
#include "tg/cmd/commands/erase_command.hpp"
#include "tg/cmd/commands/explode_command.hpp"
#include "tg/cmd/commands/file_commands.hpp"
#include "tg/cmd/commands/fillet_command.hpp"
#include "tg/cmd/commands/hatch_command.hpp"
#include "tg/cmd/commands/insert_command.hpp"
#include "tg/cmd/commands/join_command.hpp"
#include "tg/cmd/commands/layer_commands.hpp"
#include "tg/cmd/commands/line_command.hpp"
#include "tg/cmd/commands/mirror_commands.hpp"
#include "tg/cmd/commands/move_copy_commands.hpp"
#include "tg/cmd/commands/mtext_command.hpp"
#include "tg/cmd/commands/offset_command.hpp"
#include "tg/cmd/commands/pedit_command.hpp"
#include "tg/cmd/commands/pline_command.hpp"
#include "tg/cmd/commands/polygon_command.hpp"
#include "tg/cmd/commands/property_commands.hpp"
#include "tg/cmd/commands/rotate_scale_commands.hpp"
#include "tg/cmd/commands/style_commands.hpp"
#include "tg/cmd/commands/text_command.hpp"
#include "tg/cmd/commands/trim_extend_commands.hpp"

#include <memory>

namespace tg::cmd {

void registerBuiltinCommands(CommandRegistry& registry) {
    // Each command with its description (docs/adr/0026), in alphabetical order.
    registry.add(LayerCommand::description(), [] { return std::make_unique<LayerCommand>(); });
    registry.add(LinetypeCommand::description(),
                 [] { return std::make_unique<LinetypeCommand>(); });
    registry.add(detail::arcDescription(), [] { return std::make_unique<ArcCommand>(); });
    registry.add(BlockCommand::description(), [] { return std::make_unique<BlockCommand>(); });
    registry.add(ScaleCommand::description(ScaleCommand::Variable::Current),
                 [] { return std::make_unique<ScaleCommand>(ScaleCommand::Variable::Current); });
    registry.add(ChamferCommand::description(), [] { return std::make_unique<ChamferCommand>(); });
    registry.add(ChpropCommand::description(), [] { return std::make_unique<ChpropCommand>(); });
    registry.add(detail::circleDescription(), [] { return std::make_unique<CircleCommand>(); });
    registry.add(ClayerCommand::description(), [] { return std::make_unique<ClayerCommand>(); });
    registry.add(ColorCommand::description(), [] { return std::make_unique<ColorCommand>(); });
    registry.add(CopyCommand::description(), [] { return std::make_unique<CopyCommand>(); });
    registry.add(DimensionCommand::description(DimensionCommand::Kind::Aligned), [] {
        return std::make_unique<DimensionCommand>(DimensionCommand::Kind::Aligned);
    });
    registry.add(DimangularCommand::description(),
                 [] { return std::make_unique<DimangularCommand>(); });
    registry.add(RadialDimensionCommand::description(RadialDimensionCommand::Kind::Diameter), [] {
        return std::make_unique<RadialDimensionCommand>(RadialDimensionCommand::Kind::Diameter);
    });
    registry.add(DimensionCommand::description(DimensionCommand::Kind::Linear),
                 [] { return std::make_unique<DimensionCommand>(DimensionCommand::Kind::Linear); });
    registry.add(RadialDimensionCommand::description(RadialDimensionCommand::Kind::Radius), [] {
        return std::make_unique<RadialDimensionCommand>(RadialDimensionCommand::Kind::Radius);
    });
    registry.add(DimstyleCommand::description(),
                 [] { return std::make_unique<DimstyleCommand>(); });
    for (const auto& variable : core::dimVariableInfos()) {
        if (variable.name == "DIMCLRD" || variable.name == "DIMCLRE" ||
            variable.name == "DIMCLRT" || variable.name == "DIMLWD" || variable.name == "DIMLWE") {
            continue;
        }
        registry.add(DimVariableCommand::description(variable.name),
                     [name = std::string(variable.name)] {
                         return std::make_unique<DimVariableCommand>(name);
                     });
    }
    registry.add(EllipseCommand::description(), [] { return std::make_unique<EllipseCommand>(); });
    registry.add(detail::eraseDescription(), [] { return std::make_unique<EraseCommand>(); });
    registry.add(ExplodeCommand::description(), [] { return std::make_unique<ExplodeCommand>(); });
    registry.add(TrimExtendCommand::description(TrimExtendCommand::Action::Extend), [] {
        return std::make_unique<TrimExtendCommand>(TrimExtendCommand::Action::Extend);
    });
    registry.add(FilletCommand::description(), [] { return std::make_unique<FilletCommand>(); });
    registry.add(HatchCommand::description(), [] { return std::make_unique<HatchCommand>(); });
    registry.add(HpislanddetectionCommand::description(),
                 [] { return std::make_unique<HpislanddetectionCommand>(); });
    registry.add(InsertCommand::description(), [] { return std::make_unique<InsertCommand>(); });
    registry.add(detail::insunitsDescription(), [] { return std::make_unique<InsunitsCommand>(); });
    registry.add(JoinCommand::description(), [] { return std::make_unique<JoinCommand>(); });
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
    registry.add(MTextCommand::description(), [] { return std::make_unique<MTextCommand>(); });
    registry.add(detail::newDescription(), [] { return std::make_unique<NewCommand>(); });
    registry.add(OffsetCommand::description(), [] { return std::make_unique<OffsetCommand>(); });
    registry.add(detail::openDescription(), [] { return std::make_unique<OpenCommand>(); });
    registry.add(PeditCommand::description(), [] { return std::make_unique<PeditCommand>(); });
    registry.add(PeditacceptCommand::description(),
                 [] { return std::make_unique<PeditacceptCommand>(); });
    registry.add(detail::plineDescription(), [] { return std::make_unique<PlineCommand>(); });
    registry.add(PolygonCommand::description(), [] { return std::make_unique<PolygonCommand>(); });
    registry.add(PolarangCommand::description(),
                 [] { return std::make_unique<PolarangCommand>(); });
    registry.add(detail::qsaveDescription(), [] { return std::make_unique<QsaveCommand>(); });
    registry.add(RotateCommand::description(), [] { return std::make_unique<RotateCommand>(); });
    registry.add(detail::saveDescription(), [] { return std::make_unique<SaveCommand>(); });
    registry.add(detail::saveasDescription(), [] { return std::make_unique<SaveasCommand>(); });
    registry.add(ScaleObjectsCommand::description(),
                 [] { return std::make_unique<ScaleObjectsCommand>(); });
    registry.add(StyleCommand::description(), [] { return std::make_unique<StyleCommand>(); });
    registry.add(detail::textDescription(), [] { return std::make_unique<TextCommand>(); });
    registry.add(TextstyleCommand::description(),
                 [] { return std::make_unique<TextstyleCommand>(); });
    registry.add(TrimExtendCommand::description(TrimExtendCommand::Action::Trim), [] {
        return std::make_unique<TrimExtendCommand>(TrimExtendCommand::Action::Trim);
    });
    registry.add(TrimextendmodeCommand::description(),
                 [] { return std::make_unique<TrimextendmodeCommand>(); });
}

}  // namespace tg::cmd
