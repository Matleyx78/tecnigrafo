# SPDX-License-Identifier: GPL-3.0-or-later
#
# tg_cmd: interactive commands, command runner, typed coordinates.
# May depend on tg_core, tg_edit and tg_io, never on Qt (docs/adr/0001,
# docs/adr/0005, docs/adr/0017).

add_library(tg_cmd
    src/builtin_commands.cpp
    src/command_descriptions.cpp
    src/commands/arc_command.cpp
    src/commands/chprop_command.cpp
    src/commands/circle_command.cpp
    src/commands/file_commands.cpp
    src/commands/erase_command.cpp
    src/commands/explode_command.cpp
    src/commands/layer_commands.cpp
    src/commands/line_command.cpp
    src/commands/mirror_commands.cpp
    src/commands/move_copy_commands.cpp
    src/commands/pline_command.cpp
    src/commands/property_commands.cpp
    src/commands/rotate_scale_commands.cpp
    src/commands/text_command.cpp
    src/command_registry.cpp
    src/command_runner.cpp
    src/coordinate_input.cpp
    src/drawing_aids.cpp
    src/name_list.cpp
    src/quick_select.cpp
    src/selection_set.cpp
    src/value_format.cpp
)
add_library(Tecnigrafo::cmd ALIAS tg_cmd)

target_include_directories(tg_cmd PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(tg_cmd PUBLIC Tecnigrafo::edit Tecnigrafo::io)
tg_configure_target(tg_cmd)
